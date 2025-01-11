#include "locatoripreferiti.h"

LocatoriPreferiti::LocatoriPreferiti(DatabaseManager *db, QWidget *parent)
    : QDialog{parent}
    , ui(new Ui::LocatoriPreferiti)
{
    RFLog = db;

    ui->setupUi(this);

    QLocale locale = QLocale::C; // Usa la locale "C" che forza il punto come separatore
    locale.setNumberOptions(QLocale::OmitGroupSeparator); // Evita separatori di migliaia
    ui->cercaLat->setLocale(locale); // Imposta la locale per il primo spinbox
    ui->cercaLon->setLocale(locale); // Imposta la locale per il secondo spinbox


    ui->listaLocatori->setColumnCount(2);
    ui->listaLocatori->setHorizontalHeaderLabels(QStringList() << tr("Locatore") << tr("Nome"));
    ui->listaLocatori->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    connect(ui->cercaLatLonOK, &QPushButton::clicked, this, &LocatoriPreferiti::cercaLatLon);
    connect(ui->aggiungiOK, &QPushButton::clicked, this, &LocatoriPreferiti::addLocatoreOK);
    connect(ui->eliminaOK, &QPushButton::clicked, this, &LocatoriPreferiti::delLocatoreOK);
    connect(ui->listaLocatori, &QTableWidget::itemClicked, this, &LocatoriPreferiti::onItemClicked);


    DBResult *res = RFLog->executeQuery("select locatore, nome from locatoripreferiti order by locatore");
    for(int i = 0; i < res->getRigheCount(); i++) {
        addLocatore(res->getCella(i, "locatore"), res->getCella(i, "nome"));
    }
    delete res;
}

void LocatoriPreferiti::onItemClicked(QTableWidgetItem *item) {
    if (!item) return;

    // Ottieni la riga dell'elemento cliccato
    int row = item->row();

    // Ottieni i valori delle colonne della riga cliccata
    QTableWidgetItem *locItem = ui->listaLocatori->item(row, 0); // Colonna "locatore"
    QTableWidgetItem *nomeItem = ui->listaLocatori->item(row, 1); // Colonna "nome"

    // Popola i campi di input con i valori della riga cliccata
    if (locItem) {
        ui->aggiungiLocatore->setText(locItem->text());
    }
    if (nomeItem) {
        ui->nomeLocatore->setText(nomeItem->text());
    }
}


void LocatoriPreferiti::cercaLatLon() {
    QString loc;
    double lat = ui->cercaLat->value();
    double lon = ui->cercaLon->value();

    qDebug() << "Input Latitudine:" << lat << "Longitudine:" << lon;

    // Verifica che lat e lon siano nei limiti validi
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        ui->aggiungiLocatore->setText("Coordinate non valide");
        return;
    }

    // Funzione per convertire un numero in una lettera
    auto numeroToLettera = [](int n) -> QChar {
        return QChar('A' + n);
    };

    // Primo livello (blocchi principali)
    int lonLetter = static_cast<int>((lon + 180) / 20);
    int latLetter = static_cast<int>((lat + 90) / 10);

    qDebug() << "Primo livello: lonLetter=" << lonLetter << "latLetter=" << latLetter;

    // Secondo livello (blocchi numerici)
    double lonRemainder = (lon + 180) - lonLetter * 20;
    double latRemainder = (lat + 90) - latLetter * 10;

    int lonDigit = static_cast<int>(lonRemainder / 2);
    int latDigit = static_cast<int>(latRemainder / 1);

    qDebug() << "Secondo livello: lonDigit=" << lonDigit << "latDigit=" << latDigit;
    qDebug() << "Residui dopo secondo livello: lonRemainder=" << lonRemainder << "latRemainder=" << latRemainder;

    // Residuo dopo il secondo livello
    lonRemainder -= lonDigit * 2;  // ora lonRemainder è in [0..2)
    latRemainder -= latDigit * 1;  // ora latRemainder è in [0..1)

    // Terzo livello:
    // - per la longitudine, 24 sottogriglie in 2 gradi => ciascuna è 1/12°
    // - per la latitudine, 24 sottogriglie in 1 grado => ciascuna è 1/24°

    int lonSubLetter = static_cast<int>(lonRemainder * 12);
    int latSubLetter = static_cast<int>(latRemainder * 24);

    // Aggiusta eventuali errori di arrotondamento
    lonSubLetter = std::clamp(lonSubLetter, 0, 23);
    latSubLetter = std::clamp(latSubLetter, 0, 23);


    qDebug() << "Terzo livello: lonSubLetter=" << lonSubLetter << "latSubLetter=" << latSubLetter;

    // Costruzione del locatore
    loc.append(numeroToLettera(lonLetter));       // Lettera longitudine principale
    loc.append(numeroToLettera(latLetter));       // Lettera latitudine principale
    loc.append(QString::number(lonDigit));       // Numero longitudine secondaria
    loc.append(QString::number(latDigit));       // Numero latitudine secondaria
    loc.append(numeroToLettera(lonSubLetter));   // Lettera longitudine terziaria
    loc.append(numeroToLettera(latSubLetter));   // Lettera latitudine terziaria

    qDebug() << "Locatore generato:" << loc;

    // Mostra il risultato
    ui->aggiungiLocatore->setText(loc);
}

void LocatoriPreferiti::addLocatoreOK() {
    addLocatore(ui->aggiungiLocatore->text(), ui->nomeLocatore->text());
}

void LocatoriPreferiti::addLocatore(const QString &loc, const QString &txt) {
    if(!Coordinate::validaLocatore(loc.trimmed())) {
        return;
    }

    // Cerca se il locatore esiste già nella tabella
    for (int row = 0; row < ui->listaLocatori->rowCount(); ++row) {
        QTableWidgetItem *item = ui->listaLocatori->item(row, 0); // Ottieni la cella nella colonna 0
        if (item && item->text() == loc.trimmed()) {
            // Aggiorna il valore della colonna 1 (txt) se il locatore esiste
            ui->listaLocatori->item(row, 1)->setText(txt.trimmed());

            aggiornaDB();
            return; // Esci dalla funzione
        }
    }

    // Se il locatore non esiste, aggiungi una nuova riga
    int newRow = ui->listaLocatori->rowCount();
    ui->listaLocatori->insertRow(newRow);

    // Imposta i valori delle celle
    QTableWidgetItem *locItem = new QTableWidgetItem(loc.trimmed());
    QTableWidgetItem *txtItem = new QTableWidgetItem(txt.trimmed());
    ui->listaLocatori->setItem(newRow, 0, locItem);
    ui->listaLocatori->setItem(newRow, 1, txtItem);

    aggiornaDB();
}


void LocatoriPreferiti::delLocatoreOK() {
    // Ottieni la riga selezionata
    int selectedRow = ui->listaLocatori->currentRow();

    // Verifica se una riga è selezionata
    if (selectedRow != -1) {
        // Elimina la riga selezionata
        ui->listaLocatori->removeRow(selectedRow);

        aggiornaDB();
    }
}

void LocatoriPreferiti::aggiornaDB() {
    RFLog->executeQueryNoRes("DELETE FROM locatoripreferiti");

    // Inserisci i nuovi dati
    for (int row = 0; row < ui->listaLocatori->rowCount(); ++row) {
        QTableWidgetItem *item0 = ui->listaLocatori->item(row, 0);
        QTableWidgetItem *item1 = ui->listaLocatori->item(row, 1);

        QSqlQuery *q = RFLog->getQueryBind();
        q->prepare("INSERT INTO locatoripreferiti (locatore, nome) VALUES (:locatore, :nome)");
        q->bindValue(":locatore", item0->text());
        q->bindValue(":nome", item1->text());

        RFLog->executeQueryNoRes(q);
        delete q;
    }
}


