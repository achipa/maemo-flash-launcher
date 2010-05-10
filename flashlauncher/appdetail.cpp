#include "appdetail.h"
#include "ui_appdetail.h"

appdetail::appdetail(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::appdetail)
{
    ui->setupUi(this);
}

appdetail::~appdetail()
{
    delete ui;
}

void appdetail::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
