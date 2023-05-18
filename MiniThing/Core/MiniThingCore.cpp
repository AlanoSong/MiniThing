#include "MiniThingCore.h"

//==========================================================================
//                        Static Functions                                //
//==========================================================================

//==========================================================================
//                        Public Parameters                               //
//==========================================================================
extern std::list<UpdateDataBaseTaskInfo> g_updateDataBaseTaskList;
extern HANDLE g_updateDataBaseWrMutex;

//==========================================================================
//                        MiniThing Functions                             //
//==========================================================================

MiniThingCore::MiniThingCore(const char* sqlDbPath)
{
    m_sqlDbPath = sqlDbPath;
}

MiniThingCore::~MiniThingCore(void)
{
    if (FAILED(SQLiteClose()))
    {
        assert(0);
    }
}

HRESULT MiniThingCore::StartInstance(void)
{
    HRESULT ret = S_OK;

    SetChsPrintEnv();

    if (FAILED(QueryAllVolume()))
    {
        assert(0);
    }

    if (FAILED(GetAllVolumeHandle()))
    {
        assert(0);
    }

    // Create or open sql data base file
    if (FAILED(SQLiteOpen()))
    {
        assert(0);
    }

    if (!IsSqlExist())
    {
        // Create system usn info
        if (FAILED(CreateUsn()))
        {
            assert(0);
        }

        if (FAILED(QueryUsn()))
        {
            assert(0);
        }

        if (FAILED(RecordUsn()))
        {
            assert(0);
        }

        if (FAILED(SortVolumeSetAndUpdateSql()))
        {
            assert(0);
        }

        if (FAILED(DeleteUsn()))
        {
            assert(0);
        }
    }

    CloseVolumeHandle();

    return ret;
}

HRESULT MiniThingCore::QueryAllVolume(void)
{
    HRESULT ret = S_OK;
    // Get all volume
    wchar_t szLogicDriveStrings[1024];
    wchar_t* szDrive;
    ZeroMemory(szLogicDriveStrings, 1024);

    GetLogicalDriveStrings(1024 - 1, szLogicDriveStrings);
    szDrive = (wchar_t*)szLogicDriveStrings;

    while (*szDrive)
    {
        VolumeInfo volInfo;
        std::wstring name = szDrive;
        volInfo.volumeName = name.substr(0, name.length() - 1);

        if (IsNtfs(volInfo.volumeName))
        {
            m_volumeSet.push_back(volInfo);
        }

        szDrive += (_tcslen(szDrive) + 1);
    }

    return ret;
}

HRESULT MiniThingCore::GetAllVolumeHandle()
{
    HRESULT ret = S_OK;

    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        std::wstring folderPath = L"\\\\.\\";
        folderPath += it->volumeName;

        it->hVolume = CreateFile(folderPath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr);

        if (INVALID_HANDLE_VALUE == it->hVolume)
        {
            printf_s("Get handle failed : %s\n", WstringToString(it->volumeName).c_str());
            GetSystemError();
            ret = E_FAIL;
            break;
        }
    }

    return ret;
}

void MiniThingCore::CloseVolumeHandle(void)
{
    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        CloseHandle(it->hVolume);
        it->hVolume = INVALID_HANDLE_VALUE;
    }
}

bool MiniThingCore::IsNtfs(std::wstring volName)
{
    bool isNtfs = false;
    char sysNameBuf[MAX_PATH] = { 0 };
    char sysNameBuf2[MAX_PATH] = { 0 };

    int len = WstringToChar(volName, nullptr);
    char* pVol = new char[len];
    WstringToChar(volName, pVol);

    std::wstring volName2 = volName + L"\\";
    len = WstringToChar(volName2, nullptr);
    char* pVol2 = new char[len];
    WstringToChar(volName2, pVol2);

    bool status = GetVolumeInformationA(
        pVol,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        sysNameBuf,
        MAX_PATH);

    // Sometimes if pVol = 'X:', GetVolumeInformationA() not success,
    //      so we try with 'X:\' again.
    bool status2 = GetVolumeInformationA(
        pVol2,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        sysNameBuf2,
        MAX_PATH);

    if (status)
    {
        if (0 == strcmp(sysNameBuf, "NTFS"))
        {
            isNtfs = true;
        }
    }
    else if (status2)
    {
        if (0 == strcmp(sysNameBuf2, "NTFS"))
        {
            isNtfs = true;
        }
    }
    else
    {
        GetSystemError();
    }

    delete[] pVol;
    delete[] pVol2;

    return isNtfs;
}

HRESULT MiniThingCore::CreateUsn(void)
{
    HRESULT ret = S_OK;

    DWORD br;
    CREATE_USN_JOURNAL_DATA cujd;
    cujd.MaximumSize = 0;
    cujd.AllocationDelta = 0;

    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        bool status = DeviceIoControl(
            it->hVolume,
            FSCTL_CREATE_USN_JOURNAL,
            &cujd,
            sizeof(cujd),
            NULL,
            0,
            &br,
            NULL);

        if (false == status)
        {
            GetSystemError();
            ret = E_FAIL;

            break;
        }
    }

    return ret;
}

HRESULT MiniThingCore::QueryUsn(void)
{
    HRESULT ret = S_OK;

    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        DWORD br;
        USN_JOURNAL_DATA usnJournalData;

        bool status = DeviceIoControl(it->hVolume,
            FSCTL_QUERY_USN_JOURNAL,
            NULL,
            0,
            &usnJournalData,
            sizeof(usnJournalData),
            &br,
            NULL);

        if (false == status)
        {
            ret = E_FAIL;
            GetSystemError();

            break;
        }
        else
        {
            it->usnJournalData = usnJournalData;
        }
    }

    return ret;
}

HRESULT MiniThingCore::RecordUsn(void)
{
    printf_s("Record usn ...\n");

    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        MFT_ENUM_DATA med = { 0, 0, it->usnJournalData.NextUsn };
        med.MaxMajorVersion = 2;
        // Used to record usn info, must big enough
        char buffer[0x1000];
        DWORD usnDataSize = 0;
        PUSN_RECORD pUsnRecord;

        // Find the first USN record
        // return a USN followed by zero or more change journal records, each in a USN_RECORD structure
        while (false != DeviceIoControl(it->hVolume,
            FSCTL_ENUM_USN_DATA,
            &med,
            sizeof(med),
            buffer,
            _countof(buffer),
            &usnDataSize,
            NULL))
        {
            DWORD dwRetBytes = usnDataSize - sizeof(USN);
            pUsnRecord = (PUSN_RECORD)(((PCHAR)buffer) + sizeof(USN));
            DWORD cnt = 0;

            while (dwRetBytes > 0)
            {
                // Here FileNameLength may count in bytes, and each wchar_t occupy 2 bytes
                wchar_t* pWchar = new wchar_t[pUsnRecord->FileNameLength / 2 + 1];
                memcpy(pWchar, pUsnRecord->FileName, pUsnRecord->FileNameLength);
                pWchar[pUsnRecord->FileNameLength / 2] = 0x00;

                std::wstring fileNameWstr = WcharToWstring(pWchar);
                delete[] pWchar;

                UsnInfo usnInfo = { 0 };
                usnInfo.fileNameWstr = fileNameWstr;
                usnInfo.pParentRef = pUsnRecord->ParentFileReferenceNumber;
                usnInfo.pSelfRef = pUsnRecord->FileReferenceNumber;
                usnInfo.timeStamp = pUsnRecord->TimeStamp;

                it->usnRecordMap[usnInfo.pSelfRef] = usnInfo;

                // Get the next USN record
                DWORD recordLen = pUsnRecord->RecordLength;
                dwRetBytes -= recordLen;
                pUsnRecord = (PUSN_RECORD)(((PCHAR)pUsnRecord) + recordLen);
            }

            // Get next page USN record
            med.StartFileReferenceNumber = *(USN*)&buffer;
        }
    }

    return S_OK;
}

HRESULT MiniThingCore::SortVolumeSetAndUpdateSql(void)
{
    HRESULT ret = S_OK;
    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        SortVolumeAndUpdateSql(*it);
        it->usnRecordMap.clear();
    }
    return ret;
}

HRESULT MiniThingCore::SortVolumeAndUpdateSql(VolumeInfo& volume)
{
    HRESULT ret = S_OK;

    printf_s("Build sql data base ...\n");

    // 1. Get root file node
    //  cause "System Volume Information" is a system file and just under root folder
    //  so we use it to find root folder
    std::wstring cmpStr(L"System Volume Information");

    volume.rootFileRef = 0;

    for (auto ite = volume.usnRecordMap.begin(); ite != volume.usnRecordMap.end(); ite++)
    {
        UsnInfo usnInfo = ite->second;
        if (0 == usnInfo.fileNameWstr.compare(cmpStr))
        {
            volume.rootFileRef = usnInfo.pParentRef;
            break;
        }
    }

    if (volume.rootFileRef == 0)
    {
        printf_s("Cannot find root folder\n");
        assert(0);
    }

    volume.rootName = volume.volumeName;

    // 2. Get suitable thread task granularity
    int hwThreadNum = std::thread::hardware_concurrency() * 2;
    int granularity = (int)volume.usnRecordMap.size() / hwThreadNum;
    int step = 100;
    while (granularity * hwThreadNum < volume.usnRecordMap.size())
    {
        granularity += 100;
    }

    // 3. Divide file node into several sort tasks
    int fileNodeCnt = 0;
    vector<unordered_map<DWORDLONG, UsnInfo>> sortTaskSet;
    unordered_map<DWORDLONG, UsnInfo> mapTmp;

    for (auto it = volume.usnRecordMap.begin(); it != volume.usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        mapTmp[it->first] = it->second;
        ++fileNodeCnt;

        if (fileNodeCnt % granularity == 0)
        {
            sortTaskSet.push_back(mapTmp);
            mapTmp.clear();
        }
        else
        {
            continue;
        }
    }
    if (!mapTmp.empty())
    {
        fileNodeCnt += (int)mapTmp.size();
        sortTaskSet.push_back(mapTmp);
        mapTmp.clear();
    }

    vector<HANDLE> taskHandleVec;
    vector<SortTaskInfo> sortTaskVec;

    for (int i = 0; i < sortTaskSet.size(); i++)
    {
        SortTaskInfo taskInfo;
        taskInfo.taskIndex = i;
        taskInfo.rootFolderName = volume.rootName;
        taskInfo.sqlPath = m_sqlDbPath;
        taskInfo.pSortTask = &(sortTaskSet[i]);
        taskInfo.pAllUsnRecordMap = &(volume.usnRecordMap);
        taskInfo.rootRef = volume.rootFileRef;

        sortTaskVec.push_back(taskInfo);
    }

    // 4. Execute all sort tasks by threads
    for (int i = 0; i < sortTaskSet.size(); i++)
    {
        HANDLE taskThread = CreateThread(0, 0, SortThread, &(sortTaskVec[i]), CREATE_SUSPENDED, 0);
        if (taskThread)
        {
            ResumeThread(taskThread);
            taskHandleVec.push_back(taskThread);
        }
        else
        {
            assert(0);
        }
    }

    // 5. Wait all sort thread over
    for (auto it = taskHandleVec.begin(); it != taskHandleVec.end(); it++)
    {
        DWORD dwWaitCode = WaitForSingleObject(*it, INFINITE);
        assert(dwWaitCode == WAIT_OBJECT_0);
        CloseHandle(*it);
    }

    printf_s("Insert begin ...\n");
    // 6. Write all file node info into sqlite
    char* errMsg = nullptr;
    int insertNodeCnt = 0;
    char sql[1024] = { 0 };

    sqlite3_exec(m_hSql, "BEGIN", NULL, NULL, &errMsg);

    sprintf_s(sql, "INSERT INTO UsnInfo (SelfRef, ParentRef, TimeStamp, FileName, FilePath) VALUES(?, ?, ?, ?, ?);");
    sqlite3_stmt* pPrepare = nullptr;
    sqlite3_prepare_v2(m_hSql, sql, (int)strlen(sql), &pPrepare, 0);

    for (auto it = sortTaskSet.begin(); it != sortTaskSet.end(); it++)
    {
        for (auto i = (*it).begin(); i != (*it).end(); i++)
        {
            ++insertNodeCnt;
            UsnInfo usnInfo = i->second;

            std::string nameUtf8 = UnicodeToUtf8(usnInfo.fileNameWstr);
            std::string pathUtf8 = UnicodeToUtf8(usnInfo.filePathWstr);

            // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
            sqlite3_reset(pPrepare);
            sqlite3_bind_int64(pPrepare, 1, usnInfo.pSelfRef);
            sqlite3_bind_int64(pPrepare, 2, usnInfo.pParentRef);
            // sqlite3_bind_int64(pPrepare, 3, (sqlite_int64)usnInfo.timeStamp);
            sqlite3_bind_text(pPrepare, 4, nameUtf8.c_str(), (int)strlen(nameUtf8.c_str()), nullptr);
            sqlite3_bind_text(pPrepare, 5, pathUtf8.c_str(), (int)strlen(pathUtf8.c_str()), nullptr);
            int ret = sqlite3_step(pPrepare);
            assert(ret == SQLITE_DONE);

            if (insertNodeCnt % SQL_BATCH_INSERT_GRANULARITY == 0)
            {
                sqlite3_exec(m_hSql, "COMMIT", NULL, NULL, &errMsg);
                sqlite3_exec(m_hSql, "BEGIN", NULL, NULL, &errMsg);
            }
        }
    }
    sqlite3_exec(m_hSql, "COMMIT", NULL, NULL, &errMsg);
    sqlite3_finalize(pPrepare);

    // 7. After file node sorted, the map can be destroyed
    taskHandleVec.clear();
    sortTaskVec.clear();
    sortTaskSet.clear();

    wprintf_s(L"%s file node: %d\n", volume.volumeName.c_str(), fileNodeCnt);

    return ret;
}

HRESULT MiniThingCore::DeleteUsn(void)
{
    HRESULT ret = S_OK;

    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        DELETE_USN_JOURNAL_DATA dujd;
        dujd.UsnJournalID = it->usnJournalData.UsnJournalID;
        dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;
        DWORD lenRet = 0;

        int status = DeviceIoControl(it->hVolume,
            FSCTL_DELETE_USN_JOURNAL,
            &dujd,
            sizeof(dujd),
            NULL,
            0,
            &lenRet,
            NULL);

        if (false == status)
        {
            printf_s("Delete usn file failed\n");
            GetSystemError();
            ret = E_FAIL;
        }
    }

    return ret;
}

HRESULT MiniThingCore::CreateMonitorThread(void)
{
    HRESULT ret = S_OK;

    g_updateDataBaseTaskList.clear();
    g_updateDataBaseWrMutex = CreateMutex(nullptr, 0, nullptr);
    assert(g_updateDataBaseWrMutex);

    m_hUpdateSqlDataBaseExitEvent = CreateEvent(0, 0, 0, 0);
    assert(m_hUpdateSqlDataBaseExitEvent);

    m_hUpdateSqlDataBaseThread = CreateThread(0, 0, UpdateSqlDataBaseThread, this, 0, 0);
    assert(m_hUpdateSqlDataBaseThread);

    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        it->hMonitorExitEvent = CreateEvent(0, 0, 0, 0);
        it->pMonitorTaskInfo = new MonitorTaskInfo;
        MonitorTaskInfo* pTask = (MonitorTaskInfo*)it->pMonitorTaskInfo;
        pTask->pMiniThingCore = this;
        pTask->pVolumeInfo = &(*it);

        it->hMonitor = CreateThread(0, 0, MonitorThread, it->pMonitorTaskInfo, 0, 0);
        if (INVALID_HANDLE_VALUE == it->hMonitor)
        {
            GetSystemError();
            ret = E_FAIL;
        }
    }

    return ret;
}

void MiniThingCore::StartMonitorThread(void)
{
}

void MiniThingCore::StopMonitorThread(void)
{
    for (auto it = m_volumeSet.begin(); it != m_volumeSet.end(); it++)
    {
        SetEvent(it->hMonitorExitEvent);
        DWORD dwWaitCode = WaitForSingleObject(it->hMonitor, INFINITE);
        assert(dwWaitCode == WAIT_OBJECT_0);

        CloseHandle(it->hMonitor);

        delete it->pMonitorTaskInfo;
    }

    SetEvent(m_hUpdateSqlDataBaseExitEvent);
    DWORD dwWaitCode = WaitForSingleObject(m_hUpdateSqlDataBaseThread, INFINITE);
    assert(dwWaitCode == WAIT_OBJECT_0);
}

HRESULT MiniThingCore::CreateQueryThread(void)
{
    HRESULT ret = S_OK;

    m_hQueryExitEvent = CreateEvent(0, 0, 0, 0);
    m_hQueryThread = CreateThread(0, 0, QueryThread, this, CREATE_SUSPENDED, 0);
    if (INVALID_HANDLE_VALUE == m_hQueryThread)
    {
        GetSystemError();
        ret = E_FAIL;
    }

    return ret;
}

void MiniThingCore::StartQueryThread(void)
{
    ResumeThread(m_hQueryThread);
}

void MiniThingCore::StopQueryThread(void)
{
    SetEvent(m_hQueryExitEvent);
    DWORD dwWaitCode = WaitForSingleObject(m_hQueryThread, INFINITE);
    assert(dwWaitCode == WAIT_OBJECT_0);

    CloseHandle(m_hQueryThread);
}

HRESULT MiniThingCore::SQLiteOpen(void)
{
    HRESULT ret = S_OK;

    // TODO: if m_SQLitePath contain wstring ?
    assert(!m_sqlDbPath.empty());
    int result = sqlite3_open_v2(m_sqlDbPath.c_str(), &m_hSql, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, NULL);

    // Turn off sqlite write sync
    char* errMsg = nullptr;
    sqlite3_exec(m_hSql, "PRAGMA synchronous = OFF", NULL, NULL, &errMsg);

    if (result == SQLITE_OK)
    {
        char* errMsg = nullptr;

        std::string create;
        create.append("CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);");

        int res = sqlite3_exec(m_hSql, create.c_str(), 0, 0, &errMsg);
        if (res == SQLITE_OK)
        {
            m_isSqlExist = false;
        }
        else
        {
            printf_s("Sqlite: data base already exist\n");
            m_isSqlExist = true;
        }
    }
    else
    {
        ret = E_FAIL;
        printf_s("Sqlite: %s\n", errMsg);
    }

    return ret;
}

HRESULT MiniThingCore::SQLiteInsert(UsnInfo* pUsnInfo)
{
    HRESULT ret = S_OK;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    char sql[1024] = { 0 };
    std::string nameUtf8 = UnicodeToUtf8(pUsnInfo->fileNameWstr);
    std::string pathUtf8 = UnicodeToUtf8(pUsnInfo->filePathWstr);
    sprintf_s(sql, "INSERT INTO UsnInfo VALUES(%llu, %llu, %lld, '%s', '%s');",
        pUsnInfo->pSelfRef, pUsnInfo->pParentRef, pUsnInfo->timeStamp, nameUtf8.c_str(), pathUtf8.c_str());

    char* errMsg = nullptr;
    if (sqlite3_exec(m_hSql, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf_s("sqlite : insert failed\n");
        printf_s("error : %s\n", errMsg);
    }
    else
    {
        // printf_s("sqlite : insert done\n");
    }

    return ret;
}

HRESULT MiniThingCore::SQLiteQuery(std::wstring queryInfo, std::vector<std::wstring>& vec)
{
    HRESULT ret = S_OK;
    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    // 1. Conveter unicode -> utf-8
    std::string str = UnicodeToUtf8(queryInfo);

    // 2. Query and pop vector
    sprintf_s(sql, "SELECT * FROM UsnInfo WHERE FileName LIKE '%%%s%%';", str.c_str());

    sqlite3_stmt* stmt = NULL;
    int res = sqlite3_prepare_v2(m_hSql, sql, (int)strlen(sql), &stmt, NULL);

    if (SQLITE_OK == res && NULL != stmt)
    {
        // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
        while (SQLITE_ROW == sqlite3_step(stmt))
        {
            const unsigned char* pName = sqlite3_column_text(stmt, 3);
            const unsigned char* pPath = sqlite3_column_text(stmt, 4);
            std::string strName = (char*)pName;
            std::string strPath = (char*)pPath;

            std::wstring wstrName = Utf8ToUnicode(strName);
            std::wstring wstrPath = Utf8ToUnicode(strPath);

            vec.push_back(wstrPath);
        }
    }
    else
    {
        ret = E_FAIL;
        printf_s("sqlite : query failed\n");
    }

    sqlite3_finalize(stmt);

    return ret;
}

HRESULT MiniThingCore::SQLiteQueryV2(QueryInfo* queryInfo, std::vector<UsnInfo>& vec)
{
    HRESULT ret = S_OK;
    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    if (queryInfo)
    {
        switch (queryInfo->type)
        {
        case QUERY_BY_REF:
        {
            sprintf_s(sql, "SELECT * FROM UsnInfo WHERE SelfRef = %llu;", queryInfo->info.pSelfRef);
            break;
        }
        case QUERY_BY_NAME:
        {
            std::string str = UnicodeToUtf8(queryInfo->info.fileNameWstr);
            sprintf_s(sql, "SELECT * FROM UsnInfo WHERE FileName LIKE '%%%s%%';", str.c_str());
            break;
        }
        case QUERY_BY_PREPATH:
        {
            std::string str = UnicodeToUtf8(queryInfo->info.filePathWstr);
            sprintf_s(sql, "SELECT * FROM UsnInfo WHERE FilePath LIKE '%s\\%%';", str.c_str());
            break;
        }
        default:
            assert(0);
            break;
        }
    }
    else
    {
        sprintf_s(sql, "SELECT * FROM UsnInfo;");
    }

    sqlite3_stmt* stmt = NULL;
    int res = sqlite3_prepare_v2(m_hSql, sql, (int)strlen(sql), &stmt, NULL);
    if (SQLITE_OK == res && NULL != stmt)
    {
        // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
        while (SQLITE_ROW == sqlite3_step(stmt))
        {
            DWORDLONG self = sqlite3_column_int64(stmt, 0);
            DWORDLONG parent = sqlite3_column_int64(stmt, 1);
            const unsigned char* pName = sqlite3_column_text(stmt, 3);
            const unsigned char* pPath = sqlite3_column_text(stmt, 4);
            std::string strName = (char*)pName;
            std::string strPath = (char*)pPath;

            std::wstring wstrName = Utf8ToUnicode(strName);
            std::wstring wstrPath = Utf8ToUnicode(strPath);

            UsnInfo usnInfo = { 0 };
            usnInfo.pSelfRef = self;
            usnInfo.pParentRef = parent;
            usnInfo.fileNameWstr = wstrName;
            usnInfo.filePathWstr = wstrPath;

            vec.push_back(usnInfo);
        }
    }
    else
    {
        ret = E_FAIL;
        printf_s("sqlite : query failed\n");
    }

    sqlite3_finalize(stmt);

    return ret;
}

HRESULT MiniThingCore::SQLiteDelete(UsnInfo* pUsnInfo)
{
    HRESULT ret = S_OK;

    // Update sql db
    char sql[2048] = { 0 };
    memset(sql, 0x00, 2048);
    char* errMsg = nullptr;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    std::string path = UnicodeToUtf8(pUsnInfo->filePathWstr);
    sprintf_s(sql, "DELETE FROM UsnInfo WHERE FilePath = '%s';", path.c_str());
    if (sqlite3_exec(m_hSql, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf_s("sqlite : delete failed\n");
        printf_s("error : %s\n", errMsg);
    }
    else
    {
        // printf_s("sqlite : delete done\n");
    }

    char sql1[2048] = { 0 };
    memset(sql1, 0x00, 2048);
    sprintf_s(sql1, "DELETE FROM UsnInfo WHERE FilePath LIKE '%s\\%%';", path.c_str());
    if (sqlite3_exec(m_hSql, sql1, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf_s("sqlite : delete failed\n");
        printf_s("error : %s\n", errMsg);
    }
    else
    {
        // printf_s("sqlite : delete done\n");
    }

    return ret;
}

HRESULT MiniThingCore::SQLiteUpdate(UsnInfo* pOriInfo, UsnInfo* pNewInfo)
{
    HRESULT ret = S_OK;

    char sql[2048];
    memset(sql, 0x00, 2048);
    char* errMsg = nullptr;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    std::string oriPath = UnicodeToUtf8(pOriInfo->filePathWstr);
    std::string oriName = UnicodeToUtf8(pOriInfo->fileNameWstr);

    std::string newPath = UnicodeToUtf8(pNewInfo->filePathWstr);
    std::string newName = UnicodeToUtf8(pNewInfo->fileNameWstr);

    // Update file node itself
    sprintf_s(sql, "UPDATE UsnInfo SET FilePath = '%s', FileName = '%s' WHERE FilePath = '%s';", newPath.c_str(), newName.c_str(), oriPath.c_str());
    if (sqlite3_exec(m_hSql, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf_s("sqlite : update failed\n");
        printf_s("error : %s\n", errMsg);
    }
    else
    {
        // printf_s("sqlite : update done\n");
    }

    // Update file node under folder if exist
    std::vector<UsnInfo> vec;

    QueryInfo queryInfo;
    queryInfo.type = QUERY_BY_PREPATH;
    queryInfo.info.filePathWstr = pOriInfo->filePathWstr;
    SQLiteQueryV2(&queryInfo, vec);

    if (!vec.empty())
    {
        for (auto it = vec.begin(); it != vec.end(); it++)
        {
            // Assembly new file node path
            std::wstring path = (*it).filePathWstr;
            std::wstring find = pOriInfo->filePathWstr;
            std::wstring re = pNewInfo->filePathWstr;
            std::wstring newPath = path.replace(path.find(find), find.length(), re);
            std::wstring newName = GetFileNameAccordPath(newPath);

            sprintf_s(sql, "UPDATE UsnInfo SET FilePath = '%s', FileName = '%s' WHERE SelfRef = %llu;", UnicodeToUtf8(newPath).c_str(), UnicodeToUtf8(newName).c_str(), (*it).pSelfRef);
            if (sqlite3_exec(m_hSql, sql, NULL, NULL, &errMsg) != SQLITE_OK)
            {
                ret = E_FAIL;
                printf_s("sqlite : update failed\n");
                printf_s("error : %s\n", errMsg);
            }
            else
            {
                // printf_s("sqlite : update done\n");
            }
        }
    }

    return ret;
}

HRESULT MiniThingCore::SQLiteClose(void)
{
    sqlite3_close_v2(m_hSql);

    return S_OK;
}
