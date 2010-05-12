#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QSettings>
#include <QtDBus/QDBusInterface>
#include "qdebug.h"
#ifdef Q_WS_MAEMO_5
    #include <QtMaemo5/QMaemo5InformationBox>
    #include <QtGui/QAbstractKineticScroller>
#endif

#define SETTINGSPATH "/usr/share/flashlauncher/applications.conf"
#define TALKTHREAD "http://talk.maemo.org/showthread.php?t=52275"
#define MAIL "mailto:flashlauncher@csipa.in.rs?subject=Flash game/app inclusion request"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionMake_a_game_request, SIGNAL(triggered()), this, SLOT(email()));
    connect(ui->actionSupport_forum, SIGNAL(triggered()), this, SLOT(support()));
    connect(ui->actionDisplay_tips_on_startup, SIGNAL(triggered()), this, SLOT(updateSettings()));
    connect(ui->actionLaunch_fullscreen, SIGNAL(triggered()), this, SLOT(updateSettings()));

#ifdef Q_WS_MAEMO_5 // no harm if we put this in anyway... might be useful for Diablo folks
    setAttribute(Qt::WA_Maemo5StackedWindow);
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator);
#endif
// no harm if we put this in anyway... might be useful for Diablo folks
    ui->scrollArea->setProperty("FingerScrollable", true);
    ui->scrollArea->setProperty("FingerScrollBars", false);
//#endif

    QSettings localsettings("flashlauncher", "applications");
    localsettings.beginGroup("global");
    ui->actionLaunch_fullscreen->setChecked(localsettings.value("fullscreen","1").toInt());
    ui->actionDisplay_tips_on_startup->setChecked(localsettings.value("tips","1").toInt());
    localsettings.endGroup();
    updateSettings();


    QSettings settings(SETTINGSPATH, QSettings::IniFormat);
    MainWindowLine* mwline;
    foreach (QString group, settings.childGroups())
    {
        if (group != "global")
        {
            qDebug() << group;
            settings.beginGroup(group);
            mwline = new MainWindowLine(this);
#ifdef Q_WS_MAEMO_5
            mwline->setAttribute(Qt::WA_Maemo5StackedWindow);
            mwline->setScroller(ui->scrollArea->property("kineticScroller")
                                .value<QAbstractKineticScroller *>());
#endif
            mwline->loadLabels(group);
            ui->verticalLayout_2->insertWidget(ui->verticalLayout_2->count()-1, mwline);
            settings.endGroup();
        }
    }
    if (ui->actionDisplay_tips_on_startup->isChecked()) {
        int entries = settings.beginReadArray("global/tip");
        if (entries == 0)
            return;
        settings.setArrayIndex(qrand()/RAND_MAX*entries);
        qDebug() << settings.allKeys();
#ifdef Q_WS_MAEMO_5
        setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
        QMaemo5InformationBox::information ( 0,  settings.value("text").toString(), QMaemo5InformationBox::NoTimeout);
#else
        QMessageBox::information(this, "Tip of the day", settings.value("text").toString());
        ui->actionDisplay_tips_on_startup->setVisible(false);
#endif
        settings.endArray();
    }
}

void MainWindow::updateSettings()
{
    QSettings localsettings("flashlauncher", "applications");
    localsettings.beginGroup("global");
    localsettings.setValue("fullscreen", ui->actionLaunch_fullscreen->isChecked() ? "1" : "0");
    localsettings.setValue("tips", ui->actionDisplay_tips_on_startup->isChecked() ? "1" : "0");
    localsettings.endGroup();
    localsettings.sync();

}

void MainWindow::support()
{
//    dbus-send --system --type=method_call --dest="com.nokia.osso_browser" --print-reply /com/nokia/osso_browser/request com.nokia.osso_browser.load_url string:"google.com"
    QDBusInterface browser("com.nokia.osso_browser", "/com/nokia/osso_browser/request", "com.nokia.osso_browser")  ;
    browser.call("open_new_window", TALKTHREAD);
}

void MainWindow::email()
{
//    dbus-send --print-reply --type=method_call --dest=com.nokia.modest /com/nokia/modest com.nokia.modest.MailTo string:mailto:
    QDBusInterface browser("com.nokia.modest", "/com/nokia/modest", "com.nokia.modest")  ;
    browser.call("MailTo", MAIL);
}

void MainWindow::about()
{
    QMessageBox::about(this, "FlashLauncher", "FlashLauncher\nCopyright (C) 2010 by Attila Csipa");
}

MainWindow::~MainWindow()
{
    updateSettings();
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
