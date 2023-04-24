#include "iATimeCurves.h"

//make sure eigen3 is only included once
#define TAPKEE_EIGEN_INCLUDE_FILE <C:/Tools/eigen-3.4.0/Eigen/Eigen>
#ifdef HAVE_ARPACK
#define TAPKEE_WITH_ARPACK
#endif

#include "InitDialog.h"
#include "dlg_CSVInput.h"

#include "iALog.h"

#include "tapkee/tapkee.hpp"



void iATimeCurves::start(iAMainWindow* mainWindow)
{
	iATimeCurves timeCurves{};

	timeCurves.loadData();

	//do mds
	//timeCurves.mds();

	//make curve

}

bool iATimeCurves::loadData()
{
	LOG(lvlInfo, "InitDialog started.");
	InitDialog* initDialog = new InitDialog(csvFiles);
	initDialog->exec();

    return false;
}

bool iATimeCurves::parseCsv()
{
    return false;
}

bool iATimeCurves::mds()
{
	return false;
}
