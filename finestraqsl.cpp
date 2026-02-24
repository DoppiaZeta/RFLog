#include "finestraqsl.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include "databasemanager.h"

FinestraQSL::FinestraQSL(DatabaseManager *db, int idLog, QWidget *parent)
    : QDialog(parent)
    , listaNominativi(new QListWidget(this))
{
    setWindowTitle(tr("QSL richiesti"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *titolo = new QLabel(tr("Nominativi con QSL richiesto nel log corrente:"), this);
    layout->addWidget(titolo);

    listaNominativi->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(listaNominativi);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    QSqlQuery *q = db->getQueryBind();
    q->prepare(R"(
select distinct nominativoRx
from qso
where idLog = :idLog
  and qsl = 'S'
  and trim(ifnull(nominativoRx, '')) <> ''
order by nominativoRx
)");
    q->bindValue(":idLog", idLog);

    DBResult *res = db->executeQuery(q);
    for (int i = 0; i < res->getRigheCount(); ++i) {
        listaNominativi->addItem(res->getCella(i, 0));
    }

    if (res->getRigheCount() == 0) {
        listaNominativi->addItem(tr("Nessun contatto con QSL richiesto."));
    }

    delete res;
    delete q;

    resize(440, 520);
}
