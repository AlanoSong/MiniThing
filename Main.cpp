#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "./MiniThing/Core/MiniThingCore.h"
#include <QtWidgets/QApplication>
#include "./MiniThing/Qt/MiniThingQt.h"

//==========================================================================
//                        Static Functions                                //
//==========================================================================
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

//==========================================================================
//                              Main Entry                                //
//==========================================================================
int main(int argc, char *argv[])
{
    // Check if current process run as admin
    //  if not, create a new process run as admin
    //  and exit current process
    if (!IsRunAsAdmin())
    {
        WCHAR path[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, path, MAX_PATH);
        GetAdminPrivileges(path);

        return 0;
    }

    QApplication app(argc, argv);

    MiniThingQt miniThingQt;
    miniThingQt.show();

    return app.exec();
}
