# RFLog

RFLog è un’applicazione desktop basata su Qt per la gestione dei log radioamatoriali, con particolare attenzione alla **visualizzazione geografica offline**.

Il software è progettato per funzionare completamente senza connessione a Internet, risultando adatto a utilizzi portabili, postazioni remote e attività sul campo. Tutti i dati — inclusi nominativi, locator e mappe — sono gestiti localmente.

RFLog nasce con l’obiettivo di unire la gestione dei QSO all’analisi geografica, in un’applicazione desktop leggera e autonoma, senza dipendenze da servizi online.


## Requisiti (Debian/Ubuntu)

Installa i pacchetti necessari:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  qt6-base-dev \
  qt6-base-dev-tools \
  libqt6sql6-sqlite \
  libomp-dev \
  xz-utils
```

> `xz-utils` serve per decomprimere i database `.lzma`.

## Compilazione

Da questa cartella:

```bash
qmake6 RFLog.pro
make -j"$(nproc)"
```

L’eseguibile `RFLog` verrà generato nella directory corrente.

## Database richiesti

Prima di avviare l’applicazione assicurati che i database siano presenti **nella stessa cartella dell’eseguibile**:

1. **nominativi.db.lzma**
   - Decomprimi il file nella stessa cartella dell’eseguibile.
   - Esempio:
     ```bash
     unxz -k nominativi.db.lzma
     ```

2. **locatori_fine.db.lzma**
   - Scarica manualmente il file da GitHub.
   - Decomprimi il file e posiziona il database ottenuto nella stessa cartella dell’eseguibile.
   - Esempio:
     ```bash
     unxz -k locatori_fine.db.lzma
     ```

## Avvio

```bash
./RFLog
```
