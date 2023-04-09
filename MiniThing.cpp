#include "MiniThing.h"
#include "Utility/Utility.h"

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

    int len = WstringToChar(m_volumeName, nullptr);
    char* pVol = new char[len];
    WstringToChar(m_volumeName, pVol);

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
        std::cout << " File system name : " << sysNameBuf << std::endl;

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

                    m_usnRecordMap[usnInfo.pSelfRef] = usnInfo;

                    // Store to SQLite
                    SQLiteInsert(&usnInfo);
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

    if(FALSE)
    {
        // "CREATE TABLE UsnInfo(ParentRef sqlite_uint64, SelfRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
        char search[1024] = { 0 };
        sprintf_s(search, "SELECT * FROM UsnInfo WHERE SelfRef = %llu;", currentRef);
        sqlite3_stmt* stmt = NULL;

        /*将sql语句转换为sqlite3可识别的语句，返回指针到stmt*/
        int res = sqlite3_prepare_v2(m_hSQLite, search, strlen(search), &stmt, NULL);

        if (SQLITE_OK != res || NULL == stmt)
        {
            std::cout << "Prepare SQLite search failed" << std::endl;
            assert(0);
        }

        /*执行准备好的sqlite3语句*/
        while (SQLITE_ROW == sqlite3_step(stmt))
        {
            const unsigned char* pstr = sqlite3_column_text(stmt, 4);
            if (!pstr)
            {
                std::cout << "SQLite : query info failed" << std::endl;
                assert(0);
            }

            printf("SQLite : query file path : %s\n", pstr);
        }

        sqlite3_finalize(stmt);
    }

    std::wstring str = m_usnRecordMap[currentRef].fileNameWstr;
    path = str + L"\\" + path;
    GetCurrentFilePath(path, m_usnRecordMap[currentRef].pParentRef, rootRef);
}

HRESULT MiniThing::SortUsn(VOID)
{
    HRESULT ret = S_OK;

    // Get "System Volume Information"'s parent ref number
    //      cause it's under top level folder
    //      so its parent ref number is top level folder's ref number
    std::wstring cmpStr(L"System Volume Information");
    DWORDLONG topLevelRefNum = 0x0;

    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        if (0 == usnInfo.fileNameWstr.compare(cmpStr))
        {
            topLevelRefNum = usnInfo.pParentRef;
            break;
        }
    }

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

        GetCurrentFilePath(path, usnInfo.pParentRef, topLevelRefNum);

        m_usnRecordMap[usnInfo.pSelfRef].filePathWstr = path;
    }

#if _DEBUG
    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        // 打印获取到的文件信息
        std::wcout << std::endl << L"File name : " << usnInfo.fileNameWstr << std::endl;
        std::wcout << L"File path : " << usnInfo.filePathWstr << std::endl;
        std::cout << "File ref number : 0x" << std::hex << usnInfo.pSelfRef << std::endl;
        std::cout << "File parent ref number : 0x" << std::hex << usnInfo.pParentRef << std::endl << std::endl;
    }
#endif

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

        m_usnRecordMap[tmp.pSelfRef] = tmp;
        break;
    }

    case FILE_ACTION_RENAMED_OLD_NAME:
    {
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
        // TODO
        //      file name rewrite
        m_usnRecordMap[it->first] = tmp;

        break;
    }

    case FILE_ACTION_REMOVED:
    {
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
                    wstring data;
                    // TODO
                    //      F:
                    data.clear();
                    data.append(L"F:\\");
                    data.append(fileNameWstr);

                    std::wcout << L"Add file : " << data << std::endl;
                    std::wcout << flush;

                    pMiniThing->AdjustUsnRecord(L"F:\\", data, L"", FILE_ACTION_ADDED);
                }
                break;

            case FILE_ACTION_MODIFIED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos &&
                    fileNameWstr.find(L"fileAdded.txt") == wstring::npos &&
                    fileNameWstr.find(L"fileRemoved.txt") == wstring::npos)
                {
                    wstring tmp;
                    tmp.append(L"F:\\");
                    tmp.append(fileNameWstr);
                    std::wcout << L"Add file : " << tmp << std::endl;
                    std::wcout << flush;
                    //add_record(to_utf8(StringToWString(data)));
                }
                break;

            case FILE_ACTION_REMOVED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    wstring tmp;
                    tmp.clear();
                    tmp.append(L"F:\\");
                    tmp.append(fileNameWstr);

                    std::wcout << L"Remove file : " << tmp << std::endl;
                    std::wcout << flush;

                    pMiniThing->AdjustUsnRecord(L"F:\\", tmp, L"", FILE_ACTION_REMOVED);
                }
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    wstring tmp;
                    tmp.clear();
                    tmp.append(L"F:\\");
                    tmp.append(fileNameWstr);

                    wstring tmpRename;
                    tmpRename.clear();
                    tmpRename.append(L"F:\\");
                    tmpRename.append(fileRenameWstr);

                    std::wcout << L"Rename file : " << tmp << L"->" << tmpRename << std::endl;
                    std::wcout << flush;

                    pMiniThing->AdjustUsnRecord(L"F:\\", tmp, tmpRename, FILE_ACTION_RENAMED_OLD_NAME);
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

BOOL MiniThing::CreateMonitorThread(VOID)
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

HRESULT MiniThing::SQLiteOpen(CONST CHAR *path)
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

    // "CREATE TABLE UsnInfo(ParentRef sqlite_uint64, SelfRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    char insert[1024] = { 0 };
    sprintf_s(insert, "INSERT INTO UsnInfo VALUES(%llu, %llu, %lld, '%s', '%s');",
        pUsnInfo->pSelfRef, pUsnInfo->pParentRef, pUsnInfo->timeStamp, pUsnInfo->fileNameStr.c_str(), pUsnInfo->filePathStr.c_str());

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_hSQLite, insert, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQLite : insert table failed" << std::endl;
        ret = S_FALSE;
    }

    return ret;
}

HRESULT MiniThing::SQLiteQuery(UsnInfo* pUsnInfo)
{
    HRESULT ret = S_OK;
    return ret;
}

HRESULT MiniThing::SQLiteDelete(UsnInfo* pUsnInfo)
{
    return E_NOTIMPL;
}

HRESULT MiniThing::SQLiteModify(UsnInfo* pUsnInfo)
{
    return E_NOTIMPL;
}

HRESULT MiniThing::SQLiteClose(VOID)
{
    sqlite3_close_v2(m_hSQLite);

    return S_OK;
}

VOID MiniThing::MonitorFileChange(VOID)
{
    char notifyInfo[1024];

    wstring fileNameWstr;
    wstring fileRenameWstr;

    std::wstring folderPath = L"\\\\.\\";
    folderPath += m_volumeName;

    std::wcout << L"Start monitor" << folderPath << std::endl;

    

    HANDLE dirHandle = CreateFile(folderPath.c_str(),
        GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    // 若网络重定向或目标文件系统不支持该操作，函数失败，同时调用GetLastError()返回ERROR_INVALID_FUNCTION
    if (dirHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "error " << GetLastError() << std::endl;
        exit(0);
    }

    auto* pNotifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(notifyInfo);

    while (true)
    {
        DWORD retBytes;

        if (ReadDirectoryChangesW(dirHandle, &notifyInfo, 1024, true,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
            &retBytes, nullptr, nullptr))
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
                    wstring data;
                    // TODO
                    //      F:
                    data.append(L"F:\\");
                    data.append(fileNameWstr);
                    std::wcout << L"Add file : " << data << std::endl;
                    AdjustUsnRecord(L"F:\\", data, L"", FILE_ACTION_ADDED);
                }
                break;

            case FILE_ACTION_MODIFIED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos &&
                    fileNameWstr.find(L"fileAdded.txt") == wstring::npos &&
                    fileNameWstr.find(L"fileRemoved.txt") == wstring::npos)
                {
                    wstring tmp;
                    tmp.append(L"F:\\");
                    tmp.append(fileNameWstr);
                    std::wcout << L"Add file : " << tmp << std::endl;
                    //add_record(to_utf8(StringToWString(data)));
                }
                break;

            case FILE_ACTION_REMOVED:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    wstring tmp;
                    tmp.clear();
                    tmp.append(L"F:\\");
                    tmp.append(fileNameWstr);

                    std::wcout << L"Remove file : " << tmp << std::endl;

                    AdjustUsnRecord(L"F:\\", tmp, L"", FILE_ACTION_REMOVED);
                }
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                if (fileNameWstr.find(L"$RECYCLE.BIN") == wstring::npos)
                {
                    wstring tmp;
                    tmp.clear();
                    tmp.append(L"F:\\");
                    tmp.append(fileNameWstr);

                    wstring tmpRename;
                    tmpRename.clear();
                    tmpRename.append(L"F:\\");
                    tmpRename.append(fileRenameWstr);

                    std::wcout << L"file renamed : " << tmp << "->" << tmpRename << std::endl;

                    AdjustUsnRecord(L"F:\\", tmp, tmpRename, FILE_ACTION_RENAMED_OLD_NAME);
                }
                break;

            default:
                std::wcout << L"Unknown command" << std::endl;
            }
        }
    }

    CloseHandle(dirHandle);

    std::wcout << L"Stop monitor" << folderPath << std::endl;

    return;
}

