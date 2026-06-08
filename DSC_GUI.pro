QT += core gui widgets

TARGET = DSC_GUI
TEMPLATE = app

CONFIG += c++17

HEADERS += \
    mainwindow.h \
    dsc_engine.h \
    src/dsc_codec.h \
    src/dsc_types.h \
    src/dsc_utils.h \
    src/cmd_parse.h \
    src/dpx.h \
    src/fifo.h \
    src/logging.h \
    src/multiplex.h \
    src/psnr.h \
    src/utl.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    dsc_engine.cpp \
    src/codec_main.c \
    src/dsc_codec.c \
    src/dsc_utils.c \
    src/cmd_parse.c \
    src/dpx.c \
    src/fifo.c \
    src/logging.c \
    src/multiplex.c \
    src/psnr.c \
    src/utl.c

INCLUDEPATH += src

DEFINES += _FILE_OFFSET_BITS=64

win32 {
    DEFINES += WIN32
    LIBS += -luser32 -lgdi32
}
