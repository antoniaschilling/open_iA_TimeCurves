/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iANModalController.h"

#include "iANModalBackgroundRemover.h" // for BACKGROUND and FOREGROUND values

#include "iALabellingAttachment.h"
#include "dlg_labels.h"

#include "iAModality.h"
#include "iAModalityList.h"
#include "iAModalityTransfer.h"
#include "iARenderer.h"
#include "iAVolumeRenderer.h"
#include "iASlicer.h"
#include "iAChannelSlicerData.h"
#include "iAChannelData.h"
#include "mdichild.h"
#include <iAToolsVTK.h>
//#include "iAChartWithFunctionsWidget.h" // Why doesn't it work?
#include "dlg_modalities.h"
//#include "iAToolsVTK.h"
#include "iAPerformanceHelper.h"
#include "iAConsole.h"

#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageAppendComponents.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkImageReslice.h>
#include <vtkSmartPointer.h>
#include <vtkImageActor.h>
#include <vtkGPUVolumeRayCastMapper.h>
//#include <vtkImageMapToColors.h>
//#include <vtkGPUVolumeRayCastMapper.h>
//#include <vtkImageMask.h>

#include <array>

vtkStandardNewMacro(iANModalSmartVolumeMapper);

static constexpr int NUM_SLICERS = iASlicerMode::SlicerCount;

iANModalController::iANModalController(MdiChild *mdiChild) :
	m_mdiChild(mdiChild)
{
	QObject* obj = m_mdiChild->findChild<QObject*>("labels");
	if (obj)
	{
		m_dlg_labels = static_cast<dlg_labels*>(obj);
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::XY));
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::XZ));
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::YZ));
	}
	else
	{
		m_dlg_labels = new dlg_labels(mdiChild, false);
		mdiChild->splitDockWidget(mdiChild->logDockWidget(), m_dlg_labels, Qt::Vertical);
	}
	m_dlg_labels->setSeedsTracking(true);

	connect(mdiChild, SIGNAL(histogramAvailable()), this, SLOT(onHistogramAvailable()));
}

void iANModalController::initialize() {
	assert(!m_initialized);
	_initialize();
	emit(allSlicersInitialized());
}

void iANModalController::reinitialize() {
	assert(m_initialized);
	_initialize();
	emit(allSlicersReinitialized());
}

void iANModalController::_initialize() {
	assert(countModalities() <= 4); // VTK limit. TODO: don't hard-code

	for (auto slicer : m_slicers) {
		m_dlg_labels->removeSlicer(slicer);
	}

	m_slicers.clear();
	m_mapOverlayImageId2modality.clear();
	for (int i = 0; i < m_modalities.size(); i++) {
		auto modality = m_modalities[i];

		auto slicer = _initializeSlicer(modality);
		int id = m_dlg_labels->addSlicer(
			slicer,
			modality->name(),
			modality->image()->GetExtent(),
			modality->image()->GetSpacing(),
			m_slicerChannel_label);
		m_slicers.append(slicer);
		m_mapOverlayImageId2modality.insert(id, modality);
	}

	if (countModalities() > 0) {
		_initializeMainSlicers();
		_initializeCombinedVol();
	}

	m_initialized = true;
}

inline iASlicer* iANModalController::_initializeSlicer(QSharedPointer<iAModality> modality) {
	auto slicerMode = iASlicerMode::XY;
	int sliceNumber = m_mdiChild->slicer(slicerMode)->sliceNumber();
	// Hide everything except the slice itself
	auto slicer = new iASlicer(nullptr, slicerMode, /*bool decorations = */false);
	slicer->setup(m_mdiChild->slicerSettings().SingleSlicer);
	slicer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	auto image = modality->image();

	vtkColorTransferFunction* colorTf = vtkColorTransferFunction::New();
	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	colorTf->AddRGBPoint(min, 0.0, 0.0, 0.0);
	//if (m_maskApplied) colorTf->AddRGBPoint(min+1, 0.0, 0.0, 0.0);
	colorTf->AddRGBPoint(max, 1.0, 1.0, 1.0);
	slicer->addChannel(m_slicerChannel_main, iAChannelData(modality->name(), image, colorTf), true);

	double* origin = image->GetOrigin();
	int* extent = image->GetExtent();
	double* spacing = image->GetSpacing();

	double xc = origin[0] + 0.5*(extent[0] + extent[1])*spacing[0];
	double yc = origin[1] + 0.5*(extent[2] + extent[3])*spacing[1];
	//double xd = (extent[1] - extent[0] + 1)*spacing[0];
	double yd = (extent[3] - extent[2] + 1)*spacing[1];

	vtkCamera* camera = slicer->camera();
	double d = camera->GetDistance();
	camera->SetParallelScale(0.5 * yd);
	camera->SetFocalPoint(xc, yc, 0.0);
	camera->SetPosition(xc, yc, +d);

	slicer->setSliceNumber(sliceNumber);

	return slicer;
}

inline void iANModalController::_initializeMainSlicers() {
	iASlicer* slicerArray[] = {
		m_mdiChild->slicer(iASlicerMode::YZ),
		m_mdiChild->slicer(iASlicerMode::XY),
		m_mdiChild->slicer(iASlicerMode::XZ)
	};

	for (uint channelId : m_channelIds) {
		m_mdiChild->removeChannel(channelId);
	}
	m_channelIds.clear();

	for (int modalityIndex = 0; modalityIndex < countModalities(); modalityIndex++) {
		uint channelId = m_mdiChild->createChannel();
		m_channelIds.append(channelId);

		auto modality = m_modalities[modalityIndex];

		auto chData = m_mdiChild->channelData(channelId);
		vtkImageData* imageData = modality->image();
		vtkColorTransferFunction* ctf = modality->transfer()->colorTF();
		vtkPiecewiseFunction* otf = modality->transfer()->opacityTF();
		chData->setData(imageData, ctf, otf);

		m_mdiChild->initChannelRenderer(channelId, false, true);
	}

	// Allocate a 2D image for each main slicer
	// This will be useful in the method _updateMainSlicers
	for (int mainSlicerIndex = 0; mainSlicerIndex < iASlicerMode::SlicerCount; mainSlicerIndex++) {

		auto reslicer = slicerArray[mainSlicerIndex]->channel(m_channelIds[0])->reslicer();

		int const* dims = reslicer->GetOutput()->GetDimensions();
		// should be double const once VTK supports it:
		double* spc = reslicer->GetOutput()->GetSpacing();

		//data->GetImageActor()->SetOpacity(0.0);
		//data->SetManualBackground(1.0, 1.0, 1.0);
		//data->SetManualBackground(0.0, 0.0, 0.0);

		auto imgOut = vtkSmartPointer<vtkImageData>::New();
		imgOut->SetDimensions(dims);
		imgOut->SetSpacing(spc);
		imgOut->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
		m_sliceImages2D[mainSlicerIndex] = imgOut;
	}
}

void iANModalController::onHistogramAvailable() {
	if (m_initialized && countModalities() > 0) {
		//_initializeCombinedVol();
	}
}

inline void iANModalController::_initializeCombinedVol() {
	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(m_modalities[0]->image());
	for (int i = 1; i < countModalities(); i++) {
		appendFilter->AddInputData(m_modalities[i]->image());
	}
	appendFilter->Update();

	m_combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();

	for (int i = 0; i < countModalities(); i++) {
		auto transfer = m_modalities[i]->transfer();
		combinedVolProp->SetColor(i, transfer->colorTF());
		combinedVolProp->SetScalarOpacity(i, transfer->opacityTF());
	}

	m_combinedVol->SetProperty(combinedVolProp);

	m_combinedVolMapper = vtkSmartPointer<iANModalSmartVolumeMapper>::New();
	m_combinedVolMapper->SetBlendModeToComposite();
	m_combinedVolMapper->SetInputData(appendFilter->GetOutput());
	m_combinedVolMapper->Update();
	applyVolumeSettings();
	m_combinedVol->SetMapper(m_combinedVolMapper);
	m_combinedVol->Update();

	m_combinedVolRenderer = vtkSmartPointer<vtkRenderer>::New();
	m_combinedVolRenderer->SetActiveCamera(m_mdiChild->renderer()->camera());
	m_combinedVolRenderer->GetActiveCamera()->ParallelProjectionOn();
	m_combinedVolRenderer->SetLayer(1);
	m_combinedVolRenderer->AddVolume(m_combinedVol);
	//m_combinedVolRenderer->ResetCamera();

	for (int i = 0; i < countModalities(); ++i) {
		QSharedPointer<iAVolumeRenderer> renderer = m_modalities[i]->renderer();
		if (renderer && renderer->isRendered())
			renderer->remove();
	}
	m_mdiChild->renderer()->addRenderer(m_combinedVolRenderer);

	m_mdiChild->modalitiesDockWidget()->setAllChecked(Qt::Unchecked);
}

inline void iANModalController::applyVolumeSettings() {
	auto vs = m_mdiChild->volumeSettings();
	auto volProp = m_combinedVol->GetProperty();
	volProp->SetAmbient(vs.AmbientLighting);
	volProp->SetDiffuse(vs.DiffuseLighting);
	volProp->SetSpecular(vs.SpecularLighting);
	volProp->SetSpecularPower(vs.SpecularPower);
	volProp->SetInterpolationType(vs.LinearInterpolation);
	volProp->SetShade(vs.Shading);
	if (vs.ScalarOpacityUnitDistance > 0)
		volProp->SetScalarOpacityUnitDistance(vs.ScalarOpacityUnitDistance);
	if (m_mdiChild->renderSettings().ShowSlicers)
	{
		m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane1());
		m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane2());
		m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane3());
	}
	else
	{
		m_combinedVolMapper->RemoveAllClippingPlanes();
	}
#ifdef VTK_OPENGL2_BACKEND
	m_combinedVolMapper->SetSampleDistance(vs.SampleDistance);
	m_combinedVolMapper->InteractiveAdjustSampleDistancesOff();
#endif
}

int iANModalController::countModalities() {
	// Cannot be larger than 4 because of VTK limit
	int numModalities = m_modalities.size();
	assert(numModalities <= 4); // Bad: '4' is hard-coded. TODO: improve
	return numModalities;
}

bool iANModalController::_checkModalities(QList<QSharedPointer<iAModality>> modalities) {
	if (modalities.size() < 1 || modalities.size() > 4) { // Bad: '4' is hard-coded. TODO: improve
		return false;
	}

	for (int i = 1; i < modalities.size(); i++) {
		if (!_matchModalities(modalities[0], modalities[i])) {
			return false;
		}
	}

	return true;
}

bool iANModalController::_matchModalities(QSharedPointer<iAModality> m1, QSharedPointer<iAModality> m2) {
	/*
	auto image1 = m1->image();
	const int *extent1 = image1->GetExtent();
	const double *spacing1 = image1->GetSpacing();
	const double *origin1 = image1->GetOrigin();

	auto image2 = m2->image();
	const int *extent2 = image2->GetExtent();
	const double *spacing2 = image2->GetSpacing();
	const double *origin2 = image2->GetOrigin();
	*/

	return true;
}

void iANModalController::setModalities(QList<QSharedPointer<iAModality>> modalities) {
	if (!_checkModalities(modalities)) {
		return;
	}
	resetTf(modalities);
	m_modalities = modalities;
}

void iANModalController::setMask(vtkSmartPointer<vtkImageData> mask) {

	m_mask = vtkSmartPointer<vtkImageData>::New();
	m_mask->DeepCopy(mask);
	mask = m_mask;

	unsigned char *ptr = static_cast<unsigned char *>(mask->GetScalarPointer());
#pragma omp parallel for
	FOR_VTKIMG_PIXELS(mask, x, y, z) {
		int ijk[3] = { x, y, z };
		int id = mask->ComputePointId(ijk);
		ptr[id] *= 255;
	}

	auto gpuMapper = m_combinedVolMapper->getGPUMapper();
	gpuMapper->SetMaskTypeToBinary();
	gpuMapper->SetMaskInput(mask);
}

void iANModalController::resetTf(QSharedPointer<iAModality> modality) {
	double range[2];
	modality->image()->GetScalarRange(range);
	double min = range[0];
	double max = range[1];

	auto tf = modality->transfer();

	tf->colorTF()->RemoveAllPoints();
	tf->colorTF()->AddRGBPoint(min, 0.0, 0.0, 0.0);
	//if (m_maskApplied) tf->colorTF()->AddRGBPoint(min+1, 0.0, 0.0, 0.0);
	tf->colorTF()->AddRGBPoint(max, 0.0, 0.0, 0.0);

	tf->opacityTF()->RemoveAllPoints();
	tf->opacityTF()->AddPoint(min, 0.0);
	//if (m_maskApplied) tf->opacityTF()->AddPoint(min + 1, 0.0, 0.0, 0.0);
	tf->opacityTF()->AddPoint(max, 0.0);
}

void iANModalController::resetTf(QList<QSharedPointer<iAModality>> modalities) {
	for (auto modality : modalities) {
		resetTf(modality);
	}
}

void iANModalController::updateLabel(iANModalLabel label) {
	auto list = QList<iANModalLabel>();
	list.append(label);
	updateLabels(list);
}

void iANModalController::updateLabels(QList<iANModalLabel> labelsList) {
	auto labels = QVector<iANModalLabel>(m_maxLabelId + 1);
	auto used = QVector<bool>(m_maxLabelId + 1);
	used.fill(false);
	for (auto label : labelsList) {
		int id = label.id;
		assert(id >= 0);
		if (id <= m_maxLabelId) {
			labels[id] = label;
			used[id] = true;
		}
	}
	for (auto seed : m_seeds) {
		if (used[seed.labelId]) {
			auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
			auto colorTf = modality->transfer()->colorTF();
			auto opacityTf = modality->transfer()->opacityTF();

			auto label = labels[seed.labelId];
			auto o = label.opacity;
			auto c = o == 0 ? QColor(0,0,0) : label.color;

			//DEBUG_LOG("updating at " + QString::number(seed.scalar));

			colorTf->RemovePoint(seed.scalar);
			colorTf->AddRGBPoint(seed.scalar, c.redF(), c.greenF(), c.blueF());

			opacityTf->RemovePoint(seed.scalar);
			opacityTf->AddPoint(seed.scalar, o);
		}
	}
	//m_mdiChild->histogram()->updateTrf(); // TODO include iAChartWithFunctionsWidget.h
}

void iANModalController::addSeeds(QList<iANModalSeed> seeds, iANModalLabel label) {
	for (auto seed : seeds) {
		auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
		auto colorTf = modality->transfer()->colorTF();
		auto opacityTf = modality->transfer()->opacityTF();
		
		double scalar;
		//auto ite = m_seeds.constFind(seed);
		auto ite = m_seeds.find(seed);
		if (ite != m_seeds.end()) {
			colorTf->RemovePoint(seed.scalar);
			opacityTf->RemovePoint(seed.scalar);
			scalar = ite->scalar;
		} else {
			scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		}

		seed.labelId = label.id;
		seed.scalar = scalar;

		//DEBUG_LOG(QString("adding ") + QString::number(seeds.size()) + QString(" seed(s). one at ") + QString::number(seed.scalar));

		auto o = label.opacity;
		auto c = o == 0 ? QColor(0,0,0) : label.color;
		colorTf->AddRGBPoint(seed.scalar, c.redF(), c.greenF(), c.blueF());
		opacityTf->AddPoint(seed.scalar, o);

		m_seeds.insert(seed);
	}
	if (label.id > m_maxLabelId) {
		m_maxLabelId = label.id;
	}
}

void iANModalController::removeSeeds(QList<iANModalSeed> seeds) {
	for (auto seed : seeds) {
		if (m_seeds.erase(seed) == 1) { // If one element (seed) was removed
			auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
			auto colorTf = modality->transfer()->colorTF();
			auto opacityTf = modality->transfer()->opacityTF();
			//double scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
			colorTf->RemovePoint(seed.scalar);
			opacityTf->RemovePoint(seed.scalar);
		}
	}
}

void iANModalController::removeSeeds(int labelId) {
	auto ite = m_seeds.cbegin();
	while (ite != m_seeds.cend()) {
		auto const seed = *ite;
		if (seed.labelId == labelId) {
			auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
			auto colorTf = modality->transfer()->colorTF();
			auto opacityTf = modality->transfer()->opacityTF();
			colorTf->RemovePoint(seed.scalar);
			opacityTf->RemovePoint(seed.scalar);
			ite = m_seeds.erase(ite);
		} else {
			++ite;
		}
	}
}

void iANModalController::update() {
	m_mdiChild->redrawHistogram();
	m_mdiChild->renderer()->update();
	_updateMainSlicers();
}

using Rgb = std::array<unsigned char, 3>;
using Colors = std::vector<Rgb>;
using Alphas = std::vector<float>;
inline void combineColors(const Colors &colors, const Alphas &opacities, Rgb &output) {
	assert(colors.size() == opacities.size());
	output = {0, 0, 0};
	for (int i = 0; i < colors.size(); ++i) {
		float opacity = opacities[i];
		output[0] += (colors[i][0] * opacity);
		output[1] += (colors[i][1] * opacity);
		output[2] += (colors[i][2] * opacity);
	}
}

inline void setRgba(const vtkSmartPointer<vtkImageData> &img, const int &x, const int &y, const int &z, const Rgb &color, const float &alpha = 255) {
	for (int i = 0; i < 3; ++i)
		img->SetScalarComponentFromFloat(x, y, z, i, color[i]);
	img->SetScalarComponentFromFloat(x, y, z, 3, alpha);
}

inline double getScalar(const vtkSmartPointer<vtkImageData> &img, const int &x, const int &y, const int &z) {
	return img->GetScalarComponentAsDouble(x, y, z, 0);
}

inline void setRgba(unsigned char *ptr, const int &id, const Rgb &color, const float &alpha = 255) {
	ptr[id+0] = color[0];
	ptr[id+1] = color[1];
	ptr[id+2] = color[2];
	ptr[id+3] = alpha;
	//unsigned char rgba[4] = { color[0], color[1], color[2], alpha };
	//memcpy(&ptr[id], rgba, 4 * sizeof(unsigned char));
}

template <typename T>
inline double getScalar(T *ptr, const int &id) {
	return ptr[id];
}

#define iANModal_USE_GETSCALARPOINTER
#ifdef iANModal_USE_GETSCALARPOINTER
#define iANModal_IF_USE_GETSCALARPOINTER(a) a
#else
#define iANModal_IF_USE_GETSCALARPOINTER(a)
#endif

void iANModalController::_updateMainSlicers() {

	/*for (int i = 0; i < NUM_SLICERS; ++i) {
		m_mdiChild->slicer(i)->update();
	}
	return;*/
	
	iASlicerMode slicerModes[NUM_SLICERS] = {
		iASlicerMode::YZ,
		iASlicerMode::XY,
		iASlicerMode::XZ
	};

	const auto numModalities = countModalities();

	iATimeGuard testAll("Process (2D) slice images");
	iAPerformanceHelper perfHelp = iAPerformanceHelper();

	for (int mainSlicerIndex = 0; mainSlicerIndex < NUM_SLICERS; ++mainSlicerIndex) {

		QString testSliceCaption = QString("Process (2D) slice image %1/%2").arg(QString::number(mainSlicerIndex + 1)).arg(QString::number(NUM_SLICERS));
		iATimeGuard testSlice(testSliceCaption.toStdString());

		auto slicer = m_mdiChild->slicer(slicerModes[mainSlicerIndex]);

		std::vector<vtkSmartPointer<vtkImageData>> sliceImgs2D(numModalities);
		iANModal_IF_USE_GETSCALARPOINTER(std::vector<unsigned short *> sliceImgs2D_ptrs(numModalities));
		std::vector<vtkScalarsToColors*>           sliceColorTf(numModalities);
		std::vector<vtkPiecewiseFunction*>         sliceOpacityTf(numModalities);

		for (int modalityIndex = 0; modalityIndex < countModalities(); ++modalityIndex) {
			// Get channel for modality
			// ...this will allow us to get the 2D slice image and the transfer functions
			auto channel = slicer->channel(m_channelIds[modalityIndex]);

			// Make modality transparent
			// TODO: find a better way... this shouldn't be necessary
			slicer->setChannelOpacity(m_channelIds[modalityIndex], 0);

			// Get the 2D slice image
			auto sliceImg2D = channel->reslicer()->GetOutput();
			auto dim = sliceImg2D->GetDimensions();
			assert(dim[0] == 1 || dim[1] == 1 || dim[2] == 1);

			// Save 2D slice image and transfer functions for future processing
			sliceImgs2D[modalityIndex] = sliceImg2D;
			iANModal_IF_USE_GETSCALARPOINTER(sliceImgs2D_ptrs[modalityIndex] = static_cast<unsigned short *>(sliceImg2D->GetScalarPointer()));
			sliceColorTf[modalityIndex] = channel->colorTF();
			sliceOpacityTf[modalityIndex] = channel->opacityTF();

			assert(sliceImg2D->GetNumberOfScalarComponents() == 1);
			assert(sliceImg2D->GetScalarType() == VTK_UNSIGNED_SHORT); // TODO reinforce on Release (e.g. check and display error message on setModalities(...))
		}

		testSlice.time("All info gathered");

		auto sliceImg2D_out = m_sliceImages2D[mainSlicerIndex];

#ifdef iANModal_USE_GETSCALARPOINTER
		auto ptr = static_cast<unsigned char *>(sliceImg2D_out->GetScalarPointer());
#ifndef NDEBUG
		int numVoxels = sliceImg2D_out->GetDimensions()[0] * sliceImg2D_out->GetDimensions()[1] * sliceImg2D_out->GetDimensions()[2];
#endif
#endif

#pragma omp parallel for
		FOR_VTKIMG_PIXELS(sliceImg2D_out, x, y, z) {

			if (x + y + z == 0) perfHelp.start("Processing first voxel");

#ifdef iANModal_USE_GETSCALARPOINTER
			int ijk[3] = {x, y, z};
			int id_scalar = sliceImg2D_out->ComputePointId(ijk);
			int id_rgba = id_scalar * 4;

#ifndef NDEBUG
			{
				assert(id_scalar < numVoxels);
				unsigned char *ptr_test1 = &ptr[id_rgba];
				unsigned char *ptr_test2 = static_cast<unsigned char *>(sliceImg2D_out->GetScalarPointer(x, y, z));
				assert(ptr_test1 == ptr_test2);
			}
#endif
#endif
			
			Colors colors(numModalities);
			Alphas opacities(numModalities);
			float opacitySum = 0;

			// Gather the colors and opacities for this voxel xyz (for each modality)
			for (int mod_i = 0; mod_i < numModalities; ++mod_i) {
#ifdef iANModal_USE_GETSCALARPOINTER
				unsigned short *ptr2 = sliceImgs2D_ptrs[mod_i];

#ifndef NDEBUG
				{
					int id_scalar_test = sliceImgs2D[mod_i]->ComputePointId(ijk);
					assert(id_scalar == id_scalar_test);
					unsigned short *ptr2_test1 = &ptr2[id_scalar_test];
					unsigned short *ptr2_test2 = static_cast<unsigned short *>(sliceImgs2D[mod_i]->GetScalarPointer(x, y, z));
					assert(ptr2_test1 == ptr2_test2);
				}
#endif

				unsigned short scalar = getScalar(ptr2, id_scalar);

#ifndef NDEBUG
			{
					unsigned short scalar_test = sliceImgs2D[mod_i]->GetScalarComponentAsDouble(x, y, z, 0);
					assert(scalar == scalar_test);
			}
#endif
				
#else
				float scalar = getScalar(sliceImgs2D[mod_i], x, y, z);
#endif

				const unsigned char *color = sliceColorTf[mod_i]->MapValue(scalar); // 4 bytes (RGBA)
				float opacity = sliceOpacityTf[mod_i]->GetValue(scalar);

				colors[mod_i] = { color[0], color[1], color[2] }; // RGB (ignore A)
				opacities[mod_i] = opacity;
				opacitySum += opacity;
			}

			// Normalize opacities so that their sum is 1
			if (opacitySum == 0) {
				for (int mod_i = 0; mod_i < numModalities; ++mod_i) {
					opacities[mod_i] = 1 / numModalities;
					opacitySum = 1;
				}
			} else {
				for (int mod_i = 0; mod_i < numModalities; ++mod_i) {
					opacities[mod_i] /= opacitySum;
				}
			}

			Rgb combinedColor;
			combineColors(colors, opacities, combinedColor);

			/*unsigned char aaa = index % 2 == 0 ? 255 : 127;
			switch(omp_get_thread_num()) {
			case 0: combinedColor = { aaa, 0, 0 }; break;
			case 1: combinedColor = { 0, aaa, 0 }; break;
			case 2: combinedColor = { 0, 0, aaa }; break;
			case 3: combinedColor = { aaa, aaa, 0 }; break;
			case 4: combinedColor = { aaa, 0, aaa }; break;
			case 5: combinedColor = { 0, aaa, aaa }; break;
			case 6: combinedColor = { aaa, aaa, aaa }; break;
			default: combinedColor = { 0, 0, 0 }; break;
			}*/

#ifdef iANModal_USE_GETSCALARPOINTER
			setRgba(ptr, id_rgba, combinedColor);
#else
			setRgba(sliceImg2D_out, x, y, z, combinedColor);
#endif

			if (x + y + z == 0) {
				perfHelp.time("First voxel processed");
				perfHelp.stop();
			}

		} // end of FOR_VTKIMG_PIXELS

		testSlice.time("All voxels processed");

		sliceImg2D_out->Modified();
		slicer->channel(0)->imageActor()->SetInputData(sliceImg2D_out);
	}

	testAll.time("Done!");

	for (int i = 0; i < NUM_SLICERS; ++i) {
		m_mdiChild->slicer(i)->update();
	}
}