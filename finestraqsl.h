#ifndef FINESTRAQSL_H
#define FINESTRAQSL_H

#include <QDialog>
#include <QList>

class QListWidget;
class Qso;

class FinestraQSL : public QDialog
{
    Q_OBJECT

public:
    explicit FinestraQSL(const QList<Qso*> &qsoList, QWidget *parent = nullptr);

private:
    QListWidget *listaNominativi;
};

#endif // FINESTRAQSL_H
