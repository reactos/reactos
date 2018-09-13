#include "shprv.h"

//
// BUGBUG:: This temporary Thunk implementation is because there currently
// is no legal way to do a Restart windows from the 32 bit side...
BOOL WINAPI SHRestartWindows(DWORD dwReturn)
{
    return ExitWindows(dwReturn, 0);
}

//
// Getting information to fill in the about box.
//
VOID WINAPI SHGetAboutInformation(LPWORD puSysResource, LPDWORD pcbFree)
{
    if (puSysResource)
    {
        *puSysResource = GetFreeSystemResources(0);
    }

    if (pcbFree)
    {
        *pcbFree = GetFreeSpace(GFS_PHYSICALRAMSIZE);
    }
}
