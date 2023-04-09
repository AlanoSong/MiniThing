#pragma once

#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <deque>
#include <vector>
#include <corecrt_wstring.h>
#include <unordered_map>
#include <assert.h>
#include <atlstr.h>

#include "SQLite/sqlite3.h"

using namespace std;

struct UsnInfo
{
    // File reference number
    DWORDLONG pParentRef = 0;
    DWORDLONG pSelfRef = 0;

    // File time stamp
    LARGE_INTEGER timeStamp;

    // File path & name
    wstring fileNameWstr;
    wstring filePathWstr;

    string fileNameStr;
    string filePathStr;
};


class MiniThing
{
public:
    MiniThing(std::wstring volumeName, const char *sqlDBPath);
    ~MiniThing(VOID);

    std::wstring GetVolName(VOID)
    {
        return m_volumeName;
    }

    VOID MonitorFileChange(VOID);
    VOID AdjustUsnRecord(std::wstring folder, std::wstring filePath, std::wstring reFileName, DWORD op);
    BOOL CreateMonitorThread(VOID);
    VOID StartMonitorThread(VOID);
    VOID StopMonitorThread(VOID);


    // For SQLite
    sqlite3* m_hSQLite;
    string m_SQLitePath;
    HRESULT SQLiteOpen(CONST CHAR* path);
    HRESULT SQLiteInsert(UsnInfo * pUsnInfo);
    HRESULT SQLiteQuery(UsnInfo* pUsnInfo);
    HRESULT SQLiteDelete(UsnInfo* pUsnInfo);
    HRESULT SQLiteUpdate(UsnInfo* pUsnInfo, std::wstring originPath);
    HRESULT SQLiteClose(VOID);



    unordered_map<DWORDLONG, UsnInfo> m_usnRecordMap;

    DWORDLONG m_unusedFileRefNum = ((DWORDLONG)(-1)) - 1;


    // Some file always exist in root folder



    // Monitor thread
    HANDLE m_hExitEvent;
    HANDLE m_hMonitorThread;

private:
    std::wstring m_volumeName;
    HANDLE m_hVol = INVALID_HANDLE_VALUE;
    const DWORDLONG m_constFileRefNumMax = ((DWORDLONG)(-1));
    USN_JOURNAL_DATA m_usnInfo;
    vector<DWORDLONG> m_patchOfParentRef = {
        0x000100000000002b,
        // desktop.ini
        0x000100000000002c,
    };

private:
    HRESULT GetHandle(VOID);
    VOID closeHandle(VOID);
    BOOL IsNtfs(VOID);

    VOID GetCurrentFilePath(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef);
    HRESULT CreateUsn(VOID);
    HRESULT QueryUsn(VOID);
    HRESULT RecordUsn(VOID);
    HRESULT SortUsn(VOID);
    HRESULT DeleteUsn(VOID);

    VOID GetSystemError(VOID);
    BOOL IsWstringSame(std::wstring s1, std::wstring s2);
    std::wstring GetFileNameAccordPath(std::wstring path);
    std::wstring GetPathAccordPath(std::wstring path);
};


