#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <cmath>
#include <cstring>

#define numLetter 18
#define numDigit 10
#define numSub 24
#define maxCoord 4320
#define totaloneCelle 18662400
#define colonnePerRiga 4320
#define righePerColonna 4320
#define divisionePrimo 18
#define divisioneSecondo 10
#define divisioneTerzo 24

class DBResult {
public:
    DBResult();

    QVector<QString> colonne;
    QVector<QVector<QString>> tabella;
    bool successo;
};

class Coordinate {
public:
    Coordinate();
    bool operator==(const Coordinate &other) const;
    bool operator!=(const Coordinate &other) const;
    bool operator<(const Coordinate &other) const;

    static bool validaLocatore(const QString & locatore);
    static QString calcolaCoordinate(const QString &locatore, int offsetX, int offsetY);

    static void toRowCol(const QString &loc, int &row, int &col);
    static QString fromRowCol(int row, int col);

    QString getLocatore() const;
    void setLocatore(const QString &loc);

    unsigned char getColoreStato() const;
    void setColoreStato(unsigned char c);
    unsigned char getColoreRegione() const;
    void setColoreRegione(unsigned char c);
    unsigned char getColoreProvincia() const;
    void setColoreProvincia(unsigned char c);
    unsigned char getColoreComune() const;
    void setColoreComune(unsigned char c);

private:
    char locatore[6];
    unsigned char colore_stato;
    unsigned char colore_regione;
    unsigned char colore_provincia;
    unsigned char colore_comune;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &databasePath, QObject *parent = nullptr);
    ~DatabaseManager();

    bool openDatabase();
    void closeDatabase();

    QVector<QVector<Coordinate*>> * caricaMatriceDaDb(QString locatore_da, QString locatore_a);

    DBResult* executeQuery(const QString &queryStr);
    QString lastError() const;

    static QString escape(const QString &txt);

signals:
    void primoLocatore(QString primo, QString ultimo);

private:
    QSqlDatabase m_database;
    QString m_databasePath;

    QMutex mutex;
};

#endif // DATABASEMANAGER_H
