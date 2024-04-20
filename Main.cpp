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

    // Get user appdata directory, and we will save and get database under this directory
    wchar_t appDataPath[MAX_PATH];
    SHGetSpecialFolderPath(0, appDataPath, CSIDL_APPDATA, false);
    std::wstring dataBasePath(appDataPath);
    dataBasePath += L"\\MiniThing";

    // Check if directory exist
    if (_access(WstringToString(dataBasePath).c_str(), 0) == -1)
    {
        int ret = mkdir(WstringToString(dataBasePath).c_str());
        assert(ret == 0);
    }

    dataBasePath += L"\\MiniThing.db";

    MiniThingCore* pMiniThingCore = new MiniThingCore(WstringToString(dataBasePath).c_str());
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