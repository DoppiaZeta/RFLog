#include "qso.h"
#include "coordinate.h"

#include <QRegularExpression>
#include <QTimeZone>

const std::map<double, double> Qso::bandaToFreq = {
    {160, 1.9}, {80, 3.5}, {60, 5.3}, {40, 7.1}, {30, 10.1}, {20, 14.1},
    {17, 18.1}, {15, 21.1}, {12, 24.9}, {10, 28.5}, {6, 50.1}, {2, 144.1},
    {0.7, 432.1}, {0.23, 1296.1}, {0.13, 2304.1}, {0.05, 5760.1},
    {0.03, 10368.1}, {0.015, 24000.0}, {0.007, 47000.0}
};

Qso::Qso(DatabaseManager *db, int log, int id) {
    RFLog = db;
    logId = log;
    mioId = id;
    duplicato = false;

    if(log > 0 && id > 0) {
        QSqlQuery *q;
        DBResult *res;
        q = RFLog->getQueryBind();
        q->prepare(R"(
select
locatoreTx,
progressivoRx,
radioTx,
potenzaTx,
trasmissioneTx,
nominativoRx,
operatoreRx,
locatoreRx,
segnaleRx,
frequenzaRx,
orarioRx,
qsl
from qso
where id = :id
)");
        q->bindValue(":id", mioId);
        res = RFLog->executeQuery(q);
        if(res->hasRows()) {
            locatoreTx = res->getCella("locatoreTx");
            progressivoRx = res->getCella("progressivoRx");
            radioTx = res->getCella("radioTx");
            potenzaTx = res->getCella("potenzaTx").toInt();
            trasmissioneTx = res->getCella("trasmissioneTx");
            nominativoRx = res->getCella("nominativoRx");
            locatoreRx = res->getCella("locatoreRx");
            operatoreRx = res->getCella("operatoreRx");
            segnaleRx = res->getCella("segnaleRx").toDouble();
            frequenzaRx = res->getCella("frequenzaRx").toDouble();
            qsl = res->getCella("qsl") == 'S';
            orarioRx = QDateTime::fromString(res->getCella("orarioRx"), Qt::ISODate);
        }
        delete res;
        delete q;

        q = RFLog->getQueryBind();
        q->prepare("select nominativo, operatore from qsoNominativi where idQso = :id order by nominativo");
        q->bindValue(":id", mioId);
        res = RFLog->executeQuery(q);
        for(int i = 0; i < res->getRigheCount(); i++) {
            NominativoNome n;
            n.nominativo = res->getCella(i, "nominativo");
            n.operatore = res->getCella(i, "operatore");
            nominativoTx.push_back(n);
        }
        delete res;
        delete q;

        q = RFLog->getQueryBind();
        q->prepare("select nome, valore from qsoAltro where idQso = :id order by nome");
        q->bindValue(":id", mioId);
        res = RFLog->executeQuery(q);
        for(int i = 0; i < res->getRigheCount(); i++) {
            AltriParametri a;
            a.nome = res->getCella(i, "nome");
            a.valore = res->getCella(i, "valore");
            altro.push_back(a);
        }
        delete res;
        delete q;
    }
}

void Qso::eliminaDB() {
    if(mioId > 0 && logId > 0) {
        QSqlQuery *q = RFLog->getQueryBind();
        q->prepare("delete from qso where log = :log and id = :id");
        q->bindValue(":log", logId);
        q->bindValue(":id", mioId);
        RFLog->executeQueryNoRes(q);
        delete q;
    }
}

void Qso::insertAggiornaDB() {
    if (logId > 0) {
        QSqlQuery *q = RFLog->getQueryBind();

        if (mioId == 0) {
            // Query di INSERT
            q->prepare(R"(
                INSERT INTO qso (
                    idLog,
                    locatoreTx,
                    progressivoRx,
                    radioTx,
                    potenzaTx,
                    trasmissioneTx,
                    nominativoRx,
                    operatoreRx,
                    locatoreRx,
                    segnaleRx,
                    frequenzaRx,
                    orarioRx,
                    qsl
                ) VALUES (
                    :idLog,
                    :locatoreTx,
                    :progressivoRx,
                    :radioTx,
                    :potenzaTx,
                    :trasmissioneTx,
                    :nominativoRx,
                    :operatoreRx,
                    :locatoreRx,
                    :segnaleRx,
                    :frequenzaRx,
                    :orarioRx,
                    :qsl
                )
            )");
        } else {
            // Query di UPDATE
            q->prepare(R"(
                UPDATE qso
                SET
                    locatoreTx = :locatoreTx,
                    progressivoRx = :progressivoRx,
                    radioTx = :radioTx,
                    potenzaTx = :potenzaTx,
                    trasmissioneTx = :trasmissioneTx,
                    nominativoRx = :nominativoRx,
                    operatoreRx = :operatoreRx,
                    locatoreRx = :locatoreRx,
                    segnaleRx = :segnaleRx,
                    frequenzaRx = :frequenzaRx,
                    orarioRx = :orarioRx,
                    qsl = :qsl
                WHERE id = :id and idLog = :idLog
            )");
            q->bindValue(":id", mioId);
        }

        // Valori comuni per INSERT e UPDATE
        q->bindValue(":locatoreTx", locatoreTx);
        q->bindValue(":progressivoRx", progressivoRx);
        q->bindValue(":radioTx", radioTx);
        q->bindValue(":potenzaTx", potenzaTx);
        q->bindValue(":trasmissioneTx", trasmissioneTx);
        q->bindValue(":nominativoRx", nominativoRx);
        q->bindValue(":operatoreRx", operatoreRx);
        q->bindValue(":locatoreRx", locatoreRx);
        q->bindValue(":segnaleRx", segnaleRx);
        q->bindValue(":frequenzaRx", frequenzaRx);
        q->bindValue(":orarioRx", orarioRx);
        q->bindValue(":qsl", qsl ? 'S' : 'N');
        q->bindValue(":idLog", logId);

        RFLog->executeQueryNoRes(q);
        delete q;

        if(mioId == 0) {
            DBResult *res = RFLog->executeQuery("select max(id) from qso");
            if(res->hasRows())
                mioId = res->getCella(0).toInt();
            delete res;
        }

        QSqlQuery *qDeleteNom = RFLog->getQueryBind();
        qDeleteNom->prepare("delete from qsoNominativi where idQso = :id");
        qDeleteNom->bindValue(":id", mioId);
        RFLog->executeQueryNoRes(qDeleteNom);
        delete qDeleteNom;

        QSqlQuery *qDeleteAltro = RFLog->getQueryBind();
        qDeleteAltro->prepare("delete from qsoAltro where idQso = :id");
        qDeleteAltro->bindValue(":id", mioId);
        RFLog->executeQueryNoRes(qDeleteAltro);
        delete qDeleteAltro;

        for(int i = 0; i < nominativoTx.size(); i++) {
            QSqlQuery *qInsert = RFLog->getQueryBind();
            qInsert->prepare("insert into qsoNominativi (idQso, nominativo, operatore) values (:id, :nominativo, :operatore)");
            qInsert->bindValue(":id", mioId);
            qInsert->bindValue(":nominativo", nominativoTx[i].nominativo);
            qInsert->bindValue(":operatore", nominativoTx[i].operatore);
            RFLog->executeQueryNoRes(qInsert);
            delete qInsert;
        }

        for(int i = 0; i < altro.size(); i++) {
            QSqlQuery *qInsert = RFLog->getQueryBind();
            qInsert->prepare("insert into qsoAltro (idQso, nome, valore) values (:id, :nome, :valore)");
            qInsert->bindValue(":id", mioId);
            qInsert->bindValue(":nome", altro[i].nome);
            qInsert->bindValue(":valore", altro[i].valore);
            RFLog->executeQueryNoRes(qInsert);
            delete qInsert;
        }
    }
}

QVector<int> Qso::getListaQso(DatabaseManager *db, int log) {
    QVector<int> ret;

    QSqlQuery *q = db->getQueryBind();
    q->prepare("select id from qso where idLog = :id order by orarioRx desc");
    q->bindValue(":id", log);
    DBResult *res = db->executeQuery(q);
    for(int i = 0; i < res->getRigheCount(); i++) {
        ret.push_back(res->getCella(i, 0).toInt());
    }
    delete q;
    delete res;
    return ret;
}

void Qso::insertDaAdif(const QMap<QString, QString> &contatto) {
        // Popola i dati del trasmettitore
        locatoreTx = contatto.value("my_gridsquare").toUpper();
        potenzaTx = contatto.value("tx_pwr").toInt();
        trasmissioneTx = contatto.value("mode");
        progressivoRx = contatto.value("srx").trimmed();
        if (progressivoRx.isEmpty()) {
            progressivoRx = contatto.value("stx").trimmed();
        }

        // Popola i dati del ricevitore
        nominativoRx = contatto.value("call").toUpper();
        operatoreRx = contatto.value("name").toUpper();
        locatoreRx = contatto.value("gridsquare").toUpper();
        segnaleRx = contatto.value("rst_sent").toDouble();
        frequenzaRx = contatto.value("freq").toDouble();
        qsl = contatto.value("qsl_rcvd") == 'S';

        const bool hasCqz = !contatto.value("cqz").trimmed().isEmpty();
        const bool hasItuz = !contatto.value("ituz").trimmed().isEmpty();
        if ((!hasCqz || !hasItuz) && Coordinate::validaLocatore(locatoreRx)) {
            const Coordinate::CqItu cqitu = Coordinate::getCqItu(locatoreRx);
            if (!hasCqz && cqitu.cq > 0) {
                AltriParametri parametro;
                parametro.nome = "cqz";
                parametro.valore = QString::number(cqitu.cq);
                altro.append(parametro);
            }
            if (!hasItuz && cqitu.itu > 0) {
                AltriParametri parametro;
                parametro.nome = "ituz";
                parametro.valore = QString::number(cqitu.itu);
                altro.append(parametro);
            }
        }


        // Recuperi le stringhe dal contatto ADIF
        QString qsoDate = contatto.value("qso_date");  // Es: "20250111" (8 caratteri: YYYYMMDD)
        QString timeOn  = contatto.value("time_on");   // Es: "1317" (4 caratteri: HHmm)
            //   o "131700" (6 caratteri: HHmmss)

        // Se la parte dell'ora è 4 caratteri (HHmm), aggiungi "00" per i secondi
        if (timeOn.size() == 4) {
            timeOn.append("00");  // diventa "131700"
        }
        // Volendo, se l'ora è più corta/strana, puoi gestire anche quei casi:
        // else if (timeOn.size() < 4) { ... }  // errore o altra logica

        // Ora hai sempre YYYYMMDD (8 char) + HHmmss (6 char) = 14 caratteri
        QString combined = qsoDate + timeOn; // Esempio: "20250111" + "131700" = "20250111131700"

        // Parse come "yyyyMMddHHmmss"
        QDateTime dt = QDateTime::fromString(combined, "yyyyMMddHHmmss");

        // Verifica se valido
        if (!dt.isValid()) {
            qDebug() << "Errore nella conversione data/ora" << combined;
        }

        // Assegni al tuo campo orarioRx
        orarioRx = dt;


        if (!orarioRx.isValid()) {
            qDebug() << "Errore nella conversione della data e ora del QSO.";
        }

        // Popola nominativoTx con `station_callsign` e `my_name` (se presenti)
        QString stationCallsign = contatto.value("station_callsign");
        QString operatorName = contatto.value("my_name");
        if (!stationCallsign.isEmpty() || !operatorName.isEmpty()) {
            NominativoNome nominativo;
            nominativo.nominativo = stationCallsign.toUpper();
            nominativo.operatore = operatorName.toUpper();
            nominativoTx.append(nominativo);
        }

        if (hasCqz) {
            AltriParametri parametro;
            parametro.nome = "cqz";
            parametro.valore = contatto.value("cqz").trimmed();
            altro.append(parametro);
        }
        if (hasItuz) {
            AltriParametri parametro;
            parametro.nome = "ituz";
            parametro.valore = contatto.value("ituz").trimmed();
            altro.append(parametro);
        }

        // Gestisce eventuali parametri aggiuntivi
        for (auto it = contatto.begin(); it != contatto.end(); ++it) {
            if (it.key() != "my_gridsquare" && it.key() != "my_name" &&
                it.key() != "tx_pwr" && it.key() != "mode" &&
                it.key() != "call" && it.key() != "name" &&
                it.key() != "gridsquare" && it.key() != "rst_sent" &&
                it.key() != "freq" && it.key() != "qso_date" &&
                it.key() != "time_on" && it.key() != "station_callsign" &&
                it.key() != "qsl_rcvd" && it.key() != "srx" && it.key() != "stx" &&
                it.key() != "cqz" && it.key() != "ituz") {
                AltriParametri parametro;
                parametro.nome = it.key();
                parametro.valore = it.value();
                altro.append(parametro);
            }
        }

        // Inserisce o aggiorna il QSO nel database
        insertAggiornaDB();
}

void Qso::insertDaEdi(const QMap<QString, QString> &header, const EdiRecord &record) {
    locatoreTx = header.value("PWWLo").toUpper();
    potenzaTx = header.value("SPowe").toInt();

    QString band = header.value("PBand");
    if (!band.isEmpty()) {
        QRegularExpression bandRegex(R"((\d+(\.\d+)?))");
        QRegularExpressionMatch match = bandRegex.match(band);
        if (match.hasMatch()) {
            frequenzaRx = match.captured(1).toDouble();
        }
    }

    nominativoRx = record.call.toUpper();
    locatoreRx = record.locator.toUpper();
    segnaleRx = record.rstSent.toDouble();

    QString qsoDate = record.date;
    QString qsoTime = record.time;
    if (qsoDate.size() == 6 && qsoTime.size() >= 4) {
        int year = 2000 + qsoDate.left(2).toInt();
        int month = qsoDate.mid(2, 2).toInt();
        int day = qsoDate.mid(4, 2).toInt();
        int hour = qsoTime.left(2).toInt();
        int minute = qsoTime.mid(2, 2).toInt();
        QDate date(year, month, day);
        QTime time(hour, minute);
        orarioRx = QDateTime(date, time, QTimeZone::UTC);
    }

    QString stationCallsign = header.value("PCall").trimmed();
    if (!stationCallsign.isEmpty()) {
        NominativoNome nominativo;
        nominativo.nominativo = stationCallsign.toUpper();
        nominativoTx.append(nominativo);
    }

    if (!record.serialSent.isEmpty()) {
        AltriParametri parametro;
        parametro.nome = "edi_serial_sent";
        parametro.valore = record.serialSent;
        altro.append(parametro);
    }

    if (!record.serialReceived.isEmpty()) {
        AltriParametri parametro;
        parametro.nome = "edi_serial_received";
        parametro.valore = record.serialReceived;
        altro.append(parametro);
    }

    if (!record.rstReceived.isEmpty()) {
        AltriParametri parametro;
        parametro.nome = "edi_rst_received";
        parametro.valore = record.rstReceived;
        altro.append(parametro);
    }

    if (!record.distance.isEmpty()) {
        AltriParametri parametro;
        parametro.nome = "edi_distance";
        parametro.valore = record.distance;
        altro.append(parametro);
    }

    insertAggiornaDB();
}

Linee Qso::getLinea() const {
    return Linee(locatoreTx, locatoreRx);
}

bool Qso::operator==(const Qso &q) const {
    return nominativoRx == q.nominativoRx;
}

bool Qso::operator<(const Qso &q) const {
    return orarioRx < q.orarioRx;
}

bool Qso::operator>(const Qso &q) const {
    return orarioRx > q.orarioRx;
}

void Qso::sort(QList<Qso*> & qsoList) {
    std::sort(qsoList.begin(), qsoList.end(), [](Qso* a, Qso* b) {
        return a->operator>(*b);
    });
}

double Qso::getBandaMt() const {
    if (frequenzaRx == 0) {
        return 0.0; // Valore non valido
    }

    double bestBanda = 0.0;
    double minDiff = std::numeric_limits<double>::max();

    for (const auto& pair : bandaToFreq) {
        double diff = std::abs(frequenzaRx - pair.second);
        if (diff < minDiff) {
            minDiff = diff;
            bestBanda = pair.first;
        }
    }

    return bestBanda;
}

double Qso::getFrequenza(double banda) {
    auto it = bandaToFreq.find(banda);
    if (it != bandaToFreq.end()) {
        return it->second;
    }
    return 0.0; // Valore non valido
}
