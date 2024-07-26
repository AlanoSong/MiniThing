#include "MiniThingQtBackgroud.h"
//#include "MiniThingQt.h"

static MiniThingQtWorkThread* g_pMiniThingQtWorkThread = nullptr;
static void UpdateStatusBarStr(const std::string str)
{
    g_pMiniThingQtWorkThread->UpdateStatusBar(str);
}


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
    g_pMiniThingQtWorkThread = this;
    m_pMiniThingCore->StartInstance(UpdateStatusBarStr);

    m_pMiniThingCore->CreateMonitorThread();
    m_pMiniThingCore->StartMonitorThread();
}