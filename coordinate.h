#ifndef COORDINATE_H
#define COORDINATE_H

#include <QString>
#include <QDebug>

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
    float getAltezza() const;
    void setAltezza(float a);

private:
    char locatore[6];
    unsigned char colore_stato;
    unsigned char colore_regione;
    unsigned char colore_provincia;
    unsigned char colore_comune;
    float altezza;
};

#endif // COORDINATE_H
