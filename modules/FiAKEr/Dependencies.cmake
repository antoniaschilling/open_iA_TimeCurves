SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAcore
	iAobjectvis
	iArenderer    # for iARendererManager
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersExtraction           # for vtkExtractGeometry used iASelectionInteractorStyle
)
