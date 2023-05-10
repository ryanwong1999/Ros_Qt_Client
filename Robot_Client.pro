QT       += core gui websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network sql

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    appinit.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    roboItem.cpp

HEADERS += \
    appinit.h \
    login.h \
    mainwindow.h \
    roboItem.h

FORMS += \
    login.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES +=

LIBS      += -lVLCQtCore -lVLCQtWidgets

# Edit below for custom library location
#LIBS       += -LD:/APP/Qt/Qt5.14.2/5.14.2/mingw73_32/lib -lVLCQtCore -lVLCQtWidgets
#INCLUDEPATH += D:/APP/Qt/Qt5.14.2/5.14.2/mingw73_32include
LIBS       += -LD:/AAASOFTWARE/Qt/Qt5.14.2/5.14.2/mingw73_32/lib -lVLCQtCore -lVLCQtWidgets
INCLUDEPATH += D:/AAASOFTWARE/Qt/Qt5.14.2/5.14.2/mingw73_32include

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
