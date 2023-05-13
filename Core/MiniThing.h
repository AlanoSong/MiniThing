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
#include <list>

#include "../ThirdParty/SQLite/sqlite3.h"

using namespace std;

#define SORT_TASK_GRANULARITY           ( 1024 )
#define SQL_BATCH_INSERT_GRANULARITY    ( 256 )

class MiniThing;

struct UsnInfo
{
    // File reference number
    DWORDLONG pParentRef = 0;
    DWORDLONG pSelfRef = 0;

    // File time stamp
    LARGE_INTEGER timeStamp;

    // File path & name
    std::wstring fileNameWstr;
    std::wstring filePathWstr;
};

typedef struct
{
    std::wstring volumeName;
    HANDLE hVolume;

    USN_JOURNAL_DATA usnJournalData;
    unordered_map<DWORDLONG, UsnInfo> usnRecordMap;
    DWORDLONG rootFileRef;
    std::wstring rootName;

    HANDLE hMonitor;
    HANDLE hMonitorExitEvent;

    void* pMonitorTaskInfo;
}VolumeInfo;

typedef struct
{
    VolumeInfo* pVolumeInfo;
    MiniThing* pMiniThing;
}MonitorTaskInfo;

typedef struct
{
    UINT taskIndex;
    std::string sqlPath;
    std::wstring rootFolderName;
    DWORDLONG rootRef;
    unordered_map<DWORDLONG, UsnInfo>* pAllUsnRecordMap;
    unordered_map<DWORDLONG, UsnInfo>* pSortTask;
} SortTaskInfo;

typedef struct
{
    void* pVolumeInfo;
    void* pMiniThing;
    DWORD op;
    std::wstring folder;
    std::wstring oriPath;
    std::wstring newPath;
} UpdateDataBaseTaskInfo;

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
    MiniThing(const char* sqlDBPath);
    ~MiniThing(void);

    HRESULT QueryAllVolume(void);
    void AdjustUsnRecord(std::wstring folder, std::wstring filePath, std::wstring reFileName, DWORD op);

    HRESULT CreateMonitorThread(void);
    void StartMonitorThread(void);
    void StopMonitorThread(void);

    HRESULT CreateQueryThread(void);
    void StartQueryThread(void);
    void StopQueryThread(void);

    DWORDLONG GetNewFileRef(void)
    {
        return m_unusedFileRefNum--;
    }
    DWORDLONG GetParentFileRef(void)
    {
        return m_constFileRefNumMax;
    }

    // For SQLite
    sqlite3* m_hSQLite;
    string m_SQLitePath;
    HRESULT SQLiteOpen(CONST CHAR* path);
    HRESULT SQLiteInsert(UsnInfo* pUsnInfo);
    HRESULT SQLiteDelete(UsnInfo* pUsnInfo);
    HRESULT SQLiteUpdate(UsnInfo* pOriInfo, UsnInfo* pNewInfo);
    HRESULT SQLiteClose(void);
    HRESULT SQLiteQuery(std::wstring queryInfo, std::vector<std::wstring>& vec);
    HRESULT SQLiteQueryV2(QueryInfo* queryInfo, std::vector<UsnInfo>& vec);

    BOOL IsSqlExist(void)
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

    // Update sql data base thread
    HANDLE      m_hUpdateSqlDataBaseExitEvent;
    HANDLE      m_hUpdateSqlDataBaseThread;

private:
    std::vector<VolumeInfo> m_volumeSet;

    BOOL                m_isSqlExist;
    HANDLE              m_hVol = INVALID_HANDLE_VALUE;
    const DWORDLONG     m_constFileRefNumMax = ((DWORDLONG)(-1));
    USN_JOURNAL_DATA    m_usnInfo;
    DWORDLONG           m_rootFileNode;

private:
    HRESULT GetAllVolumeHandle(void);
    void closeHandle(void);
    BOOL IsNtfs(std::wstring volName);

    void GetCurrentFilePath(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef);
    void GetCurrentFilePathBySql(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef);
    HRESULT CreateUsn(void);
    HRESULT QueryUsn(void);
    HRESULT RecordUsn(void);
    HRESULT SortUsn(void);
    HRESULT SortVolumeAndUpdateSql(VolumeInfo &volumeInfo);
    HRESULT SortVolumeSetAndUpdateSql(void);
    HRESULT DeleteUsn(void);
};

