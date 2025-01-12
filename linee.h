#ifndef LINEE_H
#define LINEE_H

#include <QString>

class Linee {
public:
    Linee(const QString &da, const QString &a);
    bool operator==(const Linee& other) const;

    QString locatore_da;
    QString locatore_a;
};

#endif // LINEE_H
