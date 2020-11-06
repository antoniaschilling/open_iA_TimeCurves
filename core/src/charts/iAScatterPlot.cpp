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
#include "iAScatterPlot.h"

#include "iALog.h"
#include "iALookupTable.h"
#include "iAMathUtility.h"
#include "iAScatterPlotSelectionHandler.h"
#include "iASPLOMData.h"
#ifdef CHART_OPENGL
#include "qthelper/iAQGLWidget.h"
#endif

#include <QAbstractTextDocumentLayout>
#include <QColor>
#include <QDebug>
#include <QtMath>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPolygon>
#include <QPropertyAnimation>
#include <QWheelEvent>

namespace
{
	const size_t CordDim = 3;
	const size_t ColChan = 4;
}


iAScatterPlot::Settings::Settings() :
	pickedPointMagnification( 2.0 ),
	tickOffset( 45 ),
	tickSpacing( 5 ),
	maximizedParamsOffset( 5 ),
	textRectHeight( 30 ),
	rangeMargin( 0.08 ),
	pointRadius( 1.0/*2.5*/ ),
	maximizedPointMagnification( 1.7 ),
	defaultGridDimensions( 100 ),
	defaultMaxBtnSz( 10 ),
	paramTextOffset( 5 ),
	previewBorderWidth( 3.0 ),
	previewBorderColor( QColor( 140, 140, 140 ) ),
	selectionPolyColor( QColor( 150, 150, 150, 100 ) ),
	plotBorderColor( QColor( 170, 170, 170 ) ),
	tickLineColor( QColor( 221, 221, 221 ) ),
	tickLabelColor( QColor( 100, 100, 100 ) ),
	selectionColor( QColor(0, 0, 0) ),
	selectionMode(Polygon),
	selectionEnabled(false),
	showPCC(false),
	showSCC(false)
{}

size_t iAScatterPlot::NoPointIndex = std::numeric_limits<size_t>::max();

iAScatterPlot::iAScatterPlot(iAScatterPlotSelectionHandler * splom, iAChartParentWidget* parent,
	int numTicks /*= 5*/, bool isMaximizedPlot /*= false */):
	QObject(parent),
	settings(),
	m_parentWidget(parent),
#ifdef CHART_OPENGL
	m_pointsBuffer(nullptr),
	m_pointsOutdated(true),
#endif
	m_splom( splom ),
	m_colInd(0),
	m_lut( new iALookupTable() ),
	m_scale( 1.0 ),
	m_offset( 0.0, 0.0 ),
	m_numTicks( numTicks ),
	m_isPlotActive( false ),
	m_prevPtInd(NoPointIndex),
	m_prevInd(NoPointIndex),
	m_curInd( NoPointIndex ),
	m_isMaximizedPlot( isMaximizedPlot ),
	m_isPreviewPlot( false ),
	m_curVisiblePts ( 0 ),
	m_dragging(false),
	m_pcc(0),
	m_scc(0),
	m_pccValid(false),
	m_sccValid(false)
{
	m_paramIndices[0] = 0; m_paramIndices[1] = 1;
	initGrid();
}

iAScatterPlot::~iAScatterPlot()
{
#ifdef CHART_OPENGL
	if (m_pointsBuffer)
	{
		m_pointsBuffer->bind();
		m_pointsBuffer->destroy();
		m_pointsBuffer->release();
		delete m_pointsBuffer;
	}
#endif
}

void iAScatterPlot::setData(size_t x, size_t y, QSharedPointer<iASPLOMData>& splomData)
{
	if (m_splomData)
	{
		return;
	}

	m_splomData = splomData;
	connect(m_splomData.data(), &iASPLOMData::dataChanged, this, &iAScatterPlot::dataChanged);
	if (!hasData())
	{
		return;
	}
	setIndices(x, y);
}

void iAScatterPlot::setIndices(size_t x, size_t y)
{
	m_paramIndices[0] = x; m_paramIndices[1] = y;
	m_pccValid = false;
	m_sccValid = false;
	applyMarginToRanges();
	updateGrid();
	updatePoints();
}

bool iAScatterPlot::hasData() const
{
	if (m_splomData.isNull() || !(m_splomData->numPoints()) || !(m_splomData->numParams()))
	{
		return false;
	}
	return true;
}

void iAScatterPlot::updatePoints()
{
#ifdef CHART_OPENGL
	m_pointsOutdated = true;
#endif
}

void iAScatterPlot::setLookupTable( QSharedPointer<iALookupTable> &lut, size_t colInd )
{
	m_colInd = colInd;
	m_lut = lut;
	updatePoints();
}

void iAScatterPlot::setTransform( double scale, QPointF newOffset )
{
	bool isUpdate = false;

	if ( m_scale != scale )
	{
		m_scale = scale;
		isUpdate = true;
	}

	if ( newOffset != m_offset )
	{
		m_offset = newOffset;
		isUpdate = true;
	}

	if ( isUpdate )
	{
		calculateNiceSteps();
		m_parentWidget->update();
	}
}

void iAScatterPlot::setTransformDelta( double scale, QPointF deltaOffset )
{
	bool isUpdate = false;
	if ( m_scale != scale )
	{
		m_scale = scale;
		isUpdate = true;
	}
	if ( deltaOffset.x() || deltaOffset.y() )
	{
		m_offset += deltaOffset;
		isUpdate = true;
	}
	if ( isUpdate )
		calculateNiceSteps();
}

void iAScatterPlot::setRect( QRect val )
{
	m_globRect = val;
	updateDrawRect();
	calculateNiceSteps();
}

QPointF iAScatterPlot::getPointPosition( size_t index ) const
{
	return m_globRect.topLeft() + getPositionFromPointIndex( index );
}

void iAScatterPlot::printTicksInfo( QList<double> * posX, QList<double> * posY, QList<QString> * textX, QList<QString> * textY ) const
{
	for (double t : m_ticksX)
	{
		posX->push_back( p2x( t ) + m_globRect.x() );
		textX->push_back( QString::number( t ) );
	}
	for (double t : m_ticksY)
	{
		posY->push_back( p2y( t ) + m_globRect.y() );
		textY->push_back( QString::number( t ) );
	}
}

void iAScatterPlot::setCurrentPoint( size_t index )
{
	if ( m_curInd != index )
	{
		m_prevInd = m_curInd;
		if ( m_prevInd != NoPointIndex )
			m_prevPtInd = m_prevInd;
		m_curInd = index;
	}
}

size_t iAScatterPlot::getCurrentPoint() const
{
	return m_curInd;
}

size_t iAScatterPlot::getPreviousIndex() const
{
	return m_prevInd;
}

size_t iAScatterPlot::getPreviousPoint() const
{
	return m_prevPtInd;
}

void iAScatterPlot::setPreviewState(bool isPreviewPlot)
{
	m_isPreviewPlot = isPreviewPlot;
}

void iAScatterPlot::leave()
{
	m_curInd = m_prevPtInd = NoPointIndex;
	m_isPlotActive = false;
}

void iAScatterPlot::enter()
{
	m_isPlotActive = true;
}

void iAScatterPlot::paintOnParent( QPainter & painter )
{
	if (!hasData())
	{
		return;
	}
#ifdef CHART_OPENGL
	if (!m_pointsBuffer)
	{
		createVBO();
	}
	if (!m_pointsBuffer) // if still not initialized here, then we cannot draw
	{
		return;
	}
	if (m_pointsOutdated)
	{
		fillVBO();
	}
#endif
	painter.save();
	painter.translate( m_globRect.x(), m_globRect.y());
	QColor bg(settings.backgroundColor);
	if (!bg.isValid())
	{
		bg = m_parentWidget->palette().color(m_parentWidget->backgroundRole());
	}
	painter.setBrush(bg);
	drawTicks( painter );
	drawPoints( painter );
	if (settings.selectionEnabled)
	{
		drawSelectionPolygon(painter);
	}
	drawBorder( painter );
	if (settings.showPCC || settings.showSCC)
	{
		painter.setPen( /* QColor(0, 0, 0) */ m_parentWidget->palette().color(QPalette::Text));
		QString corrCoeffText;
		if (settings.showPCC)
		{
			corrCoeffText = "Pearson's Coeff." + QString::number(pcc(), 'g', 3);
		}
		if (settings.showSCC)
		{
			corrCoeffText += (!corrCoeffText.isEmpty()?"\n":"") + QString("Spearman's Coeff.") + QString::number(scc(), 'g', 3);
		}
		painter.drawText( QRect(0, 0, m_globRect.width(), m_globRect.height()), Qt::AlignCenter | Qt::AlignVCenter, corrCoeffText);
	}
	painter.restore();
}

void iAScatterPlot::SPLOMWheelEvent( QWheelEvent * event )
{
	QPoint numDegrees = event->angleDelta() / 8;

	if ( !numDegrees.isNull() )
	{
		double d = 0.1 / 15.0;
		double delta = ( numDegrees.y() + numDegrees.x() ) * d;
		double oldScale = m_scale;
		m_scale *= ( 1 + delta );
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
		QPointF pos = getLocalPos( event->pos() );
#else
		QPointF pos = getLocalPos(event->position().toPoint());
#endif
		QPointF oldOffset = m_offset;
		QPointF newOffset = pos - ( pos - oldOffset ) * m_scale / oldScale;
		QPointF deltaOffset = newOffset - m_offset;
		m_offset = newOffset;
		calculateNiceSteps();
		emit transformModified( m_scale, deltaOffset );
	}
}

void iAScatterPlot::SPLOMMouseMoveEvent( QMouseEvent * event )
{
	bool isUpdate = false;
	QPoint locPos = getLocalPos( event->pos() );

	if ( !( event->buttons()&Qt::RightButton ) && !( event->buttons()&Qt::LeftButton ) )
	{
		size_t newInd = getPointIndexAtPosition( locPos );
		if ( m_curInd != newInd )
		{
			setCurrentPoint( newInd );
			emit currentPointModified( m_curInd );
			isUpdate = true;
		}
	}
	else if ( m_dragging )
	{
		QPointF deltaOffset = locPos - m_prevPos;
		m_offset += locPos - m_prevPos;
		m_prevPos = locPos;
		isUpdate = true;
		calculateNiceSteps();
		emit transformModified( m_scale, deltaOffset );
	}
	else if ( event->buttons()&Qt::LeftButton && settings.selectionEnabled ) // selection
	{
		if (settings.selectionMode == Polygon)
		{
			m_selPoly.append(cropLocalPos(locPos));
		}
		else
		{
			m_selPoly.clear();
			m_selPoly.append(m_selStart);
			m_selPoly.append(QPoint(m_selStart.x(), locPos.y()));
			m_selPoly.append(QPoint(locPos.x(), locPos.y()));
			m_selPoly.append(QPoint(locPos.x(), m_selStart.y()));
		}
		isUpdate = true;
	}
	if (isUpdate)
	{
		m_parentWidget->update();
	}
}

void iAScatterPlot::SPLOMMousePressEvent( QMouseEvent * event )
{
	QPoint locPos = getLocalPos( event->pos() );
	m_prevPos = locPos;
	if (event->buttons()&Qt::RightButton || (event->buttons()&Qt::LeftButton && event->modifiers()&Qt::ControlModifier))
	{
		m_dragging = true;
	}
	if ( event->buttons()&Qt::LeftButton && settings.selectionEnabled)
	{
		if (settings.selectionMode == Rectangle)
		{
			m_selStart = locPos;
		}
		else
		{
			m_selPoly.append(locPos);
		}
	}
}

void iAScatterPlot::SPLOMMouseReleaseEvent( QMouseEvent * event )
{
	if (m_dragging)
	{
		m_dragging = false;
	}
	else if ( event->button() == Qt::LeftButton && settings.selectionEnabled)
	{
		bool append = ( event->modifiers() & Qt::ShiftModifier ) ? true : false;
		bool remove = ( event->modifiers() & Qt::AltModifier ) ? true : false;
		updateSelectedPoints( append, remove );
	}
}

int iAScatterPlot::p2binx( double p ) const
{
	double rangeDst[2] = { 0, static_cast<double>(m_gridDims[0] - 1) };
	double xbin = mapValue( m_prX, rangeDst, p );
	return (int) clamp( rangeDst[0], rangeDst[1], xbin);
}

double iAScatterPlot::p2tx( double pval ) const
{
	double norm = mapToNorm(m_prX, pval);
	if (m_splomData->isInverted(m_paramIndices[0]))
	{
		norm = 1.0 - norm;
	}
	return norm;
}

double iAScatterPlot::p2x( double pval ) const
{
	double rangeDst[2] = { m_locRect.left(), m_locRect.right() };
	double pixelX = mapValue( m_prX, rangeDst, pval);
	if (m_splomData->isInverted(m_paramIndices[0]))
	{
		pixelX = invertValue(rangeDst, pixelX);
	}
	return applyTransformX(pixelX);
}

double iAScatterPlot::x2p( double x ) const
{
	double rangeSrc[2] = { m_locRect.left(), m_locRect.right() };
	//assert(rangeSrc[0] < rangeSrc[1]);
	double revTransX = clamp(rangeSrc[0]<rangeSrc[1]?rangeSrc[0]:rangeSrc[1],
		rangeSrc[0]<rangeSrc[1] ? rangeSrc[1] : rangeSrc[0], revertTransformX(x));
	if (m_splomData->isInverted(m_paramIndices[0]))
	{
		revTransX = invertValue(rangeSrc, revTransX);
	}
	return mapValue( rangeSrc, m_prX, revTransX);
}

int iAScatterPlot::p2biny( double p ) const
{
	double rangeDst[2] = { 0, static_cast<double>(m_gridDims[1] - 1) };
	double ybin = mapValue( m_prY, rangeDst, p );
	return (int) clamp( rangeDst[0], rangeDst[1], ybin);
}

double iAScatterPlot::p2ty( double pval ) const
{
	double norm = mapToNorm( m_prY, pval );
	if (!m_splomData->isInverted(m_paramIndices[1])) // y needs to be inverted normally
	{
		norm = 1.0 - norm;
	}
	return norm;
}

double iAScatterPlot::p2y( double pval ) const
{
	double rangeDst[2] = { m_locRect.bottom(), m_locRect.top() };
	double pixelY = mapValue(m_prY, rangeDst, pval);
	if (m_splomData->isInverted(m_paramIndices[1]))
	{
		pixelY = invertValue(rangeDst, pixelY);
	}
	return applyTransformY( pixelY );
}

double iAScatterPlot::y2p(double y) const
{
	double rangeSrc[2] = { m_locRect.bottom(), m_locRect.top() };
	//assert(rangeSrc[0] > rangeSrc[1]);
	double revTransY = clamp(rangeSrc[0] < rangeSrc[1] ? rangeSrc[0] : rangeSrc[1],
		rangeSrc[0] < rangeSrc[1] ? rangeSrc[1] : rangeSrc[0], revertTransformY(y));
	if (m_splomData->isInverted(m_paramIndices[1]))
	{
		revTransY = invertValue(rangeSrc, revTransY);
	}
	return mapValue( rangeSrc, m_prY, revTransY);
}

double iAScatterPlot::applyTransformX( double v ) const
{
	return m_offset.x() + v * m_scale;
}

double iAScatterPlot::revertTransformX( double v ) const
{
	return ( v - m_offset.x() ) / m_scale;
}

double iAScatterPlot::applyTransformY( double v ) const
{
	return m_offset.y() + v * m_scale;
}

double iAScatterPlot::revertTransformY( double v ) const
{
	return ( v - m_offset.y() ) / m_scale;
}

void iAScatterPlot::initGrid()
{
	m_gridDims[0] = m_gridDims[1] = settings.defaultGridDimensions;
	for (int i = 0; i < m_gridDims[0] * m_gridDims[1]; ++i)
	{
		m_pointsGrid.push_back(QList<size_t>());
	}
}

void iAScatterPlot::updateGrid()
{
	for (int i = 0; i < m_gridDims[0] * m_gridDims[1]; ++i)
	{
		m_pointsGrid[i].clear();
	}

	for ( size_t i = 0; i < m_splomData->numPoints(); ++i )
	{
		double x = m_splomData->paramData(m_paramIndices[0] )[i];
		double y = m_splomData->paramData(m_paramIndices[1] )[i];
		int xbin = p2binx( x );
		int ybin = p2biny( y );
		int binInd = getBinIndex( xbin, ybin );
		m_pointsGrid[binInd].push_back( i );
	}
}

void iAScatterPlot::dataChanged(size_t paramIndex)
{
	if (paramIndex != m_paramIndices[0] && paramIndex != m_paramIndices[1])
	{
		return;
	}
	applyMarginToRanges();
	updateGrid();
	updatePoints();
}

void iAScatterPlot::applyMarginToRanges()
{
	m_prX[0] = m_splomData->paramRange(m_paramIndices[0])[0];
	m_prX[1] = m_splomData->paramRange(m_paramIndices[0])[1];
	m_prY[0] = m_splomData->paramRange(m_paramIndices[1])[0];
	m_prY[1] = m_splomData->paramRange(m_paramIndices[1])[1];
	if ( m_prX[0] == m_prX[1] )
	{
		m_prX[0] -= 0.1; m_prX[1] += 0.1;
	}
	if ( m_prY[0] == m_prY[1] )
	{
		m_prY[0] -= 0.1; m_prY[1] += 0.1;
	}
	double rM = settings.rangeMargin;
	double prLenX = m_prX[1] - m_prX[0], prLenY = m_prY[1] - m_prY[0];
	m_prX[0] -= rM * prLenX; m_prX[1] += rM * prLenX;
	m_prY[0] -= rM * prLenY; m_prY[1] += rM * prLenY;
	calculateNiceSteps();
}

void iAScatterPlot::calculateNiceSteps()
{
	if (m_locRect.width() == 0 || m_locRect.height() == 0)
	{
		return;
	}
	double rx[2] = { x2p( 0 ), x2p( m_locRect.width() ) };
	double ry[2] = { y2p( m_locRect.height() ), y2p( 0 ) };
	calculateNiceSteps( rx, &m_ticksX );
	calculateNiceSteps( ry, &m_ticksY );
}

void iAScatterPlot::calculateNiceSteps( double * r, QList<double> * ticks )
{
	if (m_numTicks <= 0)
	{
		return;
	}
	double range = r[1] - r[0];
	double delta = range / m_numTicks;
	int goodNums[3] = { 1, 2, 5 };
	double stepSize = delta;
	double closeness = range;
	for ( int i = 0; i < 3; ++i )
	{
		int g = goodNums[i];
		double ideal = delta / g;
		double p = log10f( ideal );
		double intpart;
		double fractpart = modf( p, &intpart );
		if ( fractpart < 0 )
		{
			fractpart += 1;
			intpart -= 1;
		}
		int n = round( powf( 10, fractpart ) );
		double curStepSize = g * n * pow( 10, intpart );
		double curCloseness = abs( delta - curStepSize );
		if ( curCloseness < closeness )
		{
			closeness = curCloseness;
			stepSize = curStepSize;
		}
	}
	ticks->clear();
	double ip; modf( r[0] / stepSize, &ip );
	double tick = stepSize*ip;
	if (stepSize > 0)
	{
		while (tick < r[0])
		{
			tick += stepSize;
		}
		while (tick <= r[1])
		{
			ticks->push_back(tick);
			tick += stepSize;
		}
	}
	else
	{
		while (tick > r[0])
		{
			tick += stepSize;
		}
		while (tick >= r[1])
		{
			ticks->push_back(tick);
			tick += stepSize;
		}
	}
}

int iAScatterPlot::getBinIndex(int x, int y) const
{
	return y*m_gridDims[0] + x;
}

size_t iAScatterPlot::getPointIndexAtPosition( QPointF mpos ) const
{
	double px = x2p( mpos.x() );
	double py = y2p( mpos.y() );
	int xbin = p2binx( px );
	int ybin = p2biny( py );
	int binInd;
	double ptRad = getPointRadius();
	double pPtMag = settings.pickedPointMagnification;

	int delta[2] = {
		static_cast<int>(2 * ptRad / m_locRect.width() * ( (double) m_gridDims[0] ) + 1),
		static_cast<int>(2 * ptRad / m_locRect.height() * ( (double) m_gridDims[1] ) + 1)
	};
	int xrange[2] = { xbin - delta[0], xbin + delta[0] };
	if ( xrange[0] < 0 ) xrange[0] = 0;
	if ( xrange[1] > m_gridDims[0] ) xrange[1] = m_gridDims[0];
	int yrange[2] = { ybin - delta[1], ybin + delta[1] };
	if ( yrange[0] < 0 ) yrange[0] = 0;
	if ( yrange[1] > m_gridDims[1] ) yrange[1] = m_gridDims[1];

	double minDist = pow( pPtMag * ptRad, 2 );
	size_t res = NoPointIndex;
	for (int x = xrange[0]; x < xrange[1]; ++x)
	{
		for (int y = yrange[0]; y < yrange[1]; ++y)
		{
			binInd = getBinIndex(x, y);
			for (int indx = m_pointsGrid[binInd].size() - 1; indx >= 0; --indx)
			{
				size_t ptIdx = m_pointsGrid[binInd][indx];
				double pixelX = p2x(m_splomData->paramData(m_paramIndices[0])[ptIdx]);
				double pixelY = p2y(m_splomData->paramData(m_paramIndices[1])[ptIdx]);
				double dist = pow(pixelX - mpos.x(), 2) + pow(pixelY - mpos.y(), 2);
				if (dist < minDist && m_splomData->matchesFilter(ptIdx))
				{
					minDist = dist;
					res = ptIdx;
				}
			}
		}
	}
	return res;
}

QPointF iAScatterPlot::getPositionFromPointIndex( size_t idx ) const
{
	double x = p2x( m_splomData->paramData( m_paramIndices[0] )[idx] );
	double y = p2y( m_splomData->paramData( m_paramIndices[1] )[idx] );
	return QPointF( x, y );
}

void iAScatterPlot::updateSelectedPoints(bool append, bool remove)
{
	bool wasModified = false;
	auto & selInds = m_splom->getSelection();
	if (!append && !remove)
	{
		wasModified = selInds.size() > 0;
		selInds.clear();
	}
	if (m_selPoly.size() > 0)
	{
		QPolygonF pPoly;
		for (int i = 0; i < m_selPoly.size(); ++i)
		{
			QPointF p(x2p(m_selPoly.point(i).x()), y2p(m_selPoly.point(i).y()));
			pPoly.append(p);
		}
		int rangeBinX[2] = { p2binx(pPoly.boundingRect().left()), p2binx(pPoly.boundingRect().right()) };
		int rangeBinY[2] = { p2biny(pPoly.boundingRect().top()), p2biny(pPoly.boundingRect().bottom()) };
		for (int binx = rangeBinX[0]; binx <= rangeBinX[1]; ++binx)
		{
			for (int biny = rangeBinY[0]; biny <= rangeBinY[1]; ++biny)
			{
				auto const & pts = m_pointsGrid[getBinIndex(binx, biny)];
				for(auto i: pts)
				{
					if (!m_splomData->matchesFilter(i))
					{
						continue;
					}
					QPointF pt(m_splomData->paramData(m_paramIndices[0])[i], m_splomData->paramData(m_paramIndices[1])[i]);
					if (pPoly.containsPoint(pt, Qt::OddEvenFill))
					{
						auto pos = std::find(selInds.begin(), selInds.end(), i);
						if (pos != selInds.end() && remove)
						{
							selInds.erase(pos);
							wasModified = true;
						}
						else if (pos == selInds.end() && (!remove || append)) // achieves XOR-like behavior if both remove and append are true
						{
							selInds.push_back(i);
							wasModified = true;
						}
					}
				}
			}
		}
	}
	bool needToClearPolygon = m_selPoly.size() > 0;
	m_selPoly.clear();
	if (wasModified)
	{
		std::sort(selInds.begin(), selInds.end());
		emit selectionModified();
	}
	else if (needToClearPolygon)
	{
		m_parentWidget->update();
	}
}

void iAScatterPlot::updateDrawRect()
{
	m_locRect = QRectF( 0.0, 0.0, m_globRect.width(), m_globRect.height() );//QRectF( 0.5, 0.5, m_rect.width() - 1, m_rect.height() - 1 );
}

QPoint iAScatterPlot::getLocalPos( QPoint pos ) const
{
	return pos - m_globRect.topLeft();
}

QPoint iAScatterPlot::cropLocalPos( QPoint locPos ) const
{
	QPoint res = locPos;
	if (locPos.x() < 0)
	{
		res.setX(0);
	}
	if (locPos.x() > m_globRect.width() - 1)
	{
		res.setX(m_globRect.width() - 1);
	}
	if (locPos.y() < 0)
	{
		res.setY(0);
	}
	if (locPos.y() > m_globRect.height() - 1)
	{
		res.setY(m_globRect.height() - 1);
	}
	return res;
}

#ifndef CHART_OPENGL
void iAScatterPlot::drawPoint(QPainter& painter, double ptX, double ptY, int radius, QColor const & color)
{
	int tx = p2x(ptX);
	int ty = p2y(ptY);
	if (tx < 0 || tx > m_globRect.width() ||
		ty < 0 || ty > m_globRect.height())
	{ // point outside current screen
		return;
	}
	int size = 2 * radius;
	painter.setBrush(color);
	painter.drawEllipse(tx - radius, ty - radius, size, size);
}
#endif

void iAScatterPlot::drawPoints( QPainter &painter )
{
	if (!m_splomData)
	{
		return;
	}
	painter.save();
#ifdef CHART_OPENGL
	// all points
	int pwidth = m_parentWidget->width();
	int pheight = m_parentWidget->height();

	double ptRad = getPointRadius();
	double ptSize = 2 * ptRad;
	painter.beginNativePainting();
	int y = pheight - m_globRect.bottom() - 1; //Qt and OpenGL have inverted Y axes

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport( 0, 0, pwidth, pheight );
	glOrtho( 0, pwidth - 1, pheight - 1, 0, -1, 1 );
	glMatrixMode( GL_MODELVIEW );

	glPushMatrix();
	glLoadIdentity();
	glTranslated( m_globRect.x(), m_globRect.y(), 0.0 );
	glTranslated( m_offset.x(), m_offset.y(), 0.0 );
	glScaled( m_globRect.width() * m_scale, m_globRect.height() * m_scale, 1.0 );

	glScissor( m_globRect.left(), y, m_globRect.width(), m_globRect.height() );
	glEnable( GL_SCISSOR_TEST );
	if (!m_pointsBuffer->bind())//TODO: proper handling (exceptions?)
	{
		LOG(lvlWarn, "Failed to bind points buffer!");
		return;
	}
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glPointSize( ptSize );
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 7 * sizeof( GLfloat ), (const void *) ( 0 ) );
	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_FLOAT, 7 * sizeof( GLfloat ), (const void *) ( 3 * sizeof( GLfloat ) ) );
	assert(m_curVisiblePts < std::numeric_limits<GLsizei>::max());
	glDrawArrays( GL_POINTS, 0, static_cast<GLsizei>(m_curVisiblePts) );//glDrawElements( GL_POINTS, m_pointsBuffer->size(), GL_UNSIGNED_INT, 0 );
	glDisableClientState( GL_COLOR_ARRAY );
	glColor3f( settings.selectionColor.red() / 255.0, settings.selectionColor.green() / 255.0, settings.selectionColor.blue() / 255.0 );

	// draw selection:
	auto const & selInds = m_splom->getFilteredSelection();
	std::vector<uint> uintSelInds;
	for (size_t idx : selInds)
	{
		// copy doesn't work as it would require explicit conversion from size_t to uint
		uintSelInds.push_back(static_cast<uint>(idx));
	}
	// TODO: This still limits the data to be drawn to the maximum of unsigned int (i.e. 2^32!)
	//       but unfortunately, there is no GL_UNSIGNED_LONG_LONG (yet)
	glDrawElements(GL_POINTS, static_cast<GLsizei>(selInds.size()), GL_UNSIGNED_INT, uintSelInds.data());
	glDisableClientState( GL_VERTEX_ARRAY );
	m_pointsBuffer->release();

	// draw current point
	double anim = m_splom->getAnimIn();
	if (m_curInd != NoPointIndex)
	{
		double pPM = settings.pickedPointMagnification;
		double curPtSize = ptSize * linterp(1.0, pPM, anim);
		glPointSize(curPtSize);
		glBegin(GL_POINTS);
		if (m_lut->initialized())
		{
			double val = m_splomData->paramData(m_colInd)[m_curInd];
			double rgba[4]; m_lut->getColor(val, rgba);
			glColor4f(rgba[0], rgba[1], rgba[2], linterp(rgba[3], 1.0, anim));
		}
		double tx = p2tx(m_splomData->paramData(m_paramIndices[0])[m_curInd]);
		double ty = p2ty(m_splomData->paramData(m_paramIndices[1])[m_curInd]);
		glVertex3f(tx, ty, 0.0f);
		glEnd();
	}

	// draw highlighted points
	auto const & highlightedPoints = m_splom->getHighlightedPoints();
	for(auto ind: highlightedPoints)
	{
		double curPtSize = ptSize * settings.pickedPointMagnification;
		glPointSize(curPtSize);
		glBegin(GL_POINTS);
		if (m_lut->initialized())
		{
			double val = m_splomData->paramData(m_colInd)[ind];
			double rgba[4]; m_lut->getColor(val, rgba);
			glColor4f(rgba[0], rgba[1], rgba[2], 1.0);
		}
		double tx = p2tx(m_splomData->paramData(m_paramIndices[0])[ind]);
		double ty = p2ty(m_splomData->paramData(m_paramIndices[1])[ind]);
		glVertex3f(tx, ty, 0.0f);
		glEnd();
	}

	// draw previous point
	anim = m_splom->getAnimOut();
	if (m_prevPtInd != NoPointIndex && anim > 0.0)
	{
		double pPM = settings.pickedPointMagnification;
		double curPtSize = ptSize * linterp(1.0, pPM, anim);
		glPointSize(curPtSize);
		glBegin(GL_POINTS);
		if (m_lut->initialized())
		{
			double val = m_splomData->paramData(m_colInd)[m_prevPtInd];
			double rgba[4]; m_lut->getColor(val, rgba);
			glColor4f(rgba[0], rgba[1], rgba[2], linterp(rgba[3], 1.0, anim));
		}
		double tx = p2tx(m_splomData->paramData(m_paramIndices[0])[m_prevPtInd]);
		double ty = p2ty(m_splomData->paramData(m_paramIndices[1])[m_prevPtInd]);
		glVertex3f(tx, ty, 0.0f);
		glEnd();
	}

	glPopMatrix();
	painter.endNativePainting();
#else
	if (!m_lut->initialized())
	{
		return;
	}
	m_curVisiblePts = 0;
	painter.setPen(Qt::NoPen);
	for (size_t i = 0; i < m_splomData->numPoints(); ++i)
	{
		if (!m_splomData->matchesFilter(i))
		{
			continue;
		}
		QColor color(m_lut->getQColor(m_splomData->paramData(m_colInd)[i]));
		drawPoint(painter, m_splomData->paramData(m_paramIndices[0])[i], m_splomData->paramData(m_paramIndices[1])[i], getPointRadius(), color);
		++m_curVisiblePts;
	}
	auto const& selInds = m_splom->getFilteredSelection();
	for (size_t idx : selInds)
	{
		drawPoint(painter, m_splomData->paramData(m_paramIndices[0])[idx], m_splomData->paramData(m_paramIndices[1])[idx],
			getPointRadius(),
			settings.selectionColor);
	}
	/*
	// not sure what highlighted points are, leave them out for the moment...
	auto const& highlightedPoints = m_splom->getHighlightedPoints();
	for (auto ind : highlightedPoints)
	{
	}
	*/
#endif
	painter.restore();
}

void iAScatterPlot::drawSelectionPolygon( QPainter &painter )
{
	if ( m_selPoly.size() )
	{
		painter.setBrush( settings.selectionPolyColor );
		painter.setPen( settings.selectionPolyColor /* m_parentWidget->palette().color(QPalette::Text) */ );
		painter.drawPolygon( m_selPoly );
	}
}

void iAScatterPlot::drawBorder( QPainter &painter )
{
	QPen pen( settings.plotBorderColor );
	pen.setJoinStyle( Qt::MiterJoin );
	if ( m_isPreviewPlot )
	{
		pen.setColor( settings.previewBorderColor );
		pen.setWidthF( settings.previewBorderWidth );
	}
	painter.setPen( pen );
	painter.drawPolyline( QRectF( 0.5, 0.5, m_locRect.width() - 1, m_locRect.height() - 1 ) ); //m_locRect );
}

void iAScatterPlot::drawTicks( QPainter &painter )
{
	painter.save();
	QPen p;
		p.setColor( /*settings.tickLineColor*/ m_parentWidget->palette().color(QPalette::Midlight) );
		p.setStyle( Qt::DotLine );
	painter.setPen( p );
	for (double t: m_ticksX)
	{
		double loc_t = p2x( t );
		painter.drawLine( QPointF( loc_t, m_locRect.top() ), QPointF( loc_t, m_locRect.bottom() ) );
	}
	for (double t : m_ticksY)
	{
		double loc_t = p2y( t );
		painter.drawLine( QPointF( m_locRect.left(), loc_t ), QPointF( m_locRect.right(), loc_t ) );
	}

	//if maximized plot also draw tick labels
	if (m_isMaximizedPlot)
	{
		drawMaximizedLabels(painter);
	}

	painter.restore();
}

void iAScatterPlot::drawMaximizedLabels( QPainter &painter )
{
	painter.save();

	//tick labels
	int tO = settings.tickOffset, \
		tS = settings.tickSpacing, \
		mPO = settings.maximizedParamsOffset, \
		tRH = settings.textRectHeight;
	painter.setPen( /*settings.tickLabelColor*/ m_parentWidget->palette().color(QPalette::Text) );

	for (double t : m_ticksY)
	{
		painter.drawText(QRectF(-tO, p2y(t) - tO, tO - tS, 2 * tO), Qt::AlignRight | Qt::AlignVCenter, QString::number(t));
	}
	painter.rotate( -90 );
	for (double t : m_ticksX)
	{
		painter.drawText(QRectF(tS, p2x(t) - tO, tO - tS, 2 * tO), Qt::AlignLeft | Qt::AlignVCenter, QString::number(t));
	}

	//parameter names
	QFont font = painter.font(); font.setBold( true ); painter.setFont( font );
	QString paramNames[2] = { m_splomData->parameterName( m_paramIndices[0] ), m_splomData->parameterName( m_paramIndices[1] ) };
	QRectF textRect( -m_locRect.height(), -tO - mPO - tRH, m_locRect.height(), tRH );
	painter.drawText( textRect, Qt::AlignHCenter | Qt::AlignBottom, paramNames[1] );
	painter.rotate( 90 );
	textRect = QRectF( 0, -tO - mPO - tRH, m_locRect.width(), tRH );
	painter.drawText( textRect, Qt::AlignHCenter | Qt::AlignBottom, paramNames[0] );

	painter.restore();
}

#ifdef CHART_OPENGL
void iAScatterPlot::createVBO()
{
	if (!m_parentWidget->isVisible())
		return;
	m_parentWidget->makeCurrent();

	if (!m_splomData)
		return;

	m_pointsBuffer = new iAQGLBuffer( iAQGLBuffer::VertexBuffer );
	if (!m_pointsBuffer->create())
	{
		m_pointsBuffer = nullptr;
		return;
	}
	bool res = m_pointsBuffer->bind();
	if (!res)
	{
		LOG(lvlWarn, "Binding points buffer failed!");
		return;
	}
	m_pointsBuffer->setUsagePattern(iAQGLBuffer::DynamicDraw);
	m_pointsBuffer->allocate(static_cast<int>((CordDim + ColChan) * m_splomData->numPoints() * sizeof(GLfloat)));
	m_pointsBuffer->release();
}

void iAScatterPlot::fillVBO()
{
	//draw data points
	if (!hasData() || !m_lut->initialized())
	{
		return;
	}
	int elSz = CordDim + ColChan;
	bool res = m_pointsBuffer->bind();
	if (!res)
	{
		LOG(lvlWarn, "Binding points buffer failed!");
		return;
	}

	GLfloat * buffer = static_cast<GLfloat *>(m_pointsBuffer->map(iAQGLBuffer::ReadWrite));

	assert(buffer);

	m_curVisiblePts = 0;
	for ( size_t i = 0; i < m_splomData->numPoints(); ++i )
	{
		if (!m_splomData->matchesFilter(i))
		{
			continue;
		}
		double tx = p2tx( m_splomData->paramData( m_paramIndices[0] )[i] );
		double ty = p2ty( m_splomData->paramData( m_paramIndices[1] )[i] );
		buffer[elSz * m_curVisiblePts + 0] = tx;
		buffer[elSz * m_curVisiblePts + 1] = ty;
		buffer[elSz * m_curVisiblePts + 2] = 0.0;
		if ( m_lut->initialized() )
		{
			double val = m_splomData->paramData( m_colInd )[i];
			double rgba[4]; m_lut->getColor( val, rgba );
			buffer[elSz * m_curVisiblePts + 3] = rgba[0];
			buffer[elSz * m_curVisiblePts + 4] = rgba[1];
			buffer[elSz * m_curVisiblePts + 5] = rgba[2];
			buffer[elSz * m_curVisiblePts + 6] = rgba[3];
		}
		++m_curVisiblePts;
	}
	bool res2 = m_pointsBuffer->unmap();
	if (!res2)
	{
		LOG(lvlWarn, "Unbinding points buffer failed!");
		return;
	}
	m_pointsBuffer->release();
	m_pointsOutdated = false;
}
#endif

void iAScatterPlot::setSelectionColor(QColor selCol)
{
	settings.selectionColor = selCol;
}

double iAScatterPlot::getPointRadius() const
{
	double res = settings.pointRadius;
	if ( m_isMaximizedPlot )
		res *= settings.maximizedPointMagnification;
	return res;
}

void iAScatterPlot::setPointRadius(double radius)
{
	settings.pointRadius = radius;
}

double iAScatterPlot::scc()
{
	if (!m_sccValid)
	{
		m_scc = spearmansCorrelationCoefficient(m_splomData->paramData(m_paramIndices[0]), m_splomData->paramData(m_paramIndices[1]));
		m_sccValid = true;
	}
	return m_scc;
}

double iAScatterPlot::pcc()
{
	if (!m_pccValid)
	{
		m_pcc = pearsonsCorrelationCoefficient(m_splomData->paramData(m_paramIndices[0]), m_splomData->paramData(m_paramIndices[1]));
		m_pccValid = true;
	}
	return m_pcc;
}
