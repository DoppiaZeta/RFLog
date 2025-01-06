#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
#include <QColor>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include "suggestivelineedit.h"
#include "databasemanager.h"
#include "mappa.h"

#include "ui_mainwindow.h"
#include "ui_mappaconfig.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event);

public slots:
    void svuotaLineEdit();
    void confermaLinea();
    void aggiornaOrario();

private slots:
    void compilaNominativo(const QString &txt);
    void locatoreDaMappa(QString loc);
    void locatoreDaMappaDPPCLK(QString loc);

    void catturaTab();
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

    void modificaCercaLocatore();
    void modificaCercaRegione(const QString & txt);
    void modificaCercaProvincia(const QString & txt);
    void modificaCercaComune(const QString & txt);
    void modificaSalva();

private:
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

    void caricaModificaRegione();
    void caricaModificaProvincia();
    void caricaModificaComune();

    Ui::MainWindow *ui;
    Ui::MappaConfig *mappaConfig;
    SuggestiveLineEdit *Nominativo;
    SuggestiveLineEdit *Locatore;
    SuggestiveLineEdit *Segnale;
    SuggestiveLineEdit *Frequenza;
    SuggestiveLineEdit *Orario;

    QTableWidget *Tabella;

    DatabaseManager *db;
    Mappa *mappa;

};

#endif // MAINWINDOW_H
