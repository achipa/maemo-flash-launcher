#include "appdetail.h"
#include "ui_appdetail.h"

#include "simplefetch.h"
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtDBus/QDBusInterface>

#include "qdebug.h"

AppDetail::AppDetail(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AppDetail)
{
    ui->setupUi(this);
}

AppDetail::~AppDetail()
{
    delete ui;
}

void AppDetail::loadLabels(QString groupname)
{
    appname = groupname;
    QSettings settings("applications.conf", QSettings::IniFormat);
    settings.beginGroup(appname);
    ui->desc->setText(settings.value("description","").toString());
    this->setWindowTitle(settings.value("name", appname).toString());
    ui->instructions->setText(settings.value("instructions","Have fun !").toString());
    ui->imgpic->setGeometry(0, 0, 96, 96);
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

void AppDetail::launch()
{
    QSettings settings("applications.conf", QSettings::IniFormat);
    settings.beginGroup(appname);
    QStringList args;
    args << QString("%1").arg(ui->engine->currentIndex()+1) << appname;
    QProcess::execute(qApp->applicationFilePath(), args);
}

void AppDetail::home()
{
    QSettings settings("applications.conf", QSettings::IniFormat);
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
        ui->imgpic->setPixmap(QPixmap(p).scaled(96, 96));
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
