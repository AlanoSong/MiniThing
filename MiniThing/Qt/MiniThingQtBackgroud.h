#pragma once

#include <QThread>
#include <QDebug>

#include "../Core/MiniThingCore.h"

class MiniThingQtWorkThread : public QThread
{
public:
    MiniThingQtWorkThread();
    ~MiniThingQtWorkThread();

    MiniThingQtWorkThread(MiniThingCore* pMiniThingCore);

    bool isMiniThingCoreReady(void) { return m_pMiniThingCore->IsCoreReady(); };

private:
    virtual void run();
    MiniThingCore* m_pMiniThingCore;
    bool m_isMiniThingCoreReady;

signals:

public slots:
};
