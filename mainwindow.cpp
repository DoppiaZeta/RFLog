#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "adif.h"

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

    ui->grigliaInput->addWidget(Nominativo, 1, 1);
    ui->grigliaInput->addWidget(Locatore, 1, 2);
    ui->grigliaInput->addWidget(Segnale, 1, 3);
    ui->grigliaInput->addWidget(Frequenza, 1, 4);
    ui->grigliaInput->addWidget(Orario, 1, 5);
    ui->grigliaVerticale->addWidget(Tabella);

    QWidget::setTabOrder(Nominativo, Locatore);
    QWidget::setTabOrder(Locatore, Segnale);
    QWidget::setTabOrder(Segnale, Frequenza);
    QWidget::setTabOrder(Frequenza, Orario);

    mappa = new Mappa(db, this);
    mappa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->grigliaDestra->addWidget(mappa, 10);
    QWidget *mappaConfigW = new QWidget(this);
    mappaConfig->setupUi(mappaConfigW);
    ui->grigliaDestra->addWidget(mappaConfigW);


    connect(ui->mappaGeografica, &QPushButton::clicked, this, &MainWindow::setGeografica);
    connect(ui->mappaPolitica, &QPushButton::clicked, this, &MainWindow::setPolitica);
    connect(ui->mappaScreenshot, &QPushButton::clicked, this, &MainWindow::mappaScreenshot);

    connect(mappa, &Mappa::mouseLocatore, this, &MainWindow::locatoreDaMappa);
    connect(mappa, &Mappa::mouseLocatoreDPPCLK, this, &MainWindow::locatoreDaMappaDPPCLK);

    connect(Nominativo, &QLineEdit::textChanged, this, &MainWindow::compilaNominativo);
    connect(Nominativo, &SuggestiveLineEdit::pressTab, this, &MainWindow::catturaTab);
    connect(Nominativo, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);

    connect(Locatore, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Segnale, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Frequenza, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Orario, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);

    mappaConfig->tabWidget->setCurrentIndex(0);

    connect(mappaConfig->locatoreCentraOK, &QPushButton::clicked, this, &MainWindow::centraDaLocatore);
    connect(mappa, &Mappa::matriceDaA, this, &MainWindow::caricaDaA);
    connect(mappaConfig->locatoreCentraDAA, &QPushButton::clicked, this, &MainWindow::centraDAA);
    connect(mappaConfig->locatoreCentraLinee, &QPushButton::clicked, this, &MainWindow::centraLinee);

    connect(mappaConfig->locatorePresetITA, &QPushButton::clicked, this, &MainWindow::centraPredefinitoITA);
    connect(mappaConfig->locatorePresetMondo, &QPushButton::clicked, this, &MainWindow::centraPredefinitoMondo);
    connect(mappaConfig->locatorePresetEU, &QPushButton::clicked, this, &MainWindow::centraPredefinitoEU);
    connect(mappaConfig->locatorePresetAsia, &QPushButton::clicked, this, &MainWindow::centraPredefinitoAsia);
    connect(mappaConfig->locatorePresetAfrica, &QPushButton::clicked, this, &MainWindow::centraPredefinitoAfrica);
    connect(mappaConfig->locatorePresetNordAmerica, &QPushButton::clicked, this, &MainWindow::centraPredefinitoNordAmerica);
    connect(mappaConfig->locatorePresetSudAmerica, &QPushButton::clicked, this, &MainWindow::centraPredefinitoSudAmerica);
    connect(mappaConfig->locatorePresetOceania, &QPushButton::clicked, this, &MainWindow::centraPredefinitoOceania);

    mappaConfig->locatoreOffsetValue->setText(QString::number(mappaConfig->locatoreOffset->value()));
    connect(mappaConfig->locatoreOffset, &QSlider::valueChanged, [this](int value) {
        mappaConfig->locatoreOffsetValue->setText(QString::number(value));
    });


    mappaConfig->statoDB->addItem(QString());
    mappaConfig->regioneDB->addItem(QString());
    mappaConfig->provinciaDB->addItem(QString());
    mappaConfig->comuneDB->addItem(QString());

    connect(mappaConfig->coloriStatoTXT, &QComboBox::textActivated, this, &MainWindow::caricaColoreStato);
    connect(mappaConfig->coloriRegioneTXT, &QComboBox::textActivated, this, &MainWindow::caricaColoreRegione);
    connect(mappaConfig->coloriProvinciaTXT, &QComboBox::textActivated, this, &MainWindow::caricaColoreProvincia);
    connect(mappaConfig->coloriComuneTXT, &QComboBox::textActivated, this, &MainWindow::caricaColoreComune);

    connect(mappaConfig->coloreStatoOK, &QPushButton::clicked, this, &MainWindow::salvaColoreStato);
    connect(mappaConfig->coloreRegioneOK, &QPushButton::clicked, this, &MainWindow::salvaColoreRegione);
    connect(mappaConfig->coloreProvinciaOK, &QPushButton::clicked, this, &MainWindow::salvaColoreProvincia);
    connect(mappaConfig->coloreComuneOK, &QPushButton::clicked, this, &MainWindow::salvaColoreComune);

    mappaConfig->coloreStato->addItem(tr("Grigio"));
    mappaConfig->coloreStato->addItem(tr("Viola"));
    mappaConfig->coloreStato->addItem(tr("Blu"));
    mappaConfig->coloreStato->addItem(tr("Verde"));
    mappaConfig->coloreStato->addItem(tr("Giallo"));
    mappaConfig->coloreStato->addItem(tr("Marrone"));
    mappaConfig->coloreStato->addItem(tr("Arancione"));

    for(int i = 0; i < 10; i++) {
        mappaConfig->coloreRegione->addItem(QString::number(i));
        mappaConfig->coloreProvincia->addItem(QString::number(i));
        mappaConfig->coloreComune->addItem(QString::number(i));
    }

    DBResult *res = caricaStatiDB();

    for(int i = 0; i < res->tabella.size(); i++) {
        mappaConfig->statoDB->addItem(res->tabella[i][0]);
        mappaConfig->coloriStatoTXT->addItem(res->tabella[i][0]);
    }

    if(res->hasRows())
        caricaColoreStato(mappaConfig->coloriStatoTXT->currentText());

    delete res;

    connect(mappaConfig->statoDB, &QComboBox::textActivated, this, &MainWindow::cercaRegione);
    connect(mappaConfig->regioneDB, &QComboBox::textActivated, this, &MainWindow::cercaProvincia);
    connect(mappaConfig->provinciaDB, &QComboBox::textActivated, this, &MainWindow::cercaComune);
    connect(mappaConfig->comuneDB, &QComboBox::textActivated, this, &MainWindow::cercaLocatore);
    connect(mappaConfig->pulisciDB, &QPushButton::clicked, this, &MainWindow::pulisciCerca);
    connect(mappaConfig->locatoreCentraDB, &QPushButton::clicked, this, &MainWindow::confermaCerca);


    connect(mappaConfig->modificaCerca, &QPushButton::clicked, this, &MainWindow::modificaCercaLocatore);
    connect(mappaConfig->modificaStato, &QComboBox::textActivated, this, &MainWindow::modificaCercaRegione);
    connect(mappaConfig->modificaRegione, &QComboBox::textActivated, this, &MainWindow::modificaCercaProvincia);
    connect(mappaConfig->modificaProvincia, &QComboBox::textActivated, this, &MainWindow::modificaCercaComune);
    connect(mappaConfig->modificaOK, &QPushButton::clicked, this, &MainWindow::modificaSalva);


    connect(ui->menuApriAdif, &QAction::triggered, this, &MainWindow::menuApriAdif);


    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &MainWindow::aggiornaOrario);
    t->start(1000);

    centraPredefinitoITA();
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

void MainWindow::setPolitica() {
    mappa->setTipoMappa(Mappa::tipoMappa::polica);
}

void MainWindow::setGeografica() {
    mappa->setTipoMappa(Mappa::tipoMappa::geografica);
}

void MainWindow::mappaScreenshot() {
    // Cattura il contenuto dell'OpenGLWidget come QImage
    QImage screenshot = mappa->grabFramebuffer();

    // Ottieni l'istanza degli appunti di sistema
    QClipboard *clipboard = QGuiApplication::clipboard();

    // Copia l'immagine negli appunti
    clipboard->setImage(screenshot);
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

DBResult * MainWindow::caricaInfoLocatore(const QString & loc) {
    if(!Coordinate::validaLocatore(loc))
        return new DBResult;

    QString q = QString(
                    "select locatore, lat_min, lat_max, lon_min, lon_max, stato, regione, provincia, comune, altezza from locatori where locatore = '%1'"
                    ).arg(DatabaseManager::escape(loc));
    return db->executeQuery(q);
}

void MainWindow::locatoreDaMappa(QString loc) {
    DBResult *res = caricaInfoLocatore(loc);
    mappaConfig->Locatore->setText(loc);

    if(res->hasRows()) {
        mappaConfig->Stato->setText(tr("Stato: ") + res->tabella[0][5]);
        mappaConfig->Regione->setText(tr("Regione: ") + res->tabella[0][6]);
        mappaConfig->Provincia->setText(tr("Provincia: ") + res->tabella[0][7]);
        mappaConfig->Comune->setText(tr("Comune: ") + res->tabella[0][8]);
        mappaConfig->lat_min->setText(res->tabella[0][1]);
        mappaConfig->lat_max->setText(res->tabella[0][2]);
        mappaConfig->lon_min->setText(res->tabella[0][3]);
        mappaConfig->lon_max->setText(res->tabella[0][4]);
        mappaConfig->altezza->setText(res->tabella[0][9]);
    } else {
        QString vuoto;
        mappaConfig->Stato->setText(vuoto);
        mappaConfig->Regione->setText(vuoto);
        mappaConfig->Provincia->setText(vuoto);
        mappaConfig->Comune->setText(vuoto);
        mappaConfig->lat_min->setText(vuoto);
        mappaConfig->lat_max->setText(vuoto);
        mappaConfig->lon_min->setText(vuoto);
        mappaConfig->lon_max->setText(vuoto);
        mappaConfig->altezza->setText(vuoto);
    }

    mappaConfig->locatoreCentra->setText(loc);

    mappaConfig->modificaLocatore->setText(loc);
    modificaCercaLocatore();

    delete res;
}

void MainWindow::modificaCercaLocatore() {
    QString loc = mappaConfig->modificaLocatore->text().trimmed();

    DBResult *res = caricaInfoLocatore(loc);
    if(res->hasRows()) {
        mappaConfig->modificaAltezza->setValue(res->tabella[0][9].toFloat());
    }
    delete res;

    if(Coordinate::validaLocatore(loc)) {
        DBResult *dbRS = caricaStatiDB();
        if(dbRS->hasRows()) {
            mappaConfig->modificaStato->clear();
            mappaConfig->modificaStato->addItem(QString());
            for(int i = 0; i < dbRS->tabella.size(); i++) {
                mappaConfig->modificaStato->addItem(dbRS->tabella[i][0]);
            }

            DBResult *locAttuale = caricaInfoLocatore(loc);
            if(locAttuale->hasRows())
                mappaConfig->modificaStato->setCurrentText(locAttuale->tabella[0][5]);
            else
                mappaConfig->modificaStato->setCurrentIndex(0);

            caricaModificaRegione();

            if(locAttuale->hasRows())
                mappaConfig->modificaRegione->setCurrentText(locAttuale->tabella[0][6]);
            else
                mappaConfig->modificaRegione->setCurrentIndex(0);

            caricaModificaProvincia();

            if(locAttuale->hasRows())
                mappaConfig->modificaProvincia->setCurrentText(locAttuale->tabella[0][7]);
            else
                mappaConfig->modificaProvincia->setCurrentIndex(0);

            caricaModificaComune();

            if(locAttuale->hasRows())
                mappaConfig->modificaComune->setCurrentText(locAttuale->tabella[0][8]);
            else
                mappaConfig->modificaComune->setCurrentIndex(0);

            delete locAttuale;
        }
        delete dbRS;
    }
}

void MainWindow::modificaCercaRegione(const QString & txt) {
    Q_UNUSED(txt);

    if(!mappaConfig->modificaAutoCompleta->isChecked())
        return;

    QString loc = mappaConfig->modificaLocatore->text().trimmed();
    if(Coordinate::validaLocatore(loc)) {
        DBResult *locAttuale = caricaInfoLocatore(loc);

        caricaModificaRegione();

        if(locAttuale->hasRows())
            mappaConfig->modificaRegione->setCurrentText(locAttuale->tabella[0][6]);
        else
            mappaConfig->modificaRegione->setCurrentIndex(0);

        caricaModificaProvincia();

        if(locAttuale->hasRows())
            mappaConfig->modificaProvincia->setCurrentText(locAttuale->tabella[0][7]);
        else
            mappaConfig->modificaProvincia->setCurrentIndex(0);

        caricaModificaComune();

        if(locAttuale->hasRows())
            mappaConfig->modificaComune->setCurrentText(locAttuale->tabella[0][8]);
        else
            mappaConfig->modificaComune->setCurrentIndex(0);

        delete locAttuale;
    }
}

void MainWindow::modificaCercaProvincia(const QString & txt) {
    Q_UNUSED(txt);
    if(!mappaConfig->modificaAutoCompleta->isChecked())
        return;

    QString loc = mappaConfig->modificaLocatore->text().trimmed();
    if(Coordinate::validaLocatore(loc)) {
        DBResult *locAttuale = caricaInfoLocatore(loc);

        caricaModificaProvincia();

        if(locAttuale->hasRows())
            mappaConfig->modificaProvincia->setCurrentText(locAttuale->tabella[0][7]);
        else
            mappaConfig->modificaProvincia->setCurrentIndex(0);

        caricaModificaComune();

        if(locAttuale->hasRows())
            mappaConfig->modificaComune->setCurrentText(locAttuale->tabella[0][8]);
        else
            mappaConfig->modificaComune->setCurrentIndex(0);

        delete locAttuale;
    }
}

void MainWindow::modificaCercaComune(const QString & txt) {
    Q_UNUSED(txt);
    if(!mappaConfig->modificaAutoCompleta->isChecked())
        return;

    QString loc = mappaConfig->modificaLocatore->text().trimmed();
    if(Coordinate::validaLocatore(loc)) {
        DBResult *locAttuale = caricaInfoLocatore(loc);

        caricaModificaComune();

        if(locAttuale->hasRows())
            mappaConfig->modificaComune->setCurrentText(locAttuale->tabella[0][8]);
        else
            mappaConfig->modificaComune->setCurrentIndex(0);

        delete locAttuale;
    }
}

void MainWindow::modificaSalva() {
    QString loc = mappaConfig->modificaLocatore->text().trimmed();
    if(Coordinate::validaLocatore(loc)) {
        QString q = QString(
                        "INSERT INTO locatori (locatore, stato, regione, provincia, comune, altezza) "
                        "VALUES ('%1', '%2', '%3', '%4', '%5', '%6') "
                        "ON CONFLICT(locatore) DO UPDATE SET "
                        "stato = '%2', "
                        "regione = '%3', "
                        "provincia = '%4', "
                        "comune = '%5', "
                        "altezza = '%6' "
                        ).arg(
                            DatabaseManager::escape(loc),
                            DatabaseManager::escape(mappaConfig->modificaStato->currentText().trimmed()),
                            DatabaseManager::escape(mappaConfig->modificaRegione->currentText().trimmed()),
                            DatabaseManager::escape(mappaConfig->modificaProvincia->currentText().trimmed()),
                            DatabaseManager::escape(mappaConfig->modificaComune->currentText().trimmed()),
                            DatabaseManager::escape(QString::number(mappaConfig->modificaAltezza->value()))
                            );

        db->executeQueryNoRes(q);
        mappa->reload();
    }
}

void MainWindow::caricaModificaRegione() {
    DBResult *dbRS = caricaRegioniDB(mappaConfig->modificaStato->currentText().trimmed());
    if(dbRS->hasRows()) {
        mappaConfig->modificaRegione->clear();
        mappaConfig->modificaRegione->addItem(QString());
        for(int i = 0; i < dbRS->tabella.size(); i++) {
            mappaConfig->modificaRegione->addItem(dbRS->tabella[i][0]);
        }
    }
    delete dbRS;
}

void MainWindow::caricaModificaProvincia() {
    DBResult *dbRS = caricaProvinceDB(mappaConfig->modificaStato->currentText().trimmed(), mappaConfig->modificaRegione->currentText().trimmed());
    if(dbRS->hasRows()) {
        mappaConfig->modificaProvincia->clear();
        mappaConfig->modificaProvincia->addItem(QString());
        for(int i = 0; i < dbRS->tabella.size(); i++) {
            mappaConfig->modificaProvincia->addItem(dbRS->tabella[i][0]);
        }
    }
    delete dbRS;
}

void MainWindow::caricaModificaComune() {
    DBResult *dbRS = caricaComuniDB(mappaConfig->modificaStato->currentText().trimmed(), mappaConfig->modificaRegione->currentText().trimmed(), mappaConfig->modificaProvincia->currentText().trimmed());
    if(dbRS->hasRows()) {
        mappaConfig->modificaComune->clear();
        mappaConfig->modificaComune->addItem(QString());
        for(int i = 0; i < dbRS->tabella.size(); i++) {
            mappaConfig->modificaComune->addItem(dbRS->tabella[i][0]);
        }
    }
    delete dbRS;
}

void MainWindow::locatoreDaMappaDPPCLK(QString loc) {
    locatoreDaMappa(loc);
    centraDaLocatore();
}

void MainWindow::centraDaLocatore() {
    QString loc = mappaConfig->locatoreCentra->text().trimmed().toUpper();
    if(Coordinate::validaLocatore(loc)) {
        QString lato1, lato2;
        int offset = mappaConfig->locatoreOffset->value() / 2;
        lato1 = Coordinate::calcolaCoordinate(loc, -offset, -offset);
        lato2 = Coordinate::calcolaCoordinate(loc, offset, offset);
        mappa->setMatrice(lato1, lato2);
    }
}

void MainWindow::centraPredefinitoITA() {
    mappa->setMatrice("JM06UL", "KN17QH");
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

DBResult * MainWindow::caricaStatiDB() {
    QString q = QString("select distinct stato from locatori where stato is not null order by stato");

    return db->executeQuery(q);
}

DBResult * MainWindow::caricaRegioniDB(const QString & stato) {
    if(stato.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT DISTINCT regione FROM locatori "
                    "WHERE stato = '%1' AND regione IS NOT NULL ORDER BY regione"
                    ).arg(DatabaseManager::escape(stato.trimmed()));

    return db->executeQuery(q);
}

void MainWindow::cercaRegione(const QString &txt) {
    if (txt.isEmpty())
        return;

    DBResult *res = caricaRegioniDB(txt);

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

DBResult * MainWindow::caricaProvinceDB(const QString & stato, const QString & regione) {
    if(stato.isEmpty() || regione.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT DISTINCT provincia FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia IS NOT NULL ORDER BY provincia"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed()),
                        DatabaseManager::escape(regione.trimmed())
                        );

    return db->executeQuery(q);
}

void MainWindow::cercaProvincia(const QString &txt) {
    if (txt.isEmpty())
        return;

    DBResult *res = caricaProvinceDB(mappaConfig->statoDB->currentText(), txt);

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

DBResult * MainWindow::caricaComuniDB(const QString & stato, const QString & regione, const QString & provincia) {
    if(stato.isEmpty() || regione.isEmpty() || provincia.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT DISTINCT comune FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia = '%3' AND comune IS NOT NULL ORDER BY comune"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed()),
                        DatabaseManager::escape(regione.trimmed()),
                        DatabaseManager::escape(provincia.trimmed())
                        );

    return db->executeQuery(q);
}

void MainWindow::cercaComune(const QString &txt) {
    if (txt.isEmpty())
        return;

    DBResult *res = caricaComuniDB(mappaConfig->statoDB->currentText(), mappaConfig->regioneDB->currentText(), txt);

    while (mappaConfig->comuneDB->count() > 1) {
        mappaConfig->comuneDB->removeItem(1);
    }
    mappaConfig->locatoreDB->clear();

    for (const auto &row : res->tabella) {
        mappaConfig->comuneDB->addItem(row[0]);
    }

    delete res;
}

DBResult * MainWindow::caricaLocatoriDB(const QString & stato, const QString & regione, const QString & provincia, const QString & comune) {
    if(stato.isEmpty() || regione.isEmpty() || provincia.isEmpty() || comune.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT locatore FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia = '%3' AND comune = '%4' ORDER BY locatore"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed()),
                        DatabaseManager::escape(regione.trimmed()),
                        DatabaseManager::escape(provincia.trimmed()),
                        DatabaseManager::escape(comune.trimmed())
                        );

    return db->executeQuery(q);
}

void MainWindow::cercaLocatore(const QString &txt) {
    if (txt.isEmpty())
        return;

    DBResult *res = caricaLocatoriDB(mappaConfig->statoDB->currentText(), mappaConfig->regioneDB->currentText(), mappaConfig->provinciaDB->currentText(), txt);

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
    QString loc = mappaConfig->locatoreDB->currentText().trimmed().toUpper();
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

void MainWindow::caricaDaA(QString da, QString a) {
    mappaConfig->locatoreDA->setText(da);
    mappaConfig->locatoreA->setText(a);
}

void MainWindow::centraDAA() {
    QString da, a;
    da = mappaConfig->locatoreDA->text().trimmed().toUpper();
    a = mappaConfig->locatoreA->text().trimmed().toUpper();
    if(Coordinate::validaLocatore(da) && Coordinate::validaLocatore(a))
        mappa->setMatrice(da, a);
}

void MainWindow::centraLinee() {
    mappa->adattaMappaLinee();
}

DBResult * MainWindow::caricaColoreStatoDB(const QString & stato) {
    if(stato.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT colore_stato FROM locatori "
                    "WHERE stato = '%1' ORDER BY stato limit 1"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed())
                        );

    return db->executeQuery(q);
}

DBResult * MainWindow::caricaColoreRegioneDB(const QString & stato, const QString & regione) {
    if(stato.isEmpty() || regione.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT colore_regione FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' ORDER BY regione limit 1"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed()),
                        DatabaseManager::escape(regione.trimmed())
                        );

    return db->executeQuery(q);
}

DBResult * MainWindow::caricaColoreProvinciaDB(const QString & stato, const QString & regione, const QString & provincia) {
    if(stato.isEmpty() || regione.isEmpty() || provincia.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT colore_provincia FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia = '%3' ORDER BY provincia limit 1"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed()),
                        DatabaseManager::escape(regione.trimmed()),
                        DatabaseManager::escape(provincia.trimmed())
                        );

    return db->executeQuery(q);
}

DBResult * MainWindow::caricaColoreComuneDB(const QString & stato, const QString & regione, const QString & provincia, const QString & comune) {
    if(stato.isEmpty() || regione.isEmpty() || provincia.isEmpty() || comune.isEmpty())
        return new DBResult();

    QString q = QString(
                    "SELECT colore_comune FROM locatori "
                    "WHERE stato = '%1' AND regione = '%2' AND provincia = '%3' AND comune = '%4' ORDER BY comune limit 1"
                    ).arg(
                        DatabaseManager::escape(stato.trimmed()),
                        DatabaseManager::escape(regione.trimmed()),
                        DatabaseManager::escape(provincia.trimmed()),
                        DatabaseManager::escape(comune.trimmed())
                        );

    return db->executeQuery(q);
}

void MainWindow::caricaColoreStato(const QString & txt) {
    DBResult *colore = caricaColoreStatoDB(txt);
    if(colore->hasRows()) {
        mappaConfig->coloreStato->setCurrentIndex(colore->tabella[0][0].toInt());

        DBResult * tabella = caricaRegioniDB(txt);
        mappaConfig->coloriRegioneTXT->clear();

        if(tabella->hasRows()) {
            for(int i = 0; i < tabella->tabella.size(); i++)
                mappaConfig->coloriRegioneTXT->addItem(tabella->tabella[i][0]);
            mappaConfig->coloriRegioneTXT->setCurrentIndex(0);
            caricaColoreRegione(tabella->tabella[0][0]);
        }
        delete tabella;
    }
    delete colore;
}

void MainWindow::caricaColoreRegione(const QString & txt) {
    DBResult *colore = caricaColoreRegioneDB(mappaConfig->coloriStatoTXT->currentText(), txt);
    if(colore->hasRows()) {
        mappaConfig->coloreRegione->setCurrentIndex(colore->tabella[0][0].toInt());

        DBResult * tabella = caricaProvinceDB(mappaConfig->coloriStatoTXT->currentText(), txt);
        mappaConfig->coloriProvinciaTXT->clear();

        if(tabella->hasRows()) {
            for(int i = 0; i < tabella->tabella.size(); i++)
                mappaConfig->coloriProvinciaTXT->addItem(tabella->tabella[i][0]);
            mappaConfig->coloriProvinciaTXT->setCurrentIndex(0);
            caricaColoreProvincia(tabella->tabella[0][0]);
        }
        delete tabella;
    }
    delete colore;
}

void MainWindow::caricaColoreProvincia(const QString & txt) {
    DBResult *colore = caricaColoreProvinciaDB(mappaConfig->coloriStatoTXT->currentText(), mappaConfig->coloriRegioneTXT->currentText(), txt);
    if(colore->hasRows()) {
        mappaConfig->coloreProvincia->setCurrentIndex(colore->tabella[0][0].toInt());

        DBResult * tabella = caricaComuniDB(mappaConfig->coloriStatoTXT->currentText(), mappaConfig->coloriRegioneTXT->currentText(), txt);
        mappaConfig->coloriComuneTXT->clear();

        if(tabella->hasRows()) {
            for(int i = 0; i < tabella->tabella.size(); i++)
                mappaConfig->coloriComuneTXT->addItem(tabella->tabella[i][0]);
            mappaConfig->coloriComuneTXT->setCurrentIndex(0);
            caricaColoreComune(tabella->tabella[0][0]);
        }
        delete tabella;
    }
    delete colore;
}

void MainWindow::caricaColoreComune(const QString & txt) {
    DBResult *colore = caricaColoreComuneDB(mappaConfig->coloriStatoTXT->currentText(), mappaConfig->coloriRegioneTXT->currentText(), mappaConfig->coloriProvinciaTXT->currentText(), txt);
    if(colore->hasRows())
        mappaConfig->coloreComune->setCurrentIndex(colore->tabella[0][0].toInt());
    delete colore;
}

void MainWindow::salvaColoreStato() {
    QString q = QString(
                    "update locatori set colore_stato = %1 "
                    "WHERE stato = '%2'"
                    ).arg(
                        DatabaseManager::escape(QString::number(mappaConfig->coloreStato->currentIndex())),
                        DatabaseManager::escape(mappaConfig->coloriStatoTXT->currentText().trimmed())
                        );

    db->executeQueryNoRes(q);
    mappa->reload();
}

void MainWindow::salvaColoreRegione() {
    QString q = QString(
                    "update locatori set colore_regione = %1 "
                    "WHERE stato = '%2' and regione = '%3'"
                    ).arg(
                        DatabaseManager::escape(QString::number(mappaConfig->coloreRegione->currentIndex())),
                        DatabaseManager::escape(mappaConfig->coloriStatoTXT->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->coloriRegioneTXT->currentText().trimmed())
                        );

    db->executeQueryNoRes(q);
    mappa->reload();
}

void MainWindow::salvaColoreProvincia() {
    QString q = QString(
                    "update locatori set colore_provincia = %1 "
                    "WHERE stato = '%2' and regione = '%3' and provincia = '%4'"
                    ).arg(
                        DatabaseManager::escape(QString::number(mappaConfig->coloreProvincia->currentIndex())),
                        DatabaseManager::escape(mappaConfig->coloriStatoTXT->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->coloriRegioneTXT->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->coloriProvinciaTXT->currentText().trimmed())
                        );

    db->executeQueryNoRes(q);
    mappa->reload();
}

void MainWindow::salvaColoreComune() {
    QString q = QString(
                    "update locatori set colore_provincia = %1 "
                    "WHERE stato = '%2' and regione = '%3' and provincia = '%4' and comune = '%5'"
                    ).arg(
                        DatabaseManager::escape(QString::number(mappaConfig->coloreComune->currentIndex())),
                        DatabaseManager::escape(mappaConfig->coloriStatoTXT->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->coloriRegioneTXT->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->coloriProvinciaTXT->currentText().trimmed()),
                        DatabaseManager::escape(mappaConfig->coloriComuneTXT->currentText().trimmed())
                        );

    db->executeQueryNoRes(q);
    mappa->reload();
}

void MainWindow::menuApriAdif() {
    // Imposta il filtro per mostrare solo i file .ADF
    QString filter = "ADIF files (*.adf *.adi)";
    // Apre il dialogo di selezione file, impostando la directory iniziale su quella di default dell'utente
    QString filePath = QFileDialog::getOpenFileName(this, "Seleziona File ADIF", QDir::homePath(), filter);

    Adif ad;
    Adif::parseAdif(filePath, ad);

    // Supponendo che tu abbia già un'istanza di Adif chiamata adif
    const auto& contatti = ad.getContatti();
    for (const auto& contatto : contatti) {
        QString myGridSquare = contatto.value("my_gridsquare");
        QString gridSquare = contatto.value("gridsquare");

        mappa->addLinea(Linee(myGridSquare, gridSquare), false);
    }

    mappa->adattaMappaLinee();
}

