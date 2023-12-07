#include "iATimeCurveChart.h"

#include <iAMathUtility.h>
#include <iALog.h>
#include "iAMapperImpl.h"
#include "iAMathUtility.h"

#include <QWheelEvent>
#include <QPainter>
#include <QApplication>


namespace
{
	const double ZoomXMin = 1.0;
	const double ZoomXMax = 2048;
	const double ZoomXMaxEmpty = 1.0;
	const double ZoomYMin = 1.0;
	const double ZoomYMax = 32768;
	const double ZoomXStep = 1.5;
	const double ZoomYStep = 1.5;
	const int CategoricalFontSize = 7;
	const int MarginLeft = 5;
	const int MarginBottom = 5;
	const int AxisTicksYMax = 5;
	const int AxisTicksXDefault = 2;
	const int TickWidth = 6;

	const double LogYMapModeMin = 0.5;

	int markerPos(int x, size_t step, size_t stepCount)
	{
		if (step == stepCount)
		{
			--x;
		}
		return x;
	}

	int textPos(int markerX, size_t step, size_t stepCount, int textWidth)
	{
		return (step == 0)             ? markerX                  // right aligned to indicator line
			: (step < (stepCount - 1)) ? markerX - textWidth / 2  // centered to the indicator line
									   : markerX - textWidth;     // left aligned to the indicator line
	}

	void ensureNonZeroRange(double* bounds, bool warn = false, double offset = 0.1)
	{
		if (dblApproxEqual(bounds[0], bounds[1]))
		{
			if (warn)
			{
				LOG(lvlWarn,
					QString("range [%1..%2] invalid (min~=max), enlarging it by %3")
						.arg(bounds[0])
						.arg(bounds[1])
						.arg(offset));
			}
			bounds[0] -= offset;
			bounds[1] += offset;
		}
	}
}


void iATimeCurveChart::wheelEvent(QWheelEvent* event)
{
	//iAChartWidget::zoomAlongX(event->angleDelta().y(), event->position().x(), true);
	//iAChartWidget::zoomAlongY(event->angleDelta().y(), true);
	zoomAlongXY(event->angleDelta().y(), event->position().x(), event->position().y(), true);
	event->accept();
	update();
}



//combines iAChartWidget::iAChartWidget::zoomAlongX() and iAChartWidget::zoomAlongY() into one function
//deltamode: 
void iATimeCurveChart::zoomAlongXY(double value, int mouseX, int mouseY, bool deltaMode)
{
	//does not support zooming with keyboard shortcut
	/*if (!deltaMode)
	{
		return;
	}*/

	// don't do anything if we're already at the limit
	if ((value < 0 && m_xZoom == 1.0) || (value > 0 && dblApproxEqual(m_xZoom, maxXZoom())) ||
		(value < 0 && m_yZoom == ZoomYMin) || (value > 0 && dblApproxEqual(m_yZoom, ZoomYMax))) 
	{
		LOG(lvlDebug, QString("Zoom at limit."));
		return;
	}

	if (deltaMode)
	{
		if (value /* = delta */ > 0)
		{
			value = m_yZoom * ZoomYStep;
			value = m_xZoom * ZoomXStep;

		}
		else
		{
			value = m_yZoom / ZoomYStep;
			value = m_xZoom /= ZoomXStep;

		}
	}
	double yZoomBefore = m_yZoom;
	double xZoomBefore = m_xZoom;
	double yTranslationBefore = m_translationY;
	double xShiftBefore = m_xShift;
	double fixedYValue = yMapper().dstToSrc(chartHeight() / 2 + yTranslationBefore);
	double fixedDataX = mouse2DataX(chartWidth()/2 - leftMargin()) + xShiftBefore;

	m_yZoom = clamp(ZoomYMin, ZoomYMax, value);
	m_xZoom = clamp(ZoomXMin, ZoomXMax, value);

	//limitXShift
	//m_xShift = clamp(0.0, (xZoom() - 1) / (xZoom()) * xRange(), xShiftAfter);

	//zoom to center
	int fullChartHeight = (chartHeight() - 1) * m_yZoom;
	m_yMapper->update(yBounds()[0], yBounds()[1], 0, fullChartHeight);
	m_xMapper->update(xBounds()[0], xBounds()[1], 0, fullChartWidth());

	int yTranslationAfter = yMapper().srcToDst(fixedYValue) - chartHeight() / 2;
	double xShiftAfter = fixedDataX - mouse2DataX((chartWidth() / 2));



	//yTranslationAfter = yTranslationBefore + mouseY - chartHeight() / m_yZoom / 2;
	//todo limit ytranslation somehow
	if (m_xZoom == 1 || m_yZoom == 1)
	{
		m_xShift = 0;
		m_translationY = 0;
	}
	else
	{
		m_translationY = yTranslationAfter;
		m_xShift = xShiftAfter;
	}

	if (!dblApproxEqual(yZoomBefore, m_yZoom) || !dblApproxEqual(xZoomBefore, m_xZoom))
	{
		emit axisChanged();
	}
	LOG(lvlDebug, QString("mousePosX: '%1', mousePosY: '%2'").arg(mouseX).arg(mouseY));
	LOG(lvlDebug, QString("xZoomBefore: '%1', xShiftBefore: '%2'")
		.arg(xZoomBefore)
		.arg(xShiftBefore));
	LOG(lvlDebug, QString("xZoomAfter:  '%1', xShiftAfter:  '%2'")
		.arg(m_xZoom)
		.arg(m_xShift));
	
	LOG(lvlDebug, QString("yZoomBefore: '%1', yTranslationBefore: '%2'")
		.arg(yZoomBefore)
		.arg(yTranslationBefore));
	LOG(lvlDebug, QString("yZoomAfter:  '%1', yTranslationAfter:  '%2'")
		.arg(m_yZoom)
		.arg(m_translationY));
}

void iATimeCurveChart::zoomAlongY(double value, bool deltaMode)
{
	//todo insert x variable
	zoomAlongXY(value, 0,0, deltaMode);
}

void iATimeCurveChart::zoomAlongX(double value, int x, bool deltaMode)
{
	zoomAlongXY(value,0,0, deltaMode);
}

void iATimeCurveChart::drawPlots(QPainter& painter)
{
	painter.translate(0, -m_translationY);
	iAChartWidget::drawPlots(painter);
	painter.translate(0, m_translationY);
}

/* Y translation */
//
//double iATimeCurveChart::visibleYStart() const
//{
//	return yBounds()[0] + m_translationY;
//}
//
//double iATimeCurveChart::visibleYEnd() const
//{
//	return visibleYStart() + xRange() /m_yZoom;
//}
//
//double iATimeCurveChart::yRange() const
//{
//	return yBounds()[1] - yBounds()[0];
//}

void iATimeCurveChart::drawYAxis(QPainter& painter)
{
	if (leftMargin() <= 0)
	{
		return;
	}
	painter.save();
	painter.translate(xMapper().srcToDst(visibleXStart()), 0);
	QColor bgColor = QApplication::palette().color(QWidget::backgroundRole());
	painter.fillRect(QRect(-leftMargin(), -chartHeight(), leftMargin(), geometry().height()), bgColor);
	QFontMetrics fm = painter.fontMetrics();
	int fontHeight = fm.height();
	int aheight = chartHeight() - 1;
	painter.setPen(QApplication::palette().color(QPalette::Text));

	// at most, make Y_AXIS_STEPS, but reduce to number actually fitting in current height:
	int stepNumber = std::min(AxisTicksYMax, static_cast<int>(aheight / (fontHeight * 1.1)));
	stepNumber = std::max(1, stepNumber);  // make sure there's at least 2 steps
	const double step = 1.0 / (stepNumber * m_yZoom);

	for (int i = 0; i <= stepNumber; ++i)
	{
		double pos = step * i;
		int y = -static_cast<int>(pos * aheight * m_yZoom) - 1;
		double yValue = yMapper().dstToSrc(-y + m_translationY - 1);
		//QString text = dblToStringWithUnits(yValue, 10);
		QString text = QString::number(yValue, 'g', 3);
		painter.drawLine(static_cast<int>(-TickWidth), y, 0, y);  // indicator line
		painter.drawText(-(fm.horizontalAdvance(text) + TickWidth),
			(i == stepNumber)
				? y + static_cast<int>(0.75 * fontHeight)  // write the text top aligned to the indicator line
				: y + static_cast<int>(0.25 * fontHeight)  // write the text centered to the indicator line
			,
			text);
	}
	painter.drawLine(0, -1, 0, -aheight);
	//write the y axis label
	painter.save();
	painter.rotate(-90);
	QPointF textPos(aheight * 0.5 - 0.5 * fm.horizontalAdvance(m_yCaption), -leftMargin() + fontHeight - 5);
	painter.drawText(textPos, m_yCaption);
	painter.restore();
	painter.restore();
}

double iATimeCurveChart::visibleXStart() const
{
	return xBounds()[0] + m_xShift;
}

double iATimeCurveChart::visibleXEnd() const
{
	double visibleRange = xRange() / m_xZoom;
	return visibleXStart() + visibleRange;
}

void iATimeCurveChart::resetView()
{
	m_xShift = 0.0;
	//m_translationY = 0.0;
	m_translationY = 0;
	m_xZoom = 1.0;
	m_yZoom = 1.0;
	emit axisChanged();
	update();
}
