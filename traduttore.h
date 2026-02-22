#ifndef TRADUTTORE_H
#define TRADUTTORE_H

#include <QHash>
#include <QTranslator>

class Traduttore final : public QTranslator
{
public:
    Traduttore();
    ~Traduttore() override = default;
    bool loadFromTs(const QString &filePath);
    QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const override;

private:
    static constexpr QChar kSeparator = QChar(0x1f);
    QHash<QString, QString> translations;
};

#endif // TRADUTTORE_H
