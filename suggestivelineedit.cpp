#include "suggestivelineedit.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QStringListModel>
#include <algorithm>

QString SuggestiveLineEdit::completionFromPopup(QCompleter *completer)
{
    if (!completer || !completer->popup())
        return {};

    QModelIndex idx = completer->popup()->currentIndex();
    if (idx.isValid())
        return idx.data(Qt::DisplayRole).toString();

    const QString currentCompletion = completer->currentCompletion();
    if (!currentCompletion.isEmpty())
        return currentCompletion;

    if (completer->completionCount() > 0)
        return completer->completionModel()->index(0, 0).data(Qt::DisplayRole).toString();

    return {};
}

SuggestiveLineEdit::SuggestiveLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    completer = new QCompleter(this);
    model = new QStringListModel(completer);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWidget(this);
    completer->setModel(model);

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

    connect(completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &SuggestiveLineEdit::completionAccepted);

    if (completer->popup()) {
        connect(completer->popup(), &QAbstractItemView::clicked, this,
                [this](const QModelIndex &index) {
                    const QString chosen = index.data(Qt::DisplayRole).toString();
                    if (chosen.isEmpty())
                        return;

                    setText(chosen);
                    setCursorPosition(chosen.length());
                    emit completionAccepted(chosen);
                    completer->popup()->hide();
                });
    }
}

void SuggestiveLineEdit::setSuggestions(const QStringList &list)
{
    QStringList normalized;
    normalized.reserve(list.size());
    for (const QString &s : list)
        normalized.append(s.toUpper());

    std::sort(normalized.begin(), normalized.end(),
              [](const QString &a, const QString &b) {
                  return a.compare(b, Qt::CaseInsensitive) < 0;
              });

    model->setStringList(normalized);

    if (hasFocus()) {
        completer->setCompletionPrefix(text());
        completer->complete();

        if (completer->popup() && completer->completionCount() > 0) {
            QModelIndex first = completer->completionModel()->index(0, 0);
            completer->popup()->setCurrentIndex(first);
        }
    }
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
                    emit completionAccepted(chosen);
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
        case Qt::Key_Left:
        case Qt::Key_Right:
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            const QString chosen = completionFromPopup(completer);
            if (!chosen.isEmpty()) {
                setText(chosen);
                setCursorPosition(chosen.length());
                emit completionAccepted(chosen);
            }

            completer->popup()->hide();
            emit returnPressed();

            event->accept();
            return;
        }
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
                emit completionAccepted(chosen);
            }

            completer->popup()->hide();
            emit pressTab();
            focusNextChild();

            event->accept();
            return;
        }
    }

    if (!popupVisible && event->key() == Qt::Key_Down) {
        emit suggestionsRequested(text());
        completer->setCompletionPrefix(text());
        completer->complete();

        if (completer->popup() && completer->completionCount() > 0) {
            QModelIndex first = completer->completionModel()->index(0, 0);
            completer->popup()->setCurrentIndex(first);
        }

        event->accept();
        return;
    }

    if (!popupVisible && event->key() == Qt::Key_Left) {
        if (cursorPosition() == 0) {
            QWidget *previous = previousInFocusChain();
            if (qobject_cast<SuggestiveLineEdit *>(previous)) {
                previous->setFocus();
                event->accept();
                return;
            }
        }
    }

    if (!popupVisible && event->key() == Qt::Key_Right) {
        if (cursorPosition() == text().length()) {
            QWidget *next = nextInFocusChain();
            if (qobject_cast<SuggestiveLineEdit *>(next)) {
                next->setFocus();
                event->accept();
                return;
            }
        }
    }

    QLineEdit::keyPressEvent(event);
}

void SuggestiveLineEdit::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);

    emit suggestionsRequested(text());

    if (completer) {
        completer->setCompletionPrefix(text());
        completer->complete();

        if (completer->popup() && completer->completionCount() > 0) {
            QModelIndex first = completer->completionModel()->index(0, 0);
            completer->popup()->setCurrentIndex(first);
        }
    }
}

void SuggestiveLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    setCursorPosition(text().length());
}
