#ifndef MAPPA_H
#define MAPPA_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include "databasemanager.h"

class Linee {
public:
    Linee(const QString &da, const QString &a);
    bool operator==(const Linee& other) const;

    QString locatore_da;
    QString locatore_a;
};


class Mappa : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit Mappa(DatabaseManager *dbm, QWidget *parent = nullptr);
    ~Mappa();

    void setMatrice(const QString& locatore_da, const QString& locatore_a);
    void addLinea(const Linee l, bool refresh = true);
    void delLinea(const Linee l);
    void delAllLinee();
    void reload();

signals:
    void mouseLocatore(QString locatore); // Segnale per il quadrato sotto il mouse
    void mouseLocatoreDPPCLK(QString locatore); // Segnale per il quadrato sotto il mouse
    void matriceCaricata();
    void matriceDaA(QString da, QString a);


protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QVector<QVector<Coordinate*>> *m_matrice;

    void drawSquare(float x, float y, float width, float height, const QColor &color, bool border = false);
    QColor generateHierarchicalColor(const QColor &nationalColor, int regionCode, int provinceCode, int municipalityCode, float intensity);

    QString calcolaLocatoreMouse(QMouseEvent *event);
    void clessidra();

    QList<Linee> *linee;
    QString primoLocatore;
    QString ultimoLocatore;

    DatabaseManager *db;
    QMutex mutex;

    QElapsedTimer timer;
    float progress;

private slots:
    void setPrimoLoatore(QString primo, QString secondo);
};

#endif // MAPPA_H
