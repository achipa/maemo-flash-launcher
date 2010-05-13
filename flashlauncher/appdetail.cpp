#include "appdetail.h"
#include "ui_appdetail.h"

#include "simplefetch.h"
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtDBus/QDBusInterface>
#include <QtCore/QDir>
#include <QtGui/QClipboard>
#ifdef Q_WS_MAEMO_5
    #include <QtMaemo5/QMaemo5InformationBox>
#endif
#include "qdebug.h"

#define MAIL "mailto:flashlauncher@csipa.in.rs?subject=FlashLauncher app config&body="

AppDetail::AppDetail(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AppDetail)
{
    ui->setupUi(this);
    connect(ui->actionCopy_config_to_clipboard, SIGNAL(triggered()), this, SLOT(setClipboard()));
    connect(ui->actionSend_config_via_email, SIGNAL(triggered()), this, SLOT(sendConfigMail()));
    connect(ui->actionHide, SIGNAL(triggered()), this, SLOT(toggleHide()));
    connect(ui->actionDelete, SIGNAL(triggered()), this, SLOT(delApp()));
}

AppDetail::~AppDetail()
{
    delete ui;
}

void AppDetail::loadLabels(QString cfgfile, QString groupname)
{
    appname = groupname;
    conffile = cfgfile;
    QSettings settings(cfgfile, QSettings::IniFormat);
    settings.beginGroup(appname);
    if (cfgfile.startsWith("/usr"))   // can't delete apps installed by the system
        ui->actionDelete->setVisible(false);

    if (settings.value("hidden","0").toInt() > 0)
        ui->actionHide->setChecked(true);

    ui->desc->setText(settings.value("description","").toString());
    this->setWindowTitle(settings.value("name", appname).toString());
    ui->instructions->setText(settings.value("instructions","Have fun !").toString());
    ui->imgpic->setGeometry(0, 0, 128, 128);
    if (!settings.value("image").toString().isEmpty())
    {
        SimpleFetch * sf = new SimpleFetch(settings.value("image").toString());
        connect(sf, SIGNAL(content(QByteArray)), this, SLOT(setImage(QByteArray)));
        connect(sf, SIGNAL(content(QByteArray)), sf, SLOT(deleteLater())); // delete simplefetch after desc set
    }
    ui->engine->setCurrentIndex(settings.value("engine").toInt()-1);
    QString qual = settings.value("quality").toString();
    if (qual == "high" || ui->engine->currentIndex() == 1 || ui->engine->currentIndex() == 3)
        ui->qualityCB->setCurrentIndex(0);
    else if (qual == "medium")
        ui->qualityCB->setCurrentIndex(1);
    else
        ui->qualityCB->setCurrentIndex(2);

    connect(ui->okB, SIGNAL(clicked()), this, SLOT(launch()));
    connect(ui->webB, SIGNAL(clicked()), this, SLOT(home()));
}

void AppDetail::delApp()
{
    QSettings settings(conffile, QSettings::IniFormat);
    settings.remove(appname);
    settings.sync();
    close();
    emit visibilityChanged();
}

void AppDetail::toggleHide()
{
    QSettings settings(conffile, QSettings::IniFormat);
    settings.beginGroup(appname);
    settings.setValue("hidden", ui->actionHide->isChecked() ? "1" : "0");
    settings.endGroup();
    settings.sync();
    close();
    emit visibilityChanged();
}

QString AppDetail::getConfig()
{
    QSettings settings(conffile, QSettings::IniFormat);
    settings.beginGroup(appname);
    QString cfgstr = QString("[%1]\nname=%2\ndescription=%3\ninstructions=%4\nfullscreen=%5\nsize=%6\nimage=%7\nswf=%8\nbase=%9\n").arg(
    appname,settings.value("description").toString(), settings.value("instructions").toString(), settings.value("fullscreen", "1").toString(), settings.value("size", "? ").toString(), settings.value("image").toString(), settings.value("swf").toString(), settings.value("base").toString());
    cfgstr += QString("engine=%1\nquality=%2\nportrait=%3").arg(settings.value("engine","1").toString(), settings.value("quality", "low").toString(), settings.value("portrait","0").toString());
    settings.endGroup();
    return cfgstr;
}

void AppDetail::launch()
{
    QSettings settings(conffile, QSettings::IniFormat);
    settings.beginGroup(appname);
    QStringList args;
    args << QString("%1").arg(ui->engine->currentIndex()+1) << appname;
    QProcess::execute(qApp->applicationFilePath(), args);
    settings.endGroup();
}

void AppDetail::setClipboard()
{
    qApp->clipboard()->setText(getConfig());
#ifdef Q_WS_MAEMO_5
    QMaemo5InformationBox::information ( this, tr("Config copied to clipboard"));
#endif

}

void AppDetail::sendConfigMail()
{
//    dbus-send --print-reply --type=method_call --dest=com.nokia.modest /com/nokia/modest com.nokia.modest.MailTo string:mailto:
    QDBusInterface browser("com.nokia.modest", "/com/nokia/modest", "com.nokia.modest")  ;
    browser.call("MailTo", QString(MAIL)+getConfig());
}

void AppDetail::home()
{
    QSettings settings(conffile, QSettings::IniFormat);
    settings.beginGroup(appname);
    QDBusInterface browser("com.nokia.osso_browser", "/com/nokia/osso_browser/request", "com.nokia.osso_browser")  ;
    browser.call("open_new_window", settings.value("base").toString());
}

void AppDetail::setImage(QByteArray ba)
{
    QPixmap p;
    p.loadFromData(ba);
    if (p.size().isEmpty())
        qDebug () << ba;
    else
        ui->imgpic->setPixmap(QPixmap(p).scaled(128, 128));
}
void AppDetail::changeEvent(QEvent *e)
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
