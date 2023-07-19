// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAXYPlotUnorderedData.h"

#include <cassert>

iAPlotData::DataType iAXYPlotUnorderedData::xValue(size_t idx) const
{
	assert(idx < m_values.size());
	return m_values[idx].first;
}

iAPlotData::DataType iAXYPlotUnorderedData::yValue(size_t idx) const
{
	assert(idx < m_values.size());
	return m_values[idx].second;
}

iAPlotData::DataType const* iAXYPlotUnorderedData::xBounds() const
{
	return m_xBounds;
}

iAPlotData::DataType const* iAXYPlotUnorderedData::yBounds() const
{
	return m_yBounds;
}

size_t iAXYPlotUnorderedData::valueCount() const
{
	return m_values.size();
}

void iAXYPlotUnorderedData::addValue(iAPlotData::DataType x, iAPlotData::DataType y)
{
	m_values.push_back(std::make_pair(x,y));
	adaptBounds(m_xBounds, x);
	adaptBounds(m_yBounds, y);
}

size_t iAXYPlotUnorderedData::nearestIdx(double dataX) const
{
	if (dataX < m_xBounds[0])
	{
		return 0;
	}
	if (dataX >= m_xBounds[1])
	{
		return valueCount() - 1;
	}
	// TODO: use binary search?
	for (size_t i = 1; i < m_values.size(); ++i)
	{
		if (m_values[i].first > dataX)
		{
			return i - 1;
		}
	}
	// shouldn't be necessary (since if dataX is >= last data value,
	//     xBounds check above would have triggered already)
	return valueCount() - 1;
}

//todo get x & y data
size_t iAXYPlotUnorderedData::findIdx(double dataX) const
{
	if (dataX < m_xBounds[0])
	{
		return 0;
	}
	if (dataX >= m_xBounds[1])
	{
		return valueCount() - 1;
	}
	// TODO: only finds 1 point
	for (size_t i = 1; i < m_values.size(); ++i)
	{
		if (fabs(m_values[i].first - dataX) < FLT_EPSILON)
		{
			return i;
		}
	}
	// no datapoint close enough
	return -1;
}

QString iAXYPlotUnorderedData::toolTipText(iAPlotData::DataType dataX) const
{
	//size_t idx = nearestIdx(dataX);
	size_t idx = findIdx(dataX);
	if (idx == -1)
	{
		return QString();
	}
	if (m_toolTipText.empty())
	{
		auto valueX = xValue(idx);
		auto valueY = yValue(idx);
		return QString("%1: %2").arg(valueX).arg(valueY);
	}
	return m_toolTipText.at(idx);
}

QSharedPointer<iAXYPlotUnorderedData> iAXYPlotUnorderedData::create(
	QString const& name, iAValueType type, size_t reservedSize, QStringList toolTipText)
{
	return QSharedPointer<iAXYPlotUnorderedData>(new iAXYPlotUnorderedData(name, type, reservedSize, toolTipText));
}

iAXYPlotUnorderedData::iAXYPlotUnorderedData(
	QString const& name, iAValueType type, size_t reservedSize) :
	iAXYPlotUnorderedData(name, type, reservedSize, QStringList())
{
}

iAXYPlotUnorderedData::iAXYPlotUnorderedData(
	QString const& name, iAValueType type, size_t reservedSize, QStringList toolTipText) :
	iAPlotData(name, type),
	m_xBounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()},
	m_yBounds{std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()},
	m_toolTipText(toolTipText)
{
	m_values.reserve(reservedSize);
}
