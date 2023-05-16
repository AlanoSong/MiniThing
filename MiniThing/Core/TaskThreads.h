#pragma once

#include "MiniThingCore.h"

DWORD WINAPI SortThread(LPVOID lp);
DWORD WINAPI UpdateSqlDataBaseThread(LPVOID lp);
DWORD WINAPI MonitorThread(LPVOID lp);
DWORD WINAPI QueryThread(LPVOID lp);