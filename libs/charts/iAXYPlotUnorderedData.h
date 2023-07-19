// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAPlotData.h"
#include "iAcharts_export.h"

#include <QSharedPointer>
#include <QStringList>

#include <utility>  // for pair
#include <vector>

class iAcharts_API iAXYPlotUnorderedData : public iAPlotData
{
public:
	iAXYPlotUnorderedData(QString const& name, iAValueType type, size_t reservedSize);
	iAXYPlotUnorderedData(QString const& name, iAValueType type, size_t reservedSize, QStringList toolTipText);
	//! @{
	//! overriding methods from iAPlotData
	DataType xValue(size_t idx) const override;
	DataType yValue(size_t idx) const override;
	DataType const* xBounds() const override;
	DataType const* yBounds() const override;
	size_t valueCount() const override;
	size_t nearestIdx(DataType dataX) const override;
	QString toolTipText(DataType dataX) const override;
	//Retrieve the index closer than epsilon to the given x data value.
	//<returns>Returns -1 if no point found.</returns>
	size_t findIdx(double dataX) const;

	//! Adds a new x/y pair.
	void addValue(DataType x, DataType y);
	//! Create an empty data object
	static QSharedPointer<iAXYPlotUnorderedData> create(
		QString const& name, iAValueType type, size_t reservedSize, QStringList toolTipText);

private:
	QStringList m_toolTipText;
	std::vector<std::pair<DataType, DataType>> m_values;
	DataType m_xBounds[2], m_yBounds[2];
};
