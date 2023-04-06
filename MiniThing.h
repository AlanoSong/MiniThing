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
    MiniThing(string volumeName);
    ~MiniThing(VOID);

    VOID GetSystemError(VOID);
    BOOL IsNtfs(VOID);
    BOOL CreateUsn(VOID);

    BOOL GetHandle(VOID);
    VOID closeHandle(VOID);

    BOOL QueryUsn(VOID);
    BOOL RecordUsn(VOID);
    VOID GetCurrentFilePath(wstring& path, DWORDLONG currentRef, DWORDLONG rootRef);
    VOID SortUsn(VOID);
    VOID DeleteUsn(VOID);

    VOID MonitorFileChange(VOID);
    VOID AdjustUsnRecord(wstring folder, wstring filePath, wstring reFileName, DWORD op);
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
    HRESULT SQLiteModify(UsnInfo* pUsnInfo);
    HRESULT SQLiteClose(VOID);

    string m_volumeName = "F:";

    HANDLE m_hVol = INVALID_HANDLE_VALUE;

    unordered_map<DWORDLONG, UsnInfo> m_usnRecordMap;

    DWORDLONG m_unusedFileRefNum = ((DWORDLONG)(-1)) - 1;

    const DWORDLONG m_constFileRefNumMax = ((DWORDLONG)(-1));

    // Some file always exist in root folder
    vector<DWORDLONG> m_systemParentRef = { 0xb00000000000b, 0x100000000001b, 0x100000000001e, 0x1000000000024, 0x1000000000029, 281474976710698, 0x000100000000002b };

    // Private functions
    LPCWSTR StrToLPCWSTR(string orig);
    LPCSTR StrToLPCSTR(string str);

    CString StrToCstr(string str);
    string LPCWSTRToStr(LPCWSTR lpcwszStr);

    wstring GetFileNameAccordPath(wstring path);
    wstring GetPathAccordPath(wstring path);
    BOOL IsWstringSame(wstring s1, wstring s2);

    // Monitor thread
    HANDLE m_hExitEvent;
    HANDLE m_hMonitorThread;
};


