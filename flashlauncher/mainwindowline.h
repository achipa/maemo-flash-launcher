#ifndef MAINWINDOWLINE_H
#define MAINWINDOWLINE_H

#include <QtGui/QWidget>
#include "appdetail.h"
#ifdef Q_WS_MAEMO_5
    #include <QtGui/QAbstractKineticScroller>
#endif

namespace Ui {
    class MainWindowLine;
}

class MainWindowLine : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindowLine(QWidget *parent = 0);
    void loadLabels(QString cfgfile, QString groupname);
    ~MainWindowLine();
    AppDetail* ad;
#ifdef Q_WS_MAEMO_5
    void setScroller(QAbstractKineticScroller *scr) { scroller = scr; }
#endif

public slots:
    void setImage(QByteArray ba);

protected:
    void changeEvent(QEvent *e);

private:
    void mouseReleaseEvent(QMouseEvent * event);
#ifdef Q_WS_MAEMO_5
    QAbstractKineticScroller *scroller;
#endif
    QString appname;
    QString configfile;
    bool localapp;
    Ui::MainWindowLine *ui;
};

#endif // MAINWINDOWLINE_H
