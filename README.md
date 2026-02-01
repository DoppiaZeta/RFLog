# RFLog

RFLog is a Qt application for managing amateur radio logs.

## Requirements (Debian/Ubuntu)

Install the required packages:

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

> `xz-utils` is required to decompress `.lzma` database files.

## Build

From this directory:

```bash
qmake6 RFLog.pro
make -j"$(nproc)"
```

The `RFLog` executable will be generated in the current directory.

## Required Databases

Before starting the application, make sure the databases are present **in the same directory as the executable**:

1. **nominativi.db.lzma**
   - Decompress the file in the same directory as the executable.
   - Example:
     ```bash
     unxz -k nominativi.db.lzma
     ```

2. **locatori_fine.db.lzma**
   - Download the file manually from GitHub.
   - Decompress the file and place the resulting database in the same directory as the executable.
   - Example:
     ```bash
     unxz -k locatori_fine.db.lzma
     ```

## Run

```bash
./RFLog
```
