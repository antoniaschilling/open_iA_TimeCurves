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
# make sure eigen3 is only included once
#add_compile_definitions(TAPKEE_EIGEN_INCLUDE_FILE=<${TAPKEE_EIGEN_INCLUDE_FILE}>)

unset(${TAPKEE_EIGEN_INCLUDE_DIR} CACHE)

set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::objectvis
)
set(DEPENDENCIES_MODULES
	CompVis #for iaMultidimensionalScaling
)
set(DEPENDENCIES_INCLUDE_DIRS
	${TAPKEE_INCLUDE_DIR}
)