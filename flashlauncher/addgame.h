#ifndef ADDGAME_H
#define ADDGAME_H

#include <QtGui/QMainWindow>

namespace Ui {
    class AddGame;
}

class AddGame : public QMainWindow
{
    Q_OBJECT

public:
    explicit AddGame(QWidget *parent = 0);
    ~AddGame();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::AddGame *ui;
};

#endif // ADDGAME_H
