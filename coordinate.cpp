#include "coordinate.h"

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

bool Coordinate::operator==(const QString &other) const {
    char otherLoc[6];
    std::memcpy(otherLoc, other.toUtf8().constData(), sizeof(otherLoc));

    return std::strncmp(locatore, otherLoc, sizeof(locatore)) == 0;
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

float Coordinate::getAltezza() const {
    return altezza;
}

void Coordinate::setAltezza(float a) {
    altezza = a;
}

unsigned char Coordinate::getColoreComune() const {
    return colore_comune;
}

void Coordinate::setColoreComune(unsigned char c) {
    colore_comune = c;
}

const char* Coordinate::getRawLocatore() const {
    return locatore;
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

QString Coordinate::calcolaLocatoreLatLon(int lat, int lon) {
    QString loc;

    // Verifica che lat e lon siano nei limiti validi
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        return QString();
    }

    // Funzione per convertire un numero in una lettera
    auto numeroToLettera = [](int n) -> QChar {
        return QChar('A' + n);
    };

    // Primo livello (blocchi principali)
    int lonLetter = static_cast<int>((lon + 180) / 20);
    int latLetter = static_cast<int>((lat + 90) / 10);

    // Secondo livello (blocchi numerici)
    double lonRemainder = (lon + 180) - lonLetter * 20;
    double latRemainder = (lat + 90) - latLetter * 10;

    int lonDigit = static_cast<int>(lonRemainder / 2);
    int latDigit = static_cast<int>(latRemainder / 1);

    // Residuo dopo il secondo livello
    lonRemainder -= lonDigit * 2;  // ora lonRemainder è in [0..2)
    latRemainder -= latDigit * 1;  // ora latRemainder è in [0..1)

    // Terzo livello:
    // - per la longitudine, 24 sottogriglie in 2 gradi => ciascuna è 1/12°
    // - per la latitudine, 24 sottogriglie in 1 grado => ciascuna è 1/24°

    int lonSubLetter = static_cast<int>(lonRemainder * 12);
    int latSubLetter = static_cast<int>(latRemainder * 24);

    // Aggiusta eventuali errori di arrotondamento
    lonSubLetter = std::clamp(lonSubLetter, 0, 23);
    latSubLetter = std::clamp(latSubLetter, 0, 23);

    // Costruzione del locatore
    loc.append(numeroToLettera(lonLetter));       // Lettera longitudine principale
    loc.append(numeroToLettera(latLetter));       // Lettera latitudine principale
    loc.append(QString::number(lonDigit));       // Numero longitudine secondaria
    loc.append(QString::number(latDigit));       // Numero latitudine secondaria
    loc.append(numeroToLettera(lonSubLetter));   // Lettera longitudine terziaria
    loc.append(numeroToLettera(latSubLetter));   // Lettera latitudine terziaria

    return loc;
}

void Coordinate::calcolaLatLonLocatore(const QString &loc, double &lat, double &lon)
{
    // Converto il locator in maiuscolo per evitare problemi di case
    QString locator = loc.toUpper();
    int length = locator.size();

    // Se il locator è troppo corto per essere valido (minimo 2 caratteri),
    // mettiamo lat/lon = 0 e usciamo
    if (length < 2) {
        lat = 0;
        lon = 0;
        return;
    }

    // Variabili di lavoro in double
    double lonTemp = 0.0;
    double latTemp = 0.0;

    // ---------------------------------------------------
    // 1) Prima coppia: 2 LETTERE (obbligatorie)
    //    locator[0] -> LONGITUDINE (A=0, B=1, ..., R=17) * 20°
    //    locator[1] -> LATITUDINE  (A=0, B=1, ..., R=17) * 10°
    // ---------------------------------------------------
    // NB: 'A' in ASCII = 65
    //    (locator[0] - 'A') va da 0 a 17 (se lettera da A a R)
    //    Controlli e limiti andrebbero aggiunti per robustezza.

    lonTemp = (locator[0].toLatin1() - 'A') * 20.0;
    latTemp = (locator[1].toLatin1() - 'A') * 10.0;

    // ---------------------------------------------------
    // 2) Seconda coppia: 2 CIFRE (opzionale, ma spesso presente)
    //    locator[2] -> LONGITUDINE (0-9) * 2°
    //    locator[3] -> LATITUDINE  (0-9) * 1°
    // ---------------------------------------------------
    if (length >= 4) {
        lonTemp += (locator[2].toLatin1() - '0') * 2.0;
        latTemp += (locator[3].toLatin1() - '0') * 1.0;
    }

    // ---------------------------------------------------
    // 3) Terza coppia: 2 LETTERE (opzionale)
    //    locator[4] -> LONGITUDINE in passi di 1/12 di grado
    //    locator[5] -> LATITUDINE  in passi di 1/24 di grado
    // ---------------------------------------------------
    if (length >= 6) {
        lonTemp += (locator[4].toLatin1() - 'A') * (2.0 / 24.0);
        latTemp += (locator[5].toLatin1() - 'A') * (1.0 / 24.0);
    }

    // ---------------------------------------------------
    // 4) Quarta coppia: 2 CIFRE (opzionale)
    //    locator[6] -> LONGITUDINE in passi di (2/24)/10 = 1/120 di grado
    //    locator[7] -> LATITUDINE  in passi di (1/24)/10 = 1/240 di grado
    // ---------------------------------------------------
    if (length >= 8) {
        lonTemp += (locator[6].toLatin1() - '0') * (2.0 / 240.0);
        latTemp += (locator[7].toLatin1() - '0') * (1.0 / 240.0);
    }

    // ---------------------------------------------------
    // SHIFT finale:
    // - Per definizione, a inizio calcolo abbiamo un "offset" di -180° in longitudine,
    //   e di -90° in latitudine. Quindi adesso sottraiamo.
    // ---------------------------------------------------
    lonTemp -= 180.0;
    latTemp -= 90.0;

    // ---------------------------------------------------
    // OPZIONALE: spostare al "centro" della cella
    // Molti preferiscono posizionarsi al centro
    // del quadrato/cella. Significa aggiungere metà
    // dell'ampiezza dell'ultimo step usato.
    //
    // Esempio se il locator ha 4 caratteri:
    //   - la cella è di 2° in longitudine e 1° in latitudine
    //   - quindi potremmo aggiungere +1.0 a lonTemp, +0.5 a latTemp
    //
    // Qui, come esempio, lo facciamo automaticamente:
    //

    if (length == 2) {
        // Cella 20° x 10°, metà cella = 10° x 5°
        lonTemp += 10.0;
        latTemp += 5.0;
    } else if (length == 4) {
        // L'ultimo "livello" è di 2° x 1°, metà = 1° x 0.5°
        lonTemp += 1.0;
        latTemp += 0.5;
    } else if (length == 6) {
        // L'ultimo livello è 2/24° x 1/24°, metà = 1/24° x 1/48°
        lonTemp += (1.0 / 24.0);
        latTemp += (1.0 / 48.0);
    } else if (length >= 8) {
        // L'ultimo livello è (2/240)° x (1/240)°, metà = (1/240)° x (1/480)°
        lonTemp += (1.0 / 240.0);
        latTemp += (1.0 / 480.0);
    }

    lon = lonTemp;
    lat = latTemp;
}

Coordinate::CqItu Coordinate::getCqItu(const QString & loc) {
    CqItu cqitu;
    cqitu.cq = getCq(loc);
    cqitu.itu = getItu(loc);
    return cqitu;
}

int Coordinate::getCq(const QString & loc) {
    QVector<int> idQuadrato;
    double lat, lon;
    calcolaLatLonLocatore(loc, lat, lon);
    QPointF puntoLatLonInverso(lon, lat);
    for(int i = 0; i < zoneCqRect.count(); i++)
        if(zoneCqRect[i].contains(puntoLatLonInverso))
            idQuadrato.push_back(i);

    puntoLatLonInverso.setX(lon - 360);
    for(int i = 0; i < zoneCqRect.count(); i++) {
        if(zoneCqRect[i].contains(puntoLatLonInverso)) {
            idQuadrato.push_back(i);
        }
    }

    for(int i = 0; i < idQuadrato.count(); i++)
        if(zoneCq[idQuadrato[i]].polygon.containsPoint(puntoLatLonInverso, Qt::OddEvenFill))
            return zoneCq[idQuadrato[i]].number;

    if(idQuadrato.count() > 0)
        return zoneCq[idQuadrato[0]].number;

    return -1;
}

int Coordinate::getItu(const QString & loc) {
    QVector<int> idQuadrato;
    double lat, lon;
    calcolaLatLonLocatore(loc, lat, lon);
    QPointF puntoLatLonInverso(lon, lat);
    for(int i = 0; i < zoneItuRect.count(); i++) {
        if(zoneItuRect[i].contains(puntoLatLonInverso)) {
            idQuadrato.push_back(i);
        }
    }

    puntoLatLonInverso.setX(lon - 360);
    for(int i = 0; i < zoneItuRect.count(); i++) {
        if(zoneItuRect[i].contains(puntoLatLonInverso)) {
            idQuadrato.push_back(i);
        }
    }

    puntoLatLonInverso.setX(lon + 360);
    for(int i = 0; i < zoneItuRect.count(); i++) {
        if(zoneItuRect[i].contains(puntoLatLonInverso)) {
            idQuadrato.push_back(i);
        }
    }

    for(int i = 0; i < idQuadrato.count(); i++)
        if(zoneItu[idQuadrato[i]].polygon.containsPoint(puntoLatLonInverso, Qt::OddEvenFill))
            return zoneItu[idQuadrato[i]].number;

    if(idQuadrato.count() > 0)
        return zoneItu[idQuadrato[0]].number;

    return -1;
}

// Helper per calcolare QRectF da un QVector<QPolygonF>
QVector<QRectF> Coordinate::calculateRects(const QVector<QPolygonF>& polygons) {
    QVector<QRectF> rects;
    for (const auto& polygon : polygons) {
        rects.append(polygon.boundingRect());
    }
    return rects;
}

// Funzione per inizializzare zoneItuRect
QVector<QRectF> Coordinate::initZoneItuRect() {
    QVector<QRectF> ret;
    for (int i = 0; i < zoneItu.count(); i++) {
        ret.append(zoneItu[i].polygon.boundingRect());
    }
    return ret;
}

// Funzione per inizializzare zoneCqRect
QVector<QRectF> Coordinate::initZoneCqRect() {
    QVector<QRectF> ret;
    for (int i = 0; i < zoneCq.count(); i++) {
        ret.push_back(zoneCq[i].polygon.boundingRect());
    }
    return ret;
}

