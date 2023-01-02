#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "converter.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonFindGMKFile_clicked();

    void on_pushButtonFindGMS2Folder_clicked();

    void on_pushButtonBreakToExitConvert_clicked();

private:
    Ui::MainWindow *ui;

    void showNotes(const QList<Converter::Note>& notes);
};
#endif // MAINWINDOW_H
