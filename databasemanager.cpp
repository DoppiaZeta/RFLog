#include "databasemanager.h"
#include <cstring>

DBResult::DBResult() {
    successo = true;
}

bool DBResult::isEmpty() const {
    return tabella.size() == 0;
}

bool DBResult::hasRows() const {
    return tabella.size() > 0;
}

DatabaseManager::DatabaseManager(const QString &databasePath, QObject *parent)
    : QObject(parent), m_databasePath(databasePath)
{
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(m_databasePath);
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase() {
    if (!m_database.open()) {
        qWarning() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase() {
    if (m_database.isOpen()) {
        m_database.close();
    }
}

QString DatabaseManager::escape(const QString &txt) {
    QString escaped = txt;
    escaped.replace("'", "''");  // Raddoppia i singoli apici, necessario per SQLite
    return escaped;
}

void DatabaseManager::executeQueryNoRes(const QString &queryStr) {
    delete executeQuery(queryStr);
}

DBResult* DatabaseManager::executeQuery(const QString &queryStr) {
    QMutexLocker locker(&mutex);
    DBResult *ret = new DBResult;

    if (!m_database.isOpen()) {
        qWarning() << "Database is not open.";
        ret->successo = false;
        return ret;
    }

    QSqlQuery query(m_database);

    if (!query.exec(queryStr)) {
        qWarning() << "Failed to execute query:" << query.lastError().text();
        ret->successo = false;
        return ret;
    }

    QSqlRecord riga = query.record();
    for (int i = 0; i < riga.count(); ++i) {
        ret->colonne.append(riga.fieldName(i));
    }

    while (query.next()) {
        QVector<QString> row;
        for (int i = 0; i < riga.count(); ++i) {
            row.append(query.value(i).toString());
        }
        ret->tabella.append(row);
    }

    ret->successo = true;
    return ret;
}

QString DatabaseManager::lastError() const {
    return m_database.lastError().text();
}



