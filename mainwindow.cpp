#include <QDoubleValidator>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTabWidget>
#include <QModelIndex>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QTimer>
#include <QTimeZone>

#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "adif.h"
#include "edi.h"
#include "locatoripreferiti.h"
#include "nuovolog.h"
#include "miaradio.h"
#include "finestraqsl.h"
#include "mappasrpc.h"
#include "traduttore.h"
#include "informazionisu.h"

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
    nominativoPrefixSet = false;
    nominativoPaletteSet = false;

    ui->setupUi(this);
    tx->setupUi(ui->tx);

    resizeTimer = new QTimer(this);
    resizeTimer->setSingleShot(true);
    connect(resizeTimer, &QTimer::timeout, this, &MainWindow::updateMappaLocatori);

    DBResult *res;


    QString formato =
                    "font-weight: bold;\n"
                    "background-color: aliceblue;\n"
        ;


    Nominativo = new SuggestiveLineEdit(this);
    Progressivo = new SuggestiveLineEdit(this);
    Locatore = new SuggestiveLineEdit(this);
    Segnale = new SuggestiveLineEdit(this);
    Frequenza = new SuggestiveLineEdit(this);
    Orario = new SuggestiveLineEdit(this);
    nominativoDefaultPalette = Nominativo->palette();
    nominativoPaletteSet = true;

    Nominativo->setSuggestions(QStringList());
    Segnale->setSuggestions(segnaleSuggerimentiPrincipali());
    Segnale->setPlaceholderText(QStringLiteral("59"));
    Frequenza->setSuggestions(frequenzaSuggerimentiPrincipali());
    Frequenza->setPlaceholderText(QStringLiteral("14.250 (20m)"));

    Nominativo->setStyleSheet(formato);
    Progressivo->setStyleSheet(formato);
    Locatore->setStyleSheet(formato);
    Segnale->setStyleSheet(formato);
    Frequenza->setStyleSheet(formato);
    Orario->setStyleSheet(formato);

    ui->grigliaInput->addWidget(Nominativo, 2, 2);
    ui->grigliaInput->addWidget(Locatore, 2, 3);
    ui->grigliaInput->addWidget(Progressivo, 2, 4);
    ui->grigliaInput->addWidget(Segnale, 2, 5);
    ui->grigliaInput->addWidget(Frequenza, 2, 6);
    ui->grigliaInput->addWidget(Orario, 2, 7);

    aggiornaStatoCampiInput();

    QFont f = ui->successivo->font();
    f.setPointSizeF(f.pointSizeF() * 1.4);
    ui->successivo->setFont(f);

    QWidget::setTabOrder(Nominativo, Progressivo);
    QWidget::setTabOrder(Progressivo, Locatore);
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

    connect(Nominativo, &QLineEdit::textEdited, this, &MainWindow::compilaNominativo);
    connect(Nominativo, &QLineEdit::textEdited, this, &MainWindow::aggiornaColoreNominativoDuplicato);
    connect(Nominativo, &QLineEdit::textEdited, this, &MainWindow::aggiornaSuggerimentiNominativo);
    connect(Nominativo, &SuggestiveLineEdit::suggestionsRequested, this, &MainWindow::aggiornaSuggerimentiNominativo);
    connect(Nominativo, &SuggestiveLineEdit::completionAccepted, this, &MainWindow::caricaLocatoriDaNominativo);
    //connect(Nominativo, &SuggestiveLineEdit::pressTab, this, &MainWindow::catturaTab);
    connect(Nominativo, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Nominativo, &QLineEdit::returnPressed, this, [this]() {
        caricaLocatoriDaNominativo(Nominativo->text());
    });

    connect(Locatore, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Segnale, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Frequenza, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);
    connect(Frequenza, &SuggestiveLineEdit::completionAccepted, this, [this](const QString &value) {
        const double frequenza = parseFrequencyValue(value);
        if (frequenza > 0) {
            Frequenza->setText(QString::number(frequenza, 'f', 3));
        }
    });
    connect(Orario, &QLineEdit::returnPressed, this, &MainWindow::confermaLinea);


    ui->Tabella->setColumnCount(7);
    updateTableHeaders();

    ui->Tabella->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    ui->Tabella->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    ui->Tabella->setAlternatingRowColors(true);
    ui->Tabella->setShowGrid(true);
    ui->Tabella->setGridStyle(Qt::SolidLine);
    ui->Tabella->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->Tabella->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->Tabella->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->Tabella->verticalHeader()->setVisible(true);
    ui->Tabella->verticalHeader()->setDefaultSectionSize(24);
    ui->Tabella->verticalHeader()->setMinimumSectionSize(20);
    ui->Tabella->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->Tabella->setStyleSheet(
        "QTableWidget {"
        "  alternate-background-color: #f7f9fc;"
        "  gridline-color: #d7dee7;"
        "  selection-background-color: #dceeff;"
        "  selection-color: #20262d;"
        "}"
        "QHeaderView::section {"
        "  background-color: #e9eef5;"
        "  border: 1px solid #d7dee7;"
        "  font-weight: 600;"
        "  padding: 4px;"
        "}"
    );
    ui->Tabella->installEventFilter(this);


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

    connect(tx->altri, &QPushButton::clicked, this, [this]() {
        modificaAltri(altriTx, this);
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

    updateColoreStatoItems();

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
    connect(ui->menuApriEdi, &QAction::triggered, this, &MainWindow::menuApriEdi);
    connect(ui->menuEsportaAdifTx, &QAction::triggered, this, &MainWindow::menuEsportaAdifTx);
    connect(ui->menuLocatoriPreferiti, &QAction::triggered, this, &MainWindow::menuLocatoriPreferiti);
    connect(ui->menuIniziaLog, &QAction::triggered, this, &MainWindow::menuIniziaLog);
    connect(ui->menuMieRadio, &QAction::triggered, this, &MainWindow::menuMiaRadio);
    connect(ui->menuQsl, &QAction::triggered, this, &MainWindow::menuQsl);
    connect(ui->menuInformazioniSu, &QAction::triggered, this, &MainWindow::menuInformazioniSu);

    //top widget
    connect(tx->preferitiOK, &QPushButton::clicked, this, &MainWindow::usaLocatorePreferito);
    connect(tx->preferiti, &QTableWidget::doubleClicked, this, &MainWindow::usaLocatorePreferito);
    connect(tx->aggiungi, &QPushButton::clicked, this, &MainWindow::aggiungiNominativo);
    connect(tx->togli, &QPushButton::clicked, this, &MainWindow::eliminaNominativo);
    connect(tx->nominativo, &QComboBox::textActivated, this, &MainWindow::setSelectedNominativoDB);
    connect(ui->Tabella, &QTableWidget::doubleClicked, this, &MainWindow::modificaTxDaTabella);


    QTimer *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &MainWindow::aggiornaOrario);
    t->start(1000);

    configuraPreferitiTable(tx->preferiti);

    tx->nominativiList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    caricaLocatoriPreferiti();
    caricaMieRadio();
    caricaNominativiDaDb();
    setupLanguageMenu();
    applySystemLanguage();
    centraPredefinitoITA();
}

MainWindow::~MainWindow() {
    delete ui;
    delete mappaConfig;
}

void MainWindow::aggiornaStatoCampiInput() {
    const bool logAttivo = (numeroLog > 0);
    Nominativo->setEnabled(logAttivo);
    Progressivo->setEnabled(logAttivo);
    Locatore->setEnabled(logAttivo);
    Segnale->setEnabled(logAttivo);
    Frequenza->setEnabled(logAttivo);
    Orario->setEnabled(logAttivo);
}

double MainWindow::parseFrequencyValue(const QString &text) {
    const QString normalized = text.trimmed().replace(',', '.');
    bool ok = false;
    const double direct = normalized.toDouble(&ok);
    if (ok) {
        return direct;
    }

    static const QRegularExpression freqRegex(R"((\d+(?:\.\d+)?))");
    const QRegularExpressionMatch match = freqRegex.match(normalized);
    if (!match.hasMatch()) {
        return 0.0;
    }

    return match.captured(1).toDouble();
}

QStringList MainWindow::frequenzaSuggerimentiPrincipali() {
    return {
        QStringLiteral("14.250 (20m SSB)") ,
        QStringLiteral("1.840 (160m)") ,
        QStringLiteral("3.760 (80m)") ,
        QStringLiteral("7.074 (40m FT8)") ,
        QStringLiteral("7.120 (40m SSB)") ,
        QStringLiteral("10.136 (30m FT8)") ,
        QStringLiteral("14.074 (20m FT8)") ,
        QStringLiteral("18.100 (17m FT8)") ,
        QStringLiteral("21.074 (15m FT8)") ,
        QStringLiteral("21.300 (15m SSB)") ,
        QStringLiteral("24.915 (12m FT8)") ,
        QStringLiteral("28.074 (10m FT8)") ,
        QStringLiteral("28.500 (10m SSB)") ,
        QStringLiteral("50.313 (6m FT8)") ,
        QStringLiteral("145.500 (2m FM)") ,
        QStringLiteral("433.500 (70cm FM)")
    };
}


double MainWindow::frequenzaDefault20mMHz() {
    return 14.250;
}


QStringList MainWindow::segnaleSuggerimentiPrincipali() {
    return {
        QStringLiteral("59"),
        QStringLiteral("599")
    };
}

QString MainWindow::rstDefaultFromMode(const QString &mode) {
    const QString normalized = mode.trimmed().toUpper();
    if (normalized.contains(QStringLiteral("CW"))) {
        return QStringLiteral("599");
    }

    static const QStringList foniaModes = {
        QStringLiteral("SSB"),
        QStringLiteral("USB"),
        QStringLiteral("LSB"),
        QStringLiteral("FM"),
        QStringLiteral("AM"),
        QStringLiteral("PHONE")
    };
    for (const QString &phoneMode : foniaModes) {
        if (normalized == phoneMode || normalized.contains(phoneMode)) {
            return QStringLiteral("59");
        }
    }

    return QString();
}



void MainWindow::setupLanguageMenu() {
    if (!ui->menuLingua) {
        return;
    }

    languageActionGroup = new QActionGroup(this);
    languageActionGroup->setExclusive(true);

    auto bindAction = [this](QAction *action, const QString &locale) {
        if (!action) {
            return;
        }
        action->setCheckable(true);
        action->setData(locale);
        languageActionGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, locale]() {
            applyLanguage(locale);
        });
    };

    bindAction(ui->actionLinguaItaliano, QStringLiteral("it_IT"));
    bindAction(ui->actionLinguaInglese, QStringLiteral("en_US"));
    bindAction(ui->actionLinguaSpagnolo, QStringLiteral("es_ES"));
    bindAction(ui->actionLinguaFrancese, QStringLiteral("fr_FR"));
    bindAction(ui->actionLinguaTedesco, QStringLiteral("de_DE"));
}

std::unique_ptr<QTranslator> MainWindow::createAppTranslator(const QString &localeName) {
    const QString baseName = QStringLiteral("RFLog_") + localeName;
    auto translator = std::make_unique<QTranslator>();
    if (translator->load(QStringLiteral(":/i18n/") + baseName)) {
        return translator;
    }

    const QString translationsDir = QCoreApplication::applicationDirPath()
        + QDir::separator()
        + QStringLiteral("i18n");
    if (translator->load(baseName, translationsDir)) {
        return translator;
    }

    auto tsTranslator = std::make_unique<Traduttore>();
    const QString resourceTs = QStringLiteral(":/i18n/") + baseName + QStringLiteral(".ts");
    if (tsTranslator->loadFromTs(resourceTs)) {
        return tsTranslator;
    }

    const QString fileTs = translationsDir + QDir::separator() + baseName + QStringLiteral(".ts");
    if (tsTranslator->loadFromTs(fileTs)) {
        return tsTranslator;
    }

    return nullptr;
}

void MainWindow::applyLanguage(const QString &localeName) {
    QString targetLocale = localeName;
    if (appTranslator) {
        qApp->removeTranslator(appTranslator.get());
        appTranslator.reset();
    }

    if (targetLocale == QLatin1String("it_IT")) {
        currentLocale = targetLocale;
        if (languageActionGroup) {
            for (QAction *action : languageActionGroup->actions()) {
                action->setChecked(action->data().toString() == currentLocale);
            }
        }
        retranslateUi();
        return;
    }

    appTranslator = createAppTranslator(targetLocale);
    if (!appTranslator && targetLocale != QLatin1String("en_US")) {
        targetLocale = QStringLiteral("en_US");
        appTranslator = createAppTranslator(targetLocale);
    }

    if (appTranslator) {
        qApp->installTranslator(appTranslator.get());
    }

    currentLocale = appTranslator ? targetLocale : QStringLiteral("it_IT");
    if (languageActionGroup) {
        for (QAction *action : languageActionGroup->actions()) {
            action->setChecked(action->data().toString() == currentLocale);
        }
    }

    retranslateUi();
}

void MainWindow::applySystemLanguage() {
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString localeName = QLocale(locale).name();
        if (QLocale(locale).language() == QLocale::Italian) {
            applyLanguage(QStringLiteral("it_IT"));
            return;
        }
        auto translator = createAppTranslator(localeName);
        if (!translator) {
            continue;
        }
        if (appTranslator) {
            qApp->removeTranslator(appTranslator.get());
        }
        appTranslator = std::move(translator);
        qApp->installTranslator(appTranslator.get());
        currentLocale = localeName;
        if (languageActionGroup) {
            for (QAction *action : languageActionGroup->actions()) {
                action->setChecked(action->data().toString() == currentLocale);
            }
        }
        retranslateUi();
        return;
    }

    applyLanguage(QStringLiteral("en_US"));
}

void MainWindow::retranslateUi() {
    ui->retranslateUi(this);
    tx->retranslateUi(ui->tx);
    updateTableHeaders();
    updateColoreStatoItems();
    aggiornaEtichettaSuccessivo();
}

void MainWindow::updateTableHeaders() {
    ui->Tabella->setHorizontalHeaderLabels(
        QStringList() << tr("TX Locatore")
                      << tr("TX")
                      << tr("RX Locatore")
                      << tr("RX")
                      << tr("TX dettagli")
                      << tr("Data/Ora UTC")
                      << tr("INFO")
        );
}

void MainWindow::updateColoreStatoItems() {
    const int currentIndex = mappaConfig->coloreStato->currentIndex();
    QSignalBlocker blocker(mappaConfig->coloreStato);
    mappaConfig->coloreStato->clear();
    mappaConfig->coloreStato->addItem(tr("Grigio"));
    mappaConfig->coloreStato->addItem(tr("Viola"));
    mappaConfig->coloreStato->addItem(tr("Blu"));
    mappaConfig->coloreStato->addItem(tr("Verde"));
    mappaConfig->coloreStato->addItem(tr("Giallo"));
    mappaConfig->coloreStato->addItem(tr("Marrone"));
    mappaConfig->coloreStato->addItem(tr("Arancione"));
    if (mappaConfig->coloreStato->count() > 0) {
        const int boundedIndex = qMin(currentIndex, mappaConfig->coloreStato->count() - 1);
        mappaConfig->coloreStato->setCurrentIndex(qMax(0, boundedIndex));
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    resizeTimer->start(200);
}

void MainWindow::compilaNominativo(const QString &txt) {
    const QString upper = txt.toUpper();
    if (upper != txt) {
        QSignalBlocker blocker(Nominativo);
        Nominativo->setText(upper);
        Nominativo->setCursorPosition(upper.length());
    }
    aggiornaColoreNominativoDuplicato(upper);
}

bool MainWindow::nominativoPresenteInLista(const QString &txt) const
{
    const QString target = txt.trimmed().toUpper();
    if (target.isEmpty())
        return false;

    for (Qso *qso : qsoList) {
        //for (const Qso::NominativoNome &entry : qso->nominativoTx) {
            if (qso->nominativoRx.trimmed().toUpper() == target)
                return true;
        //}
    }

    return false;
}

void MainWindow::aggiornaEtichettaSuccessivo() {
    ui->successivo->setText(QString::number(qsoList.count() + 1));
}

void MainWindow::ricalcolaDuplicatiQso() {
    QSet<QString> nominativiVisti;
    for (int i = qsoList.count() - 1; i >= 0; --i) {
        Qso *qso = qsoList[i];
        if (!qso) {
            continue;
        }

        const QString nominativo = qso->nominativoRx.trimmed().toUpper();
        if (nominativo.isEmpty()) {
            qso->duplicato = false;
            continue;
        }

        if (nominativiVisti.contains(nominativo)) {
            qso->duplicato = true;
        } else {
            qso->duplicato = false;
            nominativiVisti.insert(nominativo);
        }
    }
}

void MainWindow::aggiornaColoreNominativoDuplicato(const QString &txt)
{
    if (!nominativoPaletteSet)
        return;

    if (nominativoPresenteInLista(txt)) {
        QPalette palette = Nominativo->palette();
        palette.setColor(QPalette::Text, Qt::red);
        Nominativo->setPalette(palette);
    } else {
        Nominativo->setPalette(nominativoDefaultPalette);
    }
}

void MainWindow::usaLocatorePreferito() {
    usaLocatorePreferitoTx(tx);
}

void MainWindow::aggiornaSuggerimentiNominativo(const QString &txt) {
    const QString prefix = txt.trimmed().toUpper();
    if (nominativoPrefixSet && prefix == lastNominativoPrefix)
        return;

    lastNominativoPrefix = prefix;
    nominativoPrefixSet = true;

    QSqlQuery *q = nDB->getQueryBind();
    if (prefix.isEmpty()) {
        q->prepare("select nominativo from nominativi order by nominativo limit 50");
    } else {
        q->prepare("select nominativo from nominativi where nominativo like :prefix order by nominativo limit 50");
        q->bindValue(":prefix", prefix + "%");
    }

    DBResult *res = nDB->executeQuery(q);
    QStringList suggestions;
    if (res->hasRows()) {
        suggestions.reserve(res->tabella.size());
        for (int i = 0; i < res->tabella.size(); i++) {
            suggestions << res->tabella[i][0];
        }
    }
    delete res;
    delete q;

    Nominativo->setSuggestions(suggestions);
}

void MainWindow::caricaLocatoriDaNominativo(const QString &nominativo) {
    const QString nome = nominativo.trimmed().toUpper();
    if (nome.isEmpty()) {
        Locatore->setSuggestions(QStringList());
        return;
    }

    QSqlQuery *q = nDB->getQueryBind();
    q->prepare("select locatore from nominativi where nominativo = :nominativo order by locatore");
    q->bindValue(":nominativo", nome);
    DBResult *res = nDB->executeQuery(q);
    QStringList locatori;
    if (res->hasRows()) {
        locatori.reserve(res->tabella.size());
        for (int i = 0; i < res->tabella.size(); i++) {
            locatori << res->tabella[i][0];
        }
    }
    delete res;
    delete q;

    Locatore->setSuggestions(locatori);
    if (!locatori.isEmpty()) {
        Locatore->setText(locatori.first());
        Locatore->setCursorPosition(Locatore->text().length());
    }
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

/*
void MainWindow::catturaTab() {
    qDebug() << Nominativo->text();
}
*/

void MainWindow::svuotaLineEdit() {
    QString q;
    Nominativo->setText(q);
    aggiornaColoreNominativoDuplicato(q);
    Progressivo->setText(q);
    Locatore->setText(q);
    Segnale->setText(q);
    Frequenza->setText(q);
    Orario->setText(q);
}

void MainWindow::confermaLinea() {
    const QString nominativoText = Nominativo->text().trimmed().toUpper();
    if (nominativoText.isEmpty()) {
        return;
    }

    Qso *qso = new Qso(RFLog, numeroLog);
    qso->nominativoRx = nominativoText;
    qso->operatoreRx = QString();
    qso->progressivoRx = Progressivo->text().trimmed().toUpper();
    qso->locatoreRx = Locatore->text().trimmed().toUpper();
    qso->segnaleRx = Segnale->text().trimmed().toDouble();
    qso->frequenzaRx = parseFrequencyValue(Frequenza->text());
    if (qso->frequenzaRx <= 0.0) {
        qso->frequenzaRx = frequenzaDefault20mMHz();
        Frequenza->setText(QString::number(qso->frequenzaRx, 'f', 3));
    }
    qso->duplicato = nominativoPresenteInLista(nominativoText);

    aggiornaQsoDaTxDialog(*qso, tx);
    if (qso->segnaleRx <= 0.0) {
        const QString rstDefault = rstDefaultFromMode(qso->trasmissioneTx);
        if (!rstDefault.isEmpty()) {
            qso->segnaleRx = rstDefault.toDouble();
            Segnale->setText(rstDefault);
        }
    }
    qso->altro = altriTx;

    if (qso->nominativoTx.isEmpty()) {
        const QString nominativoTx = tx->nominativo->currentText().trimmed().toUpper();
        const QString operatoreTx = tx->operatore->text().trimmed().toUpper();
        if (!nominativoTx.isEmpty()) {
            Qso::NominativoNome nominativo;
            nominativo.nominativo = nominativoTx;
            nominativo.operatore = operatoreTx;
            qso->nominativoTx.append(nominativo);
        }
    }

    const QString orarioText = Orario->text().trimmed();
    if (orarioText.isEmpty()) {
        const QDateTime currentUtc = QDateTime::currentDateTimeUtc();
        qso->orarioRx = currentUtc;
        Orario->setPlaceholderText(currentUtc.toString("yyyy-MM-dd HH:mm:ss"));
    } else {
        QDateTime parsed = QDateTime::fromString(orarioText, "yyyy-MM-dd HH:mm:ss");
        if (!parsed.isValid()) {
            parsed = QDateTime::fromString(orarioText, Qt::ISODate);
        }
        if (parsed.isValid()) {
            parsed.setTimeZone(QTimeZone::utc());
            qso->orarioRx = parsed;
        } else {
            qso->orarioRx = QDateTime::currentDateTimeUtc();
        }
    }

    qso->insertAggiornaDB();
    qsoList.prepend(qso);
    aggiornaEtichettaSuccessivo();
    aggiornaTabella();
    updateMappaLocatori();
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
        setMatriceConAspect(lato1, lato2);
    }
}

void MainWindow::centraPredefinitoITA() {
    setMatriceConAspect("JM35AA", "JN97MT");
}
void MainWindow::centraPredefinitoMondo() {
    setMatriceConAspect("AC00AA", "RR89PX");
}
void MainWindow::centraPredefinitoEU() {
    setMatriceConAspect("IM32GM", "LQ22IV");
}
void MainWindow::centraPredefinitoAsia() {
    setMatriceConAspect("KJ55JB", "RQ89PX");
}
void MainWindow::centraPredefinitoAfrica() {
    setMatriceConAspect("HM69IF", "LF92KI");
}
void MainWindow::centraPredefinitoNordAmerica() {
    setMatriceConAspect("AQ51EK", "GJ84PJ");
}
void MainWindow::centraPredefinitoSudAmerica() {
    setMatriceConAspect("DK86GR", "HD51UL");
};
void MainWindow::centraPredefinitoOceania() {
    setMatriceConAspect("NK34HS", "RD95AX");
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
    if (numeroLog <= 0) {
        QMessageBox::warning(this, tr("Log non attivo"), tr("Prima di importare un ADIF devi iniziare o aprire un log."));
        return;
    }
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
        ricalcolaDuplicatiQso();
        aggiornaEtichettaSuccessivo();
        aggiornaTabella();
        updateMappaLocatori();
    }
}

void MainWindow::menuApriEdi() {
    if (numeroLog <= 0) {
        QMessageBox::warning(this, tr("Log non attivo"), tr("Prima di importare un EDI devi iniziare o aprire un log."));
        return;
    }

    QString filter = "EDI files (*.edi)";
    QString filePath = QFileDialog::getOpenFileName(this, "Seleziona File EDI", QDir::homePath(), filter);
    if (filePath.isEmpty()) {
        return;
    }

    Edi edi;
    if (!Edi::parseEdi(filePath, edi)) {
        QMessageBox::warning(this, tr("File non valido"), tr("Impossibile leggere il file EDI selezionato."));
        return;
    }

    const auto& contatti = edi.getContatti();
    const auto& header = edi.getIntestazione();
    for (int i = 0; i < contatti.count(); i++) {
        Qso *qso = new Qso(RFLog, numeroLog);
        qso->insertDaEdi(header, contatti[i]);
        qsoList.push_back(qso);

        if (qso->locatoreRx.isEmpty()) {
            QSqlQuery *q = nDB->getQueryBind();
            q->prepare("select locatore from nominativi where nominativo = :nominativo");
            q->bindValue(":nominativo", qso->nominativoRx);
            DBResult *res = nDB->executeQuery(q);
            if (res->hasRows()) {
                qso->locatoreRx = res->getCella(0).toUpper();
                qso->insertAggiornaDB();
            }
            delete res;
            delete q;
        }
    }

    Qso::sort(qsoList);
    ricalcolaDuplicatiQso();
    aggiornaEtichettaSuccessivo();
    aggiornaTabella();
    updateMappaLocatori();
}

void MainWindow::menuEsportaAdifTx() {
    if (numeroLog <= 0 || qsoList.isEmpty()) {
        if (numeroLog <= 0) {
            QMessageBox::warning(this, tr("Log non attivo"), tr("Prima di esportare devi iniziare o aprire un log."));
        }
        return;
    }

    QString nomeLog;
    QSqlQuery *q = RFLog->getQueryBind();
    q->prepare("select nome from log where id = :id");
    q->bindValue(":id", numeroLog);
    DBResult *res = RFLog->executeQuery(q);
    if (res->hasRows()) {
        nomeLog = res->getCella(0).trimmed();
    }
    delete res;
    delete q;

    const QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        tr("Seleziona cartella per export ADIF"),
        QDir::homePath()
        );
    if (directoryPath.isEmpty()) {
        return;
    }

    Adif::exportTxAdif(directoryPath, nomeLog, qsoList);
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
        QApplication::setOverrideCursor(Qt::WaitCursor);
        numeroLog = n;
        aggiornaStatoCampiInput();
        for(int i = 0; i < qsoList.count(); i++) {
            delete qsoList[i];
        }
        qsoList.clear();
        aggiornaEtichettaSuccessivo();

        auto listaId = Qso::getListaQso(RFLog, numeroLog);

        Qso *qsoAttingi = nullptr;

        for(int i = 0; i < listaId.count(); i++) {
            Qso *qso = new Qso(RFLog, numeroLog, listaId[i]);
            qsoList.push_back(qso);
            if(qsoAttingi == nullptr)
                qsoAttingi = qso;
        }

        ricalcolaDuplicatiQso();
        aggiornaEtichettaSuccessivo();

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
            altriTx = qsoAttingi->altro;
        } else {
            altriTx.clear();
        }

        updateMappaLocatori();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

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
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::caricaLocatoriPreferiti() {
    caricaLocatoriPreferitiTx(tx);
}

void MainWindow::caricaLocatoriPreferitiTx(Ui::Tx *txUi) {
    if (!txUi) {
        return;
    }

    configuraPreferitiTable(txUi->preferiti);

    DBResult *res = RFLog->executeQuery("select locatore, nome from locatoripreferiti order by nome");
    txUi->preferiti->setRowCount(res->getRigheCount());
    for (int i = 0; i < res->getRigheCount(); i++) {
        QTableWidgetItem *locItem = new QTableWidgetItem(res->getCella(i, 0));
        QTableWidgetItem *nomeItem = new QTableWidgetItem(res->getCella(i, 1));
        txUi->preferiti->setItem(i, 0, locItem);
        txUi->preferiti->setItem(i, 1, nomeItem);
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

void MainWindow::usaLocatorePreferitoTx(Ui::Tx *txUi) {
    if (!txUi) {
        return;
    }

    int selectedRow = txUi->preferiti->currentRow();
    if (selectedRow != -1) {
        txUi->locatore->setText(txUi->preferiti->item(selectedRow, 0)->text());
    }
}


void MainWindow::menuQsl() {
    if (numeroLog <= 0) {
        QMessageBox::warning(this, tr("Log non attivo"), tr("Prima di consultare i QSL devi iniziare o aprire un log."));
        return;
    }

    FinestraQSL finestra(RFLog, numeroLog, this);
    finestra.exec();
}

void MainWindow::menuInformazioniSu() {
    InformazioniSu dialog(this);
    dialog.exec();
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

            aggiungiNominativoCombo(tx->nominativo, nominativo);
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

void MainWindow::modificaTxDaTabella(const QModelIndex &index) {
    if (!index.isValid() /*|| (index.column() < 2 || index.column() > 7)*/) {
        return;
    }

    int row = index.row();
    if (row < 0 || row >= qsoList.count()) {
        return;
    }

    Qso *qso = qsoList[row];
    if (!qso) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Modifica TX dettagli"));
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QWidget *txWidget = new QWidget(&dialog);
    Ui::Tx *txUi = new Ui::Tx;
    txUi->setupUi(txWidget);
    layout->addWidget(txWidget);

    QGroupBox *rxGroup = new QGroupBox(tr("RX"), &dialog);
    QFormLayout *rxLayout = new QFormLayout(rxGroup);
    QLineEdit *locatoreEdit = new QLineEdit(rxGroup);
    QLineEdit *progressivoEdit = new QLineEdit(rxGroup);
    QLineEdit *nominativoEdit = new QLineEdit(rxGroup);
    QLineEdit *segnaleEdit = new QLineEdit(rxGroup);
    QLineEdit *frequenzaEdit = new QLineEdit(rxGroup);
    QLineEdit *orarioEdit = new QLineEdit(rxGroup);
    segnaleEdit->setValidator(new QDoubleValidator(0, 9999, 2, segnaleEdit));
    frequenzaEdit->setValidator(new QDoubleValidator(0, 1000000, 6, frequenzaEdit));
    orarioEdit->setPlaceholderText(tr("yyyy-MM-dd HH:mm:ss"));
    rxLayout->addRow(tr("Nominativo RX"), nominativoEdit);
    rxLayout->addRow(tr("Locatore RX"), locatoreEdit);
    rxLayout->addRow(tr("Progressivo"), progressivoEdit);
    rxLayout->addRow(tr("Segnale RX"), segnaleEdit);
    rxLayout->addRow(tr("Frequenza RX"), frequenzaEdit);
    rxLayout->addRow(tr("Orario RX (UTC)"), orarioEdit);
    layout->addWidget(rxGroup);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Salva"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Annulla"));
    layout->addWidget(buttons);

    for (int i = 0; i < tx->radio->count(); ++i) {
        txUi->radio->addItem(tx->radio->itemText(i));
    }
    for (int i = 0; i < tx->nominativo->count(); ++i) {
        txUi->nominativo->addItem(tx->nominativo->itemText(i));
    }

    configuraPreferitiTable(txUi->preferiti);
    caricaLocatoriPreferitiTx(txUi);

    popolaTxDialog(txUi, *qso);
    locatoreEdit->setText(qso->locatoreRx);
    progressivoEdit->setText(qso->progressivoRx);
    nominativoEdit->setText(qso->nominativoRx);
    segnaleEdit->setText(QString::number(qso->segnaleRx));
    frequenzaEdit->setText(QString::number(qso->frequenzaRx));
    orarioEdit->setText(qso->orarioRx.toUTC().toString("yyyy-MM-dd HH:mm:ss"));

    QVector<Qso::AltriParametri> altroTemp = qso->altro;

    connect(txUi->preferitiOK, &QPushButton::clicked, this, [this, txUi]() {
        usaLocatorePreferitoTx(txUi);
    });
    connect(txUi->preferiti, &QTableWidget::doubleClicked, this, [this, txUi]() {
        usaLocatorePreferitoTx(txUi);
    });
    connect(txUi->aggiungi, &QPushButton::clicked, this, [this, txUi]() {
        aggiungiNominativoTx(txUi);
    });
    connect(txUi->togli, &QPushButton::clicked, this, [this, txUi]() {
        eliminaNominativoTx(txUi);
    });
    connect(txUi->nominativo, &QComboBox::textActivated, this, [this, txUi](const QString &txt) {
        setSelectedNominativoDBTx(txt, txUi);
    });
    connect(txUi->altri, &QPushButton::clicked, this, [this, &altroTemp, &dialog]() {
        modificaAltri(altroTemp, &dialog);
    });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        aggiornaQsoDaTxDialog(*qso, txUi);
        qso->altro = altroTemp;
        qso->locatoreRx = locatoreEdit->text().trimmed().toUpper();
        qso->progressivoRx = progressivoEdit->text().trimmed().toUpper();
        qso->nominativoRx = nominativoEdit->text().trimmed().toUpper();
        qso->segnaleRx = segnaleEdit->text().trimmed().toDouble();
        if (qso->segnaleRx <= 0.0) {
            const QString rstDefault = rstDefaultFromMode(qso->trasmissioneTx);
            if (!rstDefault.isEmpty()) {
                qso->segnaleRx = rstDefault.toDouble();
                segnaleEdit->setText(rstDefault);
            }
        }
        qso->frequenzaRx = parseFrequencyValue(frequenzaEdit->text());
        if (qso->frequenzaRx <= 0.0) {
            qso->frequenzaRx = frequenzaDefault20mMHz();
            frequenzaEdit->setText(QString::number(qso->frequenzaRx, 'f', 3));
        }

        const QString orarioText = orarioEdit->text().trimmed();
        if (!orarioText.isEmpty()) {
            QDateTime parsed = QDateTime::fromString(orarioText, "yyyy-MM-dd HH:mm:ss");
            if (!parsed.isValid()) {
                parsed = QDateTime::fromString(orarioText, Qt::ISODate);
            }
            if (parsed.isValid()) {
                parsed.setTimeZone(QTimeZone::utc());
                qso->orarioRx = parsed;
            }
        }

        qso->insertAggiornaDB();
        ricalcolaDuplicatiQso();
        aggiornaEtichettaSuccessivo();
        aggiornaTabella();
        updateMappaLocatori();
    }

    delete txUi;
}

void MainWindow::popolaTxDialog(Ui::Tx *txUi, const Qso &qso) {
    if (!txUi) {
        return;
    }

    txUi->locatore->setText(qso.locatoreTx);
    txUi->radio->setCurrentText(qso.radioTx);
    txUi->potenza->setValue(qso.potenzaTx);
    txUi->trasmissione->setCurrentText(qso.trasmissioneTx);
    txUi->qsl->setCheckState(qso.qsl ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    txUi->nominativiList->setRowCount(0);
    for (int i = 0; i < qso.nominativoTx.count(); i++) {
        int rowCount = txUi->nominativiList->rowCount();
        txUi->nominativiList->insertRow(rowCount);
        QTableWidgetItem *nomItem = new QTableWidgetItem(qso.nominativoTx[i].nominativo);
        QTableWidgetItem *opItem = new QTableWidgetItem(qso.nominativoTx[i].operatore);
        txUi->nominativiList->setItem(rowCount, 0, nomItem);
        txUi->nominativiList->setItem(rowCount, 1, opItem);
    }
}

void MainWindow::aggiornaQsoDaTxDialog(Qso &qso, Ui::Tx *txUi) {
    if (!txUi) {
        return;
    }

    qso.locatoreTx = txUi->locatore->text().trimmed().toUpper();
    qso.radioTx = txUi->radio->currentText().trimmed();
    qso.potenzaTx = txUi->potenza->value();
    qso.trasmissioneTx = txUi->trasmissione->currentText().trimmed();
    qso.qsl = txUi->qsl->checkState() == Qt::CheckState::Checked;

    qso.nominativoTx.clear();
    for (int row = 0; row < txUi->nominativiList->rowCount(); ++row) {
        QTableWidgetItem *nomItem = txUi->nominativiList->item(row, 0);
        QTableWidgetItem *opItem = txUi->nominativiList->item(row, 1);
        if (!nomItem) {
            continue;
        }
        Qso::NominativoNome nominativo;
        nominativo.nominativo = nomItem->text().trimmed().toUpper();
        nominativo.operatore = opItem ? opItem->text().trimmed().toUpper() : QString();
        if (!nominativo.nominativo.isEmpty()) {
            qso.nominativoTx.append(nominativo);
        }
    }
}

void MainWindow::aggiungiNominativoTx(Ui::Tx *txUi) {
    if (!txUi) {
        return;
    }

    QString nominativo = txUi->nominativo->currentText().trimmed().toUpper();
    QString operatore = txUi->operatore->text().trimmed().toUpper();

    if (!nominativo.isEmpty()) {
        bool duplicato = false;
        for (int row = 0; row < txUi->nominativiList->rowCount(); ++row) {
            QTableWidgetItem *item = txUi->nominativiList->item(row, 0);
            if (item && item->text() == nominativo) {
                duplicato = true;
                break;
            }
        }

        if (!duplicato) {
            int rowCount = txUi->nominativiList->rowCount();
            txUi->nominativiList->insertRow(rowCount);
            QTableWidgetItem *nomItem = new QTableWidgetItem(nominativo);
            QTableWidgetItem *opItem = new QTableWidgetItem(operatore);
            txUi->nominativiList->setItem(rowCount, 0, nomItem);
            txUi->nominativiList->setItem(rowCount, 1, opItem);

            aggiungiNominativoCombo(txUi->nominativo, nominativo);
            aggiungiNominativoCombo(tx->nominativo, nominativo);
        }
    }
}

void MainWindow::eliminaNominativoTx(Ui::Tx *txUi) {
    if (!txUi) {
        return;
    }

    int selectedRow = txUi->nominativiList->currentRow();
    if (selectedRow != -1) {
        txUi->nominativiList->removeRow(selectedRow);
    }
}

void MainWindow::setSelectedNominativoDBTx(const QString &txt, Ui::Tx *txUi) {
    if (!txUi) {
        return;
    }

    QSqlQuery *q = RFLog->getQueryBind();
    q->prepare("select operatore from qsoNominativi where nominativo = :nominativo and operatore is not null and operatore <> '' order by operatore limit 1");
    q->bindValue(":nominativo", txt);
    DBResult *res = RFLog->executeQuery(q);
    txUi->operatore->setText(res->getCella(0));
    delete q;
    delete res;
}

void MainWindow::modificaAltri(QVector<Qso::AltriParametri> &altro, QWidget *parent) {
    QDialog dialog(parent ? parent : this);
    dialog.setWindowTitle(tr("Altri parametri"));
    dialog.resize(520, 360);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Nome") << tr("Valore"));
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(table);

    for (const auto &item : altro) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(item.nome));
        table->setItem(row, 1, new QTableWidgetItem(item.valore));
    }

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    QPushButton *aggiungi = new QPushButton(tr("Aggiungi"), &dialog);
    QPushButton *elimina = new QPushButton(tr("Elimina"), &dialog);
    actionsLayout->addWidget(aggiungi);
    actionsLayout->addWidget(elimina);
    actionsLayout->addStretch();
    layout->addLayout(actionsLayout);

    connect(aggiungi, &QPushButton::clicked, &dialog, [table]() {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem());
        table->setItem(row, 1, new QTableWidgetItem());
        table->setCurrentCell(row, 0);
    });

    connect(elimina, &QPushButton::clicked, &dialog, [table]() {
        int row = table->currentRow();
        if (row >= 0) {
            table->removeRow(row);
        }
    });

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Salva"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Annulla"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QVector<Qso::AltriParametri> nuovoAltro;
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *nomeItem = table->item(row, 0);
        QTableWidgetItem *valoreItem = table->item(row, 1);
        const QString nome = nomeItem ? nomeItem->text().trimmed() : QString();
        const QString valore = valoreItem ? valoreItem->text().trimmed() : QString();
        if (nome.isEmpty()) {
            continue;
        }
        Qso::AltriParametri parametro;
        parametro.nome = nome;
        parametro.valore = valore;
        nuovoAltro.append(parametro);
    }
    altro = nuovoAltro;
}

void MainWindow::aggiornaTabella()
{
    aggiornaEtichettaSuccessivo();
    ui->Tabella->setUpdatesEnabled(false);
    const bool wasSortingEnabled = ui->Tabella->isSortingEnabled();
    ui->Tabella->setSortingEnabled(false);

    // Svuota la tabella (sia contenuto sia intestazioni).
    ui->Tabella->clearContents();

    // Imposta il numero di righe e di colonne (se serve).
    // Se la tabella è già configurata, puoi evitare queste righe.
    ui->Tabella->setRowCount(qsoList.count());

    QStringList indiciRighe;
    indiciRighe.reserve(qsoList.count());
    for (int i = 0; i < qsoList.count(); ++i) {
        indiciRighe << QString::number(qsoList.count() - i);
    }
    ui->Tabella->setVerticalHeaderLabels(indiciRighe);


    for (int i = 0; i < qsoList.count(); i++) {
        aggiungiATabella(*qsoList[i], i);
    }

    ui->Tabella->resizeRowsToContents();
    ui->Tabella->setSortingEnabled(wasSortingEnabled);

    ui->Tabella->setUpdatesEnabled(true);
    ui->Tabella->viewport()->update();
}

void MainWindow::aggiungiATabella(const Qso & qso, int row) {
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
        if(!qso.progressivoRx.trimmed().isEmpty()) {
            txt += " | ";
            txt += tr("Prog. RX");
            txt += " ";
            txt += qso.progressivoRx.trimmed();
        }

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


        // INFO
        item = new QTableWidgetItem(QString::number(Coordinate::distanzaKm(qso.locatoreTx, qso.locatoreRx), 'f', 2) + " Km");

        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // sfondo
        if(qso.duplicato)
            item->setBackground(rosso);
        else
            item->setBackground(verde);

        // Inserisci l'item nella tabella
        ui->Tabella->setItem(row, 6, item);
}


void MainWindow::setMatriceConAspect(const QString &locatoreDa, const QString &locatoreA) {
    if (!Coordinate::validaLocatore(locatoreDa) || !Coordinate::validaLocatore(locatoreA)) {
        mappa->setMatrice(locatoreDa, locatoreA);
        return;
    }

    const int widgetWidth = mappa->width();
    const int widgetHeight = mappa->height();
    if (widgetWidth <= 0 || widgetHeight <= 0) {
        mappa->setMatrice(locatoreDa, locatoreA);
        return;
    }

    int rowDa;
    int colDa;
    int rowA;
    int colA;
    Coordinate::toRowCol(locatoreDa, rowDa, colDa);
    Coordinate::toRowCol(locatoreA, rowA, colA);

    int rTop = qMax(rowDa, rowA);
    int rBottom = qMin(rowDa, rowA);
    int cLeft = qMin(colDa, colA);
    int cRight = qMax(colDa, colA);

    int rows = rTop - rBottom + 1;
    int cols = cRight - cLeft + 1;

    if (rows <= 0 || cols <= 0) {
        mappa->setMatrice(locatoreDa, locatoreA);
        return;
    }

    const double targetRatio = static_cast<double>(widgetWidth) / static_cast<double>(widgetHeight);
    const double currentRatio = static_cast<double>(cols) / static_cast<double>(rows);

    if (!qFuzzyCompare(currentRatio, targetRatio)) {
        if (currentRatio < targetRatio) {
            int desiredCols = static_cast<int>(std::ceil(targetRatio * rows));
            int extra = desiredCols - cols;
            cLeft -= extra / 2;
            cRight += extra - (extra / 2);
        } else {
            int desiredRows = static_cast<int>(std::ceil(static_cast<double>(cols) / targetRatio));
            int extra = desiredRows - rows;
            rBottom -= extra / 2;
            rTop += extra - (extra / 2);
        }
    }

    cLeft = qBound(0, cLeft, colonnePerRiga - 1);
    cRight = qBound(0, cRight, colonnePerRiga - 1);
    rBottom = qBound(0, rBottom, righePerColonna - 1);
    rTop = qBound(0, rTop, righePerColonna - 1);

    mappa->setMatrice(Coordinate::fromRowCol(rTop, cLeft),
                      Coordinate::fromRowCol(rBottom, cRight));
}

void MainWindow::aggiungiNominativoCombo(QComboBox *combo, const QString &nominativo) {
    if (!combo) {
        return;
    }

    const QString upper = nominativo.trimmed().toUpper();
    if (upper.isEmpty()) {
        return;
    }

    for (int i = 0; i < combo->count(); ++i) {
        if (combo->itemText(i).trimmed().toUpper() == upper) {
            return;
        }
    }

    combo->addItem(upper);
}

void MainWindow::configuraPreferitiTable(QTableWidget *table) {
    if (!table) {
        return;
    }

    table->setColumnCount(2);
    table->verticalHeader()->setDefaultSectionSize(10);
    QFont font = table->font();
    font.setPointSize(8);
    table->setFont(font);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->Tabella && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            const QModelIndex current = ui->Tabella->currentIndex();
            if (current.isValid()) {
                modificaTxDaTabella(current);
                return true;
            }
        }
        if (keyEvent->key() == Qt::Key_Delete) {
            eliminaQsoSelezionato();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::eliminaQsoSelezionato() {
    const int row = ui->Tabella->currentRow();
    if (row < 0 || row >= qsoList.count()) {
        return;
    }

    Qso *qso = qsoList[row];
    if (!qso) {
        return;
    }

    const QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        tr("Conferma eliminazione"),
        tr("Sei sicuro di voler cancellare la riga selezionata?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (confirm != QMessageBox::Yes) {
        return;
    }

    qso->eliminaDB();
    qsoList.removeAt(row);
    delete qso;

    ricalcolaDuplicatiQso();
    aggiornaEtichettaSuccessivo();
    aggiornaTabella();
    updateMappaLocatori();
}
