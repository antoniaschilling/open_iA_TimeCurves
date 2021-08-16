#pragma once

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h"
#include "iACompHistogramTableData.h"

#include <map>
#include <vector>
#include "vtkActor.h"
#include "vtkIdTypeArray.h"

class iACompHistogramTable;
class vtkRenderer;
class iACompVisMain;
class vtkPropPicker;

namespace Pick
{
using PickedMap = std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>;
}

class iACompHistogramTableInteractorStyle : public vtkInteractorStyleTrackballCamera
{
   public:
	static iACompHistogramTableInteractorStyle* New();
	vtkTypeMacro(iACompHistogramTableInteractorStyle, vtkInteractorStyleTrackballCamera);

	virtual void OnLeftButtonDown() override;
	virtual void OnLeftButtonUp() override;

	virtual void OnMouseMove() override;

	virtual void OnMiddleButtonDown() override;
	virtual void OnRightButtonDown() override;
	virtual void OnMouseWheelForward() override;
	virtual void OnMouseWheelBackward() override;
	virtual void OnKeyPress() override;
	virtual void OnKeyRelease() override;

	virtual void Pan() override;

	//init iACompHistogramTable
	void setIACompHistogramTable(iACompHistogramTable* visualization);
	void setIACompVisMain(iACompVisMain* main);

   protected:
	iACompHistogramTableInteractorStyle();

   private:	
	
	std::map<int, std::vector<double>>* calculatePickedObjects(QList<bin::BinType*>* zoomedRowData);

	//reformats picked object such that the other charts can work with it
	csvDataType::ArrayType* formatPickedObjects(QList<std::vector<csvDataType::ArrayType*>*>* zoomedRowData);

	//general zooming in executed by the camera
	void generalZoomIn();
	//general zooming out executed by the camera
	void generalZoomOut();

	//linear zooming in over all bins of the histograms
	void linearZoomInHistogram();
	//linear zooming out over all bins of the histograms
	void linearZoomOutHistogram();

	//non linear zooming in - zooming in on currently selected bin(s)/row(s)
	void nonLinearZoomIn();
	//non linear zooming out - zooming out on currently selected bin(s)/row(s)
	void nonLinearZoomOut();

	void updateCharts();
	void updateOtherCharts(QList<std::vector<csvDataType::ArrayType*>*>* selectedObjectAttributes);
	void resetOtherCharts();

	void resetHistogramTable();

	//remove unnecessary highlights and bar char visualization 
	void reinitializeState();

	void manualTableRelocatingStart(vtkSmartPointer<vtkActor> movingActor);
	void manualTableRelocatingStop();

	//set the picklist for the propPicker to only pick original row actors
	void setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors);

	void storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id);

	//reset the bar chart visualization showing the number of objects for each dataset
	bool resetBarChartAmountObjects();

	iACompHistogramTable* m_visualization;

	//stores for each actor a vector for each picked cell according to their vtkIdType
	Pick::PickedMap* m_picked;

	//controls whether the linear or non-linear zoom on the histogram is activated
	//true --> non-linear zoom is activated, otherwise linear zoom
	bool m_controlBinsInZoomedRows;
	//controls if the number of bins is modified or if the point representation should be drawn
	//true --> point representation is drawn
	bool m_pointRepresentationOn;

	//if ture --> zooming is active, otherwise ordering according to selected bins is active
	bool m_zoomOn;

	//is always 1 -> only needed for changing the zoom for the camera
	double m_zoomLevel;

	QList<bin::BinType*>* m_zoomedRowData;

	iACompVisMain* m_main;

	vtkSmartPointer<vtkPropPicker> m_actorPicker;
	vtkSmartPointer<vtkActor> m_currentlyPickedActor;

	bool m_panActive;
};