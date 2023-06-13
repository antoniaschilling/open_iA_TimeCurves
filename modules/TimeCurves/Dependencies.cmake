set(TAPKEE_INCLUDE_DIR
	"${TAPKEE_INCLUDE_DIR}"
	CACHE
	PATH
	"Path to tapkee include directory"
)
set(TAPKEE_EIGEN_INCLUDE_FILE
	"${TAPKEE_EIGEN_INCLUDE_FILE}"
	CACHE
	FILEPATH
	"Path to eigen include file for tapkee"
)

if (NOT Qt${QT_VERSION_MAJOR}Charts_DIR)
	set(Qt${QT_VERSION_MAJOR}Charts_DIR "${Qt${QT_VERSION_MAJOR}_DIR}Charts" CACHE PATH "" FORCE)
endif()
find_package(Qt${QT_VERSION_MAJOR}Charts REQUIRED)

unset(${TAPKEE_EIGEN_INCLUDE_DIR} CACHE)

set(DEPENDENCIES_LIBRARIES
	Qt${QT_VERSION_MAJOR}::Charts
	iA::guibase
	iA::objectvis
)
set(DEPENDENCIES_MODULES
	CompVis #for iaMultidimensionalScaling
)
set(DEPENDENCIES_INCLUDE_DIRS
	${TAPKEE_INCLUDE_DIR}
)