#include "MiniThing.h"
#include "../Utility/Utility.h"

MiniThing::MiniThing(std::wstring volumeName, const char* sqlDBPath)
{
    m_volumeName = volumeName;

    // Set chinese debug output
    std::wcout.imbue(std::locale("chs"));

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
        std::wcout << L"Get handle : " << m_volumeName << std::endl;
        return S_OK;
    }
    else
    {
        std::wcout << L"Get handle failed : " << m_volumeName << std::endl;
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

    // 找到第一个 USN 记录
    // from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx ):
    // return a USN followed by zero or more change journal records, each in a USN_RECORD structure.
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
            std::string fileNameStr = WstringToString(fileNameWstr);
            delete pWchar;

            // Several system file "$***" cannot find its parent ref, so just exclude them directly
            // TO DO : remove those logic
            if (fileNameWstr.find(L"$") == std::wstring::npos)
            {
                // Several file's parent is not in current root folder, so just exclude them directly
                if (find(m_patchOfParentRef.begin(), m_patchOfParentRef.end(), pUsnRecord->ParentFileReferenceNumber) == m_patchOfParentRef.end())
                {
                    UsnInfo usnInfo = { 0 };
                    usnInfo.fileNameStr = fileNameStr;
                    usnInfo.fileNameWstr = fileNameWstr;
                    usnInfo.pParentRef = pUsnRecord->ParentFileReferenceNumber;
                    usnInfo.pSelfRef = pUsnRecord->FileReferenceNumber;
                    usnInfo.timeStamp = pUsnRecord->TimeStamp;
// #if USE_MAP_STORE
                    m_usnRecordMap[usnInfo.pSelfRef] = usnInfo;
// #else
                    SQLiteInsert(&usnInfo);
// #endif
                }
            }

            // 获取下一个记录
            DWORD recordLen = pUsnRecord->RecordLength;
            dwRetBytes -= recordLen;
            pUsnRecord = (PUSN_RECORD)(((PCHAR)pUsnRecord) + recordLen);
        }

        // 获取下一页数据， MTF 大概是分多页来储存的吧？  
        // from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx ):  
        // The USN returned as the first item in the output buffer is the USN of the next record number to be retrieved.  
        // Use this value to continue reading records from the end boundary forward.  
        med.StartFileReferenceNumber = *(USN*)&buffer;
    }

    return S_OK;
}

VOID MiniThing::GetCurrentFilePath(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef)
{
    if (currentRef == rootRef)
    {
        path = m_volumeName + L"\\" + path;
        return;
    }

    std::wstring str = m_usnRecordMap[currentRef].fileNameWstr;
    path = str + L"\\" + path;
    GetCurrentFilePath(path, m_usnRecordMap[currentRef].pParentRef, rootRef);
}

VOID MiniThing::GetCurrentFilePathBySql(std::wstring& path, DWORDLONG currentRef, DWORDLONG rootRef)
{
    // 1. This is root node, just add root path and return
    if (currentRef == rootRef)
    {
        path = m_volumeName + L"\\" + path;
        return;
    }

    QueryInfo queryInfo;
    queryInfo.type = BY_REF;
    queryInfo.info.pSelfRef = currentRef;
    std::vector<UsnInfo> vec;

    SQLiteQueryV2(&queryInfo, vec);

    if (vec.size() == 1)
    {
        // 2. Normal node, loop more
        std::wstring str = vec[0].fileNameWstr;
        path = str + L"\\" + path;

        GetCurrentFilePathBySql(path, vec[0].pParentRef, rootRef);
    }
    else if (vec.empty())
    {
        // 3. Some system files's root node is not in current folder
        std::wstring str = L"?";
        path = str + L"\\" + path;
    }
    else
    {
        assert(0);
    }
}

HRESULT MiniThing::SortUsn(VOID)
{
    HRESULT ret = S_OK;

    // Get "System Volume Information"'s parent ref number
    //      cause it's under top level folder
    //      so its parent ref number is top level folder's ref number
    std::wstring cmpStr(L"System Volume Information");
    DWORDLONG topLevelRefNum = 0x0;

#if USE_MAP_STORE
    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        if (0 == usnInfo.fileNameWstr.compare(cmpStr))
        {
            topLevelRefNum = usnInfo.pParentRef;
            break;
        }
    }
#else
    QueryInfo queryInfo;
    queryInfo.type = BY_NAME;
    queryInfo.info.fileNameWstr = cmpStr;
    std::vector<UsnInfo> vec;
    SQLiteQueryV2(&queryInfo, vec);
    assert(vec.size() == 1);

    topLevelRefNum = vec[0].pParentRef;
#endif

    if (topLevelRefNum == 0)
    {
        std::cout << "Cannot find root folder" << std::endl;
        ret = E_FAIL;
        goto exit;
    }

    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        std::wstring path(usnInfo.fileNameWstr);
        std::wstring pathBySql(usnInfo.fileNameWstr);
#if USE_MAP_STORE
        GetCurrentFilePath(path, usnInfo.pParentRef, topLevelRefNum);
        m_usnRecordMap[usnInfo.pSelfRef].filePathWstr = path;
#else
        GetCurrentFilePathBySql(pathBySql, usnInfo.pParentRef, topLevelRefNum);
        usnInfo.filePathWstr = pathBySql;
        SQLiteUpdateV2(&usnInfo, usnInfo.pSelfRef);
#endif
    }

    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;

        // Store to SQLite
        // SQLiteInsert(&usnInfo);
#if _DEBUG
        // 打印获取到的文件信息
        std::wcout << std::endl << L"File name : " << usnInfo.fileNameWstr << std::endl;
        std::wcout << L"File path : " << usnInfo.filePathWstr << std::endl;
        std::cout << "File ref number : 0x" << std::hex << usnInfo.pSelfRef << std::endl;
        std::cout << "File parent ref number : 0x" << std::hex << usnInfo.pParentRef << std::endl << std::endl;
#endif
    }

exit:
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

std::wstring MiniThing::GetFileNameAccordPath(std::wstring path)
{
    return path.substr(path.find_last_of(L"\\") + 1);
}

std::wstring MiniThing::GetPathAccordPath(std::wstring path)
{
    wstring name = GetFileNameAccordPath(path);
    return path.substr(0, path.length() - name.length());
}

VOID MiniThing::AdjustUsnRecord(std::wstring folder, std::wstring filePath, std::wstring reFileName, DWORD op)
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
#if USE_MAP_STORE
        m_usnRecordMap[tmp.pSelfRef] = tmp;
#else
        // Update sql db
        if (FAILED(SQLiteInsert(&tmp)))
        {
            assert(0);
        }
#endif
        break;
    }

    case FILE_ACTION_RENAMED_OLD_NAME:
    {
#if USE_MAP_STORE
        // TODO:
        //      if rename a folder?
        auto it = m_usnRecordMap.begin();
        for (; it != m_usnRecordMap.end(); it++)
        {
            if (IsWstringSame(it->second.filePathWstr, filePath))
            {
                break;
            }
        }
        if (it == m_usnRecordMap.end())
        {
            std::cout << "Failed find rename file" << std::endl;
            break;
        }

        auto tmp = m_usnRecordMap[it->first];
        tmp.filePathWstr = reFileName;
        tmp.fileNameWstr = GetFileNameAccordPath(reFileName);
        m_usnRecordMap[it->first] = tmp;
#else
        // Update sql db
        // TODO:
        //      if rename a folder?
        UsnInfo usnInfo = { 0 };
        usnInfo.fileNameWstr = reFileName;
        if (FAILED(SQLiteUpdate(&usnInfo, filePath)))
        {
            assert(0);
        }
#endif
        break;
    }

    case FILE_ACTION_REMOVED:
    {
#if USE_MAP_STORE
        // TODO:
        //      if remove a folder?
        auto tmp = m_usnRecordMap.end();
        for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
        {
            if (IsWstringSame(it->second.filePathWstr, filePath))
            {
                tmp = it;
                break;
            }
        }
        if (tmp != m_usnRecordMap.end())
        {
            m_usnRecordMap.erase(tmp);
        }
#else
        // Update sql db
        // TODO:
        //      if remove a folder?
        UsnInfo tmpUsn = { 0 };
        tmpUsn.filePathWstr = filePath;
        if (FAILED(SQLiteDelete(&tmpUsn)))
        {
            assert(0);
        }
#endif
        break;
    }

    default:
        assert(0);
    }
}

DWORD WINAPI MonitorThread(LPVOID lp)
{
    MiniThing* pMiniThing = (MiniThing*)lp;

    char notifyInfo[1024];

    wstring fileNameWstr;
    wstring fileRenameWstr;

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

    // 若网络重定向或目标文件系统不支持该操作，函数失败，同时调用GetLastError()返回ERROR_INVALID_FUNCTION
    if (dirHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Error " << GetLastError() << std::endl;
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
            fileNameWstr = pNotifyInfo->FileName;
            fileNameWstr[pNotifyInfo->FileNameLength / 2] = 0;

            // Rename file new name
            auto pInfo = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(reinterpret_cast<char*>(pNotifyInfo) + pNotifyInfo->NextEntryOffset);
            fileRenameWstr = pInfo->FileName;
            fileRenameWstr[pInfo->FileNameLength / 2] = 0;

            switch (pNotifyInfo->Action)
            {
            case FILE_ACTION_ADDED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    std::wstring addPath;
                    addPath.clear();
                    addPath.append(pMiniThing->GetVolName());
                    addPath.append(L"\\");
                    addPath.append(fileNameWstr);

                    // Here use printf to suit multi-thread
                    wprintf(L"\nAdd file : %s\n", addPath.c_str());

                    pMiniThing->AdjustUsnRecord(pMiniThing->GetVolName(), addPath, L"", FILE_ACTION_ADDED);
                }
                break;

            case FILE_ACTION_MODIFIED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos &&
                    fileNameWstr.find(L"fileAdded.txt") == wstring::npos &&
                    fileNameWstr.find(L"fileRemoved.txt") == wstring::npos)
                {
                    std::wstring modPath;
                    modPath.append(pMiniThing->GetVolName());
                    modPath.append(L"\\");
                    modPath.append(fileNameWstr);
                    // Here use printf to suit multi-thread
                    wprintf(L"Mod file : %s\n", modPath.c_str());
                    //add_record(to_utf8(StringToWString(data)));
                }
                break;

            case FILE_ACTION_REMOVED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    std::wstring remPath;
                    remPath.clear();
                    remPath.append(pMiniThing->GetVolName());
                    remPath.append(L"\\");
                    remPath.append(fileNameWstr);

                    // Here use printf to suit multi-thread
                    wprintf(L"\nRemove file : %s\n", remPath.c_str());

                    pMiniThing->AdjustUsnRecord(pMiniThing->GetVolName(), remPath, L"", FILE_ACTION_REMOVED);
                }
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    wstring oriName;
                    oriName.clear();
                    oriName.append(pMiniThing->GetVolName());
                    oriName.append(L"\\");
                    oriName.append(fileNameWstr);

                    wstring reName;
                    reName.clear();
                    reName.append(pMiniThing->GetVolName());
                    reName.append(L"\\");
                    reName.append(fileRenameWstr);

                    // Here use printf to suit multi-thread
                    wprintf(L"\nRename file : %s -> %s\n", oriName.c_str(), reName.c_str());

                    pMiniThing->AdjustUsnRecord(pMiniThing->GetVolName(), oriName, reName, FILE_ACTION_RENAMED_OLD_NAME);
                }
                break;

            default:
                std::wcout << L"Unknown command" << std::endl;
            }
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
        std::wcout << std::endl <<  L"==============================" << std::endl;
        std::wcout << L"Input query file info here:" << std::endl;

        std::wstring query;
        std::wcin >> query;

        std::vector<std::wstring> vec;
        pMiniThing->SQLiteQuery(query, vec);

#if _DEBUG
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
#endif
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

    if (result == SQLITE_OK)
    {
        std::string table;
        table.append(
            "DROP TABLE IF EXISTS UsnInfo;"
            "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
        );

        char* errMsg = nullptr;

        int rc = sqlite3_exec(m_hSQLite, table.c_str(), NULL, NULL, &errMsg);
        if (rc != SQLITE_OK)
        {
            std::cerr << "SQLite : create table failed" << std::endl;
            sqlite3_free(errMsg);
            sqlite3_close(m_hSQLite);

            ret = E_FAIL;
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
        printf("sqlite : insert done\n");
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

HRESULT MiniThing::SQLiteQueryV2(QueryInfo *queryInfo, std::vector<UsnInfo>& vec)
{
    HRESULT ret = S_OK;
    char sql[1024] = { 0 };
    char* errMsg = nullptr;

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
    default:
        assert(0);
        break;
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
        printf("sqlite : delete done\n");
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
        printf("sqlite : update done\n");
    }

    return ret;
}

HRESULT MiniThing::SQLiteUpdateV2(UsnInfo* pUsnInfo, DWORDLONG selfRef)
{
    HRESULT ret = S_OK;

    char sql[1024] = { 0 };
    char* errMsg = nullptr;

    // "CREATE TABLE UsnInfo(SelfRef sqlite_uint64, ParentRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    std::string newPath = UnicodeToUtf8(pUsnInfo->filePathWstr);
    sprintf_s(sql, "UPDATE UsnInfo SET FilePath = '%s' WHERE SelfRef = %llu;", newPath.c_str(), selfRef);
    if (sqlite3_exec(m_hSQLite, sql, NULL, NULL, &errMsg) != SQLITE_OK)
    {
        ret = E_FAIL;
        printf("sqlite : update failed\n");
        printf("error : %s\n", errMsg);
    }
    else
    {
        printf("sqlite : update done\n");
    }

    return ret;
}

HRESULT MiniThing::SQLiteClose(VOID)
{
    sqlite3_close_v2(m_hSQLite);

    return S_OK;
}

