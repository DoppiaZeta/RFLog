#ifndef MAPPA_H
#define MAPPA_H

#include <QPainter>
#include <QWidget>
#include <QElapsedTimer>
#include <QVector>
#include <QPair>
#include <QMouseEvent>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include "databasemanager.h"
#include "coordinate.h"
#include "linee.h"

class Mappa : public QWidget {
    Q_OBJECT

public:
    enum class tipoMappa {polica, geografica, stati, regioni, provincie, comuni};

    explicit Mappa(DatabaseManager *dbm, QWidget *mappaConfig, QWidget *parent = nullptr);
    ~Mappa();

    void setMatrice(const QString& locatore_da, const QString& locatore_a);
    void addLinea(const Linee l, bool refresh = true);
    void delLinea(const Linee l);
    void delAllLinee(bool refresh = true);
    void adattaMappaLinee();
    void reload();
    void setTipoMappa(tipoMappa t, bool ricarica = true);

    QString getStato() const;
    void setStato(const QString & s);
    tipoMappa getTipo() const;

signals:
    void mouseLocatore(QString locatore); // Segnale per il quadrato sotto il mouse
    void mouseLocatoreDPPCLK(QString locatore); // Segnale per il quadrato sotto il mouse
    void matriceCaricata();
    void matriceDaA(QString da, QString a);


protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    struct MatriceLoadRequest {
        QString locatoreDa;
        QString locatoreA;
        QString stato;
        tipoMappa tipo;
        QList<Linee> linee;
        int requestId;
    };

    struct MatriceLoadResult {
        QVector<QVector<Coordinate*>> *matrice;
        QString primoLocatore;
        QString ultimoLocatore;
        QString locatoreDa;
        QString locatoreA;
        int requestId;
    };

    QVector<QVector<Coordinate*>> * caricaMatriceDaDb(const MatriceLoadRequest &request, QString &primo, QString &ultimo);
    void startLoaderThread();
    void stopLoaderThread();
    void loaderLoop();
    void liberaMatrice(QVector<QVector<Coordinate *>> *matrice);

    void drawSquare(QPainter &painter, const QRect &rect, const QColor &color, bool border = false);
    void drawLine(QPainter &painter, float x1f, float y1f, float x2f, float y2f);
    void drawPin(QPainter &painter, float x, float y);
    QColor generateHierarchicalColor(const QColor &nationalColor, int regionCode, int provinceCode, int municipalityCode, float intensity);
    QColor calcolaColoreAltitudine(const float &altitudine);

    QString calcolaLocatoreMouse(QMouseEvent *event);
    void clessidra(QPainter &painter);
    QPointF mapToWidget(float xNorm, float yNorm) const;

    QPair<float, float> bezier(float t, const QVector<QPair<float, float>>& controlPoints);
    bool trovaStRePrCo(const Coordinate & dove, const Coordinate & cerca, tipoMappa tipoRicerca) const;

    QVector<QVector<Coordinate*>> *m_matrice;

    QList<Linee> *linee;
    QString primoLocatore;
    QString ultimoLocatore;
    QString stato;

    DatabaseManager *db;

    QElapsedTimer timer;
    float progress;

    tipoMappa tipomappa;

    QWidget *mappaConfigWidget;

    QThread *loaderThread;
    QMutex loaderMutex;
    QWaitCondition loaderCv;
    bool loaderRunning;
    bool hasPendingRequest;
    MatriceLoadRequest pendingRequest;
    int lastRequestId;
};

#endif // MAPPA_H
