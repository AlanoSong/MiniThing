#include "TaskThreads.h"
#include "../Utility/Utility.h"

#define LOG_TAG      "TaskThreads"

//==========================================================================
//                        Static Parameters                               //
//==========================================================================

//==========================================================================
//                        Public Parameters                               //
//==========================================================================
std::list<UpdateDataBaseTaskInfo> g_updateDataBaseTaskList;
HANDLE g_updateDataBaseWrMutex;

//==========================================================================
//                        Static Functions                                //
//==========================================================================

static void GetCurrentFilePath(std::wstring& path, std::wstring volName, DWORDLONG currentRef, DWORDLONG rootRef, std::unordered_map<DWORDLONG, UsnInfo>& recordMapAll)
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
        GetCurrentFilePath(path, volName, recordMapAll[currentRef].pParentRef, rootRef, recordMapAll);
    }
    else
    {
        // 3. Some special system files's root node is not in current folder
        std::wstring str = L"?";
        path = str + L"\\" + path;

        return;
    }
}

//==========================================================================
//                        Task Threads                                    //
//==========================================================================
DWORD WINAPI SortThread(LPVOID lp)
{
    SortTaskInfo* pTaskInfo = (SortTaskInfo*)lp;

    SetChsPrintEnv();

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
        GetCurrentFilePath(path, pTaskInfo->rootFolderName, usnInfo.pParentRef, pTaskInfo->rootRef, *(pTaskInfo->pAllUsnRecordMap));
        (*pTaskInfo->pSortTask)[it->first].filePathWstr = path;
    }

    QueryPerformanceCounter(&timeEnd);
    double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;

    log_i("Sort thread %d over, cost %f S", pTaskInfo->taskIndex, elapsed);

#if _DEBUG
    char tmpBuf[256];
    sprintf_s(tmpBuf, "Sort thread %d over, cost %f S", pTaskInfo->taskIndex, elapsed);
    pTaskInfo->m_statusUpdateCb(tmpBuf);
#endif

    return 0;
}

DWORD WINAPI UpdateSqlDataBaseThread(LPVOID lp)
{
    SetChsPrintEnv();

    MiniThingCore* pMiniThingCore = (MiniThingCore*)lp;

    log_i("Update sql thread start");

#if _DEBUG
    // emit pMiniThingCore->GetQtWorkThreadHandle()->UpdateStatusBar(QString("Update sql thread start"));
#endif

    while (true)
    {
        // Check if need exit thread
        DWORD dwWaitCode = WaitForSingleObject(pMiniThingCore->m_hUpdateSqlDataBaseExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            log_i("Recv the quit event");

#if _DEBUG
            pMiniThingCore->m_statusUpdateCb("Recv the quit event");
#endif
            break;
        }

        if (!g_updateDataBaseTaskList.empty())
        {
            WaitForSingleObject(g_updateDataBaseWrMutex, INFINITE);
            UpdateDataBaseTaskInfo taskInfo = g_updateDataBaseTaskList.front();
            g_updateDataBaseTaskList.pop_front();
            ReleaseMutex(g_updateDataBaseWrMutex);

            switch (taskInfo.op)
            {
            case FILE_ACTION_ADDED:
            {
                UsnInfo unsInfo = { 0 };
                unsInfo.filePathWstr = taskInfo.oriPath;
                unsInfo.fileNameWstr = GetFileNameAccordPath(taskInfo.oriPath);
                // TODO: assign valid ref
                unsInfo.pParentRef = 0;
                unsInfo.pSelfRef = 0;

                if (FAILED(((MiniThingCore*)taskInfo.pMiniThingCore)->SQLiteInsert(&unsInfo)))
                {
                    assert(0);
                }
                break;
            }

            case FILE_ACTION_RENAMED_OLD_NAME:
            {
                UsnInfo oriInfo = { 0 };
                oriInfo.fileNameWstr = GetFileNameAccordPath(taskInfo.oriPath);
                oriInfo.filePathWstr = taskInfo.oriPath;

                UsnInfo reInfo = { 0 };
                reInfo.fileNameWstr = GetFileNameAccordPath(taskInfo.newPath);
                reInfo.filePathWstr = taskInfo.newPath;

                if (FAILED(((MiniThingCore*)taskInfo.pMiniThingCore)->SQLiteUpdate(&oriInfo, &reInfo)))
                {
                    assert(0);
                }
                break;
            }

            case FILE_ACTION_REMOVED:
            {
                UsnInfo usnInfo = { 0 };
                usnInfo.filePathWstr = taskInfo.oriPath;
                if (FAILED(((MiniThingCore*)taskInfo.pMiniThingCore)->SQLiteDelete(&usnInfo)))
                {
                    assert(0);
                }
                break;
            }

            default:
                break;
            }
        }

        // Sleep for a while to reduce cpu usage ratio
        Sleep(500);
    }

    log_i("Stop update sql thread");

    return 0;
}

DWORD WINAPI MonitorThread(LPVOID lp)
{
    MonitorTaskInfo* pTaskInfo = (MonitorTaskInfo*)lp;
    VolumeInfo* pVolumeInfo = pTaskInfo->pVolumeInfo;
    MiniThingCore* pMiniThingCore = pTaskInfo->pMiniThingCore;
    std::wstring localAppDataPath = pTaskInfo->localAppDataPath;

    char notifyInfo[1024] = { 0 };

    std::wstring filePathWstr;
    std::wstring fileRePathWstr;
    std::wstring fullFilePathWstr;

    SetChsPrintEnv();

    log_i("Start monitor: %s", pVolumeInfo->volumeName.c_str());

    std::wstring folderPath = pVolumeInfo->volumeName;

    HANDLE hVolume = CreateFile(folderPath.c_str(),
        GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (hVolume == INVALID_HANDLE_VALUE)
    {
        GetSystemError();
        assert(0);
    }

    auto* pNotifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(notifyInfo);

    while (true)
    {
        // Check if need exit thread
        DWORD dwWaitCode = WaitForSingleObject(pVolumeInfo->hMonitorExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            log_i("Recv the quit event");
            break;
        }

        DWORD retBytes;

        if (ReadDirectoryChangesW(
            hVolume,
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

            // There is too many events about appdata, filter them
            fullFilePathWstr = pVolumeInfo->volumeName + L"\\" + filePathWstr;
            if (fullFilePathWstr.find(localAppDataPath) != std::wstring::npos
                // TODO: refine this
                || fullFilePathWstr.find(L"MiniThing.db-journal") != std::wstring::npos)
            {
                continue;
            }

            // Rename file new name
            auto pInfo = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(reinterpret_cast<char*>(pNotifyInfo) + pNotifyInfo->NextEntryOffset);
            fileRePathWstr = pInfo->FileName;
            fileRePathWstr.resize(pInfo->FileNameLength / 2);

            // We delete pTaskInfo in update thread when task over
            UpdateDataBaseTaskInfo updateTaskInfo;
            updateTaskInfo.pVolumeInfo = pVolumeInfo;
            updateTaskInfo.pMiniThingCore = pMiniThingCore;
            updateTaskInfo.op = 0;

            switch (pNotifyInfo->Action)
            {
            case FILE_ACTION_ADDED:
                if (filePathWstr.find(L"$RECYCLE.BIN") == std::wstring::npos)
                {
                    std::wstring addPath;
                    addPath.clear();
                    addPath.append(pVolumeInfo->volumeName);
                    addPath.append(L"\\");
                    addPath.append(filePathWstr);

                    updateTaskInfo.op = FILE_ACTION_ADDED;
                    updateTaskInfo.oriPath = addPath;
                }
                break;

            case FILE_ACTION_MODIFIED:
                if (filePathWstr.find(L"$RECYCLE.BIN") == std::wstring::npos &&
                    filePathWstr.find(L"fileAdded.txt") == std::wstring::npos &&
                    filePathWstr.find(L"fileRemoved.txt") == std::wstring::npos)
                {
                    std::wstring modPath;
                    modPath.append(pVolumeInfo->volumeName);
                    modPath.append(L"\\");
                    modPath.append(filePathWstr);

                    // Must use printf_s here cuase cout is not thread safety
                    // wprintf_s(L"Mod file : %s\n", modPath.c_str());
                }
                break;

            case FILE_ACTION_REMOVED:
                if (filePathWstr.find(L"$RECYCLE.BIN") == std::wstring::npos)
                {
                    std::wstring remPath;
                    remPath.clear();
                    remPath.append(pVolumeInfo->volumeName);
                    remPath.append(L"\\");
                    remPath.append(filePathWstr);

                    updateTaskInfo.op = FILE_ACTION_REMOVED;
                    updateTaskInfo.oriPath = remPath;
                }
                break;

            case FILE_ACTION_RENAMED_OLD_NAME:
                if (filePathWstr.find(L"$RECYCLE.BIN") == std::wstring::npos)
                {
                    std::wstring oriPath;
                    oriPath.clear();
                    oriPath.append(pVolumeInfo->volumeName);
                    oriPath.append(L"\\");
                    oriPath.append(filePathWstr);

                    std::wstring rePath;
                    rePath.clear();
                    rePath.append(pVolumeInfo->volumeName);
                    rePath.append(L"\\");
                    rePath.append(fileRePathWstr);

                    updateTaskInfo.op = FILE_ACTION_RENAMED_OLD_NAME;
                    updateTaskInfo.oriPath = oriPath;
                    updateTaskInfo.newPath = rePath;
                }
                break;

            default:
                break;
            }

            if (updateTaskInfo.op)
            {
                // We dispath those update task into list and handle them in UpdateSqlDataBaseThread,
                //  cause updating sql data base may consume some time,
                //  and we may lost more file change event at the same time.
                WaitForSingleObject(g_updateDataBaseWrMutex, INFINITE);
                g_updateDataBaseTaskList.push_back(updateTaskInfo);
                ReleaseMutex(g_updateDataBaseWrMutex);
            }
        }

        // Sleep for a while to reduce cpu usage ratio
        Sleep(100);
    }

    CloseHandle(hVolume);

    log_i("Stop monitor: %s\n", pVolumeInfo->volumeName.c_str());

    return 0;
}

DWORD WINAPI QueryThread(LPVOID lp)
{
    MiniThingCore* pMiniThingCore = (MiniThingCore*)lp;

    SetChsPrintEnv();

    log_i("Query thread start");

#if _DEBUG
    printf_s("Query thread start\n");
#endif

    while (true)
    {
        // Check if need exit thread
        DWORD dwWaitCode = WaitForSingleObject(pMiniThingCore->m_hQueryExitEvent, 0x0);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            log_i("Recv the quit event");
#if _DEBUG
            printf_s("Recv the quit event\n");
#endif
            break;
        }

        printf_s("\n==============================\n");
        printf_s("Input file name: ");

        std::wstring query;
        std::wcin >> query;

        std::vector<std::wstring> vec;

        LARGE_INTEGER timeStart;
        LARGE_INTEGER timeEnd;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double quadpart = (double)frequency.QuadPart;

        QueryPerformanceCounter(&timeStart);

        pMiniThingCore->SQLiteQuery(query, vec);

        QueryPerformanceCounter(&timeEnd);
        double elapsed = (timeEnd.QuadPart - timeStart.QuadPart) / quadpart;
        printf_s("Time elasped: %f S\n", elapsed);

        if (vec.empty())
        {
            printf_s("Not found\n");
        }
        else
        {
            int cnt = 0;
            for (auto it = vec.begin(); it != vec.end(); it++)
            {
                wprintf_s(L"NO.%d\t: %s\n", cnt++, (*it).c_str());
            }
        }
        printf_s("==============================\n");
    }

    log_i("Query thread stop");

#if _DEBUG
    printf_s("Query thread stop\n");
#endif

    return 0;
}

