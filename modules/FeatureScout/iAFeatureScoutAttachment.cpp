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
#include "iAFeatureScoutAttachment.h"

#include "dlg_FeatureScout.h"

iAFeatureScoutAttachment::iAFeatureScoutAttachment(iAMainWindow* mainWnd, iAMdiChild * child) :
	iAModuleAttachmentToChild(mainWnd, child)
{
}

void iAFeatureScoutAttachment::init(int filterID, QString const & fileName, vtkSmartPointer<vtkTable> csvtbl,
	int visType, QSharedPointer<QMap<uint, uint> > columnMapping, std::map<size_t,
	std::vector<iAVec3f> > & curvedFiberInfo, int cylinderQuality, size_t segmentSkip)
{
	imgFS = new dlg_FeatureScout(m_child, static_cast<iAObjectType>(filterID),
		fileName, csvtbl, visType, columnMapping, curvedFiberInfo, cylinderQuality, segmentSkip);
}

void iAFeatureScoutAttachment::FeatureScout_Options(int idx)
{
	if (!imgFS)
	{
		return;
	}
	imgFS->changeFeatureScout_Options(idx);
}

void iAFeatureScoutAttachment::saveProject(QSettings& projectFile)
{
	imgFS->saveProject(projectFile);
}

void iAFeatureScoutAttachment::loadProject(QSettings& projectFile)
{
	imgFS->loadProject(projectFile);
}
