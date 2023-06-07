#include "iADistanceMatrix.h"

#include<iALog.h>

//Qt
#include <QPushButton.h>

iADistanceMatrix::iADistanceMatrix(tapkee::DenseMatrix embedding, QWidget* parent) : QWidget(parent)
{
	ui.setupUi(this);
	this->embedding = embedding;
	connect(ui.saveButton, &QPushButton::clicked, this, &iADistanceMatrix::saveJson);
	populateTable();
}

void iADistanceMatrix::saveJson()
{
	return;
}

void iADistanceMatrix::populateTable()
{
	QTableWidget* tableWidget = ui.tableWidget;
	tableWidget->setRowCount(embedding.rows());
	tableWidget->setColumnCount(embedding.cols());
	for (int i = 0; i < embedding.rows(); i++)
	{
		for (int j = 0; j < embedding.cols(); j++)
		{
			double value = embedding(i, j);
			tableWidget->setItem(i, j, new QTableWidgetItem(value));
		}
	}
	LOG(lvlDebug, QString("Created table, first item is: '$1'").arg(tableWidget->itemAt(1,1)->text()));
}