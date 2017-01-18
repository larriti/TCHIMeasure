QT += core
QT -= gui
QT += serialport
QT += sql

CONFIG += c++11

TARGET = THCIMeasure
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    database.cpp \
    manager.cpp \
    serial.cpp

HEADERS += \
    database.h \
    manager.h \
    mytype.h \
    serial.h

target.path = /home/alarm/App/THCI
!isEmpty(target.path): INSTALLS += target
