#pragma once

#include <QThread>
#include <QDebug>
#include <qstatusbar.h>

#include "../Core/MiniThingCore.h"

class MiniThingQtWorkThread : public QThread
{
    Q_OBJECT

public:
    MiniThingQtWorkThread();
    ~MiniThingQtWorkThread();

    MiniThingQtWorkThread(MiniThingCore* pMiniThingCore, QStatusBar* hStatusBar);

    bool isMiniThingCoreReady(void) { return m_pMiniThingCore->IsCoreReady(); };

private:
    virtual void run();
    MiniThingCore* m_pMiniThingCore;
    QStatusBar* m_hStatusBar;
    bool m_isMiniThingCoreReady;

signals:
    void UpdateStatusBar(const QString& message);

public slots:
};
