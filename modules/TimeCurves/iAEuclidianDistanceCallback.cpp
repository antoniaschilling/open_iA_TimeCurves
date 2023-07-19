#include "iAEuclidianDistanceCallback.h"

tapkee::ScalarType iAEuclidianDistanceCallback::distance(std::vector<double> a, std::vector<double> b)
{
	double euclDist = 0;
	for (int i = 0; i < a.size(); i++)
	{
		double d = a.at(i) - b.at(i);
		euclDist += std::pow(d, 2);
	}
	return euclDist;
}