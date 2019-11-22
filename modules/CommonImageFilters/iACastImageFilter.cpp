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
#include "iACastImageFilter.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>    // for castImageTo
#include <iAToolsVTK.h>    // for VTKDataTypeList
#include <iATypedCallHelper.h>
#include <itkFHWRescaleIntensityImageFilter.h>

#include <itkCastImageFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkLabelToRGBImageFilter.h>
#include <itkRGBPixel.h>
#include <itkRGBAPixel.h>
#include <itkStatisticsImageFilter.h>


template <class InT, class OutT> void castImage(iAFilter* filter)
{
	typedef itk::Image<InT, DIM > InputImageType;
	typedef itk::Image<OutT, DIM> OutputImageType;
	typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
	typename OTIFType::Pointer castFilter = OTIFType::New();
	castFilter->SetInput(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
	filter->progress()->observe(castFilter);
	castFilter->Update();
	filter->addOutput(castFilter->GetOutput());
}



template<class T> void castImage(iAFilter* filter, int vtkType)
{
	switch (vtkType)
	{
	case VTK_UNSIGNED_CHAR:      castImage<T, unsigned char>(filter);  break;
	case VTK_CHAR:
	case VTK_SIGNED_CHAR:        castImage<T, char>(filter);           break;
	case VTK_SHORT:              castImage<T, short>(filter);          break;
	case VTK_UNSIGNED_SHORT:     castImage<T, unsigned short>(filter); break;
	case VTK_INT:                castImage<T, int>(filter);            break;
	case VTK_UNSIGNED_INT:       castImage<T, unsigned int>(filter);   break;
	case VTK_LONG:               castImage<T, long>(filter);           break;
	case VTK_UNSIGNED_LONG:      castImage<T, unsigned long>(filter);  break;
	case VTK_LONG_LONG:          castImage<T, long long>(filter);      break;
	case VTK_UNSIGNED_LONG_LONG: castImage<T, unsigned long long>(filter); break;
	case VTK_FLOAT:              castImage<T, float>(filter);          break;
	case VTK_DOUBLE:             castImage<T, double>(filter);         break;
	default:
		throw std::runtime_error("Invalid datatype in data type conversion!");
	}
}

template <class InT, class OutT>
void dataTypeConversion(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<InT, DIM>   InputImageType;
	typedef itk::Image<OutT, DIM> OutputImageType;
	typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;
	typename RIIFType::Pointer rescaleFilter = RIIFType::New();
	rescaleFilter->SetInput(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
	if (parameters["Automatic Input Range"].toBool())
	{
		double minVal, maxVal;
		getStatistics(filter->input()[0]->itkImage(), &minVal, &maxVal);
		rescaleFilter->SetInputMinimum(minVal);
		rescaleFilter->SetInputMaximum(maxVal);
	}
	else
	{
		rescaleFilter->SetInputMinimum(parameters["Input Min"].toDouble());
		rescaleFilter->SetInputMaximum(parameters["Input Max"].toDouble());
	}
	if (parameters["Use Full Output Range"].toBool())
	{
		rescaleFilter->SetOutputMinimum(std::numeric_limits<OutT>::lowest());
		rescaleFilter->SetOutputMaximum(std::numeric_limits<OutT>::max());
	}
	else
	{
		rescaleFilter->SetOutputMinimum(parameters["Output Min"].toDouble());
		rescaleFilter->SetOutputMaximum(parameters["Output Max"].toDouble());
	}
	filter->progress()->observe(rescaleFilter);
	rescaleFilter->Update();
	filter->addOutput(rescaleFilter->GetOutput());
}

template<class T>
void dataTypeConversion(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	int vtkDataType = mapReadableDataTypeToVTKType(parameters["Data Type"].toString());
	switch (vtkDataType)
	{
		case VTK_UNSIGNED_CHAR:  dataTypeConversion<T, unsigned char>(filter, parameters);  break;
		case VTK_CHAR:
		case VTK_SIGNED_CHAR:    dataTypeConversion<T, char>(filter, parameters);           break;
		case VTK_SHORT:          dataTypeConversion<T, short>(filter, parameters);          break;
		case VTK_UNSIGNED_SHORT: dataTypeConversion<T, unsigned short>(filter, parameters); break;
		case VTK_INT:            dataTypeConversion<T, int>(filter, parameters);            break;
		case VTK_UNSIGNED_INT:   dataTypeConversion<T, unsigned int>(filter, parameters);   break;
		case VTK_LONG:           dataTypeConversion<T, long>(filter, parameters);           break;
		case VTK_UNSIGNED_LONG:  dataTypeConversion<T, unsigned long>(filter, parameters);  break;
		case VTK_LONG_LONG:      dataTypeConversion<T, long long>(filter, parameters);      break;
		case VTK_UNSIGNED_LONG_LONG: dataTypeConversion<T, unsigned long long>(filter, parameters); break;
		case VTK_FLOAT:          dataTypeConversion<T, float>(filter, parameters);          break;
		case VTK_DOUBLE:         dataTypeConversion<T, double>(filter, parameters);         break;
		default:
			throw std::runtime_error("Invalid datatype in data type conversion!");
	}
}

template<class T>
void convertToRGB(iAFilter * filter)
{
	iAITKIO::ImagePointer input = filter->input()[0]->itkImage();
	if (filter->inputPixelType() != itk::ImageIOBase::ULONG)
		input = castImageTo<unsigned long>(input);

	typedef itk::Image< unsigned long, DIM > LongImageType;
	typedef itk::RGBPixel< unsigned char > RGBPixelType;
	typedef itk::Image< RGBPixelType, DIM > RGBImageType;
	typedef itk::RGBAPixel< unsigned char > RGBAPixelType;
	typedef itk::Image< RGBAPixelType, DIM>  RGBAImageType;

	typedef itk::LabelToRGBImageFilter<LongImageType, RGBImageType> RGBFilterType;
	RGBFilterType::Pointer labelToRGBFilter = RGBFilterType::New();
	labelToRGBFilter->SetInput(dynamic_cast<LongImageType *>(input.GetPointer()));
	labelToRGBFilter->Update();

	RGBImageType::RegionType region;
	region.SetSize(labelToRGBFilter->GetOutput()->GetLargestPossibleRegion().GetSize());
	region.SetIndex(labelToRGBFilter->GetOutput()->GetLargestPossibleRegion().GetIndex());

	RGBAImageType::Pointer rgbaImage = RGBAImageType::New();
	rgbaImage->SetRegions(region);
	rgbaImage->SetSpacing(labelToRGBFilter->GetOutput()->GetSpacing());
	rgbaImage->Allocate();

	itk::ImageRegionConstIterator< RGBImageType > cit(labelToRGBFilter->GetOutput(), region);
	itk::ImageRegionIterator< RGBAImageType >     it(rgbaImage, region);
	for (cit.GoToBegin(), it.GoToBegin(); !it.IsAtEnd(); ++cit, ++it)
	{
		it.Value().SetRed(cit.Value().GetRed());
		it.Value().SetBlue(cit.Value().GetBlue());
		it.Value().SetGreen(cit.Value().GetGreen());
		it.Value().SetAlpha(255);
	}
	filter->addOutput(rgbaImage);
}

IAFILTER_CREATE(iACastImageFilter)

void iACastImageFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	if (parameters["Data Type"].toString() == "Label image to color-coded RGBA image")
	{
		ITK_TYPED_CALL(convertToRGB, inputPixelType(), this);
	}
	else if (parameters["Rescale Range"].toBool())
	{
		ITK_TYPED_CALL(dataTypeConversion, inputPixelType(), this, parameters);
	}
	else
	{
		ITK_TYPED_CALL(castImage, inputPixelType(), this,
			mapReadableDataTypeToVTKType(parameters["Data Type"].toString()));
	}
}

iACastImageFilter::iACastImageFilter() :
	iAFilter("Datatype Conversion", "",
		"Converts an image to another datatype.<br/>"
		"<em>Rescale Range</em> determines whether the intensity values are transformed to another range in that process."
		"All parameters below are only considered in the case that Rescale Ranges is enabled; if it is disabled, the "
		"intensity values stay the same (to the extent that they can be represented in the output datatype).<br/>"
		"<em>Input Minimum</em> and <em>Input Maximum</em> allow to specify the start and end of the input range; "
		"if you want to just use the minimum and maximum of the input image here, enable <em>Automatic Input Range</em>."
		"If <em>Use Full Output Range</em> is checked, then <em>Output Minimum</em> and <em>Maximum</em> are ignored, "
		"instead the minimum and maximum possible values of the chosen output datatype are used.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CastImageFilter.html\">"
		"Cast Image Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RescaleIntensityImageFilter.html\">"
		"Rescale Image Filter</a> in the ITK documentation.")
{
	QStringList datatypes = readableDataTypeList(false);
	datatypes << ("Label image to color-coded RGBA image");
	addParameter("Data Type", Categorical, datatypes);
	addParameter("Rescale Range", Boolean, false);
	addParameter("Automatic Input Range", Boolean, false);
	addParameter("Input Min", Continuous, 0);
	addParameter("Input Max", Continuous, 1);
	addParameter("Use Full Output Range", Boolean, true);
	addParameter("Output Min", Continuous, 0);
	addParameter("Output Max", Continuous, 1);
}