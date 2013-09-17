TEMPLATE = app
TARGET = iThink

greaterThan(QT_MAJOR_VERSION, 4)|equals(QT_MAJOR_VERSION,5){
    message( "Qt VERSION 5 or above found..." )
    QT += webkitwidgets network widgets printsupport
    contains(QT_BUILD_PARTS, tools):!embedded: QT += uitools
    else: DEFINES += QT_NO_UITOOLS
}else{
    message( "Qt VERSION 4 or less found..." )
    QT += webkit network
    contains(QT_BUILD_PARTS, tools):!embedded: CONFIG += uitools
    else: DEFINES += QT_NO_UITOOLS
}

CONFIG += qt warn_on

TRANSLATIONS = iThink_zh_TW.ts \
               iThink_en.ts

CODECFORSRC = UTF-8

FORMS += \
    addbookmarkdialog.ui \
    bookmarks.ui \
    cookies.ui \
    cookiesexceptions.ui \
    downloaditem.ui \
    downloads.ui \
    history.ui \
    passworddialog.ui \
    proxy.ui \
    settings.ui

HEADERS += \
    autosaver.h \
    bookmarks.h \
    browserapplication.h \
    browsermainwindow.h \
    chasewidget.h \
    cookiejar.h \
    downloadmanager.h \
    edittableview.h \
    edittreeview.h \
    history.h \
    modelmenu.h \
    networkaccessmanager.h \
    searchlineedit.h \
    settings.h \
    squeezelabel.h \
    tabwidget.h \
    toolbarsearch.h \
    urllineedit.h \
    webview.h \
    xbm.h

SOURCES += \
    autosaver.cpp \
    bookmarks.cpp \
    browserapplication.cpp \
    browsermainwindow.cpp \
    chasewidget.cpp \
    cookiejar.cpp \
    downloadmanager.cpp \
    edittableview.cpp \
    edittreeview.cpp \
    history.cpp \
    modelmenu.cpp \
    networkaccessmanager.cpp \
    searchlineedit.cpp \
    settings.cpp \
    squeezelabel.cpp \
    tabwidget.cpp \
    toolbarsearch.cpp \
    urllineedit.cpp \
    webview.cpp \
    main.cpp \
    xbm.cpp

RESOURCES += data/data.qrc htmls/htmls.qrc

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

win32 {
   RC_FILE = thinkbrowser.rc
}

mac {
    ICON = thinkbrowser.icns
    QMAKE_INFO_PLIST = Info_mac.plist
    TARGET = Browser

    # No 64-bit Flash on Mac, so build the browser 32-bit
    contains(QT_CONFIG, x86) {
        CONFIG -= x86_64
        CONFIG += x86
    }
    contains(QT_CONFIG, ppc) {
        CONFIG -= ppc64
        CONFIG += ppc
    }
}

wince*: {
    DEPLOYMENT_PLUGIN += qjpeg qgif
}
