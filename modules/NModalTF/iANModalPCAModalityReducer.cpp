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

#include "iANModalPCAModalityReducer.h"

#include <defines.h> // for DIM
#include "iAModality.h"
#include "iATypedCallHelper.h"

#ifndef NDEBUG
#include "iAToolsITK.h"
#include "iAPerformanceHelper.h" // TODO
#endif

#include <vtkImageData.h>

#include <itkImagePCAShapeModelEstimator.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>

// Input modalities (volumes) must have the exact same dimensions
QList<QSharedPointer<iAModality>> iANModalPCAModalityReducer::reduce(QList<QSharedPointer<iAModality>> modalities) {

	// TODO: assert if all modalities have the same dimensions

	// Set up connectors
	std::vector<iAConnector> connectors(modalities.size());
	for (int i = 0; i < modalities.size(); i++) {
		connectors[i] = iAConnector();
		connectors[i].setImage(modalities[i]->image());
	}

	// Go!
	//ITK_TYPED_CALL(itkPCA, connectors[0].itkScalarPixelType(), connectors);
	ITK_TYPED_CALL(ownPCA, connectors[0].itkScalarPixelType(), connectors);

	// Set up output list
	modalities = QList<QSharedPointer<iAModality>>();
	for (int i = 0; i < connectors.size(); i++) {
		auto name = "Principal Component " + QString::number(i);

		auto imageData = vtkSmartPointer<vtkImageData>::New();
		imageData->DeepCopy(connectors[i].vtkImage());

		auto mod = new iAModality(name, "", -1, imageData, iAModality::NoRenderer);

#ifndef NDEBUG
		storeImage2(connectors[i].itkImage(), "pca_output_" + QString::number(i) + ".mhd", true);
#endif

		//QSharedPointer<iAVolumeRenderer> renderer(new iAVolumeRenderer(mod->transfer().data(), mod->image()));
		//mod->setRenderer(renderer);

		//m_mdiChild->modalitiesDockWidget()->addModality(...);

		modalities.append(QSharedPointer<iAModality>(mod));
	}

	// Ready to output :)
	// - length of output list <= maxOutputLength()
	auto output = modalities;
	assert(output.size() <= maxOutputLength());
	return output;
}

template<class T>
void iANModalPCAModalityReducer::itkPCA(std::vector<iAConnector> &c) {
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::ImagePCAShapeModelEstimator<ImageType, ImageType> PCASMEType;

	int inputSize = c.size();
	int outputSize = std::min((int)c.size(), maxOutputLength());
	
	auto pca = PCASMEType::New();
	pca->SetNumberOfTrainingImages(inputSize);
	pca->SetNumberOfPrincipalComponentsRequired(outputSize);
	for (int i = 0; i < inputSize; i++) {

#ifndef NDEBUG
		storeImage2(c[i].itkImage(), "pca_input_itk_" + QString::number(i) + ".mhd", true);
		storeImage2(dynamic_cast<ImageType *>(c[i].itkImage()), "pca_input_itkcast_" + QString::number(i) + ".mhd", true);
#endif

		pca->SetInput(i, dynamic_cast<ImageType *>(c[i].itkImage()));
	}

	pca->Update();

	// Debug. TODO: remove
	PCASMEType::VectorOfDoubleType eigenValues = pca->GetEigenValues();
	double sv_mean = sqrt(eigenValues[0]);
	printf("sv_mean = %d\n", sv_mean);

	auto count = pca->GetNumberOfOutputs();

	// TODO uncomment
	//c.resize(outputSize);
	//for (int i = 0; i < outputSize; i++) {
	c.resize(count);
	for (int i = 0; i < count; i++) {

#ifndef NDEBUG
		storeImage2(pca->GetOutput(i), "pca_output_before_conversion_" + QString::number(i) + ".mhd", true);
#endif

		c[i].setImage(pca->GetOutput(i));
	}
}

#ifndef NDEBUG
#define DEBUG_LOG_MATRIX(matrix, string) \
	{ QString str = string; \
	str += "\n"; \
	for (int i = 0; i < numInputs; i++) { \
		auto row = matrix.get_row(i); \
		for (int j = 0; j < row.size(); j++) { \
			str += QString::number(row[j]) + "     "; \
		} \
		str += "\n"; \
	} \
	DEBUG_LOG(str); }
#define DEBUG_LOG_VECTOR(vector, string) \
	{ QString str = string; \
	str += "\n"; \
	for (int i = 0; i < vector.size(); i++) { \
		str += QString::number(vector[i]) + "     "; \
	} \
	str += "\n"; \
	DEBUG_LOG(str); }
#else
#define DEBUG_LOG_MATRIX(matrix, string)
#define DEBUG_LOG_VECTOR(vector, string)
#endif

template<class T>
void iANModalPCAModalityReducer::ownPCA(std::vector<iAConnector> &c) {
	typedef itk::Image<T, DIM> ImageType;

	assert(c.size() > 0);

	int numInputs = c.size();
	int numOutputs = std::min((int)c.size(), maxOutputLength());

	auto itkImg0 = c[0].itkImage();

	auto size = itkImg0->GetBufferedRegion().GetSize();
	int numVoxels = 1;
	for (unsigned int dim_i = 0; dim_i < DIM; dim_i++) {
		numVoxels *= size[dim_i];
	}

	// Set up input matrix
	vnl_matrix<double> inputs(numInputs, numVoxels);
	for (int row_i = 0; row_i < numInputs; row_i++) {
		auto input = dynamic_cast<const ImageType *>(c[row_i].itkImage());
		auto iterator = itk::ImageRegionConstIterator<ImageType>(input, input->GetBufferedRegion());
		iterator.GoToBegin();

		for (int col_i = 0; col_i < numVoxels; col_i++) {
			inputs[row_i][col_i] = iterator.Get();
			++iterator;
		}

#ifndef NDEBUG
		storeImage2(c[row_i].itkImage(), "pca_input_itk_" + QString::number(row_i) + ".mhd", true);
		storeImage2(dynamic_cast<ImageType *>(c[row_i].itkImage()), "pca_input_itkcast_" + QString::number(row_i) + ".mhd", true);
#endif
	}

#ifndef NDEBUG
	int numThreads;
#pragma omp parallel
	{
#pragma omp single
		numThreads = omp_get_num_threads();
	}
	DEBUG_LOG(QString::number(numThreads) + " threads available\n");
#endif

	// Calculate means
	vnl_vector<double> means;
	means.set_size(numInputs);
	for (int img_i = 0; img_i < numInputs; img_i++) {
#pragma omp parallel
		{
			double mean = 0;
#pragma omp for
			for (int i = 0; i < numVoxels; i++) {
				//means[img_i] += inputs[img_i][i];
				mean += inputs[img_i][i];
			}
#pragma omp single
			means[img_i] = 0;
#pragma omp barrier
#pragma omp atomic
			means[img_i] += mean;
		} // end of parallel block
		means[img_i] /= numVoxels;
	}

	DEBUG_LOG_VECTOR(means, "Means");

	// Calculate inner product (lower triangle) (for covariance matrix)
	vnl_matrix<double> innerProd;
	innerProd.set_size(numInputs, numInputs);
	innerProd.fill(0);
	for (int ix = 0; ix < numInputs; ix++) {
		for (int iy = 0; iy <= ix; iy++) {
#pragma omp parallel
			{
				double innerProd_thread = 0;
#pragma omp for
				for (int i = 0; i < numVoxels; i++) {
					auto mx = inputs[ix][i] - means[ix];
					auto my = inputs[iy][i] - means[iy];
					//innerProd[ix][iy] += (mx * my); // Product takes place!
					innerProd_thread += (mx * my);
				}
#pragma omp single
				innerProd[ix][iy] = 0;
#pragma omp barrier
#pragma omp atomic
				innerProd[ix][iy] += innerProd_thread;

/*#ifndef NDEBUG
#pragma omp single
				DEBUG_LOG_MATRIX(innerProd, "Inner product (not complete)");
#endif*/
			} // end of parallel block
		}
	}

	// Fill upper triangle (make symmetric)
	for (int ix = 0; ix < (numInputs - 1); ix++) {
		for (int iy = ix + 1; iy < numInputs; iy++) {
			innerProd[ix][iy] = innerProd[iy][ix];
		}
	}
	DEBUG_LOG_MATRIX(innerProd, "Inner product");

	// Make covariance matrix (divide by N-1)
	if (numInputs - 1 != 0) {
		innerProd /= (numVoxels - 1);
	} else {
		innerProd.fill(0);
	}
	DEBUG_LOG_MATRIX(innerProd, "Covariance matrix");

	// Solve eigenproblem
	vnl_matrix<double> eye(numInputs, numInputs); // (eye)dentity matrix
	eye.set_identity();
	//DEBUG_LOG_MATRIX(eye, "Identity");
	vnl_generalized_eigensystem evecs_evals_innerProd(innerProd, eye);
	auto evecs_innerProd = evecs_evals_innerProd.V;
	evecs_innerProd.fliplr(); // Flipped because VNL sorts eigenvectors in ascending order
	if (numInputs != numOutputs) evecs_innerProd.extract(numInputs, numOutputs); // Keep only 'numOutputs' columns
	//auto evals_innerProd = evecs_evals_innerProd.D.diagonal();

	DEBUG_LOG_MATRIX(evecs_innerProd, "Eigenvectors");
	DEBUG_LOG_VECTOR(evecs_evals_innerProd.D.diagonal(), "Eigenvalues");
	
	// Initialize the reconstructed matrix (with zeros)
	vnl_matrix<double> reconstructed(numOutputs, numVoxels);
	for (int row_i = 0; row_i < numOutputs; row_i++) {
#pragma omp parallel for
		for (int col_i = 0; col_i < numVoxels; col_i++) {
			reconstructed[row_i][col_i] = 0;
		}
	}

	// Transform images to principal components

	for (int row_i = 0; row_i < numInputs; row_i++) {
		for (int vec_i = 0; vec_i < numOutputs; vec_i++) {
			auto evec_elem = evecs_innerProd[row_i][vec_i];
#pragma omp parallel for
			for (int col_i = 0; col_i < numVoxels; col_i++) {
				auto voxel_value = inputs[row_i][col_i];
				reconstructed[vec_i][col_i] += (voxel_value * evec_elem);
			}
		}
	}

	// Normalize row-wise (i.e. image-wise) to range 0..1
	for (int vec_i = 0; vec_i < numOutputs; vec_i++) {
		double max_val = -DBL_MAX;
		double min_val = DBL_MAX;
#pragma omp parallel
		{
			double max_thread = -DBL_MAX;
			double min_thread = DBL_MAX;
#pragma omp for
			for (int i = 0; i < numVoxels; i++) {
				auto rec = reconstructed[vec_i][i];
				//max_val = max_val > rec ? max_val : rec;
				//min_val = min_val < rec ? min_val : rec;
				max_thread = max_thread > rec ? max_thread : rec;
				min_thread = min_thread < rec ? min_thread : rec;
			}

#pragma omp critical // Unfortunatelly, no atomic max :(
			max_val = max_thread > max_val ? max_thread : max_val;
#pragma omp critical // Unfortunatelly, no atomic min :(
			min_val = min_thread < min_val ? min_thread : min_val;

#pragma omp barrier

#pragma omp for
			for (int i = 0; i < numVoxels; i++) {
				auto old = reconstructed[vec_i][i];
				reconstructed[vec_i][i] = (old - min_val) / (max_val - min_val) * 65536.0;
			}
		} // end of parallel block
	}

	// Reshape reconstructed vectors into image
	c.resize(numOutputs);
	for (unsigned int out_i = 0; out_i < numOutputs; out_i++) {
		auto recvec = reconstructed.get_row(out_i);

		auto output = ImageType::New();
		ImageType::RegionType region;
		region.SetSize(itkImg0->GetLargestPossibleRegion().GetSize());
		region.SetIndex(itkImg0->GetLargestPossibleRegion().GetIndex());
		output->SetRegions(region);
		output->SetSpacing(itkImg0->GetSpacing());
		output->Allocate();
		auto ite = itk::ImageRegionIterator<ImageType>(output, region);

		unsigned int i = 0;
		ite.GoToBegin();
		while (!ite.IsAtEnd()) {

			double rec = recvec[i];
			ImageType::PixelType rec_cast = static_cast<typename ImageType::PixelType>(rec);
			ite.Set(rec_cast);

			++ite;
			++i;
		}

#ifndef NDEBUG
		storeImage2(output, "pca_output_before_conversion_" + QString::number(out_i) + ".mhd", true);
#endif

		c[out_i].setImage(output);
	}
}
