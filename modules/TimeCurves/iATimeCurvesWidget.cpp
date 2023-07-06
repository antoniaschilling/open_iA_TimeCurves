#include "iATimeCurvesWidget.h"

#include <iALog.h>
#include <iAPlotTypes.h>
//#include <iASplinePlot.h>
#include <iAXYPlotData.h>
#include <iAChartWidget.h>

//Qt
#include <QPushButton.h>

iATimeCurvesWidget::iATimeCurvesWidget(tapkee::DenseMatrix embedding, QStringList* fileNames, QWidget* parent) :
	embedding(embedding), fileNames(fileNames), QWidget(parent)
{
	ui.setupUi(this);
	//todo refactor (delete=)
	chartView = new QChartView();
	ui.verticalLayout->addWidget(chartView);
	chart = new QChart();
	dataList = new QList<TimeCurve>();
	TimeCurve firstData;
	firstData.embedding = embedding;
	firstData.fileNames = fileNames;
	addSeries(firstData);
	chartView->setChart(chart);
	ui.verticalLayout->addWidget(chartView);
	connect(ui.saveButton, &QPushButton::clicked, this, &iATimeCurvesWidget::saveJson);

	selectionConf[QXYSeries::PointConfiguration::Size] = 8;
	selectionConf[QXYSeries::PointConfiguration::LabelVisibility] = true;

	populateTable();
}



void iATimeCurvesWidget::addSeries(TimeCurve data)
{
	//setup chartWidget
	iAChartWidget * chartWidget = new iAChartWidget(this, "xLabel", "Ylabel");
	chartWidget->setMinimumHeight(120);
	ui.verticalLayout->addWidget(chartWidget);
	//ui.chartWidget->layout()->addWidget(chartWidget);

	//todo plot collection
	QSharedPointer<iAXYPlotData> plotData = iAXYPlotData::create("name", iAValueType::Continuous, data.embedding.size());
	for (int i = 0; i < data.embedding.cols(); i++)
	{
		plotData.data()->addValue(data.embedding(0, i), data.embedding(1, i));
	}
	
	//todo iALinearMapper?

	chartWidget->addPlot(QSharedPointer<iALinePlot>::create(plotData, QColor(QColorConstants::Black)));
	chartWidget->addPlot(QSharedPointer<iASplinePlot>::create(plotData, QColor(QColorConstants::Black)));
	chartWidget->update();
	//chartWidget->setVisible(true);




	////example series todo delete
	//QSplineSeries* exampleSeries = new QSplineSeries();
	//exampleSeries->setPointsVisible(true);
	//exampleSeries->setPointLabelsVisible(true);
	//exampleSeries->append(0, 6);
	//exampleSeries->append(1, 1);
	//exampleSeries->append(2, 4);
	//exampleSeries->append(3, 3);
	//chartWidget->addSeries(exampleSeries);
	//connect(exampleSeries, &QXYSeries::clicked, this, &iATimeCurvesWidget::pointClicked);
	//QStringList names = {"point 1", "point 2", "point 3", "point 4"};
	//TimeCurve t;
	////todo
	//t.fileNames = fileNames;
	////seriesToFileNames.insert(exampleSeries, firstData);
	//dataList->append(t);

	//QSplineSeries* series = new QSplineSeries();
	//series->setPointsVisible(true);
	//QPen pen(2);
	//series->setPen(pen);
	////todo custom label
	////series->setPointLabelsFormat(label(@xPoint)"(@xPoint, @yPoint)");
	////todo: change pointLabelsFormat to show filename
	//for (int i = 0; i < data.embedding.cols(); i++)
	//{
	//	series->append(data.embedding(0, i), data.embedding(1, i));
	//}
	////todo transform range
	//chartWidget->addSeries(series);
	////seriesToFileNames.insert(series, data);
	//dataList->append(data);
	////chartWidget->createDefaultAxes();

	//connect(series, &QXYSeries::clicked, this, &iATimeCurvesWidget::pointClicked);
}

void iATimeCurvesWidget::hover()
{
	return;
}

void iATimeCurvesWidget::pointClicked(const QPointF& point)
{
	LOG(lvlDebug, QString("Point clicked: '%1', '%2'").arg(point.x()).arg(point.y()));
	//todo selection
	//if point in list -> add to selected, make bigger, show label
	for (QAbstractSeries* series : chart->series())
	{
		QXYSeries* xySeries = qobject_cast<QXYSeries*>(series);
		QList<QPointF> points = xySeries->points();
		int index = points.indexOf(point);
		if (index != -1)
		{
			xySeries->toggleSelection(QList<int>(index));
			if (xySeries->isPointSelected(index))
			{
				xySeries->setPointConfiguration(index, selectionConf);
				//todo change to selectionChanged slot
				QList<QPointF> list = QList<QPointF>();
				list.append(point);
				drawCustomLabels(new QPainter, list, 2);
			}
		}
	}
	return;
}

void iATimeCurvesWidget::on_showLabels_clicked(bool checked)
{
	for (QAbstractSeries* series : chart->series())
	{
		qobject_cast<QXYSeries*>(series)->setPointLabelsVisible(checked);
		drawCustomLabels(new QPainter(), qobject_cast<QXYSeries*>(series)->points(), 2);
	}
}

void iATimeCurvesWidget::drawCustomLabels(QPainter* painter, QList<QPointF> points, const int offset)
{
	if (points.isEmpty())
		return;

	QFontMetrics fm(painter->font());
	const int labelOffset = offset + 2;
	//painter->setFont(m_pointLabelsFont);        // Use QXYSeries::pointLabelsFont() to access m_pointLabelsFont
	painter->setPen(
		QPen(QColor(QColorConstants::Black)));  // Use QXYSeries::pointLabelsColor() to access m_pointLabelsColor

	for (QPointF point : points)
	{
		// todo fix drawing double labels
		for (int i = 0; i < chart->series().size(); i++)
		//for (QAbstractSeries* series : chartWidget->series())
		{
			//QXYSeries* xyseries(qobject_cast<QXYSeries*>(series));
			QXYSeries* xyseries(qobject_cast<QXYSeries*>(chart->series().at(i)));
			int index = xyseries->points().indexOf(point);
			if (index != -1)
			{
				LOG(lvlDebug, QString("point found at index '%1").arg(index));
				//QStringList* list = dataList->at(chartWidget->series().indexOf(series)).fileNames;
				QStringList* list = dataList->at(i).fileNames;
				QString pointLabel = list->at(index);
				//QString pointLabel = seriesToFileNames.value(xyseries).fileNames->at(xyseries->points().indexOf(point));  // Set the desired label for the i-th point of the series

				QPointF position(point);
				//position.setX(position.x() - pointLabelWidth / 2);
				//position.setY(position.y() - labelOffset);
				//getLabel(xyseries, position, xyseries->points().indexOf(point));
				//int pointLabelWidth = fm.width(pointLabel);
				painter->drawText(position, pointLabel);

				QRectF boundingRect = fm.boundingRect(pointLabel);
				boundingRect.moveCenter(point);
				boundingRect.marginsAdded(QMarginsF(0, 0, 0, offset));
				painter->drawText(boundingRect, Qt::AlignCenter, pointLabel);
			}
		}
	}
}

void iATimeCurvesWidget::getLabel(QXYSeries* series, QPointF position, int index)
{
}

void iATimeCurvesWidget::saveJson()
{
	return;
}

void iATimeCurvesWidget::populateTable()
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
