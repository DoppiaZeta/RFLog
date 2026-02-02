# RFLog

RFLog is a Qt-based desktop application for managing amateur radio logs with a strong focus on **offline geographic visualization**.

The software is designed to work entirely without an Internet connection, making it suitable for portable operations, remote locations, and field use. All data — including callsigns, locators, and maps — is handled locally.

RFLog aims to combine radio log management with map-based analysis in a lightweight desktop application, without relying on online services or external APIs.


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
