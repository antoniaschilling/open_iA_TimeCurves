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
#include "iATLGICTLoader.h"

#include "iAJobListView.h"
#include "iALog.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAMultiStepProgressObserver.h"
#include "iAParameterDlg.h"
#include "iAStringHelper.h"
#include "iAFileUtils.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkImageReader2.h>
#include <vtkTIFFReader.h>
#include <vtkBMPReader.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkStringArray.h>

#include <QDir>
#include <QSettings>


iATLGICTLoader::iATLGICTLoader():
	m_multiStepObserver(nullptr)
{}


bool iATLGICTLoader::setup(QString const& baseDirectory, QWidget* parent)
{
	m_baseDirectory = baseDirectory;

	if (m_baseDirectory.isEmpty())
	{
		return false;
	}
	QDir dir(m_baseDirectory);
	QStringList nameFilter;
	nameFilter << "*_Rec";
	m_subDirs = dir.entryInfoList(nameFilter, QDir::Dirs | QDir::NoDotAndDotDot);
	if (m_subDirs.size() == 0)
	{
		LOG(lvlError, "No data found (expected to find subfolders with _Rec suffix).");
		return false;
	}

	QStringList logFileFilter;
	logFileFilter << "*.log";
	QFileInfoList logFiles = dir.entryInfoList(logFileFilter, QDir::Files);
	if (logFiles.size() == 0)
	{
		LOG(lvlError, "No log file found (expected to find a file with .log suffix).");
		return false;
	}
	QSettings iniLog(logFiles[0].absoluteFilePath(), QSettings::IniFormat);
	double pixelSize = iniLog.value("Reconstruction/Pixel Size (um)", 1000).toDouble() / 1000;
	iAParameterDlg::ParamListT params;
	addParameter(params, "Spacing X", iAValueType::Continuous, pixelSize);
	addParameter(params, "Spacing Y", iAValueType::Continuous, pixelSize);
	addParameter(params, "Spacing Z", iAValueType::Continuous, pixelSize);
	addParameter(params, "Origin X", iAValueType::Continuous, 0);
	addParameter(params, "Origin Y", iAValueType::Continuous, 0);
	addParameter(params, "Origin Z", iAValueType::Continuous, 0);
	iAParameterDlg dlg(parent, "Set file parameters", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	auto values = dlg.parameterValues();
	m_spacing[0] = values["Spacing X"].toDouble();
	m_spacing[1] = values["Spacing Y"].toDouble();
	m_spacing[2] = values["Spacing Z"].toDouble();
	m_origin[0] = values["Origin X"].toDouble();
	m_origin[1] = values["Origin Y"].toDouble();
	m_origin[2] = values["Origin Z"].toDouble();
	return true;
}

void iATLGICTLoader::start(MdiChild* child)
{
	m_multiStepObserver = new iAMultiStepProgressObserver(m_subDirs.size());
	m_child = child;
	m_child->show();
	LOG(lvlInfo, tr("Loading TLGI-CT data."));
	iAJobListView::get()->addJob("Loading TLGI-CT data", m_multiStepObserver->progressObject(), this);
	connect(this, &iATLGICTLoader::finished, this, &iATLGICTLoader::finishUp);		// this needs to be last, as it deletes this object!
	QThread::start();
}

iATLGICTLoader::~iATLGICTLoader()
{
	delete m_multiStepObserver;
}

void iATLGICTLoader::run()
{
	m_modList = QSharedPointer<iAModalityList>::create();
	QStringList imgFilter;
	imgFilter << "*.tif" << "*.bmp" << "*.jpg" << "*.png";
	int completedDirs = 0;
	for (QFileInfo subDirFileInfo : m_subDirs)
	{
		QDir subDir(subDirFileInfo.absoluteFilePath());
		subDir.setFilter(QDir::Files);
		subDir.setNameFilters(imgFilter);
		QFileInfoList imgFiles = subDir.entryInfoList();
		QString fileNameBase;
		// determine most common file name base
		// TODO: merge with image stack file name guessing!
		for (QFileInfo imgFileInfo : imgFiles)
		{
			if (fileNameBase.isEmpty())
			{
				fileNameBase = imgFileInfo.absoluteFilePath();
			}
			else
			{
				fileNameBase = greatestCommonPrefix(fileNameBase, imgFileInfo.absoluteFilePath());
			}
		}
		int baseLength = fileNameBase.length();
		// determine index range:
		int min = std::numeric_limits<int>::max();
		int max = std::numeric_limits<int>::min();
		QString ext;
		int digits = -1;
		for (QFileInfo imgFileInfo : imgFiles)
		{
			QString imgFileName = imgFileInfo.absoluteFilePath();
			QString completeSuffix = imgFileInfo.completeSuffix();
			QString lastDigit = imgFileName.mid(imgFileName.length() - (completeSuffix.length() + 2), 1);
			bool ok;
			/*int myNum =*/ lastDigit.toInt(&ok);
			if (!ok)
			{
				//LOG(lvlInfo, QString("Skipping image with no number at end '%1'.").arg(imgFileName));
				continue;
			}
			if (ext.isEmpty())
			{
				ext = completeSuffix;
			}
			else
			{
				if (ext != completeSuffix)
				{
					LOG(lvlInfo, QString("Inconsistent file suffix: %1 has %2, previous files had %3.").arg(imgFileName).arg(completeSuffix).arg(ext));
					return;
				}
			}

			QString numStr = imgFileName.mid(baseLength, imgFileName.length() - baseLength - completeSuffix.length() - 1);
			if (digits == -1)
			{
				digits = numStr.length();
			}

			int num = numStr.toInt(&ok);
			if (!ok)
			{
				LOG(lvlInfo, QString("Invalid, non-numeric part (%1) in image file name '%2'.").arg(numStr).arg(imgFileName));
				return;
			}
			if (num < min)
			{
				min = num;
			}
			if (num > max)
			{
				max = num;
			}
		}

		if (max - min + 1 > imgFiles.size())
		{
			LOG(lvlInfo, QString("Stack loading: not all indices in the interval [%1, %2] are used for base name %3.").arg(min).arg(max).arg(fileNameBase));
			return;
		}

		vtkSmartPointer<vtkStringArray> fileNames = vtkSmartPointer<vtkStringArray>::New();
		for (int i = min; i <= max; i++)
		{
			QString temp = fileNameBase + QString("%1").arg(i, digits, 10, QChar('0')) + "." + ext;
			temp = temp.replace("/", "\\");
			fileNames->InsertNextValue(getLocalEncodingFileName(temp));
		}

		// load image stack // TODO: put to common location and use from iAIO!
		ext = ext.toLower();
		vtkSmartPointer<vtkImageReader2> reader;
		if (ext == "jpg" || ext == "jpeg")
		{
			reader = vtkSmartPointer<vtkJPEGReader>::New();
		}
		else if (ext == "png")
		{
			reader = vtkSmartPointer<vtkPNGReader>::New();
		}
		else if (ext == "bmp")
		{
			reader = vtkSmartPointer<vtkBMPReader>::New();
		}
		else if (ext == "tif" || ext == "tiff")
		{
			reader = vtkSmartPointer<vtkTIFFReader>::New();
		}
		else
		{
			LOG(lvlError, QString("Unknown or undefined image extension (%1)!").arg(ext));
			return;
		}
		reader->SetFileNames(fileNames);
		reader->SetDataOrigin(m_origin);
		reader->SetDataSpacing(m_spacing);
		m_multiStepObserver->observe(reader);		// intercept progress and divide by number of images!
		reader->Update();
		vtkSmartPointer<vtkImageData> img = reader->GetOutput();

		// add modality
		QString modName = subDirFileInfo.baseName();
		modName = modName.left(modName.length() - 4); // 4 => length of "_rec"
		m_modList->add(QSharedPointer<iAModality>::create(modName, subDirFileInfo.absoluteFilePath(), -1, img, 0));
		m_multiStepObserver->setCompletedSteps(++completedDirs);
	}
	if (m_modList->size() == 0)
	{
		LOG(lvlError, "No modalities loaded!");
		return;
	}
}

void iATLGICTLoader::finishUp()
{
	m_child->setCurrentFile(m_baseDirectory);
	m_child->setModalities(m_modList);
	LOG(lvlInfo, tr("Loading sequence completed; directory: %1.").arg(m_baseDirectory));
	delete this;
}
