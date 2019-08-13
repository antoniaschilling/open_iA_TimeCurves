#pragma once
namespace algorithm {
	static bool greaterThan(double u, double v) {
		return u > v;
	}

	static bool smallerThan(double u, double v) {
		return u < v;
	}

	const double epsilon = 0.0000000001;

	static bool compareDouble(double a, double b) {
		return fabs(a - b) < epsilon;
	}


	//checks whether a double is within a certian range
	static bool compareDouble(double a, double b, double toleranceVal) {
		return fabs(a - b) < toleranceVal; 
	}

}