#include "iAInitDialog.h"

#include <iALog.h>

//iAobjectvis
#include "iACsvQTableCreator.h"


//QT
#include "qfiledialog.h"
#include <QDragEnterEvent>
#include <qmimedata.h>

iAInitDialog::iAInitDialog(QStringList** csvFiles, int** headerLine, QString* name)
	: m_name(name), QDialog()
{
	headerAt = 0;
	*headerLine = &headerAt;
	*csvFiles = &fileNames;
	ui.setupUi(this);
	setAcceptDrops(true);
}

iAInitDialog::~iAInitDialog()
{
}

void iAInitDialog::on_linePathEdit_returnPressed()
{
	fileNames = ui.linePathEdit->text().split(",");
	displayData();
}

void iAInitDialog::on_nameEdit_editingFinished()
{
	*m_name = ui.nameEdit->text();
}

void iAInitDialog::on_chooseFileButton_clicked()
{
	fileDialog();
}

void iAInitDialog::fileDialog()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setNameFilter(tr("CSVs (*csv)"));
	
	if (dialog.exec())
	{
		if (dialog.selectedFiles().isEmpty())
		{
			return;
		}
		fileNames = dialog.selectedFiles();
		ui.linePathEdit->setText(fileNames.join(","));
		displayData();
	}
}

void iAInitDialog::on_tableWidget_cellClicked(int row, int column)
{
	headerAt = row;
	ui.headerAtLine->setValue(headerAt);
}

void iAInitDialog::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasFormat("text/uri-list"))
	{
		event->acceptProposedAction();
	}
}

void iAInitDialog::dropEvent(QDropEvent* event)
{
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
	{
		return;
	}
	fileNames.clear();
	for (QUrl url : urls)
	{
		if (url.fileName().split('.').last() != "csv")
		{
			LOG(lvlDebug, "Files have to be of type: csv.");
			return;
		}
		fileNames.append(url.toLocalFile());
	}
	ui.linePathEdit->setText(fileNames.join(","));
	displayData();
}

void iAInitDialog::displayData()
{
	QFile file(fileNames.at(0));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Error opening CSV file '%1'.").arg(fileNames.at(0)));
		return;
	}

	QTextStream in(&file);
	QString curLine;
	QTableWidget* tableWidget = ui.tableWidget;
	tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	uint row = 0;
	while (!file.atEnd())
	{
		tableWidget->insertRow(row);
		curLine = in.readLine();
		QStringList values = curLine.split(',');
		//todo trailing comma -> 1 col +
		if (values.size() > tableWidget->columnCount())
		{
			tableWidget->setColumnCount(values.size());
		}
		uint col = 0;
		for (const auto& value : values)
		{
			tableWidget->setItem(static_cast<int>(row), col, new QTableWidgetItem(value));
			++col;
		}
		row++;
	}
	//todo 27 rows missing??
	LOG(lvlDebug, QString("Created table with '%1' rows and '%2' columns.").arg(tableWidget->rowCount()).arg(tableWidget->columnCount()));
}

void iAInitDialog::accept()
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

void iAInitDialog::on_headerAtLine_valueChanged()
{
	headerAt = ui.headerAtLine->value();
	ui.tableWidget->selectRow(headerAt);
}

//todo check if header matches var count of data
bool iAInitDialog::validateData()
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
