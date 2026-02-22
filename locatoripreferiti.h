#ifndef LOCATORIPREFERITI_H
#define LOCATORIPREFERITI_H

#include <QDialog>
#include <QDebug>
#include "databasemanager.h"

#include "ui_locatoripreferiti.h"

class LocatoriPreferiti : public QDialog
{
    Q_OBJECT
public:
    explicit LocatoriPreferiti(DatabaseManager *db, QWidget *parent = nullptr);

signals:

private slots:
    void cercaLatLon();
    void addLocatoreOK();
    void delLocatoreOK();
    void onItemClicked(QTableWidgetItem *item);

private:
    void addLocatore(const QString &loc, const QString &txt);
    void aggiornaDB();

    Ui::LocatoriPreferiti *ui;
    DatabaseManager *RFLog;
};

#endif // LOCATORIPREFERITI_H
