#ifndef APPDETAIL_H
#define APPDETAIL_H

#include <QDialog>

namespace Ui {
    class appdetail;
}

class appdetail : public QDialog
{
    Q_OBJECT

public:
    explicit appdetail(QWidget *parent = 0);
    ~appdetail();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::appdetail *ui;
};

#endif // APPDETAIL_H
