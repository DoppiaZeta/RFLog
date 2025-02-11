#include "mappasrpc.h"

MappaSRPC::MappaSRPC(DatabaseManager *database, const QString &statoDefault, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MappaSRPC)
{
    db = database;
    applica = false;
    ui->setupUi(this);

    DBResult *res = db->executeQuery("select distinct stato from locatori order by stato");
    for(int i = 0; i < res->count(); i++) {
        ui->stati->addItem(res->getCella(i, 0));
    }
    delete res;

    ui->stati->setCurrentText(statoDefault);

    connect(ui->applicaOK, &QPushButton::clicked, this, &MappaSRPC::applicaOK);
}

MappaSRPC::~MappaSRPC()
{
    delete ui;
}

void MappaSRPC::applicaOK() {
    applica = true;
    close();
}

bool MappaSRPC::getApplica() const {
    return applica;
}

Mappa::tipoMappa MappaSRPC::getTipo() const {
    if(ui->radioR->isChecked())
        return Mappa::tipoMappa::regioni;
    if(ui->radioP->isChecked())
        return Mappa::tipoMappa::provincie;
    if(ui->radioC->isChecked())
        return Mappa::tipoMappa::comuni;

    return Mappa::tipoMappa::stati;
}

QString MappaSRPC::getStato() const {
    return ui->stati->currentText();
}
