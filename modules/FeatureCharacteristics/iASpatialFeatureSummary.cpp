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
#include "iASpatialFeatureSummary.h"

#include "iAAABB.h"
#include "iALog.h"
#include "iAStringHelper.h"    // for stringToVector
#include "iAToolsVTK.h"
#include "iAValueTypeVectorHelpers.h"

#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"

#include <vtkImageData.h>
#include <vtkTable.h>

namespace
{
	const QString CsvFileName("CSV filename");
	const QString Size("Size");
	const QString Columns("Columns");
	const QString MinCorner("Minimum corner");
	const QString MaxCorner("Maximum corner");
}

iASpatialFeatureSummary::iASpatialFeatureSummary():
	iAFilter("Calculate Spatial Fiber Summary", "Feature Characteristics",
		"Compute a spatial summary of characteristics of objects in list (.csv file).<br/>"
		"This filter takes a table of characteristics of each of the features (=objects) "
        "and computes the number of fibers and average characteristics for spatial regions of a defined size.<br/>"
		"As input, the filter requires a feature characteristics table in a .csv file specified by the given <em>" + CsvFileName + "</em>. "
		"It also requires the <em>" + Size + "</em>, i.e. the number of voxels (cubic regions) for which the averages are computed in each direction. "
		//"A .csv configuration is also required, specifying in which .csv columns the <em>Start point</em> and <em>End point</em> of the fibers can be found. "
		    // would require full .csv input spec? for now, let's only use FCP files
		"It also requires a comma-separated list of <em>" + Columns + "</em>, "
		"i.e. the indices of the columns for which the average spatial summaries will be computed (one additional output image per column)."
		"If you want the images to cover a specific extent (i.e. bounding box), "
		"you can specify a <em>" + MinCorner + "</em> and a <em>" + MaxCorner + "</em> "
		"(i.e., minimum and maximum x, y and z coordinates); leave all at 0 for them to be automatically computed from the loaded fibers. "
		"", 0, 1)
{
	// Potential for improvement:
	//     - allow choosing CSV config
	//     - fancy GUI for choosing columns, and displaying the spacing of the resulting dataset for the chosen size
	//     - automatic spacing determination (for fixed size of 50x50x50 or so)
	//     - fiber density as 0..1 value
	addParameter(CsvFileName, iAValueType::FileNameOpen, ".csv");
	addParameter(Size, iAValueType::Vector3i, variantVector<int>({50, 50, 50}));
	addParameter(Columns, iAValueType::String, "7,8,9,10,11,17,18,19,20,21");
	addParameter(MinCorner, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addParameter(MaxCorner, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	setOutputName(0, "Number of fibers");
}

namespace
{
	std::vector<iAVec3i> findCellIdsBresenham3D(iAVec3i const& start, iAVec3i const& end)
	{
		//bresenham 3D
		//http://www.ict.griffith.edu.au/anthony/info/graphics/bresenham.procs

		int i, l, m, n, x_inc, y_inc, z_inc, err_1, err_2, dx2, dy2, dz2;
		int id = 0;
		std::vector<iAVec3i> cellIds;

		iAVec3i pixel = start;
		auto dir = end - start;
		x_inc = (dir.x() < 0) ? -1 : 1;
		l = std::abs(dir.x());
		y_inc = (dir.y() < 0) ? -1 : 1;
		m = std::abs(dir.y());
		z_inc = (dir.z() < 0) ? -1 : 1;
		n = std::abs(dir.z());
		dx2 = l << 1;
		dy2 = m << 1;
		dz2 = n << 1;

		if ((l >= m) && (l >= n))
		{
			err_1 = dy2 - l;
			err_2 = dz2 - l;
			for (i = 0; i < l; i++)
			{
				cellIds.push_back(pixel);
				id++;
				if (err_1 > 0)
				{
					pixel[1] += y_inc;
					err_1 -= dx2;
				}
				if (err_2 > 0)
				{
					pixel[2] += z_inc;
					err_2 -= dx2;
				}
				err_1 += dy2;
				err_2 += dz2;
				pixel[0] += x_inc;
			}
		}
		else if ((m >= l) && (m >= n))
		{
			err_1 = dx2 - m;
			err_2 = dz2 - m;
			for (i = 0; i < m; i++)
			{
				cellIds.push_back(pixel);
				id++;
				if (err_1 > 0)
				{
					pixel[0] += x_inc;
					err_1 -= dy2;
				}
				if (err_2 > 0)
				{
					pixel[2] += z_inc;
					err_2 -= dy2;
				}
				err_1 += dx2;
				err_2 += dz2;
				pixel[1] += y_inc;
			}
		}
		else
		{
			err_1 = dy2 - n;
			err_2 = dx2 - n;
			for (i = 0; i < n; i++)
			{
				cellIds.push_back(pixel);
				id++;
				if (err_1 > 0)
				{
					pixel[1] += y_inc;
					err_1 -= dz2;
				}
				if (err_2 > 0)
				{
					pixel[0] += x_inc;
					err_2 -= dz2;
				}
				err_1 += dy2;
				err_2 += dx2;
				pixel[2] += z_inc;
			}
		}
		cellIds.push_back(pixel);
		id++;

		return cellIds;
	}
}

void iASpatialFeatureSummary::performWork(QVariantMap const & parameters)
{
	QString csvFileName = parameters[CsvFileName].toString();
	auto columns = stringToVector<QVector<int>, int>(parameters[Columns].toString());

	// load csv:
	iACsvIO io;
	iACsvVtkTableCreator tableCreator;
	auto config = iACsvConfig::getFCPFiberFormat(csvFileName);
	if (!io.loadCSV(tableCreator, config))
	{
		LOG(lvlError, QString("Error loading csv file %1!").arg(csvFileName));
		return;
	}
	auto headers = io.getOutputHeaders();
	auto csvTable = tableCreator.table();

	// compute overall bounding box:
	iAVec3i startIdx(config.columnMapping[iACsvConfig::StartX], config.columnMapping[iACsvConfig::StartY], config.columnMapping[iACsvConfig::StartZ]);
	iAVec3i endIdx(config.columnMapping[iACsvConfig::EndX], config.columnMapping[iACsvConfig::EndY], config.columnMapping[iACsvConfig::EndZ]);

	auto minCorner = iAVec3d(parameters[MinCorner].value<QVector<double>>().data());
	auto maxCorner = iAVec3d(parameters[MaxCorner].value<QVector<double>>().data());
	iAVec3d nullVec(0, 0, 0);
	iAAABB overallBB;
	if (minCorner != nullVec || maxCorner != nullVec)
	{
		overallBB.addPointToBox(minCorner);
		overallBB.addPointToBox(maxCorner);
	}
	else
	{
		for (int o = 0; o < csvTable->GetNumberOfRows(); ++o)
		{
			iAVec3d startPoint(csvTable->GetValue(o, startIdx[0]).ToDouble(), csvTable->GetValue(o, startIdx[1]).ToDouble(), csvTable->GetValue(o, startIdx[2]).ToDouble());
			iAVec3d endPoint(csvTable->GetValue(o, endIdx[0]).ToDouble(), csvTable->GetValue(o, endIdx[1]).ToDouble(), csvTable->GetValue(o, endIdx[2]).ToDouble());
			overallBB.addPointToBox(startPoint);
			overallBB.addPointToBox(endPoint);
		}
	}

	// compute image measurements
	auto metaOrigin = overallBB.minCorner().data();
	auto metaDim = parameters[Size].value<QVector<int>>();
	auto metaSpacing = (overallBB.maxCorner() - overallBB.minCorner()) / iAVec3i(metaDim.data());
	LOG(lvlDebug, QString("Creating image; ") +
		QString("dimensions: %1x%2x%3; ")
		.arg(metaDim[0]).arg(metaDim[1]).arg(metaDim[2]) +
		QString("spacing: %1, %2, %3; ").arg(metaSpacing[0]).arg(metaSpacing[1]).arg(metaSpacing[2]) +
		QString("origin: %1, %2, %3; ").arg(metaOrigin[0]).arg(metaOrigin[1]).arg(metaOrigin[2]));

	
	// determine number of fibers in each cell:
	auto numberOfFibersImage = allocateImage(VTK_DOUBLE, metaDim.data(), metaSpacing.data());
	fillImage(numberOfFibersImage, 0.0);
	for (int o = 0; o < csvTable->GetNumberOfRows(); ++o)
	{
		iAVec3i startVoxel(iAVec3d(csvTable->GetValue(o, startIdx[0]).ToDouble(), csvTable->GetValue(o, startIdx[1]).ToDouble(), csvTable->GetValue(o, startIdx[2]).ToDouble()) / metaSpacing);
		iAVec3i endVoxel(iAVec3d(csvTable->GetValue(o, endIdx[0]).ToDouble(), csvTable->GetValue(o, endIdx[1]).ToDouble(), csvTable->GetValue(o, endIdx[2]).ToDouble()) / metaSpacing);
		auto cellIds = findCellIdsBresenham3D(startVoxel, endVoxel);
		for (auto c : cellIds)
		{
			if (c[0] < 0 || c[0] >= metaDim[0] || c[1] < 0 || c[1] >= metaDim[1] || c[2] < 0 || c[2] >= metaDim[2])
			{
				LOG(lvlWarn, QString("Invalid coordinate %1").arg(c.toString()));
				continue;
			}
			numberOfFibersImage->SetScalarComponentFromDouble(c.x(), c.y(), c.z(), 0,
				numberOfFibersImage->GetScalarComponentAsDouble(c.x(), c.y(), c.z(), 0) + 1);
		}
	}
	addOutput(numberOfFibersImage);
	auto r = numberOfFibersImage->GetScalarRange();
	LOG(lvlDebug, QString("Number of fibers: from %1 to %2").arg(r[0]).arg(r[1]));

	// determine characteristics averages:
	auto curOutput = 1;
	for (auto col : columns)
	{
		auto metaImage = allocateImage(VTK_DOUBLE, metaDim.data(), metaSpacing.data());
		metaImage->SetOrigin(metaOrigin);
		fillImage(metaImage, 0.0);
		for (int o = 0; o < csvTable->GetNumberOfRows(); ++o)
		{
			iAVec3i startVoxel(iAVec3d(csvTable->GetValue(o, startIdx[0]).ToDouble(), csvTable->GetValue(o, startIdx[1]).ToDouble(), csvTable->GetValue(o, startIdx[2]).ToDouble()) / metaSpacing);
			iAVec3i endVoxel(iAVec3d(csvTable->GetValue(o, endIdx[0]).ToDouble(), csvTable->GetValue(o, endIdx[1]).ToDouble(), csvTable->GetValue(o, endIdx[2]).ToDouble()) / metaSpacing);
			auto val = csvTable->GetValue(o, col).ToDouble();
			auto cellIds = findCellIdsBresenham3D(startVoxel, endVoxel);
			for (auto c : cellIds)
			{
				if (c[0] < 0 || c[0] >= metaDim[0] || c[1] < 0 || c[1] >= metaDim[1] || c[2] < 0 || c[2] >= metaDim[2])
				{
					LOG(lvlWarn, QString("Invalid coordinate %1").arg(c.toString()));
					continue;
				}
				auto cellValue = metaImage->GetScalarComponentAsDouble(c.x(), c.y(), c.z(), 0);
				auto numOfFibersInCell = numberOfFibersImage->GetScalarComponentAsDouble(c.x(), c.y(), c.z(), 0);
				assert(numOfFibersInCell != 0); // should not happen, since above we visited the same cells
				metaImage->SetScalarComponentFromDouble(c.x(), c.y(), c.z(), 0, cellValue + val / numOfFibersInCell	);
			}
		}
		setOutputName(curOutput, headers[col]);
		addOutput(metaImage);
		auto r = metaImage->GetScalarRange();
		LOG(lvlDebug, QString("Output %1: values from %2 to %3").arg(headers[col]).arg(r[0]).arg(r[1]));
		++curOutput;
	}
}
