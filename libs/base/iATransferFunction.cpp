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
#include "iATransferFunction.h"

#include "iALog.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QString>    // to be able to create QString from const char *

iATransferFunction::~iATransferFunction()
{}

void defaultColorTF(vtkSmartPointer<vtkColorTransferFunction> cTF, double const range[2])
{
	cTF->RemoveAllPoints();
	cTF->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
	cTF->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
	cTF->Build();
}

void defaultOpacityTF(vtkSmartPointer<vtkPiecewiseFunction> pWF, double const range[2], bool opacityRamp)
{
	pWF->RemoveAllPoints();
	pWF->AddPoint(range[0], opacityRamp ? 0.0 : 1.0);
	pWF->AddPoint(range[1], 1.0);
}

vtkSmartPointer<vtkColorTransferFunction> defaultColorTF(double const range[2])
{
	auto cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	defaultColorTF(cTF, range);
	return cTF;
}

vtkSmartPointer<vtkPiecewiseFunction> defaultOpacityTF(double const range[2], bool opacityRamp)
{
	auto pWF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	defaultOpacityTF(pWF, range, opacityRamp);
	return pWF;
}