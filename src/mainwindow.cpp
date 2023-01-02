#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "converter.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Converter::convert("D:/Projects/tnu4_engine_8_530_gms2");
}

MainWindow::~MainWindow()
{
    delete ui;
}

