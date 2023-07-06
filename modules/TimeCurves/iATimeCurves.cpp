#include "iATimeCurves.h"

#include <fstream>

//Qt
#include <qwidget.h>
#include <qfile.h>

//iA
#include "InitDialog.h"
#include "dlg_CSVInput.h"
#include "iALog.h"
#include "iATimeCurvesWidget.h"


#include "tapkee/tapkee.hpp"

using namespace tapkee;

void iATimeCurves::start(iAMainWindow* mainWindow)
{
	iATimeCurves timeCurves{};
	timeCurves.headerLine = new int;
	timeCurves.csvFiles = new QStringList;
	timeCurves.m_mainWindow = mainWindow;

	//for faster debugging:
	timeCurves.filePath = true;
	timeCurves.precomputedFVs = false;
	timeCurves.precomputedMDS = false;

	if (timeCurves.filePath)
	{
		QStringList list = {
			"C:/Users/tonik/Documents/Antonia/Informatik Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
							"CR-PPsGF24-0deg-6-2um-0N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv",
			"C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"CR-PPsGF24-0deg-6-2um-132N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv",
		"C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"CR-PPsGF24-0deg-6-2um-228N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv",
		"C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"CR-PPsGF24-0deg-6-2um-263N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv",
		"C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"CR-PPsGF24-0deg-6-2um-299N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv",
		"C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"CR-PPsGF24-0deg-6-2um-334N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv",
		"C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"CR-PPsGF24-0deg-6-2um-369N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv"
			,
			"C:/Users/tonik/Documents/Antonia/Informatik "
			"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
			"CR-PPsGF24-0deg-6-2um-404N-BHC8_16bit_1579x1092x1651_ROI-TM_Rev10b.csv"
		};
		*timeCurves.csvFiles = list;
		int x = 4;
		*timeCurves.headerLine = x;
	}
	else
	{
		if (!timeCurves.loadData())
		{
			return;
		}
	}

	//do mds
	timeCurves.simpleMds();

	//make curve

}

bool iATimeCurves::loadData()
{
	LOG(lvlInfo, "InitDialog started.");
	InitDialog* initDialog = new InitDialog(&csvFiles, &headerLine);
	if (initDialog->exec() == 1)
	{
		LOG(lvlDebug, QString("Got valid data with header at line: '%1'").arg(*headerLine));
		LOG(lvlDebug, QString("Got valid data with csvFiles: '%1'").arg(csvFiles->at(0)));
		return true;
	}
    return false;
}

//Eigen::MatrixXd iATimeCurves::stdVectorToMatrix(std::vector<std::vector<double>>* data)
//{
//
//}

std::vector<std::vector<double>>* iATimeCurves::parseCsvToStdVector(QString fileName)
{
	LOG(lvlDebug, QString("Parsing csv '%1'.").arg(fileName));
	std::vector<std::vector<double>>* data = new std::vector<std::vector<double>>;
	uint rows = 0;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Error opening CSV file '%1'.").arg(csvFiles->at(0)));
	}
	QTextStream in(&file);
	QString curLine;
	std::vector<double> buff;
	int line = 0;
	int varCount;
	while (!file.atEnd())
	{
		curLine = in.readLine();
		if (line == *headerLine)
		{
			varCount = curLine.split(',').length();
		}
		if (line > *headerLine)
		{
			QStringList values = curLine.split(',');
			//todo trailing comma -> 1 col +
			int cols = 0;
			for (const auto& value : values)
			{
				buff.push_back(value.toDouble());
				++cols;
			}
			if (buff.size() == varCount)
			{
				data->push_back(buff);
				rows++;
			}
			else
			{
				LOG(lvlInfo, QString("Row discarded, col number did noch match cols of header at  row '%2'").arg(line));
			}
		}
		
		buff.clear();
		line++;
	}

	LOG(lvlDebug, QString("Parsed data to vector with '%1' rows and '%2' columns.").arg(rows).arg(data->at(0).size()));
	return data;
}

struct EuclidianDistanceCallback
{
	ScalarType distance(std::vector<double> a, std::vector<double> b)
	{
		double euclDist = 0;
		for (int i = 0; i < a.size(); i++)
		{
			double d = a.at(i) - b.at(i);
			euclDist += std::pow(d, 2);
		}
		return euclDist;
	}
};

//struct AverageLinkageDistanceCallback
//{
//	ScalarType distance(std::vector<double> a, std::vector<double> b)
//	{
//
//		std::vector<std::vector<double>>* data
//
//		//distance Matrix must be precomputed
//
//		//how do i know the index of my a,b datasets in the matrix??
//		//add additional info as columns with index?? i.e. col 0: n° dataset (in parse)
//
//	}
//};

bool iATimeCurves::simpleMds()
{
	std::vector<std::vector<double>>* data = new std::vector<std::vector<double>>;
	EuclidianDistanceCallback euclidianDistance;

	//preprocess data
	for (int i = 0; i < csvFiles->length(); i++)
	{
		//parse
		std::vector<std::vector<double>>* dataset = parseCsvToStdVector(csvFiles->at(i));
		int rows = dataset->size();
		int cols = dataset->at(0).size();

		//compute average feature
		std::vector<double> avgFeat(cols);
		for (int col = 0; col < cols; col++)
		{
			double avg = 0;
			for (int row = 0; row < rows; row++)
			{
				avg += dataset->at(row).at(col);
			}
			avgFeat[col] = avg / rows;
		}
		data->push_back(avgFeat);
	}

	//todo fix
	////normalize data
	//Eigen::MatrixXd normalized;
	//normalized.resize(data->size(), data->at(0).size());
	//for (int i = 0; i < data->size(); i++)
	//{
	//	Eigen::Map<Eigen::VectorXd> v(data->data()->data(), data[i].size());
	//	normalized.row(i) = v;
	//}
	//for (int i = 0; i < normalized.size(); i++)
	//{
	//	normalized.col(i).normalize();
	//}


	//mds

	DenseMatrix* embedding = new DenseMatrix;
	if (precomputedMDS)
	{
		QString fileName("C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"embedding.csv");
		readDistanceMatrixFromFile(embedding, fileName);
		printTapkeeDistanceMatrix(*embedding);
	}
	else
	{
		//mds
		TapkeeOutput output = tapkee::initialize()
								  .withParameters((method = MultidimensionalScaling, target_dimension = 2))
								  .withDistance(euclidianDistance)
								  .embedUsing(*data);
		*embedding = output.embedding.transpose();
		//todo delete after debugging
		QString outfileName("C:/Users/tonik/Documents/Antonia/Informatik "
		"Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
		"embedding.csv");
		printTapkeeOutput(output, outfileName);
		LOG(lvlInfo, QString("embedding written to '%1'").arg(outfileName));
	}

	//DenseSymmetricMatrix distanceMatrix =
	//	tapkee_internal::compute_distance_matrix(data->begin(), data->end(), euclidianDistance);	//ca 10 Min
	//printTapkeeDistanceMatrix(distanceMatrix);

	//DenseSymmetricMatrix* distanceMatrixFromFile;
	////todo: Run-Time Check Failure #3 - The variable 'distanceMatrixFromFile' is being used without being initialized.
	//readDistanceMatrixFromFile(distanceMatrixFromFile);

	QWidget* iaDistanceMatrixWidget = new iATimeCurvesWidget(*embedding, csvFiles);
	//
	////todo why take so long???
	m_mainWindow->addSubWindow(iaDistanceMatrixWidget);
	iaDistanceMatrixWidget->show();

	return false;
}


void iATimeCurves::printTapkeeOutput(TapkeeOutput output, QString fileName)
{
	Eigen::IOFormat CleanFmt(4, Eigen::DontAlignCols, ",", "\n", "", "");
	std::stringstream buffer;
	if (fileName != nullptr)
	{
		std::ofstream outFile(fileName.toStdString());
		outFile << output.embedding.transpose().format(CleanFmt);
		outFile.close();
		LOG(lvlInfo, QString("Feature vector written to '%1'").arg(fileName));
	}
	else
	{
		buffer << output.embedding.transpose().format(CleanFmt);
		std::string str = buffer.str();
		LOG(lvlInfo, QString("Output:"));
		LOG(lvlInfo, QString::fromStdString(str));
	}
}

void iATimeCurves::printTapkeeDistanceMatrix(DenseMatrix output)
{
	std::ofstream outFile("C:/Users/tonik/Documents/Antonia/Informatik Bachelor/22_23WiSe/Bachelorarbeit/ExampleData/"
						  "testmatrix.txt");
	Eigen::IOFormat CleanFmt(4, Eigen::DontAlignCols, ",", "\n", "", "");
	outFile << output.format(CleanFmt);
	outFile.close();
	LOG(lvlInfo, QString("Distance matrix written to testmatrix.txt"));
}


void iATimeCurves::readDistanceMatrixFromFile(DenseSymmetricMatrix* distanceMatrix, QString fileName)
{
	
	LOG(lvlDebug, QString("trying to read distance matrix from csv"));

	uint rows = 0;
	uint cols = 0;
	
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Error opening file '%1'.").arg(fileName));
	}
	QTextStream in(&file);
	QString curLine;
	std::vector<double> buff;

	while (!file.atEnd())
	{
		curLine = in.readLine();

		QStringList values = curLine.split(',');
		//todo trailing comma -> 1 col +
		cols = 0;
		for (const auto& value : values)
		{
			buff.push_back(value.toDouble());
			++cols;
		}
		rows++;
	}
	distanceMatrix->resize(rows, cols);
	*distanceMatrix = Eigen::Map<Eigen::MatrixXd>(&buff[0], rows, cols);
	
	LOG(lvlDebug, QString("Read file to distance matrix with '%1' rows and '%2' columns.").arg(rows).arg(cols));
	return;
}


void iATimeCurves::parseCsvAllToOne(std::vector<std::vector<double>>* data)
{
	LOG(lvlDebug, QString("begin parsing csvs."));

	uint rows = 0;
	uint cols;
	for (int i = 0; i < csvFiles->length(); i++)
	{
		QFile file(csvFiles->at(i));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			LOG(lvlError, QString("Error opening CSV file '%1'.").arg(csvFiles->at(0)));
		}
		QTextStream in(&file);
		QString curLine;
		std::vector<double> buff;
		int line = 0;

		while (!file.atEnd())
		{
			curLine = in.readLine();
			if (line > *headerLine)
			{
				QStringList values = curLine.split(',');
				//todo trailing comma -> 1 col +
				cols = 0;
				for (const auto& value : values)
				{
					buff.push_back(value.toDouble());
					++cols;
				}
			}
			//todo change to headerline length
			if (buff.size() == 15)
			{
				data->push_back(buff);
				rows++;
			}
			else
			{
				LOG(lvlInfo, QString("Row discarded, was not vector of 15 at file '%1', row '%2'").arg(i).arg(line));
			}
			buff.clear();
			line++;
		}
	}
	LOG(lvlDebug, QString("Parsed data to vector with '%1' rows and '%2' columns.").arg(rows).arg(data->at(0).size()));
	return;
}


bool iATimeCurves::mds()
{
	std::vector<std::vector<double>>* data = new std::vector<std::vector<double>>;
	EuclidianDistanceCallback euclidianDistance;

	//compute mds for each dataset
	for (int i = 0; i < csvFiles->length(); i++)
	{
		DenseMatrix* featureVector = new DenseMatrix;
		if (precomputedFVs)
		{
			QString outfileName(csvFiles->at(i) + "_fv.txt");
			readDistanceMatrixFromFile(featureVector, outfileName);
			printTapkeeDistanceMatrix(*featureVector);
		}
		else
		{
			//parse
			std::vector<std::vector<double>>* dataset = parseCsvToStdVector(csvFiles->at(i));
			//mds
			TapkeeOutput output = tapkee::initialize()
									  .withParameters((method = MultidimensionalScaling, target_dimension = 1))
									  .withDistance(euclidianDistance)
									  .embedUsing(*dataset);
			//todo resize?
			*featureVector = output.embedding.transpose();
			//todo delete after debugging
			QString outfileName(csvFiles->at(i) + "_fv.txt");
			printTapkeeOutput(output, outfileName);
		}

		//append
		//printTapkeeDistanceMatrix(toutput);
		std::vector<double> vec(featureVector->data(), featureVector->data() + featureVector->size());
		data->push_back(vec);
	}

	TapkeeOutput output = tapkee::initialize()
							  .withParameters((method = MultidimensionalScaling, target_dimension = 2))
							  .withDistance(euclidianDistance)
							  .embedUsing(*data);

	printTapkeeOutput(output, nullptr);

	//DenseSymmetricMatrix distanceMatrix =
	//	tapkee_internal::compute_distance_matrix(data->begin(), data->end(), euclidianDistance);	//ca 10 Min
	//printTapkeeDistanceMatrix(distanceMatrix);

	//DenseSymmetricMatrix* distanceMatrixFromFile;
	////todo: Run-Time Check Failure #3 - The variable 'distanceMatrixFromFile' is being used without being initialized.
	//readDistanceMatrixFromFile(distanceMatrixFromFile);

	//QWidget* iaDistanceMatrixWidget = new iATimeCurvesWidget(distanceMatrix);
	//
	////todo why take so long???
	//m_mainWindow->addSubWindow(iaDistanceMatrixWidget);
	//iaDistanceMatrixWidget->show();

	return false;
}



struct ExampleDistanceCallback
{
	ScalarType distance(IndexType l, IndexType r)
	{
		return abs(l - r);
	}
};


void iATimeCurves::mdsExample()
{
	//example start
	
	const int N = 100;
	std::vector<IndexType> indices(N);
	for (int i = 0; i < N; i++) indices[i] = i;

	ExampleDistanceCallback distance_callback;

	TapkeeOutput exampleOutput =
		tapkee::initialize()
		.withParameters((method = MultidimensionalScaling, target_dimension = 1))
		.withDistance(distance_callback)
		.embedUsing(indices);

	printTapkeeOutput(exampleOutput, nullptr);
	
	//example end
}