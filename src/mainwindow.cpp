#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gms1corrector.h"
#include "gms2corrector.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    GMS1Corrector::setLogCallback([this](const QString& text)
    {
        log->addLine(text);
    });

    GMS2Corrector::setLogCallback([this](const QString& text)
    {
        log->addLine(text);
    });
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

void MainWindow::on_pushButtonFindGMS1Folder_clicked()
{
    const QString dirName = QFileDialog::getExistingDirectory(this, tr("GMS1 Project directory"), QString());
    if (dirName.isEmpty())
    {
        return;
    }

    ui->lineEditGMS1Folder->setText(dirName);
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

void MainWindow::on_pushButtonBreakToExitCorrect_clicked()
{
    log->clear();
    GMS2Corrector::breakToExit(ui->lineEditGMS2Folder->text());
    log->show();
}

void MainWindow::on_pushButtonCorrectFunctions_clicked()
{
    log->clear();

    if (ui->checkBoxWindowCaption->isChecked())
    {
        GMS2Corrector::replace(ui->lineEditGMS2Folder->text(), "window_set_taskbar_caption(", "window_set_caption(");
    }

    if (ui->checkBoxVariableInstanceExists->isChecked())
    {
        GMS2Corrector::replace(ui->lineEditGMS2Folder->text(), "variable_local_exists(", "variable_instance_exists(id, ");
    }

    if (ui->checkBoxDisplayReset->isChecked())
    {
        GMS2Corrector::replace(ui->lineEditGMS2Folder->text(), "display_reset()", "display_reset(0, false)");
    }

    if (ui->checkBoxDisplaySetSize->isChecked())
    {
        GMS2Corrector::replace(ui->lineEditGMS2Folder->text(), "display_set_size(", "display_set_gui_size(");
    }

    if (log->isEmpty())
    {
        log->addLine(tr("Nothing changed"));
    }

    log->show();
}

void MainWindow::on_pushButtonCorrectTextEncoding_clicked()
{
    log->clear();
    GMS1Corrector::convertAnsiToUtf8(ui->lineEditGMKFile->text(), ui->lineEditGMS1Folder->text());
    log->show();
}

