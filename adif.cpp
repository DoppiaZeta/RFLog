#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <utility>

#include "adif.h"
#include "coordinate.h"
#include "qso.h"

namespace {
QString adifField(const QString &name, const QString &value) {
    const QString normalizedName = name.trimmed().toUpper();
    const QString normalizedValue = value.trimmed().toUpper();
    if (normalizedName.isEmpty() || normalizedValue.isEmpty()) {
        return QString();
    }
    return QString("<%1:%2>%3").arg(normalizedName, QString::number(normalizedValue.size()), normalizedValue);
}

QPair<QString, QString> adifModeSubmodeFromTxMode(const QString &txMode) {
    const QString normalized = txMode.trimmed().toUpper();
    if (normalized == QStringLiteral("USB") || normalized == QStringLiteral("LSB")) {
        return qMakePair(QStringLiteral("SSB"), normalized);
    }
    return qMakePair(normalized, QString());
}

QString adifBandFromFrequency(double freqMHz) {
    if (freqMHz <= 0.0) {
        return QString();
    }

    struct BandRange {
        double min;
        double max;
        const char *band;
    };

    static const BandRange ranges[] = {
        {1.8, 2.0, "160m"},
        {3.5, 4.0, "80m"},
        {5.0, 5.5, "60m"},
        {7.0, 7.3, "40m"},
        {10.1, 10.15, "30m"},
        {14.0, 14.35, "20m"},
        {18.068, 18.168, "17m"},
        {21.0, 21.45, "15m"},
        {24.89, 24.99, "12m"},
        {28.0, 29.7, "10m"},
        {50.0, 54.0, "6m"},
        {144.0, 148.0, "2m"},
        {420.0, 450.0, "70cm"},
        {1240.0, 1300.0, "23cm"}
    };

    for (const auto &range : ranges) {
        if (freqMHz >= range.min && freqMHz <= range.max) {
            return QString::fromLatin1(range.band);
        }
    }

    return QString();
}

QString rstDefaultFromMode(const QString &mode) {
    const QString normalized = mode.trimmed().toUpper();
    if (normalized.contains(QStringLiteral("CW"))) {
        return QStringLiteral("599");
    }

    static const QStringList foniaModes = {
        QStringLiteral("SSB"),
        QStringLiteral("USB"),
        QStringLiteral("LSB"),
        QStringLiteral("FM"),
        QStringLiteral("AM"),
        QStringLiteral("PHONE")
    };
    for (const QString &phoneMode : foniaModes) {
        if (normalized == phoneMode || normalized.contains(phoneMode)) {
            return QStringLiteral("59");
        }
    }

    return QString();
}

QString safeAdifFilename(const QString &value) {
    QString safe = value.trimmed();
    safe.replace(QRegularExpression("[^A-Za-z0-9_-]+"), "_");
    if (safe.isEmpty()) {
        return QStringLiteral("tx");
    }
    return safe;
}

}

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

bool Adif::exportTxAdif(const QString& directoryPath,
                        const QString& nomeLog,
                        const QList<Qso*>& qsoList)
{
    if (directoryPath.isEmpty() || qsoList.isEmpty()) {
        return false;
    }

    QMap<QString, QList<QPair<Qso*, Qso::NominativoNome>>> qsosPerTx;
    for (Qso *qso : qsoList) {
        if (!qso) {
            continue;
        }
        QSet<QString> aggiunti;
        for (const auto &txInfo : qso->nominativoTx) {
            const QString nominativo = txInfo.nominativo.trimmed().toUpper();
            if (nominativo.isEmpty() || aggiunti.contains(nominativo)) {
                continue;
            }
            aggiunti.insert(nominativo);
            qsosPerTx[nominativo].append(qMakePair(qso, txInfo));
        }
    }

    if (qsosPerTx.isEmpty()) {
        return false;
    }

    QDir outDir(directoryPath);
    const QString createdTimestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd HHmmss");
    const QString fileTimestamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss");
    const QString logTag = safeAdifFilename(nomeLog.isEmpty() ? QStringLiteral("log") : nomeLog);

    bool exportedAnyFile = false;
    for (auto it = qsosPerTx.begin(); it != qsosPerTx.end(); ++it) {
        const QString nominativo = it.key();
        const QString fileName = QString("adif_%1_%2_%3.adi")
                                     .arg(logTag, safeAdifFilename(nominativo), fileTimestamp);
        QFile file(outDir.filePath(fileName));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            continue;
        }

        exportedAnyFile = true;
        QTextStream out(&file);
        out << "Generated by RFLog\n";
        out << adifField("ADIF_VER", "3.1.0");
        out << adifField("PROGRAMID", "RFLOG");
        out << adifField("CREATED_TIMESTAMP", createdTimestamp);
        out << "<EOH>\n";

        const QList<QPair<Qso*, Qso::NominativoNome>> &qsoEntries = it.value();
        for (const auto &entry : qsoEntries) {
            Qso *qso = entry.first;
            const auto &txInfo = entry.second;
            if (!qso) {
                continue;
            }

            const QString txCall = txInfo.nominativo.trimmed().toUpper();
            const QString txOperatore = txInfo.operatore.trimmed().toUpper();
            const QString rxCall = qso->nominativoRx.trimmed().toUpper();
            const QString rxOperatore = qso->operatoreRx.trimmed().toUpper();

            QDateTime orario = qso->orarioRx;
            if (orario.isValid()) {
                orario = orario.toUTC();
            }

            QString record;
            record += adifField("STATION_CALLSIGN", txCall);
            record += adifField("MY_NAME", txOperatore);
            record += adifField("CALL", rxCall);
            record += adifField("NAME", rxOperatore);
            record += adifField("MY_GRIDSQUARE", qso->locatoreTx.trimmed());
            record += adifField("GRIDSQUARE", qso->locatoreRx.trimmed());
            record += adifField("SRX", qso->progressivoRx.trimmed());

            const Coordinate::CqItu txCqItu = Coordinate::getCqItu(qso->locatoreTx.trimmed().toUpper());
            const Coordinate::CqItu rxCqItu = Coordinate::getCqItu(qso->locatoreRx.trimmed().toUpper());
            if (txCqItu.cq > 0) {
                record += adifField("MY_CQ_ZONE", QString::number(txCqItu.cq));
            }
            if (txCqItu.itu > 0) {
                record += adifField("MY_ITU_ZONE", QString::number(txCqItu.itu));
            }
            if (rxCqItu.cq > 0) {
                record += adifField("CQZ", QString::number(rxCqItu.cq));
            }
            if (rxCqItu.itu > 0) {
                record += adifField("ITUZ", QString::number(rxCqItu.itu));
            }
            record += adifField("TX_PWR", qso->potenzaTx > 0 ? QString::number(qso->potenzaTx) : QString());
            const auto modeSubmode = adifModeSubmodeFromTxMode(qso->trasmissioneTx);
            record += adifField("MODE", modeSubmode.first);
            record += adifField("SUBMODE", modeSubmode.second);
            QString rst = qso->segnaleRx > 0 ? QString::number(qso->segnaleRx) : QString();
            if (rst.isEmpty()) {
                rst = rstDefaultFromMode(qso->trasmissioneTx);
            }
            record += adifField("RST_SENT", rst);
            record += adifField("RST_RCVD", rst);
            record += adifField("FREQ", qso->frequenzaRx > 0 ? QString::number(qso->frequenzaRx, 'f', 3) : QString());
            record += adifField("BAND", adifBandFromFrequency(qso->frequenzaRx));
            record += adifField("QSL_RCVD", qso->qsl ? QStringLiteral("Y") : QStringLiteral("N"));

            if (orario.isValid()) {
                record += adifField("QSO_DATE", orario.toString("yyyyMMdd"));
                record += adifField("TIME_ON", orario.toString("HHmmss"));
            }

            for (const auto &altro : qso->altro) {
                record += adifField(altro.nome, altro.valore);
            }

            record += "<EOR>\n";
            out << record;
        }
    }

    return exportedAnyFile;
}
