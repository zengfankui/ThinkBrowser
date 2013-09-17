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

#include "browserapplication.h"

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(data);

    //设置程序运行的编码
    QTextCodec::setCodecForLocale(QTextCodec::codecForName(_SZ_CodecForLocale));
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0) )
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName(_SZ_CodecForCStrings));
    QTextCodec::setCodecForTr(QTextCodec::codecForName(_SZ_CodecForTr));
#endif

	//创建程序并运行
    BrowserApplication application(argc, argv);
    if (!application.isTheOnlyBrowser())
        return 0;

	//运行
    application.newMainWindow();
    return application.exec();
}

