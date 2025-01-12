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

void Coordinate::calcolaLatLonLocatore(int lat, int lon, QString &loc)
{
    // Svuota la stringa di output
    loc.clear();

    // Controllo di validità delle coordinate
    // (puoi gestire gli errori come preferisci)
    if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
        loc = "Coordinate non valide";
        return;
    }

    // Helper per convertire un numero (0..25) in lettera ASCII (A..Z)
    auto numeroToLettera = [](int n) {
        return QChar('A' + n);
    };

    /*
     * Calcolo del Maidenhead Locator a 6 caratteri
     *
     *  1) Primo livello (blocchi principali)
     *     - latLetter = floor( (lat + 90) / 10 )
     *     - lonLetter = floor( (lon + 180) / 20 )
     *
     *  2) Secondo livello (blocchi numerici)
     *     - latRemainder = (lat + 90) - (latLetter * 10)
     *     - lonRemainder = (lon + 180) - (lonLetter * 20)
     *     - latDigit = floor(latRemainder / 1)
     *     - lonDigit = floor(lonRemainder / 2)
     *
     *  3) Terzo livello (sottogriglie)
     *     Dopo aver “consumato” i gradi interi,
     *     - latRemainder = latRemainder - latDigit
     *     - lonRemainder = lonRemainder - (lonDigit * 2)
     *     - latSubLetter = floor(latRemainder * 24)
     *     - lonSubLetter = floor(lonRemainder * 12)
     *
     *  4) Costruzione del codice: Lettere + Numeri + Lettere
     *     (es. "JN56PK")
     */

    // ---- PRIMO LIVELLO ----
    int latLetter = static_cast<int>((lat + 90) / 10);
    int lonLetter = static_cast<int>((lon + 180) / 20);

    // ---- SECONDO LIVELLO ----
    double latRemainder = (lat + 90) - (latLetter * 10);
    double lonRemainder = (lon + 180) - (lonLetter * 20);

    int latDigit = static_cast<int>(latRemainder / 1);
    int lonDigit = static_cast<int>(lonRemainder / 2);

    // Aggiorniamo il residuo dopo aver tolto i gradi “consumati”
    latRemainder -= latDigit;
    lonRemainder -= lonDigit * 2;

    // ---- TERZO LIVELLO (sottogriglie) ----
    // - latRemainder in [0..1], suddiviso in 24 = blocchi di 1/24 di grado
    // - lonRemainder in [0..2], suddiviso in 24 = blocchi di 1/12 di grado
    int latSubLetter = static_cast<int>(latRemainder * 24);
    int lonSubLetter = static_cast<int>(lonRemainder * 12);

    // Evita eventuali overshoot per problemi di floating-point
    latSubLetter = std::clamp(latSubLetter, 0, 23);
    lonSubLetter = std::clamp(lonSubLetter, 0, 23);

    // ---- COSTRUZIONE DEL LOCATORE ----
    loc.append(numeroToLettera(lonLetter));   // Lettera di longitudine (prima)
    loc.append(numeroToLettera(latLetter));   // Lettera di latitudine (prima)
    loc.append(QString::number(lonDigit));    // Cifra di longitudine
    loc.append(QString::number(latDigit));    // Cifra di latitudine
    loc.append(numeroToLettera(lonSubLetter));// Lettera di longitudine (terza)
    loc.append(numeroToLettera(latSubLetter));// Lettera di latitudine (terza)
}

