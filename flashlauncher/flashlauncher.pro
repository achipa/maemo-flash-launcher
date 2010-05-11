#-------------------------------------------------
#
# Project created by QtCreator 2010-05-04T20:52:45
#
#-------------------------------------------------

QT       += core gui webkit network

TARGET = flashlauncher
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    appdetail.cpp \
    simplefetch.cpp \
    launcher.cpp \
    mainwindowline.cpp

HEADERS  += mainwindow.h \
    appdetail.h \
    simplefetch.h \
    launcher.h \
    mainwindowline.h

FORMS    += mainwindow.ui \
    appdetail.ui \
    mainwindowline.ui

CONFIG += qdbus # mobility
MOBILITY = 

symbian {
    TARGET.UID3 = 0xebaacee4
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}
maemo5 {
    message("Maemo5 build")
    QT+= maemo5
}
unix {
    # VARIABLES
    isEmpty(PREFIX):PREFIX = /usr # /local ?
    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    DEFINES += DATADIR=\"$$DATADIR\" \
        PKGDATADIR=\"$$PKGDATADIR\"
    contains(QT_CONFIG, hildon)::DEFINES += CHIMGDIR=\'\"$$DATADIR/$${TARGET}\"\'

    # MAKE INSTALL
    INSTALLS += target \
        imagery \
        appconf \
        desktop \
        iconxpm \
        icon26 \
        icon40 \
        icon64
    target.path = $$BINDIR
    imagery.path = $$DATADIR/$${TARGET}/images
    imagery.files += ../flashlauncher/images/*png
    desktop.path = $$DATADIR/applications/hildon
    desktop.files += $${TARGET}.desktop
    iconxpm.path = $$DATADIR/pixmap
    iconxpm.files += ../data/maemo/$${TARGET}.xpm
    icon26.path = $$DATADIR/icons/hicolor/26x26/apps
    icon26.files += ../data/26x26/$${TARGET}.png
    icon40.path = $$DATADIR/icons/hicolor/40x40/apps
    icon40.files += ../data/40x40/$${TARGET}.png
    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += ../data/64x64/$${TARGET}.png
    appconf.path = $$DATADIR/flashlauncher
    appconf.files += ../flashlauncher/applications.conf
}

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    applications.conf
