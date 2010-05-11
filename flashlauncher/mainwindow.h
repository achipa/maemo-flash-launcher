#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

#include "mainwindowline.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void about();
protected:
    void changeEvent(QEvent *e);

private slots:
    void updateSettings();
    void email();
    void support();
private:
    Ui::MainWindow *ui;
    QVector<MainWindowLine*> mwlines;
};

#endif // MAINWINDOW_H
