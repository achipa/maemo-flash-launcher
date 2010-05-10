#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qdebug.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << "ahoj";
    ui->setupUi(this);
    qDebug() << "poplacek";
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
#ifdef Q_WS_MAEMO_5
    ui->scrollArea->setProperty("FingerScrollable", true);
#endif
    QMessageBox::about(this, "FlashLauncher", "FlashLauncher\nCopyright (C) 2010 by Attila Csipa");

}

void MainWindow::about()
{
    QMessageBox::about(this, "FlashLauncher", "FlashLauncher\nCopyright (C) 2010 by Attila Csipa");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
