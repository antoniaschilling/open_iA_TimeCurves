
#include "iASimpleTester.h"

#include "tapkee/tapkee.hpp"

//#include "iATimeCurves.h"	//fails because of iAMainwindow.h include


struct MyDistanceCallback
{
	tapkee::ScalarType distance(tapkee::IndexType l, tapkee::IndexType r)
	{
		return abs(l - r);
	}
};

//test takpee functionality
BEGIN_TEST
{
	bool testExecuted = false;
	tapkee::DenseMatrix* embedding = new tapkee::DenseMatrix;
	tapkee::TapkeeOutput output;
	Eigen::IOFormat CleanFmt(4, Eigen::DontAlignCols, ",", "\n", "", "");
	std::stringstream buffer;

	// minimal example taken from: https://tapkee.lisitsyn.me/#minimal
	const int N = 100;
	std::vector<tapkee::IndexType> indices(N);
	for (int i = 0; i < N; i++) indices[i] = i;
	MyDistanceCallback distance;
	output = tapkee::initialize()
							  .withParameters((tapkee::method = tapkee::MultidimensionalScaling, tapkee::target_dimension = 1))
							  .withDistance(distance)
							  .embedUsing(indices);
	std::cout << "output: " << output.embedding.transpose().format(CleanFmt) << std::endl;

	std::cout << "Test executed: " << testExecuted << std::endl;
	TestAssert(testExecuted);
}
END_TEST
