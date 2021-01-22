SET( DEPENDENCIES_LIBRARIES
	iAbase
)
SET( DEPENDENCIES_ITK_MODULES
	ITKAnisotropicSmoothing      # for CurvatureAnisotropicDiffusionImageFilter, GradientAnisotropicDiffusionImageFilter
	ITKConnectedComponents       # for ...ConnectedComponentImageFilter, RelabelComponentImageFilter
	ITKConvolution               # for ConvolutionImageFilter
	ITKCurvatureFlow             # for CurvatureFlowImageFilter
	ITKDenoising                 # for PatchBasedDenoisingImageFilter
	ITKDistanceMap               # for (Danielsson|SignedMaurer)DistanceMapImageFilter
	ITKFFT                       # for (Real)HalfHermitianToRealInverseFFTImageFilter, dependency of FFTConvolutionImageFilter
	ITKFiniteDifference          # for DenseFiniteDifferenceImageFilter (dependency of AnisotropicDiffusionImageFilter), FiniteDifferenceImageFilter (dependency of SparseFieldLevelSetImageFilter)
	ITKImageAdaptors             # for NthElementImageAdaptor (dependency of HessianRecursiveGaussianImageFilter, PatchBasedDenoisingImageFilter)
	ITKImageFeature              # for HessianRecursiveGaussianImageFilter
	ITKImageFusion               # for LabelToRGBImageFilter
	ITKImageGradient             # for GradientMagnitudeImageFilter
	ITKImageLabel                # for ScanlineFilterCommon, dependency of ConnectedComponentImageFilter
	ITKImageNoise                # for AdditiveGaussianNoiseImageFilter, ...
	ITKImageSources              # for GaussianImageSource, dependency of BilateralImageFilter
	ITKMathematicalMorphology    # for BinaryBallStructuringElement
	ITKSmoothing
	ITKStatistics                # for Histogram, dependency of HistogramMatchingImageFilter
	ITKThresholding              # for BinaryThresholdImageFilter, dependency of SignedMaurerDistanceMapImageFilter
	ITKTestKernel                # for PipelineMonitorImageFilter
	ITKTransform                 # for Transform, dependency of ResampleImageFilter
)
IF (HigherOrderAccurateGradient_LOADED)
	LIST (APPEND DEPENDENCIES_ITK_MODULES HigherOrderAccurateGradient)
ENDIF()
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	RemovePeaksOtsu    # for itkFHWRescaleIntensityImageFilter
)
