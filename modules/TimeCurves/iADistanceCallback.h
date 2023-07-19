#pragma once

#include "tapkee/tapkee.hpp"

class iADistanceCallback
{
public:
	virtual tapkee::ScalarType distance(std::vector<double> a, std::vector<double> b) = 0;
};