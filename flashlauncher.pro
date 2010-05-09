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
    launcher.cpp

HEADERS  += mainwindow.h \
    appdetail.h \
    simplefetch.h \
    launcher.h

FORMS    += mainwindow.ui \
    appdetail.ui

CONFIG += mobility qdbus
MOBILITY = 

symbian {
    TARGET.UID3 = 0xebaacee4
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    applications.conf
