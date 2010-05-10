#include "simplefetch.h"

#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QSemaphore>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>

#include "qdebug.h"

SimpleFetch::SimpleFetch(QString uri) :
    manager(new QNetworkAccessManager(this)),
    settings("flashlauncher", "http"),
    url(uri),
    redirects(0)
{
    qDebug() << settings.fileName();
    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(QFileInfo(settings.fileName()).absolutePath() + "/cacheDir");
    manager->setCache(diskCache);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(loader(QNetworkReply*)));
    QNetworkRequest request(QUrl(url.toAscii()));
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    qDebug() << "getting" <<url ;
    response = manager->get(request);
}

void SimpleFetch::loader(QNetworkReply* reply)
{
    QVariant fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute);
    qDebug() << "page from cache?" << fromCache.toBool();
    response = reply;
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    qDebug() << possibleRedirectUrl.toString();
    QByteArray data(response->readAll());
    /* If the URL is not empty, we're being redirected. */
    if(!possibleRedirectUrl.toString().isEmpty()) {
        /* We'll do another request to the redirection url. */
        redirects++;
        if (redirects > 5) {
            qDebug() << "Redirect treshold exceeded" ;
            return;
        }
        qDebug() << "smart redirected to " << possibleRedirectUrl.toString();
        manager->get(QNetworkRequest(possibleRedirectUrl.toUrl()));
    } else if (QString(data).startsWith("You are being redirected")) { // super kludgy workaround for 4.5.3-maemo4 not detecting redirects
        QRegExp re("<a href=\"(.*)\">");
        re.indexIn(QString(data));
        QStringList matches(re.capturedTexts());
        if (matches.length() > 1) {
            redirects++;
            if (redirects > 5) {
                qDebug() << "Redirect treshold exceeded" ;
                return;
            }
            manager->get(QNetworkRequest(QUrl(matches[1])));
        }
        qDebug() << "dumb redirected to " << matches[1];
    } else
        emit content(data);

}

SimpleFetch::~SimpleFetch()
{
    delete manager;
}
