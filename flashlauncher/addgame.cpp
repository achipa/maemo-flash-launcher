#include "addgame.h"
#include "ui_addgame.h"

#include <QtCore/QSettings>
#include <QtGui/QFileDialog>
#ifdef Q_WS_MAEMO_5
    #include <QtMaemo5/QMaemo5InformationBox>
#endif

#define FLSET(x,y) localsettings.value(x, settings.value(x,y))

AddGame::AddGame(QWidget *parent) :
    QMainWindow(parent),
    gameid(""),
    ui(new Ui::AddGame)
{
    ui->setupUi(this);
    connect(ui->okB, SIGNAL(clicked()), this, SLOT(verifyFields()));
    connect(ui->fileB, SIGNAL(clicked()), this, SLOT(pickFileDialog()));
}

AddGame::~AddGame()
{
    delete ui;
}

void AddGame::edit(QString cfgfile, QString appname)
{
    gameid = appname;
    QSettings settings(cfgfile, QSettings::IniFormat);
    QSettings localsettings("flashlauncher", "applications");
    settings.beginGroup(appname);
    localsettings.beginGroup(appname);
    ui->nameE->setText(FLSET("name", appname).toString());
    ui->swfE->setText(FLSET("swf", "").toString());
    ui->baseE->setText(FLSET("base", "").toString());
    ui->descE->setText(FLSET("description", "").toString());
    ui->instrE->setText(FLSET("instructions", "").toString());
    ui->imageE->setText(FLSET("image", "").toString());
    ui->sizeSB->setValue(FLSET("size", 0).toInt());
    ui->engineCB->setCurrentIndex(FLSET("engine", 1).toInt() -1);
    if (FLSET("quality", "low").toString() == "low")
        ui->qualityCB->setCurrentIndex(2);
    else if (FLSET("quality","").toString() == "med")
        ui->qualityCB->setCurrentIndex(1);
    else
        ui->qualityCB->setCurrentIndex(0);
    settings.endGroup();
}

void AddGame::pickFileDialog()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "/home/user/MyDocs",
                                                    tr("Flash (*.swf)"));
    if (!filename.isEmpty())
        ui->swfE->setText(QString("file://") + filename);
}

void AddGame::verifyFields()
{
    if (ui->nameE->text().length() > 0 && ui->swfE->text().length() > 0 )
    {
        QSettings settings("flashlauncher", "applications");
        if (gameid.isEmpty()) // new games don't have ids
            settings.beginGroup(ui->nameE->text());
        else
            settings.beginGroup(gameid);
        settings.setValue("name", ui->nameE->text());
        settings.setValue("swf", ui->swfE->text());
        settings.setValue("base", ui->baseE->text());
        settings.setValue("description", ui->descE->text());
        settings.setValue("instructions", ui->instrE->toHtml());
        settings.setValue("image", ui->imageE->text());
        settings.setValue("size", ui->sizeSB->value());
        settings.setValue("engine", ui->engineCB->currentIndex()+1);
        settings.setValue("quality", ui->qualityCB->currentText());
        settings.setValue("portrait", "0");
        settings.endGroup();
        settings.sync();
#ifdef Q_WS_MAEMO_5
        QMaemo5InformationBox::information ( this, tr("Application added to database"));
#endif
        close();
        emit gameAdded();
    }
    else
    {
#ifdef Q_WS_MAEMO_5
        QMaemo5InformationBox::information ( this, tr("Name and Launch URI fields are mandatory"));
#endif
    }
}

void AddGame::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
