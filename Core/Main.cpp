// MiniThing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "MiniThing.h"
#include "../Utility/Utility.h"

int main()
{
    LARGE_INTEGER timeStart;
    LARGE_INTEGER timeEnd;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;

    QueryPerformanceCounter(&timeStart);

    std::wstring folderName;
    std::cout << "Which folder ? (eg. C:)" << std::endl;
    std::wcin >> folderName;
    MiniThing* pMiniThing = new MiniThing(folderName, ".\\MiniThing.db");

    QueryPerformanceCounter(&timeEnd);
    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
    std::cout << "Time elapsed : " << elapsed << " S" << std::endl;

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
