#ifndef SIMPLEFETCH_H
#define SIMPLEFETCH_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextStream>
#include <QSemaphore>
#include <QSettings>

class SimpleFetch : public QObject
{
Q_OBJECT
private:
    QNetworkAccessManager* manager;
    QSettings settings;
    QString url;
    int redirects;
public:
    SimpleFetch(QString url = "");
    ~SimpleFetch();
    QSemaphore sema;
    QNetworkReply* response;
signals:
    void content(QByteArray);
private slots:
    void loader(QNetworkReply* reply);
};

#endif // SIMPLEFETCH_H
