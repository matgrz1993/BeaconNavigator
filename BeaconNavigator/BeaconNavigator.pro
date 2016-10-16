TARGET = BeaconNavigator
INCLUDEPATH += .

QT += quick bluetooth

# Input
SOURCES += main.cpp \
    device.cpp \
    deviceinfo.cpp

OTHER_FILES += assets/*.qml

HEADERS += \
    device.h \
    deviceinfo.h

RESOURCES += \
    resources.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/bluetooth/BeaconNavigator
INSTALLS += target