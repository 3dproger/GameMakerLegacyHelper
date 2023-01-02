#include "logwindow.h"
#include "ui_logwindow.h"

LogWindow::LogWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogWindow)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog
                   | Qt::WindowTitleHint
                   | Qt::WindowMinimizeButtonHint
                   | Qt::WindowMaximizeButtonHint
                   | Qt::WindowCloseButtonHint);
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

