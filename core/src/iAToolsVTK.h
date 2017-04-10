/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

class vtkImageData;

class QString;

// image creation:
void DeepCopy(vtkSmartPointer<vtkImageData> input, vtkSmartPointer<vtkImageData> output);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(vtkSmartPointer<vtkImageData> img);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int dimensions[3], double spacing[3]);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int dimensions[3], double spacing[3], int numComponents);

// image I/O (using ITK methods of iAITKIO)
void StoreImage(vtkSmartPointer<vtkImageData> image, QString const & filename, bool useCompression = true);
vtkSmartPointer<vtkImageData> ReadImage(QString const & filename, bool releaseFlag);

void WriteSingleSliceImage(QString const & filename, vtkImageData* imageData);


#define FOR_VTKIMG_PIXELS(img, x, y, z) \
    int * dims = img->GetDimensions(); \
    for (int x = 0; x < dims[0]; ++x) \
        for (int y = 0; y < dims[1]; ++y) \
            for (int z = 0; z < dims[2]; ++z)
