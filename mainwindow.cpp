#include <QGridLayout>
#include <QLocale>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>

#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "adif.h"
#include "locatoripreferiti.h"
#include "nuovolog.h"
#include "miaradio.h"
#include "mappasrpc.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mappaConfig(new Ui::MappaConfig)
    , tx(new Ui::Tx)
{
    db = new DatabaseManager("locatori_fine.db", this);
    RFLog = new DatabaseManager("RFLog.db", this);
    nDB = new DatabaseManager("nominativi.db", this);

    creaDBRFLog();

    numeroLog = 0;

    ui->setupUi(this);
    tx->setupUi(ui->tx);

    DBResult *res;

    QString formato = "border: none;\n"
                      "color: blue;\n"
                      "font-weight: bold;\n";

    Nominativo = new SuggestiveLineEdit(this);
    Locatore = new SuggestiveLineEdit(this);
    Segnale = new SuggestiveLineEdit(this);
    Frequenza = new SuggestiveLineEdit(this);
    Orario = new SuggestiveLineEdit(this);

    QStringList suggestions = {"in3kgw", "in3ivc", "in3ktt", "in3hkz"};
    Nominativo->setCompleter(suggestions);

    Nominativo->setStyleSheet(formato);
    Locatore->setStyleSheet(formato);
    Segnale->setStyleSheet(formato);
    Frequenza->setStyleSheet(formato);
    Orario->setStyleSheet(formato);

    ui->grigliaInput->addWidget(Nominativo, 1, 1);
    ui->grigliaInput->addWidget(Locatore, 1, 2);
    ui->grigliaInput->addWidget(Segnale, 1, 3);
    ui->grigliaInput->addWidget(Frequenza, 1, 4);
    ui->grigliaInput->addWidget(Orario, 1, 5);

    QWidget::setTabOrder(Nominativo, Locatore);
    QWidget::setTabOrder(Locatore, Segnale);
    QWidget::setTabOrder(Segnale, Frequenza);
    QWidget::setTabOrder(Frequenza, Orario);

    QWidget *mappaConfigW = new QWidget(this);
    mappaConfig->setupUi(mappaConfigW);

    mappa = new Mappa(db, mappaConfigW, this);
    ui->grigliaDestra->addWidget(mappa, 10);

    mappa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->grigliaDestra->addWidget(mappaConfigW);


    connect(ui->mappaGeografica, &QPushButton::clicked, this, &MainWindow::setGeografica);
    connect(ui->mappaPolitica, &QPushButton::clicked, this, &MainWindow::setPolitica);
    connect(ui->mappaGruppo, &QPushButton::clicked, this, &MainWindow::mappaGruppoSRPC);
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


    ui->Tabella->setColumnCount(8);
    ui->Tabella->setHorizontalHeaderLabels(
        QStringList() << tr("TX Locatore")
                      << tr("TX")
                      << tr("RX Locatore")
                      << tr("RX")
                      << tr("TX dettagli")
                      << tr("Data/Ora UTC")
                      << tr("QSL")
                      << tr("INFO")
        );

    ui->Tabella->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);


    mappaConfig->tabWidget->setCurrentIndex(0);

    connect(mappaConfig->locatoreCentraOK, &QPushButton::clicked, this, &MainWindow::centraDaLocatore);
    connect(mappa, &Mappa::matriceDaA, this, &MainWindow::caricaDaA);
    connect(mappaConfig->locatoreCentraDAA, &QPushButton::clicked, this, &MainWindow::centraDAA);
    connect(mappaConfig->locatoreCentraLinee, &QPushButton::clicked, this, &MainWindow::centraLinee);
    connect(mappaConfig->centraGruppo, &QPushButton::clicked, this, &MainWindow::centraGruppo);

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

    res = caricaStatiDB();

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

    //menu
    connect(ui->menuApriAdif, &QAction::triggered, this, &MainWindow::menuApriAdif);
    connect(ui->menuLocatoriPreferiti, &QAction::triggered, this, &MainWindow::menuLocatoriPreferiti);
    connect(ui->menuIniziaLog, &QAction::triggered, this, &MainWindow::menuIniziaLog);
    connect(ui->menuMieRadio, &QAction::triggered, this, &MainWindow::menuMiaRadio);
    connect(ui->menuInformazioniSu, &QAction::triggered, this, &MainWindow::menuInformazioniSu);

    //top widget
    connect(tx->preferitiOK, &QPushButton::clicked, this, &MainWindow::usaLocatorePreferito);
    connect(tx->preferiti, &QTableWidget::doubleClicked, this, &MainWindow::usaLocatorePreferito);
    connect(tx->aggiungi, &QPushButton::clicked, this, &MainWindow::aggiungiNominativo);
    connect(tx->togli, &QPushButton::clicked, this, &MainWindow::eliminaNominativo);
    connect(tx->nominativo, &QComboBox::textActivated, this, &MainWindow::setSelectedNominativoDB);


    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &MainWindow::aggiornaOrario);
    t->start(1000);

    tx->preferiti->setColumnCount(2); // Imposta il numero di colonne
    // Riduci l'altezza delle righe
    tx->preferiti->verticalHeader()->setDefaultSectionSize(10);
    // Riduci il font
    QFont font = tx->preferiti->font();
    font.setPointSize(8); // Dimensione più piccola
    tx->preferiti->setFont(font);
    // streccia
    tx->preferiti->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    tx->nominativiList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    caricaLocatoriPreferiti();
    caricaMieRadio();
    caricaNominativiDaDb();
    centraPredefinitoITA();
}


MainWindow::~MainWindow()
{
    delete ui;
    delete mappaConfig;
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
    // Cattura il contenuto del widget come QImage
    QImage screenshot = mappa->grab().toImage();

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
    /*
    QString nominativoText = Nominativo->text().trimmed(); // Elimina spazi inutili
    if (nominativoText.isEmpty()) {
        return; // Interrompe l'inserimento
    }

    // Controlla se il nominativo è già presente
    bool nominativoDuplicato = false;
    for (int row = 0; row < ui->Tabella->rowCount(); ++row) {
        QTableWidgetItem *item = ui->Tabella->item(row, 0); // Colonna del nominativo
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
    ui->Tabella->insertRow(0); // Inserisce la nuova riga nella posizione 0

    // Inserisci dati nella nuova riga
    QTableWidgetItem *nominativoItem = new QTableWidgetItem(nominativoText);
    ui->Tabella->setItem(0, 0, nominativoItem);
    ui->Tabella->setItem(0, 1, new QTableWidgetItem(Locatore->text().trimmed()));
    ui->Tabella->setItem(0, 2, new QTableWidgetItem(Segnale->text().trimmed()));
    ui->Tabella->setItem(0, 3, new QTableWidgetItem(Frequenza->text().trimmed()));
    ui->Tabella->setItem(0, 4, new QTableWidgetItem(currentDateTimeGMT));

    // Se duplicato, colora la nuova riga di rosso
    if (nominativoDuplicato) {
        for (int col = 0; col < ui->Tabella->columnCount(); ++col) {
            QTableWidgetItem *rowItem = ui->Tabella->item(0, col);
            if (rowItem) {
                rowItem->setBackground(QColor(190, 190, 255));
            }
        }
    }

    if(!nominativoDuplicato && Coordinate::validaLocatore(Locatore->text().trimmed())) {
        mappa->addLinea(Linee("JN56PK", Locatore->text().trimmed().left(6)));
    }
*/
    aggiornaTabella();
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

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "select locatore, lat_min, lat_max, lon_min, lon_max, stato, regione, provincia, comune, altezza "
        "from locatori where locatore = :locatore"
    );
    q->bindValue(":locatore", loc);
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
}

void MainWindow::locatoreDaMappa(QString loc) {
    QString vuoto;
    DBResult *res = caricaInfoLocatore(loc);
    mappaConfig->Locatore->setText(loc);
    Coordinate::CqItu cqitu = Coordinate::getCqItu(loc);

    if(cqitu.cq > 0) {
        mappaConfig->cq->setText(QString::number(cqitu.cq));
    } else {
        mappaConfig->cq->setText(vuoto);
    }

    if(cqitu.itu > 0) {
        mappaConfig->itu->setText(QString::number(cqitu.itu));
    } else {
        mappaConfig->itu->setText(vuoto);
    }

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
        QSqlQuery *q = db->getQueryBind();
        q->prepare(
            "INSERT INTO locatori (locatore, stato, regione, provincia, comune, altezza) "
            "VALUES (:locatore, :stato, :regione, :provincia, :comune, :altezza) "
            "ON CONFLICT(locatore) DO UPDATE SET "
            "stato = :stato, "
            "regione = :regione, "
            "provincia = :provincia, "
            "comune = :comune, "
            "altezza = :altezza"
        );
        q->bindValue(":locatore", loc);
        q->bindValue(":stato", mappaConfig->modificaStato->currentText().trimmed());
        q->bindValue(":regione", mappaConfig->modificaRegione->currentText().trimmed());
        q->bindValue(":provincia", mappaConfig->modificaProvincia->currentText().trimmed());
        q->bindValue(":comune", mappaConfig->modificaComune->currentText().trimmed());
        q->bindValue(":altezza", mappaConfig->modificaAltezza->value());
        db->executeQueryNoRes(q);
        delete q;
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

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT DISTINCT regione FROM locatori "
        "WHERE stato = :stato AND regione IS NOT NULL ORDER BY regione"
    );
    q->bindValue(":stato", stato.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
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

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT DISTINCT provincia FROM locatori "
        "WHERE stato = :stato AND regione = :regione AND provincia IS NOT NULL ORDER BY provincia"
    );
    q->bindValue(":stato", stato.trimmed());
    q->bindValue(":regione", regione.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
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

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT DISTINCT comune FROM locatori "
        "WHERE stato = :stato AND regione = :regione AND provincia = :provincia AND comune IS NOT NULL ORDER BY comune"
    );
    q->bindValue(":stato", stato.trimmed());
    q->bindValue(":regione", regione.trimmed());
    q->bindValue(":provincia", provincia.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
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

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT locatore FROM locatori "
        "WHERE stato = :stato AND regione = :regione AND provincia = :provincia AND comune = :comune "
        "ORDER BY locatore"
    );
    q->bindValue(":stato", stato.trimmed());
    q->bindValue(":regione", regione.trimmed());
    q->bindValue(":provincia", provincia.trimmed());
    q->bindValue(":comune", comune.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
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

void MainWindow::centraGruppo() {
    if(
        mappa->getTipo() == Mappa::tipoMappa::stati ||
        mappa->getTipo() == Mappa::tipoMappa::regioni ||
        mappa->getTipo() == Mappa::tipoMappa::provincie ||
        mappa->getTipo() == Mappa::tipoMappa::comuni
        ) {
    QSqlQuery *q = db->getQueryBind();
    q->prepare(R"(
SELECT
  MIN(lat_min) AS lat_min,
  MAX(lat_max) AS lat_max,
  MIN(lon_min) AS lon_min,
  MAX(lon_max) AS lon_max
FROM locatori
WHERE stato = :stato
)");
    q->bindValue(":stato", mappa->getStato());
    DBResult *res = db->executeQuery(q);
    mappaConfig->locatoreDA->setText(Coordinate::calcolaLocatoreLatLon(res->getCella("lat_min").toDouble(), res->getCella("lon_min").toDouble()));
    mappaConfig->locatoreA->setText(Coordinate::calcolaLocatoreLatLon(res->getCella("lat_max").toDouble(), res->getCella("lon_max").toDouble()));
    delete res;
    delete q;

    centraDAA();
    }
}

DBResult * MainWindow::caricaColoreStatoDB(const QString & stato) {
    if(stato.isEmpty())
        return new DBResult();

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT colore_stato FROM locatori "
        "WHERE stato = :stato ORDER BY stato limit 1"
    );
    q->bindValue(":stato", stato.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
}

DBResult * MainWindow::caricaColoreRegioneDB(const QString & stato, const QString & regione) {
    if(stato.isEmpty() || regione.isEmpty())
        return new DBResult();

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT colore_regione FROM locatori "
        "WHERE stato = :stato AND regione = :regione ORDER BY regione limit 1"
    );
    q->bindValue(":stato", stato.trimmed());
    q->bindValue(":regione", regione.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
}

DBResult * MainWindow::caricaColoreProvinciaDB(const QString & stato, const QString & regione, const QString & provincia) {
    if(stato.isEmpty() || regione.isEmpty() || provincia.isEmpty())
        return new DBResult();

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT colore_provincia FROM locatori "
        "WHERE stato = :stato AND regione = :regione AND provincia = :provincia ORDER BY provincia limit 1"
    );
    q->bindValue(":stato", stato.trimmed());
    q->bindValue(":regione", regione.trimmed());
    q->bindValue(":provincia", provincia.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
}

DBResult * MainWindow::caricaColoreComuneDB(const QString & stato, const QString & regione, const QString & provincia, const QString & comune) {
    if(stato.isEmpty() || regione.isEmpty() || provincia.isEmpty() || comune.isEmpty())
        return new DBResult();

    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "SELECT colore_comune FROM locatori "
        "WHERE stato = :stato AND regione = :regione AND provincia = :provincia AND comune = :comune "
        "ORDER BY comune limit 1"
    );
    q->bindValue(":stato", stato.trimmed());
    q->bindValue(":regione", regione.trimmed());
    q->bindValue(":provincia", provincia.trimmed());
    q->bindValue(":comune", comune.trimmed());
    DBResult *res = db->executeQuery(q);
    delete q;
    return res;
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
    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "update locatori set colore_stato = :colore "
        "WHERE stato = :stato"
    );
    q->bindValue(":colore", mappaConfig->coloreStato->currentIndex());
    q->bindValue(":stato", mappaConfig->coloriStatoTXT->currentText().trimmed());
    db->executeQueryNoRes(q);
    delete q;
    mappa->reload();
}

void MainWindow::salvaColoreRegione() {
    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "update locatori set colore_regione = :colore "
        "WHERE stato = :stato and regione = :regione"
    );
    q->bindValue(":colore", mappaConfig->coloreRegione->currentIndex());
    q->bindValue(":stato", mappaConfig->coloriStatoTXT->currentText().trimmed());
    q->bindValue(":regione", mappaConfig->coloriRegioneTXT->currentText().trimmed());
    db->executeQueryNoRes(q);
    delete q;
    mappa->reload();
}

void MainWindow::salvaColoreProvincia() {
    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "update locatori set colore_provincia = :colore "
        "WHERE stato = :stato and regione = :regione and provincia = :provincia"
    );
    q->bindValue(":colore", mappaConfig->coloreProvincia->currentIndex());
    q->bindValue(":stato", mappaConfig->coloriStatoTXT->currentText().trimmed());
    q->bindValue(":regione", mappaConfig->coloriRegioneTXT->currentText().trimmed());
    q->bindValue(":provincia", mappaConfig->coloriProvinciaTXT->currentText().trimmed());
    db->executeQueryNoRes(q);
    delete q;
    mappa->reload();
}

void MainWindow::salvaColoreComune() {
    QSqlQuery *q = db->getQueryBind();
    q->prepare(
        "update locatori set colore_provincia = :colore "
        "WHERE stato = :stato and regione = :regione and provincia = :provincia and comune = :comune"
    );
    q->bindValue(":colore", mappaConfig->coloreComune->currentIndex());
    q->bindValue(":stato", mappaConfig->coloriStatoTXT->currentText().trimmed());
    q->bindValue(":regione", mappaConfig->coloriRegioneTXT->currentText().trimmed());
    q->bindValue(":provincia", mappaConfig->coloriProvinciaTXT->currentText().trimmed());
    q->bindValue(":comune", mappaConfig->coloriComuneTXT->currentText().trimmed());
    db->executeQueryNoRes(q);
    delete q;
    mappa->reload();
}

void MainWindow::menuApriAdif() {
    // Imposta il filtro per mostrare solo i file .ADF
    QString filter = "ADIF files (*.adf *.adi)";
    // Apre il dialogo di selezione file, impostando la directory iniziale su quella di default dell'utente
    QString filePath = QFileDialog::getOpenFileName(this, "Seleziona File ADIF", QDir::homePath(), filter);

    // Controlla se il percorso del file è vuoto (l'utente ha chiuso il dialogo senza scegliere un file)
    if (filePath.isEmpty()) {
        return; // Termina la funzione se nessun file è stato selezionato
    }

    Adif ad;
    Adif::parseAdif(filePath, ad);


    if(numeroLog > 0) {
        const auto& contatti = ad.getContatti();
        for(int i = 0; i < contatti.count(); i++) {
            Qso *qso = new Qso(RFLog, numeroLog);
            qso->insertDaAdif(contatti[i]);
            qsoList.push_back(qso);

            if(qso->locatoreRx.isEmpty()) {
                QSqlQuery *q = nDB->getQueryBind();
                q->prepare("select locatore from nominativi where nominativo = :nominativo");
                q->bindValue(":nominativo", qso->nominativoRx);
                DBResult *res = nDB->executeQuery(q);
                if(res->hasRows()) {
                    qso->locatoreRx = res->getCella(0).toUpper();
                    qso->insertAggiornaDB();
                }
                delete res;
                delete q;
            }
        }

        Qso::sort(qsoList);
        aggiornaTabella();
        updateMappaLocatori();
    }
}

void MainWindow::updateMappaLocatori() {
    mappa->delAllLinee(false);
    for(int i = 0; i < qsoList.count(); i++)
        mappa->addLinea(qsoList[i]->getLinea(), false);
    if(qsoList.count() > 0)
        mappa->adattaMappaLinee();
    else
        centraPredefinitoITA();
}

void MainWindow::menuMiaRadio() {
    MiaRadio mr(RFLog, this);
    mr.exec();
    caricaMieRadio();
}

void MainWindow::menuLocatoriPreferiti() {
    LocatoriPreferiti lp(RFLog, this);
    lp.exec();
    caricaLocatoriPreferiti();
}

void MainWindow::menuIniziaLog() {
    NuovoLog nl(RFLog, this);
    nl.exec();
    int n = nl.getLogSelezionato();
    if(n > 0) {
        numeroLog = n;
        for(int i = 0; i < qsoList.count(); i++) {
            delete qsoList[i];
        }
        qsoList.clear();

        auto listaId = Qso::getListaQso(RFLog, numeroLog);

        Qso *qsoAttingi = nullptr;

        for(int i = 0; i < listaId.count(); i++) {
            Qso *qso = new Qso(RFLog, numeroLog, listaId[i]);
            qsoList.push_back(qso);
            if(qsoAttingi == nullptr)
                qsoAttingi = qso;
        }

        if(qsoAttingi != nullptr) {
            tx->locatore->setText(qsoAttingi->locatoreTx);
            tx->radio->setCurrentText(qsoAttingi->radioTx);
            tx->potenza->setValue(qsoAttingi->potenzaTx);
            tx->trasmissione->setCurrentText(qsoAttingi->trasmissioneTx);
            tx->qsl->setCheckState(qsoAttingi->qsl ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
            tx->nominativiList->setRowCount(0);
            for(int i = 0; i < qsoAttingi->nominativoTx.count(); i++) {
                int rowCount = tx->nominativiList->rowCount();

                // Aggiungi una nuova riga
                tx->nominativiList->insertRow(rowCount);

                // Crea gli elementi per la nuova riga
                QTableWidgetItem *nomItem = new QTableWidgetItem(qsoAttingi->nominativoTx[i].nominativo);
                QTableWidgetItem *opItem = new QTableWidgetItem(qsoAttingi->nominativoTx[i].operatore);

                // Imposta gli elementi nella nuova riga
                tx->nominativiList->setItem(rowCount, 0, nomItem);
                tx->nominativiList->setItem(rowCount, 1, opItem);
            }
        }

        updateMappaLocatori();

        QSqlQuery *q = RFLog->getQueryBind();
        q->prepare("select nome from log where id = :id");
        q->bindValue(":id", numeroLog);
        DBResult *res = RFLog->executeQuery(q);

        QString css = R"(
font-weight: bold;
background-color: #cff;
color: darkblue;
)";
        ui->attualeLog->setStyleSheet(css);
        ui->attualeLog->setText(tr("Stai lavorando con il log:") + " " + res->getCella(0).toUpper());

        delete q;
        delete res;

        aggiornaTabella();
    }
}

void MainWindow::caricaLocatoriPreferiti() {
    DBResult *res = RFLog->executeQuery("select locatore, nome from locatoripreferiti order by nome");
    tx->preferiti->setRowCount(res->getRigheCount());
    for (int i = 0; i < res->getRigheCount(); i++) {
        QTableWidgetItem *locItem = new QTableWidgetItem(res->getCella(i, 0));
        QTableWidgetItem *nomeItem = new QTableWidgetItem(res->getCella(i, 1));
        tx->preferiti->setItem(i, 0, locItem);
        tx->preferiti->setItem(i, 1, nomeItem);
    }
    delete res;
}

void MainWindow::caricaMieRadio() {
    DBResult *res = RFLog->executeQuery("select nome from radio order by nome");
    tx->radio->clear();
    for(int i = 0; i < res->getRigheCount(); i++) {
        tx->radio->addItem(res->getCella(i, 0));
    }
    delete res;
}

void MainWindow::caricaNominativiDaDb() {
    DBResult *res = RFLog->executeQuery("select nominativo, count(nominativo) as conta from qsoNominativi nominativo group by nominativo order by conta desc, nominativo");
    tx->nominativo->clear();
    for(int i = 0; i < res->getRigheCount(); i++) {
        tx->nominativo->addItem(res->getCella(i, 0));
    }
    delete res;

    if (tx->nominativo->count() > 0)
        setSelectedNominativoDB(tx->nominativo->currentText());
}

void MainWindow::usaLocatorePreferito() {
    int selectedRow = tx->preferiti->currentRow();
    if (selectedRow != -1) {
        tx->locatore->setText(tx->preferiti->item(selectedRow, 0)->text());
    }
}

void MainWindow::menuInformazioniSu() {
    QDialog d;

    QGridLayout *griglia = new QGridLayout(&d);
    d.setLayout(griglia);
    d.setWindowTitle(tr("Informazioni su RFLog"));

    QLabel img;
    QPixmap pix(":/antenna_log_trasparente.png");
    img.setPixmap(pix);
    img.setScaledContents(true);
    img.resize(50, 50);

    griglia->addWidget(&img, 1, 0, 1, 2);

    d.resize(400, 300);

    d.exec();
}

void MainWindow::mappaGruppoSRPC() {
    MappaSRPC d(db, mappa->getStato(), this);

    d.exec();

    if(d.getApplica()) {
        mappa->setStato(d.getStato());
        mappa->setTipoMappa(d.getTipo(), false);
        centraGruppo();
    }
}

void MainWindow::aggiungiNominativo() {
    QString nominativo = tx->nominativo->currentText().trimmed().toUpper();
    QString operatore = tx->operatore->text().trimmed().toUpper();

    if (!nominativo.isEmpty()) {
        // Controlla se il nominativo è già presente nella prima colonna
        bool duplicato = false;
        for (int row = 0; row < tx->nominativiList->rowCount(); ++row) {
            QTableWidgetItem *item = tx->nominativiList->item(row, 0);
            if (item && item->text() == nominativo) {
                duplicato = true;
                break;
            }
        }

        if (!duplicato) {
            // Ottieni il numero di righe esistenti
            int rowCount = tx->nominativiList->rowCount();

            // Aggiungi una nuova riga
            tx->nominativiList->insertRow(rowCount);

            // Crea gli elementi per la nuova riga
            QTableWidgetItem *nomItem = new QTableWidgetItem(nominativo);
            QTableWidgetItem *opItem = new QTableWidgetItem(operatore);

            // Imposta gli elementi nella nuova riga
            tx->nominativiList->setItem(rowCount, 0, nomItem);
            tx->nominativiList->setItem(rowCount, 1, opItem);
        }
    }
}


void MainWindow::eliminaNominativo() {
    // Ottieni l'indice della riga selezionata
    int selectedRow = tx->nominativiList->currentRow();

    // Verifica se una riga è selezionata
    if (selectedRow != -1) {
        // Rimuovi la riga selezionata
        tx->nominativiList->removeRow(selectedRow);
    }
}

void MainWindow::setSelectedNominativoDB(const QString & txt) {
    QSqlQuery *q = RFLog->getQueryBind();
    q->prepare("select operatore from qsoNominativi where nominativo = :nominativo and operatore is not null and operatore <> '' order by operatore limit 1");
    q->bindValue(":nominativo", txt);
    DBResult *res = RFLog->executeQuery(q);
    tx->operatore->setText(res->getCella(0));
    delete q;
    delete res;
}

void MainWindow::aggiornaTabella()
{
    // Svuota la tabella (sia contenuto sia intestazioni).
    ui->Tabella->clearContents();

    // Imposta il numero di righe e di colonne (se serve).
    // Se la tabella è già configurata, puoi evitare queste righe.
    ui->Tabella->setRowCount(qsoList.count());

    for (int i = 0; i < qsoList.count(); i++) {
        aggiungiATabella(*qsoList[i], i);
    }
}

void MainWindow::aggiungiATabella(const Qso & qso, int row)
{
        QString txt;
        QTableWidgetItem* item;
        QColor rosso(0xff, 0xdf, 0xdf);
        QColor verde(0xdf, 0xff, 0xdf);
        QColor verdescuro(0x0, 0x40, 0x0);
        QString freccia(QString(' ') + QChar(0x279C) + QString(' '));
        QFont font;

        // TX LOCATORE
        item = new QTableWidgetItem(qso.locatoreTx);

        item->setForeground(QBrush(verdescuro));

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 0, item);


        int numNominativi = qso.nominativoTx.count();
        for (int j = 0; j < numNominativi; j++) {
            // Aggiungi sempre il nominativo
            txt += qso.nominativoTx[j].nominativo;

            // Se c'è l'operatore, appendi " -> operatore"
            if (!qso.nominativoTx[j].operatore.isEmpty()) {
                txt += freccia + qso.nominativoTx[j].operatore;
            }

            // Aggiungi il newline solo se NON siamo sull'ultimo elemento
            if (j < numNominativi - 1) {
                txt += "\n";
            }
        }

        // Crea l'item per la cella (riga = i, colonna = 1)
        item = new QTableWidgetItem(txt);

        item->setForeground(QBrush(verdescuro));

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Font in grassetto
        font = item->font();
        font.setBold(true);
        item->setFont(font);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 1, item);


        // RX LOCATORE
        item = new QTableWidgetItem(qso.locatoreRx);

        // Se duplicato == false, colore blu; altrimenti rosso
        if (!qso.duplicato) {
            item->setForeground(QBrush(Qt::blue));
        } else {
            item->setForeground(QBrush(Qt::red));
        }

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 2, item);


        // RX NOMINATIVO
        txt = qso.nominativoRx;
        if(!qso.operatoreRx.isEmpty())
            txt += freccia + qso.operatoreRx;

        item = new QTableWidgetItem(txt);

        // Se duplicato == false, colore blu; altrimenti rosso
        if (!qso.duplicato)
            item->setForeground(QBrush(Qt::blue));
        else
            item->setForeground(QBrush(Qt::red));

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Font in grassetto
        font = item->font();
        font.setBold(true);
        item->setFont(font);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 3, item);


        // TX DETTAGLI
        txt = QString::number(qso.frequenzaRx) + "MHz";
        txt += freccia;
        txt += QString::number(qso.getBandaMt()) + "mt";
        txt += " | ";
        txt += QString::number(qso.segnaleRx);
        txt += " | ";
        txt += qso.trasmissioneTx;

        item = new QTableWidgetItem(txt);

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 4, item);


        // ORA
        QLocale locale = QLocale::system();
        txt = locale.toString(qso.orarioRx, QLocale::ShortFormat);
        item = new QTableWidgetItem(txt);

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 5, item);


        // QSL
        item = new QTableWidgetItem(qso.qsl ? tr("Si") : tr("No"));

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 6, item);


        // INFO
        item = new QTableWidgetItem(QString::number(Coordinate::distanzaKm(qso.locatoreTx, qso.locatoreRx), 'f', 2) + " Km");

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 7, item);


        // Facoltativo: adatta l'altezza della riga al contenuto
        ui->Tabella->resizeRowToContents(row);
}
