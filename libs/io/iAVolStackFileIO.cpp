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
#include "iAVolStackFileIO.h"

#include <iAProgress.h>
#include <iASettings.h>    // for mapFromQSettings

#include "iAFileStackParams.h"
#include "iAFileTypeRegistry.h"
#include "iAFileUtils.h"
#include "iAValueTypeVectorHelpers.h"

#include <QFileInfo>
#include <QSettings>

namespace
{
	static const QString ProjectFileVersionKey("FileVersion");
	static const QString ProjectFileVersionValue("1.0");
	static const QString IndexRange("Index Range");

	static const QString FileKeyFileNameBase("file_names_base");
	static const QString FileKeyExtension("extension");
	static const QString FileKeyNumOfDigits("number_of_digits_in_index");
	static const QString FileKeyMinIdx("minimum_index");
	static const QString FileKeyMaxIdx("maximum_index");

	// TODO NEWIO: currently only used here, with same separator, check if it can be reused somewhere
	// similar function (getParameterValues) was used in iAIO for other uses too:
	//     - PARS reader (not in new framework yet)
	//     - VGI reader (now using QSettings instead)

	QMap<QString, QString> readSettingsFile(QString const& fileName, QString const & keyValueSeparator = ":")
	{
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
		{
			throw std::runtime_error(QString("Could not open file %1").arg(fileName).toStdString());
		}
		QTextStream textStream(&file);
		QString currentSection;
		QMap<QString, QString> result;
		int lineNr = 0;
		while (!textStream.atEnd())
		{
			++lineNr;
			QString currentLine = textStream.readLine();
			if (currentLine.trimmed().isEmpty())
			{
				continue;
			}
			if (currentLine.indexOf("[") == 0 && currentLine.indexOf("]") > 0)
			{
				currentSection = currentLine.mid(1, currentLine.indexOf("]")-2);
			}
			else
			{
				auto tokens = currentLine.split(keyValueSeparator);
				if (tokens.size() != 2)
				{
					throw std::runtime_error(QString("Invalid key/value line (#%1: %2) - could not split at separator %3")
						.arg(lineNr).arg(currentLine).arg(keyValueSeparator).toStdString());
				}
				auto key = (currentSection.isEmpty() ? "" : currentSection + "/") + tokens[0].trimmed();
				auto value = tokens[1].trimmed();
				result[key] = value;
			}
		}
		// file closed automatically by ~QFile
		return result;
	}
}

const QString iAVolStackFileIO::Name("Volume Stack descriptor");

iAVolStackFileIO::iAVolStackFileIO() : iAFileIO(iADataSetType::All, iADataSetType::None)
{
	addAttr(m_params[Save], iAFileStackParams::FileNameBase, iAValueType::String, "");
	addAttr(m_params[Save], iAFileStackParams::Extension, iAValueType::String, "");
	addAttr(m_params[Save], iAFileStackParams::NumDigits, iAValueType::Discrete, 0);
	addAttr(m_params[Save], iAFileStackParams::MinimumIndex, iAValueType::Discrete, 0);
	addAttr(m_params[Save], iAFileIO::CompressionStr, iAValueType::Boolean, false);
	//addAttr(m_params[Save], iAFileStackParams::MaximumIndex, iAValueType::Discrete, 0);    // not needed, follows from min idx and num of datasets
}

std::shared_ptr<iADataSet> iAVolStackFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);

	QFileInfo fi(fileName);
	auto volStackSettings = readSettingsFile(fileName);
	auto fileNameBase = fi.absolutePath() + "/" + volStackSettings[FileKeyFileNameBase];
	auto extension = volStackSettings[FileKeyExtension];
	bool ok;
	int digitsInIndex = volStackSettings[FileKeyNumOfDigits].toInt(&ok);
	if (!ok || digitsInIndex < 0)
	{
		throw std::runtime_error(QString("VolStack I/O: Invalid value (%1) for %2 - not a number or smaller 0!")
			.arg(volStackSettings[FileKeyNumOfDigits]).arg(FileKeyNumOfDigits).toStdString());
	}
	bool ok1, ok2;
	int minIdx = volStackSettings[FileKeyMinIdx].toInt(&ok1);
	int maxIdx = volStackSettings[FileKeyMaxIdx].toInt(&ok2);
	if (!ok1 || !ok2 || minIdx >= maxIdx)
	{
		throw std::runtime_error(QString("VolStack I/O: Invalid index range %1 - %2 (invalid numbers or min >= max).")
			.arg(volStackSettings[FileKeyMinIdx]).arg(volStackSettings[FileKeyMaxIdx]).toStdString());
	}
	auto result = std::make_shared<iADataCollection>(maxIdx - minIdx + 1, std::shared_ptr<QSettings>());
	for (auto const & key : volStackSettings.keys())
	{
		result->setMetaData(key, volStackSettings[key]);
	}

	// use iAMultiStepProgress?
	for (int i = minIdx; i <= maxIdx; ++i)
	{
		QString curFileName = fileNameBase + QString("%1").arg(i, digitsInIndex, 10, QChar('0')) + extension;
		auto io = iAFileTypeRegistry::createIO(curFileName, iAFileIO::Load);
		if (!io)
		{
			throw std::runtime_error(QString("VolStack I/O: Cannot read file (%1) - no suitable reader found!").arg(curFileName).toStdString());
		}
		if (io->parameter(iAFileIO::Load).size() != 1 || io->parameter(iAFileIO::Load)[0]->name() != iADataSet::FileNameKey)
		{
			throw std::runtime_error(QString("VolStack I/O: Cannot read file (%1) - reader requires other parameters !").arg(curFileName).toStdString());
		}
		iAProgress dummyProgress;
		QVariantMap curParamValues;
		auto dataSet = io->load(curFileName, curParamValues, dummyProgress);
		result->addDataSet(dataSet);
		progress.emitProgress(100 * (i - minIdx) / (maxIdx - minIdx + 1));
	}
	return result;
}

void iAVolStackFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	auto collection = dynamic_cast<iADataCollection*>(dataSet.get());
	if (!collection)
	{
		LOG(lvlError, "VolStack I/O save called with dataset which is not a collection of datasets!");
	}
	Q_UNUSED(paramValues);
	QFile volstackFile(fileName);
	auto fileNameBase = paramValues[iAFileStackParams::FileNameBase].toString();
	QFileInfo fi(fileName);
	if (fileNameBase.isEmpty())
	{
		LOG(lvlWarn, QString("Empty %1, setting to %2").arg(iAFileStackParams::FileNameBase).arg(fi.completeBaseName()));
		fileNameBase = fi.completeBaseName();
		paramValues[iAFileStackParams::FileNameBase] = fileNameBase;
	}
	auto extension = paramValues[iAFileStackParams::Extension].toString();
	if (extension.isEmpty())
	{
		const QString DefaultExtension = ".mhd";
		LOG(lvlWarn, QString("Empty %1, setting to %2").arg(iAFileStackParams::Extension).arg(DefaultExtension));
		extension = DefaultExtension;
		paramValues[iAFileStackParams::Extension] = extension;
	}
	int numOfDigits = paramValues[iAFileStackParams::NumDigits].toInt();
	int minIdx = paramValues[iAFileStackParams::MinimumIndex].toInt();
	int maxIdx = paramValues[iAFileStackParams::MaximumIndex].toInt();
	int expectedMaxIdx = (minIdx + collection->dataSets().size() - 1);
	if (maxIdx != expectedMaxIdx)
	{
		LOG(lvlWarn, QString("Invalid value for %1; expected %2 (number of datasets given), but was %3").arg(iAFileStackParams::MaximumIndex).arg(expectedMaxIdx).arg(maxIdx));
		maxIdx = expectedMaxIdx;
		paramValues[iAFileStackParams::MaximumIndex] = maxIdx;
	}
	if (volstackFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&volstackFile);
		for (auto const & key : paramValues.keys())
		{
			out << key << ": " << paramValues[key].toString() << "\n";
		}
	}
	//// write mhd images:
	for (int m = 0; m < collection->dataSets().size(); m++)
	{
		QString curFileName = fi.absolutePath() + "/" + fileNameBase + QString("%1").arg(minIdx + m, numOfDigits, 10, QChar('0')) + extension;
		auto io = iAFileTypeRegistry::createIO(curFileName, iAFileIO::Save);
		if (!io)
		{
			LOG(lvlError, QString("Could not find a writer suitable for file name %1!").arg(curFileName));
			return;
		}
		iAProgress dummyProgress;
		QVariantMap curParamValues;
		curParamValues[iAFileIO::CompressionStr] = paramValues[iAFileIO::CompressionStr];
		io->save(curFileName, collection->dataSets()[m], curParamValues, dummyProgress);
		progress.emitProgress(m * 100.0 / collection->dataSets().size());
	}
}

QString iAVolStackFileIO::name() const
{
	return Name;
}

QStringList iAVolStackFileIO::extensions() const
{
	return QStringList{ "volstack" };
}
