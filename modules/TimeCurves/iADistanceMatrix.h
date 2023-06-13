#pragma once

#include "tapkee/tapkee.hpp"
#include "ui_iADistanceMatrix.h"

#include <qchart.h>
#include <qsplineseries.h>
#include <qwidget.h>
#include <qchartview.h>

class iADistanceMatrix : public QWidget
{
	Q_OBJECT

public:
	explicit iADistanceMatrix(tapkee::DenseMatrix embedding, QStringList* fileNames, QWidget* parent = nullptr);

private:
	Ui::DistanceMatrixClass ui;
	tapkee::DenseMatrix embedding;
	QStringList* fileNames;
	QChartView* chartView;
	QChart* chart;
	QSplineSeries* series;

private slots:
	void saveJson();
	void populateTable();
	void addSeries(tapkee::DenseMatrix embedding);
	void hover();
	void pointClicked(const QPointF &point);

	
	//example:
	//void on_tableWidget_cellClicked(int row, int column);

	//todo void selectPoint();
	//todo void hover()
};
