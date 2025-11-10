#include "core.h"

EXTERN_C_START
PVOID
WINAPI
MTC_Create(
    PVOID printCb
)
{
    MiniThingCore* pMTC = new MiniThingCore();
    pMTC->StartInstance((PFN_UPDATE_STATUS_CB)printCb);

    return pMTC;
}
EXTERN_C_END

EXTERN_C_START
VOID
WINAPI
MTC_Destroy(
    PVOID pCore
)
{
    MiniThingCore* pMTC = (MiniThingCore*)pCore;
    delete pMTC;
}
EXTERN_C_END