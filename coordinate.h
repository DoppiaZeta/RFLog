#ifndef COORDINATE_H
#define COORDINATE_H

#include <QString>
#include <QPolygonF>
#include <QPointF>
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
    struct CqItu {
        int cq;
        int itu;
    };

    Coordinate();

    bool operator==(const Coordinate &other) const;
    bool operator==(const QString &other) const;
    bool operator!=(const Coordinate &other) const;
    bool operator<(const Coordinate &other) const;

    static bool validaLocatore(const QString & locatore);
    static QString calcolaCoordinate(const QString &locatore, int offsetX, int offsetY);

    static CqItu getCqItu(const QString & loc);

    QString getLocatore() const;
    void setLocatore(const QString &loc);

    static void toRowCol(const QString &loc, int &row, int &col);
    static QString fromRowCol(int row, int col);

    static QString calcolaLocatoreLatLon(int lat, int lon);
    static void calcolaLatLonLocatore(const QString &loc, double &lat, double &lon);

    const char* getRawLocatore() const;

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

protected:
    constexpr static int cqitu_molt = 100000;

    struct ZoneItu {
        int number;
        QPolygon polygon;
    };
    static const QVector<ZoneItu> zoneItu;
    static const QVector<QRect> zoneItuRect;

    struct ZoneCq {
        int number;
        QPolygon polygon;
    };
    static const QVector<ZoneCq> zoneCq;
    static const QVector<QRect> zoneCqRect;

    // Funzione helper per inizializzare zoneItuRect e zoneCqRect
    static QVector<QRect> calculateRects(const QVector<QPolygon>& polygons);

    static int getCq(const QString & loc);
    static int getItu(const QString & loc);

private:
    char locatore[6];
    unsigned char colore_stato;
    unsigned char colore_regione;
    unsigned char colore_provincia;
    unsigned char colore_comune;
    float altezza;

    static QVector<QRect> initZoneItuRect();
    static QVector<QRect> initZoneCqRect();
};

#endif // COORDINATE_H
