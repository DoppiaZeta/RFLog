#include "suggestivelineedit.h"
#include <qabstractitemview.h>

SuggestiveLineEdit::SuggestiveLineEdit(QWidget *parent)
    : QLineEdit(parent) {
    completer = new QCompleter(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWidget(this); // Associa il completer al LineEdit


    connect(this, &QLineEdit::textEdited, this, [this](const QString &text) {
        completer->setCompletionPrefix(text); // Filtra in base al testo
        completer->complete(); // Mostra il popup aggiornato

        // Aggiorna il placeholder con la prima voce
        if (completer->completionCount() > 0) {
            QString firstCompletion = completer->completionModel()->index(0, 0).data(Qt::DisplayRole).toString();
            setPlaceholderText(firstCompletion);
        } else {
            setPlaceholderText(""); // Rimuove il placeholder se non ci sono completamenti
        }
    });

}

void SuggestiveLineEdit::paintEvent(QPaintEvent *event) {
    QLineEdit::paintEvent(event);

    if (completer && completer->completionCount() > 0) {
        // Ottieni il primo suggerimento
        QString firstCompletion = completer->completionModel()->index(0, 0).data(Qt::DisplayRole).toString();

        if (!firstCompletion.isEmpty() && firstCompletion.startsWith(text(), Qt::CaseInsensitive)) {
            QPainter painter(this);
            painter.setPen(Qt::gray);

            // Mostra il suggerimento dinamico (solo la parte mancante)
            QString remainingSuggestion = firstCompletion.mid(text().length());
            int textWidth = fontMetrics().horizontalAdvance(text());
            QRect rect = this->rect().adjusted(textWidth + 2, 0, -5, 0);

            painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, remainingSuggestion);
        }
    }
}

void SuggestiveLineEdit::setSuggestion(const QString &suggestionText) {
    suggestion = suggestionText;
    update(); // Forza il ridisegno
}

void SuggestiveLineEdit::setCompleter(QStringList &list) {
    // Converte ogni elemento della lista in maiuscolo
    for (QString &item : list) {
        item = item.toUpper();
    }

    // Ordina la lista alfabeticamente (case insensitive, anche se ora è tutto maiuscolo)
    std::sort(list.begin(), list.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    // Crea e imposta il modello
    QStringListModel *model = new QStringListModel(list, completer);
    completer->setModel(model); // Imposta il modello nel completer
}


bool SuggestiveLineEdit::focusNextPrevChild(bool next) {
    if (completer && completer->popup()->isVisible()) {
        QString currentCompletion = completer->currentCompletion();

        if (!currentCompletion.isEmpty()) {
            setText(currentCompletion); // Completa con l'elemento selezionato
        } else if (completer->completionCount() > 0) {
            setText(completer->completionModel()->index(0, 0).data(Qt::DisplayRole).toString()); // Completa con il primo elemento
        }

        completer->popup()->hide(); // Nascondi il popup
        emit pressTab();
        return true; // Blocca il cambio di focus
    }

    // Comportamento normale per altri tasti
    return QLineEdit::focusNextPrevChild(next);
}

