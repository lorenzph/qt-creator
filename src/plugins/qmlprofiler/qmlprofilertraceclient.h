/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlprofiler_global.h"

#include <qmldebug/qmldebugclient.h>

#include <QStack>
#include <QStringList>

namespace QmlProfiler {

class QmlProfilerTraceClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    QmlProfilerTraceClient(QmlDebug::QmlDebugConnection *client, quint64 features);
    ~QmlProfilerTraceClient();

    bool isRecording() const;
    void setRecording(bool);
    quint64 recordedFeatures() const;

public slots:
    void clearData();
    void sendRecordingStatus(int engineId = -1);
    void setRequestedFeatures(quint64 features);
    void setFlushInterval(quint32 flushInterval);

signals:
    void complete(qint64 maximumTime);
    void traceFinished(qint64 timestamp, const QList<int> &engineIds);
    void traceStarted(qint64 timestamp, const QList<int> &engineIds);

    void rangedEvent(Message, RangeType, int detailType, qint64 startTime, qint64 length,
                     const QString &data, const QmlEventLocation &location, qint64 param1,
                     qint64 param2, qint64 param3, qint64 param4, qint64 param5);

    void debugMessage(qint64 timestamp, QtMsgType type, const QString &text,
                      const QmlEventLocation &location);

    void recordingChanged(bool arg);
    void recordedFeaturesChanged(quint64 features);
    void newEngine(int engineId);

    void cleared();

protected:
    virtual void stateChanged(State status);
    virtual void messageReceived(const QByteArray &);

private:
    void setRecordingFromServer(bool);

private:
    class QmlProfilerTraceClientPrivate *d;
};

} // namespace QmlProfiler
