#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QColor>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QClipboard>
#include <QDebug>
#include <QPalette>
#include <QComboBox>
#include <QActionGroup>
#include <QTranslator>
#include <memory>

#include "suggestivelineedit.h"
#include "databasemanager.h"
#include "mappa.h"
#include "qso.h"

#include "ui_mainwindow.h"
#include "ui_mappaconfig.h"
#include "ui_tx.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void svuotaLineEdit();
    void confermaLinea();
    void aggiornaOrario();

private slots:
    void compilaNominativo(const QString &txt);
    void aggiornaColoreNominativoDuplicato(const QString &txt);
    void aggiornaSuggerimentiNominativo(const QString &txt);
    void caricaLocatoriDaNominativo(const QString &nominativo);
    void locatoreDaMappa(QString loc);
    void locatoreDaMappaDPPCLK(QString loc);

    //void catturaTab();
    void centraDaLocatore();

    void centraPredefinitoITA();
    void centraPredefinitoEU();
    void centraPredefinitoNordAmerica();
    void centraPredefinitoSudAmerica();
    void centraPredefinitoAsia();
    void centraPredefinitoAfrica();
    void centraPredefinitoOceania();
    void centraPredefinitoMondo();

    void cercaRegione(const QString & txt);
    void cercaProvincia(const QString & txt);
    void cercaComune(const QString & txt);
    void cercaLocatore(const QString & txt);

    void caricaColoreStato(const QString & txt);
    void caricaColoreRegione(const QString & txt);
    void caricaColoreProvincia(const QString & txt);
    void caricaColoreComune(const QString & txt);

    void salvaColoreStato();
    void salvaColoreRegione();
    void salvaColoreProvincia();
    void salvaColoreComune();

    void pulisciCerca();
    void confermaCerca();
    void caricaDaA(QString da, QString a);
    void centraDAA();
    void centraLinee();
    void centraGruppo();

    void modificaCercaLocatore();
    void modificaCercaRegione(const QString & txt);
    void modificaCercaProvincia(const QString & txt);
    void modificaCercaComune(const QString & txt);
    void modificaSalva();

    void setPolitica();
    void setGeografica();
    void mappaGruppoSRPC();
    void mappaScreenshot();

    void setSelectedNominativoDB(const QString & txt);

    void menuApriAdif();
    void menuApriEdi();
    void menuEsportaAdifTx();
    void menuLocatoriPreferiti();
    void menuIniziaLog();
    void menuMiaRadio();
    void menuInformazioniSu();

    void aggiungiNominativo();
    void eliminaNominativo();
    void modificaTxDaTabella(const QModelIndex &index);

private:
    bool nominativoPresenteInLista(const QString &txt) const;
    DBResult * caricaStatiDB();
    DBResult * caricaRegioniDB(const QString & stato);
    DBResult * caricaProvinceDB(const QString & stato, const QString & regione);
    DBResult * caricaComuniDB(const QString & stato, const QString & regione, const QString & provincia);
    DBResult * caricaLocatoriDB(const QString & stato, const QString & regione, const QString & provincia, const QString & comune);

    DBResult * caricaColoreStatoDB(const QString & stato);
    DBResult * caricaColoreRegioneDB(const QString & stato, const QString & regione);
    DBResult * caricaColoreProvinciaDB(const QString & stato, const QString & regione, const QString & provincia);
    DBResult * caricaColoreComuneDB(const QString & stato, const QString & regione, const QString & provincia, const QString & comune);

    DBResult * caricaInfoLocatore(const QString & loc);

    void creaDBRFLog();

    void caricaModificaRegione();
    void caricaModificaProvincia();
    void caricaModificaComune();

    void caricaLocatoriPreferiti();
    void caricaMieRadio();
    void caricaNominativiDaDb();
    void caricaLocatoriPreferitiTx(Ui::Tx *txUi);
    void usaLocatorePreferitoTx(Ui::Tx *txUi);
    void usaLocatorePreferito();
    void configuraPreferitiTable(QTableWidget *table);
    void aggiungiNominativoCombo(QComboBox *combo, const QString &nominativo);


    void updateMappaLocatori();

    void aggiornaTabella();
    void aggiungiATabella(const Qso & qso, int row);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void popolaTxDialog(Ui::Tx *txUi, const Qso &qso);
    void aggiornaQsoDaTxDialog(Qso &qso, Ui::Tx *txUi);
    void aggiungiNominativoTx(Ui::Tx *txUi);
    void eliminaNominativoTx(Ui::Tx *txUi);
    void setSelectedNominativoDBTx(const QString &txt, Ui::Tx *txUi);
    void setMatriceConAspect(const QString &locatoreDa, const QString &locatoreA);
    void modificaAltri(QVector<Qso::AltriParametri> &altro, QWidget *parent = nullptr);
    void setupLanguageMenu();
    void applyLanguage(const QString &localeName);
    void applySystemLanguage();
    void retranslateUi();
    void updateTableHeaders();
    void updateColoreStatoItems();
    void aggiornaEtichettaSuccessivo();
    void ricalcolaDuplicatiQso();
    std::unique_ptr<QTranslator> createAppTranslator(const QString &localeName);
    static double parseFrequencyValue(const QString &text);
    static QStringList frequenzaSuggerimentiPrincipali();
    static double frequenzaDefault20mMHz();
    static QStringList segnaleSuggerimentiPrincipali();
    static QString rstDefaultFromMode(const QString &mode);

    Ui::MainWindow *ui;
    Ui::MappaConfig *mappaConfig;
    Ui::Tx *tx;
    QTimer *resizeTimer;
    QActionGroup *languageActionGroup = nullptr;
    std::unique_ptr<QTranslator> appTranslator;
    QString currentLocale;

    SuggestiveLineEdit *Nominativo;
    SuggestiveLineEdit *Progressivo;
    SuggestiveLineEdit *Locatore;
    SuggestiveLineEdit *Segnale;
    SuggestiveLineEdit *Frequenza;
    SuggestiveLineEdit *Orario;

    DatabaseManager *db;
    DatabaseManager *RFLog;
    DatabaseManager *nDB;
    Mappa *mappa;

    QList<Qso*> qsoList;
    int numeroLog;
    QString lastNominativoPrefix;
    bool nominativoPrefixSet;
    QPalette nominativoDefaultPalette;
    bool nominativoPaletteSet;
    QVector<Qso::AltriParametri> altriTx;

};

#endif // MAINWINDOW_H
