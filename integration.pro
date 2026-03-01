QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    integration.cpp\
    connection.cpp \
    publication.cpp \
    equipement.cpp \
    crudebiosimple.cpp \
    crudexperience.cpp \
    chatbotbiosimple.cpp \
    cong.cpp \
    main.cpp

HEADERS += \
    integration.h\
    connection.h \
    publication.h \
    equipement.h \
    crudebiosimple.h \
    crudexperience.h \
    chatbotbiosimple.h \
    cong.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
