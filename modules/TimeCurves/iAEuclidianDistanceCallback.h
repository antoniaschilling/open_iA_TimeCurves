#pragma once

#include "iADistanceCallback.h"

class iAEuclidianDistanceCallback : public iADistanceCallback
{
public:
	tapkee::ScalarType distance(std::vector<double> a, std::vector<double> b);
};