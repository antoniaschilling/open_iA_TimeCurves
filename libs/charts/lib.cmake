TARGET_LINK_LIBRARIES(${libname} PUBLIC
	iAqthelper
)
IF (MSVC AND MSVC_VERSION GREATER_EQUAL 1910)              # apparently required for VS < 2019:
	TARGET_LINK_LIBRARIES(${libname} PUBLIC Opengl32)
ENDIF ()
SET(VTK_REQUIRED_LIBS_PRIVATE
	ImagingStatistics       # for vtkImageAccumulate
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
if (openiA_CHART_SP_OLDOPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE SP_OLDOPENGL)
endif()
if (openiA_OPENGL_DEBUG)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE OPENGL_DEBUG)
endif()