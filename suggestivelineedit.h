#ifndef SUGGESTIVELINEEDIT_H
#define SUGGESTIVELINEEDIT_H

#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QKeyEvent>
#include <QMouseEvent>

class SuggestiveLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    SuggestiveLineEdit(QWidget *parent);

    void setSuggestions(const QStringList &list);

signals:
    void pressTab();
    void completionAccepted(const QString &text);
    void suggestionsRequested(const QString &text);

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private:
    static QString completionFromPopup(QCompleter *completer);

    QCompleter *completer;
    QStringListModel *model;
};


#endif // SUGGESTIVELINEEDIT_H
