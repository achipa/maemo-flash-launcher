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
    void loadLabels(QString cfgfile, QString groupname);
    QString getConfig();
Q_SIGNAL
    void visibilityChanged();
public slots:
    void setImage(QByteArray ba);
    void launch();
    void home();
    void edit();

protected:
    void changeEvent(QEvent *e);

private slots:
    void setClipboard();
    void sendConfigMail();
    void delApp();
    void toggleHide();

private:
    QString appname;
    QString conffile;
    Ui::AppDetail *ui;
};

#endif // APPDETAIL_H
