/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "mdichild.h"

#include "dlg_slicer.h"
#include "dlg_volumePlayer.h"
#include "iADataSetViewer.h"
#include "iADataSetRenderer.h"
#include "iAVolumeViewer.h"    // TODO NEWIO: remove this from here
#include "iAFileParamDlg.h"
#include "iAFileUtils.h"    // for safeFileName
#include "iAParametricSpline.h"
#include "iAProfileProbe.h"
#include "iAvtkInteractStyleActor.h"
#include "mainwindow.h"

// renderer
#include <iARendererImpl.h>
#include <iARenderObserver.h>

// slicer
#include <iASlicerImpl.h>

// guibase
#include <iAAlgorithm.h>
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iADataSetListWidget.h>
#include <iAJobListView.h>
#include <iAMovieHelper.h>
#include <iAParameterDlg.h>
#include <iAPreferences.h>
#include <iATool.h>
#include <iAToolRegistry.h>
#include <iARenderSettings.h>
#include <iARunAsync.h>
#include <iAVolumeStack.h>

// qthelper
#include <iADockWidgetWrapper.h>

// charts
#include <iAPlotTypes.h>
#include <iAProfileWidget.h>

// io
#include <iAFileStackParams.h>
#include <iAImageStackFileIO.h>
#include <iAProjectFileIO.h>
#include <iAVolStackFileIO.h>

// base
#include <iADataSet.h>
#include <iAFileTypeRegistry.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iAStringHelper.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCornerAnnotation.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>

// TODO: refactor methods using the following out of mdichild!
#include <vtkTransform.h>

#include <QByteArray>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QSpinBox>
#include <QtGlobal> // for QT_VERSION


MdiChild::MdiChild(MainWindow* mainWnd, iAPreferences const& prefs, bool unsavedChanges) :
	m_mainWnd(mainWnd),
	m_preferences(prefs),
	m_isSmthMaximized(false),
	m_isUntitled(true),
	m_isSliceProfileEnabled(false),
	m_profileHandlesEnabled(false),
	m_isMagicLensEnabled(false),
	m_snakeSlicer(false),
	m_worldSnakePoints(vtkSmartPointer<vtkPoints>::New()),
	m_parametricSpline(vtkSmartPointer<iAParametricSpline>::New()),
	m_polyData(vtkPolyData::New()),
	m_axesTransform(vtkTransform::New()),
	m_slicerTransform(vtkTransform::New()),
	m_volumeStack(new iAVolumeStack),
	m_dataSetInfo(new QListWidget(this)),
	m_dwInfo(new iADockWidgetWrapper(m_dataSetInfo, "Dataset Info", "DataInfo")),
	m_dataSetListWidget(new iADataSetListWidget()),
	m_dwDataSets(new iADockWidgetWrapper(m_dataSetListWidget, "Datasets", "DataSets")),
	m_dwVolumePlayer(nullptr),
	m_nextChannelID(0),
	m_magicLensChannel(NotExistingChannel),
	m_magicLensDataSet(0),
	m_initVolumeRenderers(false),
	m_interactionMode(imCamera),
	m_nextDataSetID(0)
{
	std::fill(m_slicerVisibility, m_slicerVisibility + 3, false);
	setAttribute(Qt::WA_DeleteOnClose);
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
	setWindowModified(unsavedChanges);
	setupUi(this);
	//prepare window for handling dock widgets
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	//insert default dock widgets and arrange them in a simple layout
	m_dwRenderer = new dlg_renderer(this);
	for (int i = 0; i < 3; ++i)
	{
		m_slicer[i] = new iASlicerImpl(this, static_cast<iASlicerMode>(i), true, true, m_slicerTransform, m_worldSnakePoints);
		m_dwSlicer[i] = new dlg_slicer(m_slicer[i]);
	}

	addDockWidget(Qt::LeftDockWidgetArea, m_dwRenderer);
	m_initialLayoutState = saveState();

	splitDockWidget(m_dwRenderer, m_dwSlicer[iASlicerMode::XY], Qt::Horizontal);
	splitDockWidget(m_dwSlicer[iASlicerMode::XY], m_dwSlicer[iASlicerMode::XZ], Qt::Vertical);
	splitDockWidget(m_dwSlicer[iASlicerMode::XZ], m_dwSlicer[iASlicerMode::YZ], Qt::Vertical);
	splitDockWidget(m_dwRenderer, m_dwDataSets, Qt::Vertical);
	splitDockWidget(m_dwDataSets, m_dwInfo, Qt::Horizontal);

	m_parametricSpline->SetPoints(m_worldSnakePoints);

	m_renderer = new iARendererImpl(this, dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_dwRenderer->vtkWidgetRC->renderWindow()));
	connect(m_renderer, &iARendererImpl::bgColorChanged, m_dwRenderer->vtkWidgetRC, &iAAbstractMagicLensWidget::setLensBackground);
	connect(m_renderer, &iARendererImpl::interactionModeChanged, this, [this](bool camera) {
		setInteractionMode(camera ? imCamera : imRegistration);
		});
	m_renderer->setAxesTransform(m_axesTransform);

	applyViewerPreferences();
	connectSignalsToSlots();
	connect(mainWnd, &MainWindow::fullScreenToggled, this, &MdiChild::toggleFullScreen);
	connect(mainWnd, &MainWindow::styleChanged, this, &MdiChild::styleChanged);

	// Dataset list events:
	connect(m_dataSetListWidget, &iADataSetListWidget::removeDataSet, this, &iAMdiChild::removeDataSet);
	connect(m_dataSetListWidget, &iADataSetListWidget::dataSetSelected, this, &iAMdiChild::dataSetSelected);

	for (int i = 0; i <= iASlicerMode::SlicerCount; ++i)
	{
		m_manualMoveStyle[i] = vtkSmartPointer<iAvtkInteractStyleActor>::New();
		connect(m_manualMoveStyle[i].Get(), &iAvtkInteractStyleActor::actorsUpdated, this, &iAMdiChild::updateViews);
	}
}

void MdiChild::initializeViews()
{
	// avoid 3D renderer being resized to very small (resulting from splitDockWidget)
	float h = geometry().height();
	resizeDocks(QList<QDockWidget*>{ m_dwRenderer, m_dwDataSets, m_dwInfo },
		QList<int>{ static_cast<int>(0.7 * h), static_cast<int>(0.2 * h), static_cast<int>(0.2 * h) }, Qt::Vertical);
}

void MdiChild::toggleFullScreen()
{
	QWidget* mdiSubWin = qobject_cast<QWidget*>(parent());
	if (m_mainWnd->isFullScreen())
	{
		mdiSubWin->setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	}
	else
	{
		mdiSubWin->setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
	}
	mdiSubWin->show();
}

MdiChild::~MdiChild()
{
	cleanWorkingAlgorithms();
	m_polyData->ReleaseData();
	m_axesTransform->Delete();
	m_slicerTransform->Delete();

	m_polyData->Delete();

	// should be deleted automatically by Qt's mechanism of parents deleting their children
	//for (int s = 0; s < 3; ++s)
	//{
	//	delete m_slicer[s];
	//}
	//delete m_renderer; m_renderer = nullptr;
}

void MdiChild::slicerVisibilityChanged(int mode)
{
	if (m_dwSlicer[mode]->isVisible() == m_slicerVisibility[mode])
	{
		return; // no change
	}
	m_slicerVisibility[mode] = m_dwSlicer[mode]->isVisible();
	if (m_renderSettings.ShowSlicePlanes)
	{
		m_renderer->showSlicePlane(mode, m_slicerVisibility[mode]);
		m_renderer->update();
	}
}

void MdiChild::connectSignalsToSlots()
{
	for (int mode = 0; mode < iASlicerMode::SlicerCount; ++mode)
	{
		connect(m_dwSlicer[mode]->pbMax, &QPushButton::clicked, [this, mode] { maximizeSlicer(mode); });
		connect(m_dwSlicer[mode], &QDockWidget::visibilityChanged, [this, mode]{ slicerVisibilityChanged(mode); });
	}

	connect(m_dwRenderer->pushMaxRC, &QPushButton::clicked, this, &MdiChild::maximizeRC);
	connect(m_dwRenderer->pushStopRC, &QPushButton::clicked, this, &MdiChild::toggleRendererInteraction);

	connect(m_dwRenderer->pushPX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PX); });
	connect(m_dwRenderer->pushPY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PY); });
	connect(m_dwRenderer->pushPZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PZ); });
	connect(m_dwRenderer->pushMX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MX); });
	connect(m_dwRenderer->pushMY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MY); });
	connect(m_dwRenderer->pushMZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MZ); });
	connect(m_dwRenderer->pushIso, &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::Iso); });
	connect(m_dwRenderer->pushSaveRC, &QPushButton::clicked, this, &MdiChild::saveRC);
	connect(m_dwRenderer->pushMovRC, &QPushButton::clicked, this, &MdiChild::saveMovRC);

	// TODO: move to renderer library?
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARendererImpl::mouseRightButtonReleasedSlot);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARendererImpl::mouseLeftButtonReleasedSlot);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::touchStart, m_renderer, &iARendererImpl::touchStart);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::touchScale, m_renderer, &iARendererImpl::touchScaleSlot);

	for (int s = 0; s < 3; ++s)
	{
		connect(m_slicer[s], &iASlicer::shiftMouseWheel, this, &MdiChild::changeMagicLensDataSet);
		connect(m_slicer[s], &iASlicer::altMouseWheel, this, &MdiChild::changeMagicLensOpacity);
		connect(m_slicer[s], &iASlicer::ctrlMouseWheel, this, &MdiChild::changeMagicLensSize);
		connect(m_slicer[s], &iASlicerImpl::sliceRotated, this, &MdiChild::slicerRotationChanged);
		connect(m_slicer[s], &iASlicer::sliceNumberChanged, this, &MdiChild::setSlice);
		connect(m_slicer[s], &iASlicer::mouseMoved, this, &MdiChild::updatePositionMarker);
		connect(m_slicer[s], &iASlicerImpl::regionSelected, this, [this](int minVal, int maxVal, uint channelID)
		{
			// TODO NEWIO: move to better place, e.g. dataset viewer for image data? slicer?
			if (minVal == maxVal)
			{
				return;
			}
			for (auto viewer: m_dataSetViewers)
			{
				viewer.second->slicerRegionSelected(minVal, maxVal, channelID);
			}
			updateViews();
		});
	}
}

void MdiChild::connectThreadSignalsToChildSlots(iAAlgorithm* thread)
{
	// TODO NEWIO: update mechanism to bring in output of "old style" algorithm results
	//connect(thread, &iAAlgorithm::startUpdate, this, &MdiChild::updateRenderWindows);
	//connect(thread, &iAAlgorithm::finished, this, &MdiChild::enableRenderWindows);
	connectAlgorithmSignalsToChildSlots(thread);
}

void MdiChild::connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread)
{
	addAlgorithm(thread);
}

void MdiChild::addAlgorithm(iAAlgorithm* thread)
{
	m_workingAlgorithms.push_back(thread);
	connect(thread, &iAAlgorithm::finished, this, &MdiChild::removeFinishedAlgorithms);
}

void MdiChild::updatePositionMarker(double x, double y, double z, int mode)
{
	double pos[3] = { x, y, z };
	if (m_renderSettings.ShowRPosition)
	{
		m_renderer->setPositionMarkerCenter(x, y, z);
	}
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (mode == i)  // only update other slicers
		{
			continue;
		}
		if (m_slicerSettings.LinkViews && m_slicer[i]->hasChannel(0))  // TODO: check for whether dataset is shown in slicer?
		{   // TODO: set "slice" based on world coordinates directly instead of spacing?
			double spacing = m_slicer[i]->channel(0)->input()->GetSpacing()[i];
			m_dwSlicer[i]->sbSlice->setValue(pos[mapSliceToGlobalAxis(i, iAAxisIndex::Z)] / spacing);
		}
		if (m_slicerSettings.SingleSlicer.ShowPosition)
		{
			int slicerXAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::X);
			int slicerYAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::Y);
			int slicerZAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::Z);
			m_slicer[i]->setPositionMarkerCenter(
				pos[slicerXAxisIdx],
				pos[slicerYAxisIdx],
				pos[slicerZAxisIdx]);
		}
	}
}

size_t MdiChild::addDataSet(std::shared_ptr<iADataSet> dataSet)
{
	size_t dataSetIdx;
	{
		QMutexLocker locker(&m_dataSetMutex);
		dataSetIdx = m_nextDataSetID;
		++m_nextDataSetID;
	}
	m_dataSets[dataSetIdx] = dataSet;
	if (m_curFile.isEmpty())
	{
		LOG(lvlDebug, "Developer Warning - consider calling setWindowTitleAndFile directly where you first call addDataSet");
		setWindowTitleAndFile(
			dataSet->hasMetaData(iADataSet::FileNameKey) ?
			dataSet->metaData(iADataSet::FileNameKey).toString() :
			dataSet->name());
	}
	auto p = std::make_shared<iAProgress>();
	auto viewer = createDataSetViewer(dataSet.get());
	connect(viewer.get(), &iADataSetViewer::dataSetChanged, this, [this, dataSetIdx] {
		updateDataSetInfo();
		emit dataSetChanged(dataSetIdx);
	});
	m_dataSetViewers[dataSetIdx] = viewer;
	auto fw = runAsync(
		[this, viewer, dataSetIdx, p]()
		{
			viewer->prepare(m_preferences, p.get());
			emit dataSetPrepared(dataSetIdx);
		},
		[this, viewer, dataSetIdx]
		{
			viewer->createGUI(this, dataSetIdx);
			emit dataSetRendered(dataSetIdx);
		},
		this);
	iAJobListView::get()->addJob(QString("Computing display data for %1").arg(dataSet->name()), p.get(), fw);
	return dataSetIdx;
}

void MdiChild::removeDataSet(size_t dataSetIdx)
{
	assert(m_dataSetViewers.find(dataSetIdx) != m_dataSetViewers.end());
	m_dataSetViewers.erase(dataSetIdx);
	m_dataSets.erase(dataSetIdx);
	updateViews();
	if (m_isMagicLensEnabled && m_magicLensDataSet == dataSetIdx)
	{
		changeMagicLensDataSet(0);
	}
	updateDataSetInfo();
	emit dataSetRemoved(dataSetIdx);
}

void MdiChild::clearDataSets()
{
	std::vector<size_t> dataSetKeys;
	dataSetKeys.reserve(m_dataSets.size());
	std::transform(m_dataSets.begin(), m_dataSets.end(), std::back_inserter(dataSetKeys), [](auto const& p) { return p.first; });
	for (auto dataSetIdx : dataSetKeys)
	{
		removeDataSet(dataSetIdx);
	}
}

namespace
{
	const QString CameraPositionKey("CameraPosition");
	const QString CameraFocalPointKey("CameraFocalPoint");
	const QString CameraViewUpKey("CameraViewUp");
	const QString CameraParallelProjection("CameraParallelProjection");
	const QString CameraParallelScale("CameraParallelScale");
}

vtkPolyData* MdiChild::polyData()
{
	return m_polyData;
}

iARenderer* MdiChild::renderer()
{
	return m_renderer;
}

void MdiChild::updateVolumePlayerView(int updateIndex, bool isApplyForAll)
{
	// TODO NEWIO: REDO by simply showing the new dataset and hiding the old one; MOVE to volume stack widget
	
	// TODO: VOLUME: Test!!! copy from currently selected instead of fixed 0 index?
	// This function probbl never called, update(int, bool) signal doesn't seem to be emitted anywhere?
	/*
	vtkColorTransferFunction* colorTransferFunction = modality(0)->transfer()->colorTF();
	vtkPiecewiseFunction* piecewiseFunction = modality(0)->transfer()->opacityTF();
	m_volumeStack->colorTF(m_previousIndexOfVolume)->DeepCopy(colorTransferFunction);
	m_volumeStack->opacityTF(m_previousIndexOfVolume)->DeepCopy(piecewiseFunction);
	m_previousIndexOfVolume = updateIndex;

	m_imageData->DeepCopy(m_volumeStack->volume(updateIndex));
	assert(m_volumeStack->numberOfVolumes() < std::numeric_limits<int>::max());
	if (isApplyForAll)
	{
		for (size_t i = 0; i < m_volumeStack->numberOfVolumes(); ++i)
		{
			if (static_cast<int>(i) != updateIndex)
			{
				m_volumeStack->colorTF(i)->DeepCopy(colorTransferFunction);
				m_volumeStack->opacityTF(i)->DeepCopy(piecewiseFunction);
			}
		}
	}

	colorTransferFunction->DeepCopy(m_volumeStack->colorTF(updateIndex));
	piecewiseFunction->DeepCopy(m_volumeStack->opacityTF(updateIndex));

	//setHistogramModality(0);

	m_renderer->reInitialize(m_imageData, m_polyData);
	for (int s = 0; s < 3; ++s)
	{
		// TODO: check how to update s:
		m_slicer[s]->updateChannel(0, iAChannelData(modality(0)->name(), m_imageData, colorTransferFunction));
	}
	updateViews();

	if (m_checkedList.at(updateIndex) != 0)
	{
		enableRenderWindows();
	}

	return true;
	*/
}

void MdiChild::setupStackView(bool active)
{
	addVolumePlayer();
	// TODO NEWIO: check / REWRITE

	/*
	m_previousIndexOfVolume = 0;
	if (m_volumeStack->numberOfVolumes() == 0)
	{
		LOG(lvlError, "Invalid call to setupStackView: No Volumes loaded!");
		return;
	}

	int currentIndexOfVolume = 0;
	m_imageData->DeepCopy(m_volumeStack->volume(currentIndexOfVolume));
	setupViewInternal(active);
	for (size_t i = 0; i < m_volumeStack->numberOfVolumes(); ++i)
	{
		vtkSmartPointer<vtkColorTransferFunction> cTF = defaultColorTF(m_imageData->GetScalarRange());
		vtkSmartPointer<vtkPiecewiseFunction> pWF = defaultOpacityTF(m_imageData->GetScalarRange(), m_imageData->GetNumberOfScalarComponents() == 1);
		m_volumeStack->addColorTransferFunction(cTF);
		m_volumeStack->addPiecewiseFunction(pWF);
	}
	auto modTrans = modality(0)->transfer();
	modTrans->colorTF()->DeepCopy(m_volumeStack->colorTF(0));
	modTrans->opacityTF()->DeepCopy(m_volumeStack->opacityTF(0));
	m_renderer->reInitialize(m_imageData, m_polyData);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateChannel(0, iAChannelData(modality(0)->name(), m_imageData, modTrans->colorTF()));
	}
	updateViews();
	*/
}

/*
int MdiChild::chooseComponentNr(int modalityNr)
{
	if (!isVolumeDataLoaded())
	{
		return -1;
	}
	int nrOfComponents = modality(modalityNr)->image()->GetNumberOfScalarComponents();
	if (nrOfComponents == 1)
	{
		return 0;
	}
	QStringList components;
	for (int i = 0; i < nrOfComponents; ++i)
	{
		components << QString::number(i);
	}
	components << "All components";
	iAAttributes params;
	addAttr(params, "Component", iAValueType::Categorical, components);
	iAParameterDlg componentChoice(this, "Choose Component", params);
	if (componentChoice.exec() != QDialog::Accepted)
	{
		return -1;
	}
	return components.indexOf(componentChoice.parameterValues()["Component"].toString());
}
*/

std::shared_ptr<iADataSet> MdiChild::chooseDataSet(QString const & title)
{
	if (m_dataSets.size() == 1)
	{
		return m_dataSets.begin()->second;
	}
	iAAttributes params;
	QStringList dataSetNames;
	for (auto dataSet : dataSets())
	{
		dataSetNames << dataSet->name();
	}
	const QString DataSetStr("Dataset");
	addAttr(params, DataSetStr, iAValueType::Categorical, dataSetNames);
	iAParameterDlg dataSetChoice(this, title, params);
	if (dataSetChoice.exec() == QDialog::Accepted)
	{
		auto dataSetName = dataSetChoice.parameterValues()[DataSetStr].toString();
		for (auto dataSet : m_dataSets)
		{
			if (dataSet.second->name() == dataSetName)
			{
				return dataSet.second;
			}
		}
	}
	return nullptr;
}

iADataSetListWidget* MdiChild::dataSetListWidget()
{
	return m_dataSetListWidget;
}

void MdiChild::saveVolumeStack()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save File"),
		QDir::currentPath(),
		tr("Volstack files (*.volstack);;All files (*)")
	);
	if (fileName.isEmpty())
	{
		return;
	}
	auto allDataSets = dataSets();
	auto imgDataSets = std::make_shared<iADataCollection>(allDataSets.size(), std::shared_ptr<QSettings>());
	for(auto d: allDataSets)
	{
		if (dynamic_cast<iAImageData*>(d.get()))
		{
			imgDataSets->addDataSet(d);
		}
	}

	QFileInfo fi(fileName);
	QVariantMap paramValues;
	paramValues[iAFileStackParams::FileNameBase] = fi.completeBaseName();
	paramValues[iAFileStackParams::Extension] = ".mhd";
	paramValues[iAFileStackParams::NumDigits] = static_cast<int>(std::log10(static_cast<double>(imgDataSets->dataSets().size())) + 1);
	paramValues[iAFileStackParams::MinimumIndex] = 0;
	//paramValues[iAFileStackParams::MaximumIndex] = imgDataSets->size() - 1;

	auto io = std::make_shared<iAVolStackFileIO>();
	iAFileParamDlg dlg;
	if (!dlg.askForParameters(this, io->parameter(iAFileIO::Save), io->name(), paramValues, fileName))
	{
		return;
	}
	auto p = std::make_shared<iAProgress>();
	auto fw = runAsync([=]()
		{
			io->save(fileName,imgDataSets, paramValues, *p.get());
		},
		[]() {},
		this);
	iAJobListView::get()->addJob(QString("Saving volume stack %1").arg(fileName), p.get(), fw);
}

bool MdiChild::saveNew()
{
	return saveNew(chooseDataSet());
}

bool MdiChild::saveNew(std::shared_ptr<iADataSet> dataSet)
{
	if (!dataSet)
	{
		return false;
	}
	// TODO NEWIO: provide option to only store single component, in case dataSet is an image data and contains multiple components!
	// below the code from previous iAModality-based parts:
	/*
		int componentNr = chooseComponentNr(modalityNr);
		if (componentNr == -1)
		{
			return false;
		}
		if (m_tmpSaveImg->GetNumberOfScalarComponents() > 1 &&
			componentNr != m_tmpSaveImg->GetNumberOfScalarComponents())
		{
			auto imgExtract = vtkSmartPointer<vtkImageExtractComponents>::New();
			imgExtract->SetInputData(m_tmpSaveImg);
			imgExtract->SetComponents(componentNr);
			imgExtract->Update();
			m_tmpSaveImg = imgExtract->GetOutput();
		}
	*/
	QString path = m_path.isEmpty() ? m_mainWnd->path() : m_path;
	QString defaultFilter = iAFileTypeRegistry::defaultExtFilterString(dataSet->type());
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
		path + "/" + safeFileName(dataSet->name()),
		iAFileTypeRegistry::registeredFileTypes(iAFileIO::Save, dataSet->type()),
		 &defaultFilter);
	return saveNew(dataSet, fileName);
}

bool MdiChild::saveNew(std::shared_ptr<iADataSet> dataSet, QString const& fileName)
{
	if (fileName.isEmpty())
	{
		return false;
	}
	auto io = iAFileTypeRegistry::createIO(fileName, iAFileIO::Save);
	if (!io || !io->isDataSetSupported(dataSet, fileName))
	{
		LOG(lvlError, "The chosen file format does not support this kind of dataset!");
		return false;
	}
	QVariantMap paramValues;
	if (!iAFileParamDlg::getParameters(this, io.get(), iAFileIO::Save, fileName, paramValues))
	{
		return false;
	}
	auto p = std::make_shared<iAProgress>();
	auto futureWatcher = new QFutureWatcher<bool>(this);
	QObject::connect(futureWatcher, &QFutureWatcher<bool>::finished, this, [this, dataSet, fileName, futureWatcher]()
		{
			if (futureWatcher->result())
			{
				LOG(lvlInfo, QString("Saved file %1").arg(fileName));
				dataSet->setMetaData(iADataSet::FileNameKey, fileName);
				if (m_dataSets.size() == 1)
				{
					setWindowTitleAndFile(fileName);
				}
				else
				{
					m_mainWnd->addRecentFile(fileName);
					setWindowModified(hasUnsavedData());
				}
			}
			delete futureWatcher;
		});
	auto future = QtConcurrent::run([fileName, p, io, dataSet, paramValues]()
		{
			try
			{
				io->save(fileName, dataSet, paramValues, *p.get());
				return true;
			}
			// TODO: unify exception handling?
			catch (itk::ExceptionObject& e)
			{
				LOG(lvlError, QString("Error saving file %1: %2").arg(fileName).arg(e.GetDescription()));
			}
			catch (std::exception& e)
			{
				LOG(lvlError, QString("Error saving file %1: %2").arg(fileName).arg(e.what()));
			}
			catch (...)
			{
				LOG(lvlError, QString("Unknown error while saving file %1!").arg(fileName));
			}
			return false;
		});
	futureWatcher->setFuture(future);
	iAJobListView::get()->addJob("Save File", p.get(), futureWatcher);
	return true;
}

void MdiChild::updateViews()
{
	updateSlicers();
	updateRenderer();
	emit viewsUpdated();
}

void MdiChild::maximizeSlicer(int mode)
{
	resizeDockWidget(m_dwSlicer[mode]);
}

void MdiChild::maximizeRC()
{
	resizeDockWidget(m_dwRenderer);
}

void MdiChild::saveRC()
{
	iAImageStackFileIO io;
	QString file = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"",
		io.filterString());
	if (file.isEmpty())
	{
		return;
	}
	vtkSmartPointer<vtkWindowToImageFilter> filter = vtkSmartPointer<vtkWindowToImageFilter>::New();
	filter->SetInput(m_renderer->renderWindow());
	filter->Update();
	writeSingleSliceImage(file, filter->GetOutput());
}

void MdiChild::saveMovRC()
{
	QString movie_file_types = GetAvailableMovieFormats();

	// If VTK was built without video support, display error message and quit.
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "Sorry, but movie export support is disabled.");
		return;
	}

	QStringList modes = (QStringList() << tr("Rotate Z") << tr("Rotate X") << tr("Rotate Y"));
	iAAttributes params;
	addAttr(params, "Rotation mode", iAValueType::Categorical, modes);
	iAParameterDlg dlg(this, "Save movie options", params,
		"Creates a movie by rotating the object around a user-defined axis in the 3D renderer.");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	QString mode = dlg.parameterValues()["Rotation mode"].toString();
	int imode = modes.indexOf(mode);

	// Show standard save file dialog using available movie file types.
	m_renderer->saveMovie(QFileDialog::getSaveFileName(this, tr("Export movie %1").arg(mode),
							m_fileInfo.absolutePath() + "/" +
								((mode.isEmpty()) ? m_fileInfo.baseName() : m_fileInfo.baseName() + "_" + mode),
							movie_file_types),
		imode);
}

void MdiChild::camPosition(double* camOptions)
{
	m_renderer->camPosition(camOptions);
}

void MdiChild::setCamPosition(int pos)
{
	m_renderer->setCamPosition(pos);
}

void MdiChild::setCamPosition(double* camOptions, bool rsParallelProjection)
{
	m_renderer->setCamPosition(camOptions, rsParallelProjection);
}

void MdiChild::toggleRendererInteraction()
{
	if (m_renderer->interactor()->GetEnabled())
	{
		m_renderer->disableInteractor();
		LOG(lvlInfo, tr("Renderer disabled."));
	}
	else
	{
		m_renderer->enableInteractor();
		LOG(lvlInfo, tr("Renderer enabled."));
	}
}

void MdiChild::setSlice(int mode, int s)
{
	if (m_snakeSlicer)
	{
		int sliceAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::Z);
		updateSnakeSlicer(m_dwSlicer[mode]->sbSlice, m_slicer[mode], sliceAxis, s);
	}
	else
	{
		//Update Slicer if changed
		if (m_dwSlicer[mode]->sbSlice->value() != s)
		{
			QSignalBlocker block(m_dwSlicer[mode]->sbSlice);
			m_dwSlicer[mode]->sbSlice->setValue(s);
		}
		if (m_dwSlicer[mode]->verticalScrollBar->value() != s)
		{
			QSignalBlocker block(m_dwSlicer[mode]->verticalScrollBar);
			m_dwSlicer[mode]->verticalScrollBar->setValue(s);
		}

		if (m_renderSettings.ShowSlicers || m_renderSettings.ShowSlicePlanes)
		{
			set3DSlicePlanePos(mode, s);
		}
	}
}

void MdiChild::set3DSlicePlanePos(int mode, int slice)
{
	if (!firstImageData())
	{
		return;
	}
	int sliceAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::Z);
	double plane[3];
	std::fill(plane, plane + 3, 0);
	auto const spacing = firstImageData()->GetSpacing();
	// + 0.5 to place slice plane in the middle of the sliced voxel:
	plane[sliceAxis] = (slice + 0.5) * spacing[sliceAxis];
	m_renderer->setSlicePlanePos(sliceAxis, plane[0], plane[1], plane[2]);
}

void MdiChild::updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s)
{
	if (!firstImageData())
	{
		return;
	}
	double spacing[3];
	firstImageData()->GetSpacing(spacing);

	double splinelength = (int)m_parametricSpline->GetLength();
	double length_percent = 100 / splinelength;
	double mf1 = s + 1; //multiplication factor for first point
	double mf2 = s + 2; //multiplication factor for second point
	spinBox->setRange(0, (splinelength - 1));//set the number of slices to scroll through

													//calculate the percentage for 2 points
	double t1[3] = { length_percent * mf1 / 100, length_percent * mf1 / 100, length_percent * mf1 / 100 };
	double t2[3] = { length_percent * mf2 / 100, length_percent * mf2 / 100, length_percent * mf2 / 100 };
	double point1[3], point2[3];
	//calculate the points
	m_parametricSpline->Evaluate(t1, point1, nullptr);
	m_parametricSpline->Evaluate(t2, point2, nullptr);

	//calculate normal
	double normal[3];
	normal[0] = point2[0] - point1[0];
	normal[1] = point2[1] - point1[1];
	normal[2] = point2[2] - point1[2];

	vtkMatrixToLinearTransform* final_transform = vtkMatrixToLinearTransform::New();

	if (normal[0] == 0 && normal[1] == 0)
	{
		//Move the point to origin Translation
		double PointToOrigin_matrix[16] = { 1, 0, 0, point1[0],
			0, 1, 0, point1[1],
			0, 0, 1, point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* PointToOrigin_Translation = vtkMatrix4x4::New();
		PointToOrigin_Translation->DeepCopy(PointToOrigin_matrix);

		//Move the origin to point Translation
		double OriginToPoint_matrix[16] = { 1, 0, 0, -point1[0],
			0, 1, 0, -point1[1],
			0, 0, 1, -point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* OriginToPoint_Translation = vtkMatrix4x4::New();
		OriginToPoint_Translation->DeepCopy(OriginToPoint_matrix);

		///multiplication of transformation matics
		vtkMatrix4x4* Transformation_4 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(PointToOrigin_Translation, OriginToPoint_Translation, Transformation_4);

		final_transform->SetInput(Transformation_4);
		final_transform->Update();
	}
	else
	{
		//Move the point to origin Translation
		double PointToOrigin_matrix[16] = { 1, 0, 0, point1[0],
			0, 1, 0, point1[1],
			0, 0, 1, point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* PointToOrigin_Translation = vtkMatrix4x4::New();
		PointToOrigin_Translation->DeepCopy(PointToOrigin_matrix);

		//rotate around Z to bring the vector to XZ plane
		double alpha = std::acos(std::pow(normal[0], 2) / (std::sqrt(std::pow(normal[0], 2)) * (std::sqrt(std::pow(normal[0], 2) + std::pow(normal[1], 2)))));
		double cos_theta_xz = std::cos(alpha);
		double sin_theta_xz = std::sin(alpha);

		double rxz_matrix[16] = { cos_theta_xz,	-sin_theta_xz,	0,	 0,
			sin_theta_xz,	cos_theta_xz,	0,	 0,
			0,			0,		1,	 0,
			0,			0,		0,	 1 };

		vtkMatrix4x4* rotate_around_xz = vtkMatrix4x4::New();
		rotate_around_xz->DeepCopy(rxz_matrix);

		//rotate around Y to bring vector parallel to Z axis
		double beta = std::acos(std::pow(normal[2], 2) / std::sqrt(std::pow(normal[2], 2)) + std::sqrt(std::pow(cos_theta_xz, 2) + std::pow(normal[2], 2)));
		double cos_theta_y = std::cos(beta);
		double sin_theta_y = std::sin(beta);

		double ry_matrix[16] = { cos_theta_y,	0,	sin_theta_y,	0,
			0,			1,		0,			0,
			-sin_theta_y,	0,	cos_theta_y,	0,
			0,			0,		0,			1 };

		vtkMatrix4x4* rotate_around_y = vtkMatrix4x4::New();
		rotate_around_y->DeepCopy(ry_matrix);

		//rotate around Z by 180 degree - to bring object correct view
		double cos_theta_z = std::cos(vtkMath::Pi());
		double sin_theta_z = std::sin(vtkMath::Pi());

		double rz_matrix[16] = { cos_theta_z,	-sin_theta_z,	0,	0,
			sin_theta_z,	cos_theta_z,	0,	0,
			0,				0,			1,	0,
			0,				0,			0,	1 };

		vtkMatrix4x4* rotate_around_z = vtkMatrix4x4::New();
		rotate_around_z->DeepCopy(rz_matrix);

		//Move the origin to point Translation
		double OriginToPoint_matrix[16] = { 1, 0, 0, -point1[0],
			0, 1, 0, -point1[1],
			0, 0, 1, -point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* OriginToPoint_Translation = vtkMatrix4x4::New();
		OriginToPoint_Translation->DeepCopy(OriginToPoint_matrix);

		///multiplication of transformation matics
		vtkMatrix4x4* Transformation_1 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(PointToOrigin_Translation, rotate_around_xz, Transformation_1);

		vtkMatrix4x4* Transformation_2 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_1, rotate_around_y, Transformation_2);

		vtkMatrix4x4* Transformation_3 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_2, rotate_around_z, Transformation_3);

		vtkMatrix4x4* Transformation_4 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_3, OriginToPoint_Translation, Transformation_4);

		final_transform->SetInput(Transformation_4);
		final_transform->Update();
	}

	slicer->channel(0)->setTransform(final_transform);
	QSignalBlocker block(slicer);
	slicer->setSliceNumber(point1[ptIndex]);
}

void MdiChild::slicerRotationChanged()
{
	m_renderer->setPlaneNormals(m_slicerTransform);
}

void MdiChild::linkViews(bool l)
{
	m_slicerSettings.LinkViews = l;
}

void MdiChild::linkMDIs(bool lm)
{
	m_slicerSettings.LinkMDIs = lm;
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_slicer[s]->setLinkedMdiChild(lm ? this : nullptr);
	}
}

void MdiChild::enableSlicerInteraction(bool b)
{
	for (int s = 0; s < 3; ++s)
	{
		if (b)
		{
			m_slicer[s]->enableInteractor();
		}
		else
		{
			m_slicer[s]->disableInteractor();
		}
	}
}

bool MdiChild::applyPreferences(iAPreferences const& prefs)
{
	m_preferences = prefs;
	applyViewerPreferences();
	if (isMagicLens2DEnabled())
	{
		updateSlicers();  // for updating MagicLensSize, MagicLensFrameWidth
	}
	return true;
}

void MdiChild::applyViewerPreferences()
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensFrameWidth(m_preferences.MagicLensFrameWidth);
		m_slicer[s]->setMagicLensSize(m_preferences.MagicLensSize);
		m_slicer[s]->setStatisticalExtent(m_preferences.StatisticalExtent);
	}
	m_dwRenderer->vtkWidgetRC->setLensSize(m_preferences.MagicLensSize, m_preferences.MagicLensSize);
	updatePositionMarkerSize();
	emit preferencesChanged();
}

void MdiChild::updatePositionMarkerSize()
{
	const double MinSpacing = 0.00000001;
	std::array<double, 3> maxSpacing{ MinSpacing, MinSpacing, MinSpacing };
	for (auto dataSet: m_dataSets)
	{
		auto unitDist = dataSet.second->unitDistance();
		for (int c = 0; c < 3; ++c)
		{
			maxSpacing[c] = std::max(maxSpacing[c], unitDist[c] * m_preferences.StatisticalExtent);
		}
	}
	m_renderer->setUnitSize(maxSpacing);
}

void MdiChild::setRenderSettings(iARenderSettings const& rs, iAVolumeSettings const& vs)
{
	// TODO NEWIO: there are now settings of the individual viewers -> REMOVE?
	m_renderSettings = rs;
	m_volumeSettings = vs;
}

void MdiChild::applyVolumeSettings()
{
	// TODO NEWIO: there are now settings of the individual viewers -> REMOVE?
	for (int i = 0; i < 3; ++i)
	{
		m_dwSlicer[i]->showBorder(m_renderSettings.ShowSlicePlanes);
	}
	for (auto v : m_dataSetViewers)
	{
		if (v.second->renderer())
		{
			continue;
		}
		if (m_renderSettings.ShowSlicers && !m_snakeSlicer)
		{
			v.second->renderer()->setCuttingPlanes(m_renderer->plane1(), m_renderer->plane2(), m_renderer->plane3());
		}
		else
		{
			v.second->renderer()->removeCuttingPlanes();
		}
	}
}

QString MdiChild::layoutName() const
{
	return m_layout;
}

void MdiChild::updateLayout()
{
	m_mainWnd->loadLayout();
}

void MdiChild::loadLayout(QString const& layout)
{
	m_layout = layout;
	QSettings settings;
	QByteArray state = settings.value("Layout/state" + layout).value<QByteArray>();
	hide();
	restoreState(state, 0);
	m_isSmthMaximized = false;
	show();
}

void MdiChild::resetLayout()
{
	restoreState(m_initialLayoutState);
	m_isSmthMaximized = false;
}

void MdiChild::setupSlicers(iASlicerSettings const& ss, bool init)
{
	// TODO: separate applying slicer setting from slicer set up
	m_slicerSettings = ss;

	if (m_snakeSlicer)
	{
		// TODO: check why only XY slice here?
		int prevMax = m_dwSlicer[iASlicerMode::XY]->sbSlice->maximum();
		int prevValue = m_dwSlicer[iASlicerMode::XY]->sbSlice->value();
		m_dwSlicer[iASlicerMode::XY]->sbSlice->setRange(0, ss.SnakeSlices - 1);
		m_dwSlicer[iASlicerMode::XY]->sbSlice->setValue((double)prevValue / prevMax * (ss.SnakeSlices - 1));
	}

	linkViews(ss.LinkViews);
	linkMDIs(ss.LinkMDIs);

	for (int s = 0; s < 3; ++s)
	{
		auto settings(ss.SingleSlicer);
		if (!ss.BackgroundColor[s].isEmpty())
		{
			settings.backgroundColor = QColor(ss.BackgroundColor[s]);
		}
		m_slicer[s]->setup(settings);
	}

	if (init)
	{
		// connect signals for making slicers update other views in snake slicers mode:
		for (int i = 0; i < 3; ++i)
		{
			connect(m_slicer[i], &iASlicerImpl::profilePointChanged, m_renderer, &iARendererImpl::setProfilePoint);
			connect(m_slicer[i], &iASlicer::magicLensToggled, this, &MdiChild::toggleMagicLens2D);
			for (int j = 0; j < 3; ++j)
			{
				if (i != j)	// connect each slicer's signals to the other slicer's slots, except for its own:
				{
					connect(m_slicer[i], &iASlicerImpl::addedPoint, m_slicer[j], &iASlicerImpl::addPoint);
					connect(m_slicer[i], &iASlicerImpl::movedPoint, m_slicer[j], &iASlicerImpl::movePoint);
					connect(m_slicer[i], &iASlicerImpl::profilePointChanged, m_slicer[j], &iASlicerImpl::setProfilePoint);
					connect(m_slicer[i], &iASlicerImpl::switchedMode, m_slicer[j], &iASlicerImpl::switchInteractionMode);
					connect(m_slicer[i], &iASlicerImpl::deletedSnakeLine, m_slicer[j], &iASlicerImpl::deleteSnakeLine);
					connect(m_slicer[i], &iASlicerImpl::deselectedPoint, m_slicer[j], &iASlicerImpl::deselectPoint);
				}
			}
		}
	}
}

bool MdiChild::applyRendererSettings(iARenderSettings const& rs, iAVolumeSettings const& vs)
{
	setRenderSettings(rs, vs);
	applyVolumeSettings();
	m_renderer->applySettings(renderSettings(), m_slicerVisibility);
	m_dwRenderer->vtkWidgetRC->show();
	m_dwRenderer->vtkWidgetRC->renderWindow()->Render();
	emit renderSettingsChanged();
	return true;
}

iARenderSettings const& MdiChild::renderSettings() const
{
	return m_renderSettings;
}

iAVolumeSettings const& MdiChild::volumeSettings() const
{
	return m_volumeSettings;
}

iASlicerSettings const& MdiChild::slicerSettings() const
{
	return m_slicerSettings;
}

iAPreferences const& MdiChild::preferences() const
{
	return m_preferences;
}

bool MdiChild::editSlicerSettings(iASlicerSettings const& slicerSettings)
{
	setupSlicers(slicerSettings, false);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->show();
	}
	emit slicerSettingsChanged();
	return true;
}

void MdiChild::toggleSnakeSlicer(bool isChecked)
{
	if (!firstImageData())
	{
		return;
	}
	m_snakeSlicer = isChecked;

	if (m_snakeSlicer)
	{
		if (m_renderSettings.ShowSlicers)
		{
			for (auto v : m_dataSetViewers)
			{
				v.second->renderer()->removeCuttingPlanes();
			}
		}

		// save the slicer transforms
		for (int s = 0; s < 3; ++s)
		{
			m_savedSlicerTransform[s] = m_slicer[s]->channel(0)->reslicer()->GetResliceTransform();
		}

		m_parametricSpline->Modified();
		double emptyper[3]; emptyper[0] = 0; emptyper[1] = 0; emptyper[2] = 0;
		double emptyp[3]; emptyp[0] = 0; emptyp[1] = 0; emptyp[2] = 0;
		m_parametricSpline->Evaluate(emptyper, emptyp, nullptr);

		// save the slicer transforms
		for (int s = 0; s < 3; ++s)
		{
			m_savedSlicerTransform[s] = m_slicer[s]->channel(0)->reslicer()->GetResliceTransform();
			m_slicer[s]->switchInteractionMode(iASlicerImpl::SnakeShow);
			m_dwSlicer[s]->sbSlice->setValue(0);
		}
	}
	else
	{	// restore the slicer transforms
		m_slicer[iASlicerMode::YZ]->channel(0)->reslicer()->SetResliceAxesDirectionCosines(0, 1, 0, 0, 0, 1, 1, 0, 0);
		m_slicer[iASlicerMode::XZ]->channel(0)->reslicer()->SetResliceAxesDirectionCosines(1, 0, 0, 0, 0, 1, 0, -1, 0);
		m_slicer[iASlicerMode::XY]->channel(0)->reslicer()->SetResliceAxesDirectionCosines(1, 0, 0, 0, 1, 0, 0, 0, 1);

		for (int s = 0; s < 3; ++s)
		{
			m_dwSlicer[s]->sbSlice->setValue(firstImageData()->GetDimensions()[mapSliceToGlobalAxis(s, iAAxisIndex::Z)] >> 1);
			m_slicer[s]->channel(0)->reslicer()->SetResliceTransform(m_savedSlicerTransform[s]);
			m_slicer[s]->channel(0)->reslicer()->SetOutputExtentToDefault();
			m_slicer[s]->resetCamera();
			m_slicer[s]->renderer()->Render();
			m_slicer[s]->switchInteractionMode(iASlicerImpl::Normal);
		}
		if (m_renderSettings.ShowSlicers)
		{
			for (auto v : m_dataSetViewers)
			{
				v.second->renderer()->setCuttingPlanes(m_renderer->plane1(), m_renderer->plane2(), m_renderer->plane3());
			}
		}
	}
}

void MdiChild::snakeNormal(int index, double point[3], double normal[3])
{
	if (!firstImageData())
	{
		return;
	}
	int i1 = index;
	int i2 = index + 1;

	double spacing[3];
	firstImageData()->GetSpacing(spacing);

	int snakeSlices = m_slicerSettings.SnakeSlices;
	if (index == (snakeSlices - 1))
	{
		i1--;
		i2--;
	}

	if (index >= 0 && index < snakeSlices)
	{
		double p1[3], p2[3];
		double t1[3] =
		{ (double)i1 / (snakeSlices - 1), (double)i1 / (snakeSlices - 1), (double)i1 / (snakeSlices - 1) };
		double t2[3] = { (double)i2 / (snakeSlices - 1), (double)i2 / (snakeSlices - 1), (double)i2 / (snakeSlices - 1) };
		m_parametricSpline->Evaluate(t1, p1, nullptr);
		m_parametricSpline->Evaluate(t2, p2, nullptr);

		//calculate the points
		p1[0] /= spacing[0]; p1[1] /= spacing[1]; p1[2] /= spacing[2];
		p2[0] /= spacing[0]; p2[1] /= spacing[1]; p2[2] /= spacing[2];

		//calculate the vector between to points
		if (normal)
		{
			normal[0] = p2[0] - p1[0];
			normal[1] = p2[1] - p1[1];
			normal[2] = p2[2] - p1[2];
		}

		point[0] = p1[0]; point[1] = p1[1]; point[2] = p1[2];
	}
}

bool MdiChild::isSnakeSlicerToggled() const
{
	return m_snakeSlicer;
}

void MdiChild::toggleSliceProfile(bool isChecked)
{
	m_isSliceProfileEnabled = isChecked;
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setSliceProfileOn(m_isSliceProfileEnabled);
	}
}

bool MdiChild::isSliceProfileEnabled() const
{
	return m_isSliceProfileEnabled;
}

void MdiChild::toggleMagicLens2D(bool isEnabled)
{
	m_isMagicLensEnabled = isEnabled;
	if (isEnabled)
	{
		changeMagicLensDataSet(0);
	}
	setMagicLensEnabled(isEnabled);
	updateSlicers();
	m_mainWnd->updateMagicLens2DCheckState(isEnabled);
	emit magicLensToggled(m_isMagicLensEnabled);
}

void MdiChild::toggleMagicLens3D(bool isEnabled)
{
	if (isEnabled)
	{
		m_dwRenderer->vtkWidgetRC->magicLensOn();
	}
	else
	{
		m_dwRenderer->vtkWidgetRC->magicLensOff();
	}
}

bool MdiChild::isMagicLens2DEnabled() const
{
	return m_isMagicLensEnabled;
}

bool MdiChild::isMagicLens3DEnabled() const
{
	return m_dwRenderer->vtkWidgetRC->isMagicLensEnabled();
}

void MdiChild::updateDataSetInfo()
{   // TODO: optimize - don't fully recreate each time, just do necessary adjustments?
	m_dataSetInfo->clear();
	for (auto dataSet: m_dataSets)
	{
		if (!m_dataSetViewers[dataSet.first])    // probably not computed yet...
		{
			continue;
		}
		auto lines = m_dataSetViewers[dataSet.first]->information().split("\n", Qt::SkipEmptyParts);
		if (dataSet.second->hasMetaData(iADataSet::FileNameKey))
		{
			lines.prepend(iADataSet::FileNameKey + ": " + dataSet.second->metaData(iADataSet::FileNameKey).toString());
		}
		std::for_each(lines.begin(), lines.end(), [](QString& s) { s = "    " + s; });
		m_dataSetInfo->addItem(dataSet.second->name() + "\n" + lines.join("\n"));
	}
}

bool MdiChild::addVolumePlayer()
{
	m_dwVolumePlayer = new dlg_volumePlayer(this, m_volumeStack.data());
	splitDockWidget(m_dwDataSets, m_dwVolumePlayer, Qt::Horizontal);
	for (size_t id = 0; id < m_volumeStack->numberOfVolumes(); ++id)
	{
		m_checkedList.append(0);
	}
	return true;
}

void MdiChild::updateROI(int const roi[6])
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateROI(roi);
	}
	auto img = firstImageData();
	if (!img)
	{
		return;
	}
	m_renderer->setSlicingBounds(roi, img->GetSpacing());
}

void MdiChild::setROIVisible(bool visible)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setROIVisible(visible);
	}
	m_renderer->setROIVisible(visible);
	updateViews();
}

QString MdiChild::currentFile() const
{
	return m_curFile;
}

QFileInfo const & MdiChild::fileInfo() const
{
	return m_fileInfo;
}

void MdiChild::closeEvent(QCloseEvent* event)
{
	// TODO NEWIO: Check whether it's still required to check whether currently an I/O operation is in progress
	/*
	{
		LOG(lvlWarn, "Cannot close window while I/O operation is in progress!");
		event->ignore();
		return;
	}
	else
	{
	*/
	if (isWindowModified())
	{
		auto reply = QMessageBox::question(this, "Unsaved changes",
			"You have unsaved changes. Are you sure you want to close this window?",
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes)
		{
			event->ignore();
			return;
		}
	}
	emit closed();
	event->accept();
}

void MdiChild::setWindowTitleAndFile(const QString& f)
{
	QString title = f;
	if (QFile::exists(f))
	{
		m_fileInfo.setFile(f);
		m_curFile = f;
		m_path = m_fileInfo.canonicalPath();
		m_isUntitled = f.isEmpty();
		m_mainWnd->addRecentFile(f);
		title = m_fileInfo.fileName();
	}
	setWindowTitle(title + "[*]");
	setWindowModified(hasUnsavedData());
}

void MdiChild::multiview()
{
	m_dwRenderer->setVisible(true);
	m_dwSlicer[iASlicerMode::XY]->setVisible(true);
	m_dwSlicer[iASlicerMode::YZ]->setVisible(true);
	m_dwSlicer[iASlicerMode::XZ]->setVisible(true);
}

void MdiChild::updateSlicer(int index)
{
	if (index < 0 || index > 2)
	{
		QMessageBox::warning(this, tr("Wrong slice plane index"), tr("updateSlicer(int index) is called with the wrong index parameter"));
		return;
	}
	m_slicer[index]->update();
}

void MdiChild::initChannelRenderer(uint id, bool use3D, bool enableChannel)
{
	iAChannelData* chData = channelData(id);
	assert(chData);
	if (!chData)
	{
		return;
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->addChannel(id, *chData, false);
	}

	if (use3D)
	{
		chData->set3D(true);
		// TODO: VOLUME: rewrite using separate volume /
		//    add capabilities of combining volumes from TripleHistogramTF module to renderer
		// m_renderer->addChannel(chData);
	}
	setChannelRenderingEnabled(id, enableChannel);
}

iAChannelData* MdiChild::channelData(uint id)
{
	auto it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->data();
}

iAChannelData const* MdiChild::channelData(uint id) const
{
	auto it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->data();
}

uint MdiChild::createChannel()
{
	uint newChannelID = m_nextChannelID;
	++m_nextChannelID;
	m_channels.insert(newChannelID, QSharedPointer<iAChannelData>::create());
	return newChannelID;
}

void MdiChild::updateSlicers()
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->update();
	}
}

void MdiChild::updateRenderer()
{
	m_renderer->update();
	m_renderer->renderWindow()->GetInteractor()->Modified();
	m_renderer->renderWindow()->GetInteractor()->Render();
	m_dwRenderer->vtkWidgetRC->update();
}

void MdiChild::updateChannelOpacity(uint id, double opacity)
{
	if (!channelData(id))
	{
		return;
	}
	channelData(id)->setOpacity(opacity);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setChannelOpacity(id, opacity);
	}
	updateSlicers();
}

void MdiChild::setChannelRenderingEnabled(uint id, bool enabled)
{
	iAChannelData* chData = channelData(id);
	if (!chData || chData->isEnabled() == enabled)
	{
		// the channel with the given ID doesn't exist or hasn't changed
		return;
	}
	chData->setEnabled(enabled);
	setSlicerChannelEnabled(id, enabled);
	/*
	// TODO: VOLUME: rewrite using volume manager:
	if (chData->Uses3D())
	{
		renderWidget()->updateChannelImages();
	}
	*/
	updateViews();
}

void MdiChild::setSlicerChannelEnabled(uint id, bool enabled)
{
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		slicer(i)->enableChannel(id, enabled);
	}
}

void MdiChild::removeChannel(uint id)
{
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (slicer(i)->hasChannel(id))
		{
			slicer(i)->removeChannel(id);
		}
	}
	m_channels.remove(id);
}

void MdiChild::removeFinishedAlgorithms()
{
	for (size_t i = 0; i < m_workingAlgorithms.size(); )
	{
		if (m_workingAlgorithms[i]->isFinished())
		{
			delete m_workingAlgorithms[i];
			m_workingAlgorithms.erase(m_workingAlgorithms.begin() + i);
		}
		else
		{
			++i;
		}
	}
}

void MdiChild::cleanWorkingAlgorithms()
{
	for (size_t i = 0; i < m_workingAlgorithms.size(); ++i)
	{
		if (m_workingAlgorithms[i]->isRunning())
		{
			m_workingAlgorithms[i]->SafeTerminate();
			delete m_workingAlgorithms[i];
		}
	}
	m_workingAlgorithms.clear();
}

void MdiChild::toggleProfileHandles(bool isChecked)
{
	m_profileHandlesEnabled = isChecked;
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setProfileHandlesOn(m_profileHandlesEnabled);
	}
	m_renderer->setProfileHandlesOn(m_profileHandlesEnabled);
	updateViews();
}

bool MdiChild::profileHandlesEnabled() const
{
	return m_profileHandlesEnabled;
}

int MdiChild::sliceNumber(int mode) const
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode]->sliceNumber();
}

void MdiChild::maximizeDockWidget(QDockWidget* dw)
{
	// TODO: not ideal - user could have changed something in layout since last maximization -> confusion!
	m_beforeMaximizeState = saveState();
	QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>();
	for (int i = 0; i < dockWidgets.size(); ++i)
	{
		QDockWidget* curDW = dockWidgets[i];
		if (curDW != dw)
		{
			curDW->setVisible(false);
		}
	}
	m_whatMaximized = dw;
	m_isSmthMaximized = true;
}

void MdiChild::demaximizeDockWidget(QDockWidget* /*dw*/)
{
	restoreState(m_beforeMaximizeState);
	m_isSmthMaximized = false;
}

void MdiChild::resizeDockWidget(QDockWidget* dw)
{
	if (m_isSmthMaximized)
	{
		if (m_whatMaximized == dw)
		{
			demaximizeDockWidget(dw);
		}
		else
		{
			demaximizeDockWidget(m_whatMaximized);
			maximizeDockWidget(dw);
		}
	}
	else
	{
		maximizeDockWidget(dw);
	}
}

iASlicer* MdiChild::slicer(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode];
}

QWidget* MdiChild::rendererWidget()
{
	return m_dwRenderer->vtkWidgetRC;
}

QSlider* MdiChild::slicerScrollBar(int mode)
{
	return m_dwSlicer[mode]->verticalScrollBar;
}

QHBoxLayout* MdiChild::slicerContainerLayout(int mode)
{
	return m_dwSlicer[mode]->horizontalLayout_2;
}

QDockWidget* MdiChild::slicerDockWidget(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_dwSlicer[mode];
}

QDockWidget* MdiChild::renderDockWidget()
{
	return m_dwRenderer;
}

QDockWidget* MdiChild::dataInfoDockWidget()
{
	return m_dwInfo;
}

vtkTransform* MdiChild::slicerTransform()
{
	return m_slicerTransform;
}

bool MdiChild::resultInNewWindow() const
{
	return m_preferences.ResultInNewWindow;
}

bool MdiChild::linkedMDIs() const
{
	return m_slicerSettings.LinkMDIs;
}

bool MdiChild::linkedViews() const
{
	return m_slicerSettings.LinkViews;
}

void MdiChild::adapt3DViewDisplay()
{
	// TODO NEWIO:
	//     - move (part of functionality) into dataset viewers (the check for whether specific slicer required)
	std::array<bool, 3> slicerVisibility = { false, false, false };
	bool rendererVisibility = false;
	for (auto ds : m_dataSets)
	{
		auto imgDS = dynamic_cast<iAImageData*>(ds.second.get());
		if (!imgDS)
		{
			rendererVisibility = true;  // all non-image datasets currently have a 3D representation!
			continue;
		}
		const int* dim = imgDS->vtkImage()->GetDimensions();
		rendererVisibility |= dim[0] > 1 && dim[1] > 1 && dim[2] > 1;
		for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
		{
			slicerVisibility[s] |= dim[mapSliceToGlobalAxis(s, 0)] > 1 && dim[mapSliceToGlobalAxis(s, 1)] > 1;
		}
	}
	m_dwRenderer->setVisible(rendererVisibility);
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_dwSlicer[s]->setVisible(slicerVisibility[s]);
	}
}

void MdiChild::setMagicLensInput(uint id)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensInput(id);
	}
}

void MdiChild::setMagicLensEnabled(bool isOn)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensEnabled(isOn);
	}
}

void MdiChild::updateChannel(uint id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf, bool enable)
{
	iAChannelData* chData = channelData(id);
	if (!chData)
	{
		return;
	}
	chData->setData(imgData, ctf, otf);
	for (uint s = 0; s < 3; ++s)
	{
		if (m_slicer[s]->hasChannel(id))
		{
			m_slicer[s]->updateChannel(id, *chData);
		}
		else
		{
			m_slicer[s]->addChannel(id, *chData, enable);
		}
	}
}
/*
void MdiChild::reInitMagicLens(uint id, QString const& name, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf)
{
	if (!m_isMagicLensEnabled)
	{
		return;
	}

	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateChannel(id, iAChannelData(name, imgData, ctf));
	}
	setMagicLensInput(id);
	updateSlicers();
}
*/

int MdiChild::magicLensSize() const
{
	return m_preferences.MagicLensSize;
}

int MdiChild::magicLensFrameWidth() const
{
	return m_preferences.MagicLensFrameWidth;
}

vtkRenderer* MdiChild::magicLens3DRenderer() const
{
	return m_dwRenderer->vtkWidgetRC->getLensRenderer();
}

QString MdiChild::filePath() const
{
	return m_path;
}

iAVolumeStack* MdiChild::volumeStack()
{
	return m_volumeStack.data();
}

bool MdiChild::isVolumeDataLoaded() const
{
	return firstImageDataSetIdx() != NoDataSet;
}

void MdiChild::changeMagicLensDataSet(int chg)
{
	// maybe move to slicer?
	if (!m_isMagicLensEnabled)
	{
		return;
	}
	std::vector<size_t> imageDataSets;
	for (auto dataSet : m_dataSets)
	{
		if (dataSet.second->type() == iADataSetType::Volume)
		{
			imageDataSets.push_back(dataSet.first);
		}
	}
	if (imageDataSets.empty())
	{
		setMagicLensEnabled(false);
		return;
	}
	if (chg == 0)   // initialization
	{
		m_magicLensDataSet = imageDataSets[0];
	}
	else            // we need to switch datasets
	{
		if (imageDataSets.size() <= 1)
		{   // only 1 dataset, nothing to change
			return;
		}
		auto it = std::find(imageDataSets.begin(), imageDataSets.end(), m_magicLensDataSet);
		if (it == imageDataSets.end())
		{
			m_magicLensDataSet = imageDataSets[0];
		}
		else
		{
			auto idx = it - imageDataSets.begin();
			auto newIdx = (idx + imageDataSets.size() + chg) % imageDataSets.size();
			m_magicLensDataSet = imageDataSets[newIdx];
		}
	}
	// To check: support for multiple components in a vtk image? or separating those components?
	auto imgData = dynamic_cast<iAImageData*>(m_dataSets[m_magicLensDataSet].get());
	auto viewer = dynamic_cast<iAVolumeViewer*>(m_dataSetViewers[m_magicLensDataSet].get());
	if (m_magicLensChannel == NotExistingChannel)
	{
		m_magicLensChannel = createChannel();
	}
	channelData(m_magicLensChannel)->setOpacity(0.5);
	QString name(imgData->name());
	channelData(m_magicLensChannel)->setName(name);
	updateChannel(m_magicLensChannel, imgData->vtkImage(), viewer->transfer()->colorTF(), viewer->transfer()->opacityTF(), false);
	setMagicLensInput(m_magicLensChannel);
}

void MdiChild::changeMagicLensOpacity(int chg)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensOpacity(m_slicer[s]->magicLensOpacity() + (chg * 0.05));
	}
}

void MdiChild::changeMagicLensSize(int chg)
{
	if (!isMagicLens2DEnabled())
	{
		return;
	}
	double sizeFactor = 1.1 * (std::abs(chg));
	if (chg < 0)
	{
		sizeFactor = 1 / sizeFactor;
	}
	int newSize = std::max(MinimumMagicLensSize, static_cast<int>(m_preferences.MagicLensSize * sizeFactor));
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensSize(newSize);
		newSize = std::min(m_slicer[s]->magicLensSize(), newSize);
	}
	m_preferences.MagicLensSize = newSize;
	updateSlicers();
}

void MdiChild::set3DControlVisibility(bool visible)
{
	m_dwRenderer->widget3D->setVisible(visible);
}

std::shared_ptr<iADataSet> MdiChild::dataSet(size_t dataSetIdx) const
{
	if (m_dataSets.find(dataSetIdx) != m_dataSets.end())
	{
		return m_dataSets.at(dataSetIdx);
	}
	return {};
}

size_t MdiChild::dataSetIndex(iADataSet const* dataSet) const
{
	auto it = std::find_if(m_dataSets.begin(), m_dataSets.end(), [dataSet](auto const & ds) { return ds.second.get() == dataSet; });
	if (it != m_dataSets.end())
	{
		return it->first;
	}
	return NoDataSet;
}

std::vector<std::shared_ptr<iADataSet>> MdiChild::dataSets() const
{
	std::vector<std::shared_ptr<iADataSet>> result;
	result.reserve(m_dataSets.size());
	std::transform(m_dataSets.begin(), m_dataSets.end(), std::back_inserter(result), [](auto const& p) { return p.second; });
	return result;
}

std::map<size_t, std::shared_ptr<iADataSet>> const& MdiChild::dataSetMap() const
{
	return m_dataSets;
}

void MdiChild::applyRenderSettings(size_t dataSetIdx, QVariantMap const& renderSettings)
{
	m_dataSetViewers[dataSetIdx]->setAttributes(joinValues(extractValues(m_dataSetViewers[dataSetIdx]->attributesWithValues()), renderSettings));
}

size_t MdiChild::firstImageDataSetIdx() const
{
	for (auto dataSet : m_dataSets)
	{
		auto imgData = dynamic_cast<iAImageData*>(dataSet.second.get());
		if (imgData)
		{
			return dataSet.first;
		}
	}
	return NoDataSet;
}

vtkSmartPointer<vtkImageData> MdiChild::firstImageData() const
{
	for (auto dataSet : m_dataSets)
	{
		auto imgData = dynamic_cast<iAImageData*>(dataSet.second.get());
		if (imgData)
		{
			return imgData->vtkImage();
		}
	}
	LOG(lvlError, "No image/volume data loaded!");
	return nullptr;
}

iADataSetViewer* MdiChild::dataSetViewer(size_t idx) const
{
	return m_dataSetViewers.at(idx).get();
}

bool MdiChild::hasUnsavedData() const
{
	for (auto dataSet: m_dataSets)
	{
		QString fn = dataSet.second->metaData(iADataSet::FileNameKey).toString();
		if (fn.isEmpty() || !QFileInfo(fn).exists())
		{
			return true;
		}
	}
	return false;
}

void MdiChild::saveSettings(QSettings& settings)
{
	// move to iARenderer, create saveSettings method there?
	// {
	auto cam = renderer()->renderer()->GetActiveCamera();
	settings.setValue(CameraPositionKey, arrayToString(cam->GetPosition(), 3));
	settings.setValue(CameraFocalPointKey, arrayToString(cam->GetFocalPoint(), 3));
	settings.setValue(CameraViewUpKey, arrayToString(cam->GetViewUp(), 3));
	settings.setValue(CameraParallelScale, cam->GetParallelScale());
	settings.setValue(CameraParallelProjection, cam->GetParallelProjection());
	// }
}

void MdiChild::loadSettings(QSettings const& settings)
{
	// move to iARenderer, create loadSettings method there?
	// {
	double camPos[3], camFocalPt[3], camViewUp[3];
	if (!stringToArray<double>(settings.value(CameraPositionKey).toString(), camPos, 3) ||
		!stringToArray<double>(settings.value(CameraFocalPointKey).toString(), camFocalPt, 3) ||
		!stringToArray<double>(settings.value(CameraViewUpKey).toString(), camViewUp, 3))
	{
		LOG(lvlWarn, QString("Invalid or missing camera information."));
		return;
	}
	auto cam = renderer()->renderer()->GetActiveCamera();
	bool parProj = settings.value(CameraParallelProjection, cam->GetParallelProjection()).toBool();
	double parScale = settings.value(CameraParallelScale, cam->GetParallelScale()).toDouble();
	cam->SetPosition(camPos);
	cam->SetViewUp(camViewUp);
	cam->SetPosition(camPos);
	cam->SetParallelProjection(parProj);
	cam->SetParallelScale(parScale);
	// }
}

bool MdiChild::doSaveProject(QString const & projectFileName)
{
	QVector<size_t> unsavedDataSets;
	for (auto d: m_dataSets)
	{
		if (!d.second->hasMetaData(iADataSet::FileNameKey))
		{
			unsavedDataSets.push_back(d.first);
		}
	}
	if (!unsavedDataSets.isEmpty())
	{
		auto result = QMessageBox::question(m_mainWnd, "Unsaved datasets",
			"Before saving as a project, some unsaved datasets need to be saved first. Should I choose a filename automatically (in same folder as project file)?",
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
		if (result == QMessageBox::Cancel)
		{
			return false;
		}
		QFileInfo fi(projectFileName);
		for (size_t dataSetIdx : unsavedDataSets)
		{
			bool saveSuccess = true;
			bool saved = false;
			if (result == QMessageBox::Yes)
			{
				// try auto-creating; but if file exists, let user choose!
				QString fileName = fi.absoluteFilePath() + QString("dataSet%1.%2").arg(dataSetIdx).arg(iAFileTypeRegistry::defaultExtension(m_dataSets[dataSetIdx]->type()));
				if (!QFileInfo::exists(fileName))
				{
					saveSuccess = saveNew(m_dataSets[dataSetIdx], fileName);
					saved = true;
				}
			}
			if (!saved)
			{
				saveSuccess = saveNew(m_dataSets[dataSetIdx]);
			}
			if (!saveSuccess)
			{
				return false;
			}
		}
	}
	auto s = std::make_shared<QSettings>(projectFileName, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	s.setIniCodec("UTF-8");
#endif
	s->setValue("UseMdiChild", true);
	saveSettings(*s.get());
	iAProjectFileIO io;
	auto dataSets = std::make_shared<iADataCollection>(m_dataSets.size(), s);
	for (auto d : m_dataSets)
	{
		dataSets->addDataSet(d.second);
	}
	io.save(projectFileName, dataSets, QVariantMap());
	setWindowTitleAndFile(projectFileName);
	// TODO NEWIO: store viewer settings with datasets!

	// store settings of any open tool:
	for (auto toolKey : m_tools.keys())
	{
		s->beginGroup(toolKey);
		m_tools[toolKey]->saveState(*s.get(), projectFileName);
		s->endGroup();
	}
	return true;
}

void MdiChild::addTool(QString const& key, std::shared_ptr<iATool> tool)
{
	m_tools.insert(key, tool);
}

void MdiChild::removeTool(QString const& key)
{
	m_tools.remove(key);
}

QMap<QString, std::shared_ptr<iATool>> const& MdiChild::tools()
{
	return m_tools;
}

MdiChild::iAInteractionMode MdiChild::interactionMode() const
{
	return m_interactionMode;
}

void MdiChild::setInteractionMode(iAInteractionMode mode)
{
	m_interactionMode = mode;
	m_mainWnd->updateInteractionModeControls(mode);
	for (auto v : m_dataSetViewers)
	{
		v.second->setPickActionVisible(mode == imRegistration);
	}
	try
	{
		if (m_interactionMode == imRegistration)
		{
			size_t dataSetIdx = NoDataSet;
			for (auto dataSet: m_dataSets)
			{
				if (dynamic_cast<iAImageData*>(dataSet.second.get()) &&
					m_dataSetViewers.find(dataSet.first) != m_dataSetViewers.end() &&
					m_dataSetViewers[dataSet.first]->renderer()->isPickable())
				{
					dataSetIdx = dataSet.first;
				}
			}
			if (dataSetIdx == NoDataSet)
			{
				LOG(lvlError, QString("No valid dataset loaded for moving (%1).").arg(dataSetIdx));
			}
			else
			{
				auto editDataSet = m_dataSets[dataSetIdx];
				setDataSetMovable(dataSetIdx);
			}
			renderer()->interactor()->SetInteractorStyle(m_manualMoveStyle[iASlicerMode::SlicerCount]);
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				slicer(i)->interactor()->SetInteractorStyle(m_manualMoveStyle[i]);
			}
		}
		else
		{
			renderer()->setDefaultInteractor();
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				slicer(i)->setDefaultInteractor();
			}
		}
	}
	catch (std::invalid_argument& ivae)
	{
		LOG(lvlError, ivae.what());
	}
}

void MdiChild::setDataSetMovable(size_t dataSetIdx)
{
	for (auto ds: m_dataSetViewers)
	{
		bool pickable = (ds.first == dataSetIdx);
		ds.second->setPickable(pickable);
	}

	// below required for synchronized slicers
	auto imgData = dynamic_cast<iAImageData*>(m_dataSets[dataSetIdx].get());
	if (!imgData)
	{
		LOG(lvlError, "Selected dataset is not an image.");
		return;
	}
	assert(m_dataSetViewers.find(dataSetIdx) != m_dataSetViewers.end());
	auto img = imgData->vtkImage();
	uint chID = m_dataSetViewers[dataSetIdx]->slicerChannelID();
	iAChannelSlicerData* props[3];
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (!slicer(i)->hasChannel(chID))
		{
			LOG(lvlWarn, "This modality cannot be moved as it isn't active in slicer, please select another one!");
			return;
		}
		else
		{
			props[i] = slicer(i)->channel(chID);
		}
	}
	for (int i = 0; i <= iASlicerMode::SlicerCount; ++i)
	{
		m_manualMoveStyle[i]->initialize(img, m_dataSetViewers[dataSetIdx]->renderer(), props, i);
	}
}

void MdiChild::styleChanged()
{
	if (renderSettings().UseStyleBGColor)
	{
		m_renderer->setBackgroundColors(m_renderSettings);
		m_renderer->update();
	}
}
