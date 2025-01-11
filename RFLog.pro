TEMPLATE = app
TARGET = RFLog
INCLUDEPATH += .

# Aggiungi i moduli necessari
QT += core gui widgets sql opengl openglwidgets

win32: LIBS += -lopengl32

# Input
HEADERS += databasemanager.h mainwindow.h mappa.h suggestivelineedit.h \
   adif.h \
   coordinate.h \
   locatoripreferiti.h \
   nuovolog.h
FORMS += mainwindow.ui \
   locatoripreferiti.ui \
   mappaconfig.ui \
   nuovolog.ui
SOURCES += databasemanager.cpp \
           adif.cpp \
           coordinate.cpp \
           locatoripreferiti.cpp \
           main.cpp \
           mainwindow.cpp \
           mappa.cpp \
           nuovolog.cpp \
           suggestivelineedit.cpp

# Aggiungi definizioni per OpenGL se necessario
DEFINES += QT_OPENGL_LIB

RESOURCES += \
   icone.qrc

RC_FILE = appicon.rc


