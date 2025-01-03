#ifndef MAPPA_H
#define MAPPA_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtConcurrent/QtConcurrent>
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

signals:
    void mouseLocatore(QString locatore); // Segnale per il quadrato sotto il mouse
    void mouseOceano();
    void matriceCaricata();


protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QVector<QVector<Coordinate*>> *m_matrice;

    void drawSquare(float x, float y, float width, float height, const QColor &color, bool border = false);
    QList<Linee> *linee;
    QString primoLocatore;
    QString ultimoLocatore;

    DatabaseManager *db;

private slots:
    void setPrimoLoatore(QString primo, QString secondo);
};

#endif // MAPPA_H
