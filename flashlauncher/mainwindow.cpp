#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QSettings>
#include "qdebug.h"

#define SETTINGSPATH "/usr/share/flashlauncher/applications.conf"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
//#ifdef Q_WS_MAEMO_5
    ui->scrollArea->setProperty("FingerScrollable", true);
    ui->scrollArea->setProperty("FingerScrollBars", false);
//#endif
    QSettings settings(SETTINGSPATH, QSettings::IniFormat);
    QSettings localsettings("flashlauncher", "applications");
    MainWindowLine* mwline;
    foreach (QString group, settings.childGroups())
    {
        if (group != "global")
        {
            qDebug() << group;
            settings.beginGroup(group);
            mwline = new MainWindowLine(this);
            mwline->loadLabels(group);
            ui->verticalLayout_2->insertWidget(ui->verticalLayout_2->count()-1, mwline);
            settings.endGroup();
        }
    }
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
