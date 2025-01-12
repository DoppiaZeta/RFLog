#include "linee.h"

bool Linee::operator==(const Linee& other) const {
    return locatore_da == other.locatore_da && locatore_a == other.locatore_a;
}

Linee::Linee(const QString &da, const QString &a) :
    locatore_da(da.trimmed().toUpper()), locatore_a(a.trimmed().toUpper()) {
}
