#include "MonitorThread.h"

using namespace std;

MonitorThread::MonitorThread(HANDLE exitEvent)
{
    m_hExitEvent = exitEvent;
}

BOOL MonitorThread::StartMonitor(VOID* pPrivate)
{
    while (true)
    {
        // �鿴�¼��Ƿ񱻼���
        DWORD dwWaitCode = WaitForSingleObject(m_hExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            cout << "Recv the quit event" << endl;
            break;
        }


    }

    return 0;
}

BOOL MonitorThread::StopMonitor(VOID)
{
    return 0;
}
