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
#include "iAFileTypeRegistry.h"

#include "iAFileIO.h"

std::vector<std::shared_ptr<iAIFileIOFactory>> iAFileTypeRegistry::m_fileIOs;
QMap<QString, size_t> iAFileTypeRegistry::m_fileTypes;

std::shared_ptr<iAFileIO> iAFileTypeRegistry::createIO(QString const& fileExtension)
{
	auto ext = fileExtension.toLower();
	if (m_fileTypes.contains(ext))
	{
		return m_fileIOs[m_fileTypes[ext]]->create();
	}
	else
	{
		return std::shared_ptr<iAFileIO>();
	}
}

QString iAFileTypeRegistry::registeredLoadFileTypes(iADataSetTypes allowedTypes)
{
	QStringList allExtensions;
	QString singleTypes;
	for (auto ioFactory : m_fileIOs)  // all registered file types
	{
		auto io = ioFactory->create();
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
		if ( (io->supportedReadDataSetTypes() & allowedTypes) == 0 )
#else
		if (!io->supportedLoadDataSetTypes().testAnyFlags(allowedTypes))
#endif
		{   // current I/O does not support any of the allowed types
			continue;
		}
		auto extCpy = io->extensions();
		for (auto & ext: extCpy)
		{
			ext = "*." + ext;
		}
		singleTypes += QString("%1 (%2);;").arg(io->name()).arg(extCpy.join(" "));
		allExtensions.append(extCpy);
	}
	if (singleTypes.isEmpty())
	{
		LOG(lvlWarn, "No matching registered file types found!");
		return ";;";
	}
	return QString("Any supported format (%1);;").arg(allExtensions.join(" ")) + singleTypes;
}

QString iAFileTypeRegistry::registeredSaveFileTypes(iADataSetType type)
{
	QStringList allExtensions;
	QString singleTypes;
	for (auto ioFactory : m_fileIOs)  // all registered file types
	{
		auto io = ioFactory->create();
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
		if ((io->supportedSaveDataSetTypes() & allowedTypes) == 0)
#else
		if (!io->supportedSaveDataSetTypes().testFlag(type))
#endif
		{   // current I/O does not support any of the allowed types
			continue;
		}
		auto extCpy = io->extensions();
		for (auto& ext : extCpy)
		{
			ext = "*." + ext;
		}
		singleTypes += QString("%1 (%2);;").arg(io->name()).arg(extCpy.join(" "));
		allExtensions.append(extCpy);
	}
	if (singleTypes.isEmpty())
	{
		LOG(lvlWarn, "No matching registered file types found!");
		return ";;";
	}
	return QString("Any supported format (%1);;").arg(allExtensions.join(" ")) + singleTypes;
}

// ---------- iAFileTypeRegistry::setupDefaultIOFactories (needs to be after declaration of specific IO classes) ----------

#include "iAAmiraVolumeFileIO.h"
#include "iACSVImageFileIO.h"
#include "iADCMFileIO.h"
#include "iAGraphFileIO.h"
#include "iAHDF5IO.h"
#include "iAImageStackFileIO.h"
#include "iAMetaFileIO.h"
#include "iANKCFileIO.h"
#include "iAProjectFileIO.h"
#include "iAOIFFileIO.h"
#include "iARawFileIO.h"
#include "iASTLFileIO.h"
#include "iAVTIFileIO.h"
#include "iAVTKFileIO.h"

void iAFileTypeRegistry::setupDefaultIOFactories()
{
	// volume file formats:
	iAFileTypeRegistry::addFileType<iACSVImageFileIO>();
	iAFileTypeRegistry::addFileType<iAAmiraVolumeFileIO>();
	iAFileTypeRegistry::addFileType<iADCMFileIO>();
	iAFileTypeRegistry::addFileType<iAImageStackFileIO>();
	iAFileTypeRegistry::addFileType<iAMetaFileIO>();
	iAFileTypeRegistry::addFileType<iANKCFileIO>();
	iAFileTypeRegistry::addFileType<iAOIFFileIO>();
	iAFileTypeRegistry::addFileType<iAVTIFileIO>();
	iAFileTypeRegistry::addFileType<iARawFileIO>();
#ifdef USE_HDF5
	iAFileTypeRegistry::addFileType<iAHDF5IO>();
#endif

	// mesh file formats:
	iAFileTypeRegistry::addFileType<iASTLFileIO>();

	// graph file formats:
	iAFileTypeRegistry::addFileType<iAGraphFileIO>();

	// file formats which can contain different types of data:
	iAFileTypeRegistry::addFileType<iAVTKFileIO>();
	
	// collection file formats:
	iAFileTypeRegistry::addFileType<iAProjectFileIO>();
}

#include <QFileInfo>

namespace iANewIO
{
	std::shared_ptr<iAFileIO> createIO(QString fileName)
	{
		QFileInfo fi(fileName);
		// special handling for directory ? TLGICT-loader... -> fi.isDir();
		auto io = iAFileTypeRegistry::createIO(fi.suffix());
		if (!io)
		{
			LOG(lvlWarn,
				QString("Failed to load %1: There is no handler registered files with suffix '%2'")
					.arg(fileName)
					.arg(fi.suffix()));
		}
		// for file formats that support multiple dataset types: check if an allowed type was loaded?
		// BUT currently no such format supported
		return io;
	}
}
