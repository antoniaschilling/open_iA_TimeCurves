#pragma once
#include <iAChartWidget.h>
class iATimeCurveChart :
    public iAChartWidget
{
public:
	iATimeCurveChart(QWidget* parent, QString const& xLabel, QString const& yLabel) :
		iAChartWidget(parent, xLabel, yLabel)
		//m_translationY(0.0),
		//m_translationStartY(0.0)
	{};

	void wheelEvent(QWheelEvent* event) override;
	void zoomAlongXY(double value, int x, int y, bool deltaMode);
	void zoomAlongY(double value, bool deltaMode);
	void zoomAlongX(double value, int x, bool deltaMode);
	//labels
	//xAxisTickMarkLabel
	//drawyaxis

public slots:
	void resetView();

	protected:
	virtual void drawPlots(QPainter& painter) override;

private:
	virtual void drawYAxis(QPainter& painter) override;

	double visibleXStart() const;
	double visibleXEnd() const;
};

