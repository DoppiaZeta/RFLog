#include "databasemanager.h"

int DatabaseManager::m_int_static = 0;

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

int DBResult::size() const {
    return tabella.length();
}

int DBResult::count() const {
    return tabella.length();
}

QString DBResult::getCella(const int & riga, const QString &colonna) {
    if(tabella.size() > riga && riga >= 0) {
        int c = colonne.indexOf(colonna);
        if(c >= 0)
            return tabella[riga][c];
    }

    return QString();
}

QString DBResult::getCella(const QString &colonna) {
    return getCella(0, colonna);
}

QString DBResult::getCella(const int & riga, const int & colonna) {
    if(tabella.size() > riga && colonne.size() > colonna && riga >= 0 && colonna >= 0) {
            return tabella[riga][colonna];
    }

    return QString();
}

QString DBResult::getCella(const int & colonna) {
    return getCella(0, colonna);
}

DatabaseManager::DatabaseManager(const QString &databasePath, QObject *parent)
    : QObject(parent), m_databasePath(databasePath), m_int(m_int_static++)
{
    // Genera un nome unico per la connessione
    QSqlDatabase m_database = getConnection(true);
    executeQueryNoRes("PRAGMA foreign_keys = ON;");
}

DatabaseManager::~DatabaseManager() {
    cleanUpConnections();
}

QSqlQuery * DatabaseManager::getQueryBind() {
    return new QSqlQuery(getConnection());
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
    QSqlQuery query(queryStr, getConnection());
    return executeQuery(&query);
}

DBResult* DatabaseManager::executeQuery(QSqlQuery *query) {
    DBResult *ret = new DBResult;

    if (!query->exec()) {
        qWarning() << "Failed to execute query:" << query->lastError().text();
        qWarning() << "Query:" << query->lastQuery();
        QString connectionName = getConnectionName();
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase db = QSqlDatabase::database(connectionName, false);
            if (!db.isValid()) {
                qWarning() << "Database connection is invalid.";
            } else if (!db.isOpen()) {
                qWarning() << "Database connection is not open:" << db.lastError().text();
            }
        }
        if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
            qWarning() << "QSQLITE driver not available. Install the Qt SQLite plugin (e.g. libqt5sql5-sqlite).";
        }
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

QString DatabaseManager::lastError() {
    return getConnection().lastError().text();
}

QString DatabaseManager::getConnectionName() {
    return QString("Conn_%1").arg(QString::number(m_int) + "_" + QString::number((quintptr)QThread::currentThread()));
}

QSqlDatabase DatabaseManager::getConnection(bool scrittura) {
    QMutexLocker locker(&mutex);
    QString connectionName = getConnectionName();

    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase dbret = QSqlDatabase::database(connectionName);

        if(dbret.isOpen())
            return dbret;
        else
            activeConnections.remove(connectionName);
    }

    // Crea una nuova connessione per il thread
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        qDebug() << "Errore nell'apertura del database:" << db.lastError().text();
    }

    QSqlQuery q(db);
    q.exec("PRAGMA temp_store=MEMORY;");

    if(scrittura) {
        q.exec("PRAGMA journal_mode=WAL;");
        q.exec("PRAGMA synchronous=NORMAL;");
        q.exec("PRAGMA cache_size=5000;");
        q.exec("PRAGMA locking_mode=NORMAL;");
        q.exec("PRAGMA busy_timeout=5000;");
    } else {
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        q.exec("PRAGMA journal_mode=OFF;");
        q.exec("PRAGMA synchronous=OFF;");
    }

    // Registra la connessione
    activeConnections[connectionName] = QThread::currentThread();

    return db;
}

void DatabaseManager::releaseConnection() {
    QMutexLocker locker(&mutex);
    QString connectionName = getConnectionName();

    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
        activeConnections.remove(connectionName);
    }
}

void DatabaseManager::cleanUpConnections() {
    QMutexLocker locker(&mutex);

    QList<QString> toRemove;
    for (auto it = activeConnections.begin(); it != activeConnections.end(); ++it) {
        if (!it.value()->isRunning()) { // Controlla se il thread è terminato
            toRemove.append(it.key());
        }
    }

    for (const auto &connectionName : toRemove) {
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase::removeDatabase(connectionName);
        }
        activeConnections.remove(connectionName);
    }
}

void DatabaseManager::transactionBegin() {
    getConnection().transaction();
}

void DatabaseManager::transactionCommit() {
    getConnection().commit();
}

void DatabaseManager::transactionRollback() {
    getConnection().rollback();
}
