#include "MiniThing.h"


USN_JOURNAL_DATA m_usnInfo;

MiniThing::MiniThing(string volumeName)
{
    m_volumeName = volumeName;
}

MiniThing::~MiniThing(VOID)
{
}

CString MiniThing::StrToCstr(string str)
{
    return CString(str.c_str());
}

LPCWSTR MiniThing::StrToLPCWSTR(string orig)
{
    size_t origsize = orig.length() + 1;
    const size_t newsize = 100;
    size_t convertedChars = 0;
    wchar_t* wcstring = (wchar_t*)malloc(sizeof(wchar_t) * (orig.length() - 1));
    mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

    return wcstring;
}

LPCSTR MiniThing::StrToLPCSTR(string str)
{
    return str.c_str();
}

string MiniThing::LPCWSTRToStr(LPCWSTR lpcwszStr)
{
    string str;
    DWORD dwMinSize = 0;
    LPSTR lpszStr = NULL;
    dwMinSize = WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE);
    if (0 == dwMinSize)
    {
        return FALSE;
    }
    lpszStr = new char[dwMinSize];
    WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, lpszStr, dwMinSize, NULL, FALSE);
    str = lpszStr;
    delete[] lpszStr;
    return str;
}

BOOL MiniThing::IsWstringSame(wstring s1, wstring s2)
{
    CString c1 = s1.c_str();
    CString c2 = s2.c_str();

    return c1.CompareNoCase(c2) == 0 ? TRUE : FALSE;
}

BOOL MiniThing::GetHandle()
{
    CString folderPath = _T("\\\\.\\");
    folderPath += StrToCstr(m_volumeName);

    m_hVol = CreateFile(folderPath,
        GENERIC_READ | GENERIC_WRITE, // 可以为0
        FILE_SHARE_READ | FILE_SHARE_WRITE, // 必须包含有FILE_SHARE_WRITE
        nullptr,
        OPEN_EXISTING, // 必须包含OPEN_EXISTING, CREATE_ALWAYS可能会导致错误
        FILE_FLAG_BACKUP_SEMANTICS, // FILE_ATTRIBUTE_NORMAL可能会导致错误
        nullptr);

    if (INVALID_HANDLE_VALUE != m_hVol)
    {
        cout << "Get handle : " << m_volumeName << endl;
        return TRUE;
    }
    else
    {
        cout << "Get handle failed : " << m_volumeName << endl;
        return FALSE;
    }
}

VOID MiniThing::closeHandle(VOID)
{
    if (TRUE == CloseHandle(m_hVol))
    {
        cout << "Close handle : " << m_volumeName << endl;
    }
    else
    {
        cout << "Get handle : " << m_volumeName << endl;
    }
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

    setlocale(LC_ALL, "chs");

    LPCTSTR strValue = lpMsgBuf;
    _tprintf(_T("ERROR msg ：%s"), strValue);
}

BOOL MiniThing::IsNtfs(VOID)
{
    BOOL isNtfs = FALSE;
    char sysNameBuf[MAX_PATH] = { 0 };

    BOOL status = GetVolumeInformationA(
        StrToLPCSTR(m_volumeName),
        NULL,
        0,
        NULL,
        NULL,
        NULL,
        sysNameBuf,
        MAX_PATH);

    if (FALSE != status) {

        cout << " File system name : " << sysNameBuf << endl;

        if (0 == strcmp(sysNameBuf, "NTFS"))
        {
            isNtfs = true;
        }
        else
        {
            cout << "File system not NTFS format !!!" << endl;
            GetSystemError();
        }
    }

    return isNtfs;
}

BOOL MiniThing::CreateUsn(VOID)
{
    DWORD br;
    CREATE_USN_JOURNAL_DATA cujd;
    cujd.MaximumSize = 0; // 0表示使用默认值  
    cujd.AllocationDelta = 0; // 0表示使用默认值  
    BOOL status = DeviceIoControl(m_hVol,
        FSCTL_CREATE_USN_JOURNAL,
        &cujd,
        sizeof(cujd),
        NULL,
        0,
        &br,
        NULL);

    if (FALSE != status)
    {
        cout << "Create usn file success" << endl;
        return status;
    }
    else
    {
        cout << "Create usn file failed" << endl;
        GetSystemError();
        return false;
    }
}

BOOL MiniThing::QueryUsn(VOID)
{
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
        cout << "Query usn info success" << endl;
    }
    else
    {
        cout << "Query usn info failed" << endl;
        GetSystemError();
    }

    return TRUE;
}

BOOL MiniThing::RecordUsn(VOID)
{
    MFT_ENUM_DATA med = { 0, 0, m_usnInfo.NextUsn };
    med.MaxMajorVersion = 2;
    char buffer[0x1000]; // Used to record usn info, must big enough
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
            const int strLen = pUsnRecord->FileNameLength;
            char fileName[MAX_PATH] = { 0 };
            WideCharToMultiByte(CP_OEMCP, NULL, pUsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);

            UINT iSize = MultiByteToWideChar(CP_ACP, 0, fileName, -1, NULL, 0);
            wchar_t* fileNameWstr = (wchar_t*)malloc(iSize * sizeof(wchar_t));
            MultiByteToWideChar(CP_ACP, 0, fileName, -1, fileNameWstr, iSize);

            string fileNameStr = fileName;


            // Several system file need be filtered
            if (find(m_systemParentRef.begin(), m_systemParentRef.end(), pUsnRecord->ParentFileReferenceNumber) == m_systemParentRef.end())
            {
                UsnInfo usnInfo = { 0 };
                usnInfo.fileNameStr = fileNameStr;
                usnInfo.fileNameWstr = fileNameWstr;
                usnInfo.pParentRef = pUsnRecord->ParentFileReferenceNumber;
                usnInfo.pSelfRef = pUsnRecord->FileReferenceNumber;
                usnInfo.timeStamp = pUsnRecord->TimeStamp;
#if _DEBUG
                // 打印获取到的文件信息
                cout << "File name : " << fileNameStr << endl;
                cout << "File ref number : " << pUsnRecord->FileReferenceNumber << endl;
                cout << "File parent ref number : " << pUsnRecord->ParentFileReferenceNumber << endl;
#endif
                m_usnRecordMap[usnInfo.pSelfRef] = usnInfo;

                // Store to SQLite
                SQLiteInsert(&usnInfo);
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

    return TRUE;
}

VOID MiniThing::GetCurrentFilePath(wstring& path, DWORDLONG currentRef, DWORDLONG rootRef)
{
    if (currentRef == rootRef)
    {
        // TODO
        // m_volumeName -> wstring
        path = L"F:\\" + path;
        return;
    }

    // "CREATE TABLE UsnInfo(ParentRef sqlite_uint64, SelfRef sqlite_uint64, TimeStamp sqlite_int64, FileName TEXT, FilePath TEXT);"
    char search[1024] = { 0 };
    sprintf_s(search, "SELECT * FROM UsnInfo WHERE SelfRef = %llu;", currentRef);
    sqlite3_stmt* stmt = NULL;

    /*将sql语句转换为sqlite3可识别的语句，返回指针到stmt*/
    int res = sqlite3_prepare_v2(m_hSQLite, search, strlen(search), &stmt, NULL);

    if (SQLITE_OK != res || NULL == stmt)
    {
        std::cout << "Prepare SQLite search failed" << endl;
        assert(0);
    }

    /*执行准备好的sqlite3语句*/
    while (SQLITE_ROW == sqlite3_step(stmt))
    {
        const unsigned char *pstr = sqlite3_column_text(stmt, 4);
        if (!pstr)
        {
            cout << "SQLite : query info failed" << endl;
            assert(0);
        }

        printf("SQLite : query file path : %s\n", pstr);
    }

    sqlite3_finalize(stmt);

    wstring str = m_usnRecordMap[currentRef].fileNameWstr;
    path = str + L"\\" + path;
    GetCurrentFilePath(path, m_usnRecordMap[currentRef].pParentRef, rootRef);
}

VOID MiniThing::SortUsn(VOID)
{
    // Get "System Volume Information"'s parent ref number
    //      cause it's under top level folder
    //      so its parent ref number is top level folder's ref number
    wstring cmpStr(L"System Volume Information");
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
        cout << "Cannot find root folder" << endl;
        return;
    }

    for (auto it = m_usnRecordMap.begin(); it != m_usnRecordMap.end(); it++)
    {
        UsnInfo usnInfo = it->second;
        wstring path(m_usnRecordMap[usnInfo.pSelfRef].fileNameWstr);

        GetCurrentFilePath(path, usnInfo.pParentRef, topLevelRefNum);

        m_usnRecordMap[usnInfo.pSelfRef].filePathWstr = path;

        // wcout << L"File [" << usnInfo.fileNameWstr << L"] path: " << path << endl;
    }
}

VOID MiniThing::DeleteUsn(VOID)
{
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
        cout << "Delete usn file success" << endl;
    }
    else
    {
        GetSystemError();
        cout << "Delete usn file failed" << endl;
    }
}

wstring MiniThing::GetFileNameAccordPath(wstring path)
{
    return path.substr(path.find_last_of(L"\\") + 1);
}

wstring MiniThing::GetPathAccordPath(wstring path)
{
    wstring name = GetFileNameAccordPath(path);
    return path.substr(0, path.length() - name.length());
}

VOID MiniThing::AdjustUsnRecord(wstring folder, wstring filePath, wstring reFileName, DWORD op)
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
            cout << "Failed find rename file" << endl;
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

    // Set up Chinese output env
    wcout.imbue(std::locale("chs"));

    cout << "Monitor thread start" << endl;

    char notifyInfo[1024];

    wstring fileNameWstr;
    wstring fileRenameWstr;

    cout << "Start monitor : " << pMiniThing->m_volumeName << endl;

    CString folderPath = pMiniThing->StrToCstr(pMiniThing->m_volumeName);
    HANDLE dirHandle = CreateFile(folderPath,
        GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    // 若网络重定向或目标文件系统不支持该操作，函数失败，同时调用GetLastError()返回ERROR_INVALID_FUNCTION
    if (dirHandle == INVALID_HANDLE_VALUE)
    {
        cout << "error " << GetLastError() << endl;
        exit(0);
    }

    auto* pNotifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(notifyInfo);

    while (true)
    {
        // Check if need exit thread
        DWORD dwWaitCode = WaitForSingleObject(pMiniThing->m_hExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            cout << "Recv the quit event" << endl;
            break;
        }

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
                    data.clear();
                    data.append(L"F:\\");
                    data.append(fileNameWstr);

                    wcout << L"Add file : " << data << endl;
                    wcout << flush;

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
                    wcout << L"Add file : " << tmp << endl;
                    wcout << flush;
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

                    wcout << L"Remove file : " << tmp << endl;
                    wcout << flush;

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

                    wcout << L"Rename file : " << tmp << L"->" << tmpRename << endl;
                    wcout << flush;

                    pMiniThing->AdjustUsnRecord(L"F:\\", tmp, tmpRename, FILE_ACTION_RENAMED_OLD_NAME);
                }
                break;

            default:
                wcout << L"Unknown command" << endl;
            }
        }
    }

    CloseHandle(dirHandle);

    cout << "Monitor thread stop" << endl;

    return 0;
}

BOOL MiniThing::CreateMonitorThread(VOID)
{
    m_hExitEvent = CreateEvent(0, 0, 0, 0);

    // 以挂起方式创建线程
    m_hMonitorThread = CreateThread(0, 0, MonitorThread, this, CREATE_SUSPENDED, 0);
    if (INVALID_HANDLE_VALUE == m_hMonitorThread)
    {
        GetLastError();
        return FALSE;
    }

    return TRUE;
}

VOID MiniThing::StartMonitorThread(VOID)
{
    // 使线程开始运行
    ResumeThread(m_hMonitorThread);

    return VOID();
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

    return VOID();
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

            ret = S_FALSE;
        }

        cout << "SQLite : open success" << endl;
    }
    else
    {
        ret = S_FALSE;
        cout << "SQLite : open failed" << endl;
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
    CString folderPath = StrToCstr(m_volumeName);

    char notifyInfo[1024];

    wstring fileNameWstr;
    wstring fileRenameWstr;

    wcout << L"Start monitor" << folderPath << endl;

    HANDLE dirHandle = CreateFile(folderPath,
        GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    // 若网络重定向或目标文件系统不支持该操作，函数失败，同时调用GetLastError()返回ERROR_INVALID_FUNCTION
    if (dirHandle == INVALID_HANDLE_VALUE)
    {
        cout << "error " << GetLastError() << endl;
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
                    wcout << L"Add file : " << data << endl;
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
                    wcout << L"Add file : " << tmp << endl;
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

                    wcout << L"Remove file : " << tmp << endl;

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

                    wcout << L"file renamed : " << tmp << "->" << tmpRename << endl;

                    AdjustUsnRecord(L"F:\\", tmp, tmpRename, FILE_ACTION_RENAMED_OLD_NAME);
                }
                break;

            default:
                wcout << L"Unknown command" << endl;
            }
        }
    }

    CloseHandle(dirHandle);

    wcout << L"Stop monitor" << folderPath << endl;

    return;
}

