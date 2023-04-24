#pragma once

#include <iAGUIModuleInterface.h>

class iATimeCurvesModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
private slots:
	void TimeCurves();
};