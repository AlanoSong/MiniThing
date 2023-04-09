#pragma once

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

class MonitorThread
{
public:
    MonitorThread(HANDLE exitEvent);
    ~MonitorThread(VOID);

    BOOL StartMonitor(VOID* pPrivate);
    BOOL StopMonitor(VOID);

private:
    HANDLE m_hExitEvent;
};