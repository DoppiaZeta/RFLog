TEMPLATE = app
TARGET = RFLog
INCLUDEPATH += .

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
   miaradio.h \
   nuovolog.h \
   qso.h
FORMS += mainwindow.ui \
   locatoripreferiti.ui \
   mappaconfig.ui \
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


