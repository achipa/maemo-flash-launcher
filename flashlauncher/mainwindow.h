#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QSettings>
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
    void showTip();
public slots:
    void about();
    void regenAppList();
protected:
    void changeEvent(QEvent *e);

private slots:
    void updateSettings();
    void email();
    void support();
    void addGame();
    void importSWF();
private:
    Ui::MainWindow *ui;
    QVector<MainWindowLine*> mwlines;
    QSettings settings;
};

#endif // MAINWINDOW_H
