#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QSettings>
#include <QtDBus/QDBusInterface>
#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>


#include "qdebug.h"
#ifdef Q_WS_MAEMO_5
    #include <QtMaemo5/QMaemo5InformationBox>
    #include <QtGui/QAbstractKineticScroller>
#endif
#include "addgame.h"
#include "addswfs.h"

#define SETTINGSPATH "/usr/share/flashlauncher"
#define TALKTHREAD "http://talk.maemo.org/showthread.php?t=52275"
#define MAIL "mailto:flashlauncher@csipa.in.rs?subject=Flash game/app inclusion request"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(QString(SETTINGSPATH)+"applications.conf", QSettings::IniFormat)
{
    ui->setupUi(this);
//    ui->centralWidget->setStyleSheet("QWidget { background-image: url(:/elem/background.png); }");
    connect(ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionMake_a_game_request, SIGNAL(triggered()), this, SLOT(email()));
    connect(ui->actionSupport_forum, SIGNAL(triggered()), this, SLOT(support()));
    connect(ui->actionDisplay_tips_on_startup, SIGNAL(triggered()), this, SLOT(updateSettings()));
    connect(ui->actionLaunch_fullscreen, SIGNAL(triggered()), this, SLOT(updateSettings()));
    connect(ui->actionAdd_game_manually, SIGNAL(triggered()), this, SLOT(addGame()));
    connect(ui->actionShow_hidden, SIGNAL(triggered()), this, SLOT(regenAppList()));
    connect(ui->actionImport_local_SWF, SIGNAL(triggered()), this, SLOT(importSWF()));

#ifdef Q_WS_MAEMO_5 // no harm if we put this in anyway... might be useful for Diablo folks
    setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
// no harm if we put this in anyway... might be useful for Diablo folks
    ui->scrollArea->setProperty("FingerScrollable", true);
    ui->scrollArea->setProperty("FingerScrollBars", false);
//#endif

    QSettings localsettings("flashlauncher", "applications");
    localsettings.beginGroup("global");
    ui->actionLaunch_fullscreen->setChecked(localsettings.value("fullscreen","1").toInt());
    ui->actionDisplay_tips_on_startup->setChecked(localsettings.value("tips","0").toInt()); // tips are off by default until PR1.2
    ui->actionShow_hidden->setChecked(localsettings.value("showhidden","0").toInt());
    localsettings.endGroup();
    updateSettings();
    regenAppList();
}

void MainWindow::regenAppList()
{
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator);
#endif

    // very very very very very very very dirty way of resetting the scrollarea, suggestions welcome

/*    ui->setupUi(this);
    ui->scrollArea->setProperty("FingerScrollable", true);
    ui->scrollArea->setProperty("FingerScrollBars", false);
*/
    while (ui->verticalLayout_2->count() > 1)
        delete ui->verticalLayout_2->takeAt(0);


    QSettings localsettings("flashlauncher", "applications");
    MainWindowLine* mwline;
    QDir cfgDir(SETTINGSPATH);
    QStringList conflist = cfgDir.entryList (QStringList("*.conf"));
    conflist.append(localsettings.fileName());
    QStringList overridelist;
    foreach (QString conffile, conflist)
    {
        qDebug() << conffile;
        QSettings s(cfgDir.filePath(conffile), QSettings::IniFormat);
        foreach (QString group, s.childGroups())
        {
            if (!overridelist.contains(group) && group != "global")
            {
                qDebug() << group;
                s.beginGroup(group);
                mwline = new MainWindowLine(this);
#ifdef Q_WS_MAEMO_5
                mwline->setAttribute(Qt::WA_Maemo5StackedWindow);
                mwline->setScroller(ui->scrollArea->property("kineticScroller")
                                    .value<QAbstractKineticScroller *>());
#endif
                connect(mwline->ad, SIGNAL(visibilityChanged()), this, SLOT(regenAppList()));
                mwline->loadLabels(s.fileName(), group);
                if (ui->actionShow_hidden->isChecked())
                    mwline->setVisible(true);

                ui->verticalLayout_2->insertWidget(ui->verticalLayout_2->count()-1, mwline);
                s.endGroup();
                overridelist << group;
            }
        }
    }
//    ui->scrollArea->adjustSize();
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
}

void MainWindow::importSWF()
{
    AddSWFs as;
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
    as.setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
    qApp->processEvents();
#endif
    QStringList namefilters;
    namefilters << "*.swf" << "*.SWF" << "*.Swf";
    QDirIterator it("/home/user/MyDocs", namefilters, QDir::NoFilter, QDirIterator::Subdirectories);
    QStringList swflist;
    while (it.hasNext()) {
         QString fname = it.next();
         swflist << fname;
         as.addPath(fname);
         qApp->processEvents();
    }
    QDirIterator it2("/home/user/.macromedia/Flash_Player/#SharedObjects/", namefilters, QDir::NoFilter, QDirIterator::Subdirectories);
    while (it2.hasNext()) {
         QString fname = it2.next();
         swflist << fname;
         as.addPath(fname);
         qApp->processEvents();
    }

#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
    as.setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
    if (as.exec())
    {
        QSettings s("flashlauncher", "applications");
        foreach (QString selectedfname, as.getSelected())
        {
            QFileInfo fi(selectedfname);
            s.beginGroup(fi.baseName());
            s.setValue("name", fi.baseName());
            s.setValue("base", fi.absolutePath());
            s.setValue("swf", QString("file://") + selectedfname);
            s.setValue("size", fi.size()/1000);
            s.setValue("engine", "1");
            s.setValue("quality", "low");
            s.endGroup();
            s.sync();
        }
        regenAppList();
    }

}

void MainWindow::addGame()
{
    AddGame* ag = new AddGame(this);
#ifdef Q_WS_MAEMO_5
    ag->setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
    connect(ag, SIGNAL(gameAdded()), this, SLOT(regenAppList()));
    ag->show();
}

void MainWindow::showTip()
{
    if (ui->actionDisplay_tips_on_startup->isChecked()) {
        int entries = settings.beginReadArray("global/tip");
        if (entries == 0)
            return;
        settings.setArrayIndex(float(qrand())/RAND_MAX*entries);
#ifdef Q_WS_MAEMO_5
        QMaemo5InformationBox::information ( 0, tr("Tip of the day: ") + settings.value("text").toString(), QMaemo5InformationBox::NoTimeout);
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
    localsettings.setValue("showhidden", ui->actionShow_hidden->isChecked() ? "1" : "0");
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
