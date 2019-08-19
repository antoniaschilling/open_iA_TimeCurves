/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "dlg_labels.h"

#include "iAImageCoordinate.h"

#include <dlg_commoninput.h>
#include <iAChannelData.h>
#include <iAColorTheme.h>
#include <iAConsole.h>
#include <iALUT.h>
#include <iAModality.h>
#include <iAModalityList.h>
//#include <iARenderer.h>
#include <iAPerformanceHelper.h>
#include <iASlicer.h>
#include <iAToolsVTK.h>
#include <iAvec3.h>
#include <iAVtkDraw.h>
#include <io/iAFileUtils.h>
#include <mdichild.h>

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkMetaImageWriter.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QThread>
#include <QXmlStreamReader>
#include <QSignalMapper>

#include <random>

namespace
{
	const double DefaultOpacity = 0.5;
	typedef int LabelPixelType;
}

dlg_labels::dlg_labels(MdiChild* mdiChild, bool addMainSlicers /* = true*/):
	m_itemModel(new QStandardItemModel()),
	m_colorTheme(iAColorThemeManager::instance().theme("Brewer Set3 (max. 12)")),
	m_mdiChild(mdiChild)
{
	cbColorTheme->addItems(iAColorThemeManager::instance().availableThemes());
	cbColorTheme->setCurrentText(m_colorTheme->name());
	connect(pbAdd, &QPushButton::clicked, this, &dlg_labels::add);
	connect(pbRemove, &QPushButton::clicked, this, &dlg_labels::remove);
	connect(pbStore, &QPushButton::clicked, this, &dlg_labels::storeLabels);
	connect(pbLoad, &QPushButton::clicked, this, &dlg_labels::loadLabels);
	connect(pbStoreImage, &QPushButton::clicked, this, &dlg_labels::storeImage);
	connect(pbSample, &QPushButton::clicked, this, &dlg_labels::sample);
	connect(pbClear, &QPushButton::clicked, this, &dlg_labels::clear);
	connect(cbColorTheme, &QComboBox::currentTextChanged, this, &dlg_labels::colorThemeChanged);
	slOpacity->setValue(DefaultOpacity * slOpacity->maximum());
	connect(slOpacity, &QSlider::valueChanged, this, &dlg_labels::opacityChanged);
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	m_itemModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	lvLabels->setModel(m_itemModel);
}

void dlg_labels::addSlicer(iASlicer *slicer, QSharedPointer<iAModality> modality)
{
	assert(!m_slicers.contains(slicer));

	auto image = modality->image();

	// TODO: necessary...?
	//uint id = nextId();
	//slicer->addChannel(id, iAChannelData(...), true);
	uint id = 0;

	// TODO: name
	auto channelData = iAChannelData("name", image, m_labelColorTF, m_labelOpacityTF);
	slicer->addChannel(id, channelData, true);

	auto labelOverlayImg = vtkSmartPointer<iAvtkImageData>::New();
	labelOverlayImg->SetExtent(image->GetExtent());
	labelOverlayImg->SetSpacing(image->GetSpacing());
	labelOverlayImg->AllocateScalars(VTK_INT, 1);
	clearImage<LabelPixelType>(labelOverlayImg, 0);
	ModalityOverlayImage moi(modality, labelOverlayImg, channelData);

	m_slicers.append(slicer);
	SlicerConnections conns;
	conns.c[0] = connect(slicer, &iASlicer::leftClicked, this, [this, slicer](int x, int y, int z) { slicerClicked(x, y, z, slicer); });
	conns.c[1] = connect(slicer, &iASlicer::leftDragged, this, [this, slicer](int x, int y, int z) { slicerDragged(x, y, z, slicer); });
	conns.c[2] = connect(slicer, &iASlicer::rightClicked, this, [this, slicer](int x, int y, int z) { slicerRightClicked(x, y, z, slicer); });
	m_connections.append(conns);

	m_slicersData.insert(slicer, moi);
}

void dlg_labels::removeSlicer(iASlicer *slicer)
{
	int i = m_slicers.indexOf(slicer);
	if (i != -1) {
		m_slicers.removeAt(i);
		SlicerConnections conns = m_connections[i];
		disconnect(conns.c[0]);
		disconnect(conns.c[1]);
		disconnect(conns.c[2]);
		m_connections.removeAt(i);
		m_slicersData.remove(slicer);
	}
}

namespace
{
	QStandardItem* createCoordinateItem(int x, int y, int z)
	{
		QStandardItem* item = new QStandardItem("(" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + ")");
		item->setData(x, Qt::UserRole + 1);
		item->setData(y, Qt::UserRole + 2);
		item->setData(z, Qt::UserRole + 3);
		return item;
	}
}

void dlg_labels::rendererClicked(int x, int y, int z, iASlicer *slicer)
{
	addSeed(x, y, z, slicer);
}

int findSeed(QStandardItem* labelItem, int x, int y, int z)
{
	for (int i = 0; i<labelItem->rowCount(); ++i)
	{
		QStandardItem* coordItem = labelItem->child(i);
		int ix = coordItem->data(Qt::UserRole + 1).toInt();
		int iy = coordItem->data(Qt::UserRole + 2).toInt();
		int iz = coordItem->data(Qt::UserRole + 3).toInt();
		if (x == ix && y == iy && z == iz)
			return i;
	}
	return -1;
}

bool seedAlreadyExists(QStandardItem* labelItem, int x, int y, int z)
{
	return findSeed(labelItem, x, y, z) != -1;
}

void dlg_labels::addSeed(int cx, int cy, int cz, iASlicer *slicer)
{
	iAVec3i center(cx, cy, cz);
	if (!cbEnableEditing->isChecked())
	{
		return;
	}

	int labelRow = curLabelRow();
	if (labelRow == -1)
	{
		return;
	}

	int radius = spinBox->value() - 1; // -1 because the center voxel shouldn't count
	iATimeGuard timer(QString("Drawing circle of radius %1").arg(radius).toStdString());
	auto extent = m_slicersData.value(slicer).modality->image()->GetExtent();

	auto mode = slicer->mode();
	
	int xAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::X),
		yAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::Y),
		minSlicerX = vtkMath::Max(center[xAxis] - radius, extent[xAxis * 2]),
		maxSlicerX = vtkMath::Min(center[xAxis] + radius, extent[xAxis * 2 + 1]),
		minSlicerY = vtkMath::Max(center[yAxis] - radius, extent[yAxis * 2]),
		maxSlicerY = vtkMath::Min(center[yAxis] + radius, extent[yAxis * 2 + 1]);

	QList<QStandardItem*> items;
	iAVec3i coord(center);
	// compute radius^2, then we don't have to take square root every loop iteration:
	double squareRadius = std::pow(radius, 2);
	for (coord[xAxis] = minSlicerX; coord[xAxis] <= maxSlicerX; ++coord[xAxis])
	{
		for (coord[yAxis] = minSlicerY; coord[yAxis] <= maxSlicerY; ++coord[yAxis])
		{
			int dx = coord[xAxis] - center[xAxis];
			int dy = coord[yAxis] - center[yAxis];
			double squareDis = dx*dx + dy*dy;
			if (squareDis <= squareRadius)
			{
				auto item = addSeedItem(labelRow, coord[0], coord[1], coord[2], slicer);
				if (item)
					items.append(item);
			}
		}
	}
	if (items.size() == 0)
		return;
	appendSeeds(labelRow, items);
	updateChannel(slicer);
}

void dlg_labels::slicerClicked(int x, int y, int z, iASlicer *slicer)
{
	addSeed(x, y, z, slicer);
}

void dlg_labels::slicerDragged(int x, int y, int z, iASlicer *slicer)
{
	addSeed(x, y, z, slicer);
}

void dlg_labels::slicerRightClicked(int x, int y, int z, iASlicer *slicer)
{
	for (int l = 0; l < m_itemModel->rowCount(); ++l)
	{
		int idx = findSeed(m_itemModel->item(l), x, y, z);
		if (idx != -1)
		{
			removeSeed(m_itemModel->item(l)->child(idx), x, y, z, slicer);
			updateChannel(slicer);
			break;
		}
	}
}

QStandardItem* dlg_labels::addSeedItem(int labelRow, int x, int y, int z, iASlicer* slicer)
{
	// make sure we're not adding the same seed twice:
	for (int l = 0; l < m_itemModel->rowCount(); ++l)
	{
		auto item = m_itemModel->item(l);
		int childIdx = findSeed(item, x, y, z);
		if (childIdx != -1)
		{
			if (l == labelRow)
				return nullptr; // seed already exists with same label, nothing to do
			else
				// seed already exists with different label; need to remove other
				// TODO: do that removal somehow "in batch" too?
				removeSeed(item->child(childIdx), x, y, z, slicer);
		}
	}
	drawPixel<LabelPixelType>(m_slicersData.value(slicer).modality->image(), x, y, z, labelRow + 1);
	return createCoordinateItem(x, y, z);
}

void dlg_labels::appendSeeds(int label, QList<QStandardItem*> const & items)
{
	m_itemModel->item(label)->appendRows(items);
	m_itemModel->item(label, 1)->setText(QString::number(m_itemModel->item(label)->rowCount()));
}

int dlg_labels::addLabelItem(QString const & labelText)
{
	QStandardItem* newItem = new QStandardItem(labelText);
	QStandardItem* newItemCount = new QStandardItem("0");
	newItem->setData(m_colorTheme->color(m_itemModel->rowCount()), Qt::DecorationRole);
	QList<QStandardItem* > newRow;
	newRow.append(newItem);
	newRow.append(newItemCount);
	m_itemModel->appendRow(newRow);
	return newItem->row();
}

void dlg_labels::add()
{
	pbStore->setEnabled(true);
	addLabelItem(QString::number( m_itemModel->rowCount() ));
	reInitChannelTF();
}

void dlg_labels::reInitChannelTF()
{
	m_labelColorTF = iALUT::BuildLabelColorTF(m_itemModel->rowCount(), m_colorTheme);
	m_labelOpacityTF = iALUT::BuildLabelOpacityTF(m_itemModel->rowCount());
}

void dlg_labels::recolorItems()
{
	for (int row = 0; row < m_itemModel->rowCount(); ++row)
		m_itemModel->item(row)->setData(m_colorTheme->color(row), Qt::DecorationRole);
}

void dlg_labels::updateChannel(iASlicer *slicer)
{
	auto img = m_slicersData.value(slicer).overlayImage;
	img->Modified();
	img->SetScalarRange(0, m_itemModel->rowCount());
	//m_mdiChild->updateChannel(m_labelChannelID, m_labelOverlayImg, m_labelColorTF, m_labelOpacityTF, true);
	//m_mdiChild->updateViews();
	for (auto slicer : m_slicers) {
		// TODO
		slicer->updateChannel(0, m_slicersData.value(slicer).channelData);
		//slicer->updateChannelMappers // TODO: call this on color pallet changed?
		slicer->update();
	}
}

void dlg_labels::remove()
{
	QModelIndexList indices = lvLabels->selectionModel()->selectedIndexes();
	if (indices.size() == 0)
		return;
	QStandardItem* item = m_itemModel->itemFromIndex(indices[0]);
	if (!item)
		return;
	bool updateOverlay = true;
	if (item->parent() == nullptr)  // a label was selected
	{
		if (item->rowCount() > 0)
		{
			auto reply = QMessageBox::question(this, "Remove Label",
				QString("Are you sure you want to remove the whole label and all of its %1 seeds?").arg(item->rowCount()),
				QMessageBox::Yes | QMessageBox::No);
			if (reply != QMessageBox::Yes) {
				return;
			}
		}
		int curLabel = curLabelRow();
		if (curLabel == -1)
		{
			return;
		}
		for (int s = item->rowCount()-1; s >= 0; --s)
		{
			auto seed = item->child(s);
			int x = seed->data(Qt::UserRole + 1).toInt();
			int y = seed->data(Qt::UserRole + 2).toInt();
			int z = seed->data(Qt::UserRole + 3).toInt();
			for (ModalityOverlayImage moi : m_slicersData.values()) {
				drawPixel<LabelPixelType>(moi.overlayImage, x, y, z, 0);
			}
		}
		m_itemModel->removeRow(curLabel);
		for (int l = curLabel; l < m_itemModel->rowCount(); ++l)
		{
			auto itemAfter = m_itemModel->item(l);
			itemAfter->setText(QString::number(l));
			for (int s = itemAfter->rowCount() - 1; s >= 0; --s)
			{
				auto seed = itemAfter->child(s);
				int x = seed->data(Qt::UserRole + 1).toInt();
				int y = seed->data(Qt::UserRole + 2).toInt();
				int z = seed->data(Qt::UserRole + 3).toInt();
				for (ModalityOverlayImage moi : m_slicersData.values()) {
					drawPixel<LabelPixelType>(moi.overlayImage, x, y, z, l + 1);
				}
			}
		}
		if (m_itemModel->rowCount() == 0)
		{
			pbStore->setEnabled(false);
		}
		recolorItems();
		for (auto slicer : m_slicers) {
			updateChannel(slicer);
		}
	}
	else
	{	// remove a single seed
		int x = item->data(Qt::UserRole + 1).toInt();
		int y = item->data(Qt::UserRole + 2).toInt();
		int z = item->data(Qt::UserRole + 3).toInt();
		for (auto slicer : m_slicers) {
			removeSeed(item, x, y, z, slicer);
			updateChannel(slicer);
		}
	}
}

void dlg_labels::removeSeed(QStandardItem* item, int x, int y, int z, iASlicer *slicer)
{
	drawPixel<LabelPixelType>(m_slicersData.value(slicer).overlayImage, x, y, z, 0);
	int labelRow = item->parent()->row();
	item->parent()->removeRow(item->row());
	m_itemModel->item(labelRow, 1)->setText(QString::number(m_itemModel->item(labelRow, 1)->text().toInt() - 1));
}

int dlg_labels::curLabelRow() const
{
	QModelIndexList indices = lvLabels->selectionModel()->selectedIndexes();
	if (indices.size() <= 0)
	{
		return -1;
	}
	QStandardItem* item = m_itemModel->itemFromIndex(indices[0]);
	while (item->parent() != nullptr)
	{
		item = item->parent();
	}
	return item->row();
}

int dlg_labels::seedCount(int labelIdx) const
{
	QStandardItem* labelItem = m_itemModel->item(labelIdx);
	return labelItem->rowCount();
}

// Load
/*bool dlg_labels::load(QString const & filename)
{
	if (m_labelOverlayImg)
	{
		clearImage<LabelPixelType>(m_labelOverlayImg, 0);
	}
	m_itemModel->clear();
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	m_itemModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{	
		DEBUG_LOG(QString("Seed file loading: Failed to open file '%1'!").arg(filename));
		return false;
	}
	QXmlStreamReader stream(&file);
	stream.readNext();
	int curLabelRow = -1;
	
	bool enableStoreBtn = false;
	QList<QStandardItem*> items;
	while (!stream.atEnd())
	{
		if (stream.isStartElement())
		{
			if (stream.name() == "Labels")
			{
				// root element, no action required
			}
			else if (stream.name() == "Label")
			{
				if (items.size() > 0)
				{
					if (curLabelRow == -1)
					{
						DEBUG_LOG(QString("Error loading seed file '%1': Current label not set!")
							.arg(filename));
					}
					appendSeeds(curLabelRow, items);
				}

				enableStoreBtn = true;
				QString id = stream.attributes().value("id").toString();
				QString name = stream.attributes().value("name").toString();
				curLabelRow = addLabelItem(name);
				items.clear();
				if (m_itemModel->rowCount()-1 != id.toInt())
				{
					DEBUG_LOG(QString("Inserting row: rowCount %1 <-> label id %2 mismatch!")
						.arg(m_itemModel->rowCount())
						.arg(id) );
				}
			}
			else if (stream.name() == "Seed")
			{
				if (curLabelRow == -1)
				{
					DEBUG_LOG(QString("Error loading seed file '%1': Current label not set!")
						.arg(filename) );
					return false;
				}
				int x = stream.attributes().value("x").toInt();
				int y = stream.attributes().value("y").toInt();
				int z = stream.attributes().value("z").toInt();
				auto item = addSeedItem(curLabelRow, x, y, z);
				if (item)
					items.append(item);
				else
					DEBUG_LOG(QString("Item %1, %2, %3, label %4 already exists!")
						.arg(x).arg(y).arg(z).arg(curLabelRow));
			}
		}
		stream.readNext();
	}

	file.close();
	if (stream.hasError())
	{
	   DEBUG_LOG(QString("Error: Failed to parse seed xml file '%1': %2")
		   .arg(filename)
		   .arg(stream.errorString()) );
	   return false;
	}
	else if (file.error() != QFile::NoError)
	{
		DEBUG_LOG(QString("Error: Cannot read file '%1': %2")
			.arg(filename )
			.arg(file.errorString()) );
	   return false;
	}

	QFileInfo fileInfo(file);
	m_fileName = MakeAbsolute(fileInfo.absolutePath(), filename);
	pbStore->setEnabled(enableStoreBtn);
	reInitChannelTF();
	updateChannel();
	return true;
}*/

// Store
/*bool dlg_labels::store(QString const & filename, bool extendedFormat)
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{	
		QMessageBox::warning(this, "GEMSe", "Seed file storing: Failed to open file '" + filename + "'!");
		return false;
	}
	QFileInfo fileInfo(file);
	m_fileName = MakeAbsolute(fileInfo.absolutePath(), filename);
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Labels");

	auto modalities = m_mdiChild->modalities();
	for (int l=0; l<m_itemModel->rowCount(); ++l)
	{
		QStandardItem * labelItem = m_itemModel->item(l);
		stream.writeStartElement("Label");
		stream.writeAttribute("id", QString::number(l));
		stream.writeAttribute("name", labelItem->text());
		for (int i=0; i<labelItem->rowCount(); ++i)
		{
			stream.writeStartElement("Seed");
			QStandardItem* coordItem = labelItem->child(i);
			int x = coordItem->data(Qt::UserRole + 1).toInt();
			int y = coordItem->data(Qt::UserRole + 2).toInt();
			int z = coordItem->data(Qt::UserRole + 3).toInt();
			stream.writeAttribute("x", QString::number(x));
			stream.writeAttribute("y", QString::number(y));
			stream.writeAttribute("z", QString::number(z));
			if (extendedFormat)
			{
				for (int m = 0; m < modalities->size(); ++m)
				{
					auto mod = modalities->get(m);
					for (int c = 0; c < mod->componentCount(); ++c)
					{
						double value = mod->component(c)->GetScalarComponentAsDouble(x, y, z, 0);
						stream.writeStartElement("Value");
						stream.writeAttribute("modality", QString::number(m));
						stream.writeAttribute("component", QString::number(c));
						stream.writeAttribute("value", QString::number(value, 'g', 16));
						stream.writeEndElement();
					}
				}
			}
			stream.writeEndElement();
		}
		stream.writeEndElement();
	}
	stream.writeEndElement();
	stream.writeEndDocument();
	file.close();
	return true;
}*/

// Load labels
/*void dlg_labels::loadLabels()
{
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open Seed File"),
		QString() // TODO get directory of current file
		,
		tr("Seed file (*.seed);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	if (!load(fileName))
	{
		QMessageBox::warning(this, "GEMSe", "Loading seed file '" + fileName + "' failed!");
	}
}*/

// Store labels
/*void dlg_labels::storeLabels()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save Seed File"),
		QString() // TODO get directory of current file
		,
		tr("Seed file (*.seed);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	QStringList inList;
	inList << tr("$Extended Format (also write pixel values, not only positions)");
	QList<QVariant> inPara;
	inPara << tr("%1").arg(true);
	dlg_commoninput extendedFormatInput(this, "Seed File Format", inList, inPara, nullptr);
	if (extendedFormatInput.exec() != QDialog::Accepted)
	{
		DEBUG_LOG("Selection of format aborted, aborting seed file storing");
		return;
	}
	if (!store(fileName, extendedFormatInput.getCheckValue(0)))
	{
		QMessageBox::warning(this, "GEMSe", "Storing seed file '" + fileName + "' failed!");
	}
}*/

// Store image
/*void dlg_labels::storeImage()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save Seeds as Image"),
		QString() // TODO get directory of current file
		,
		tr("Seed file (*.mhd);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	vtkMetaImageWriter *metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName( getLocalEncodingFileName(fileName).c_str() );
	metaImageWriter->SetInputData(m_labelOverlayImg);
	metaImageWriter->SetCompression( false );
	metaImageWriter->Write();
	metaImageWriter->Delete();
}*/

bool haveAllSeeds(QVector<int> const & label2SeedCounts, std::vector<int> const & requiredNumOfSeedsPerLabel)
{
	for (int i=0; i<label2SeedCounts.size(); ++i)
	{
		if (label2SeedCounts[i] < requiredNumOfSeedsPerLabel[i])
		{
			return false;
		}
	}
	return true;
}

// Sample
/*void dlg_labels::sample()
{
	int gt = m_mdiChild->chooseModalityNr("Choose Ground Truth");
	vtkSmartPointer<vtkImageData> img = m_mdiChild->modality(gt)->image();
	int labelCount = img->GetScalarRange()[1]+1;
	if (labelCount > 50)
	{
		auto reply = QMessageBox::question(this, "Sample Seeds",
			QString("According to min/max values in image, there are %1 labels, this seems a bit much. Do you really want to proceed?").arg(labelCount),
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes) {
			return;
		}
	}

	QStringList     inParams; inParams
		<< "*Number of Seeds per Label"
		<< "$Reduce seed number for all labels to size of smallest labeling";
	QList<QVariant> inValues; inValues
		<< 50
		<< true;
	dlg_commoninput input(this, "Sample Seeds", inParams, inValues, nullptr);
	if (input.exec() != QDialog::Accepted)
	{
		return;
	}
	int numOfSeeds = input.getIntValue(0);
	bool reduceNum = input.getCheckValue(1);

	std::vector<int> numOfSeedsPerLabel(labelCount, numOfSeeds);
	// check that there is at least numOfSeedsPerLabel pixels per label
	std::vector<int> histogram(labelCount, 0);
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		int label = static_cast<int>(img->GetScalarComponentAsFloat(x, y, z, 0));
		histogram[label]++;
	}
	int minNumOfSeeds = numOfSeeds;
	for (int i = 0; i < labelCount; ++i)
	{
		if ((histogram[i] * 3 / 4) < numOfSeedsPerLabel[i])
		{
			numOfSeedsPerLabel[i] = histogram[i] * 3 / 4;
			DEBUG_LOG(QString("Reducing number of seeds for label %1 to %2 because it only has %3 pixels")
				.arg(i).arg(numOfSeedsPerLabel[i]).arg(histogram[i]));
		}
		if (numOfSeedsPerLabel[i] < minNumOfSeeds)
		{
			minNumOfSeeds = numOfSeedsPerLabel[i];
		}
		if (m_itemModel->rowCount() <= i)
		{
			addLabelItem(QString::number(i));
		}
	}
	if (reduceNum)
	{
		for (int i = 0; i < labelCount; ++i)
		{
			numOfSeedsPerLabel[i] = minNumOfSeeds;
		}
	}
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> xDist(0, img->GetDimensions()[0] - 1);
	std::uniform_int_distribution<> yDist(0, img->GetDimensions()[1] - 1);
	std::uniform_int_distribution<> zDist(0, img->GetDimensions()[2] - 1);

	QVector<int> label2SeedCount(labelCount, 0);
	QVector<QList<QStandardItem*> > items(labelCount);
	while (!haveAllSeeds(label2SeedCount, numOfSeedsPerLabel))
	{
		int x = xDist(gen);
		int y = yDist(gen);
		int z = zDist(gen);
		int label = static_cast<int>(img->GetScalarComponentAsFloat(x, y, z, 0));

		if (label2SeedCount[label] < numOfSeedsPerLabel[label] && !seedAlreadyExists(m_itemModel->item(label), x, y, z))
		{
			// m_itemModel->item(label)->appendRow(GetCoordinateItem(x, y, z));
			auto item = addSeedItem(label, x, y, z);
			if (item)
			{
				label2SeedCount[label]++;
				items[label].append(item);
			}
		}
	}
	for (int l = 0; l < labelCount; ++l)
	{
		appendSeeds(l, items[l]);
	}
	reInitChannelTF();
	updateChannel();
	pbStore->setEnabled(true);
}*/

void dlg_labels::clear()
{
	for (auto slicer : m_slicers) {
		auto img = m_slicersData.value(slicer).overlayImage;
		if (img)
		{
			clearImage<LabelPixelType>(img, 0);
			updateChannel(slicer);
		}
	}
	m_itemModel->clear();
}

QString const & dlg_labels::fileName()
{
	return m_fileName;
}

void dlg_labels::colorThemeChanged(QString const & newThemeName)
{
	m_colorTheme = iAColorThemeManager::instance().theme(newThemeName);
	reInitChannelTF();
	recolorItems();
	for (auto slicer : m_slicers) {
		if (m_slicersData.value(slicer).overlayImage)
			updateChannel(slicer);
	}
}

void dlg_labels::opacityChanged(int newValue)
{
	double opacity = static_cast<double>(newValue) / slOpacity->maximum();
	//m_mdiChild->updateChannelOpacity(m_labelChannelID, opacity);
	for (auto slicer : m_slicers) {
		//slicer->updatechannelOpacity // Method does not exist! TODO: what to do?
	}
}
