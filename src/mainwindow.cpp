#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButtonFindGMKFile_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("GMK file"), QString(), "*.gmk");
    if (fileName.isEmpty())
    {
        return;
    }

    ui->lineEditGMKFile->setText(fileName);
}

void MainWindow::on_pushButtonFindGMS2Folder_clicked()
{
    const QString dirName = QFileDialog::getExistingDirectory(this, tr("GMS2 Project directory"), QString());
    if (dirName.isEmpty())
    {
        return;
    }

    ui->lineEditGMS2Folder->setText(dirName);
}

void MainWindow::on_pushButtonBreakToExitConvert_clicked()
{
    showNotes(Converter::breakToExit(ui->lineEditGMS2Folder->text()));
}

void MainWindow::showNotes(const QList<Converter::Note> &notes)
{
    for (const Converter::Note& note : qAsConst(notes))
    {
        switch (note.type)
        {
        case Converter::Note::Error:
            QMessageBox::critical(this, tr("Error"), note.text);
            break;

        default:
            QMessageBox::information(this, tr("Note"), note.text);
            break;
        }
    }

    if (notes.isEmpty())
    {
        QMessageBox::information(this, tr("Note"), tr("Done"));
    }
}

