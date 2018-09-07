/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iATripleModalityWidget.h"

// TODO: should be removed in the future
#include <QMouseEvent>
#include <QWheelEvent>

class iAHistogramTriangle : public iATripleModalityWidget
{
public:
	iAHistogramTriangle(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f = 0);

	void initialize() override;
	bool isSlicerInteractionEnabled() override { return true; }
	void setModalityLabel(QString label, int index) override;

	void paintHistograms(QPainter& p);
	void paintSlicers(QPainter& p);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event);

	void mousePressEvent(QMouseEvent *event) { forwardMouseEvent(event); lastMousePos = event->pos(); }
	void mouseMoveEvent(QMouseEvent *event) { forwardMouseEvent(event); lastMousePos = event->pos(); }
	void mouseReleaseEvent(QMouseEvent *event) { forwardMouseEvent(event); dragging = false; lastMousePos = event->pos(); }
	void wheelEvent(QWheelEvent *event) { forwardWheelEvent(event); }

private:
	void calculatePositions() { calculatePositions(size().width(), size().height()); }
	void calculatePositions(int w, int h);

	QPoint lastMousePos;
	QWidget *lastTarget;
	bool dragging = false;

	void forwardMouseEvent(QMouseEvent *event);
	void forwardWheelEvent(QWheelEvent *event);
	iADiagramFctWidget* onHistogram(QPoint p, QPoint &transformed);
	bool onTriangle(QPoint p);
	iASimpleSlicerWidget* onSlicer(QPoint p, QPoint &transformed);

	QPainterPath m_clipPath;
	QPen m_clipPathPen;

	QTransform m_transformHistograms[3]; // left, right, bottom (respectively)
	QRect m_histogramsRect;

	QTransform m_transformSlicers[3]; // left(A), top(B), right(C) (respectively)
	BarycentricTriangle m_slicerTriangles[3];
	QPainterPath m_slicerClipPaths[3];
	QBrush m_slicerBackgroundBrush;

	int m_triangleBigWidth;
};