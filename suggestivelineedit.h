#ifndef SUGGESTIVELINEEDIT_H
#define SUGGESTIVELINEEDIT_H

#include <QLineEdit>
#include <QPainter>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>

class SuggestiveLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    SuggestiveLineEdit(QWidget *parent);

    void setSuggestion(const QString &suggestionText);

    void setCompleter(QStringList &list);

signals:
    void pressTab();

protected:
    void paintEvent(QPaintEvent *event);
    bool focusNextPrevChild(bool next);

private:
    QString suggestion;
    QCompleter *completer;
};


#endif // SUGGESTIVELINEEDIT_H
