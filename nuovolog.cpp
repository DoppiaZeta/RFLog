#include "nuovolog.h"

NuovoLog::NuovoLog(DatabaseManager *db, QWidget *parent)
    : QDialog{parent}
    , ui(new Ui::NuovoLog)
{
    RFLog = db;
    numeroLogDB = 0;

    ui->setupUi(this);


    ui->logPassato->setColumnCount(3);
    ui->logPassato->setHorizontalHeaderLabels(QStringList() << "Id" << tr("Nome") << tr("Data di creazione"));
    ui->logPassato->hideColumn(0);
    ui->logPassato->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    connect(ui->nomeLogOK, &QPushButton::clicked, this, &NuovoLog::creaNuovoLog);
    connect(ui->logPassatoOK, &QPushButton::clicked, this, &NuovoLog::caricaLogPassato);
    connect(ui->logPassato, &QTableWidget::doubleClicked, this, &NuovoLog::caricaLogPassato);


    DBResult *res = RFLog->executeQuery("select id, nome, data from log order by data desc");
    for(int i = 0; i < res->getRigheCount(); i++) {
        int newRow = ui->logPassato->rowCount();
        ui->logPassato->insertRow(newRow);
        QTableWidgetItem *idItem = new QTableWidgetItem(res->getCella(i, "id"));
        QTableWidgetItem *nomeItem = new QTableWidgetItem(res->getCella(i, "nome"));
        QTableWidgetItem *dataIem = new QTableWidgetItem(res->getCella(i, "data"));
        ui->logPassato->setItem(newRow, 0, idItem);
        ui->logPassato->setItem(newRow, 1, nomeItem);
        ui->logPassato->setItem(newRow, 2, dataIem);
    }
    delete res;
}

void NuovoLog::mouseDoubleClickEvent(QMouseEvent *event) {
    QDialog::mouseDoubleClickEvent(event);
    caricaLogPassato();
}

void NuovoLog::creaNuovoLog() {
    QString nome = ui->nomeLog->text().trimmed();
    if(!nome.isEmpty()) {
        QString currentDateTimeGMT = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");

        QSqlQuery *q = RFLog->getQueryBind();
        q->prepare("insert into log (nome, data) values (:nome, :data)");
        q->bindValue(":nome", nome);
        q->bindValue(":data", currentDateTimeGMT);
        RFLog->executeQueryNoRes(q);
        delete q;

        DBResult *res = RFLog->executeQuery("select max(id) as max from log");
        if(res->hasRows())
            numeroLogDB = res->getCella("max").toInt();
        delete res;

        close();
    }
}

void NuovoLog::caricaLogPassato() {
    // Ottieni l'indice della riga selezionata
    int selectedRow = ui->logPassato->currentRow();

    // Verifica se una riga è selezionata
    if (selectedRow == -1) {
        return;
    }

    // Ottieni l'elemento nella prima colonna della riga selezionata
    QTableWidgetItem *item = ui->logPassato->item(selectedRow, 0);

    // Verifica che l'elemento non sia nullo e ottieni il testo
    if (item) {
        numeroLogDB = item->text().toInt();
        close();
    } else {
        return;
    }
}

int NuovoLog::getLogSelezionato() const {
    return numeroLogDB;
}
