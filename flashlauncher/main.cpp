#include <QtGui/QApplication>
#include <QtCore/QTextStream>
#include "mainwindow.h"
#include "launcher.h"
#include "qdebug.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextStream out(stdout);
    out << "FlashLauncher, Copyright (C) 2010 by Attila Csipa" << endl;
    QStringList args = a.arguments();
    qDebug() << args;
    if (args.length() > 2) {
        launcher(args);
    } else {
        qDebug() << "Starting in GUI mode\n";
        MainWindow w;

#if defined(Q_WS_S60)
        w.showMaximized();
#else
        w.show();
#endif
        w.showTip();
        return a.exec();
    }
}
