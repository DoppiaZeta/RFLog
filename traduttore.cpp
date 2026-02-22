#include "traduttore.h"

#include <QFile>
#include <QXmlStreamReader>

Traduttore::Traduttore() = default;

bool Traduttore::loadFromTs(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader xml(&file);
    QString contextName;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) {
            continue;
        }

        if (xml.name() == QLatin1String("context")) {
            contextName.clear();
            continue;
        }

        if (xml.name() == QLatin1String("name")) {
            contextName = xml.readElementText();
            continue;
        }

        if (xml.name() == QLatin1String("message")) {
            QString sourceText;
            QString translationText;
            bool unfinished = false;
            while (!(xml.isEndElement() && xml.name() == QLatin1String("message")) && !xml.atEnd()) {
                xml.readNext();
                if (!xml.isStartElement()) {
                    continue;
                }

                if (xml.name() == QLatin1String("source")) {
                    sourceText = xml.readElementText();
                    continue;
                }

                if (xml.name() == QLatin1String("translation")) {
                    const auto typeAttr = xml.attributes().value(QLatin1String("type"));
                    unfinished = (typeAttr == QLatin1String("unfinished"));
                    translationText = xml.readElementText();
                    continue;
                }
            }

            if (!contextName.isEmpty() && !sourceText.isEmpty() && !translationText.isEmpty() && !unfinished) {
                const QString key = contextName + kSeparator + sourceText;
                translations.insert(key, translationText);
            }
        }
    }

    return !xml.hasError() && !translations.isEmpty();
}

QString Traduttore::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    Q_UNUSED(disambiguation);
    Q_UNUSED(n);

    const QString key = QString::fromUtf8(context) + kSeparator + QString::fromUtf8(sourceText);
    return translations.value(key);
}
