#include "iATimeCurvesWidget.h"

#include <iALog.h>
#include <iAPlotTypes.h>
#include <iAXYPlotData.h>
#include <iAXYPlotUnorderedData.h>

#include "iAMapperImpl.h"

//Qt
#include <QPushButton.h>
#include <QRect.h>

//Eigen
//#include <Eigen/Geometry>

iATimeCurvesWidget::iATimeCurvesWidget(tapkee::DenseMatrix embedding, QStringList* fileNames, QWidget* parent) :
	embedding(embedding), fileNames(fileNames), QWidget(parent)
{
	ui.setupUi(this);

	//chartWidget config
	chartWidget = new iAChartWidget(this, "xLabel", "Ylabel");
	chartWidget->setMinimumSize(500, 500);
	//or setHeightForWidth dependent
	//constexpr QMargins(int left, int top, int right, int bottom) noexcept;
	chartWidget->setContentsMargins(5,5,5,5);
	chartWidget->setSizePolicy(QSizePolicy::QSizePolicy());
	chartWidget->setYMappingMode(iAChartWidget::AxisMappingType::Linear);
	chartWidget->showLegend(true);
	//todo selection

	ui.verticalLayout->addWidget(chartWidget);
	dataList = new QList<TimeCurve>();
	TimeCurve firstData{embedding, fileNames, QString()};
	addSeries(firstData);
	connect(ui.saveButton, &QPushButton::clicked, this, &iATimeCurvesWidget::saveJson);
	connect(chartWidget, &iAChartWidget::clicked, this, &iATimeCurvesWidget::pointClicked);
}

void iATimeCurvesWidget::addSeries(TimeCurve data)
{
	//todo: map data?
	rotateData(&data);
	populateTable(data);
	//todo plot collection
	if (data.name.isEmpty())
	{
		data.name = "TimeCurve";
	}
	QSharedPointer<iAXYPlotUnorderedData> plotData =
		iAXYPlotUnorderedData::create(data.name, iAValueType::Continuous, data.embedding.size(), *data.fileNames);
	for (int i = 0; i < data.embedding.cols(); i++)
	{
		plotData.data()->addValue(data.embedding(0, i), data.embedding(1, i));
	}

	QSharedPointer<iASplinePlot> splinePlot =
		QSharedPointer<iASplinePlot>::create(plotData, QColor(QColorConstants::Black));
	chartWidget->addPlot(splinePlot);

	//set custom bounds to display full spline
	//todo multiple plots
	QSharedPointer<iAMapper> const xMapper = QSharedPointer<iALinearMapper>::create(
		chartWidget->xBounds()[0], chartWidget->xBounds()[1], chartWidget->xBounds()[0], chartWidget->xBounds()[1]);
	QSharedPointer<iAMapper> const yMapper = QSharedPointer<iALinearMapper>::create(
		chartWidget->yBounds()[0], chartWidget->yBounds()[1], chartWidget->yBounds()[0], chartWidget->yBounds()[1]);
	QRectF boundingBox = splinePlot.data()->getBoundingBox(*xMapper.data(), *yMapper.data());
	chartWidget->setXBounds(boundingBox.left(), boundingBox.right());
	chartWidget->setYBounds(boundingBox.top(), boundingBox.bottom());
	LOG(lvlDebug,
		QString("Added plot -> custom chart xbounds = ['%1','%2'] and ybounds = ['%3','%4']")
			.arg(chartWidget->xBounds()[0])
			.arg(chartWidget->xBounds()[1])
			.arg(chartWidget->yBounds()[0])
			.arg(chartWidget->yBounds()[1]));

	chartWidget->update();
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
	return;
}

void iATimeCurvesWidget::on_resetViewButton_clicked(bool checked)
{
	chartWidget->resetView();
}

void iATimeCurvesWidget::saveJson()
{
	return;
}

void iATimeCurvesWidget::rotateData(TimeCurve* timecurve)
{
	double angle;
	LOG(lvlDebug, QString("Embedding before rotation:"));
	printMatrixToLog(timecurve->embedding);
	auto cols = timecurve->embedding.cols();
	angle = std::atan2(timecurve->embedding.col(cols - 1).y() - timecurve->embedding.col(0).y(),
		timecurve->embedding.col(cols - 1).x() - timecurve->embedding.col(0).x());
	Eigen::Matrix data(timecurve->embedding);
	Eigen::Rotation2D<double> rot2(-angle);
	for (int i = 0; i < cols; i++)
	{
		timecurve->embedding.col(i) =  rot2.toRotationMatrix() * timecurve->embedding.col(i); 
	}
	LOG(lvlDebug, QString("Embedding rotated with angle: '%1'").arg(angle));
	printMatrixToLog(timecurve->embedding);
}

void iATimeCurvesWidget::printMatrixToLog(Eigen::MatrixXd matrix)
{
	Eigen::IOFormat CleanFmt(4, Eigen::DontAlignCols, ",", "\n", "", "");
	std::stringstream buffer;
	buffer << matrix.transpose().format(CleanFmt);
	std::string str = buffer.str();
	LOG(lvlInfo, QString::fromStdString(str));
}

void iATimeCurvesWidget::populateTable(TimeCurve data)
{
	QTableWidget* tableWidget = ui.tableWidget;
	tableWidget->setRowCount(data.embedding.rows());
	tableWidget->setColumnCount(data.embedding.cols());
	for (int i = 0; i < data.embedding.rows(); i++)
	{
		for (int j = 0; j < data.embedding.cols(); j++)
		{
			double value = data.embedding(i, j);
			tableWidget->setItem(i, j, new QTableWidgetItem(QString::number(value)));
		}
	}
	LOG(lvlDebug, QString("Created table, first item is: '%1'").arg(tableWidget->itemAt(1, 1)->text()));
}
