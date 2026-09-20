QT       += core gui widgets

TARGET   = todo-list
TEMPLATE = app

CONFIG  += c++17

SOURCES += \
    main.cpp \
    sources/task.cpp \
    sources/todolistapp.cpp \
    sources/edittaskdialog.cpp

HEADERS += \
    headers/task.h \
    headers/todolistapp.h \
    headers/edittaskdialog.h

INCLUDEPATH += headers