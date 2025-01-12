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

class DBResult {
public:
    DBResult();
    bool isEmpty() const;
    bool hasRows() const;
    int getRigheCount() const;

    QString getCella(const int & riga, const QString &colonna);
    QString getCella(const QString &colonna); // monoriga

    QString getCella(const int & riga, const int & colonna);
    QString getCella(const int & colonna); // monoriga

    QVector<QString> colonne;
    QVector<QVector<QString>> tabella;
    bool successo;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(const QString &databasePath, QObject *parent = nullptr);
    ~DatabaseManager();

    QSqlQuery * getQueryBind();
    DBResult* executeQuery(const QString &queryStr);
    DBResult* executeQuery(QSqlQuery *query);
    void executeQueryNoRes(const QString &queryStr);
    void executeQueryNoRes(QSqlQuery *query);
    QString lastError() const;

    unsigned int uniqueId();

    static QString escape(const QString &txt);

private:
    QSqlDatabase m_database;
    QString m_databasePath;

    QMutex mutex;
    unsigned int uid;
};

#endif // DATABASEMANAGER_H
