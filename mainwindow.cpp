#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mappaConfig(new Ui::MappaConfig)
{
    db = new DatabaseManager("locatori_fine.db", this);

    if (!db->openDatabase()) {
        qWarning() << "Failed to open database.";
        return;
    }

    QString formato = "border: none;\n"
                      "color: blue;\n"
                      "font-weight: bold;\n";

    Nominativo = new SuggestiveLineEdit(this);
    Locatore = new SuggestiveLineEdit(this);
    Segnale = new SuggestiveLineEdit(this);
    Frequenza = new SuggestiveLineEdit(this);
    Orario = new SuggestiveLineEdit(this);

    Tabella = new QTableWidget(0, 5, this);
    Tabella->horizontalHeader()->setVisible(false);
    Tabella->verticalHeader()->setVisible(false);

    QHeaderView *header = Tabella->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setStretchLastSection(false);

    QStringList suggestions = {"in3kgw", "in3ivc", "in3ktt", "in3hkz"};
    Nominativo->setCompleter(suggestions);

    Nominativo->setStyleSheet(formato);
    Locatore->setStyleSheet(formato);
    Segnale->setStyleSheet(formato);
    Frequenza->setStyleSheet(formato);
    Orario->setStyleSheet(formato);

    Tabella->setStyleSheet("border: none;");

    ui->setupUi(this);

    ui->Griglia->addWidget(Nominativo, 1, 0);
    ui->Griglia->addWidget(Locatore, 1, 1);
    ui->Griglia->addWidget(Segnale, 1, 2);
    ui->Griglia->addWidget(Frequenza, 1, 3);
    ui->Griglia->addWidget(Orario, 1, 4);
    ui->Griglia->addWidget(Tabella, 2, 0, 2, 5);

    QWidget::setTabOrder(Nominativo, Locatore);
    QWidget::setTabOrder(Locatore, Segnale);
    QWidget::setTabOrder(Segnale, Frequenza);
    QWidget::setTabOrder(Frequenza, Orario);

    mappa = new Mappa(db, this);
    mappa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->Griglia->addWidget(mappa, 1, 5, 2, 1);
    QWidget *mappaConfigW = new QWidget(this);
    mappaConfig->setupUi(mappaConfigW);
    ui->Griglia->addWidget(mappaConfigW, 3, 5, 1, 1);


    // Configura lo stretch per le colonne
    ui->Griglia->setColumnStretch(0, 1); // Colonne 0-4 occupano metà schermo
    ui->Griglia->setColumnStretch(1, 1);
    ui->Griglia->setColumnStretch(2, 1);
    ui->Griglia->setColumnStretch(3, 1);
    ui->Griglia->setColumnStretch(4, 1);
    ui->Griglia->setColumnStretch(5, 5); // La colonna mappa occupa metà schermo

    // Configura lo stretch per le righe
    ui->Griglia->setRowStretch(2, 2); // La terza riga torna a occupare meno spazio
    ui->Griglia->setRowStretch(2, 1);

    connect(mappa, &Mappa::mouseLocatore, this, &MainWindow::locatoreDaMappa);
    connect(mappa, &Mappa::mouseOceano, this, &MainWindow::locatoreDaMappaOceano);

    connect(Nominativo, &QLineEdit::textChanged, this, &MainWindow::compilaNominativo);
    connect(Nominativo, &SuggestiveLineEdit::pressTab, this, &MainWindow::catturaTab);
    connect(Nominativo, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);

    connect(Locatore, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Segnale, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Frequenza, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Orario, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);


    mappaConfig->tabWidget->setCurrentIndex(0);

    connect(mappaConfig->locatoreCentraOK, &QPushButton::clicked, this, &MainWindow::centraDaLocatore);

    connect(mappaConfig->locatorePresetITA, &QPushButton::clicked, this, &MainWindow::centraPredefinitoITA);
    connect(mappaConfig->locatorePresetMondo, &QPushButton::clicked, this, &MainWindow::centraPredefinitoMondo);
    connect(mappaConfig->locatorePresetEU, &QPushButton::clicked, this, &MainWindow::centraPredefinitoEU);
    connect(mappaConfig->locatorePresetAsia, &QPushButton::clicked, this, &MainWindow::centraPredefinitoAsia);
    connect(mappaConfig->locatorePresetAfrica, &QPushButton::clicked, this, &MainWindow::centraPredefinitoAfrica);
    connect(mappaConfig->locatorePresetNordAmerica, &QPushButton::clicked, this, &MainWindow::centraPredefinitoNordAmerica);
    connect(mappaConfig->locatorePresetSudAmerica, &QPushButton::clicked, this, &MainWindow::centraPredefinitoSudAmerica);
    connect(mappaConfig->locatorePresetOceania, &QPushButton::clicked, this, &MainWindow::centraPredefinitoOceania);


    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &MainWindow::aggiornaOrario);
    t->start(1000);


    mappaConfig->statoDB->addItem(QString());
    mappaConfig->regioneDB->addItem(QString());
    mappaConfig->provinciaDB->addItem(QString());
    mappaConfig->comuneDB->addItem(QString());

    QString q = QString("select distinct stato from locatori where stato is not null order by stato");
    DBResult *res = db->executeQuery(q);

    for(int i = 0; i < res->tabella.size(); i++) {
        mappaConfig->statoDB->addItem(res->tabella[i][0]);
    }

    delete res;

    connect(mappaConfig->statoDB, &QComboBox::textActivated, this, &MainWindow::cercaRegione);
    connect(mappaConfig->regioneDB, &QComboBox::textActivated, this, &MainWindow::cercaProvincia);
    connect(mappaConfig->provinciaDB, &QComboBox::textActivated, this, &MainWindow::cercaComune);
    connect(mappaConfig->comuneDB, &QComboBox::textActivated, this, &MainWindow::cercaLocatore);
    connect(mappaConfig->pulisciDB, &QPushButton::clicked, this, &MainWindow::pulisciCerca);
    connect(mappaConfig->locatoreCentraDB, &QPushButton::clicked, this, &MainWindow::confermaCerca);


    centraPredefinitoITA();


    // Linee che rientrano nel range
    mappa->addLinea({"JN56PK", "JN40UC"});
    mappa->addLinea({"JN56PK", "RR99XX"});
    mappa->addLinea({"JN56PK", "LQ22ES"});
    mappa->addLinea({"JN56PK", "JI64DF"}); // Punto vicino all'origine

    // Linee che escono dal range
    mappa->addLinea({"JN56PK", "FF33OU"}); // Lontano dall'origine, fuori dal range
    mappa->addLinea({"JN56PK", "BO38SH"}); // Estremo superiore fuori dal range
    mappa->addLinea({"JN56PK", "AA00AA"}); // Vicino all'origine ma fuori
}


MainWindow::~MainWindow()
{
    delete ui; // Pulisci la memoria
    db->closeDatabase();
}

void MainWindow::compilaNominativo(const QString &txt) {
    Nominativo->setText(txt.toUpper());
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        svuotaLineEdit();
    }

    // Comportamento normale per altri tasti
    QWidget::keyPressEvent(event);
}

void MainWindow::catturaTab() {
    qDebug() << Nominativo->text();
}

void MainWindow::svuotaLineEdit() {
    QString q;
    Nominativo->setText(q);
    Locatore->setText(q);
    Segnale->setText(q);
    Frequenza->setText(q);
    Orario->setText(q);
}

void MainWindow::confermaLinea() {
    QString nominativoText = Nominativo->text().trimmed(); // Elimina spazi inutili
    if (nominativoText.isEmpty()) {
        return; // Interrompe l'inserimento
    }

    // Controlla se il nominativo è già presente
    bool nominativoDuplicato = false;
    for (int row = 0; row < Tabella->rowCount(); ++row) {
        QTableWidgetItem *item = Tabella->item(row, 0); // Colonna del nominativo
        if (item && item->text() == nominativoText) {
            qDebug() << "Nominativo duplicato: " << nominativoText;
            nominativoDuplicato = true;
            break;
        }
    }

    // Ottieni l'orario GMT attuale
    QString currentDateTimeGMT = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");
    if(!Orario->text().trimmed().isEmpty())
        currentDateTimeGMT = Orario->text().trimmed();

    // Aggiungi una nuova riga in cima
    qDebug() << "Inserisci riga in cima";
    Tabella->insertRow(0); // Inserisce la nuova riga nella posizione 0

    // Inserisci dati nella nuova riga
    QTableWidgetItem *nominativoItem = new QTableWidgetItem(nominativoText);
    Tabella->setItem(0, 0, nominativoItem);
    Tabella->setItem(0, 1, new QTableWidgetItem(Locatore->text().trimmed()));
    Tabella->setItem(0, 2, new QTableWidgetItem(Segnale->text().trimmed()));
    Tabella->setItem(0, 3, new QTableWidgetItem(Frequenza->text().trimmed()));
    Tabella->setItem(0, 4, new QTableWidgetItem(currentDateTimeGMT));

    // Se duplicato, colora la nuova riga di rosso
    if (nominativoDuplicato) {
        for (int col = 0; col < Tabella->columnCount(); ++col) {
            QTableWidgetItem *rowItem = Tabella->item(0, col);
            if (rowItem) {
                rowItem->setBackground(QColor(190, 190, 255));
            }
        }
    }

    if(!nominativoDuplicato && Coordinate::validaLocatore(Locatore->text().trimmed())) {
        mappa->addLinea(Linee("JN56PK", Locatore->text().trimmed().left(6)));
    }
}

void MainWindow::aggiornaOrario() {
    if(Orario->text().isEmpty()) {
        // Ottieni l'orario GMT attuale
        QString currentDateTimeGMT = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");
        Orario->setPlaceholderText(currentDateTimeGMT);
    }
}

void MainWindow::locatoreDaMappa(QString loc) {
    QString q = QString(
                    "select locatore, lat_min, lat_max, lon_min, lon_max, stato, regione, provincia, comune from locatori where locatore = '%1'"
                    ).arg(DatabaseManager::escape(loc));
    DBResult *res = db->executeQuery(q);
    mappaConfig->Locatore->setText(loc);

    mappaConfig->Stato->setText(tr("Stato: ") + res->tabella[0][5]);
    mappaConfig->Regione->setText(tr("Regione: ") + res->tabella[0][6]);
    mappaConfig->Provincia->setText(tr("Provincia: ") + res->tabella[0][7]);
    mappaConfig->Comune->setText(tr("Comune: ") + res->tabella[0][8]);
    mappaConfig->lat_min->setText(res->tabella[0][1]);
    mappaConfig->lat_max->setText(res->tabella[0][2]);
    mappaConfig->lon_min->setText(res->tabella[0][3]);
    mappaConfig->lon_max->setText(res->tabella[0][4]);

    mappaConfig->locatoreCentra->setText(loc);
    delete res;
}

void MainWindow::locatoreDaMappaOceano() {
    QString vuoto;
    mappaConfig->Locatore->setText(vuoto);
    mappaConfig->Stato->setText(vuoto);
    mappaConfig->Regione->setText(vuoto);
    mappaConfig->Provincia->setText(vuoto);
    mappaConfig->Comune->setText(vuoto);

    mappaConfig->lat_min->setText(vuoto);
    mappaConfig->lat_max->setText(vuoto);
    mappaConfig->lon_min->setText(vuoto);
    mappaConfig->lon_max->setText(vuoto);

    mappaConfig->locatoreCentra->setText(vuoto);
}

void MainWindow::centraDaLocatore() {
    if(Coordinate::validaLocatore(mappaConfig->locatoreCentra->text().trimmed())) {
        QString lato1, lato2;
        int offset = mappaConfig->locatoreOffset->text().trimmed().toInt() / 2;
        lato1 = Coordinate::calcolaCoordinate(mappaConfig->locatoreCentra->text().trimmed(), -offset, -offset);
        lato2 = Coordinate::calcolaCoordinate(mappaConfig->locatoreCentra->text().trimmed(), offset, offset);
        mappa->setMatrice(lato1, lato2);
    }
}

void MainWindow::centraPredefinitoITA() {
    mappa->setMatrice("JM24XG", "JN98MP");
}
void MainWindow::centraPredefinitoMondo() {
    mappa->setMatrice("AC00AA", "RR89PX");
}
void MainWindow::centraPredefinitoEU() {
    mappa->setMatrice("IM32GM", "LQ22IV");
}
void MainWindow::centraPredefinitoAsia() {
    mappa->setMatrice("KJ55JB", "RQ89PX");
}
void MainWindow::centraPredefinitoAfrica() {
    mappa->setMatrice("HM69IF", "LF92KI");
}
void MainWindow::centraPredefinitoNordAmerica() {
    mappa->setMatrice("AQ51EK", "GJ84PJ");
}
void MainWindow::centraPredefinitoSudAmerica() {
    mappa->setMatrice("DK86GR", "HD51UL");
};
void MainWindow::centraPredefinitoOceania() {
    mappa->setMatrice("NK34HS", "RD95AX");
}

void MainWindow::cercaRegione(const QString &txt) {
    if (txt.isEmpty())
        return;

    QString q = QString(
                    "SELECT DISTINCT regione FROM locatori "
                    "WHERE stato = '%1' AND regione IS NOT NULL ORDER BY regione"
                    ).arg(DatabaseManager::escape(txt.trimmed()));

    DBResult *res = db->executeQuery(q);

    while (mappaConfig->regioneDB->count() > 1) {
        mappaConfig->regioneDB->removeItem(1);
    }
    while (mappaConfig->provinciaDB->count() > 1) {
        mappaConfig->provinciaDB->removeItem(1);
    }
    while (mappaConfig->comuneDB->count() > 1) {
        mappaConfig->comuneDB->removeItem(1);
    }
    mappaConfig->provinciaDB->setCurrentIndex(0);
    mappaConfig->comuneDB->setCurrentIndex(0);
    mappaConfig->locatoreDB->clear();

    for (const auto &row : res->tabella) {
        mappaConfig->regioneDB->addItem(row[0]);
    }

    delete res;
}

void MainWindow::cercaProvincia(const QString &txt) {
    if (txt.isEmpty())
        return;

    QString q = QString(
                    "SELECT DISTINCT provincia FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia IS NOT NULL ORDER BY provincia"
                    ).arg(
                        DatabaseManager::escape(mappaConfig->statoDB->currentText().trimmed()),
                        DatabaseManager::escape(txt.trimmed())
                        );

    DBResult *res = db->executeQuery(q);

    while (mappaConfig->provinciaDB->count() > 1) {
        mappaConfig->provinciaDB->removeItem(1);
    }
    while (mappaConfig->comuneDB->count() > 1) {
        mappaConfig->comuneDB->removeItem(1);
    }
    mappaConfig->comuneDB->setCurrentIndex(0);
    mappaConfig->locatoreDB->clear();

    for (const auto &row : res->tabella) {
        mappaConfig->provinciaDB->addItem(row[0]);
    }

    delete res;
}

void MainWindow::cercaComune(const QString &txt) {
    if (txt.isEmpty())
        return;

    QString q = QString(
                    "SELECT DISTINCT comune FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia = '%3' AND comune IS NOT NULL ORDER BY comune"
                    ).arg(
                        DatabaseManager::escape(mappaConfig->statoDB->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->regioneDB->currentText().trimmed()),
                        DatabaseManager::escape(txt.trimmed())
                        );

    DBResult *res = db->executeQuery(q);

    while (mappaConfig->comuneDB->count() > 1) {
        mappaConfig->comuneDB->removeItem(1);
    }
    mappaConfig->locatoreDB->clear();

    for (const auto &row : res->tabella) {
        mappaConfig->comuneDB->addItem(row[0]);
    }

    delete res;
}

void MainWindow::cercaLocatore(const QString &txt) {
    if (txt.isEmpty())
        return;

    QString q = QString(
                    "SELECT locatore FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia = '%3' AND comune = '%4' ORDER BY locatore"
                    ).arg(
                        DatabaseManager::escape(mappaConfig->statoDB->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->regioneDB->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->provinciaDB->currentText().trimmed()),
                        DatabaseManager::escape(txt.trimmed())
                        );

    DBResult *res = db->executeQuery(q);

    mappaConfig->locatoreDB->clear();

    for (const auto &row : res->tabella) {
        mappaConfig->locatoreDB->addItem(row[0]);
    }

    delete res;
}

void MainWindow::pulisciCerca() {
    mappaConfig->statoDB->setCurrentIndex(0);
    mappaConfig->regioneDB->setCurrentIndex(0);
    mappaConfig->provinciaDB->setCurrentIndex(0);
    mappaConfig->comuneDB->setCurrentIndex(0);
    mappaConfig->locatoreDB->clear();
}

void MainWindow::confermaCerca() {
    QString loc = mappaConfig->locatoreDB->currentText().trimmed();
    if(Coordinate::validaLocatore(loc)) {
        QString lato1, lato2;
        int offset = 20;
        lato1 = Coordinate::calcolaCoordinate(loc, -offset, -offset);
        lato2 = Coordinate::calcolaCoordinate(loc, offset, offset);
        mappa->setMatrice(lato1, lato2);

        mappaConfig->locatoreCentra->setText(loc);
        locatoreDaMappa(loc);
    }
}

