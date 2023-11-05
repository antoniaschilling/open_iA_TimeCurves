#pragma once
#include "iADistanceCallback.h"

class iACosineDistanceCallback : public iADistanceCallback
{
public:
	iACosineDistanceCallback(QString name = "Cosine Distance") : iADistanceCallback{name} {};
	tapkee::ScalarType distance(std::vector<double> a, std::vector<double> b) override;
	double cosineSimilarity(std::vector<double> a, std::vector<double> b);
};
