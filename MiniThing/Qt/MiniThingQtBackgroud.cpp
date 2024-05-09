#include "MiniThingQtBackgroud.h"
//#include "MiniThingQt.h"


MiniThingQtWorkThread::MiniThingQtWorkThread()
{
}

MiniThingQtWorkThread::~MiniThingQtWorkThread()
{
    // Terminate monitor thread, and update sql base thread within it
    m_pMiniThingCore->StopMonitorThread();

    // Close sqlite database before return
    m_pMiniThingCore->SQLiteClose();
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