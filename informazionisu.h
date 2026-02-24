#ifndef INFORMAZIONISU_H
#define INFORMAZIONISU_H

#include <QDialog>
#include <QString>

class InformazioniSu : public QDialog {
    Q_OBJECT

public:
    explicit InformazioniSu(QWidget *parent = nullptr);

private:
    QString versioneCompilatore() const;
};

#endif // INFORMAZIONISU_H
