#-------------------------------------------------
#
# Project created by QtCreator 2016-12-01T06:41:33
#
#-------------------------------------------------

QT       += core gui
QT       += sql

QT       += network

QT       += xml

TRANSLATIONS = cn.ts
TRANSLATIONS += de.ts
TRANSLATIONS += en.ts
TRANSLATIONS += sp.ts
TRANSLATIONS += fr.ts
TRANSLATIONS += it.ts
TRANSLATIONS += kr.ts
TRANSLATIONS += rus.ts
TRANSLATIONS += por.ts

TARGET = controller
TEMPLATE = app

DEFINES += CONFIG_CTRL_IFACE

DEFINES += CONFIG_CTRL_IFACE_UNIX

#INCLUDEPATH	+= . .. ../../../../../../860210000/linux/am3358/wpa/wpa_supplicant-0.7.3/src/utils
INCLUDEPATH	+= . .. ./jsoncpp/include ./server ./ecspos ../../../../../../860210000/linux/am3358/wpa/wpa_supplicant-0.7.3/src/utils

SOURCES += main.cpp\
    ccb.cpp \
        mainwindow.cpp \
    ctrlapplication.cpp \
    list.c \
    log.c \
    helper.c \
    dnetworkaccessmanager.cpp \
    dhttpworker.cpp \
    dsqltablemodelforuser.cpp \
    printer.cpp \
    dxmlgenerator.cpp \
    dcalcpackflow.cpp \
    dcheckconsumaleinstall.cpp \
    dloginstate.cpp \
    duserinfochecker.cpp \
    exconfig.cpp \
    jsoncpp/json_writer.cpp \
    jsoncpp/json_value.cpp \
    jsoncpp/json_reader.cpp \
    server/mongoose.c \
    web_server.cpp \
    canItf.cpp \
    crecordparams.cpp
HEADERS  += mainwindow.h \
    ctrlapplication.h \
    log.h \
    eco_w.h \
    MyParams.h \
    memory.h \
    DNetworkConfig.h \
    dnetworkaccessmanager.h \
    dhttpworker.h \
    dsqltablemodelforuser.h \
    escpos/init_parser.h \
    printer.h \
    dxmlgenerator.h \
    exdisplay.h \
    exconfig.h \
    dcalcpackflow.h \
    dcheckconsumaleinstall.h \
    dloginstate.h \
    duserinfochecker.h \
    jsoncpp/include/json/json.h \
    web_server.h \
    ccb.h \
    config.h \
    canItf.h \
    cminterface.h \
    crecordparams.h

FORMS    += mainwindow.ui

RESOURCES += \
    ime.qrc \
    syszuxpinyin.qrc \
    app.qrc \
    image.qrc \
    language.qrc \
    other.qrc \
    json.qrc

OTHER_FILES += \
    pics/image/arrow_down_active.png \
    pics/image/battery0.png \
    slider.qss \
    musicslider.qss \
    cn.ts \
    de.ts \
    en.ts \
    Table.qss \
    combox.qss

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/release/ -lcommon
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/debug/ -lcommon
#else:symbian: LIBS += -lcommon
#else:unix: LIBS += -L$$PWD  -lcommon
#INCLUDEPATH += $$PWD/
#DEPENDPATH += $$PWD/

#win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/release/common.lib
#else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/debug/common.lib
#else:unix:!symbian: PRE_TARGETDEPS += $$PWD/libcommon.a

##
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/release/ -lcommon -lescpos
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/debug/ -lcommon -lescpos
else:symbian: LIBS += -lcommon
else:unix: LIBS += -L$$PWD  -lcommon -lescpos
INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/release/common.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/debug/common.lib
else:unix:!symbian: PRE_TARGETDEPS += $$PWD/libcommon.a

