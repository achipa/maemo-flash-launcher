#include <QtGui/QApplication>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtCore/QTemporaryFile>
#include <QtCore/QSettings>
#include <QtWebKit/QWebView>
#include <QtDBus/QDBusInterface>
#include <qdebug.h>
#include "mainwindow.h"

#define WEBKIT 1
#define WEBKITBL 2
#define MICROB 3
#define MICROBBL 4
#define KMPLAYER 5

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextStream out(stdout);
    out << "FlashLauncher, Copyright (C) 2010 by Attila Csipa\n";
    QStringList args = a.arguments();
    qDebug() << args;
    if (args.length() > 2) {
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
            if (engine == WEBKIT) {
                QWebView* qwv = new QWebView();
                connect(qwv, SIGNAL(destroyed()), qApp, SLOT(quit()));
                qwv->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
//                qwv->load(QUrl("http://www.bubblebox.com"));
//                qwv->load(settings.value("swf").toUrl());
                qwv->load(QUrl(settings.value("swf").toString()));
                if (forcefullscreen || localsettings.value("fullscreen", settings.value("fullscreen", 1)).toBool())
                    qwv->showFullScreen();
                qwv->show();
            } else if (engine == WEBKITBL) {
                QWebView* qwv = new QWebView();
                connect(qwv, SIGNAL(destroyed()), qApp, SLOT(quit()));
                QString base = settings.value("base", "").toString();
                qwv->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
                html = settings.value("html", html).toString();
                html.replace("$QUALITY$",localsettings.value("quality", settings.value("quality", "low")).toString());
                html.replace("$SWF$",settings.value("swf").toString());
                html.replace("$BASE$",base);
                qDebug () << html << html.length() << base;
                qwv->setHtml(html, base);
                if (forcefullscreen || localsettings.value("fullscreen", settings.value("fullscreen", 1)).toBool())
                    qwv->showFullScreen();
                qwv->show();
            } else if (engine == MICROB) {
                QDBusInterface browser("com.nokia.osso_browser", "/com/nokia/osso_browser/request", "com.nokia.osso_browser")  ;
                browser.call("open_new_window", settings.value("swf").toString());
                return;
            } else if (engine == MICROBBL) {
                html = settings.value("html", html).toString();
                QString base = settings.value("base", "").toString();
                html.replace("$QUALITY$",settings.value("quality").toString());
                html.replace("$SWF$",settings.value("swf").toString());
                html.replace("$BASE$",base);
                QTemporaryFile tempfile;
                if (tempfile.open()) {
                    tempfile.write(html.toAscii());
                    tempfile.close();
                    QDBusInterface browser("com.nokia.osso_browser", "/com/nokia/osso_browser/request", "com.nokia.osso_browser")  ;
                    browser.call("open_new_window", tempfile.fileName());
                }
            } else if (engine == KMPLAYER) {
                // /opt/kmplayer/bin/knpplayer -m application/x-shockwave-flash -p /usr/lib/browser/plugins/libflashplayer.so file://[file] -wid id --args arg0=val0
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
    } else {
        qDebug() << "Starting in GUI mode\n";
        MainWindow w;
#if defined(Q_WS_S60)
        w.showMaximized();
#else
        w.show();
#endif
    }
    return a.exec();
}
