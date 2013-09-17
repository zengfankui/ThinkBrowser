/****************************************************************************
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

#include "browsermainwindow.h"

#include "autosaver.h"
#include "bookmarks.h"
#include "browserapplication.h"
#include "chasewidget.h"
#include "downloadmanager.h"
#include "history.h"
#include "settings.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "ui_passworddialog.h"
#include "webview.h"

#include <QtCore/QSettings>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtGui/QDesktopWidget>
#include <QtGui/QFileDialog>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrintPreviewDialog>
#include <QtGui/QPrinter>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QInputDialog>
#include <QtWebKit/QWebFrame>
#else
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QPlainTextEdit>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QInputDialog>
#include <QtWebKitWidgets/QWebFrame>
#endif

#include <QtWebKit/QWebHistory>

#include <QtCore/QDebug>

BrowserMainWindow::BrowserMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_tabWidget(new TabWidget(this))
    , m_autoSaver(new AutoSaver(this))
    , m_historyBack(0)
    , m_historyForward(0)
    , m_stop(0)
    , m_reload(0)
{

    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAttribute(Qt::WA_DeleteOnClose, true);
    statusBar()->setSizeGripEnabled(true);
    setupMenu();
    setupToolBar();

    QWidget *centralWidget = new QWidget(this);
    BookmarksModel *boomarksModel = BrowserApplication::bookmarksManager()->bookmarksModel();
    m_bookmarksToolbar = new BookmarksToolBar(boomarksModel, this);
    connect(m_bookmarksToolbar, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(m_bookmarksToolbar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateBookmarksToolbarActionText(bool)));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
#if defined(Q_WS_MAC)
    layout->addWidget(m_bookmarksToolbar);
    layout->addWidget(new QWidget); // <- OS X tab widget style bug
#else
    addToolBarBreak();
    addToolBar(m_bookmarksToolbar);
#endif
    layout->addWidget(m_tabWidget);
    centralWidget->setLayout(layout);
	setCentralWidget(centralWidget);

    connect(m_tabWidget, SIGNAL(loadPage(QString)),
        this, SLOT(loadPage(QString)));
    connect(m_tabWidget, SIGNAL(setCurrentTitle(QString)),
        this, SLOT(slotUpdateWindowTitle(QString)));
    connect(m_tabWidget, SIGNAL(showStatusBarMessage(QString)),
            statusBar(), SLOT(showMessage(QString)));
    connect(m_tabWidget, SIGNAL(linkHovered(QString)),
            statusBar(), SLOT(showMessage(QString)));
    connect(m_tabWidget, SIGNAL(loadProgress(int)),
            this, SLOT(slotLoadProgress(int)));
    connect(m_tabWidget, SIGNAL(tabsChanged()),
            m_autoSaver, SLOT(changeOccurred()));
    connect(m_tabWidget, SIGNAL(geometryChangeRequested(QRect)),
            this, SLOT(geometryChangeRequested(QRect)));
    connect(m_tabWidget, SIGNAL(printRequested(QWebFrame*)),
            this, SLOT(printRequested(QWebFrame*)));
    connect(m_tabWidget, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            menuBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            statusBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_navigationBar, SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_bookmarksToolbar, SLOT(setVisible(bool)));
#if defined(Q_WS_MAC)
    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            this, SLOT(close()));
#else
    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            m_tabWidget, SLOT(newTab()));
#endif

    slotUpdateWindowTitle();
    loadDefaultState();
    m_tabWidget->newTab();

    int size = m_tabWidget->lineEditStack()->sizeHint().height();
    m_navigationBar->setIconSize(QSize(size, size));

}

BrowserMainWindow::~BrowserMainWindow()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNeccessary();
}

void BrowserMainWindow::loadDefaultState()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = settings.value(QLatin1String("defaultState")).toByteArray();
    restoreState(data);
    settings.endGroup();
}

QSize BrowserMainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

void BrowserMainWindow::save()
{
    BrowserApplication::instance()->saveSession();

    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = saveState(false);
    settings.setValue(QLatin1String("defaultState"), data);
    settings.endGroup();
}

static const qint32 BrowserMainWindowMagic = 0xba;

QByteArray BrowserMainWindow::saveState(bool withTabs) const
{
    int version = 2;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(BrowserMainWindowMagic);
    stream << qint32(version);

    stream << size();
    stream << !m_navigationBar->isHidden();
    stream << !m_bookmarksToolbar->isHidden();
    stream << !statusBar()->isHidden();
    if (withTabs)
        stream << tabWidget()->saveState();
    else
        stream << QByteArray();
    return data;
}

bool BrowserMainWindow::restoreState(const QByteArray &state)
{
    int version = 2;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != BrowserMainWindowMagic || v != version)
        return false;

    QSize size;
    bool showToolbar;
    bool showBookmarksBar;
    bool showStatusbar;
    QByteArray tabState;

    stream >> size;
    stream >> showToolbar;
    stream >> showBookmarksBar;
    stream >> showStatusbar;
    stream >> tabState;

    resize(size);

    m_navigationBar->setVisible(showToolbar);
    updateToolbarActionText(showToolbar);

    m_bookmarksToolbar->setVisible(showBookmarksBar);
    updateBookmarksToolbarActionText(showBookmarksBar);

    statusBar()->setVisible(showStatusbar);
    updateStatusbarActionText(showStatusbar);

    if (!tabWidget()->restoreState(tabState))
        return false;

    return true;
}

void BrowserMainWindow::setupMenu()
{
    new QShortcut(QKeySequence(Qt::Key_F6), this, SLOT(slotSwapFocus()));

    // File
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    fileMenu->addAction(tr("新建窗口(&N)"), this, SLOT(slotFileNew()), QKeySequence::New);
    fileMenu->addAction(m_tabWidget->newTabAction());
    fileMenu->addAction(tr("打开文件(&O)..."), this, SLOT(slotFileOpen()), QKeySequence::Open);
    fileMenu->addAction(tr("打开位置(&L)..."), this,
                SLOT(slotSelectLineEdit()), QKeySequence(Qt::ControlModifier + Qt::Key_L));
    fileMenu->addSeparator();
    fileMenu->addAction(m_tabWidget->closeTabAction());
    fileMenu->addSeparator();
    fileMenu->addAction(tr("另存为(&S)..."), this,
                SLOT(slotFileSaveAs()), QKeySequence(QKeySequence::Save));
    fileMenu->addSeparator();
    BookmarksManager *bookmarksManager = BrowserApplication::bookmarksManager();
    fileMenu->addAction(tr("导入书签(&I)..."), bookmarksManager, SLOT(importBookmarks()));
    fileMenu->addAction(tr("导出书签(&E)..."), bookmarksManager, SLOT(exportBookmarks()));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("打印预览(&V)..."), this, SLOT(slotFilePrintPreview()));
    fileMenu->addAction(tr("打印(&P)..."), this, SLOT(slotFilePrint()), QKeySequence::Print);
    fileMenu->addSeparator();
    QAction *action = fileMenu->addAction(tr("私有浏览模式..."), this, SLOT(slotPrivateBrowsing()));
    action->setCheckable(true);
    fileMenu->addSeparator();

#if defined(Q_WS_MAC)
    fileMenu->addAction(tr("关闭(&Q)"), BrowserApplication::instance(), SLOT(quitBrowser()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#else
    fileMenu->addAction(tr("关闭(&Q)"), this, SLOT(close()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#endif

    // Edit
    QMenu *editMenu = menuBar()->addMenu(tr("编辑(&E)"));
    QAction *m_undo = editMenu->addAction(tr("撤销(&U)"));
    m_undo->setShortcuts(QKeySequence::Undo);
    m_tabWidget->addWebAction(m_undo, QWebPage::Undo);
    QAction *m_redo = editMenu->addAction(tr("重做(&R)"));
    m_redo->setShortcuts(QKeySequence::Redo);
    m_tabWidget->addWebAction(m_redo, QWebPage::Redo);
    editMenu->addSeparator();
    QAction *m_cut = editMenu->addAction(tr("剪切(&T)"));
    m_cut->setShortcuts(QKeySequence::Cut);
    m_tabWidget->addWebAction(m_cut, QWebPage::Cut);
    QAction *m_copy = editMenu->addAction(tr("复制(&C)"));
    m_copy->setShortcuts(QKeySequence::Copy);
    m_tabWidget->addWebAction(m_copy, QWebPage::Copy);
    QAction *m_paste = editMenu->addAction(tr("粘贴(&P)"));
    m_paste->setShortcuts(QKeySequence::Paste);
    m_tabWidget->addWebAction(m_paste, QWebPage::Paste);
    editMenu->addSeparator();

    QAction *m_find = editMenu->addAction(tr("查找(&F)"));
    m_find->setShortcuts(QKeySequence::Find);
    connect(m_find, SIGNAL(triggered()), this, SLOT(slotEditFind()));
    new QShortcut(QKeySequence(Qt::Key_Slash), this, SLOT(slotEditFind()));

    QAction *m_findNext = editMenu->addAction(tr("查找下一个(&F)"));
    m_findNext->setShortcuts(QKeySequence::FindNext);
    connect(m_findNext, SIGNAL(triggered()), this, SLOT(slotEditFindNext()));

    QAction *m_findPrevious = editMenu->addAction(tr("查找上一个(&F)"));
    m_findPrevious->setShortcuts(QKeySequence::FindPrevious);
    connect(m_findPrevious, SIGNAL(triggered()), this, SLOT(slotEditFindPrevious()));

    editMenu->addSeparator();
    editMenu->addAction(tr("首选项设置(&P)"), this, SLOT(slotPreferences()), tr("Ctrl+,"));

    // View
    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));

    m_viewBookmarkBar = new QAction(this);
    updateBookmarksToolbarActionText(true);
    m_viewBookmarkBar->setShortcut(tr("Shift+Ctrl+B"));
    connect(m_viewBookmarkBar, SIGNAL(triggered()), this, SLOT(slotViewBookmarksBar()));
    viewMenu->addAction(m_viewBookmarkBar);

    m_viewToolbar = new QAction(this);
    updateToolbarActionText(true);
    m_viewToolbar->setShortcut(tr("Ctrl+|"));
    connect(m_viewToolbar, SIGNAL(triggered()), this, SLOT(slotViewToolbar()));
    viewMenu->addAction(m_viewToolbar);

    m_viewStatusbar = new QAction(this);
    updateStatusbarActionText(true);
    m_viewStatusbar->setShortcut(tr("Ctrl+/"));
    connect(m_viewStatusbar, SIGNAL(triggered()), this, SLOT(slotViewStatusbar()));
    viewMenu->addAction(m_viewStatusbar);

    viewMenu->addSeparator();

    m_stop = viewMenu->addAction(tr("停止(&S)"));
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Period));
    shortcuts.append(Qt::Key_Escape);
    m_stop->setShortcuts(shortcuts);
    m_tabWidget->addWebAction(m_stop, QWebPage::Stop);

    m_reload = viewMenu->addAction(tr("重新装载页面"));
    m_reload->setShortcuts(QKeySequence::Refresh);
    m_tabWidget->addWebAction(m_reload, QWebPage::Reload);

    viewMenu->addAction(tr("放大(&I)"), this, SLOT(slotViewZoomIn()), QKeySequence(Qt::CTRL | Qt::Key_Plus));
    viewMenu->addAction(tr("缩小(&O)"), this, SLOT(slotViewZoomOut()), QKeySequence(Qt::CTRL | Qt::Key_Minus));
    viewMenu->addAction(tr("重置(&Z)"), this, SLOT(slotViewResetZoom()), QKeySequence(Qt::CTRL | Qt::Key_0));
    QAction *zoomTextOnlyAction = viewMenu->addAction(tr("只缩放文本(&T)"));
    connect(zoomTextOnlyAction, SIGNAL(toggled(bool)), this, SLOT(slotViewZoomTextOnly(bool)));
    zoomTextOnlyAction->setCheckable(true);
    zoomTextOnlyAction->setChecked(false);

    viewMenu->addSeparator();
    viewMenu->addAction(tr("查看页面源代码..."), this, SLOT(slotViewPageSource()), tr("Ctrl+Alt+U"));
    QAction *a = viewMenu->addAction(tr("全屏幕显示(&F)"), this, SLOT(slotViewFullScreen(bool)),  Qt::Key_F11);
    a->setCheckable(true);

    // History
    HistoryMenu *historyMenu = new HistoryMenu(this);
    connect(historyMenu, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(historyMenu, SIGNAL(hovered(QString)), this,
            SLOT(slotUpdateStatusbar(QString)));
    historyMenu->setTitle(tr("历史(&s)"));
    menuBar()->addMenu(historyMenu);
    QList<QAction*> historyActions;

    m_historyBack = new QAction(tr("向后"), this);
    m_tabWidget->addWebAction(m_historyBack, QWebPage::Back);
    m_historyBack->setShortcuts(QKeySequence::Back);
    m_historyBack->setIconVisibleInMenu(false);

    m_historyForward = new QAction(tr("向前"), this);
    m_tabWidget->addWebAction(m_historyForward, QWebPage::Forward);
    m_historyForward->setShortcuts(QKeySequence::Forward);
    m_historyForward->setIconVisibleInMenu(false);

    QAction *m_historyHome = new QAction(tr("首页"), this);
    connect(m_historyHome, SIGNAL(triggered()), this, SLOT(slotHome()));
    m_historyHome->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));

    m_restoreLastSession = new QAction(tr("恢复最后会话"), this);
    connect(m_restoreLastSession, SIGNAL(triggered()), BrowserApplication::instance(), SLOT(restoreLastSession()));
    m_restoreLastSession->setEnabled(BrowserApplication::instance()->canRestoreSession());

    historyActions.append(m_historyBack);
    historyActions.append(m_historyForward);
    historyActions.append(m_historyHome);
    historyActions.append(m_tabWidget->recentlyClosedTabsAction());
    historyActions.append(m_restoreLastSession);
    historyMenu->setInitialActions(historyActions);

    // Bookmarks
    BookmarksMenu *bookmarksMenu = new BookmarksMenu(this);
    connect(bookmarksMenu, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(bookmarksMenu, SIGNAL(hovered(QString)),
            this, SLOT(slotUpdateStatusbar(QString)));
    bookmarksMenu->setTitle(tr("书签(&B)"));
    menuBar()->addMenu(bookmarksMenu);

    QList<QAction*> bookmarksActions;

    QAction *showAllBookmarksAction = new QAction(tr("显示全部书签"), this);
    connect(showAllBookmarksAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksDialog()));
    m_addBookmark = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("添加书签..."), this);
    m_addBookmark->setIconVisibleInMenu(false);

    connect(m_addBookmark, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
    m_addBookmark->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    bookmarksActions.append(showAllBookmarksAction);
    bookmarksActions.append(m_addBookmark);
    bookmarksMenu->setInitialActions(bookmarksActions);

    // Window
    m_windowMenu = menuBar()->addMenu(tr("窗口(&W)"));
    connect(m_windowMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowWindowMenu()));
    slotAboutToShowWindowMenu();

    QMenu *toolsMenu = menuBar()->addMenu(tr("工具(&T)"));
    toolsMenu->addAction(tr("企业搜索(&S)"), this, SLOT(slotWebSearch()), QKeySequence(tr("Ctrl+K", "企业收索")));
    a = toolsMenu->addAction(tr("启用页面观察器(&I)"), this, SLOT(slotToggleInspector(bool)));
    a->setCheckable(true);

    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(tr("关于..."), this, SLOT(slotAboutApplication()));
}

void BrowserMainWindow::setupToolBar()
{
    setUnifiedTitleAndToolBarOnMac(true);
    m_navigationBar = addToolBar(tr("导航"));
    connect(m_navigationBar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateToolbarActionText(bool)));

    m_historyBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_historyBackMenu = new QMenu(this);
    m_historyBack->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotOpenActionUrl(QAction*)));
    m_navigationBar->addAction(m_historyBack);

    m_historyForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_historyForwardMenu = new QMenu(this);
    connect(m_historyForwardMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowForwardMenu()));
    connect(m_historyForwardMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotOpenActionUrl(QAction*)));
    m_historyForward->setMenu(m_historyForwardMenu);
    m_navigationBar->addAction(m_historyForward);

    m_stopReload = new QAction(this);
    m_reloadIcon = style()->standardIcon(QStyle::SP_BrowserReload);
    m_stopReload->setIcon(m_reloadIcon);

    m_navigationBar->addAction(m_stopReload);

    m_navigationBar->addWidget(m_tabWidget->lineEditStack());

    m_toolbarSearch = new ToolbarSearch(m_navigationBar);
    m_navigationBar->addWidget(m_toolbarSearch);
    connect(m_toolbarSearch, SIGNAL(search(QUrl)), SLOT(loadUrl(QUrl)));

    m_chaseWidget = new ChaseWidget(this);
    m_navigationBar->addWidget(m_chaseWidget);
}

void BrowserMainWindow::slotShowBookmarksDialog()
{
    BookmarksDialog *dialog = new BookmarksDialog(this);
    connect(dialog, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    dialog->show();
}

void BrowserMainWindow::slotAddBookmark()
{
    WebView *webView = currentTab();
    QString url = webView->url().toString();
    QString title = webView->title();
    AddBookmarkDialog dialog(url, title);
    dialog.exec();
}

void BrowserMainWindow::slotViewToolbar()
{
    if (m_navigationBar->isVisible()) {
        updateToolbarActionText(false);
        m_navigationBar->close();
    } else {
        updateToolbarActionText(true);
        m_navigationBar->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::slotViewBookmarksBar()
{
    if (m_bookmarksToolbar->isVisible()) {
        updateBookmarksToolbarActionText(false);
        m_bookmarksToolbar->close();
    } else {
        updateBookmarksToolbarActionText(true);
        m_bookmarksToolbar->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::updateStatusbarActionText(bool visible)
{
    m_viewStatusbar->setText(!visible ? tr("显示状态栏") : tr("隐藏状态栏"));
}

void BrowserMainWindow::updateToolbarActionText(bool visible)
{
    m_viewToolbar->setText(!visible ? tr("显示工具栏") : tr("隐藏工具栏"));
}

void BrowserMainWindow::updateBookmarksToolbarActionText(bool visible)
{
    m_viewBookmarkBar->setText(!visible ? tr("显示书签栏") : tr("隐藏书签栏"));
}

void BrowserMainWindow::slotViewStatusbar()
{
    if (statusBar()->isVisible()) {
        updateStatusbarActionText(false);
        statusBar()->close();
    } else {
        updateStatusbarActionText(true);
        statusBar()->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::loadUrl(const QUrl &url)
{
    if (!currentTab() || !url.isValid())
        return;

    m_tabWidget->currentLineEdit()->setText(QString::fromUtf8(url.toEncoded()));
    m_tabWidget->loadUrlInCurrentTab(url);
}

void BrowserMainWindow::slotDownloadManager()
{
    BrowserApplication::downloadManager()->show();
}

void BrowserMainWindow::slotSelectLineEdit()
{
    m_tabWidget->currentLineEdit()->selectAll();
    m_tabWidget->currentLineEdit()->setFocus();
}

void BrowserMainWindow::slotFileSaveAs()
{
    BrowserApplication::downloadManager()->download(currentTab()->url(), true);
}

void BrowserMainWindow::slotPreferences()
{
    SettingsDialog *s = new SettingsDialog(this);
    s->show();
}

void BrowserMainWindow::slotUpdateStatusbar(const QString &string)
{
    statusBar()->showMessage(string, 2000);
}

void BrowserMainWindow::slotUpdateWindowTitle(const QString &title)
{
    if (title.isEmpty()) {
        setWindowTitle(tr("Think 企业应用浏览器"));
    } else {
#if defined(Q_WS_MAC)
        setWindowTitle(title);
#else
        setWindowTitle(tr("%1 - Think 企业应用浏览器", "Think 企业应用浏览器").arg(title));
#endif
    }
}

void BrowserMainWindow::slotAboutApplication()
{
    QMessageBox::about(this, tr("关于"), tr(
        "名称：Think Browser"
        "<p>版本 %1"
        "<p>Think浏览器为企业应用浏览器，提供数据浏览、文档浏览、网页浏览功能， "
        "为企业办公应用提供更方便、有效的办公专用浏览器平台。 "
        "<p>该软件由ThinkBrowser组织提供 <a href=\"http://www.thinkbrowser.org/\">http://www.thinkbrowser.org/</a>."
        "<p>该浏览器基于开源的QtWebKit 以及 WebKit项目 <a href=\"http://qt-project.org/\">http://qt-project.org/</a> <a href=\"http://webkit.org/\">http://webkit.org/</a>."
        ).arg(QCoreApplication::applicationVersion()));
}

void BrowserMainWindow::slotFileNew()
{
    BrowserApplication::instance()->newMainWindow();
    BrowserMainWindow *mw = BrowserApplication::instance()->mainWindow();
    mw->slotHome();
}

void BrowserMainWindow::slotFileOpen()
{
    QString file = QFileDialog::getOpenFileName(this, tr("打开资源"), QString(),
            tr("资源文件 (*.html *.htm *.svg *.png *.gif *.svgz);;所有文件 (*.*)"));

    if (file.isEmpty())
        return;

    loadPage(file);
}

void BrowserMainWindow::slotFilePrintPreview()
{
#ifndef QT_NO_PRINTER
    if (!currentTab())
        return;
    QPrintPreviewDialog *dialog = new QPrintPreviewDialog(this);
    connect(dialog, SIGNAL(paintRequested(QPrinter*)),
            currentTab(), SLOT(print(QPrinter*)));
    dialog->exec();
#endif
}

void BrowserMainWindow::slotFilePrint()
{
    if (!currentTab())
        return;
    printRequested(currentTab()->page()->mainFrame());
}

void BrowserMainWindow::printRequested(QWebFrame *frame)
{
#ifndef QT_NO_PRINTER
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(tr("文档打印"));
    if (dialog->exec() != QDialog::Accepted)
        return;
    frame->print(&printer);
#endif
}

void BrowserMainWindow::slotPrivateBrowsing()
{
    QWebSettings *settings = QWebSettings::globalSettings();
    bool pb = settings->testAttribute(QWebSettings::PrivateBrowsingEnabled);
    if (!pb) {
        QString title = tr("确定启用私有浏览模式吗?");
        QString text = tr("<b>%1</b><br><br>在私有浏览模式下，不会形成浏览历史记录，"\
            "不会留下下载痕迹，不保存和访问当前cookie，"\
            " 不存储会话以及网站图标，不记录搜索痕迹。" ).arg(title);

        QMessageBox::StandardButton button = QMessageBox::question(this, QString(), text,
                               QMessageBox::Ok | QMessageBox::Cancel,
                               QMessageBox::Ok);
        if (button == QMessageBox::Ok) {
            settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
        }
    } else {
        settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, false);

        QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
        for (int i = 0; i < windows.count(); ++i) {
            BrowserMainWindow *window = windows.at(i);
            window->m_lastSearch = QString::null;
            window->tabWidget()->clear();
        }
    }
}

void BrowserMainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tabWidget->count() > 1) {
        int ret = QMessageBox::warning(this, QString(),
                           tr("确定要关闭窗口吗?"
                              "当前还有 %1 个标签页于打开状态！").arg(m_tabWidget->count()),
                           QMessageBox::Yes | QMessageBox::No,
                           QMessageBox::No);
        if (ret == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    event->accept();
    deleteLater();
}

void BrowserMainWindow::slotEditFind()
{
    if (!currentTab())
        return;
    bool ok;
    QString search = QInputDialog::getText(this, tr("查找"),
                                          tr("文本:"), QLineEdit::Normal,
                                          m_lastSearch, &ok);
    if (ok && !search.isEmpty()) {
        m_lastSearch = search;
        if (!currentTab()->findText(m_lastSearch))
            slotUpdateStatusbar(tr("\"%1\" 未找到.").arg(m_lastSearch));
    }
}

void BrowserMainWindow::slotEditFindNext()
{
    if (!currentTab() && !m_lastSearch.isEmpty())
        return;
    currentTab()->findText(m_lastSearch);
}

void BrowserMainWindow::slotEditFindPrevious()
{
    if (!currentTab() && !m_lastSearch.isEmpty())
        return;
    currentTab()->findText(m_lastSearch, QWebPage::FindBackward);
}

void BrowserMainWindow::slotViewZoomIn()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(currentTab()->zoomFactor() + 0.1);
}

void BrowserMainWindow::slotViewZoomOut()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(currentTab()->zoomFactor() - 0.1);
}

void BrowserMainWindow::slotViewResetZoom()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(1.0);
}

void BrowserMainWindow::slotViewZoomTextOnly(bool enable)
{
    if (!currentTab())
        return;
    currentTab()->page()->settings()->setAttribute(QWebSettings::ZoomTextOnly, enable);
}

void BrowserMainWindow::slotViewFullScreen(bool makeFullScreen)
{
    if (makeFullScreen) {
        showFullScreen();
    } else {
        if (isMinimized())
            showMinimized();
        else if (isMaximized())
            showMaximized();
        else showNormal();
    }
}

void BrowserMainWindow::slotViewPageSource()
{
    if (!currentTab())
        return;

    QString markup = currentTab()->page()->mainFrame()->toHtml();
    QPlainTextEdit *view = new QPlainTextEdit(markup);
    view->setWindowTitle(tr("页面 %1 的源代码").arg(currentTab()->title()));
    view->setMinimumWidth(640);
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->show();
}

void BrowserMainWindow::slotHome()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("主窗口"));
    QString home = settings.value(QLatin1String("首页"), QLatin1String("file:///localhost/")).toString();
    loadPage(home);
}

void BrowserMainWindow::slotWebSearch()
{
    m_toolbarSearch->lineEdit()->selectAll();
    m_toolbarSearch->lineEdit()->setFocus();
}

void BrowserMainWindow::slotToggleInspector(bool enable)
{
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, enable);
    if (enable) {
        int result = QMessageBox::question(this, tr("页面观察器"),
                                           tr("已经打开的页面需要重新装载才能使用观察器观察，\n"
                                           "需要重新装载当前打开的页面吗？"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            m_tabWidget->reloadAllTabs();
        }
    }
}

void BrowserMainWindow::slotSwapFocus()
{
    if (currentTab()->hasFocus())
        m_tabWidget->currentLineEdit()->setFocus();
    else
        currentTab()->setFocus();
}

void BrowserMainWindow::loadPage(const QString &page)
{
    QUrl url = QUrl::fromUserInput(page);
    loadUrl(url);
}

TabWidget *BrowserMainWindow::tabWidget() const
{
    return m_tabWidget;
}

WebView *BrowserMainWindow::currentTab() const
{
    return m_tabWidget->currentWebView();
}

void BrowserMainWindow::slotLoadProgress(int progress)
{
    if (progress < 100 && progress > 0) {
        m_chaseWidget->setAnimated(true);
        disconnect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        if (m_stopIcon.isNull())
            m_stopIcon = style()->standardIcon(QStyle::SP_BrowserStop);
        m_stopReload->setIcon(m_stopIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setToolTip(tr("停止加载当前页面"));
    } else {
        m_chaseWidget->setAnimated(false);
        disconnect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setIcon(m_reloadIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        m_stopReload->setToolTip(tr("重新加载当前页面"));
    }
}

void BrowserMainWindow::slotAboutToShowBackMenu()
{
    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    QWebHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = history->backItems(historyCount).count() - 1; i >= 0; --i) {
        QWebHistoryItem item = history->backItems(history->count()).at(i);
        QAction *action = new QAction(this);
        action->setData(-1*(historyCount-i-1));
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowForwardMenu()
{
    m_historyForwardMenu->clear();
    if (!currentTab())
        return;
    QWebHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = 0; i < history->forwardItems(history->count()).count(); ++i) {
        QWebHistoryItem item = history->forwardItems(historyCount).at(i);
        QAction *action = new QAction(this);
        action->setData(historyCount-i);
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyForwardMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowWindowMenu()
{
    m_windowMenu->clear();
    m_windowMenu->addAction(m_tabWidget->nextTabAction());
    m_windowMenu->addAction(m_tabWidget->previousTabAction());
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(tr("下载"), this, SLOT(slotDownloadManager()), QKeySequence(tr("Alt+Ctrl+L", "下载管理器")));

    m_windowMenu->addSeparator();
    QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
    for (int i = 0; i < windows.count(); ++i) {
        BrowserMainWindow *window = windows.at(i);
        QAction *action = m_windowMenu->addAction(window->windowTitle(), this, SLOT(slotShowWindow()));
        action->setData(i);
        action->setCheckable(true);
        if (window == this)
            action->setChecked(true);
    }
}

void BrowserMainWindow::slotShowWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QVariant v = action->data();
        if (v.canConvert<int>()) {
            int offset = qvariant_cast<int>(v);
            QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
            windows.at(offset)->activateWindow();
            windows.at(offset)->currentTab()->setFocus();
        }
    }
}

void BrowserMainWindow::slotOpenActionUrl(QAction *action)
{
    int offset = action->data().toInt();
    QWebHistory *history = currentTab()->history();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first()); // back
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back()); // forward
 }

void BrowserMainWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

