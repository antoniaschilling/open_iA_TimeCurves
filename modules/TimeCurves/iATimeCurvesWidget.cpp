#include "iATimeCurvesWidget.h"

#include <iALog.h>
#include <iAPlotTypes.h>
#include <iAXYPlotData.h>
#include <iAXYPlotUnorderedData.h>

#include "iAMapperImpl.h"

//Qt
#include <QPushButton.h>
#include <QRect.h>

iATimeCurvesWidget::iATimeCurvesWidget(tapkee::DenseMatrix embedding, QStringList* fileNames, QWidget* parent) :
	embedding(embedding), fileNames(fileNames), QWidget(parent)
{
	ui.setupUi(this);

	//chartWidget config
	chartWidget = new iAChartWidget(this, "xLabel", "Ylabel");
	//chartWidget->setMinimumHeight(300);
	//todo size hint
	//todo size policy
	chartWidget->setMinimumSize(500, 500);
	//or setHeightForWidth dependent
	chartWidget->setContentsMargins(10,10,10,10);
	chartWidget->setSizePolicy(QSizePolicy::QSizePolicy());
	chartWidget->setYMappingMode(iAChartWidget::AxisMappingType::Linear);
	chartWidget->showLegend(true);
	//todo selection

	ui.verticalLayout->addWidget(chartWidget);
	//todo refactor (delete=)
	//chartView = new QChartView();
	//ui.verticalLayout->addWidget(chartView);
	//chart = new QChart();
	dataList = new QList<TimeCurve>();
	TimeCurve firstData;
	firstData.embedding = embedding;
	firstData.fileNames = fileNames;
	addSeries(firstData);
	//chartView->setChart(chart);
	//ui.verticalLayout->addWidget(chartView);
	connect(ui.saveButton, &QPushButton::clicked, this, &iATimeCurvesWidget::saveJson);
	connect(chartWidget, &iAChartWidget::clicked, this, &iATimeCurvesWidget::pointClicked);
	//connect(ui.rese)
	/*selectionConf[QXYSeries::PointConfiguration::Size] = 8;
	selectionConf[QXYSeries::PointConfiguration::LabelVisibility] = true;*/

	populateTable();
}



void iATimeCurvesWidget::addSeries(TimeCurve data)
{
	//setup chartWidget
	//ui.chartWidget->layout()->addWidget(chartWidget);

	//todo plot collection
	QSharedPointer<iAXYPlotUnorderedData> plotData =
		iAXYPlotUnorderedData::create("name", iAValueType::Continuous, data.embedding.size(), *data.fileNames);
	//for debugging
	QSharedPointer<iAXYPlotData> plotDataOrdered =
		iAXYPlotData::create("name", iAValueType::Continuous, data.embedding.size());
	for (int i = 0; i < data.embedding.cols(); i++)
	{
		plotData.data()->addValue(data.embedding(0, i), data.embedding(1, i));
		//todo delete
		plotDataOrdered.data()->addValue(data.embedding(0, i), data.embedding(1, i));
	}
	
	QSharedPointer<iASplinePlot> splinePlot = QSharedPointer<iASplinePlot>::create(plotData, QColor(QColorConstants::Black));
	//chartWidget->addPlot(QSharedPointer<iALinePlot>::create(plotDataOrdered, QColor(QColorConstants::Black)));
	chartWidget->addPlot(splinePlot);
	LOG(lvlDebug, QString("Chart with fullChartWidth: '%1'").arg(chartWidget->fullChartWidth()));
	//todo set custom bounds
	//QSharedPointer<iAMapper> const xMapper = QSharedPointer<iALinearMapper>::create(
	//	chartWidget->xBounds()[0], chartWidget->xBounds()[1], chartWidget->xBounds()[0], chartWidget->xBounds()[1]);
	//QSharedPointer<iAMapper> const yMapper = QSharedPointer<iALinearMapper>::create(
	//	chartWidget->yBounds()[0], chartWidget->yBounds()[1], chartWidget->yBounds()[0], chartWidget->yBounds()[1]);
	//QRectF boundingBox = splinePlot.data()->getBoundingBox(*xMapper.data(), *yMapper.data());
	////todo multiple plots
	//chartWidget->setXBounds(boundingBox.left(), boundingBox.right());
	//chartWidget->setYBounds(boundingBox.bottom(), boundingBox.top());
	//chartWidget->addPlot(splinePlot);


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

void iATimeCurvesWidget::pointClicked(double xValue, Qt::KeyboardModifiers _t2)
{
	//todo iterate over plots
	iAPlot* plot = chartWidget->plots().at(0).data();
	int idx = plot->data().data()->nearestIdx(xValue);
	if (idx != -1)
	{
		QString name = plot->data().data()->toolTipText(idx);
		LOG(lvlDebug, QString("Point at x: '%1' is data: '%2' in dataset '%3'").arg(xValue).arg(name).arg(plot->data().data()->name()));
	}
	return;
}

void iATimeCurvesWidget::pointClicked_old(const QPointF& point)
{
	LOG(lvlDebug, QString("Point clicked: '%1', '%2'").arg(point.x()).arg(point.y()));
	//todo selection
	//if point in list -> add to selected, make bigger, show label
	//for (QAbstractSeries* series : chart->series())
	//{
	//	QXYSeries* xySeries = qobject_cast<QXYSeries*>(series);
	//	QList<QPointF> points = xySeries->points();
	//	int index = points.indexOf(point);
	//	if (index != -1)
	//	{
	//		xySeries->toggleSelection(QList<int>(index));
	//		if (xySeries->isPointSelected(index))
	//		{
	//			xySeries->setPointConfiguration(index, selectionConf);
	//			//todo change to selectionChanged slot
	//			QList<QPointF> list = QList<QPointF>();
	//			list.append(point);
	//			drawCustomLabels(new QPainter, list, 2);
	//		}
	//	}
	//}
	return;
}

void iATimeCurvesWidget::on_showLabels_clicked(bool checked)
{
	/*for (QAbstractSeries* series : chart->series())
	{
		qobject_cast<QXYSeries*>(series)->setPointLabelsVisible(checked);
		drawCustomLabels(new QPainter(), qobject_cast<QXYSeries*>(series)->points(), 2);
	}*/
}

void iATimeCurvesWidget::on_resetViewButton_clicked(bool checked)
{
	chartWidget->resetView();
}

void iATimeCurvesWidget::drawCustomLabels(QPainter* painter, QList<QPointF> points, const int offset)
{
	//if (points.isEmpty())
	//	return;

	//QFontMetrics fm(painter->font());
	//const int labelOffset = offset + 2;
	////painter->setFont(m_pointLabelsFont);        // Use QXYSeries::pointLabelsFont() to access m_pointLabelsFont
	//painter->setPen(
	//	QPen(QColor(QColorConstants::Black)));  // Use QXYSeries::pointLabelsColor() to access m_pointLabelsColor

	//for (QPointF point : points)
	//{
	//	// todo fix drawing double labels
	//	for (int i = 0; i < chart->series().size(); i++)
	//	//for (QAbstractSeries* series : chartWidget->series())
	//	{
	//		//QXYSeries* xyseries(qobject_cast<QXYSeries*>(series));
	//		QXYSeries* xyseries(qobject_cast<QXYSeries*>(chart->series().at(i)));
	//		int index = xyseries->points().indexOf(point);
	//		if (index != -1)
	//		{
	//			LOG(lvlDebug, QString("point found at index '%1").arg(index));
	//			//QStringList* list = dataList->at(chartWidget->series().indexOf(series)).fileNames;
	//			QStringList* list = dataList->at(i).fileNames;
	//			QString pointLabel = list->at(index);
	//			//QString pointLabel = seriesToFileNames.value(xyseries).fileNames->at(xyseries->points().indexOf(point));  // Set the desired label for the i-th point of the series

	//			QPointF position(point);
	//			//position.setX(position.x() - pointLabelWidth / 2);
	//			//position.setY(position.y() - labelOffset);
	//			//getLabel(xyseries, position, xyseries->points().indexOf(point));
	//			//int pointLabelWidth = fm.width(pointLabel);
	//			painter->drawText(position, pointLabel);

	//			QRectF boundingRect = fm.boundingRect(pointLabel);
	//			boundingRect.moveCenter(point);
	//			boundingRect.marginsAdded(QMarginsF(0, 0, 0, offset));
	//			painter->drawText(boundingRect, Qt::AlignCenter, pointLabel);
	//		}
	//	}
	//}
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
