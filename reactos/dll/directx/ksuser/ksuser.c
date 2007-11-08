

#define _KSDDK_
#include <windows.h>
#include <ks.h>
#include "ksuser.h"

#include <debug.h>




KSDDKAPI
NTSTATUS
NTAPI
KsCreateAllocator(HANDLE ConnectionHandle,
                  PKSALLOCATOR_FRAMING AllocatorFraming,
                  PHANDLE AllocatorHandle)
{

    return 0;
}

/*
KSDDKAPI
NTSTATUS
NTAPI
KsCreateClock(HANDLE ConnectionHandle,
              PKSCLOCK_CREATE ClockCreate,
              PHANDLE  ClockHandle)
{
    UNIMPLEMENTED
    return 0;
}

KSDDKAPI
NTSTATUS
NTAPI
KsCreatePin(HANDLE FilterHandle,
            PKSPIN_CONNECT Connect,
            ACCESS_MASK DesiredAccess,
            OUT PHANDLE  ConnectionHandle)
{
    UNIMPLEMENTED
    return 0;
}

KSDDKAPI
NTSTATUS
NTAPI
KsCreateTopologyNode(HANDLE ParentHandle,
                     PKSNODE_CREATE NodeCreate,
                     IN ACCESS_MASK DesiredAccess,
                     OUT PHANDLE NodeHandle)
{
    UNIMPLEMENTED
    return 0;
}
*/

BOOL 
APIENTRY 
DllMain(HANDLE hModule, DWORD ulreason, LPVOID lpReserved)
{
    switch (ulreason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
