#include "databasemanager.h"

static unsigned int DatabaseManager_id_univoco_static = 0;

DBResult::DBResult() {
    successo = true;
}

bool DBResult::isEmpty() const {
    return tabella.size() == 0;
}

bool DBResult::hasRows() const {
    return tabella.size() > 0;
}

int DBResult::getRigheCount() const {
    return tabella.length();
}

QString DBResult::getCella(int riga, const QString &colonna) {
    if(tabella.size() > riga) {
        int c = colonne.indexOf(colonna);
        if(c >= 0)
            return tabella[riga][c];
    }

    return QString();
}

QString DBResult::getCella(const QString &colonna) {
    return getCella(0, colonna);
}

DatabaseManager::DatabaseManager(const QString &databasePath, QObject *parent)
    : QObject(parent), m_databasePath(databasePath)
{
    uid= 0;
    // Genera un nome unico per la connessione
    QString connectionName = QString("connection_%1").arg(QString::number(DatabaseManager_id_univoco_static++));
    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName(m_databasePath);
    m_database.open();
}

DatabaseManager::~DatabaseManager() {
    if (m_database.isOpen()) {
        m_database.close();
    }
}

QSqlQuery * DatabaseManager::getQueryBind() {
    return new QSqlQuery(m_database);
}

QString DatabaseManager::escape(const QString &txt) {
    QString escaped = txt;
    escaped.replace("'", "''");  // Raddoppia i singoli apici, necessario per SQLite
    return escaped;
}

void DatabaseManager::executeQueryNoRes(const QString &queryStr) {
    delete executeQuery(queryStr);
}

void DatabaseManager::executeQueryNoRes(QSqlQuery *query) {
    delete executeQuery(query);
}

DBResult* DatabaseManager::executeQuery(const QString &queryStr) {
    QSqlQuery query(queryStr, m_database);
    return executeQuery(&query);
}

DBResult* DatabaseManager::executeQuery(QSqlQuery *query) {
    QMutexLocker locker(&mutex);
    DBResult *ret = new DBResult;

    if (!m_database.isOpen()) {
        qWarning() << "Database is not open.";
        ret->successo = false;
        return ret;
    }

    if (!query->exec()) {
        qWarning() << "Failed to execute query:" << query->lastError().text();
        qWarning() << "Query:" << query->lastQuery();
        ret->successo = false;
        return ret;
    }


    QSqlRecord riga = query->record();
    for (int i = 0; i < riga.count(); ++i) {
        ret->colonne.append(riga.fieldName(i));
    }

    while (query->next()) {
        QVector<QString> row;
        for (int i = 0; i < riga.count(); ++i) {
            row.append(query->value(i).toString());
        }
        ret->tabella.append(row);
    }

    ret->successo = true;
    return ret;
}

QString DatabaseManager::lastError() const {
    return m_database.lastError().text();
}

unsigned int DatabaseManager::uniqueId() {
    QMutexLocker locker(&mutex);
    return uid++;
}

