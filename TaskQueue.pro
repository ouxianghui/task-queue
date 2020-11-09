TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        event.cpp \
        main.cpp \
        task_queue.cpp \
        task_queue_base.cpp \
        task_queue_std.cpp

HEADERS += \
    event.h \
    queued_task.h \
    task_queue.h \
    task_queue_base.h \
    task_queue_std.h
