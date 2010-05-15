#include "mainwindowline.h"
#include "ui_mainwindowline.h"

#include <QtCore/QSettings>
#include "simplefetch.h"
#include "qdebug.h"

#define DEFAULTSETTINGS "/usr/share/flashlauncher/applications.conf"
#define FLSET(x,y) localsettings.value(x, settings.value(x,y))

MainWindowLine::MainWindowLine(QWidget *parent) :
    QWidget(parent),
    configfile(DEFAULTSETTINGS),
    localapp(false),
    ui(new Ui::MainWindowLine)

{
    ad = new AppDetail();
    ui->setupUi(this);

    connect(ui->playB, SIGNAL(clicked()), this, SLOT(go()));
    connect(ui->infoB, SIGNAL(clicked()), this, SLOT(details()));
}

MainWindowLine::~MainWindowLine()
{
    delete ad;
    delete ui;
}

void MainWindowLine::loadLabels(QString cfgfile, QString groupname)
{
    configfile = cfgfile;
    appname = groupname;
    QSettings settings(configfile, QSettings::IniFormat);
    QSettings localsettings("flashlauncher", "applications");
    settings.beginGroup(groupname);
    localsettings.beginGroup(groupname);
    ui->desc->setText(FLSET("description","").toString());
    ui->name->setText(FLSET("name", appname).toString());
    ui->kbsize->setText(FLSET("size", "? ").toString()+QString("KiB"));
    ui->img->setGeometry(0, 0, 92, 92);
    if (!FLSET("image","").toString().isEmpty())
    {
        SimpleFetch * sf = new SimpleFetch(FLSET("image","").toString());
        connect(sf, SIGNAL(content(QByteArray)), this, SLOT(setImage(QByteArray)));
        connect(sf, SIGNAL(content(QByteArray)), sf, SLOT(deleteLater())); // delete simplefetch after desc set
    }
    if (FLSET("hidden","0").toInt() > 0)
    {
        qDebug() << appname << "is hidden";
        setVisible(false);
    }
    settings.endGroup();
    localsettings.endGroup();
}

void MainWindowLine::setImage(QByteArray ba)
{
    QPixmap p;
    p.loadFromData(ba);
    if (p.size().isEmpty())
        qDebug () << ba;
    else
        ui->img->setPixmap(QPixmap(p).scaled(92, 92));
}

void MainWindowLine::go()
{
}

void MainWindowLine::details()
{
    qDebug() << appname << " details";
//    AppDetail* ad = new AppDetail(qobject_cast<QMainWindow*>(this->parent()));

    ad->setParent(qobject_cast<QMainWindow*>(qApp->activeWindow()));
    ad->loadLabels(configfile, appname);
#ifdef Q_WS_MAEMO_5
    ad->setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
    ad->setWindowFlags(ad->windowFlags() | Qt::Window);
    ad->show();
}
/*
void MainWindowLine::mouseReleaseEvent(QMouseEvent * event)
{
#ifdef Q_WS_MAEMO_5
    if (scroller && (scroller->state() == QAbstractKineticScroller::Pushing || scroller->state() == QAbstractKineticScroller::MousePressed))
        return;
    qDebug() << "Scrollstate" << scroller->state() << event;
#endif

    qDebug() << appname << " details";
//    AppDetail* ad = new AppDetail(qobject_cast<QMainWindow*>(this->parent()));

    ad->setParent(qobject_cast<QMainWindow*>(qApp->activeWindow()));

    ad->loadLabels(configfile, appname);
#ifdef Q_WS_MAEMO_5
    ad->setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
    ad->setWindowFlags(ad->windowFlags() | Qt::Window);
    ad->show();
}
*/
void MainWindowLine::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
