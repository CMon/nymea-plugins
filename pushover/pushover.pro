include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginpushover)

QT+= network

SOURCES += \
    integrationpluginpushover.cpp

HEADERS += \
    integrationpluginpushover.h
