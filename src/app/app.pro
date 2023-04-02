QT       += core gui opengl
DEFINES += CORE_UTILS=\\\"C:\\\\\\\\mingw\\\\\\\\coreutils-5.3.0\\\\\\\\bin\\\"
CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# QMAKE_LFLAGS += rdynamic

# CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    ../library \
    ../qtrenderer \
    C:/Qt/Tools/mingw810_64/x86_64-w64-mingw32/include/GL

SOURCES += \
    main.cpp \
    ui/SimulationController.cpp \
    ui/gui/BrainMonitorView.cpp \
    ui/gui/ChartMonitorView.cpp \
    ui/gui/MainWindow.cpp \
    ui/gui/MonitorView.cpp \
    ui/gui/PovMonitorView.cpp \
    ui/gui/SceneMonitorView.cpp \
    ui/gui/StatusTextMonitorView.cpp \
    ui/gui/ToggleWidgetOpenAction.cpp

HEADERS += \
    ui/SimulationController.h \
    ui/gui/BinChartViewMonitor.h \
    ui/gui/BrainMonitorView.h \
    ui/gui/ChartMonitorView.h \
    ui/gui/MainWindow.h \
    ui/gui/MonitorView.h \
    ui/gui/PovMonitorView.h \
    ui/gui/SceneMonitorView.h \
    ui/gui/StatusTextMonitorView.h \
    ui/gui/ToggleWidgetOpenAction.h

TARGET = polyworld

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../library/release/ -llibrary
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../library/debug/ -llibrary

INCLUDEPATH += $$PWD/../library
DEPENDPATH += $$PWD/../library

win32: LIBS += -lgsl

win32: LIBS += -lglu32

win32: LIBS += -lopengl32

win32: LIBS += -lws2_32

win32: LIBS += -lz
