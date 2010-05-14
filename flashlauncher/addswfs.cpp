#include "addswfs.h"
#include "ui_addswfs.h"
#include "QtGui/QCheckBox"

AddSWFs::AddSWFs(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddSWFs)
{
    ui->setupUi(this);
    connect(ui->importB, SIGNAL(clicked()), this, SLOT(accept()));
}

AddSWFs::~AddSWFs()
{
    delete ui;
    foreach (QCheckBox* cb, cblist)
        delete cb;
}

void AddSWFs::addPath(QString p)
{
    QCheckBox* cb = new QCheckBox();
    cb->setText(p);
    ui->verticalLayout_2->insertWidget(0, cb);
    cblist << cb;

}

QStringList AddSWFs::getSelected()
{
    QStringList sl;
    foreach (QCheckBox* cb, cblist)
        if (cb->isChecked())
            sl << cb->text();
    return sl;
}

void AddSWFs::changeEvent(QEvent *e)
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
