#include "mainwindowline.h"
#include "ui_mainwindowline.h"

#include <QtCore/QSettings>
#include "simplefetch.h"
#include "appdetail.h"
#include "qdebug.h"


#define SETTINGSPATH "/usr/share/flashlauncher/applications.conf"

MainWindowLine::MainWindowLine(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindowLine)
{
    ui->setupUi(this);
}

MainWindowLine::~MainWindowLine()
{
    delete ui;
}

void MainWindowLine::loadLabels(QString groupname)
{
    appname = groupname;
    QSettings settings(SETTINGSPATH, QSettings::IniFormat);
    settings.beginGroup(groupname);
    ui->desc->setText(settings.value("description","").toString());
    ui->name->setText(settings.value("name", appname).toString());
    ui->kbsize->setText(settings.value("size", appname).toString()+QString("KiB"));
    ui->img->setGeometry(0, 0, 64, 64);
    if (!settings.value("image").toString().isEmpty())
    {
        SimpleFetch * sf = new SimpleFetch(settings.value("image").toString());
        connect(sf, SIGNAL(content(QByteArray)), this, SLOT(setImage(QByteArray)));
        connect(sf, SIGNAL(content(QByteArray)), sf, SLOT(deleteLater())); // delete simplefetch after desc set
    }
}

void MainWindowLine::setImage(QByteArray ba)
{
    QPixmap p;
    p.loadFromData(ba);
    if (p.size().isEmpty())
        qDebug () << ba;
    else
        ui->img->setPixmap(QPixmap(p).scaled(64, 64));
}

void MainWindowLine::mouseReleaseEvent(QMouseEvent * event)
{
#ifdef Q_WS_MAEMO_5
    if (scroller && (scroller->state() == QAbstractKineticScroller::Pushing || scroller->state() == QAbstractKineticScroller::MousePressed))
        return;
    qDebug() << "Scrollstate" << scroller->state() << event;
#endif

    qDebug() << appname << " details";
//    AppDetail* ad = new AppDetail(qobject_cast<QMainWindow*>(this->parent()));
    AppDetail* ad = new AppDetail( qobject_cast<QMainWindow*>(qApp->activeWindow()));
    ad->loadLabels(appname);
#ifdef Q_WS_MAEMO_5
    ad->setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
    ad->setWindowFlags(ad->windowFlags() | Qt::Window);
    ad->show();
}

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
