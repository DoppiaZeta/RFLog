#ifndef MIARADIO_H
#define MIARADIO_H

#include <QDialog>
#include "databasemanager.h"

namespace Ui {
class MiaRadio;
}

class MiaRadio : public QDialog
{
    Q_OBJECT

public:
    explicit MiaRadio(DatabaseManager *db, QWidget *parent);
    ~MiaRadio();

private slots:
    void aggiungiRadio();
    void eliminaRadio();

private:
    void aggiornaDB();

    Ui::MiaRadio *ui;
    DatabaseManager *RFLog;
};

#endif // MIARADIO_H
