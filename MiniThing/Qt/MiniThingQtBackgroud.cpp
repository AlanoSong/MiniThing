#include "MiniThingQtBackgroud.h"
//#include "MiniThingQt.h"


MiniThingQtWorkThread::MiniThingQtWorkThread()
{
}

MiniThingQtWorkThread::~MiniThingQtWorkThread()
{
}


MiniThingQtWorkThread::MiniThingQtWorkThread(MiniThingCore* pMiniThingCore)
{
    m_pMiniThingCore = pMiniThingCore;
}

void MiniThingQtWorkThread::run()
{
    m_pMiniThingCore->StartInstance();

    m_pMiniThingCore->CreateMonitorThread();
    m_pMiniThingCore->StartMonitorThread();
}