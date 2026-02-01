#include "suggestivelineedit.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QStringListModel>
#include <algorithm>

// Prende la voce evidenziata nel popup (NON sempre la prima!)
static QString completionFromPopup(QCompleter *c)
{
    if (!c || !c->popup())
        return {};

    QModelIndex idx = c->popup()->currentIndex();
    if (idx.isValid())
        return idx.data(Qt::DisplayRole).toString();

    // fallback
    const QString cc = c->currentCompletion();
    if (!cc.isEmpty())
        return cc;

    if (c->completionCount() > 0)
        return c->completionModel()->index(0, 0).data(Qt::DisplayRole).toString();

    return {};
}

SuggestiveLineEdit::SuggestiveLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    completer = new QCompleter(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWidget(this);

    connect(this, &QLineEdit::textEdited, this,
            [this](const QString &text)
            {
                completer->setCompletionPrefix(text);
                completer->complete();

                // forza sempre una riga selezionata (la prima), così ->/Tab hanno un target
                if (completer->popup() && completer->completionCount() > 0) {
                    QModelIndex first = completer->completionModel()->index(0, 0);
                    completer->popup()->setCurrentIndex(first);
                }
            });
}

void SuggestiveLineEdit::setCompleter(QStringList &list)
{
    for (QString &s : list)
        s = s.toUpper();

    std::sort(list.begin(), list.end(),
              [](const QString &a, const QString &b) {
                  return a.compare(b, Qt::CaseInsensitive) < 0;
              });

    auto *model = new QStringListModel(list, completer);
    completer->setModel(model);
}

// QUI intercettiamo TAB PRIMA che QWidget lo trasformi in cambio focus
bool SuggestiveLineEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(e);

        const bool popupVisible =
            completer && completer->popup() && completer->popup()->isVisible();

        if (popupVisible) {
            // TAB o SHIFT+TAB: accetta suggerimento e cambia focus come vuoi tu
            if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
                const QString chosen = completionFromPopup(completer);
                if (!chosen.isEmpty()) {
                    setText(chosen);
                    setCursorPosition(chosen.length());
                }

                completer->popup()->hide();
                emit pressTab();

                if (ke->key() == Qt::Key_Backtab)
                    focusPreviousChild();
                else
                    focusNextChild();

                ke->accept();
                return true; // IMPORTANTISSIMO: blocca il focus traversal automatico di Qt
            }
        }
    }

    return QLineEdit::event(e);
}

void SuggestiveLineEdit::keyPressEvent(QKeyEvent *event)
{
    const bool popupVisible =
        completer && completer->popup() && completer->popup()->isVisible();

    if (popupVisible) {
        // Navigazione popup (così si muove davvero la selezione)
        switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
            QApplication::sendEvent(completer->popup(), event);
            return;
        default:
            break;
        }

        // FRECCIA DESTRA: accetta + focus next (come hai chiesto)
        if (event->key() == Qt::Key_Right) {
            const QString chosen = completionFromPopup(completer);
            if (!chosen.isEmpty()) {
                setText(chosen);
                setCursorPosition(chosen.length());
            }

            completer->popup()->hide();
            emit pressTab();
            focusNextChild();

            event->accept();
            return;
        }
    }

    QLineEdit::keyPressEvent(event);
}
