#include "iATimeCurvesWidget.h"

#include <iALog.h>
#include <iAPlotTypes.h>
#include <iAXYPlotData.h>
#include <iAXYPlotUnorderedData.h>

#include "iAMapperImpl.h"

//Qt
#include <QPushButton.h>
#include <QRect.h>
#include <qmimedata.h>
#include <qdrag.h>

//Eigen
//#include <Eigen/Geometry>

iATimeCurvesWidget::iATimeCurvesWidget(
	TimeCurve timecurvedata, iATimeCurves* main, iAMainWindow* mainWindow, QWidget* parent) :
	m_main(main), m_mainWindow(mainWindow), QWidget(parent)
{
	ui.setupUi(this);

	//chartWidget config
	//todo meaningful labels
	chartWidget = new iAChartWidget(this, "Dimension 1", "Dimension 2");
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
	// todo refactor
	TimeCurve firstData{timecurvedata};
	addSeries(firstData);
	connect(ui.saveButton, &QPushButton::clicked, this, &iATimeCurvesWidget::saveJson);
	connect(chartWidget, &iAChartWidget::clicked, this, &iATimeCurvesWidget::pointClicked);
	//drag and drop
	setAcceptDrops(true);
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

	LOG(lvlDebug,
		QString("Added plot -> chart xbounds = ['%1','%2'] and ybounds = ['%3','%4']")
			.arg(chartWidget->xBounds()[0])
			.arg(chartWidget->xBounds()[1])
			.arg(chartWidget->yBounds()[0])
			.arg(chartWidget->yBounds()[1]));
	//mapper 1 -> 1 todo: make easier to read
	QSharedPointer<iAMapper> const xMapper = QSharedPointer<iALinearMapper>::create(
		0, 1, 0, 1);
	//QSharedPointer<iAMapper> const xMapper = QSharedPointer<iALinearMapper>::create(
		//chartWidget->xBounds()[0], chartWidget->xBounds()[1], chartWidget->xBounds()[0], chartWidget->xBounds()[1]);
	QSharedPointer<iAMapper> const yMapper = QSharedPointer<iALinearMapper>::create(
		chartWidget->yBounds()[0], chartWidget->yBounds()[1], chartWidget->yBounds()[0], chartWidget->yBounds()[1]);
	QRectF boundingBox = splinePlot.data()->getBoundingBox(*xMapper.data(), *yMapper.data());
	LOG(lvlDebug,
		QString("Added plot -> bounding box = ['%1','%2'] and ybounds = ['%3','%4']")
			.arg(boundingBox.left())
			.arg(boundingBox.right())
			.arg(boundingBox.top())
			.arg(boundingBox.bottom()));
	chartWidget->setXBounds(boundingBox.left(), 0.002);
	chartWidget->setYBounds(boundingBox.top(), boundingBox.bottom());
	LOG(lvlDebug,
		QString("Added plot -> custom chart xbounds = ['%1','%2'] and ybounds = ['%3','%4']")
			.arg(chartWidget->xBounds()[0])
			.arg(chartWidget->xBounds()[1])
			.arg(chartWidget->yBounds()[0])
			.arg(chartWidget->yBounds()[1]));
	chartWidget->addPlot(splinePlot);

	//set custom bounds to display full spline
	//todo multiple plots
	//chartWidget->xMapper();
	
	dataList->append(data);
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

void iATimeCurvesWidget::dragEnterEvent(QDragEnterEvent* event)
{
	iATimeCurvesWidget* source = qobject_cast<iATimeCurvesWidget*>(event->source());
	if (source && source != this)
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
}

void iATimeCurvesWidget::dragMoveEvent(QDragMoveEvent* event)
{
	iATimeCurvesWidget* source = qobject_cast<iATimeCurvesWidget*>(event->source());
	if (source && source != this)
	{
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
}

void iATimeCurvesWidget::dropEvent(QDropEvent* event)
{
	iATimeCurvesWidget* source = qobject_cast<iATimeCurvesWidget*>(event->source());
	if (source && source != this)
	{
		LOG(lvlDebug, QString("curve has been dropped with text: '%1'").arg(event->mimeData()->text()));
		TimeCurve timeCurve = timeCurveFromString(event->mimeData()->text());
		addSeries(timeCurve);
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
}

void iATimeCurvesWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		startPos = event->pos();
	}
	QWidget::mousePressEvent(event);
}

void iATimeCurvesWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() == Qt::LeftButton)
	{
		int distance = (event->pos() - startPos).manhattanLength();
		if (distance >= QApplication::startDragDistance())
		{
			startDrag();
		}
	}
}

void iATimeCurvesWidget::startDrag()
{
	iATimeCurvesWidget* widget = this;
	if (widget)
	{
		QMimeData* mimeData = new QMimeData;
		//parse embedding as String
		mimeData->setText(widget->dataList->first().toString());
		QDrag* drag = new QDrag(this);
		drag->setMimeData(mimeData);
		//todo: icon drag->setPixmap
		if (drag->exec(Qt::MoveAction) == Qt::MoveAction)
		{
			LOG(lvlDebug, "Widget has been moved.");
			//todo close window
		}
	}
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

TimeCurve iATimeCurvesWidget::timeCurveFromString(QString str)
{
	QStringList list = str.split(";");
	int size = (list.size() - 1) / 2;
	//todo parse
	QString name(list.at(0));
	QStringList* fileNames = new QStringList();
	for (int i = 0; i < size; i++)
	{
		fileNames->append(list.at(i + 1));
	}
	//Eigen::MatrixXd* dataset = new Eigen::MatrixXd(8,2);
	Eigen::MatrixXd matrix(2,size);
	for (int i = 0; i < size; i++)
	{
		QString pair = list.at(size + 1 + i);
		QStringList values = pair.split(',');
		matrix(0, i) = values.at(0).toDouble();
		matrix(1, i) = values.at(1).toDouble();
	}

	//Eigen::DenseMatrix embedding = new Eigen::DenseMatrix;
	TimeCurve timeCurve{matrix, fileNames, name};
	return timeCurve;
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
