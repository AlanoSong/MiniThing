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

static void GetAdminPrivileges(CString strApp, std::wstring args)
{
    SHELLEXECUTEINFO executeInfo;
    memset(&executeInfo, 0, sizeof(executeInfo));
    executeInfo.lpFile = strApp;
    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.lpVerb = _T("runas");
    executeInfo.fMask = SEE_MASK_NO_CONSOLE;
    executeInfo.nShow = SW_SHOWDEFAULT;
    executeInfo.lpParameters = args.c_str();

    ShellExecuteEx(&executeInfo);

    WaitForSingleObject(executeInfo.hProcess, INFINITE);
}

static void CleanRegValue(void)
{
    HKEY hKey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey))
    {
        LSTATUS ret = RegDeleteKey(hKey, L"MiniThing");

        RegCloseKey(hKey);
    }
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

        // Pass down args
        std::wstring args;
        // Pass the first one cause is exe name itself
        for (int i = 1; i < argc; i++)
        {
            std::string tmpStr = argv[i];
            args += StringToWstring(tmpStr);
            args += L" ";
        }
        GetAdminPrivileges(path, args);

        return 0;
    }

    cmdline::parser parser;

    // add specified type of variable.
    // 1st argument is long name
    // 2nd argument is short name (no short name if '\0' specified)
    // 3rd argument is description
    // 4th argument is mandatory (optional. default is false)
    // 5th argument is default value  (optional. it used when mandatory is false)

    // parser.add<std::string>("clean", 'c', "clean app data", false, "false", cmdline::oneof<std::string>("true", "false"));
    parser.add("clean", 'c', "clean app data");
    // Parse command line
    parser.parse_check(argc, argv);

    if (parser.exist("clean"))
    {
        CleanRegValue();
        return 0;
    }

    QApplication app(argc, argv);

    MiniThingQt miniThingQt;
    miniThingQt.show();

    return app.exec();
}
