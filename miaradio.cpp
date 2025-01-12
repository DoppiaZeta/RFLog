#include "miaradio.h"
#include "ui_miaradio.h"

MiaRadio::MiaRadio(DatabaseManager *db, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MiaRadio)
{
    RFLog = db;
    ui->setupUi(this);


    connect(ui->nomeRadioOK, &QPushButton::clicked, this, &MiaRadio::aggiungiRadio);
    connect(ui->eliminaRadioOK, &QPushButton::clicked, this, &MiaRadio::eliminaRadio);


    DBResult *res = RFLog->executeQuery("select nome from radio order by nome");
    for(int i = 0; i < res->getRigheCount(); i++) {
        ui->radioList->addItem(res->getCella(i, 0));
    }
    delete res;
}

MiaRadio::~MiaRadio()
{
    delete ui;
}

void MiaRadio::aggiungiRadio() {
    QString r = ui->nomeRadio->text().trimmed();

    if (!r.isEmpty()) {
        // Verifica se l'elemento esiste già nella lista
        bool duplicato = false;
        for (int i = 0; i < ui->radioList->count(); ++i) {
            if (ui->radioList->item(i)->text() == r) {
                duplicato = true;
                break;
            }
        }

        // Aggiungi l'elemento solo se non è un duplicato
        if (!duplicato) {
            ui->radioList->addItem(r);
            aggiornaDB();
        }
    }
}


void MiaRadio::eliminaRadio() {
    // Ottieni l'elemento selezionato
    QListWidgetItem *item = ui->radioList->currentItem();

    // Verifica se un elemento è selezionato
    if (item) {
        delete ui->radioList->takeItem(ui->radioList->row(item));
        aggiornaDB();
    }
}


void MiaRadio::aggiornaDB() {
    RFLog->executeQueryNoRes("delete from radio");
    for (int i = 0; i < ui->radioList->count(); ++i) {
        QSqlQuery *q = RFLog->getQueryBind();
        q->prepare("insert into radio (nome) values (:nome)");
        q->bindValue(":nome", ui->radioList->item(i)->text());
        RFLog->executeQueryNoRes(q);
        delete q;
    }
}
