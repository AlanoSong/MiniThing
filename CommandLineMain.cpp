#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "./MiniThing/Core/MiniThingCore.h"

int CommandLineMain(int argc, char *argv[])
{
    LARGE_INTEGER timeStart;
    LARGE_INTEGER timeEnd;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;
    QueryPerformanceCounter(&timeStart);

    MiniThingCore* pMiniThingCore = new MiniThingCore(".\\MiniThing.db");

    QueryPerformanceCounter(&timeEnd);
    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
    printf_s("Time elasped: %f S\n", elapsed);

    if (FAILED(pMiniThingCore->CreateMonitorThread()))
    {
        assert(0);
    }
    if (FAILED(pMiniThingCore->CreateQueryThread()))
    {
        assert(0);
    }
    pMiniThingCore->StartMonitorThread();
    pMiniThingCore->StartQueryThread();

    Sleep(1000 * 1200);

    pMiniThingCore->StopMonitorThread();
    pMiniThingCore->StopQueryThread();

    return 0;
}