#include "MiniThingQtBackgroud.h"


MiniThingQtWorkThread::MiniThingQtWorkThread()
{
}

MiniThingQtWorkThread::~MiniThingQtWorkThread()
{
}

MiniThingQtWorkThread::MiniThingQtWorkThread(MiniThingCore* pMiniThingCore)
{
    m_pMiniThingCore = pMiniThingCore;
    m_isMiniThingCoreReady = false;
}

void MiniThingQtWorkThread::run()
{
    m_isMiniThingCoreReady = false;
    m_pMiniThingCore->StartInstance();
    qDebug() << "Minithing core ready";

    m_isMiniThingCoreReady = true;
}