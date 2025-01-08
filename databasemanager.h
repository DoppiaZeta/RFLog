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

    bool openDatabase();
    void closeDatabase();

    DBResult* executeQuery(const QString &queryStr);
    void executeQueryNoRes(const QString &queryStr);
    QString lastError() const;

    static QString escape(const QString &txt);

private:
    QSqlDatabase m_database;
    QString m_databasePath;

    QMutex mutex;
};

#endif // DATABASEMANAGER_H
