#ifndef FINESTRAQSL_H
#define FINESTRAQSL_H

#include <QDialog>

class QListWidget;
class DatabaseManager;

class FinestraQSL : public QDialog
{
    Q_OBJECT

public:
    explicit FinestraQSL(DatabaseManager *db, int idLog, QWidget *parent = nullptr);

private:
    QListWidget *listaNominativi;
};

#endif // FINESTRAQSL_H
