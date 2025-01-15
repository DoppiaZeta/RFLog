#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "adif.h"

void Adif::impostaIntestazione(const QMap<QString, QString>& header) {
    intestazione = header;
}

void Adif::aggiungiContatto(const QMap<QString, QString>& contatto) {
    contatti.append(contatto);
}

const QList<QMap<QString, QString>>& Adif::getContatti() const {
    return contatti;
}

const QMap<QString, QString>& Adif::getIntestazione() const {
    return intestazione;
}

void Adif::parseAdif(const QString& filePath, Adif& adif)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QString content = in.readAll();

    // Pulizia del contenuto per rimuovere spazi bianchi superflui
    content = content.simplified().trimmed();

    // 1) Cerchiamo la posizione di <eoh>, ignorando maiuscole/minuscole
    int eohPosition = content.indexOf("<eoh>", 0, Qt::CaseInsensitive);

    QString headerContent;
    QString qsoContent;

    if (eohPosition == -1) {
        // NESSUN <eoh> TROVATO:
        // -> Tutto il contenuto lo consideriamo come QSO,
        //    oppure potresti voler gestire un 'header' vuoto.
        headerContent.clear();
        qsoContent = content;
    } else {
        // ABBIAMO TROVATO <eoh>:
        // -> header è prima del tag, QSO è dopo.
        headerContent = content.left(eohPosition).trimmed();
        // Saltiamo i 5 caratteri di "<eoh>"
        qsoContent = content.mid(eohPosition + 5).trimmed();
    }

    // 2) Parsing dell'intestazione (se esiste)
    //    Cerchiamo tag del tipo <TAG:LEN>VALORE
    if (!headerContent.isEmpty()) {
        QMap<QString, QString> header;
        QRegularExpression headerTagRegex("<(\\w+):\\d+>([^<]+)");
        QRegularExpressionMatchIterator headerIt = headerTagRegex.globalMatch(headerContent);
        while (headerIt.hasNext()) {
            QRegularExpressionMatch match = headerIt.next();
            QString tag = match.captured(1).toLower().trimmed();
            QString valore = match.captured(2).trimmed();
            header[tag] = valore;
        }

        adif.impostaIntestazione(header);
    } else {
        // Se vuoi, puoi impostare un header vuoto
        // adif.impostaIntestazione(QMap<QString, QString>());
    }

    // 3) Parsing dei QSO
    //    Cerchiamo i singoli QSO separati dal tag <eor> (oppure <EOR>)
    QRegularExpression qsoTagRegex("<(\\w+):\\d+>([^<]+)");

    // Separiamo i record QSO con <eor> (ignora case)
    // Per farlo in modo case-insensitive, usiamo un replace preliminare
    // di <EOR> -> <eor>, poi uno split. Oppure potremmo usare un QRegularExpression.
    QString qsoContentLower = qsoContent;
    qsoContentLower.replace(QRegularExpression("<eor>", QRegularExpression::CaseInsensitiveOption), "<eor>");

    QStringList qsoRecords = qsoContentLower.split("<eor>", Qt::SkipEmptyParts);

    for (const QString& record : qsoRecords) {
        QMap<QString, QString> contatto;
        QRegularExpressionMatchIterator qsoIt = qsoTagRegex.globalMatch(record.trimmed());
        while (qsoIt.hasNext()) {
            QRegularExpressionMatch match = qsoIt.next();
            QString tag = match.captured(1).toLower().trimmed();
            QString valore = match.captured(2).trimmed();
            contatto[tag] = valore;
        }
        if (!contatto.isEmpty()) {
            adif.aggiungiContatto(contatto);
        }
    }

    file.close();
}


