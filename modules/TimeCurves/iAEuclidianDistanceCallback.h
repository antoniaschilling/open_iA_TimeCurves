#pragma once

#include "iADistanceCallback.h"

class iAEuclidianDistanceCallback : public iADistanceCallback
{
public:
	iAEuclidianDistanceCallback(QString name = "Euclidian Distance") : iADistanceCallback{name} {};

	tapkee::ScalarType distance(std::vector<double> a, std::vector<double> b) override;
	tapkee::ScalarType distance(double a[], double b[]);
	template <typename Derived>
	tapkee::ScalarType distance(Eigen::MatrixBase<Derived> a, Eigen::MatrixBase<Derived> b);
};