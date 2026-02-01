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
    });

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


void SuggestiveLineEdit::keyPressEvent(QKeyEvent *event) {
    if ((event->key() == Qt::Key_Tab || event->key() == Qt::Key_Right) && completer && completer->popup()->isVisible()) {
        QString currentCompletion = completer->currentCompletion();

        if (!currentCompletion.isEmpty()) {
            setText(currentCompletion); // Completa con l'elemento selezionato
        } else if (completer->completionCount() > 0) {
            setText(completer->completionModel()->index(0, 0).data(Qt::DisplayRole).toString()); // Completa con il primo elemento
        }

        completer->popup()->hide(); // Nascondi il popup
        emit pressTab();
        focusNextChild();
        event->accept();
        return;
    }

    QLineEdit::keyPressEvent(event);
}

