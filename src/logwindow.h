#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QDialog>

namespace Ui {
class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();
    void clear();
    void addLine(const QString& line);
    bool isEmpty() const;

private slots:
    void on_pushButtonOk_clicked();

private:
    Ui::LogWindow *ui;
};

#endif // LOGWINDOW_H
