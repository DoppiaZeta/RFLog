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

Coordinate::Coordinate() {
    std::memset(locatore, 0, sizeof(locatore));
    colore_stato = 1;
    colore_regione = 0;
    colore_provincia = 0;
    colore_comune = 0;
}

bool Coordinate::operator==(const Coordinate &other) const {
    return std::strncmp(locatore, other.locatore, sizeof(locatore)) == 0;
}

bool Coordinate::operator!=(const Coordinate &other) const {
    return !(*this == other);
}

bool Coordinate::operator<(const Coordinate &other) const {
    return std::strncmp(locatore, other.locatore, sizeof(locatore)) < 0;
}

QString Coordinate::getLocatore() const {
    return QString::fromUtf8(locatore, sizeof(locatore));
}

void Coordinate::setLocatore(const QString &loc) {
    if (loc.size() == sizeof(locatore))
        std::memcpy(locatore, loc.toUtf8().constData(), sizeof(locatore));
}

unsigned char Coordinate::getColoreStato() const {
    return colore_stato;
}

void Coordinate::setColoreStato(unsigned char c) {
    colore_stato = c;
}

unsigned char Coordinate::getColoreRegione() const {
    return colore_regione;
}

void Coordinate::setColoreRegione(unsigned char c) {
    colore_regione = c;
}

unsigned char Coordinate::getColoreProvincia() const {
    return colore_provincia;
}

void Coordinate::setColoreProvincia(unsigned char c) {
    colore_provincia = c;
}

unsigned char Coordinate::getColoreComune() const {
    return colore_comune;
}

void Coordinate::setColoreComune(unsigned char c) {
    colore_comune = c;
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

bool Coordinate::validaLocatore(const QString & locatore) {
    // Controlla che il locatore abbia almeno 6 caratteri
    if (locatore.length() < 6) {
        return false;
    }

    // Controlla che i primi due caratteri siano lettere maiuscole (A-X)
    if (!(locatore[0] >= 'A' && locatore[0] <= 'X') ||
        !(locatore[1] >= 'A' && locatore[1] <= 'X')) {
        return false;
    }

    // Controlla che i caratteri 3 e 4 siano numeri (0-9)
    if (!locatore[2].isDigit() || !locatore[3].isDigit()) {
        return false;
    }

    // Controlla che i caratteri 5 e 6 siano lettere maiuscole (A-X)
    if (!(locatore[4] >= 'A' && locatore[4] <= 'X') ||
        !(locatore[5] >= 'A' && locatore[5] <= 'X')) {
        return false;
    }

    // Se tutti i controlli sono passati, il locatore è valido
    return true;
}

QString Coordinate::calcolaCoordinate(const QString &locatore, int offsetX, int offsetY) {
    offsetX = offsetX % colonnePerRiga;
    offsetY = offsetY % colonnePerRiga;

    // Validazione del locatore
    if (locatore.size() != 6) {
        qWarning() << "[calcolaCoordinate] Locatore non valido:" << locatore;
        return "AA00AA";  // Valore di fallback
    }

    // Converti il locatore in coordinate riga/colonna
    int row = 0, col = 0;
    toRowCol(locatore, row, col);

    // Applicazione degli offset
    col += offsetX;
    row += offsetY;

    // Regola `col` (x)
    if (col >= colonnePerRiga) {
        col = colonnePerRiga;
    } else if (col < 0) {
        col = 0;
    }

    // Regola `row` (y)
    if (row >= righePerColonna) {
        row = righePerColonna;
    } else if (row < 0) {
        row = 0;
    }

    // Converti le coordinate aggiornate in un nuovo locatore
    return fromRowCol(row, col);
}



void Coordinate::toRowCol(const QString &loc, int &row, int &col) {
    int latField = loc[0].toLatin1() - 'A';
    int lonField = loc[1].toLatin1() - 'A';
    int latSquare = loc[2].toLatin1() - '0';
    int lonSquare = loc[3].toLatin1() - '0';
    int latSub = loc[4].toLatin1() - 'A';
    int lonSub = loc[5].toLatin1() - 'A';

    row = latField * 240 + latSquare * 24 + latSub;
    col = lonField * 240 + lonSquare * 24 + lonSub;
}

QString Coordinate::fromRowCol(int row, int col) {
    int latField = row / 240;
    int rowResto = row % 240;
    int latSquare = rowResto / 24;
    int latSub = rowResto % 24;

    int lonField = col / 240;
    int colResto = col % 240;
    int lonSquare = colResto / 24;
    int lonSub = colResto % 24;

    QChar c0 = QChar('A' + latField);
    QChar c1 = QChar('A' + lonField);
    QChar c2 = QChar('0' + latSquare);
    QChar c3 = QChar('0' + lonSquare);
    QChar c4 = QChar('A' + latSub);
    QChar c5 = QChar('A' + lonSub);

    return QString("%1%2%3%4%5%6").arg(c0).arg(c1).arg(c2).arg(c3).arg(c4).arg(c5);
}

QVector<QVector<Coordinate*>>* DatabaseManager::caricaMatriceDaDb(QString locatore_da, QString locatore_a) {
    int rowDa, colDa, rowA, colA;
    Coordinate::toRowCol(locatore_da, rowDa, colDa);
    Coordinate::toRowCol(locatore_a, rowA, colA);

    int rTop = qMax(rowDa, rowA);
    int rBottom = qMin(rowDa, rowA);
    int cLeft = qMin(colDa, colA);
    int cRight = qMax(colDa, colA);

    auto matrix = new QVector<QVector<Coordinate*>>();
    matrix->reserve(rTop - rBottom + 1);

    // Determina il numero totale di colonne e righe
    int totalCols = cRight - cLeft + 1;
    int totalRows = rTop - rBottom + 1;

    // Calcola il passo di riduzione progressivo
    auto calculateStep = [](int total) -> int {
        if (total <= 500) return 1;   // Precisione massima
        if (total <= 1200) return 2;   // Salta 1 ogni 2
        if (total <= 1800) return 4;   // Salta 3 ogni 4
        if (total <= 2500) return 7;   // Salta 7 ogni 8
        return 12;
    };

    int colStep = calculateStep(totalCols);
    int rowStep = calculateStep(totalRows);

    //qDebug() << "Moltiplicatori" << colStep << rowStep;

    emit primoLocatore(Coordinate::fromRowCol(rBottom, cRight), Coordinate::fromRowCol(rTop, cLeft));

    for (int c = cRight; c >= cLeft; c -= colStep) {
    //for (int c = cLeft; c <= cRight; c += colStep) {
        QVector<Coordinate*> rowVector;
        rowVector.reserve(totalRows / rowStep + 1);

        QStringList locatoriQuoted;

        // Costruisce la lista di locatori per la colonna corrente con salto progressivo
        //for (int r = rTop; r >= rBottom; r -= rowStep) {
        for (int r = rBottom; r <= rTop; r += rowStep) {
            QString newLoc = Coordinate::fromRowCol(r, c);
            Coordinate* coord = new Coordinate();
            coord->setLocatore(newLoc);
            rowVector.push_back(coord);
            locatoriQuoted.append(QString("('%1')").arg(newLoc));
        }

        // Inserisce i locatori in una tabella temporanea e recupera i dati aggiornati
        if (!locatoriQuoted.isEmpty()) {
            QString values = locatoriQuoted.join(", ");
            QString createTempTableQuery = "CREATE TEMP TABLE temp_locatori (locatore TEXT);";
            QString insertTempTableQuery = QString("INSERT INTO temp_locatori (locatore) VALUES %1;").arg(values);
            QString queryString = R"(
              SELECT l.locatore, l.colore_stato, l.colore_regione, l.colore_provincia, l.colore_comune
              FROM locatori l
              JOIN temp_locatori t
                ON l.locatore = t.locatore
            )";

            executeQuery(createTempTableQuery);
            executeQuery(insertTempTableQuery);

            DBResult* resQuery = executeQuery(queryString);

            // Mappa per aggiornare i dati della riga corrente
            QHash<QString, QVector<QString>> queryMap;
            for (const auto& row : resQuery->tabella) {
                queryMap.insert(row[0], row);
            }

            for (auto& coord : rowVector) {
                if (coord != nullptr) {
                    QString locatoreStr = coord->getLocatore();
                    if (queryMap.contains(locatoreStr)) {
                        const QVector<QString>& data = queryMap[locatoreStr];
                        coord->setColoreStato(data[1].toInt());
                        coord->setColoreRegione(data[2].toInt());
                        coord->setColoreProvincia(data[3].toInt());
                        coord->setColoreComune(data[4].toInt());
                    } else {
                        delete coord;
                        coord = nullptr;
                    }
                }
            }

            delete resQuery;
            executeQuery("DROP TABLE temp_locatori;"); // Rimuove la tabella temporanea
        }

        // Aggiunge la riga aggiornata alla matrice
        matrix->push_back(rowVector);
    }
/*
    qDebug() << matrix << matrix->at(0).at(matrix->at(0).size() - 1)->getLocatore();
    if(matrix->size() > 0 && matrix->at(0).size() > 0) {
        emit primoLocatore(matrix->at(0).at(matrix->at(0).size() - 1)->getLocatore());
    }
*/
    return matrix;
}


