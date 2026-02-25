#include "locatoripreferiti.h"
#include "coordinate.h"

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
    ui->aggiungiLocatore->setText(Coordinate::calcolaLocatoreLatLon(ui->cercaLat->value(), ui->cercaLon->value()));
}

void LocatoriPreferiti::addLocatoreOK() {
    addLocatore(ui->aggiungiLocatore->text().trimmed().toUpper(), ui->nomeLocatore->text());
}

void LocatoriPreferiti::addLocatore(const QString &loc, const QString &txt) {
    const QString normalizedLoc = loc.trimmed().toUpper();
    if(!Coordinate::validaLocatore(normalizedLoc)) {
        return;
    }

    // Cerca se il locatore esiste già nella tabella
    for (int row = 0; row < ui->listaLocatori->rowCount(); ++row) {
        QTableWidgetItem *item = ui->listaLocatori->item(row, 0); // Ottieni la cella nella colonna 0
        if (item && item->text().trimmed().toUpper() == normalizedLoc) {
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
    QTableWidgetItem *locItem = new QTableWidgetItem(normalizedLoc);
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


