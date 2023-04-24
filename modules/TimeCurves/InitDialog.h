#pragma once

#include <QDialog>
#include <qerrormessage.h>
#include "ui_InitDialog.h"

//#include "istream.h"

class InitDialog : public QDialog
{
	Q_OBJECT

public:
	InitDialog(QStringList* csvFiles);
	~InitDialog();

private:
	Ui::InitDialogClass ui;
	QStringList fileNames;
	int headerAt;

	private slots:
		void on_chooseFileButton_clicked();
		void on_linePathEdit_returnPressed();
		void fileDialog(QString path);
		void displayData();
		void accept();

		void on_headerAtLine_valueChanged();

		//Validates the given data. Checks if the header is present in all files.
		bool validateData();
};
