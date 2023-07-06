#pragma once

#include "tapkee/tapkee.hpp"
#include "ui_TimeCurvesWidget.h"

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
	QChartView* chartView;
	QChart* chart;
	QHash<QXYSeries::PointConfiguration, QVariant> selectionConf;
	//QHash<QXYSeries*,TimeCurve> seriesToFileNames;
	QList<TimeCurve>* dataList;
	void drawCustomLabels(QPainter* painter, const QList<QPointF> points, const int offset);
	void getLabel(QXYSeries* series, QPointF position, int index);

private slots:
	void saveJson();
	void populateTable();
	void addSeries(TimeCurve data);
	void hover();
	void pointClicked(const QPointF &point);
	void on_showLabels_clicked(bool checked);

	//todo void selectPoint();
	//todo void hover()
};
