#include "iADistanceMatrix.h"

#include <iALog.h>

//Qt
#include <QPushButton.h>

iADistanceMatrix::iADistanceMatrix(tapkee::DenseMatrix embedding, QStringList* fileNames, QWidget* parent) :
	embedding(embedding), fileNames(fileNames), QWidget(parent)
{
	ui.setupUi(this);
	chartView = new QChartView();
	ui.verticalLayout->addWidget(chartView);
	chart = new QChart();
	addSeries(this->embedding);
	chartView->setChart(chart);
	ui.verticalLayout->addWidget(chartView);
	connect(ui.saveButton, &QPushButton::clicked, 
		this, &iADistanceMatrix::saveJson);
	
	populateTable();
}

void iADistanceMatrix::addSeries(tapkee::DenseMatrix embedding)
{
	//example series todo delete
	QSplineSeries* exampleSeries = new QSplineSeries();
	exampleSeries->setPointsVisible(true);
	exampleSeries->setPointLabelsVisible(true);
	exampleSeries->append(0, 6);
	exampleSeries->append(1, 1);
	exampleSeries->append(2, 4);
	chart->addSeries(exampleSeries);

	QSplineSeries* series = new QSplineSeries();
	series->setPointsVisible(true);
	QPen pen(2);
	series->setPen(pen);
	series->setPointLabelsVisible(true);
	//todo: change pointLabelsFormat to show filename
	for (int i = 0; i < embedding.cols(); i++)
	{
		series->append(embedding(0, i), embedding(1, i));
	}
	//todo transform range
	chart->addSeries(series);
	//chart->createDefaultAxes();

	connect(series, &QXYSeries::clicked, this, &iADistanceMatrix::pointClicked);
}

void iADistanceMatrix::hover()
{
	return;
}

void iADistanceMatrix::pointClicked(const QPointF &point)
{
	LOG(lvlDebug, QString("Point clicked: '%1', '%2'").arg(point.x()).arg(point.y()));
	//todo selection
	//if point in list -> add to selected
	return;
}

void iADistanceMatrix::saveJson()
{
	return;
}

void iADistanceMatrix::populateTable()
{
	QTableWidget* tableWidget = ui.tableWidget;
	tableWidget->setRowCount(embedding.rows());
	tableWidget->setColumnCount(embedding.cols());
	for (int i = 0; i < embedding.rows(); i++)
	{
		for (int j = 0; j < embedding.cols(); j++)
		{
			double value = embedding(i, j);
			tableWidget->setItem(i, j, new QTableWidgetItem(QString::number(value)));
		}
	}
	LOG(lvlDebug, QString("Created table, first item is: '%1'").arg(tableWidget->itemAt(1, 1)->text()));
}
