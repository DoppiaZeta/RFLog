#ifndef QSO_H
#define QSO_H

#include <QString>
#include <QList>
#include <QVector>
#include <QMap>
#include <QDateTime>
#include <QDebug>

#include "databasemanager.h"
#include "linee.h"

class Qso : public QObject
{
    Q_OBJECT

public:
    struct NominativoNome {
        QString nominativo;
        QString operatore;
    };

    struct AltriParametri {
        QString nome;
        QString valore;
    };

    Qso(DatabaseManager *db, int log, int id = 0);
    bool operator==(const Qso &q) const;

    void insertAggiornaDB();
    void eliminaDB();
    void insertDaAdif(const QMap<QString, QString> &contatto);
    Linee getLinea() const;

    static QVector<int> getListaQso(DatabaseManager *db, int log);

    QList<NominativoNome> nominativoTx;
    QString locatoreTx;
    QString radioTx;
    int potenzaTx;
    QString trasmissioneTx;

    QString nominativoRx;
    QString operatoreRx;
    QString locatoreRx;
    double segnaleRx;
    double frequenzaRx;
    QDateTime orarioRx;
    bool qsl;
    bool duplicato;

    QVector<AltriParametri> altro;

private:
    DatabaseManager *RFLog;
    int logId;
    int mioId;
};

#endif // QSO_H
