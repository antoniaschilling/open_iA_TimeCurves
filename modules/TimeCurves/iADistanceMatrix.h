#pragma once

#include <qwidget.h>
#include "ui_iADistanceMatrix.h"
#include "tapkee/tapkee.hpp"



	class iADistanceMatrix : public QWidget
{
	Q_OBJECT

public:
	explicit iADistanceMatrix(tapkee::DenseMatrix embedding, QWidget* parent = nullptr);

private:
	Ui::DistanceMatrixClass ui;
	tapkee::DenseMatrix embedding;

private slots:
	void saveJson();
	void populateTable();
};

