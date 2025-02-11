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
    int size() const;
    int count() const;

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
    QString lastError();

    static QString escape(const QString &txt);

    void releaseConnection();
    void cleanUpConnections();

private:
    QSqlDatabase getConnection(bool scrittura = false);
    QString getConnectionName();

    QString m_databasePath;

    QMutex mutex;
    QMap<QString, QThread*> activeConnections;

    static int m_int_static;
    int m_int;
};

#endif // DATABASEMANAGER_H
