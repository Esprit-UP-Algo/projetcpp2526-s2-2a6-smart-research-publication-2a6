QT       += core gui sql network printsupport multimedia texttospeech serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17
QMAKE_CC = C:/Qt/Tools/mingw1120_64/bin/gcc.exe
QMAKE_CXX = C:/Qt/Tools/mingw1120_64/bin/g++.exe
QMAKE_LINK = C:/Qt/Tools/mingw1120_64/bin/g++.exe
INCLUDEPATH += \
    $$PWD \
    $$PWD/models \
    $$PWD/views

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    views/arduino.cpp \
    models/publicationScorer.cpp \
    views/LeconsApprises.cpp \
    views/captchawidget.cpp \
    views/gestproj.cpp \
    views/TracabiliteManager.cpp \
    views/gestproj_financial_report.cpp \
    views/integration.cpp \
    views/signupserver.cpp \
    views/connection.cpp \
    views/employes.cpp \
    views/publication.cpp \
    views/crudEquipement.cpp \
    views/equipement.cpp \
    views/crudebiosimple.cpp \
    views/crudexperience.cpp \
    views/saisiebio.cpp \
    views/chatbotbiosimple.cpp \
    views/cong.cpp \
    views/basicbio.cpp \
    views/pdfbiosample.cpp \
    views/pdfequipement.cpp \
    views/pdfExp.cpp \
    views/pdfemploye.cpp \
    views/floatingchatbtn.cpp \
    views/voicecommande.cpp \
    views/simple.cpp \
    views/emailsender.cpp \
    views/main.cpp

HEADERS += \
    models/arduino.h \
    models/LeconsApprises.h \
    models/apiconfig.h \
    models/basicbio.h \
    models/captchawidget.h \
    models/chatbotbiosimple.h \
    models/TracabiliteManager.h \
    models/cong.h \
    models/connection.h \
    models/crudebiosimple.h \
    models/crudEquipement.h \
    models/crudexperience.h \
    models/employes.h \
    models/equipement.h \
    models/floatingchatbtn.h \
    models/gestproj.h \
    models/integration.h \
    models/pdfbiosample.h \
    models/pdfequipement.h \
    models/pdfExp.h \
    models/pdfemploye.h \
    models/publication.h \
    models/publicationScorer.h \
    models/saisiebio.h \
    models/simple.h \
    models/voicecommande.h \
    models/emailsender.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# smtp_mail.ini is discovered at runtime from the executable directory,
# current directory, and parent folders, so no fragile post-link copy is needed.

RESOURCES += \
    resources.qrc
