

#include "ksuser.h"

NTSTATUS NTAPI  KsiCreateObjectType( HANDLE hHandle, PVOID guidstr, PVOID Buffer, ULONG BufferSize, ACCESS_MASK DesiredAccess, PHANDLE phHandle);

NTSTATUS
NTAPI
KsiCreateObjectType( HANDLE hHandle,
                     PVOID IID,
                     PVOID Buffer,
                     ULONG BufferSize,
                     ACCESS_MASK DesiredAccess,
                     PHANDLE phHandle)
{
    UNIMPLEMENTED
    return 0;
}

/*++
* @name KsCreateAllocator
* @implemented
*
* The function KsCreateAllocator
*
* @param 
*
* @param 
*
* @param ACCESS_MASK  DesiredAccess
* Desrided access
*
* @param 
*
* @return 
* Return NTSTATUS error code or sussess code.
*
* @remarks.
* none
*
*--*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateAllocator(HANDLE ConnectionHandle,
                  PKSALLOCATOR_FRAMING AllocatorFraming,
                  PHANDLE AllocatorHandle)

{
    return KsiCreateObjectType( ConnectionHandle,
                                KSSTRING_Allocator,
                                (PVOID) AllocatorFraming,
                                sizeof(KSALLOCATOR_FRAMING),
                                GENERIC_READ,
                                AllocatorHandle);
}

/*++
* @name KsCreateClock
* @implemented
*
* The function KsCreateClock
*
* @param 
*
* @param 
*
* @param ACCESS_MASK  DesiredAccess
* Desrided access
*
* @param 
*
* @return 
* Return NTSTATUS error code or sussess code.
*
* @remarks.
* none
*
*--*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateClock(HANDLE ConnectionHandle,
              PKSCLOCK_CREATE ClockCreate,
              PHANDLE  ClockHandle)
{
    return KsiCreateObjectType( ConnectionHandle,
                                KSSTRING_Clock,
                                (PVOID) ClockCreate,
                                sizeof(KSCLOCK_CREATE),
                                GENERIC_READ,
                                ClockHandle);
}



/*++
* @name KsCreatePin
* @implemented
*
* The function KsCreatePin
*
* @param 
*
* @param 
*
* @param ACCESS_MASK  DesiredAccess
* Desrided access
*
* @param 
*
* @return 
* Return NTSTATUS error code or sussess code.
*
* @remarks.
* none
*
*--*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreatePin(HANDLE FilterHandle,
            PKSPIN_CONNECT Connect,
            ACCESS_MASK DesiredAccess,
            OUT PHANDLE  ConnectionHandle)
{
    return KsiCreateObjectType(FilterHandle,
                               KSSTRING_Pin,
                               Connect,
                               sizeof(KSPIN_CONNECT),
                               DesiredAccess,
                               ConnectionHandle);

}

/*++
* @name KsCreateTopologyNode
* @implemented
*
* The function KsCreateTopologyNode  creates a handle to a topology node instance 
*
* @param HANDLE ParentHandle
* Handle to parent when want to use when we created the node on
* 
*
* @param PKSNODE_CREATE  NodeCreate
* topology node parameters to use when it is create
*
* @param ACCESS_MASK  DesiredAccess
* Desrided access
*
* @param PHANDLE  NodeHandle
* Location for the topology node handle
*
* @return 
* Return NTSTATUS error code or sussess code.
*
* @remarks.
* none
*
*--*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateTopologyNode(HANDLE ParentHandle,
                     PKSNODE_CREATE NodeCreate,
                     IN ACCESS_MASK DesiredAccess,
                     OUT PHANDLE NodeHandle)
{
    return KsiCreateObjectType( ParentHandle,
                                KSSTRING_TopologyNode,
                                (PVOID) NodeCreate,
                                sizeof(KSNODE_CREATE),
                                DesiredAccess,
                                NodeHandle);
}


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
