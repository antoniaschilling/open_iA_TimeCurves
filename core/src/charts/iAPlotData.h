/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"

#include "iAValueType.h"

#include <cstddef> // for size_t
#include <cmath>   // for log

//! Abstract base class providing data used for drawing a plot in a chart.
class open_iA_Core_API iAPlotData
{
public:
	typedef double DataType;
	virtual ~iAPlotData();
	//! Value on the x axis for a datum with given index.
	//! @param idx data index
	virtual double xValue(size_t idx) const = 0;
	//! Value on the y axis for a datum with given index.
	//! @param idx data index
	virtual DataType yValue(size_t idx) const = 0;
	//! The range of values for x; i.e. xBounds()[0] is the minimum of all xValue(...), xBounds()[1] is the maximum.
	virtual double const* xBounds() const = 0;
	//! The range of values for y; i.e. yBounds()[0] is the minimum of all yValue(...), yBounds()[1] is the maximum.
	virtual DataType const* yBounds() const = 0;
	//! The type of the values held by this data object.
	virtual iAValueType valueType() const;
	//! The number of available data elements (i.e. in xValue/yValue, idx can go from 0 to valueCount()-1).
	virtual size_t valueCount() const = 0;
	//! Retrieve the index closest to the given x data value.
	//! @param dataX the value (on the x axis) for which to search the closest datapoint in this data object.
	//! @return the index (such as can be passed to xValue/yValue) of the datapoint closest to dataX
	//!         calling xValue on the returned index will always give a value lower than or equal to dataX.
	size_t nearestIdx(double dataX) const;  // TODO: make abstract - current implementation assumes constant spacing!
	
	//! The tooltip text for this data when the user is currently hovering over the given x position.
	//! Note that currently only the x axis position of the user is considered.
	//! @param dataX the value (on the x axis) the user currently is hovering over.
	//! @return a description of the datapoint that the user currrently is over/closest to.
	// TODO: make abstract (?)
	virtual QString toolTipText(double dataX) const;
};
