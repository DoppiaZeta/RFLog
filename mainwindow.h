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
    void locatoreDaMappaOceano();

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
    void pulisciCerca();
    void confermaCerca();


private:
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
