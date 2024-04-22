#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "./MiniThing/Core/MiniThingCore.h"

#ifdef BUILD_FOR_QT

#include <QtWidgets/QApplication>
#include "./MiniThing/Qt/MiniThingQt.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MiniThingQt miniThingQt;
    miniThingQt.show();

    return app.exec();
}

#else // BUILD_FOR_QT

int main(int argc, char* argv[])
{
    LARGE_INTEGER timeStart;
    LARGE_INTEGER timeEnd;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double quadpart = (double)frequency.QuadPart;
    QueryPerformanceCounter(&timeStart);

    MiniThingCore* pMiniThingCore = new MiniThingCore();
    pMiniThingCore->StartInstance();

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

#endif // BUILD_FOR_QT