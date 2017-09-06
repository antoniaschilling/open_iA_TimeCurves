/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAChartView.h"

#include "iAColors.h"
#include "iAConsole.h"
#include "iAToolsVTK.h"
#include "qcustomplot.h"

#include <vtkImageData.h>


iAChartView::iAChartView()
{
	m_plot = new QCustomPlot();
	m_plot->setInteraction(QCP::iRangeDrag, true);
	m_plot->setInteraction(QCP::iRangeZoom, true);
	m_plot->setInteraction(QCP::iMultiSelect, true);
	m_plot->setMultiSelectModifier(Qt::ShiftModifier);
	m_plot->setInteraction(QCP::iSelectPlottables, true);
	connect(m_plot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(chartMousePress(QMouseEvent *)));
	setLayout(new QVBoxLayout());
	layout()->addWidget(m_plot);
	auto datasetChoiceContainer = new QWidget();
	datasetChoiceContainer->setLayout(new QVBoxLayout());
	m_xAxisChooser = new QWidget();
	m_xAxisChooser->setLayout(new QHBoxLayout());
	m_xAxisChooser->layout()->setSpacing(0);
	m_xAxisChooser->layout()->addWidget(new QLabel("X Axis"));
	m_yAxisChooser = new QWidget();
	m_yAxisChooser->setLayout(new QHBoxLayout());
	m_yAxisChooser->layout()->setSpacing(0);
	m_yAxisChooser->layout()->addWidget(new QLabel("Y Axis"));
	datasetChoiceContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	datasetChoiceContainer->layout()->addWidget(m_xAxisChooser);
	datasetChoiceContainer->layout()->addWidget(m_yAxisChooser);
	layout()->addWidget(datasetChoiceContainer);
}


void iAChartView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
	m_plot->removePlottable(0);

	int * dim = imgX->GetDimensions();
	m_voxelCount = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
	QVector<double> x, y, t;
	x.reserve(m_voxelCount);
	y.reserve(m_voxelCount);
	t.reserve(m_voxelCount);
	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	std::copy(bufX, bufX + m_voxelCount, std::back_inserter(x));
	std::copy(bufY, bufY + m_voxelCount, std::back_inserter(y));
	for (int i = 0; i < m_voxelCount; ++i)
	{	// unfortunately we seem to require this additional storage
		t.push_back(i);  // to make QCustomPlot not sort the data
	}

	auto curve = new QCPCurve(m_plot->xAxis, m_plot->yAxis);
	curve->setData(t, x, y, true);
	curve->setLineStyle(QCPCurve::lsNone);
	curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Uncertainty::ChartColor, 2));
	curve->setSelectable(QCP::stMultipleDataRanges);
	curve->selectionDecorator()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Uncertainty::SelectionColor, 2));
	connect(curve, SIGNAL(selectionChanged(QCPDataSelection const &)), this, SLOT(selectionChanged(QCPDataSelection const &)));

	m_plot->xAxis->setLabel(captionX);
	m_plot->yAxis->setLabel(captionY);
	m_plot->xAxis->setRange(0, 1);
	m_plot->yAxis->setRange(0, 1);
	m_plot->replot();

	m_selectionImg = AllocateImage(imgX);
}


void iAChartView::SetDatasets(QSharedPointer<iAUncertaintyImages> imgs)
{
	m_imgs = imgs;
	m_xAxisChoice = iAUncertaintyImages::LabelDistributionEntropy;
	m_yAxisChoice = iAUncertaintyImages::AvgAlgorithmEntropyProbSum;
	for (int i = 0; i < iAUncertaintyImages::SourceCount; ++i)
	{
		QToolButton* xButton = new QToolButton(); xButton->setText(imgs->GetSourceName(i));
		QToolButton* yButton = new QToolButton(); yButton->setText(imgs->GetSourceName(i));
		xButton->setProperty("imgId", i);
		yButton->setProperty("imgId", i);
		xButton->setAutoExclusive(true);
		xButton->setCheckable(true);
		yButton->setAutoExclusive(true);
		yButton->setCheckable(true);
		if (i == m_xAxisChoice)
		{
			xButton->setChecked(true);
		}
		if (i == m_yAxisChoice)
		{
			yButton->setChecked(true);
		}
		connect(xButton, SIGNAL(clicked()), this, SLOT(xAxisChoice()));
		connect(yButton, SIGNAL(clicked()), this, SLOT(yAxisChoice()));
		m_xAxisChooser->layout()->addWidget(xButton);
		m_yAxisChooser->layout()->addWidget(yButton);
	}
	AddPlot(imgs->GetEntropy(m_xAxisChoice), imgs->GetEntropy(m_yAxisChoice),
		imgs->GetSourceName(m_xAxisChoice), imgs->GetSourceName(m_yAxisChoice));
}


void iAChartView::xAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_xAxisChoice)
		return;
	m_xAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAChartView::yAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_yAxisChoice)
		return;
	m_yAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAChartView::selectionChanged(QCPDataSelection const & selection)
{
	DEBUG_LOG("Selection Changed");
	double* buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int v=0; v<m_voxelCount; ++v)
	{
		*buf = 0;
		buf++;
	}
	buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int r = 0; r < selection.dataRangeCount(); ++r)
	{
		std::fill(buf + selection.dataRange(r).begin(), buf + selection.dataRange(r).end(), 1);
	}
	m_selectionImg->Modified();

	//StoreImage(m_selectionImg, "C:/Users/p41143/selection.mhd", true);
	emit SelectionChanged();
}


vtkImagePointer iAChartView::GetSelectionImage()
{
	return m_selectionImg;
}


void iAChartView::chartMousePress(QMouseEvent *)
{
	if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
	{	// allow selection with Ctrl key
		m_plot->setSelectionRectMode(QCP::srmSelect);
	}
	else
	{	// enable dragging otherwise
		m_plot->setSelectionRectMode(QCP::srmNone);
	}
}
