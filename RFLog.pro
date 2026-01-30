TEMPLATE = app
TARGET = RFLog
INCLUDEPATH += .

# per compilare sta roba:
# sudo apt-get install build-essential qt6-base-dev qt6-base-dev-tools libqt6sql6-sqlite libgl1-mesa-dev libomp-dev

# Aggiungi i moduli necessari
QT += core gui widgets sql opengl openglwidgets

win32: LIBS += -lopengl32
win32:LIBS += -lodbc32

# Input
HEADERS += databasemanager.h mainwindow.h mappa.h suggestivelineedit.h \
   adif.h \
   coordinate.h \
   linee.h \
   locatoripreferiti.h \
   mappasrpc.h \
   miaradio.h \
   nuovolog.h \
   qso.h
FORMS += mainwindow.ui \
   locatoripreferiti.ui \
   mappaconfig.ui \
   mappasrpc.ui \
   miaradio.ui \
   nuovolog.ui \
   tx.ui
SOURCES += databasemanager.cpp \
           adif.cpp \
           coordinate.cpp \
           coordinate_cq.cpp \
           coordinate_itu.cpp \
           linee.cpp \
           locatoripreferiti.cpp \
           main.cpp \
           mainwindow.cpp \
           mappa.cpp \
           mappasrpc.cpp \
           miaradio.cpp \
           nuovolog.cpp \
           qso.cpp \
           suggestivelineedit.cpp

# Aggiungi definizioni per OpenGL se necessario
DEFINES += QT_OPENGL_LIB

RESOURCES += \
   icone.qrc

RC_FILE = appicon.rc

DISTFILES += \
   LICENSE \
   LICENSE_locatoriDB.txt \
   appicon.rc


# Flag globali (opzionale)
QMAKE_CXXFLAGS += -Wall

# Flag comuni a tutte le configurazioni
QMAKE_CXXFLAGS += -Wall

# Flag specifici per la release
QMAKE_CXXFLAGS_RELEASE = -O2 -DNDEBUG -Wall
QMAKE_LFLAGS_RELEASE += -flto -Wl,--gc-sections

# Flag specifici per il debug
QMAKE_CXXFLAGS_DEBUG = -g -DDEBUG -Wall

# Abilita il linking con LTO per minimizzare il codice finale
QMAKE_LFLAGS += -flto -Wl,--gc-sections

# OpenMP
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp
win32:QMAKE_LFLAGS += -fopenmp -static-libgcc -static-libstdc++ -static
msvc:QMAKE_CXXFLAGS += /openmp
msvc:QMAKE_LFLAGS += /openmp

