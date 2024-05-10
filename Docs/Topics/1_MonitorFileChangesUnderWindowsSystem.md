# Monitor File Changes Under Windows System

## Method 1

### Details
- Windows system API: `ReadDirectoryChangesW()`
- Like `DWORD WINAPI MonitorThread(LPVOID lp)` in `TaskThreads.cpp`
- Firstly we create a handle for each volume in system, like `C:\`
- Then we monitor `C:\` with the handle by `ReadDirectoryChangesW()`
- We can set many flags when we monitor the volume, like `FILE_NOTIFY_CHANGE_FILE_NAME`, `FILE_NOTIFY_CHANGE_DIR_NAME`, `FILE_NOTIFY_CHANGE_SIZE`, and we only concern file add, delete, modify
```cpp
// hVolume is the volume handle
// notifyInfo is a FILE_NOTIFY_INFORMATION structure, system will populate data in it
ReadDirectoryChangesW(
            hVolume,
            &notifyInfo,
            sizeof(notifyInfo),
            true, // Monitor sub directory
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
            &retBytes,
            nullptr,
            nullptr)
```
- We call `ReadDirectoryChangesW()` in a sub thread, in this thread we use this function in a dead loop until receive exit event
- After we receive the file changes event, we won't handle it immediately in monitor thread
- Cause there are too many event in system, we may lost other events if we cost time to handle current event.
- So we just push this event into a global task queue `g_updateDataBaseTaskList`, and handle those events one by one in another thread `UpdateSqlDataBaseThread`

### Advantages
- It's very simple to carry on, cause Windows system offers everything

### Disadvantages
- Sometimes we cannot receive file change events under a specail volume, such as `C:`, Windows system won't send the event ? or there are bugs in this software ? don't know

## Method 2

- So how does `Everything` monitor files change events ? need some time to analyse
- To be continue ...