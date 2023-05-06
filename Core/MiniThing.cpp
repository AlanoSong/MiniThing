#include "MiniThing.h"
#include "../Utility/Utility.h"

MiniThing::MiniThing(std::wstring volumeName, const char* sqlDBPath)
{
    m_volumeName = volumeName;

    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));
    setlocale(LC_ALL, "zh-CN");

    // Get folder handle
    if (FAILED(GetHandle()))
    {
        assert(0);
    }

    // Check if folder is ntfs ?
    if (!IsNtfs())
    {
        assert(0);
    }

    // Create or open sql data base file
    if (FAILED(SQLiteOpen(sqlDBPath)))
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

        if (FAILED(SortUsn()))
        {
            assert(0);
        }

        if (FAILED(DeleteUsn()))
        {
            assert(0);
        }
    }

    closeHandle();
}

MiniThing::~MiniThing(VOID)
{
    if (FAILED(SQLiteClose()))
    {
        assert(0);
    }

}

BOOL MiniThing::IsWstringSame(std::wstring s1, std::wstring s2)
{
    CString c1 = s1.c_str();
    CString c2 = s2.c_str();

    return c1.CompareNoCase(c2) == 0 ? TRUE : FALSE;
}

BOOL MiniThing::IsSubStr(std::wstring s1, std::wstring s2)
{
    return (s1.find(s2) != std::wstring::npos);
}

HRESULT MiniThing::GetHandle()
{
    std::wstring folderPath = L"\\\\.\\";
    folderPath += m_volumeName;

    m_hVol = CreateFile(folderPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);


    if (INVALID_HANDLE_VALUE != m_hVol)
    {
        printf("Get handle success : %s\n", WstringToString(m_volumeName).c_str());
        return S_OK;
    }
    else
    {
        printf("Get handle failed : %s\n", WstringToString(m_volumeName).c_str());
        GetSystemError();
        return E_FAIL;
    }
}

VOID MiniThing::closeHandle(VOID)
{
    if (TRUE == CloseHandle(m_hVol))
    {
        std::wcout << L"Close handle : " << m_volumeName << std::endl;
    }
    else
    {
        std::wcout << L"Get handle : " << m_volumeName << std::endl;
    }

    m_hVol = INVALID_HANDLE_VALUE;
}

VOID MiniThing::GetSystemError(VOID)
{
    LPCTSTR   lpMsgBuf;
    DWORD lastError = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        lastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL
    );

    LPCTSTR strValue = lpMsgBuf;
    _tprintf(_T("ERROR msg : %s\n"), strValue);
}

BOOL MiniThing::IsNtfs(VOID)
{
    BOOL isNtfs = FALSE;
    char sysNameBuf[MAX_PATH] = { 0 };

    int len = WstringToChar(m_volumeName + L"\\", nullptr);
    char* pVol = new char[len];
    WstringToChar(m_volumeName + L"\\", pVol);

    BOOL status = GetVolumeInformationA(
        pVol,
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        sysNameBuf,
        MAX_PATH);

    if (FALSE != status)
    {
        std::cout << "File system name : " << sysNameBuf << std::endl;

        if (0 == strcmp(sysNameBuf, "NTFS"))
        {
            isNtfs = true;
        }
        else
        {
            std::cout << "File system not NTFS format !!!" << std::endl;
            GetSystemError();
        }
    }

    return isNtfs;
}

HRESULT MiniThing::CreateUsn(VOID)
{
    HRESULT ret = S_OK;

    DWORD br;
    CREATE_USN_JOURNAL_DATA cujd;
    cujd.MaximumSize = 0;
    cujd.AllocationDelta = 0;
    BOOL status = DeviceIoControl(
        m_hVol,
        FSCTL_CREATE_USN_JOURNAL,
        &cujd,
        sizeof(cujd),
        NULL,
        0,
        &br,
        NULL);

    if (FALSE != status)
    {
        std::cout << "Create usn file success" << std::endl;
        ret = S_OK;
    }
    else
    {
        std::cout << "Create usn file failed" << std::endl;
        GetSystemError();
        ret = E_FAIL;
    }

    return ret;
}

HRESULT MiniThing::QueryUsn(VOID)
{
    HRESULT ret = S_OK;

    DWORD br;
    BOOL status = DeviceIoControl(m_hVol,
        FSCTL_QUERY_USN_JOURNAL,
        NULL,
        0,
        &m_usnInfo,
        sizeof(m_usnInfo),
        &br,
        NULL);

    if (FALSE != status)
    {
        std::cout << "Query usn info success" << std::endl;
    }
    else
    {
        ret = E_FAIL;
        std::cout << "Query usn info failed" << std::endl;
        GetSystemError();
    }

    return ret;
}

HRESULT MiniThing::RecordUsn(VOID)
{
    MFT_ENUM_DATA med = { 0, 0, m_usnInfo.NextUsn };
    med.MaxMajorVersion = 2;
    // Used to record usn info, must big enough
    char buffer[0x1000];
    DWORD usnDataSize = 0;
    PUSN_RECORD pUsnRecord;

    // Find the first USN record
    // return a USN followed by zero or more change journal records, each in a USN_RECORD structure
    while (FALSE != DeviceIoControl(m_hVol,
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
            // wcsncpy_s(pWchar, pUsnRecord->FileNameLength / 2, pUsnRecord->FileName, pUsnRecord->FileNameLength / 2);
            std::wstring fileNameWstr = WcharToWstring(pWchar);
            delete pWchar;

            UsnInfo usnInfo = { 0 };
            usnInfo.fileNameWstr = fileNameWstr;
            usnInfo.pParentRef = pUsnRecord->ParentFileReferenceNumber;
            usnInfo.pSelfRef = pUsnRecord->FileReferenceNumber;
            usnInfo.timeStamp = pUsnRecord->TimeStamp;

            m_usnRecordMap[usnInfo.pSelfRef] = usnInfo;

            // Get the next USN record
            DWORD recordLen = pUsnRecord->RecordLength;
            dwRetBytes -= recordLen;
            pUsnRecord = (PUSN_RECORD)(((PCHAR)pUsnRecord) + recordLen);
        }

        // Get next page USN record
        // from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx ):  
        // The USN returned as the first item in the output buffer is the USN of the next record number to be retrieved.  
        // Use this value to continue reading records from the end boundary forward.  
        med.StartFileReferenceNumber = *(USN*)&buffer;
    }

    return S_OK;
}

VOID MiniThing::GetCurrentFilePath(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef)
{
    // 1. This is root node, just add root path and return
    if (currentRef == rootRef)
    {
        path = m_volumeName + L"\\" + path;
        return;
    }

    if (m_usnRecordMap.find(currentRef) != m_usnRecordMap.end())
    {
        // 2. Normal node, loop more
        std::wstring str = m_usnRecordMap[currentRef].fileNameWstr;
        path = str + L"\\" + path;
        GetCurrentFilePath(path, m_usnRecordMap[currentRef].pParentRef, rootRef);
    }
    else
    {
        // 3. Some system files's root node is not in current folder
        std::wstring str = L"?";
        path = str + L"\\" + path;

        return;
    }
}

VOID GetCurrentFilePathV2(std::wstring& path, std::wstring volName, DWORDLONG currentRef, DWORDLONG rootRef, unordered_map<DWORDLONG, UsnInfo>& recordMapAll)
{
    // 1. This is root node, just add root path and return
    if (currentRef == rootRef)
    {
        path = volName + L"\\" + path;
        return;
    }

    if (recordMapAll.find(currentRef) != recordMapAll.end())
    {
        // 2. Normal node, loop more
        std::wstring str = recordMapAll[currentRef].fileNameWstr;
        path = str + L"\\" + path;
        GetCurrentFilePathV2(path, volName, recordMapAll[currentRef].pParentRef, rootRef, recordMapAll);
    }
    else
    {
        // 3. Some system files's root node is not in current folder
        std::wstring str = L"?";
        path = str + L"\\" + path;

        return;
    }
}

HRESULT SQLiteInsertV2(sqlite3* hSql, UsnInfo* pUsnInfo)
{
    HRESULT ret = S_OK;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    char sql[1024] = { 0 };
    std::string nameUtf8 = UnicodeToUtf8(pUsnInfo->fileNameWstr);
    std::string pathUtf8 = UnicodeToUtf8(pUsnInfo->filePathWstr);
    sprintf_s(sql, "INSERT INTO UsnInfo VALUES(%llu, %llu, %lld, '%s', '%s');",
        pUsnInfo->pSelfRef, pUsnInfo->pParentRef, pUsnInfo->timeStamp, nameUtf8.c_str(), pathUtf8.c_str());

    char* errMsg = nullptr;
    if (sqlite3_exec(hSql, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : insert failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        // printf("sqlite : insert done\n");
    }

    return ret;
}

DWORD WINAPI SortThread(LPVOID lp)
{
    SortTaskInfo* pTaskInfo = (SortTaskInfo*)lp;

    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));
    setlocale(LC_ALL, "zh-CN");

    printf("Sort task %d start\n", pTaskInfo->taskIndex);

    int ret = 0;

    if(ret == SQLITE_OK)
    {
        // 1. Get start time
        LARGE_INTEGER timeStart;
        LARGE_INTEGER timeEnd;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double quadpart = (double)frequency.QuadPart;
        QueryPerformanceCounter(&timeStart);

        for (auto it = (*pTaskInfo->pSortTask).begin(); it != (*pTaskInfo->pSortTask).end(); it++)
        {
            UsnInfo usnInfo = it->second;
            std::wstring path(usnInfo.fileNameWstr);
            GetCurrentFilePathV2(path, pTaskInfo->rootFolderName, usnInfo.pParentRef, pTaskInfo->rootRef, *(pTaskInfo->pAllUsnRecordMap));
            (*pTaskInfo->pSortTask)[it->first].filePathWstr = path;
        }

        QueryPerformanceCounter(&timeEnd);
        double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
        printf("Sort task %d over, %f S\n", pTaskInfo->taskIndex, elapsed);
    }
    else
    {
        std::cout << "Cannot connect to sqlite db" << std::endl;
    }

    return 0;
}

HRESULT MiniThing::SortUsn(VOID)
{
    HRESULT ret = S_OK;

    std::cout << "Generating sql data base......" << std::endl;

    // 1. Get root file node
    //  cause "System Volume Information" is a system file and just under root folder
    //  so we use it to find root folder
    std::wstring cmpStr(L"System Volume Information");
    m_rootFileNode = 0x0;

    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        if (0 == usnInfo.fileNameWstr.compare(cmpStr))
        {
            m_rootFileNode = usnInfo.pParentRef;
            break;
        }
    }

    if (m_rootFileNode == 0)
    {
        std::cout << "Cannot find root folder" << std::endl;
        ret = E_FAIL;
        assert(0);
    }

    // 2. Get suitable thread task granularity
    int hwThreadNum = std::thread::hardware_concurrency();
    int granularity = m_usnRecordMap.size() / hwThreadNum;
    int step = 100;
    while (granularity * hwThreadNum < m_usnRecordMap.size())
    {
        granularity += 100;
    }

    // 3. Divide file node into several sort tasks
    int fileNodeCnt = 0;
    vector<unordered_map<DWORDLONG, UsnInfo>> sortTaskSet;
    unordered_map<DWORDLONG, UsnInfo> mapTmp;

    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
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
        fileNodeCnt += mapTmp.size();
        sortTaskSet.push_back(mapTmp);
        mapTmp.clear();
    }

    vector<HANDLE> taskHandleVec;
    vector<SortTaskInfo> sortTaskVec;

    for (int i = 0; i < sortTaskSet.size(); i++)
    {
        SortTaskInfo taskInfo;
        taskInfo.taskIndex = i;
        taskInfo.rootFolderName = m_volumeName;
        taskInfo.sqlPath = m_SQLitePath;
        taskInfo.pSortTask = &(sortTaskSet[i]);
        taskInfo.pAllUsnRecordMap = &m_usnRecordMap;
        taskInfo.rootRef = m_rootFileNode;

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

    printf("Insert begin...\n");
    // 6. Write all file node info into sqlite
    char* errMsg = nullptr;
    int insertNodeCnt = 0;
    char sql[1024] = { 0 };

    sqlite3_exec(m_hSQLite, "BEGIN", NULL, NULL, &errMsg);

    sprintf_s(sql, "INSERT INTO UsnInfo (SelfRef, ParentRef, TimeStamp, FileName, FilePath) VALUES(?, ?, ?, ?, ?);");
    sqlite3_stmt* pPrepare = nullptr;
    sqlite3_prepare_v2(m_hSQLite, sql, strlen(sql), &pPrepare, 0);

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
            sqlite3_bind_text(pPrepare, 4, nameUtf8.c_str(), strlen(nameUtf8.c_str()), nullptr);
            sqlite3_bind_text(pPrepare, 5, pathUtf8.c_str(), strlen(pathUtf8.c_str()), nullptr);
            int ret = sqlite3_step(pPrepare);
            assert(ret == SQLITE_DONE);

            if (insertNodeCnt % SQL_BATCH_INSERT_GRANULARITY == 0)
            {
                sqlite3_exec(m_hSQLite, "COMMIT", NULL, NULL, &errMsg);
                sqlite3_exec(m_hSQLite, "BEGIN", NULL, NULL, &errMsg);
            }
        }
    }
    sqlite3_exec(m_hSQLite, "COMMIT", NULL, NULL, &errMsg);
    sqlite3_finalize(pPrepare);

    // 7. After file node sorted, the map can be destroyed
    taskHandleVec.clear();
    sortTaskVec.clear();
    sortTaskSet.clear();
    m_usnRecordMap.clear();

    std::wcout << "Sort file node sum : " << fileNodeCnt << std::endl;

    return ret;
}

HRESULT MiniThing::DeleteUsn(VOID)
{
    HRESULT ret = S_OK;

    DELETE_USN_JOURNAL_DATA dujd;
    dujd.UsnJournalID = m_usnInfo.UsnJournalID;
    dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;
    DWORD lenRet = 0;

    int status = DeviceIoControl(m_hVol,
        FSCTL_DELETE_USN_JOURNAL,
        &dujd,
        sizeof(dujd),
        NULL,
        0,
        &lenRet,
        NULL);

    if (FALSE != status)
    {
        std::cout << "Delete usn file success" << std::endl;
    }
    else
    {
        GetSystemError();
        ret = E_FAIL;
        std::cout << "Delete usn file failed" << std::endl;
    }

    return ret;
}

static std::wstring GetFileNameAccordPath(std::wstring path)
{
    return path.substr(path.find_last_of(L"\\") + 1);
}

static std::wstring GetPathAccordPath(std::wstring path)
{
    wstring name = GetFileNameAccordPath(path);
    return path.substr(0, path.length() - name.length());
}

VOID MiniThing::AdjustUsnRecord(std::wstring folder, std::wstring filePath, std::wstring fileRePath, DWORD op)
{
    switch (op)
    {
    case FILE_ACTION_ADDED:
    {
        UsnInfo tmp = { 0 };
        tmp.filePathWstr = filePath;
        tmp.fileNameWstr = GetFileNameAccordPath(filePath);
        tmp.pParentRef = m_constFileRefNumMax;
        tmp.pSelfRef = m_unusedFileRefNum--;

        // Update sql db
        if (FAILED(SQLiteInsert(&tmp)))
        {
            assert(0);
        }
        break;
    }

    case FILE_ACTION_RENAMED_OLD_NAME:
    {
        // Update sql db
        UsnInfo oriInfo = { 0 };
        oriInfo.fileNameWstr = GetFileNameAccordPath(filePath);
        oriInfo.filePathWstr = filePath;
        UsnInfo reInfo = { 0 };
        reInfo.fileNameWstr = GetFileNameAccordPath(fileRePath);
        reInfo.filePathWstr = fileRePath;
        if (FAILED(SQLiteUpdateV2(&oriInfo, &reInfo)))
        {
            assert(0);
        }
        break;
    }

    case FILE_ACTION_REMOVED:
    {
        // Update sql db
        UsnInfo tmpUsn = { 0 };
        tmpUsn.filePathWstr = filePath;
        if (FAILED(SQLiteDelete(&tmpUsn)))
        {
            assert(0);
        }
        break;
    }

    default:
        assert(0);
    }
}

DWORD WINAPI UpdateDataBaseThread(LPVOID lp)
{
    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));
    setlocale(LC_ALL, "zh-CN");

    UpdateDataBaseTaskInfo* pTaskInfo = (UpdateDataBaseTaskInfo*)lp;
    MiniThing* pMiniThing = (MiniThing*)pTaskInfo->pMiniThing;

    switch (pTaskInfo->op)
    {
    case FILE_ACTION_ADDED:
    {
        wprintf(L"Add: '%s'\n", pTaskInfo->oriPath.c_str());

        UsnInfo tmp = { 0 };
        tmp.filePathWstr = pTaskInfo->oriPath;
        tmp.fileNameWstr = GetFileNameAccordPath(pTaskInfo->oriPath);
        tmp.pParentRef = pMiniThing->GetParentFileRef();
        tmp.pSelfRef = pMiniThing->GetNewFileRef();

        if (FAILED(pMiniThing->SQLiteInsert(&tmp)))
        {
            assert(0);
        }
        break;
    }

    case FILE_ACTION_RENAMED_OLD_NAME:
    {
        wprintf(L"Ren: '%s' -> '%s'\n", pTaskInfo->oriPath.c_str(), pTaskInfo->newPath.c_str());

        UsnInfo oriInfo = { 0 };
        oriInfo.fileNameWstr = GetFileNameAccordPath(pTaskInfo->oriPath);
        oriInfo.filePathWstr = pTaskInfo->oriPath;

        UsnInfo reInfo = { 0 };
        reInfo.fileNameWstr = GetFileNameAccordPath(pTaskInfo->newPath);
        reInfo.filePathWstr = pTaskInfo->newPath;

        if (FAILED(pMiniThing->SQLiteUpdateV2(&oriInfo, &reInfo)))
        {
            assert(0);
        }
        break;
    }

    case FILE_ACTION_REMOVED:
    {
        wprintf(L"Rem: '%s'\n", pTaskInfo->oriPath.c_str());

        UsnInfo tmpUsn = { 0 };
        tmpUsn.filePathWstr = pTaskInfo->oriPath;
        if (FAILED(pMiniThing->SQLiteDelete(&tmpUsn)))
        {
            assert(0);
        }
        break;
    }

    default:
        break;
    }

    delete pTaskInfo;

    return 0;
}

DWORD WINAPI MonitorThread(LPVOID lp)
{
    MiniThing* pMiniThing = (MiniThing*)lp;

    char notifyInfo[1024];

    wstring filePathWstr;
    wstring fileRePathWstr;

    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));
    setlocale(LC_ALL, "zh-CN");

    std::wcout << L"Start monitor : " << pMiniThing->GetVolName() << std::endl;

    std::wstring folderPath = pMiniThing->GetVolName();

    HANDLE dirHandle = CreateFile(folderPath.c_str(),
        GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (dirHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Error: " << GetLastError() << std::endl;
        assert(0);
    }

    auto* pNotifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(notifyInfo);

    while (true)
    {
        // Check if need exit thread
        DWORD dwWaitCode = WaitForSingleObject(pMiniThing->m_hExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            std::cout << "Recv the quit event" << std::endl;
            break;
        }

        DWORD retBytes;

        if (ReadDirectoryChangesW(
            dirHandle,
            &notifyInfo,
            sizeof(notifyInfo),
            true, // Monitor sub directory
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
            &retBytes,
            nullptr,
            nullptr))
        {
            // Change file name
            filePathWstr = pNotifyInfo->FileName;
            filePathWstr.resize(pNotifyInfo->FileNameLength / 2);

            // Rename file new name
            auto pInfo = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(reinterpret_cast<char*>(pNotifyInfo) + pNotifyInfo->NextEntryOffset);
            fileRePathWstr = pInfo->FileName;
            fileRePathWstr.resize(pInfo->FileNameLength / 2);

            // We delete pTaskInfo in update thread when task over
            UpdateDataBaseTaskInfo* pTaskInfo = new UpdateDataBaseTaskInfo;
            pTaskInfo->pMiniThing = pMiniThing;
            pTaskInfo->op = 0;

            switch (pNotifyInfo->Action)
            {
            case FILE_ACTION_ADDED:
                if (filePathWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    std::wstring addPath;
                    addPath.clear();
                    addPath.append(pMiniThing->GetVolName());
                    addPath.append(L"\\");
                    addPath.append(filePathWstr);

                    pTaskInfo->op = FILE_ACTION_ADDED;
                    pTaskInfo->oriPath = addPath;

                    // pMiniThing->AdjustUsnRecord(pMiniThing->GetVolName(), addPath, L"", FILE_ACTION_ADDED);
                }
                break;

            case FILE_ACTION_MODIFIED:
                if (filePathWstr.find(L"$RECYCLE.BIN") == wstring::npos &&
                    filePathWstr.find(L"fileAdded.txt") == wstring::npos &&
                    filePathWstr.find(L"fileRemoved.txt") == wstring::npos)
                {
                    std::wstring modPath;
                    modPath.append(pMiniThing->GetVolName());
                    modPath.append(L"\\");
                    modPath.append(filePathWstr);

                    // Must use printf here cuase cout is not thread safety
                    wprintf(L"Mod file : %s\n", modPath.c_str());
                }
                break;

            case FILE_ACTION_REMOVED:
                if (filePathWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    std::wstring remPath;
                    remPath.clear();
                    remPath.append(pMiniThing->GetVolName());
                    remPath.append(L"\\");
                    remPath.append(filePathWstr);

                    pTaskInfo->op = FILE_ACTION_REMOVED;
                    pTaskInfo->oriPath = remPath;

                    // pMiniThing->AdjustUsnRecord(pMiniThing->GetVolName(), remPath, L"", FILE_ACTION_REMOVED);
                }
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                if (filePathWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    wstring oriPath;
                    oriPath.clear();
                    oriPath.append(pMiniThing->GetVolName());
                    oriPath.append(L"\\");
                    oriPath.append(filePathWstr);

                    wstring rePath;
                    rePath.clear();
                    rePath.append(pMiniThing->GetVolName());
                    rePath.append(L"\\");
                    rePath.append(fileRePathWstr);

                    pTaskInfo->op = FILE_ACTION_RENAMED_OLD_NAME;
                    pTaskInfo->oriPath = oriPath;
                    pTaskInfo->newPath = rePath;

                    // pMiniThing->AdjustUsnRecord(pMiniThing->GetVolName(), oriPath, rePath, FILE_ACTION_RENAMED_OLD_NAME);
                }
                break;

            default:
                std::wcout << L"Unknown command" << std::endl;
            }

            // We dispath those update task in sub thread,
            //  cause the update sql data base may consume some time,
            //  and we may lost file change message in this process.
            CreateThread(0, 0, UpdateDataBaseThread, (VOID*)pTaskInfo, CREATE_NEW_CONSOLE, 0);
        }
    }

    CloseHandle(dirHandle);

    std::cout << "Monitor thread stop" << std::endl;

    return 0;
}

HRESULT MiniThing::CreateMonitorThread(VOID)
{
    HRESULT ret = S_OK;

    m_hExitEvent = CreateEvent(0, 0, 0, 0);

    // 以挂起方式创建线程
    m_hMonitorThread = CreateThread(0, 0, MonitorThread, this, CREATE_SUSPENDED, 0);
    if (INVALID_HANDLE_VALUE == m_hMonitorThread)
    {
        GetSystemError();
        ret = E_FAIL;
    }

    return ret;
}

VOID MiniThing::StartMonitorThread(VOID)
{
    // 使线程开始运行
    ResumeThread(m_hMonitorThread);
}

VOID MiniThing::StopMonitorThread(VOID)
{
    // 使事件处于激发的状态
    SetEvent(m_hExitEvent);

    // 等待线程运行结束
    DWORD dwWaitCode = WaitForSingleObject(m_hMonitorThread, INFINITE);

    // 断言判断线程是否正常结束
    assert(dwWaitCode == WAIT_OBJECT_0);

    // 释放线程句柄
    CloseHandle(m_hMonitorThread);
}

DWORD WINAPI QueryThread(LPVOID lp)
{
    MiniThing* pMiniThing = (MiniThing*)lp;

    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));
    setlocale(LC_ALL, "zh-CN");

    printf("Query thread start\n");

    while (TRUE)
    {
        // Check if need exit thread
        DWORD dwWaitCode = WaitForSingleObject(pMiniThing->m_hQueryExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            std::cout << "Recv the quit event" << std::endl;
            break;
        }

        std::wcout << std::endl << L"==============================" << std::endl;
        std::wcout << L"Input query file info here:" << std::endl;

        std::wstring query;
        std::wcin >> query;

        std::vector<std::wstring> vec;

        LARGE_INTEGER timeStart;
        LARGE_INTEGER timeEnd;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double quadpart = (double)frequency.QuadPart;

        QueryPerformanceCounter(&timeStart);

        pMiniThing->SQLiteQuery(query, vec);

        QueryPerformanceCounter(&timeEnd);
        double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
        std::cout << "Time elapsed : " << elapsed << " S" << std::endl;

        if (vec.empty())
        {
            std::wcout << "Not found" << std::endl;
        }
        else
        {
            int cnt = 0;
            for (auto it = vec.begin(); it != vec.end(); it++)
            {
                std::wcout << L"No." << cnt++ << " : " << *it << endl;
            }
        }
        std::wcout << L"==============================" << std::endl;
    }

    printf("Query thread stop\n");
    return 0;
}

HRESULT MiniThing::CreateQueryThread(VOID)
{
    HRESULT ret = S_OK;

    m_hQueryExitEvent = CreateEvent(0, 0, 0, 0);

    // 以挂起方式创建线程
    m_hQueryThread = CreateThread(0, 0, QueryThread, this, CREATE_SUSPENDED, 0);
    if (INVALID_HANDLE_VALUE == m_hQueryThread)
    {
        GetSystemError();
        ret = E_FAIL;
    }

    return ret;
}

VOID MiniThing::StartQueryThread(VOID)
{
    // 使线程开始运行
    ResumeThread(m_hQueryThread);
}

VOID MiniThing::StopQueryThread(VOID)
{
    // 使事件处于激发的状态
    SetEvent(m_hQueryExitEvent);

    // 等待线程运行结束
    DWORD dwWaitCode = WaitForSingleObject(m_hQueryThread, INFINITE);

    // 断言判断线程是否正常结束
    assert(dwWaitCode == WAIT_OBJECT_0);

    // 释放线程句柄
    CloseHandle(m_hQueryThread);
}

HRESULT MiniThing::SQLiteOpen(CONST CHAR* path)
{
    HRESULT ret = S_OK;
    m_SQLitePath = path;
    int result = sqlite3_open_v2(path, &m_hSQLite, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, NULL);

    // Turn off sqlite write sync
    char* errMsg = nullptr;
    sqlite3_exec(m_hSQLite, "PRAGMA synchronous = OFF", NULL, NULL, &errMsg);

    if (result == SQLITE_OK)
    {
        char* errMsg = nullptr;

        std::string create;
        create.append("CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);");

        int res = sqlite3_exec(m_hSQLite, create.c_str(), 0, 0, &errMsg);
        if (res == SQLITE_OK)
        {
            m_isSqlExist = FALSE;
        }
        else
        {
            std::cout << "SQLite : data base already exist" << std::endl;
            m_isSqlExist = TRUE;
        }

        std::cout << "SQLite : open success" << std::endl;
    }
    else
    {
        ret = E_FAIL;
        std::cout << "SQLite : open failed" << std::endl;
    }

    return ret;
}

HRESULT MiniThing::SQLiteInsert(UsnInfo* pUsnInfo)
{
    HRESULT ret = S_OK;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    char sql[1024] = { 0 };
    std::string nameUtf8 = UnicodeToUtf8(pUsnInfo->fileNameWstr);
    std::string pathUtf8 = UnicodeToUtf8(pUsnInfo->filePathWstr);
    sprintf_s(sql, "INSERT INTO UsnInfo VALUES(%llu, %llu, %lld, '%s', '%s');",
        pUsnInfo->pSelfRef, pUsnInfo->pParentRef, pUsnInfo->timeStamp, nameUtf8.c_str(), pathUtf8.c_str());

    char* errMsg = nullptr;
    if (sqlite3_exec(m_hSQLite, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : insert failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        // printf("sqlite : insert done\n");
    }

    return ret;
}

HRESULT MiniThing::SQLiteQuery(std::wstring queryInfo, std::vector<std::wstring>& vec)
{
    HRESULT ret = S_OK;
    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    // 1. Conveter unicode -> utf-8
    std::string str = UnicodeToUtf8(queryInfo);

    // 2. Query and pop vector
    sprintf_s(sql, "SELECT * FROM UsnInfo WHERE FileName LIKE '%%%s%%';", str.c_str());

    sqlite3_stmt* stmt = NULL;
    int res = sqlite3_prepare_v2(m_hSQLite, sql, (int)strlen(sql), &stmt, NULL);
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
        printf("sqlite : query failed\n");
    }

    sqlite3_finalize(stmt);

    return ret;
}

HRESULT MiniThing::SQLiteQueryV2(QueryInfo* queryInfo, std::vector<UsnInfo>& vec)
{
    HRESULT ret = S_OK;
    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    if (queryInfo)
    {
        switch (queryInfo->type)
        {
        case BY_REF:
        {
            sprintf_s(sql, "SELECT * FROM UsnInfo WHERE SelfRef = %llu;", queryInfo->info.pSelfRef);
            break;
        }
        case BY_NAME:
        {
            std::string str = UnicodeToUtf8(queryInfo->info.fileNameWstr);
            sprintf_s(sql, "SELECT * FROM UsnInfo WHERE FileName LIKE '%%%s%%';", str.c_str());
            break;
        }
        case BY_PREPATH:
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
    int res = sqlite3_prepare_v2(m_hSQLite, sql, (int)strlen(sql), &stmt, NULL);
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
        printf("sqlite : query failed\n");
    }

    sqlite3_finalize(stmt);

    return ret;
}

HRESULT MiniThing::SQLiteDelete(UsnInfo* pUsnInfo)
{
    HRESULT ret = S_OK;

    // Update sql db
    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    std::string path = UnicodeToUtf8(pUsnInfo->filePathWstr);
    sprintf_s(sql, "DELETE FROM UsnInfo WHERE FilePath = '%s';", path.c_str());
    if (sqlite3_exec(m_hSQLite, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : delete failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        // printf("sqlite : delete done\n");
    }

    char sql1[1024] = { 0 };
    sprintf_s(sql1, "DELETE FROM UsnInfo WHERE FilePath LIKE '%s\\%%';", path.c_str());
    if (sqlite3_exec(m_hSQLite, sql1, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : delete failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        // printf("sqlite : delete done\n");
    }

    return ret;
}

HRESULT MiniThing::SQLiteUpdate(UsnInfo* pUsnInfo, std::wstring originPath)
{
    HRESULT ret = S_OK;

    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    std::string oriPath = UnicodeToUtf8(originPath);
    std::string newPath = UnicodeToUtf8(pUsnInfo->filePathWstr);
    std::string newName = UnicodeToUtf8(pUsnInfo->fileNameWstr);
    sprintf_s(sql, "UPDATE UsnInfo SET FileName = '%s', FilePath = '%s' WHERE FilePath = '%s';", newName.c_str(), newPath.c_str(), oriPath.c_str());
    if (sqlite3_exec(m_hSQLite, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : update failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        // printf("sqlite : update done\n");
    }

    return ret;
}

HRESULT MiniThing::SQLiteUpdateV2(UsnInfo* pOriInfo, UsnInfo* pNewInfo)
{
    HRESULT ret = S_OK;

    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    std::string oriPath = UnicodeToUtf8(pOriInfo->filePathWstr);
    std::string oriName = UnicodeToUtf8(pOriInfo->fileNameWstr);

    std::string newPath = UnicodeToUtf8(pNewInfo->filePathWstr);
    std::string newName = UnicodeToUtf8(pNewInfo->fileNameWstr);

    // Update file node itself
    sprintf_s(sql, "UPDATE UsnInfo SET FilePath = '%s', FileName = '%s' WHERE FilePath = '%s';", newPath.c_str(), newName.c_str(), oriPath.c_str());
    if (sqlite3_exec(m_hSQLite, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : update failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        // printf("sqlite : update done\n");
    }

    // Update file node under folder if exist
    std::vector<UsnInfo> vec;

    QueryInfo queryInfo;
    queryInfo.type = BY_PREPATH;
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
            if (sqlite3_exec(m_hSQLite, sql, NULL, NULL, &errMsg) != SQLITE_OK)
            {
                ret = E_FAIL;
                printf("sqlite : update failed\n");
                printf("error : %s\n", errMsg);
            }
            else
            {
                // printf("sqlite : update done\n");
            }
        }
    }

    return ret;
}

HRESULT MiniThing::SQLiteClose(VOID)
{
    sqlite3_close_v2(m_hSQLite);

    return S_OK;
}

