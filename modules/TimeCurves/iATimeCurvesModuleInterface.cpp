#include "iAMainWindow.h"
#include "iATimeCurvesModuleInterface.h"

#include <QAction>
#include <QMessageBox>

#include "iATimeCurves.h"

void iATimeCurvesModuleInterface::Initialize()
{
	if (!m_mainWnd)  // if m_mainWnd is not set, we are running in command line mode
	{
		return;  // in that case, we do not do anything as we can not add a menu entry there
	}
	QAction* actionTimeCurves = new QAction(tr("Time Curves"), m_mainWnd);
	connect(actionTimeCurves, &QAction::triggered, this, &iATimeCurvesModuleInterface::TimeCurves);
	// m_mainWnd->makeActionChildDependent(actionTest);   // uncomment this to enable action only if child window is open
	addToMenuSorted(m_mainWnd->toolsMenu(), actionTimeCurves);
}

void iATimeCurvesModuleInterface::TimeCurves()
{
	iATimeCurves* timeCurves = new iATimeCurves();

	//todo what in cmd-mode?
	timeCurves->start(m_mainWnd);
}