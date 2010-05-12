#include "addgame.h"
#include "ui_addgame.h"

AddGame::AddGame(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AddGame)
{
    ui->setupUi(this);
}

AddGame::~AddGame()
{
    delete ui;
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
