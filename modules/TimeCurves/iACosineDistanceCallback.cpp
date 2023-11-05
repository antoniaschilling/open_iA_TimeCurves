#include "iACosineDistanceCallback.h"

tapkee::ScalarType iACosineDistanceCallback::distance(std::vector<double> a, std::vector<double> b)
{
	return 1 - cosineSimilarity(a, b);
}

//todo get source
double iACosineDistanceCallback::cosineSimilarity(
	std::vector<double> a, std::vector<double> b)
{
	double dot = 0.0, denom_a = 0.0, denom_b = 0.0;
	for (unsigned int i = 0u; i < a.size(); ++i)
	{
		dot += a[i] * b[i];
		denom_a += a[i] * a[i];
		denom_b += b[i] * b[i];
	}
	return dot / (sqrt(denom_a) * sqrt(denom_b));
}
