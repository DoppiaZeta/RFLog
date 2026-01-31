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

}
