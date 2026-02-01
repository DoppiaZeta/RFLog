#ifndef EDI_H
#define EDI_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

struct EdiRecord {
    QString date;
    QString time;
    QString call;
    QString serialSent;
    QString rstSent;
    QString serialReceived;
    QString rstReceived;
    QString locator;
    QString distance;
};

class Edi : public QObject
{
    Q_OBJECT
public:
    static bool parseEdi(const QString& filePath, Edi& edi);

    void impostaIntestazione(const QMap<QString, QString>& header);
    void aggiungiContatto(const EdiRecord& record);

    const QMap<QString, QString>& getIntestazione() const;
    const QList<EdiRecord>& getContatti() const;

private:
    QMap<QString, QString> intestazione;
    QList<EdiRecord> contatti;
};

#endif // EDI_H
