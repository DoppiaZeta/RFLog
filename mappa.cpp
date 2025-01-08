#include "mappa.h"
#include <QMouseEvent>
#include <cmath>

bool Linee::operator==(const Linee& other) const {
    return locatore_da == other.locatore_da && locatore_a == other.locatore_a;
}

Linee::Linee(const QString &da, const QString &a) :
    locatore_da(da.trimmed().toUpper()), locatore_a(a.trimmed().toUpper()) {
}

Mappa::Mappa(DatabaseManager *dbm, QWidget *parent)
    : QOpenGLWidget(parent), m_matrice(nullptr), linee(new QVector<Linee>()) {
    db = dbm;
    primoLocatore = QString();
    ultimoLocatore = QString();
    timer.start();
    progress = 0;
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

    timer.start();
    update();

    auto future = QtConcurrent::run([this, locatore_da, locatore_a]() {
        // Carica la matrice direttamente dal database
        QMutexLocker locker(&mutex);
        QVector<QVector<Coordinate*>>* nuovaMatrice = caricaMatriceDaDb(locatore_da, locatore_a);

        // Trasferisci la matrice al thread principale
        QMetaObject::invokeMethod(this, [this, nuovaMatrice, locatore_a, locatore_da]() {
            // Assegna la nuova matrice
            m_matrice = nuovaMatrice;

            // Aggiorna il widget
            update();

            // Emetti il segnale che la matrice è stata caricata
            emit matriceCaricata();
            emit matriceDaA(locatore_da, locatore_a);
        }, Qt::QueuedConnection);
    });
}

void Mappa::addLinea(const Linee l, bool refresh) {
    if (!linee->contains(l) && Coordinate::validaLocatore(l.locatore_da) && Coordinate::validaLocatore(l.locatore_a)) {
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

QVector<QVector<Coordinate*>>* Mappa::caricaMatriceDaDb(QString locatore_da, QString locatore_a) {
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

    primoLocatore = Coordinate::fromRowCol(rBottom, cRight);
    ultimoLocatore = Coordinate::fromRowCol(rTop, cLeft);

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

            db->executeQueryNoRes(createTempTableQuery);
            db->executeQueryNoRes(insertTempTableQuery);

            DBResult* resQuery = db->executeQuery(queryString);

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
            db->executeQueryNoRes("DROP TABLE temp_locatori;"); // Rimuove la tabella temporanea
        }

        // Aggiunge la riga aggiornata alla matrice
        matrix->push_back(rowVector);
    }

    return matrix;
}

void Mappa::clessidra() {
    // Cancella lo schermo con un colore di sfondo
    glClearColor(20 / 255.0f, 50 / 255.0f, 100 / 255.0f, 1.0f); // Blu oceano molto scuro
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity(); // Resetta la matrice del modello

    // Colore della clessidra
    glColor3f(0.8f, 0.8f, 0.8f); // Bianco
    glLineWidth(3.0f); // Imposta lo spessore della linea a 2

    // Disegna il contorno della clessidra
    glBegin(GL_LINES);
    glVertex2f(-0.2f, 0.5f);  // Angolo superiore sinistro
    glVertex2f(0.2f, 0.5f);   // Angolo superiore destro

    glVertex2f(-0.2f, 0.5f);  // Angolo superiore sinistro
    glVertex2f(0.0f, 0.0f);   // Centro

    glVertex2f(0.2f, 0.5f);   // Angolo superiore destro
    glVertex2f(0.0f, 0.0f);   // Centro

    glVertex2f(-0.2f, -0.5f); // Angolo inferiore sinistro
    glVertex2f(0.2f, -0.5f);  // Angolo inferiore destro

    glVertex2f(-0.2f, -0.5f); // Angolo inferiore sinistro
    glVertex2f(0.0f, 0.0f);   // Centro

    glVertex2f(0.2f, -0.5f);  // Angolo inferiore destro
    glVertex2f(0.0f, 0.0f);   // Centro
    glEnd();

    // Aggiorna il riempimento in base al tempo
    int elapsed = timer.elapsed();
    float totalTime = 3000.0f;  // Tempo totale per il ciclo (3 secondi)
    float progress = (elapsed % int(totalTime)) / totalTime / 2;        // Progress parte inferiore

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Colore del riempimento (sabbia)


    auto yellowSandTransparent = []() {
        glColor4f(1.0f, 0.8f, 0.2f, 0.0f); // Giallo sabbia completamente trasparente
    };

    auto yellowSandOpaque = []() {
        glColor4f(1.0f, 0.8f, 0.2f, 1.0f); // Giallo sabbia completamente opaco
    };


    // Disegna il riempimento superiore (sabbia che scende)
    glBegin(GL_TRIANGLES);
    yellowSandOpaque();
    glVertex2f(-0.2f, 0.5f);          // Angolo superiore sinistro
    yellowSandTransparent();
    glVertex2f(0.0f, 0.5f - progress);           // Angolo superiore destro
    yellowSandOpaque();
    glVertex2f(0.0f, 0.0f); // Punto variabile (altezza sabbia)
    glEnd();

    // Disegna il riempimento superiore (sabbia che scende)
    glBegin(GL_TRIANGLES);
    yellowSandTransparent();
    glVertex2f(0.0f, 0.5f - progress);          // Angolo superiore sinistro
    yellowSandOpaque();
    glVertex2f(0.2f, 0.5f);           // Angolo superiore destro
    glVertex2f(0.0f, 0.0f); // Punto variabile (altezza sabbia)
    glEnd();

    // Disegna il riempimento inferiore (sabbia accumulata)
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.2f, -0.5f);           // Angolo inferiore sinistro
    glVertex2f(0.2f, -0.5f);            // Angolo inferiore destro
    yellowSandTransparent();
    glVertex2f(0.0f, -0.5f + progress); // Punto variabile (altezza sabbia)
    glEnd();

    glDisable(GL_BLEND);

    // Richiedi il ridisegno per l'animazione continua
    update();
}


void Mappa::initializeGL() {
    initializeOpenGLFunctions();
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

QString Mappa::calcolaLocatoreMouse(QMouseEvent *event) {
    QPointF pos = event->position();
    float mouseX = (2.0f * pos.x() / this->width()) - 1.0f;
    float mouseY = 1.0f - (2.0f * pos.y() / this->height());

    int xprimo, yprimo, xultimo, yultimo, xdiviso, ydiviso;
    Coordinate::toRowCol(primoLocatore, xprimo, yprimo);
    Coordinate::toRowCol(ultimoLocatore, xultimo, yultimo);

    xdiviso = xultimo - xprimo + 1;
    ydiviso = yprimo - yultimo + 1;

    if(xdiviso == 0)
        xdiviso = 1;

    if(ydiviso == 0)
        ydiviso = 1;

    float col = (mouseX + 1.0f) / 2 * xdiviso;
    float row = (mouseY + 1.0f) / 2 * ydiviso;

    //qDebug() << xprimo << yprimo << xultimo << yultimo << col << row << cellWidth << cellHeight << primoLocatore << ultimoLocatore << mouseX << mouseY << Coordinate::fromRowCol(xprimo + col, yultimo + row);

    return Coordinate::fromRowCol(xprimo + col, yultimo + row);
}

void Mappa::mousePressEvent(QMouseEvent *event) {
    QString loc = calcolaLocatoreMouse(event);

    if(Coordinate::validaLocatore(loc))
        emit mouseLocatore(loc);
}

void Mappa::mouseDoubleClickEvent(QMouseEvent *event) {
    QString loc = calcolaLocatoreMouse(event);

    if(Coordinate::validaLocatore(loc))
        emit mouseLocatoreDPPCLK(loc);
}

void Mappa::adattaMappaLinee() {
    int x1, y1, x2, y2;
    x1 = colonnePerRiga;
    y1 = righePerColonna;
    x2 = 0;
    y2 = 0;

    for(int i = 0; i < linee->count(); i++) {
        int xora, yora;
        Coordinate::toRowCol(linee->at(i).locatore_da, xora, yora);
        if(xora < x1)
            x1 = xora;
        if(yora < y1)
            y1 = yora;
        if(xora > x2)
            x2 = xora;
        if(yora > y2)
            y2 = yora;
        Coordinate::toRowCol(linee->at(i).locatore_a, xora, yora);
        if(xora < x1)
            x1 = xora;
        if(yora < y1)
            y1 = yora;
        if(xora > x2)
            x2 = xora;
        if(yora > y2)
            y2 = yora;
    }

    QString loc1 = Coordinate::fromRowCol(x1, y1);
    QString loc2 = Coordinate::fromRowCol(x2, y2);

    int offset = 50;

    setMatrice(Coordinate::calcolaCoordinate(loc1, -offset, -offset), Coordinate::calcolaCoordinate(loc2, offset, offset));
}

QColor Mappa::generateHierarchicalColor(const QColor &nationalColor, int regionCode, int provinceCode, int municipalityCode, float intensity) {
    // Converti il colore in HSL per manipolazione
    float h, s, l, a;
    nationalColor.getHslF(&h, &s, &l, &a);

    // Parametri base per le variazioni
    float baseRegionHueShift = 0.1f;
    float baseProvinceHueShift = 0.03f;
    float baseMunicipalityLightShift = 0.005f;

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

QPair<float, float> Mappa::bezier(float t, const QVector<QPair<float, float>>& controlPoints) {
    int n = controlPoints.size() - 1; // Grado della curva
    float x = 0.0f;
    float y = 0.0f;

    for (int i = 0; i <= n; ++i) {
        float binomial = tgamma(n + 1) / (tgamma(i + 1) * tgamma(n - i + 1)); // Coefficiente binomiale
        float coefficient = binomial * std::pow(1 - t, n - i) * std::pow(t, i);
        x += coefficient * controlPoints[i].first;
        y += coefficient * controlPoints[i].second;
    }

    return qMakePair(x, y);
}

void Mappa::drawPin(float &x, float &y) {
    // Abilita il blending per la trasparenza
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float triangleSize = 0.05f; // Dimensione del triangolo
    float borderWidth = 0.02f; // Larghezza del bordo

    // Disegna il bordo esterno del primo triangolo
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f); // Colore del bordo (nero opaco)
    glLineWidth(borderWidth * 100);    // Spessore del bordo

    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);                           // Punta inferiore
    glVertex2f(x - triangleSize / 4, y + triangleSize); // Vertice sinistro
    glVertex2f(x, y + triangleSize / 2);        // Vertice destro
    glVertex2f(x + triangleSize / 4, y + triangleSize); // Vertice destro
    glEnd();

    // Disegna il primo triangolo (interno)
    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f); // Rosso opaco
    glVertex2f(x, y);              // Punta inferiore

    glColor4f(1.0f, 0.0f, 0.0f, 0.5f); // Rosso trasparente
    glVertex2f(x, y + triangleSize / 2); // Vertice sinistro

    glVertex2f(x + triangleSize / 4, y + triangleSize); // Vertice destro
    glEnd();

    // Disegna il secondo triangolo (interno)
    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f); // Rosso opaco
    glVertex2f(x, y);              // Punta inferiore

    glColor4f(1.0f, 0.0f, 0.0f, 0.5f); // Rosso trasparente
    glVertex2f(x - triangleSize / 4, y + triangleSize); // Vertice sinistro

    glVertex2f(x, y + triangleSize / 2); // Vertice destro
    glEnd();

    // Disabilita il blending
    glDisable(GL_BLEND);
}

void Mappa::drawLine(float &x1f, float &y1f, float &x2f, float &y2f) {
    // Abilita il blending per la trasparenza
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Imposta il colore (rosso trasparente al 50%)
    glColor4f(200 / 255.0f, 0 / 255.0f, 0 / 255.0f, 0.25f);

    // Imposta lo spessore della linea
    glLineWidth(2.0f);

    // Calcola il punto di controllo centrale
    float controlX, controlY;
    if (x2f > x1f) {
        // Pancia in alto a sinistra
        controlX = (x1f + x2f) / 2 - std::abs(x2f - x1f) / 4; // Sposta verso sinistra
        controlY = (y1f + y2f) / 2 + std::abs(y2f - y1f) / 2; // Sposta verso l'alto
    } else {
        // Pancia in alto a destra
        controlX = (x1f + x2f) / 2 + std::abs(x2f - x1f) / 4; // Sposta verso destra
        controlY = (y1f + y2f) / 2 + std::abs(y2f - y1f) / 2; // Sposta verso l'alto
    }


    // Definisci i punti di controllo per la curva di Bézier
    QVector<QPair<float, float>> controlPoints = {
        {x1f, y1f},               // Punto iniziale
        {controlX, controlY},     // Punto di controllo centrale
        {x2f, y2f}                // Punto finale
    };

    // Disegna la curva come una serie di linee
    glBegin(GL_LINE_STRIP);
    for (float t = 0.0f; t <= 1.0f; t += 0.01f) {
        // Calcola il punto sulla curva di Bézier
        QPair<float, float> point = bezier(t, controlPoints);
        glVertex2f(point.first, point.second);
    }
    glEnd();

    // Disabilita il blending
    glDisable(GL_BLEND);

}

void Mappa::paintGL() {
    glClearColor(102 / 255.0f, 208 / 255.0f, 230 / 255.0f, 1.0f); // Blu oceano profondo pastello
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (m_matrice == nullptr || m_matrice->isEmpty()) {
        clessidra();
        return;
    }

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
                colore = QColor(120, 90, 160); // Viola più scuro
                break;
            case 2:
                colore = QColor(60, 100, 160); // Blu più scuro
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

            drawLine(x1f, y1f, x2f, y2f);

            drawPin(x2f, y2f);

        }
    }
}

void Mappa::reload() {
    setMatrice(primoLocatore, ultimoLocatore);
}
