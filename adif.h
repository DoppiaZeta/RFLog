#ifndef ADIF_H
#define ADIF_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMap>
#include <QList>

class Adif : public QObject
{
    Q_OBJECT
public:
    // Init
    static void parseAdif(const QString& filePath, Adif& adif);

    // Imposta i dati dell'intestazione
    void impostaIntestazione(const QMap<QString, QString>& header);

    // Aggiunge un contatto al log
    void aggiungiContatto(const QMap<QString, QString>& contatto);

    // Restituisce la lista di contatti
    const QList<QMap<QString, QString>>& getContatti() const;

    // Restituisce i dati dell'intestazione
    const QMap<QString, QString>& getIntestazione() const;

private:
    QMap<QString, QString> intestazione; // Dati dell'intestazione
    QList<QMap<QString, QString>> contatti; // Lista di mappe, ciascuna rappresentante un contatto
};

#endif // ADIF_H
