#pragma once

#include "tapkee/tapkee.hpp"
#include "ui_TimeCurvesWidget.h"
#include "iAChartWidget.h"
#include "iATimeCurves.h"
#include <iALog.h>

#include <qchart.h>
#include <qsplineseries.h>
#include <qwidget.h>
#include <qchartview.h>
#include <QMimeData.h>

struct TimeCurve
{
	tapkee::DenseMatrix embedding;
	QStringList* fileNames;
	QString name;

	QString toString()
	{
		Eigen::IOFormat CleanFmt(4, Eigen::DontAlignCols, ",", ";", "", "");
		std::stringstream buffer;
		buffer << name.toStdString() << ";";
		for (int i = 0; i < fileNames->size(); i++)
		{
			buffer << fileNames->at(i).toStdString() << ";";
		}
		buffer << embedding.transpose().format(CleanFmt);
		QString str = QString::fromStdString(buffer.str());
		LOG(lvlDebug, str);
		return str;
	};
}; 

class iATimeCurvesWidget : public QWidget
{
	Q_OBJECT

public:
	explicit iATimeCurvesWidget(
		TimeCurve timecurvedata, iATimeCurves* main, iAMainWindow* mainWindow, QWidget* parent = nullptr);

private:
	Ui::TimeCurvesWidgetClass ui;
	iAMainWindow* m_mainWindow;
	iATimeCurves* m_main;
	tapkee::DenseMatrix embedding;
	QStringList* fileNames;
	iAChartWidget* chartWidget;
	QList<TimeCurve>* dataList;
	//for drag event
	QPoint startPos;
	//Rotates the embedding so that start and end point are horizontally aligned
	void rotateData(TimeCurve* timecurve);
	//Parses time curve struct from string representation
	TimeCurve timeCurveFromString(QString str);
	//prints matrix to Log
	void printMatrixToLog(Eigen::MatrixXd matrix);

private slots:
	void saveJson();
	void populateTable(TimeCurve data);
	void addSeries(TimeCurve data);
	void pointClicked_old(const QPointF &point);
	void pointClicked(double xValue, Qt::KeyboardModifiers _t2);
	void on_resetViewButton_clicked(bool checked);
	//drag and drop
	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dropEvent(QDropEvent* event);
	//start drag events
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void startDrag();

	//todo void selectPoint();
	//todo void hover()
};
