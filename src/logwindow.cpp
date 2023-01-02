#include "logwindow.h"
#include "ui_logwindow.h"

LogWindow::LogWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);
}

LogWindow::~LogWindow()
{
    delete ui;
}

void LogWindow::clear()
{
    ui->textBrowserLog->setText(QString());
}

void LogWindow::addLine(const QString &line)
{
    ui->textBrowserLog->append(line);
}

void LogWindow::on_pushButtonOk_clicked()
{
    close();
}

