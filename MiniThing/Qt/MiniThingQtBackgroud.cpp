#include "MiniThingQtBackgroud.h"
//#include "MiniThingQt.h"


MiniThingQtWorkThread::MiniThingQtWorkThread()
{
}

MiniThingQtWorkThread::~MiniThingQtWorkThread()
{
}


MiniThingQtWorkThread::MiniThingQtWorkThread(MiniThingCore* pMiniThingCore, QStatusBar* hStatusBar = nullptr)
{
    m_pMiniThingCore = pMiniThingCore;
    m_hStatusBar = hStatusBar;
}

void MiniThingQtWorkThread::run()
{
    m_pMiniThingCore->StartInstance(this);

    m_pMiniThingCore->CreateMonitorThread();
    m_pMiniThingCore->StartMonitorThread();
}