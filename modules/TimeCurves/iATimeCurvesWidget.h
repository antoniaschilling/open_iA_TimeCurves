#pragma once

#include "tapkee/tapkee.hpp"
#include "ui_TimeCurvesWidget.h"
#include "iAChartWidget.h"

#include <qchart.h>
#include <qsplineseries.h>
#include <qwidget.h>
#include <qchartview.h>

struct TimeCurve
{
	tapkee::DenseMatrix embedding;
	QStringList* fileNames;
	QString name;
};

class iATimeCurvesWidget : public QWidget
{
	Q_OBJECT

public:
	explicit iATimeCurvesWidget(tapkee::DenseMatrix embedding, QStringList* fileNames, QWidget* parent = nullptr);

private:
	Ui::TimeCurvesWidgetClass ui;
	tapkee::DenseMatrix embedding;
	QStringList* fileNames;
	iAChartWidget* chartWidget;
	QList<TimeCurve>* dataList;
	//Rotates the embedding so that start and end point are horizontally aligned
	void rotateData(TimeCurve* timecurve);
	//prints matrix to Log
	void printMatrixToLog(Eigen::MatrixXd matrix);

private slots:
	void saveJson();
	void populateTable(TimeCurve data);
	void addSeries(TimeCurve data);
	void pointClicked_old(const QPointF &point);
	void pointClicked(double xValue, Qt::KeyboardModifiers _t2);
	void on_resetViewButton_clicked(bool checked);
	//todo void selectPoint();
	//todo void hover()
};
