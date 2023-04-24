#include "InitDialog.h"
#include "qfiledialog.h"
#include "iACsvQTableCreator.h"

#include <iALog.h>

InitDialog::InitDialog(QStringList* csvFiles)
	: QDialog()
{
	headerAt = 0;
	csvFiles = &fileNames;
	ui.setupUi(this);
}

InitDialog::~InitDialog()
{
}

void InitDialog::on_linePathEdit_returnPressed()
{
	fileDialog(ui.linePathEdit->text());
}

void InitDialog::on_chooseFileButton_clicked()
{
	fileDialog(QString());
}

void InitDialog::fileDialog(QString path)
{
	QFileDialog dialog(this);
	if (!path.isEmpty())
	{
		dialog.setDirectory(path);
	}
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setNameFilter(tr("CSVs (*csv)"));
	
	if (dialog.exec())
	{
		fileNames = dialog.selectedFiles();
		//todo fix bug on "cancel"
	}
	ui.linePathEdit->setText(fileNames.first());

	//todo show file in table
	

}

void InitDialog::displayData()
{
	//this method shows the csv content in a table

	//iACsvQTableCreator creator = new iACsvQTableCreator();

	 
}

void InitDialog::accept()
{
	if (!validateData())
	{
		done(0);
	}
	else
	{
		done(1);
	}
}

void InitDialog::on_headerAtLine_valueChanged()
{
	headerAt = ui.headerAtLine->value();
}

//todo check if header matches var count
bool InitDialog::validateData()
{
	QString header;
	for (int i = 0; i < fileNames.size(); i++)
	{
		QFile file(fileNames.at(i));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			LOG(lvlError, QString("Error opening CSV file '%1'.").arg(fileNames.at(i)));
			return false;
		}

		QTextStream in(&file);
		int curRow = 0;
		QString curLine;
		while (!file.atEnd() && curRow <= headerAt)
		{
			curLine = in.readLine();
			curRow++;
		}
		if (i == 0)  //set header for first dataset
		{
			header = curLine;
			LOG(lvlDebug, QString("Header is: '%1").arg(header));
		}
		else
		{
			if (header != curLine)
			{
				LOG(lvlError, QString("Error: Header not equal in CSV file '%1'.").arg(fileNames.at(i)));
				return false;
			}
		}
	}
	return true;
}
