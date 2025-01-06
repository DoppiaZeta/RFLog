TEMPLATE = app
TARGET = RFLog
INCLUDEPATH += .

# Aggiungi i moduli necessari
QT += core gui widgets sql opengl openglwidgets

win32: LIBS += -lopengl32

# Input
HEADERS += databasemanager.h mainwindow.h mappa.h suggestivelineedit.h
FORMS += mainwindow.ui \
   mappaconfig.ui
SOURCES += databasemanager.cpp \
           main.cpp \
           mainwindow.cpp \
           mappa.cpp \
           suggestivelineedit.cpp

# Aggiungi definizioni per OpenGL se necessario
DEFINES += QT_OPENGL_LIB
