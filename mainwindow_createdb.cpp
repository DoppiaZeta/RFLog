#include "mainwindow.h"

void MainWindow::creaDBRFLog() {
    RFLog->executeQueryNoRes(
        "CREATE TABLE IF NOT EXISTS log ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  nome TEXT NOT NULL,"
        "  data TEXT NOT NULL"
        ");"
        );

    RFLog->executeQueryNoRes(
        "CREATE TABLE IF NOT EXISTS radio ("
        "  nome TEXT NOT NULL"
        ");"
        );

    RFLog->executeQueryNoRes(
        "CREATE TABLE IF NOT EXISTS locatoripreferiti ("
        "  locatore TEXT NOT NULL,"
        "  nome TEXT"
        ");"
        );

    RFLog->executeQueryNoRes(
        "CREATE TABLE IF NOT EXISTS qso ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  idLog INTEGER NOT NULL,"
        "  locatoreTx TEXT,"
        "  progressivoRx TEXT,"
        "  radioTx TEXT,"
        "  potenzaTx INTEGER,"
        "  trasmissioneTx TEXT,"
        "  nominativoRx TEXT,"
        "  operatoreRx TEXT,"
        "  locatoreRx TEXT,"
        "  segnaleRx REAL,"
        "  frequenzaRx REAL,"
        "  orarioRx TEXT,"
        "  qsl TEXT,"
        "  FOREIGN KEY (idLog) REFERENCES log(id)"
        ");"
        );

    RFLog->executeQueryNoRes(
        "CREATE TABLE IF NOT EXISTS qsoNominativi ("
        "  idQso INTEGER NOT NULL,"
        "  nominativo TEXT NOT NULL,"
        "  operatore TEXT,"
        "  FOREIGN KEY (idQso) REFERENCES qso(id)"
        ");"
        );

    RFLog->executeQueryNoRes(
        "CREATE TABLE IF NOT EXISTS qsoAltro ("
        "  idQso INTEGER NOT NULL,"
        "  nome TEXT NOT NULL,"
        "  valore TEXT,"
        "  FOREIGN KEY (idQso) REFERENCES qso(id)"
        ");"
        );


    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qso_idLog ON qso(idLog);");
    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qso_locRx ON qso(locatoreRx);");
    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qso_locTx ON qso(locatoreTx);");
    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qso_orario ON qso(orarioRx);");

    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qsoNom_idQso ON qsoNominativi(idQso);");
    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qsoAltro_idQso ON qsoAltro(idQso);");
    RFLog->executeQueryNoRes("CREATE INDEX IF NOT EXISTS idx_qsoAltro_nome_idQso ON qsoAltro(nome, idQso);");

}
