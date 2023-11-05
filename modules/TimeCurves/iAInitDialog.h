#pragma once

#include <QDialog>
#include <qerrormessage.h>
#include "ui_InitDialog.h"

//#include "istream.h"

class iAInitDialog : public QDialog
{
	Q_OBJECT

public:
	iAInitDialog(QStringList** csvFiles, int** headerLine, QString* name, QString* selectedDistance);
	~iAInitDialog();

private:
	Ui::InitDialogClass ui;
	QStringList fileNames;
	int headerAt;
	QStringList headers;
	QString* m_name;
	QString* m_selectedDistance;
	QButtonGroup* buttonGroup;


	private slots:
		void on_chooseFileButton_clicked();
		void on_linePathEdit_returnPressed();
		void on_nameEdit_editingFinished();
		void fileDialog();
		void on_tableWidget_cellClicked(int row, int column);
		void dragEnterEvent(QDragEnterEvent *event);
		void dropEvent(QDropEvent* event);
		void distanceToggled(QAbstractButton* button, bool checked);

		//this method shows the csv content in a table
		void displayData();
		void accept();

		void on_headerAtLine_valueChanged();

		//Validates the given data. Checks if the header is present in all files.
		bool validateData();
};
