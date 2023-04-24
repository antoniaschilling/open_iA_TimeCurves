#pragma once

#include "iAMainWindow.h"


class iATimeCurves
{
public:
	//launches upl window?
	static void start(iAMainWindow* mainWindow);

	//load data
	bool loadData();

	//save matrix to computer
	bool downloadDistanceMatrix;

private:
	QStringList* csvFiles;
	//compute distance matrix

	//parse csv
	bool parseCsv();

	bool mds();

	
};


