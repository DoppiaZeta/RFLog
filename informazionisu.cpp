#include "informazionisu.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPixmap>

InformazioniSu::InformazioniSu(QWidget *parent)
    : QDialog(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    setWindowTitle(tr("Informazioni su RFLog"));

    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *img = new QLabel(this);
    QPixmap pix(":/antenna_log_trasparente.png");
    img->setPixmap(pix);
    img->setScaledContents(true);
    img->setFixedSize(72, 72);

    const QString dettagliVersioni = tr("Compilatore: %1\nQt: %2")
                                         .arg(versioneCompilatore(), QString::fromLatin1(QT_VERSION_STR));

    QLabel *creatoDa = new QLabel(
        tr("RFLog\n"
           "Creato da Tommaso Moro\n"
           "IN3KGW DoppiaZeta KaksiTzeta\n"
           "Anno: 2025 e successivi\n"
           "%1")
            .arg(dettagliVersioni),
        this);
    creatoDa->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(img);
    headerLayout->addStretch();
    headerLayout->addWidget(creatoDa);

    QTabWidget *tabs = new QTabWidget(this);
    auto loadFile = [](const QString &path, const QString &fallbackLabel) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
        return QObject::tr("%1\n\nImpossibile leggere il file: %2").arg(fallbackLabel, path);
    };

    QPlainTextEdit *licenseRfLog = new QPlainTextEdit(this);
    licenseRfLog->setReadOnly(true);
    licenseRfLog->setPlainText(loadFile(QStringLiteral("LICENSE.txt"), tr("Licenza RFLog")));

    QPlainTextEdit *licenseLocatori = new QPlainTextEdit(this);
    licenseLocatori->setReadOnly(true);
    licenseLocatori->setPlainText(loadFile(QStringLiteral("LICENSE_locatoriDB.txt"), tr("Licenza Locatori DB")));

    tabs->addTab(licenseRfLog, tr("Licenza RFLog"));
    tabs->addTab(licenseLocatori, tr("Licenza Locatori DB"));

    layout->addLayout(headerLayout);
    layout->addWidget(tabs);

    resize(640, 480);
}

QString InformazioniSu::versioneCompilatore() const {
#if defined(__clang__)
    return QStringLiteral("Clang %1.%2.%3")
        .arg(__clang_major__)
        .arg(__clang_minor__)
        .arg(__clang_patchlevel__);
#elif defined(__GNUC__)
    return QStringLiteral("GCC %1.%2.%3")
        .arg(__GNUC__)
        .arg(__GNUC_MINOR__)
        .arg(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    return QStringLiteral("MSVC %1").arg(_MSC_VER);
#else
    return tr("Sconosciuto");
#endif
}
