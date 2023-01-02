#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

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
