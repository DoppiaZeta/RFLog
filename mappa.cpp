#include "mappa.h"
#include <QPainterPath>
#include <QSet>
#include <QtMath>
#include <QtConcurrent/QtConcurrent>
Mappa::Mappa(DatabaseManager *dbm, QWidget *mappaConfig, QWidget *parent)
    : QWidget(parent), m_matrice(nullptr), linee(new QVector<Linee>()), loaderThread(nullptr), loaderRunning(false), hasPendingRequest(false), lastRequestId(0) {
    db = dbm;
    mappaConfigWidget = mappaConfig;
    primoLocatore = QString();
    ultimoLocatore = QString();
    timer.start();
    progress = 0;
    tipomappa = tipoMappa::geografica;
    stato = "Italy";

    setMinimumSize(640, 480);
}

Mappa::~Mappa() {
    stopLoaderThread();
    liberaMatrice(m_matrice);
    m_matrice = nullptr;
    delete linee;
}

void Mappa::setMatrice(const QString& locatore_da, const QString& locatore_a) {
    liberaMatrice(m_matrice);
    m_matrice = nullptr;

    timer.start();
    update();

    startLoaderThread();

    MatriceLoadRequest request;
    request.locatoreDa = locatore_da;
    request.locatoreA = locatore_a;
    request.stato = stato;
    request.tipo = tipomappa;
    request.linee = *linee;

    {
        QMutexLocker lock(&loaderMutex);
        request.requestId = ++lastRequestId;
        pendingRequest = request;
        hasPendingRequest = true;
    }

    loaderCv.wakeOne();
}

void Mappa::startLoaderThread() {
    QMutexLocker lock(&loaderMutex);
    if (loaderRunning) {
        return;
    }

    loaderRunning = true;
    loaderThread = QThread::create([this]() {
        loaderLoop();
    });
    loaderThread->start();
}

void Mappa::stopLoaderThread() {
    QThread *threadToStop = nullptr;

    {
        QMutexLocker lock(&loaderMutex);
        if (!loaderRunning) {
            return;
        }

        loaderRunning = false;
        hasPendingRequest = false;
        threadToStop = loaderThread;
        loaderThread = nullptr;
    }

    loaderCv.wakeOne();

    if (threadToStop) {
        threadToStop->wait();
        delete threadToStop;
    }
}

void Mappa::liberaMatrice(QVector<QVector<Coordinate *>> *matrice) {
    if (!matrice) {
        return;
    }

    for (auto &row : *matrice) {
        for (auto &coord : row) {
            delete coord;
        }
    }

    delete matrice;
}

void Mappa::loaderLoop() {
    while (true) {
        MatriceLoadRequest request;

        {
            QMutexLocker lock(&loaderMutex);
            while (loaderRunning && !hasPendingRequest) {
                loaderCv.wait(&loaderMutex);
            }

            if (!loaderRunning) {
                return;
            }

            request = pendingRequest;
            hasPendingRequest = false;
        }

        QString primo;
        QString ultimo;
        QVector<QVector<Coordinate*>> *nuovaMatrice = caricaMatriceDaDb(request, primo, ultimo);

        MatriceLoadResult result{nuovaMatrice, primo, ultimo, request.locatoreDa, request.locatoreA, request.requestId};

        QMetaObject::invokeMethod(this, [this, result]() mutable {
            {
                QMutexLocker lock(&loaderMutex);
                if (result.requestId != lastRequestId) {
                    liberaMatrice(result.matrice);
                    return;
                }
            }

            liberaMatrice(m_matrice);
            m_matrice = result.matrice;
            primoLocatore = result.primoLocatore;
            ultimoLocatore = result.ultimoLocatore;

            update();
            emit matriceCaricata();
            emit matriceDaA(result.locatoreDa, result.locatoreA);
        }, Qt::QueuedConnection);
    }
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

void Mappa::delAllLinee(bool refresh) {
    linee->clear();
    if(refresh)
        update();
}

void Mappa::setTipoMappa(tipoMappa t, bool ricarica) {
    tipomappa = t;
    if(ricarica)
        reload();
}

QVector<QVector<Coordinate*>>* Mappa::caricaMatriceDaDb(const MatriceLoadRequest &request, QString &primo, QString &ultimo) {
    int rowDa, colDa, rowA, colA;
    Coordinate::toRowCol(request.locatoreDa, rowDa, colDa);
    Coordinate::toRowCol(request.locatoreA, rowA, colA);

    int rTop = qMax(rowDa, rowA);
    int rBottom = qMin(rowDa, rowA);
    int cLeft = qMin(colDa, colA);
    int cRight = qMax(colDa, colA);

    // Determina il numero totale di colonne e righe
    int totalCols = cRight - cLeft + 1;
    int totalRows = rTop - rBottom + 1;

    // Calcola il passo di riduzione per mantenere il numero di celle entro un limite.
    const int maxCells = 250000;
    const int maxAxis = 2000;
    const double totalCells = static_cast<double>(totalCols) * static_cast<double>(totalRows);
    const double scale = (totalCells > maxCells) ? qSqrt(totalCells / maxCells) : 1.0;

    auto calculateStep = [scale](int total) -> int {
        int step = qMax(1, static_cast<int>(qCeil(scale)));
        if (total / step > maxAxis) {
            step = qMax(step, static_cast<int>(qCeil(static_cast<double>(total) / maxAxis)));
        }
        return step;
    };

    int colStep = calculateStep(totalCols);
    int rowStep = calculateStep(totalRows);

    //qDebug() << "Moltiplicatori" << colStep << rowStep;

    primo = Coordinate::fromRowCol(rBottom, cRight);
    ultimo = Coordinate::fromRowCol(rTop, cLeft);

    const bool includeConfini = (
        request.tipo == tipoMappa::stati ||
        request.tipo == tipoMappa::regioni ||
        request.tipo == tipoMappa::provincie ||
        request.tipo == tipoMappa::comuni
        );

    QSet<QString> gialloStati;
    QSet<QString> gialloRegioni;
    QSet<QString> gialloProvince;
    QSet<QString> gialloComuni;

    auto keyRegione = [](const QString &statoVal, const QString &regioneVal) {
        return statoVal + QLatin1Char('|') + regioneVal;
    };
    auto keyProvincia = [](const QString &statoVal, const QString &regioneVal, const QString &provinciaVal) {
        return statoVal + QLatin1Char('|') + regioneVal + QLatin1Char('|') + provinciaVal;
    };
    auto keyComune = [](const QString &statoVal, const QString &regioneVal, const QString &provinciaVal, const QString &comuneVal) {
        return statoVal + QLatin1Char('|') + regioneVal + QLatin1Char('|') + provinciaVal + QLatin1Char('|') + comuneVal;
    };

    if (includeConfini) {
        for(int i = 0; i < request.linee.count(); i++) {
            QSqlQuery *q = db->getQueryBind();
            q->prepare(R"(
    SELECT locatore, stato, regione, provincia, comune
    FROM locatori
    WHERE locatore = :locatore
)");
            q->bindValue(":locatore", request.linee.at(i).locatore_a);
            DBResult *res = db->executeQuery(q);

            const QString statoVal = res->getCella("stato");
            const QString regioneVal = res->getCella("regione");
            const QString provinciaVal = res->getCella("provincia");
            const QString comuneVal = res->getCella("comune");

            if (!statoVal.isEmpty()) {
                gialloStati.insert(statoVal);
            }
            if (!regioneVal.isEmpty()) {
                gialloRegioni.insert(keyRegione(statoVal, regioneVal));
            }
            if (!provinciaVal.isEmpty()) {
                gialloProvince.insert(keyProvincia(statoVal, regioneVal, provinciaVal));
            }
            if (!comuneVal.isEmpty()) {
                gialloComuni.insert(keyComune(statoVal, regioneVal, provinciaVal, comuneVal));
            }

            delete q;
            delete res;
        }
    }

    auto matrix = new QVector<QVector<Coordinate*>>((cRight - cLeft) / colStep + 1);

    QVector<int> colonne;
    colonne.reserve(matrix->size());
    for (int c = cRight; c >= cLeft; c -= colStep) {
        colonne.push_back(c);
    }

    QMutex matrixMutex;

    // Elaborazione multi-thread con QtConcurrent (senza OpenMP).
    QtConcurrent::blockingMap(colonne, [&](const int c) {
        const int columnIndex = (cRight - c) / colStep;
        QVector<Coordinate*> columnCoordinates;
        columnCoordinates.reserve(totalRows / rowStep + 1);

        QStringList tempLocatoriValues;

        for (int r = rBottom; r <= rTop; r += rowStep) {
            const QString newLoc = Coordinate::fromRowCol(r, c);
            Coordinate* coord = new Coordinate(includeConfini);
            coord->setLocatore(newLoc);
            columnCoordinates.push_back(coord);
            tempLocatoriValues.append(QString("('%1')").arg(newLoc));
        }

        if (!tempLocatoriValues.isEmpty()) {
            const QString values = tempLocatoriValues.join(", ");
            const QString tempTableName = QString("temp_locatori_%1_%2")
                                              .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                                              .arg(columnIndex);

            const QString createTempTableQuery = QString("CREATE TEMP TABLE IF NOT EXISTS %1 (locatore TEXT);").arg(tempTableName);
            db->executeQueryNoRes(createTempTableQuery);

            const QString insertTempTableQuery = QString("INSERT INTO %1 (locatore) VALUES %2;").arg(tempTableName, values);
            db->executeQueryNoRes(insertTempTableQuery);

            QString selectQuery;
            if (includeConfini) {
                selectQuery = QString(R"(
    SELECT l.locatore, l.colore_stato, l.colore_regione, l.colore_provincia, l.colore_comune, l.altezza, l.confine_stato, l.confine_regione, l.confine_provincia, l.confine_comune, l.stato, l.regione, l.provincia, l.comune
    FROM view_confini l
    JOIN %1 t
    ON l.locatore = t.locatore
    WHERE l.stato = :stato
)").arg(tempTableName);
            } else {
                selectQuery = QString(R"(
    SELECT l.locatore, l.colore_stato, l.colore_regione, l.colore_provincia, l.colore_comune, l.altezza
    FROM locatori l
    JOIN %1 t
    ON l.locatore = t.locatore
)").arg(tempTableName);
            }

            DBResult* resQuery = nullptr;
            if (includeConfini) {
                QSqlQuery *q = db->getQueryBind();
                q->prepare(selectQuery);
                q->bindValue(":stato", request.stato);
                resQuery = db->executeQuery(q);
                delete q;
            } else {
                resQuery = db->executeQuery(selectQuery);
            }

            const QString dropTempTableQuery = QString("DROP TABLE IF EXISTS %1;").arg(tempTableName);
            db->executeQueryNoRes(dropTempTableQuery);

            QHash<QString, QVector<QString>> queryMap;
            for (const auto& row : resQuery->tabella) {
                queryMap.insert(row[0], row);
            }

            enum ColumnIndex {
                Locatore = 0,
                ColoreStato,
                ColoreRegione,
                ColoreProvincia,
                ColoreComune,
                Altezza,
                ConfineStato,
                ConfineRegione,
                ConfineProvincia,
                ConfineComune,
                Stato,
                Regione,
                Provincia,
                Comune
            };

            for (auto& coord : columnCoordinates) {
                if (!coord) {
                    continue;
                }

                const QString locatoreStr = coord->getLocatore();
                if (!queryMap.contains(locatoreStr)) {
                    delete coord;
                    coord = nullptr;
                    continue;
                }

                const QVector<QString>& data = queryMap[locatoreStr];
                coord->setColoreStato(data[ColoreStato].toInt());
                coord->setColoreRegione(data[ColoreRegione].toInt());
                coord->setColoreProvincia(data[ColoreProvincia].toInt());
                coord->setColoreComune(data[ColoreComune].toInt());
                coord->setAltezza(data[Altezza].toFloat());
                if (includeConfini) {
                    coord->setConfineStato(data[ConfineStato].toInt());
                    coord->setConfineRegione(data[ConfineRegione].toInt());
                    coord->setConfineProvincia(data[ConfineProvincia].toInt());
                    coord->setConfineComune(data[ConfineComune].toInt());
                    coord->setStato(data[Stato]);
                    coord->setRegione(data[Regione]);
                    coord->setProvincia(data[Provincia]);
                    coord->setComune(data[Comune]);

                    const QString statoVal = coord->getStato();
                    const QString regioneVal = coord->getRegione();
                    const QString provinciaVal = coord->getProvincia();
                    const QString comuneVal = coord->getComune();

                    coord->setGialloStato(gialloStati.contains(statoVal));
                    coord->setGialloRegione(gialloRegioni.contains(keyRegione(statoVal, regioneVal)));
                    coord->setGialloProvincia(gialloProvince.contains(keyProvincia(statoVal, regioneVal, provinciaVal)));
                    coord->setGialloComune(gialloComuni.contains(keyComune(statoVal, regioneVal, provinciaVal, comuneVal)));
                }
            }

            delete resQuery;
        }

        {
            QMutexLocker matrixLock(&matrixMutex);
            matrix->operator[](columnIndex) = columnCoordinates;
        }

        db->cleanUpConnections();
    });

    db->cleanUpConnections();

    return matrix;
}

QColor Mappa::calcolaColoreAltitudine(const float &altitudine) {
    // Definizione dei range di altitudine e colori associati
    const float pianuraMax = 100.0f;  // Fino a 200 metri - Verde
    const float collinaMax = 500.0f;  // Fino a 800 metri - Giallo
    const float montagnaMax = 2000.0f; // Fino a 2000 metri - Marrone
    const float montagnaAltaMax = 8000.0f; // Oltre 4000 metri - Grigio neve

    QColor pianuraColor(28, 100, 28); // Verde più scuro e profondo
    QColor collinaColor(189, 183, 107); // Verde-giallo oliva
    QColor montagnaColor(139, 69, 19);  // Marrone (montagna)
    QColor montagnaAltaColor(240, 248, 255); // Bianco azzurrato (Alice Blue)


    // Se l'altitudine è inferiore alla pianura
    if (altitudine <= pianuraMax) {
        return pianuraColor;
    }
    // Se l'altitudine è tra pianura e collina
    else if (altitudine <= collinaMax) {
        float t = (altitudine - pianuraMax) / (collinaMax - pianuraMax);
        return QColor(
            pianuraColor.red() + t * (collinaColor.red() - pianuraColor.red()),
            pianuraColor.green() + t * (collinaColor.green() - pianuraColor.green()),
            pianuraColor.blue() + t * (collinaColor.blue() - pianuraColor.blue())
            );
    }
    // Se l'altitudine è tra collina e montagna
    else if (altitudine <= montagnaMax) {
        float t = (altitudine - collinaMax) / (montagnaMax - collinaMax);
        return QColor(
            collinaColor.red() + t * (montagnaColor.red() - collinaColor.red()),
            collinaColor.green() + t * (montagnaColor.green() - collinaColor.green()),
            collinaColor.blue() + t * (montagnaColor.blue() - collinaColor.blue())
            );
    }
    // Se l'altitudine è tra montagna e montagna alta
    else if (altitudine <= montagnaAltaMax) {
        float t = (altitudine - montagnaMax) / (montagnaAltaMax - montagnaMax);
        return QColor(
            montagnaColor.red() + t * (montagnaAltaColor.red() - montagnaColor.red()),
            montagnaColor.green() + t * (montagnaAltaColor.green() - montagnaColor.green()),
            montagnaColor.blue() + t * (montagnaAltaColor.blue() - montagnaColor.blue())
            );
    }
    // Se l'altitudine è superiore alla montagna alta
    else {
        return montagnaAltaColor;
    }
}

void Mappa::clessidra(QPainter &painter) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(20, 50, 100));

    float scale = 0.3f;
    QPen outlinePen(QColor(204, 204, 204));
    outlinePen.setWidthF(2.0f);
    painter.setPen(outlinePen);
    painter.setBrush(Qt::NoBrush);

    auto p1 = mapToWidget(-0.2f * scale, 0.5f * scale);
    auto p2 = mapToWidget(0.2f * scale, 0.5f * scale);
    auto p3 = mapToWidget(0.0f * scale, 0.0f * scale);
    auto p4 = mapToWidget(-0.2f * scale, -0.5f * scale);
    auto p5 = mapToWidget(0.2f * scale, -0.5f * scale);

    painter.drawLine(p1, p2);
    painter.drawLine(p1, p3);
    painter.drawLine(p2, p3);
    painter.drawLine(p4, p5);
    painter.drawLine(p4, p3);
    painter.drawLine(p5, p3);

    int elapsed = timer.elapsed();
    float totalTime = 3000.0f;
    float progress = (elapsed % int(totalTime)) / totalTime / 2;

    QColor sandTransparent(255, 204, 51, 77);
    QColor sandOpaque(255, 204, 51, 204);

    QPolygonF topLeft;
    topLeft << mapToWidget(-0.2f * scale, 0.5f * scale)
            << mapToWidget(0.0f * scale, (0.5f - progress) * scale)
            << mapToWidget(0.0f * scale, 0.0f * scale);

    QPolygonF topRight;
    topRight << mapToWidget(0.0f * scale, (0.5f - progress) * scale)
             << mapToWidget(0.2f * scale, 0.5f * scale)
             << mapToWidget(0.0f * scale, 0.0f * scale);

    QPolygonF bottom;
    bottom << mapToWidget(-0.2f * scale, -0.5f * scale)
           << mapToWidget(0.2f * scale, -0.5f * scale)
           << mapToWidget(0.0f * scale, (-0.5f + progress) * scale);

    painter.setPen(Qt::NoPen);
    painter.setBrush(sandOpaque);
    painter.drawPolygon(topLeft);
    painter.drawPolygon(topRight);
    painter.setBrush(sandTransparent);
    painter.drawPolygon(bottom);

    painter.restore();
    update();
}

QPointF Mappa::mapToWidget(float xNorm, float yNorm) const {
    float x = (xNorm + 1.0f) * 0.5f * width();
    float y = (1.0f - (yNorm + 1.0f) * 0.5f) * height();
    return QPointF(x, y);
}

void Mappa::drawSquare(QPainter &painter, const QRect &rect, const QColor &color, bool border) {
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRect(rect);

    if (border) {
        QPen pen(QColor(0, 0, 0, 13));
        pen.setWidthF(1.0f);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect);
    }
    painter.restore();
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
    // Controlla se è stato premuto il pulsante destro del mouse
    if(event->button() == Qt::RightButton) {
        QString loc = calcolaLocatoreMouse(event);
        if(Coordinate::validaLocatore(loc))
            emit mouseLocatoreDPPCLK(loc);
        return; // Ritorna subito per evitare di eseguire altro codice
    }

    // Tratta il clic sinistro normalmente
    if(event->button() == Qt::LeftButton) {
        QString loc = calcolaLocatoreMouse(event);
        if(Coordinate::validaLocatore(loc))
            emit mouseLocatore(loc);
    }
}

void Mappa::mouseDoubleClickEvent(QMouseEvent *event) {
    QString loc = calcolaLocatoreMouse(event);
    if(Coordinate::validaLocatore(loc))
        emit mouseLocatoreDPPCLK(loc);
    return; // Ritorna subito per evitare di eseguire altro codice
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

void Mappa::drawPin(QPainter &painter, float x, float y) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPointF base = mapToWidget(x, y);

    float triangleSize = 0.05f;
    float borderWidth = 2.0f;

    QPointF left = mapToWidget(x - triangleSize / 4, y + triangleSize);
    QPointF right = mapToWidget(x + triangleSize / 4, y + triangleSize);
    QPointF mid = mapToWidget(x, y + triangleSize / 2);

    QPen borderPen(QColor(0, 0, 0, 204));
    borderPen.setWidthF(borderWidth);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    QPolygonF border;
    border << base << left << mid << right;
    painter.drawPolygon(border);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 0, 0, 204));
    QPolygonF tri1;
    tri1 << base << mid << right;
    painter.drawPolygon(tri1);

    painter.setBrush(QColor(255, 0, 0, 128));
    QPolygonF tri2;
    tri2 << base << left << mid;
    painter.drawPolygon(tri2);

    painter.restore();
}

void Mappa::drawLine(QPainter &painter, float x1f, float y1f, float x2f, float y2f) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(200, 0, 0, 64));
    pen.setWidthF(2.0f);
    painter.setPen(pen);

    float controlX;
    float controlY;
    if (x2f > x1f) {
        controlX = (x1f + x2f) / 2 - std::abs(x2f - x1f) / 4;
        controlY = (y1f + y2f) / 2 + std::abs(y2f - y1f) / 2;
    } else {
        controlX = (x1f + x2f) / 2 + std::abs(x2f - x1f) / 4;
        controlY = (y1f + y2f) / 2 + std::abs(y2f - y1f) / 2;
    }

    QPointF p1 = mapToWidget(x1f, y1f);
    QPointF p2 = mapToWidget(controlX, controlY);
    QPointF p3 = mapToWidget(x2f, y2f);

    QPainterPath path(p1);
    path.quadTo(p2, p3);
    painter.drawPath(path);
    painter.restore();
}

void Mappa::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), QColor(108, 210, 231));

    static const QColor grigio(0x80, 0x80, 0x80);
    static const QColor beige(0xe0, 0xe0, 0xe0);
    static const QColor giallo(0xe0, 0xe0, 0x0);

    if (m_matrice == nullptr || m_matrice->isEmpty()) {
        clessidra(painter);
        return;
    }

    int rows = m_matrice->size();
    int cols = m_matrice->at(0).size();

    if (rows == 0 || cols == 0)
        return;

    const double cellWidth = width() / static_cast<double>(cols);
    const double cellHeight = height() / static_cast<double>(rows);

    const bool useGroupBorders = (
        tipomappa == tipoMappa::stati ||
        tipomappa == tipoMappa::regioni ||
        tipomappa == tipoMappa::provincie ||
        tipomappa == tipoMappa::comuni
    );

    QPen borderPen(grigio);
    borderPen.setWidthF(1.0f);

    auto sameGroup = [&](const Coordinate *left, const Coordinate *right) -> bool {
        if (!left || !right)
            return false;

        switch (tipomappa) {
        case tipoMappa::stati:
            return left->getStato() == right->getStato();
        case tipoMappa::regioni:
            return left->getStato() == right->getStato()
                && left->getRegione() == right->getRegione();
        case tipoMappa::provincie:
            return left->getStato() == right->getStato()
                && left->getRegione() == right->getRegione()
                && left->getProvincia() == right->getProvincia();
        case tipoMappa::comuni:
            return left->getStato() == right->getStato()
                && left->getRegione() == right->getRegione()
                && left->getProvincia() == right->getProvincia()
                && left->getComune() == right->getComune();
        default:
            return false;
        }
    };

    // Disegna i quadrati
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const Coordinate *coord = m_matrice->at(row).at(col);

            if (coord == nullptr)
                continue;

            const int left = static_cast<int>(std::floor(col * cellWidth));
            const int right = static_cast<int>(std::floor((col + 1) * cellWidth));
            const int top = static_cast<int>(std::floor(row * cellHeight));
            const int bottom = static_cast<int>(std::floor((row + 1) * cellHeight));
            const QRect cellRect(left, top, right - left, bottom - top);

            QColor colore;
            if(tipomappa == tipoMappa::polica) {
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
            } else if (tipomappa == tipoMappa::geografica) {
                colore = calcolaColoreAltitudine(coord->getAltezza());
            } else if (tipomappa == tipoMappa::stati) {
                if(coord->getGialloStato()) {
                    colore = giallo;
                } else {
                    colore = beige;
                }
            } else if (tipomappa == tipoMappa::regioni) {
                if(coord->getGialloRegione()) {
                    colore = giallo;
                } else {
                    colore = beige;
                }
            } else if (tipomappa == tipoMappa::provincie) {
                if(coord->getGialloProvincia()) {
                    colore = giallo;
                } else {
                    colore = beige;
                }
            } else if (tipomappa == tipoMappa::comuni) {
                if(coord->getGialloComune()) {
                    colore = giallo;
                } else {
                    colore = beige;
                }
            }

            drawSquare(painter, cellRect, colore, cols <= 110);

            if (useGroupBorders) {
                const Coordinate *rightCoord = (col + 1 < cols) ? m_matrice->at(row).at(col + 1) : nullptr;
                const Coordinate *bottomCoord = (row + 1 < rows) ? m_matrice->at(row + 1).at(col) : nullptr;
                const Coordinate *leftCoord = (col > 0) ? m_matrice->at(row).at(col - 1) : nullptr;
                const Coordinate *topCoord = (row > 0) ? m_matrice->at(row - 1).at(col) : nullptr;

                const bool drawLeft = (col == 0) || !sameGroup(coord, leftCoord);
                const bool drawTop = (row == 0) || !sameGroup(coord, topCoord);
                const bool drawRight = (col + 1 == cols) || !sameGroup(coord, rightCoord);
                const bool drawBottom = (row + 1 == rows) || !sameGroup(coord, bottomCoord);

                painter.save();
                painter.setPen(borderPen);
                painter.setBrush(Qt::NoBrush);

                if (drawLeft) {
                    painter.drawLine(cellRect.topLeft(), cellRect.bottomLeft());
                }
                if (drawTop) {
                    painter.drawLine(cellRect.topLeft(), cellRect.topRight());
                }
                if (drawRight) {
                    painter.drawLine(cellRect.topRight(), cellRect.bottomRight());
                }
                if (drawBottom) {
                    painter.drawLine(cellRect.bottomLeft(), cellRect.bottomRight());
                }
                painter.restore();
            }
        }
    }

    if(!primoLocatore.isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing, true);
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

            drawLine(painter, x1f, y1f, x2f, y2f);

            drawPin(painter, x2f, y2f);

        }
    }
}

void Mappa::reload() {
    setMatrice(primoLocatore, ultimoLocatore);
}

bool Mappa::trovaStRePrCo(const Coordinate & dove, const Coordinate & cerca, tipoMappa tipoRicerca) const {
    bool trovato = true;

    if(tipoRicerca == tipoMappa::stati || tipoRicerca == tipoMappa::regioni || tipoRicerca == tipoMappa::provincie || tipoRicerca == tipoMappa::comuni) {
        trovato = trovato && dove.getStato() == cerca.getStato();
        if(tipoRicerca == tipoMappa::regioni || tipoRicerca == tipoMappa::provincie || tipoRicerca == tipoMappa::comuni) {
            trovato = trovato && dove.getRegione() == cerca.getRegione();
            if(tipoRicerca == tipoMappa::provincie || tipoRicerca == tipoMappa::comuni) {
                trovato = trovato && dove.getProvincia() == cerca.getProvincia();
                if(tipoRicerca == tipoMappa::comuni) {
                    trovato = trovato && dove.getComune() == cerca.getComune();
                }
            }
        }
    } else
        return false;

    return trovato;
}

QString Mappa::getStato() const {
    return stato;
}

void Mappa::setStato(const QString & s) {
    stato = s;
}

Mappa::tipoMappa Mappa::getTipo() const {
    return tipomappa;
}
