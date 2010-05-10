#ifndef MAINWINDOWLINE_H
#define MAINWINDOWLINE_H

#include <QtGui/QWidget>

namespace Ui {
    class MainWindowLine;
}

class MainWindowLine : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindowLine(QWidget *parent = 0);
    void loadLabels(QString groupname);
    ~MainWindowLine();

public slots:
    void setImage(QByteArray ba);

protected:
    void changeEvent(QEvent *e);

private:
    void mouseReleaseEvent(QMouseEvent * event);
    QString appname;
    Ui::MainWindowLine *ui;
};

#endif // MAINWINDOWLINE_H
