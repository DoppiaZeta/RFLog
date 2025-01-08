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

void Adif::parseAdif(const QString& filePath, Adif& adif) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QString content = in.readAll();

    // Pulizia del contenuto per rimuovere spazi bianchi inutili
    content = content.simplified().trimmed();

    // Separazione dell'intestazione dai QSO
    int eohPosition = content.indexOf("<eoh>");
    QString headerContent = content.left(eohPosition).trimmed();
    QString qsoContent = content.mid(eohPosition + 5).trimmed();  // Salta "<eoh>"

    // Parsing dell'intestazione
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

    // Parsing dei QSO
    QRegularExpression qsoTagRegex("<(\\w+):\\d+>([^<]+)");
    QStringList qsoRecords = qsoContent.split("<eor>", Qt::SkipEmptyParts);
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

