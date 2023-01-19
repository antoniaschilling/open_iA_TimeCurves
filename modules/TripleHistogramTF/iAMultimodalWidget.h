/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "tf_3mod/iABCoord.h"

#include "iASimpleSlicerWidget.h"

#include <iAChartWithFunctionsWidget.h>

#include <vtkSmartPointer.h>

#include <QComboBox>
#include <QWidget>
#include <QVector>
#include <QSharedPointer>

#include <memory>

class iADataSetRenderer;
class iAImageData;
class iAMdiChild;
class iATransferFunctionOwner;

class vtkCamera;
class vtkColorTransferFunction;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkSmartVolumeMapper;
class vtkRenderer;
class vtkVolume;

class QLabel;
class QStackedLayout;
class QCheckBox;

enum NumOfMod
{
	UNDEFINED = -1,
	TWO = 2,
	THREE = 3
};

class iAMultimodalWidget : public QWidget
{
	Q_OBJECT

// Private methods used by public/protected
private:
	double bCoord_to_t(iABCoord bCoord)
	{
		return bCoord[1];
	}
	iABCoord t_to_bCoord(double t)
	{
		return iABCoord(1-t, t);
	}
	//void setWeights(iABCoord bCoord, double t);

public:
	iAMultimodalWidget(iAMdiChild* mdiChild, NumOfMod num);

	iAChartWithFunctionsWidget* w_histogram(int i)
	{
		return m_histograms[i].data();
	}

	iASimpleSlicerWidget* w_slicer(int i)
	{
		return m_slicerWidgets[i].data();
	}

	QCheckBox* w_checkBox_weightByOpacity()
	{
		return m_checkBox_weightByOpacity;
	}

	QCheckBox* w_checkBox_syncedCamera()
	{
		return m_checkBox_syncedCamera;
	}

	QLabel* w_slicerModeLabel()
	{
		return m_slicerModeLabel;
	}

	QLabel* w_sliceNumberLabel()
	{
		return m_sliceNumberLabel;
	}

	//void setWeights(iABCoord bCoord)
	//{
	//	setWeights(bCoord, bCoord_to_t(bCoord));
	//}
	//void setWeights(double t)
	//{
	//	setWeights(t_to_bCoord(t), t);
	//}
	iABCoord getWeights();
	double getWeight(int i);

	void setSlicerMode(iASlicerMode slicerMode);
	iASlicerMode slicerMode() const;
	void setSliceNumber(int sliceNumber);
	int sliceNumber() const;

	iATransferFunction* dataSetTransfer(int index);
	iADataSetRenderer* dataSetRenderer(int index);
	vtkSmartPointer<vtkImageData> dataSetImage(int index);
	QString dataSetName(int index);

	int getDataSetCount();
	bool containsDataSet(size_t dataSetIdx);
	bool isReady();

	void updateTransferFunction(int index);

protected:
	void setWeightsProtected(iABCoord bCoord, double t);

	void resetSlicer(int i);

	void setWeightsProtected(double t)
	{
		setWeightsProtected(t_to_bCoord(t), t);
	}

	void setWeightsProtected(iABCoord bCoord)
	{
		setWeightsProtected(bCoord, bCoord_to_t(bCoord));
	}

	iAMdiChild *m_mdiChild;
	QLayout *m_innerLayout;

private:
	// User interface {
	QVector<QSharedPointer<iAChartWithFunctionsWidget>> m_histograms;
	QVector<QSharedPointer<iASimpleSlicerWidget>> m_slicerWidgets;
	QVector<uint> m_channelID;
	QStackedLayout *m_stackedLayout;
	QCheckBox *m_checkBox_weightByOpacity;
	QLabel *m_disabledLabel;
	QString m_disabledReason;
	QCheckBox *m_checkBox_syncedCamera;
	// }
	virtual void dataSetChanged(size_t dataSetIdx) =0;
	void initGUI();

	QTimer *m_timer_updateVisualizations;
	int m_timerWait_updateVisualizations;
	void updateVisualizationsNow();
	void updateVisualizationsLater();

	//! Called when the original transfer function changes.
	//! RESETS THE COPY (admit numerical imprecision when setting the copy values)
	//! => effective / weight = copy
	void updateCopyTransferFunction(int index);
	//! Called when the copy transfer function changes
	//! ADD NODES TO THE EFFECTIVE ONLY (clear and repopulate with adjusted effective values)
	//! => copy * weight ~= effective
	void updateOriginalTransferFunction(int index);

	void originalHistogramChanged(int index);

	//! Resets the values of all nodes in the effective transfer function using the values present in the
	//! copy of the transfer function, using m_weightCur for the adjustment
	//! CHANGES THE NODES OF THE EFFECTIVE ONLY (based on the copy)
	void applyWeights();

	//! @{ Synced slicer camera helpers
	void setMainSlicerCamera();
	void resetSlicerCamera();
	void connectMainSlicer();
	void disconnectMainSlicer();
	void connectAcrossSlicers();
	void disconnectAcrossSlicers();
	//! @}

	iABCoord m_weights;

	void updateLabels();
	iASlicerMode m_slicerMode;
	QLabel *m_slicerModeLabel;
	QLabel *m_sliceNumberLabel;

	// Slicers
	//vtkSmartPointer<vtkImageData> m_slicerInputs[3][3];
	vtkSmartPointer<vtkImageData> m_slicerImages[3];

	NumOfMod m_numOfDS = UNDEFINED;
	QVector<size_t> m_dataSetsActive;

	vtkSmartPointer<vtkSmartVolumeMapper> m_combinedVolMapper;
	vtkSmartPointer<vtkRenderer> m_combinedVolRenderer;
	vtkSmartPointer<vtkVolume> m_combinedVol;
	bool m_mainSlicersInitialized;
	//bool m_weightByOpacity;
	double m_minimumWeight;

	// Background stuff
	//void alertWeightIsZero(QString const & name);
	QVector<std::shared_ptr<iATransferFunctionOwner>> m_copyTFs;

signals:
	void weightsChanged3(iABCoord weights);
	void weightsChanged2(double t);

	void slicerModeChangedExternally(iASlicerMode slicerMode);
	void sliceNumberChangedExternally(int sliceNumber);

	void dataSetsLoaded_beforeUpdate();

private slots:


	void checkBoxWeightByOpacityChanged();
	void checkBoxSyncedCameraChanged();

	void onMainSliceNumberChanged(int mode, int sliceNumber);

	void dataSetAdded(size_t dataSetIdx);
	void dataSetRemoved(size_t dataSetIdx);
	void dataSetChangedSlot(size_t dataSetIdx);
	void applyVolumeSettings();
	void applySlicerSettings();

	// Timers
	void onUpdateVisualizationsTimeout();
};