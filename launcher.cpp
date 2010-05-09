#include "launcher.h"

#include <QtCore/QThread>
#include <QtCore/QProcess>
#include <QtCore/QUrl>
#include <QtCore/QTemporaryFile>
#include <QtCore/QSettings>
#include <QtWebKit/QWebView>
#include <QtDBus/QDBusInterface>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QMessageBox>
#include <qdebug.h>

#define WEBKIT 1
#define WEBKITBL 2
#define MICROB 3
#define MICROBBL 4
#define KMPLAYER 5

#define KNPPLAYERPATH "/opt/kmplayer/bin/knpplayer"
#define FLASHPLUGINPATH "/usr/lib/browser/plugins/libflashplayer.so"

int launcher(QStringList args)
{
        QSettings settings("applications.conf", QSettings::IniFormat);
        QSettings localsettings("flashlauncher", "applications");
        int engine = args.at(1).toInt();
        QString appname = args.at(2);
        QString html = settings.value("global/html").toString();
        bool forcefullscreen = settings.value("global/fullscreen").toBool();
        settings.beginGroup(appname);
        localsettings.beginGroup(appname);
        if (settings.allKeys().length() || localsettings.allKeys().length()) {
            engine = engine ? engine : localsettings.value("engine", settings.value("engine", 1)).toInt();
            qDebug () << "Launching " << appname << "(" << settings.value("swf").toString() << ") in engine " << engine;

            if (engine == WEBKIT || engine == WEBKITBL) {
                QWebView* qwv = new QWebView();
                qwv->connect(qwv, SIGNAL(destroyed()), qApp, SLOT(quit()));
                qwv->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
//                qwv->load(QUrl("http://www.bubblebox.com"));
//                qwv->load(settings.value("swf").toUrl());
                if (engine == WEBKITBL) {
                    QString base = settings.value("base", "").toString();
                    html = settings.value("html", html).toString();
                    html.replace("$QUALITY$",localsettings.value("quality", settings.value("quality", "low")).toString());
                    html.replace("$SWF$",settings.value("swf").toString());
                    html.replace("$BASE$",base);
                    qDebug () << html << html.length() << base;
                    qwv->setHtml(html, base);
                } else
                    qwv->load(QUrl(settings.value("swf").toString()));
                if (forcefullscreen || localsettings.value("fullscreen", settings.value("fullscreen", 1)).toBool())
                    qwv->showFullScreen();
                qwv->show();

            } else if (engine == MICROB) {
                QDBusInterface browser("com.nokia.osso_browser", "/com/nokia/osso_browser/request", "com.nokia.osso_browser")  ;
                browser.call("open_new_window", settings.value("swf").toString());
                return 0;

            } else if (engine == MICROBBL) {
                html = settings.value("html", html).toString();
                QString base = settings.value("base", "").toString();
                html.replace("$QUALITY$",settings.value("quality").toString());
                html.replace("$SWF$",settings.value("swf").toString());
                html.replace("$BASE$",base);
                QTemporaryFile tempfile;
                if (tempfile.open()) {
                    tempfile.write(html.toAscii());
                    QDBusInterface browser("com.nokia.osso_browser", "/com/nokia/osso_browser/request", "com.nokia.osso_browser")  ;
                    browser.call("open_new_window", tempfile.fileName());
                    // ideally, we should wait until we get a DBUS signal that the page is loaded. Is there such a signal ?
                    sleep(3);
                    tempfile.close();
                    return 0;
                }
            } else if (engine == KMPLAYER) {
                // /opt/kmplayer/bin/knpplayer -m application/x-shockwave-flash -p /usr/lib/browser/plugins/libflashplayer.so file://[file] -wid id --args arg0=val0
                if (QFile::exists(KNPPLAYERPATH)) {
                    qDebug () << "Kmplayer found";
                    QStringList params;
                    // winId is not what it seems to be. Beware the segfault
//                    params << "-m" << "application/x-shockwave-flash" << "-p" << FLASHPLUGINPATH << localsettings.value("swf", settings.value("swf")).toString() << "-wid" << QString(int(qApp->activeWindow()->winId()));
                    params << "-m" << "application/x-shockwave-flash" << "-p" << FLASHPLUGINPATH << localsettings.value("swf", settings.value("swf")).toString() ;
                    qDebug() << params;
                    return QProcess::execute(KNPPLAYERPATH, params);
                } else {
                    qDebug () << "No kmplayer";
                    QDialog dummy;
                    QMessageBox::warning(&dummy, qApp->tr("KMPlayer not available"), qApp->tr("Could not find KMPlayer executable. Please check KMPlayer if is installed.")) ;
                    return 1;
                }
            }
        } else { // assume it's an url or swf file
            qDebug() << "Configuration for " << appname << " not found\n";
            QWebView* qwv = new QWebView();
            qwv->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
            qwv->load(args.at(1));
            qwv->show();
//        if (QFile::exists(args.at(2))) {            // argument is an existing file
//        } else if (QUrl(args.at(2)).isValid()){       // argument is a URL
        }
        return 0;
}
