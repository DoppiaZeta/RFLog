#ifndef NUOVOLOG_H
#define NUOVOLOG_H

#include <QDialog>
#include <QWidget>
#include <QDateTime>
#include <QMouseEvent>

#include "databasemanager.h"

#include "ui_nuovolog.h"

class NuovoLog : public QDialog
{
    Q_OBJECT
public:
    NuovoLog(DatabaseManager *db, QWidget *parent);

    int getLogSelezionato() const;

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void creaNuovoLog();
    void caricaLogPassato();

private:
    Ui::NuovoLog *ui;
    DatabaseManager *RFLog;
    int numeroLogDB;
};

#endif // NUOVOLOG_H
