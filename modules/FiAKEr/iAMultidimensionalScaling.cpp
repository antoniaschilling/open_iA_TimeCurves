#include "iAMultidimensionalScaling.h"

#ifdef NO_DLL_LINKAGE
// TODO: Log output support in tests!
#include <iostream>
#define DEBUG_LOG(msg) std::cout << msg.toStdString() << std::endl;
#else
#include "iAConsole.h"
#endif
#include "iAStringHelper.h"

#include <algorithm>
#include <cmath>

namespace
{

	void initializeRandom(iAMatrixType& result, size_t rows, size_t cols)
	{
		result.resize(rows);
		double rand2 = 1.0 / rows;
		for (size_t r = 0; r < rows; r++)
		{
			result[r] = std::vector<double>(cols, rand2);
			rand2 += (1.0 / rows);
		}
	}

	double matrixSum(iAMatrixType const& input)
	{
		return std::accumulate(input.cbegin(), input.cend(), 0,
			[](double val, std::vector<double> const& vec)
			{
				return std::accumulate(vec.cbegin(), vec.cend(), val);
			});
	}

	//! mean of a 2D matrix (inequal column/row sizes are accounted for)
	double matrixMean(iAMatrixType const& input)
	{
		size_t num = std::accumulate(input.cbegin(), input.cend(), 0,
			[](double val, std::vector<double> const& vec)
			{
				return val + vec.size();
			});
		return matrixSum(input) / num;
	}

	void matrixAddScalar(iAMatrixType& input, double value)
	{
		for (auto& d : input)
		{
			for (auto& v : d)
			{
				v += value;
			}
		}
	}

	void matrixMultiplyScalar(iAMatrixType& input, double value)
	{
		for (auto& d : input)
		{
			for (auto& v : d)
			{
				v *= value;
			}
		}
	}

	void computeDistance(iAMatrixType const& dataMatrix, iAMatrixType& distanceMatrix)
	{
		for (int r = 0; r < dataMatrix.size() - 1; r++)
		{
			for (int c = r + 1; c < dataMatrix.size(); c++)
			{
				double temp = 0;
				for (int c2 = 0; c2 < dataMatrix[0].size(); c2++)
				{
					temp += pow(dataMatrix[r][c2] - dataMatrix[c][c2], 2);
				}
				distanceMatrix[r][c] = std::sqrt(temp);
			}
		}
		// fill other half triangle:
		for (int r = 1; r < distanceMatrix.size(); r++)
		{
			for (int c = 0; c < r; c++)
			{
				distanceMatrix[r][c] = distanceMatrix[c][r];
			}
		}
		//DEBUG_LOG(QString("Euclidean Distance:\n%1").arg(matrixToString(distanceMatrix)));
	}


	// DEBUG methods:

	iAMatrixType vectorDiff(iAMatrixType const& a, iAMatrixType const& b)
	{
		iAMatrixType result(a.size(), std::vector<double>(a[0].size(), 0.0));
		for (int i = 0; i < a.size(); ++i)
		{
			for (int j = 0; j < a[0].size(); ++j)
			{
				result[i][j] = std::abs(a[i][j] - b[i][j]);
			}
		}
		return result;
	}
}

/*
QString distanceMetricToString(int i)
{
	switch (i)
	{
	case 0: return QString("Euclidean Distance");
	case 1: return QString("Minkowski Distance");
	default: return QString("Invalid DistanceMetric");
	}
}

iADistanceMetricID stringToDistanceMetric(QString const & string)
{
	if (string == QString("Euclidean Distance"))
	{
		return iADistanceMetricID::EuclideanDistance;
	}
	else if (string == QString("Minkowski Distance"))
	{
		return iADistanceMetricID::MinkowskiDistance;
	}
	else
	{
		return iADistanceMetricID::Unknown;
	}
}

iAMatrixDistance* initializeDistanceMetric(iADistanceMetricID distanceMetric)
{
	if (distanceMetric == iADistanceMetricID::EuclideanDistance)
	{
		return new iAEuclideanDistance();
	}
	else if (distanceMetric == iADistanceMetricID::MinkowskiDistance)
	{
		return new iAMinkowskiDistance();
	}
	return nullptr;
}
*/

QString matrixToString(iAMatrixType const& input)
{
	QString output = QString("%1x%2:\n").arg(input.size()).arg(input[0].size());
	for (int r = 0; r < input.size(); ++r)
	{
		output += joinAsString(input[r], ",",
			[](double val) -> QString { return QString::number(val, 'g', 7); })
			+ "\n";
	}
	return output;
}

std::vector<std::vector<double>> computeMDS(std::vector<std::vector<double>> const& distanceMatrix,
	int outputDimensions, int iterations, double maxError/*, iADistanceMetricID distanceMetric*/)
{
	//DEBUG_LOG(QString("DistanceMatrix: %1").arg(matrixToString(distanceMatrix)));

	//X - configuration of points in Euclidean space
	//initialize X with one vector filled with random values between [0,1]
	
	auto numElems = distanceMatrix.size();
	assert(numElems > 2 && numElems == distanceMatrix[0].size()); // at least 3 elements and quadratic matrix
	iAMatrixType X;
	initializeRandom(X, numElems, outputDimensions);
	//DEBUG_LOG(QString("init X: %1").arg(matrixToString(X)));
		
	//DEBUG_LOG(QString("init X:\n%1").arg(matrixToString(X)));

	// mean value of distance matrix
	double meanD = matrixMean(distanceMatrix);
	//DEBUG_LOG(QString("mean=%1").arg(meanD));

	// move to the center
	matrixAddScalar(X, -0.5);
	//DEBUG_LOG(QString("subt X: %1").arg(matrixToString(X)));

	// before this step, mean distance is 1/3*sqrt(d)
	matrixMultiplyScalar(X, 0.1 * meanD / (1.0 / 3.0 * sqrt((double)outputDimensions)));
	//DEBUG_LOG(QString("normalized X: %1").arg(matrixToString(X)));

	iAMatrixType Z(X);
	iAMatrixType D(numElems, std::vector<double>(numElems, 0.0));
	iAMatrixType B(numElems, std::vector<double>(numElems, 0.0));

	computeDistance(X, D);
	//DEBUG_LOG(QString("D: %1").arg(matrixToString(D)));

	const double Epsilon = 0.000001;
	//MDS iteration
	double diffSum = 1;
	for (int it = 0; it < iterations && diffSum > maxError; it++)
	{
		DEBUG_LOG(QString("ITERATION %1:").arg(it));
		//DEBUG_LOG(QString("B old: %1").arg(matrixToString(B)));

		// B = calc_B(D_,D);
		for (int r = 0; r < numElems; r++)
		{
			for (int c = 0; c < numElems; c++)
			{
				if ( r == c || std::fabs(D[r][c]) < Epsilon)
				{
					B[r][c] = 0.0;
				}
				else
				{
					B[r][c] = -distanceMatrix[r][c] / D[r][c];
				}
			}
		}

		for (int c = 0; c < numElems; c++)
		{
			double temp = 0;
			for (int r = 0; r < numElems; r++)
			{
				temp += B[r][c];

			}

			B[c][c] = -temp;
		}
		//DEBUG_LOG(QString("B new: %1").arg(matrixToString(B)));

		//DEBUG_LOG(QString("Z: %1").arg(matrixToString(Z)));

		// X = B*Z/size(D,1);
		for (int r = 0; r < X.size(); r++)	
		{
			for (int xCols = 0; xCols < X[0].size(); xCols++)
			{
				double temp = 0;
				for (int bCols = 0; bCols < B[0].size(); bCols++)
				{
					temp += (B[r][bCols] * Z[bCols][xCols]);
				}

				X[r][xCols] = temp / numElems;
			}
		}
		//DEBUG_LOG(QString("X: %1").arg(matrixToString(X)));

		//D_ = calc_D (X);
		computeDistance(X, D);
		//DEBUG_LOG(QString("D: %1").arg(matrixToString(D)));

		auto vecDiff = vectorDiff(Z, X);
		diffSum = matrixSum(vecDiff);
		DEBUG_LOG(QString("diff Z-X (sum=%1): %2").arg(diffSum).arg(matrixToString(vecDiff)));
		Z = X;
	}
	return X;
}
