#ifndef MAPPASRPC_H
#define MAPPASRPC_H

#include <QDialog>
#include "databasemanager.h"
#include "mappa.h"

#include "ui_mappasrpc.h"

class MappaSRPC : public QDialog
{
    Q_OBJECT

public:
    explicit MappaSRPC(DatabaseManager *database, const QString & statoDefault, QWidget *parent = nullptr);
    ~MappaSRPC();

    Mappa::tipoMappa getTipo() const;
    QString getStato() const;
    bool getApplica() const;

private slots:
    void applicaOK();

private:
    DatabaseManager *db;
    bool applica;

    Ui::MappaSRPC *ui;
};

#endif // MAPPASRPC_H
