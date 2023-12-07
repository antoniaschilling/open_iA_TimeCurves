#pragma once

#include "iAMainWindow.h"
#include "tapkee/tapkee.hpp"
#include "tapkee/defines/types.hpp"


class iATimeCurves
{
public:
	static iATimeCurves& start(iAMainWindow* mainWindow);

	void addCurve();
	//load data
	bool loadData();

	//static int getIndex(iATimeCurvesWidget widget);

	//Eigen::MatrixXd stdVectorToMatrix(std::vector<std::vector<double>>* data);

	//save matrix to computer
	bool downloadDistanceMatrix;

private:
	iAMainWindow* m_mainWindow;
	QList<QWidget*> widgets;
	QStringList* csvFiles;

	//list of possible distance functions
	//chosen distance function

	int* headerLine;
	QString* name;
	QString* selectedDistance;
	//compute distance matrix

	//parse csv
	void parseCsvAllToOne(std::vector<std::vector<double>>* data);

	std::vector<std::vector<double>>* parseCsvToStdVector(QString fileName);

	void parseCsvToMatrix(QString fileName, Eigen::MatrixXd* matrix);
	//todo
	bool mds();

	tapkee::DenseMatrix* simpleMds();
	void normalize(Eigen::MatrixXd* data);
	void matrixToStdVector(Eigen::MatrixXd* matrix, std::vector<std::vector<double>>* vector);

	//for debugging
	bool filePath;
	bool precomputedFVs;
	bool precomputedMDS;
	bool testEmbedding;
	void printTapkeeOutput(tapkee::TapkeeOutput output, QString fileName);
	void printTapkeeDistanceMatrix(tapkee::DenseMatrix output);
	void readDistanceMatrixFromFile(tapkee::DenseSymmetricMatrix* distanceMatrix, QString fileName);
	void mdsExample();
};
