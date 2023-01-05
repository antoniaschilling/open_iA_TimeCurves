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

#include "iAChartFilter.h"   // for iAResultFilter
#include "iAImageTreeNode.h" // for LabelImagePointer

#include <iAAttributes.h>
#include <iAITKIO.h> // TODO: replace?

#include <vtkSmartPointer.h>

#include <QWidget>

typedef iAITKIO::ImagePointer ClusterImageType;

class QLabel;
class QPushButton;
class QSplitter;
class QStandardItemModel;
class QTextEdit;

class iAChannelData;
class iAChartAttributeMapper;
class iAColorTheme;
class iADataSet;
class iAImageCoordinate;
class iAImageTreeNode;
class iAImagePreviewWidget;
class iALabelInfo;
class iATimedEvent;
class iATransferFunction;
class iAvtkImageData;

class vtkColorTransferFunction;
class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;

class QListView;

class iADetailView: public QWidget
{
	Q_OBJECT
public:
	iADetailView(iAImagePreviewWidget* prevWdgt,
		iAImagePreviewWidget* compareWdgt,
		ClusterImageType nullImage,
		std::vector<std::shared_ptr<iADataSet>> const & dataSets,
		std::vector<iATransferFunction*> const & transfer,
		iALabelInfo const & labelInfo,
		iAColorTheme const * colorTheme,
		int representativeType,
		QWidget* comparisonDetailsWidget);
	void SetNode(iAImageTreeNode const * node,
		QSharedPointer<iAAttributes> allAttributes,
		iAChartAttributeMapper const & mapper);
	void SetCompareNode(iAImageTreeNode const * node);
	int sliceNumber() const;
	void UpdateLikeHate(bool isLike, bool isHate);
	bool IsShowingCluster() const;
	void setSliceNumber(int sliceNr);
	void SetMagicLensOpacity(double opacity);
	void setMagicLensCount(int count);
	void UpdateMagicLensColors();
	void SetLabelInfo(iALabelInfo const & labelInfo, iAColorTheme const * colorTheme);
	void SetRepresentativeType(int representativeType);
	int GetRepresentativeType();
	QString GetLabelNames() const;
	iAResultFilter const & GetResultFilter() const;
	void SetRefImg(LabelImagePointer refImg);
	void SetCorrectnessUncertaintyOverlay(bool enabled);

signals:
	void Like();
	void Hate();
	void GoToCluster();
	void ViewUpdated();
	void SlicerHover(double x, double y, double z, int);
	void ResultFilterUpdate();

private slots:
	void dblClicked();
	void changeModality(int);
	void changeMagicLensOpacity(int);
	void SlicerClicked(double x, double y, double z);
	void SlicerMouseMove(double x, double y, double z, int c);
	void SlicerReleased(double x, double y, double z);
	void TriggerResultFilterUpdate();
	void ResetResultFilter();

private:
	void paintEvent(QPaintEvent * ) override;
	void setImage();
	void AddResultFilterPixel(double x, double y, double z);
	void AddMagicLensInput(vtkSmartPointer<vtkImageData> img, vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, QString const & name);
	void UpdateComparisonNumbers();
	int GetCurLabelRow() const;

	iAImageTreeNode const * m_node;
	iAImageTreeNode const * m_compareNode;
	iAImagePreviewWidget* m_previewWidget;
	iAImagePreviewWidget* m_compareWidget;
	QPushButton *m_pbLike, *m_pbHate, *m_pbGoto;
	QTextEdit* m_detailText;
	QListView* m_lvLegend;
	QWidget* m_cmpDetailsWidget;
	QLabel* m_cmpDetailsLabel;
	QStandardItemModel* m_labelItemModel;
	bool m_showingClusterRepresentative;
	std::vector<std::shared_ptr<iADataSet>> m_dataSets;
	std::vector<iATransferFunction*> m_transfer;
	ClusterImageType m_nullImage;
	int m_representativeType;
	LabelImagePointer m_refImg;

	int m_magicLensDataSetIdx;
	bool m_magicLensEnabled;
	int m_magicLensCount;
	iAColorTheme const * m_colorTheme;

	uint m_nextChannelID;

	vtkSmartPointer<iAvtkImageData> m_resultFilterImg;
	vtkSmartPointer<vtkLookupTable> m_resultFilterOverlayLUT;
	vtkSmartPointer<vtkPiecewiseFunction> m_resultFilterOverlayOTF;
	iAResultFilter m_resultFilter;
	QSharedPointer<iAChannelData> m_resultFilterChannel;
	int m_lastResultFilterX, m_lastResultFilterY, m_lastResultFilterZ;
	iATimedEvent* m_resultFilterTriggerThread;
	bool m_MouseButtonDown;
	int m_labelCount;
	double m_spacing[3];
	int m_dimensions[3];
	bool m_correctnessUncertaintyOverlayEnabled;
};
