﻿/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BROWSERAPPLICATION_H
#define BROWSERAPPLICATION_H

#include <QtCore/QTextCodec>
#include <QtCore/QUrl>
#include <QtCore/QPointer>
#if (QT_VERSION < 0x050000)
#include <QtGui/QApplication>
#else
#include <QtWidgets/QApplication>
#endif

//字符集转换
#if ( QT_VERSION < QT_VERSION_CHECK(5,0,0) )
#ifdef Q_WS_X11
    #define _SZ_CodecForCStrings	"UTF-8"
    #define _SZ_CodecForLocale		"UTF-8"
    #define _SZ_CodecForTr			"UTF-8"
#endif

#ifdef Q_WS_WIN
    #define _SZ_CodecForCStrings	"UTF-8"
    #define _SZ_CodecForLocale		"UTF-8"
    #define _SZ_CodecForTr			"GB18030"
#endif
#else
#ifdef Q_OS_LINUX
    #define _SZ_CodecForCStrings	"UTF-8"
    #define _SZ_CodecForLocale		"UTF-8"
    #define _SZ_CodecForTr			"UTF-8"
#endif

#ifdef Q_OS_WIN
    #define _SZ_CodecForCStrings	"UTF-8"
    #define _SZ_CodecForLocale		"UTF-8"
    #define _SZ_CodecForTr			"GB18030"
#endif
#endif

#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QLocalServer;
QT_END_NAMESPACE

class BookmarksManager;
class BrowserMainWindow;
class CookieJar;
class DownloadManager;
class HistoryManager;
class NetworkAccessManager;
class BrowserApplication : public QApplication
{
    Q_OBJECT

public:
    BrowserApplication(int &argc, char **argv);
    ~BrowserApplication();
    static BrowserApplication *instance();
    void loadSettings();

    bool isTheOnlyBrowser() const;
    BrowserMainWindow *mainWindow();
    QList<BrowserMainWindow*> mainWindows();
    QIcon icon(const QUrl &url) const;

    void saveSession();
    bool canRestoreSession() const;

    static HistoryManager *historyManager();
    static CookieJar *cookieJar();
    static DownloadManager *downloadManager();
    static NetworkAccessManager *networkAccessManager();
    static BookmarksManager *bookmarksManager();

#if defined(Q_WS_MAC)
    bool event(QEvent *event);
#endif

public slots:
    BrowserMainWindow *newMainWindow();
    void restoreLastSession();
#if defined(Q_WS_MAC)
    void lastWindowClosed();
    void quitBrowser();
#endif

private slots:
    void postLaunch();
    void openUrl(const QUrl &url);
    void newLocalSocketConnection();

private:
    void clean();
    void installTranslator(const QString &name);

    static HistoryManager *s_historyManager;
    static DownloadManager *s_downloadManager;
    static NetworkAccessManager *s_networkAccessManager;
    static BookmarksManager *s_bookmarksManager;

    QList<QPointer<BrowserMainWindow> > m_mainWindows;
    QLocalServer *m_localServer;
    QByteArray m_lastSession;
    mutable QIcon m_defaultIcon;
};

#endif // BROWSERAPPLICATION_H

