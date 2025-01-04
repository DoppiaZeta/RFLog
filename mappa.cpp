#include "mappa.h"
#include <QMouseEvent>
#include <cmath>

bool Linee::operator==(const Linee& other) const {
    return locatore_da == other.locatore_da && locatore_a == other.locatore_a;
}

Linee::Linee(const QString &da, const QString &a) :
    locatore_da(da), locatore_a(a) {
}

Mappa::Mappa(DatabaseManager *dbm, QWidget *parent)
    : QOpenGLWidget(parent), m_matrice(nullptr), linee(new QVector<Linee>()) {
    db = dbm;
    connect(db, &DatabaseManager::primoLocatore, this, &Mappa::setPrimoLoatore);
    primoLocatore = QString();
    ultimoLocatore = QString();
}

Mappa::~Mappa() {
    if (m_matrice) {
        for (auto& row : *m_matrice) {
            for (auto& coord : row) {
                delete coord;
            }
        }
        delete m_matrice;
    }
    delete linee;
}

void Mappa::setPrimoLoatore(QString primo, QString ultimo) {
    primoLocatore = primo;
    ultimoLocatore = ultimo;
}

void Mappa::setMatrice(const QString& locatore_da, const QString& locatore_a) {
    // Libera la matrice precedente se esiste
    if (m_matrice) {
        for (auto& row : *m_matrice) {
            for (auto& coord : row) {
                delete coord;
            }
        }
        delete m_matrice;
        m_matrice = nullptr; // Evita dangling pointers
    }

    auto future = QtConcurrent::run([this, locatore_da, locatore_a]() {
        // Carica la matrice direttamente dal database
        QMutexLocker locker(&mutex);
        QVector<QVector<Coordinate*>>* nuovaMatrice = db->caricaMatriceDaDb(locatore_da, locatore_a);

        // Trasferisci la matrice al thread principale
        QMetaObject::invokeMethod(this, [this, nuovaMatrice]() {
            // Assegna la nuova matrice
            m_matrice = nuovaMatrice;

            // Aggiorna il widget
            update();

            // Emetti il segnale che la matrice è stata caricata
            emit matriceCaricata();
        }, Qt::QueuedConnection);
    });
}

void Mappa::addLinea(const Linee l, bool refresh) {
    if (!linee->contains(l)) {
        linee->append(l);
        if(refresh)
            update();
    }
}

void Mappa::delLinea(const Linee l) {
    if (linee->removeOne(l)) {
        update();
    }
}

void Mappa::delAllLinee() {
    linee->empty();
}

void Mappa::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(50 / 255.0f, 100 / 255.0f, 200 / 255.0f, 1.0f); // Blu oceano profondo pastello
}

void Mappa::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void Mappa::drawSquare(float x, float y, float width, float height, const QColor &color, bool border) {
    // Disegna il riempimento del quadrato
    glColor4f(color.redF(), color.greenF(), color.blueF(), 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Disegna il bordo se richiesto
    if (border) {
        glEnable(GL_BLEND); // Abilita il blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Configura il blending per trasparenza

        glColor4f(0.0f, 0.0f, 0.0f, 0.05f); // Nero per il bordo
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
        glEnd();

        glDisable(GL_BLEND); // Disabilita il blending dopo aver disegnato
    }
}

void Mappa::mousePressEvent(QMouseEvent *event) {
    if (!m_matrice || m_matrice->isEmpty())
        return;

    int rows = m_matrice->size();
    int cols = m_matrice->at(0).size();

    if (rows == 0 || cols == 0)
        return;

    QPointF pos = event->position();
    float mouseX = (2.0f * pos.x() / this->width()) - 1.0f;
    float mouseY = 1.0f - (2.0f * pos.y() / this->height());


    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;

    int col = static_cast<int>((mouseX + 1.0f) / cellWidth);
    int row = static_cast<int>((1.0f - mouseY) / cellHeight);

    if (col >= 0 && col < cols && row >= 0 && row < rows) {
        const Coordinate *coord = m_matrice->at(row).at(col);

        if (coord != nullptr) {
            emit mouseLocatore(coord->getLocatore());
        } else {
            emit mouseOceano();
        }
    }
}

void Mappa::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!m_matrice || m_matrice->isEmpty())
        return;

    int rows = m_matrice->size();
    int cols = m_matrice->at(0).size();

    if (rows == 0 || cols == 0)
        return;

    QPointF pos = event->position();
    float mouseX = (2.0f * pos.x() / this->width()) - 1.0f;
    float mouseY = 1.0f - (2.0f * pos.y() / this->height());


    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;

    int col = static_cast<int>((mouseX + 1.0f) / cellWidth);
    int row = static_cast<int>((1.0f - mouseY) / cellHeight);

    if (col >= 0 && col < cols && row >= 0 && row < rows) {
        const Coordinate *coord = m_matrice->at(row).at(col);

        if (coord != nullptr) {
            emit mouseLocatoreDPPCLK(coord->getLocatore());
        }
    }
}

QColor Mappa::generateHierarchicalColor(const QColor &nationalColor, int regionCode, int provinceCode, int municipalityCode, float intensity) {
    // Converti il colore in HSL per manipolazione
    float h, s, l, a;
    nationalColor.getHslF(&h, &s, &l, &a);

    // Parametri base per le variazioni
    float baseRegionHueShift = 0.1f;
    float baseProvinceHueShift = 0.02f;
    float baseMunicipalityLightShift = 0.05f;

    float baseRegionSaturationShift = 0.2f;
    float baseProvinceSaturationShift = 0.1f;
    float baseMunicipalitySaturationShift = 0.05f;

    // Scala le variazioni in base all'intensità
    float regionHueShift = baseRegionHueShift * intensity;
    float provinceHueShift = baseProvinceHueShift * intensity;
    float municipalityLightShift = baseMunicipalityLightShift * intensity;

    float regionSaturationShift = baseRegionSaturationShift * intensity;
    float provinceSaturationShift = baseProvinceSaturationShift * intensity;
    float municipalitySaturationShift = baseMunicipalitySaturationShift * intensity;

    // Calcola il nuovo colore
    h += (regionCode % 10) * regionHueShift / 10.0f;  // Cambia tonalità per regione
    h += (provinceCode % 10) * provinceHueShift / 10.0f; // Cambia tonalità per provincia
    l += (municipalityCode % 10) * municipalityLightShift / 10.0f; // Cambia luminosità per comune

    s += (regionCode % 10) * regionSaturationShift / 10.0f; // Cambia saturazione per regione
    s -= (provinceCode % 10) * provinceSaturationShift / 10.0f; // Riduce saturazione per provincia
    s -= (municipalityCode % 10) * municipalitySaturationShift / 10.0f; // Riduce saturazione per comune

    // Normalizza i valori di HSL
    h = std::fmod(h + 1.0f, 1.0f); // Assicura che h sia tra 0 e 1
    s = std::clamp(s, 0.0f, 1.0f);
    l = std::clamp(l, 0.0f, 1.0f);

    // Genera il nuovo colore
    QColor modifiedColor;
    modifiedColor.setHslF(h, s, l, a);

    return modifiedColor;
}

void Mappa::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_matrice == nullptr || m_matrice->isEmpty())
        return;

    int rows = m_matrice->size();
    int cols = m_matrice->at(0).size();

    if (rows == 0 || cols == 0)
        return;

    float cellWidth = 2.0f / cols;
    float cellHeight = 2.0f / rows;

    // Disegna i quadrati
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const Coordinate *coord = m_matrice->at(row).at(col);

            if (coord == nullptr)
                continue;

            float x = -1.0f + col * cellWidth;
            float y = 1.0f - row * cellHeight - cellHeight;

            QColor colore;
            switch (coord->getColoreStato()) {
            case 1:
                colore = QColor(170, 120, 210); // Viola pastello scuro
                break;
            case 2:
                colore = QColor(90, 150, 220); // Blu pastello scuro
                break;
            case 3:
                colore = QColor(70, 120, 70); // Verde oliva scuro
                break;
            case 4:
                colore = QColor(220, 220, 100); // Giallo pastello scuro
                break;
            case 5:
                colore = QColor(180, 140, 100); // Marrone sabbia pastello scuro
                break;
            case 6:
                colore = QColor(230, 150, 80); // Arancione pastello scuro
                break;
            default:
                colore = QColor(170, 170, 170); // Grigio pastello scuro
            }

            colore = generateHierarchicalColor(colore, coord->getColoreRegione(), coord->getColoreProvincia(), coord->getColoreComune(), 1.5);

            drawSquare(x, y, cellWidth, cellHeight, colore, cols <= 110);
        }
    }

/*
    // Linea diagonale principale (da angolo in alto a sinistra a angolo in basso a destra)
    glBegin(GL_LINES);
    glColor4f(0.0f, 1.0f, 0.0f, 0.1f); // Verde trasparente
    glVertex2f(-1.0f, 1.0f); // Angolo in alto a sinistra
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f); // Verde pieno
    glVertex2f(1.0f, -1.0f); // Angolo in basso a destra
    glEnd();

    // Linea diagonale opposta (da angolo in alto a destra a angolo in basso a sinistra)
    glBegin(GL_LINES);
    glColor4f(0.0f, 1.0f, 0.0f, 0.1f); // Verde trasparente
    glVertex2f(1.0f, 1.0f); // Angolo in alto a destra
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f); // Verde pieno
    glVertex2f(-1.0f, -1.0f); // Angolo in basso a sinistra
    glEnd();
*/

    if(!primoLocatore.isEmpty()) {
        int primox, primoy,ultimox, ultimoy, collen, rowlen;
        Coordinate::toRowCol(primoLocatore, primox, primoy);
        Coordinate::toRowCol(ultimoLocatore, ultimox, ultimoy);

        collen = ultimox - primox;
        rowlen = primoy - ultimoy;

        for (const Linee& linea : *linee) {
            int x1, y1, x2, y2;
            float x1f, x2f, y1f, y2f;

            Coordinate::toRowCol(linea.locatore_da, x1, y1);
            Coordinate::toRowCol(linea.locatore_a, x2, y2);

            x1f = static_cast<float>(x1 - primox) / collen;
            y1f = static_cast<float>(y1 - ultimoy) / rowlen;

            x1f = x1f * 2 - 1;
            y1f = y1f * 2 - 1;

            x2f = static_cast<float>(x2 - primox) / collen;
            y2f = static_cast<float>(y2 - ultimoy) / rowlen;

            x2f = x2f * 2 - 1;
            y2f = y2f * 2 - 1;
/*
            qDebug() << "Primo" << primoLocatore << primox << primoy;
            qDebug() << "Ultimo" << ultimoLocatore << ultimox << ultimoy;
            qDebug() << "Collen" << collen << "Rowlen" << rowlen;
            qDebug() << "Loc da" << linea.locatore_da << x1 << y1 << x1f << y1f;
            qDebug() << "Loc a" << linea.locatore_a << x2 << y2 << x2f << y2f;
*/

            // Imposta il colore rosso
            glColor4f(0.9f, 0.1f, 0.1f, 1.0f); // Rosso più tenue
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(1, 0x00FF); // Pattern tratteggiato
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex2f(x1f, y1f);
            glVertex2f(x2f, y2f);
            glEnd();
            glDisable(GL_LINE_STIPPLE);

            // Abilita il blending per la trasparenza
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            float triangleSize = 0.05f; // Dimensione del triangolo

            glBegin(GL_TRIANGLES);
            // Vertice inferiore (punta del triangolo) - colore pieno
            glColor4f(0.9f, 0.1f, 0.1f, 1.0f); // Rosso opaco
            glVertex2f(x2f, y2f);

            // Vertice sinistro - trasparente
            glColor4f(0.9f, 0.1f, 0.1f, 0.2f); // Rosso trasparente
            glVertex2f(x2f - triangleSize / 2, y2f + triangleSize);

            // Vertice destro - trasparente
            glColor4f(0.9f, 0.1f, 0.1f, 0.2f); // Rosso trasparente
            glVertex2f(x2f + triangleSize / 2, y2f + triangleSize);
            glEnd();

            // Ripristina il colore pieno per eventuali disegni successivi
            glColor4f(0.9f, 0.1f, 0.1f, 1.0f);



        }
    }
}

