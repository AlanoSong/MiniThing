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

#include "TaskThreads.h"
#include "../Utility/Utility.h"
#include "../ThirdParty/SQLite/sqlite3.h"

using namespace std;

#define SQL_BATCH_INSERT_GRANULARITY    ( 4096 )

class MiniThingCore;

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
} VolumeInfo;

typedef enum _QUERY_TYPE
{
    QUERY_BY_NAME = 0,
    QUERY_BY_REF,
    QUERY_BY_PREPATH,
} QUERY_TYPE;

typedef struct _QueryInfo
{
    QUERY_TYPE type;
    UsnInfo info;
} QueryInfo;

typedef struct
{
    VolumeInfo* pVolumeInfo;
    MiniThingCore* pMiniThingCore;
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
    void* pMiniThingCore;
    DWORD op;
    std::wstring folder;
    std::wstring oriPath;
    std::wstring newPath;
} UpdateDataBaseTaskInfo;

class MiniThingCore
{
public:
    MiniThingCore() {};
    MiniThingCore(const char* sqlDbPath);
    ~MiniThingCore(void);

public:
    // System related functions
    HRESULT StartInstance(void);
    HRESULT QueryAllVolume(void);
    HRESULT GetAllVolumeHandle(void);
    void CloseVolumeHandle(void);
    bool IsNtfs(std::wstring volName);
    bool IsSqlExist(void) { return m_isSqlExist; }
    bool IsCoreReady(void) { return m_isCoreReady; };

public:
    // Monitor thread related parameters
    HANDLE      m_hExitEvent;
    HANDLE      m_hMonitorThread;

    // Monitor thread related functions
    HRESULT CreateMonitorThread(void);
    void StartMonitorThread(void);
    void StopMonitorThread(void);

public:
    // Query thread related parameters
    HANDLE      m_hQueryExitEvent;
    HANDLE      m_hQueryThread;

    // Query thread thread related functions
    HRESULT CreateQueryThread(void);
    void StartQueryThread(void);
    void StopQueryThread(void);

public:
    // Sqlite data base related paremeters
    HANDLE      m_hUpdateSqlDataBaseExitEvent;
    HANDLE      m_hUpdateSqlDataBaseThread;

    // Sqlite data base related functions
    HRESULT SortVolumeAndUpdateSql(VolumeInfo& volumeInfo);
    HRESULT SortVolumeSetAndUpdateSql(void);

    HRESULT SQLiteOpen(void);
    HRESULT SQLiteInsert(UsnInfo* pUsnInfo);
    HRESULT SQLiteDelete(UsnInfo* pUsnInfo);
    HRESULT SQLiteUpdate(UsnInfo* pOriInfo, UsnInfo* pNewInfo);
    HRESULT SQLiteQuery(std::wstring queryInfo, std::vector<std::wstring>& vec);
    HRESULT SQLiteQueryV2(QueryInfo* queryInfo, std::vector<UsnInfo>& vec);
    HRESULT SQLiteClose(void);

private:
    // Usn related functions
    HRESULT CreateUsn(void);
    HRESULT QueryUsn(void);
    HRESULT RecordUsn(void);
    HRESULT DeleteUsn(void);

private:
    std::vector<VolumeInfo>             m_volumeSet;
    unordered_map<DWORDLONG, UsnInfo>   m_usnRecordMap;
    sqlite3                             *m_hSql;
    std::string                         m_sqlDbPath;
    bool                                m_isSqlExist;
    bool                                m_isCoreReady;
};

