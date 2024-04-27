#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "./MiniThing/Core/MiniThingCore.h"

#ifdef BUILD_FOR_QT
#include <QtWidgets/QApplication>
#include "./MiniThing/Qt/MiniThingQt.h"
#endif

static bool IsRunAsAdmin(void)
{
    BOOL isRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdminGroup = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdminGroup))
    {
        dwError = GetLastError();
        goto Exit;
    }

    if (!CheckTokenMembership(NULL, pAdminGroup, &isRunAsAdmin))
    {
        dwError = GetLastError();
        goto Exit;
    }

Exit:
    if (pAdminGroup)
    {
        FreeSid(pAdminGroup);
        pAdminGroup = NULL;
    }

    if (ERROR_SUCCESS != dwError)
    {
        throw dwError;
    }

    return isRunAsAdmin;
}

static void GetAdminPrivileges(CString strApp)
{
    SHELLEXECUTEINFO executeInfo;
    memset(&executeInfo, 0, sizeof(executeInfo));
    executeInfo.lpFile = strApp;
    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.lpVerb = _T("runas");
    executeInfo.fMask = SEE_MASK_NO_CONSOLE;
    executeInfo.nShow = SW_SHOWDEFAULT;

    ShellExecuteEx(&executeInfo);

    WaitForSingleObject(executeInfo.hProcess, INFINITE);
}

int main(int argc, char *argv[])
{
    // Check if current app run as admin
    //  if not, create a new app run as admin
    //  and exit current app
    if (!IsRunAsAdmin())
    {
        WCHAR path[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, path, MAX_PATH);
        GetAdminPrivileges(path);

        return 0;
    }

#ifdef BUILD_FOR_QT
    QApplication app(argc, argv);

    MiniThingQt miniThingQt;
    miniThingQt.show();

    return app.exec();
#else
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
#endif
}
