#include "edi.h"

#include <QFile>
#include <QTextStream>

void Edi::impostaIntestazione(const QMap<QString, QString>& header) {
    intestazione = header;
}

void Edi::aggiungiContatto(const EdiRecord& record) {
    contatti.append(record);
}

const QMap<QString, QString>& Edi::getIntestazione() const {
    return intestazione;
}

const QList<EdiRecord>& Edi::getContatti() const {
    return contatti;
}

bool Edi::parseEdi(const QString& filePath, Edi& edi) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    QString currentSection;
    QMap<QString, QString> header;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2).trimmed();
            continue;
        }

        if (currentSection.startsWith("REG1TEST", Qt::CaseInsensitive)) {
            int eqPos = line.indexOf('=');
            if (eqPos > 0) {
                QString key = line.left(eqPos).trimmed();
                QString value = line.mid(eqPos + 1).trimmed();
                header[key] = value;
            }
            continue;
        }

        if (currentSection.startsWith("QSORecords", Qt::CaseInsensitive)) {
            QStringList parts = line.split(';', Qt::KeepEmptyParts);
            if (parts.size() < 3) {
                continue;
            }
            EdiRecord record;
            record.date = parts.value(0).trimmed();
            record.time = parts.value(1).trimmed();
            record.call = parts.value(2).trimmed();
            record.serialSent = parts.value(3).trimmed();
            record.rstSent = parts.value(4).trimmed();
            record.serialReceived = parts.value(5).trimmed();
            record.rstReceived = parts.value(6).trimmed();
            record.locator = parts.value(9).trimmed();
            record.distance = parts.value(10).trimmed();
            edi.aggiungiContatto(record);
        }
    }

    file.close();
    edi.impostaIntestazione(header);
    return !edi.getContatti().isEmpty();
}
