#ifndef ADDSWFS_H
#define ADDSWFS_H

#include <QtGui/QDialog>
#include <QtGui/QCheckBox>

namespace Ui {
    class AddSWFs;
}

class AddSWFs : public QDialog
{
    Q_OBJECT

public:
    explicit AddSWFs(QWidget *parent = 0);
    ~AddSWFs();
    void addPath(QString p);
    QStringList getSelected();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::AddSWFs *ui;
    QVector<QCheckBox*> cblist;
};

#endif // ADDSWFS_H
