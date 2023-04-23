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
#include <thread>

#include "../SQLite/sqlite3.h"

using namespace std;

#define SORT_TASK_GRANULARITY           ( 1024 )
#define SQL_BATCH_INSERT_GRANULARITY    ( 256 )

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
};

typedef struct
{
    UINT taskIndex;
    std::string sqlPath;
    std::wstring rootFolderName;
    DWORDLONG rootRef;
    unordered_map<DWORDLONG, UsnInfo>* pAllUsnRecordMap;
    unordered_map<DWORDLONG, UsnInfo>* pSortTask;
} SortTaskInfo;

typedef enum _QUERY_TYPE
{
    BY_NAME = 0,
    BY_REF,
    BY_PREPATH,
}QUERY_TYPE;

typedef struct _QueryInfo
{
    QUERY_TYPE type;
    UsnInfo info;
}QueryInfo;

class MiniThing
{
public:
    MiniThing(std::wstring volumeName, const char* sqlDBPath);
    ~MiniThing(VOID);

    std::wstring GetVolName(VOID)
    {
        return m_volumeName;
    }

    VOID AdjustUsnRecord(std::wstring folder, std::wstring filePath, std::wstring reFileName, DWORD op);

    HRESULT CreateMonitorThread(VOID);
    VOID StartMonitorThread(VOID);
    VOID StopMonitorThread(VOID);

    HRESULT CreateQueryThread(VOID);
    VOID StartQueryThread(VOID);
    VOID StopQueryThread(VOID);

    // For SQLite
    sqlite3* m_hSQLite;
    string m_SQLitePath;
    HRESULT SQLiteOpen(CONST CHAR* path);
    HRESULT SQLiteInsert(UsnInfo* pUsnInfo);
    HRESULT SQLiteDelete(UsnInfo* pUsnInfo);
    HRESULT SQLiteUpdate(UsnInfo* pUsnInfo, std::wstring originPath);
    HRESULT SQLiteUpdateV2(UsnInfo* pOriInfo, UsnInfo* pNewInfo);
    HRESULT SQLiteClose(VOID);
    HRESULT SQLiteQuery(std::wstring queryInfo, std::vector<std::wstring>& vec);
    HRESULT SQLiteQueryV2(QueryInfo* queryInfo, std::vector<UsnInfo>& vec);

    BOOL IsWstringSame(std::wstring s1, std::wstring s2);
    BOOL IsSubStr(std::wstring s1, std::wstring s2);

    BOOL IsSqlExist(VOID)
    {
        return m_isSqlExist;
    }

    unordered_map<DWORDLONG, UsnInfo>   m_usnRecordMap;

    DWORDLONG   m_unusedFileRefNum = ((DWORDLONG)(-1)) - 1;

    // Monitor thread
    HANDLE      m_hExitEvent;
    HANDLE      m_hMonitorThread;

    // Query thread
    HANDLE      m_hQueryExitEvent;
    HANDLE      m_hQueryThread;

private:
    std::wstring        m_volumeName;
    BOOL                m_isSqlExist;
    HANDLE              m_hVol = INVALID_HANDLE_VALUE;
    const DWORDLONG     m_constFileRefNumMax = ((DWORDLONG)(-1));
    USN_JOURNAL_DATA    m_usnInfo;
    DWORDLONG           m_rootFileNode;

private:
    HRESULT GetHandle(VOID);
    VOID closeHandle(VOID);
    BOOL IsNtfs(VOID);

    VOID GetCurrentFilePath(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef);
    VOID GetCurrentFilePathBySql(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef);
    HRESULT CreateUsn(VOID);
    HRESULT QueryUsn(VOID);
    HRESULT RecordUsn(VOID);
    HRESULT SortUsn(VOID);
    HRESULT DeleteUsn(VOID);

    VOID GetSystemError(VOID);

    std::wstring GetFileNameAccordPath(std::wstring path);
    std::wstring GetPathAccordPath(std::wstring path);
};

