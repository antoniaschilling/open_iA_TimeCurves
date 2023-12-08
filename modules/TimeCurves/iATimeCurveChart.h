#pragma once
#include <iAChartWidget.h>
class iATimeCurveChart : public iAChartWidget
{
public:
	iATimeCurveChart(QWidget* parent, QString const& xLabel, QString const& yLabel) :
		iAChartWidget(parent, xLabel, yLabel){};

	void wheelEvent(QWheelEvent* event) override;
	//zooms equally along x and y axis to the center of the chart
	void zoomAlongXY(double value, int x, int y, bool deltaMode);
	//overriden method of upper class, redirects to zoomAlongXY
	void zoomAlongY(double value, bool deltaMode);
	//overriden method of upper class, redirects to zoomAlongXY
	void zoomAlongX(double value, int x, bool deltaMode);
	//! @{ Convert mouse Y coordinates (in chart already, i.e. without left/bottom margin) to chart y coordinates and vice versa
	int data2MouseY(double dataY);
	double mouse2DataY(int mouseY);
	//! @}
	//! height in pixels that the chart would have if it were fully shown (considering current zoom level)k
	double fullChartHeight() const;

public slots:
	void resetView();

protected:
	virtual void drawPlots(QPainter& painter) override;

private:
	virtual void drawYAxis(QPainter& painter) override;

	double visibleXStart() const;
	double visibleXEnd() const;
};
