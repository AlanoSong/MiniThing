// MiniThing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "MiniThing.h"

int main()
{
    LARGE_INTEGER timeStart;
    LARGE_INTEGER timeEnd;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;
    QueryPerformanceCounter(&timeStart);

    MiniThing* pMiniThing = new MiniThing(".\\MiniThing.db");

    QueryPerformanceCounter(&timeEnd);
    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
    printf_s("Time elasped: %f S\n", elapsed);

    if (FAILED(pMiniThing->CreateMonitorThread()))
    {
        assert(0);
    }
    if (FAILED(pMiniThing->CreateQueryThread()))
    {
        assert(0);
    }
    pMiniThing->StartMonitorThread();
    pMiniThing->StartQueryThread();

    Sleep(1000 * 1200);

    pMiniThing->StopMonitorThread();
    pMiniThing->StopQueryThread();
}