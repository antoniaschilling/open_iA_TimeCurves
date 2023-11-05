#pragma once

#include "tapkee/tapkee.hpp"

#include <QString>

class iADistanceCallback
{
public:
	//iADistanceCallback() : displayName{"display"} {};
	iADistanceCallback(QString name = "name") : displayName{name} {};
	QString getDisplayName()
	{
		return displayName;
	};
	virtual tapkee::ScalarType distance(std::vector<double> a, std::vector<double> b) = 0;

private:
	QString displayName;
};