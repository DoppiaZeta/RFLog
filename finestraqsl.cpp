#include <QClipboard>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QListWidget>
#include <QShortcut>
#include <QStringList>
#include <QVBoxLayout>

#include "finestraqsl.h"
#include "qso.h"

FinestraQSL::FinestraQSL(const QList<Qso*> &qsoList, QWidget *parent)
    : QDialog(parent)
    , listaNominativi(new QListWidget(this))
{
    setWindowTitle(tr("QSL richiesti"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *titolo = new QLabel(tr("Nominativi con QSL richiesto nel log corrente:"), this);
    layout->addWidget(titolo);

    listaNominativi->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(listaNominativi);

    QShortcut *copyShortcut = new QShortcut(QKeySequence::Copy, listaNominativi);
    connect(copyShortcut, &QShortcut::activated, this, [this]() {
        QStringList selectedTexts;
        const QList<QListWidgetItem*> selectedItems = listaNominativi->selectedItems();
        for (QListWidgetItem *item : selectedItems) {
            if (item) {
                selectedTexts << item->text();
            }
        }

        if (!selectedTexts.isEmpty()) {
            QGuiApplication::clipboard()->setText(selectedTexts.join("\n"));
        }
    });

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    QStringList nominativi;
    for (Qso *qso : qsoList) {
        if (!qso || !qso->qsl) {
            continue;
        }

        const QString nominativo = qso->nominativoRx.trimmed();
        if (!nominativo.isEmpty()) {
            nominativi << nominativo;
        }
    }

    nominativi.removeDuplicates();
    nominativi.sort();

    for (const QString &nominativo : nominativi) {
        listaNominativi->addItem(nominativo);
    }

    if (nominativi.isEmpty()) {
        listaNominativi->addItem(tr("Nessun contatto con QSL richiesto."));
    }

    resize(440, 520);
}
