#ifndef APPDETAIL_H
#define APPDETAIL_H

#include <QMainWindow>

namespace Ui {
    class AppDetail;
}

class AppDetail : public QMainWindow
{
    Q_OBJECT

public:
    explicit AppDetail(QWidget *parent = 0);
    ~AppDetail();
    void loadLabels(QString groupname);

public slots:
    void setImage(QByteArray ba);
    void launch();
    void home();

protected:
    void changeEvent(QEvent *e);

private:
    QString appname;
    Ui::AppDetail *ui;
};

#endif // APPDETAIL_H
