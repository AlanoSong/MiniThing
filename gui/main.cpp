#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

#include "cmdline.h"
#include "MiniThingCore.h"

void UpdateProgressCb(
    const std::string str
)
{
    std::cout << str << std::endl;
}

int
main(
    int argc,
    char *argv[]
)
{
    // Load DLL from the same directory as the executable
    PFN_MTC_CREATE pfnMtcCreate = NULL;
    PFN_MTC_DESTROY pfnMtcDestroy = NULL;
    HMODULE hMtcDll = LoadLibraryA("MiniThingCore.dll");

    if (hMtcDll == NULL)
    {
        std::cerr << "Failed to load dll" << std::endl;
        return 0;
    }

    // Get function addresses
    pfnMtcCreate = (PFN_MTC_CREATE)GetProcAddress(hMtcDll, "MTC_Create");
    pfnMtcDestroy = (PFN_MTC_DESTROY)GetProcAddress(hMtcDll, "MTC_Destroy");

    // Call MTC_Create
    void* instance = pfnMtcCreate((PVOID)UpdateProgressCb);

    // Call MTC_Destroy
    pfnMtcDestroy(instance);

    // Unload DLL
    FreeLibrary(hMtcDll);

    return 0;
}
