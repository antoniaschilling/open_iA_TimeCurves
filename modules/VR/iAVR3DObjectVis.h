/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkDataSet.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkGlyph3DMapper.h>
#include <QColor>

#include "iAVROctree.h"
#include "vtkCellLocator.h"
#include "vtkPropPicker.h"

//! Base class for VR 3D visualizations of primtive objects
class iAVR3DObjectVis
{
public:
	iAVR3DObjectVis(vtkRenderer* ren);
	void show();
	void hide();
	void createModelInMiniature();
	void createCube(QColor col, double size[3], double center[3]);
	void createSphere(QColor col);
	void setScale(double x, double y, double z);
	void setPos(double x, double y, double z);
	void setOrientation(double x, double y, double z);
	void setCubeColor(QColor col, int regionID);
	void applyHeatmapColoring(std::vector<QColor>* colorPerRegion);
	void applyLinearCubeOffset(double offset);
	void applyRelativeCubeOffset(double offset);
	void apply4RegionCubeOffset(double offset);
	vtkIdType getClosestCellID(double pos[3], double eventOrientation[3]);
	void setOctree(iAVROctree* octree);
	vtkSmartPointer<vtkPolyData> getDataSet();
	vtkActor* getActor();

	double defaultActorSize[3]; // Initial resize of the cube
private:
	QColor defaultColor;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkDataSet> m_dataSet;
	vtkSmartPointer<vtkPolyData> m_cubePolyData;
	vtkSmartPointer<vtkGlyph3D> glyph3D;
	vtkSmartPointer<vtkCellLocator> cellLocator;
	vtkSmartPointer<vtkUnsignedCharArray> currentColorArr;
	iAVROctree* m_octree;
	bool m_visible;

	void calculateStartPoints();
	void iAVR3DObjectVis::drawPoint(std::vector<double*>* pos, QColor color);
};