#ifndef SUGGESTIVELINEEDIT_H
#define SUGGESTIVELINEEDIT_H

#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>

class SuggestiveLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    SuggestiveLineEdit(QWidget *parent);

    void setCompleter(QStringList &list);

signals:
    void pressTab();

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QCompleter *completer;
};


#endif // SUGGESTIVELINEEDIT_H
