

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
* The function KsCreateAllocator creates a handle to an allocator for the given sink connection handle
*
* @param HANDLE ConnectionHandle
* Handle to the sink connection on which to create the allocator
*
* @param PKSALLOCATOR_FRAMING AllocatorFraming
* the input param we using to alloc our framing
*
* @param PHANDLE AllocatorHandle
* Our new handle that we have alloc
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
* The function KsCreateClock  creates handle to clock instance
*
* @param HANDLE ConnectionHandle
* Handle to use to create the clock 
*
* @param PKSCLOCK_CREATE ClockCreate
* paramenter to use to create the clock
*
* @param PHANDLE  ClockHandle
* The new handle
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
* The function KsCreatePin passes a connection request to device and create pin instance
*
* @param HANDLE FilterHandle
* handle of the filter initiating the create request
*
* @param PKSPIN_CONNECT Connect
* Pointer to a KSPIN_CONNECT structure that contains parameters for the requested connection. 
* This should be followed in memory by a KSDATAFORMAT data structure, describing the data format
* requested for the connection. 

* @param ACCESS_MASK  DesiredAccess
* Desrided access
*
* @param PHANDLE ConnectionHandle
* connection handle passed
*
* @return 
* Return NTSTATUS error code or sussess code.
*
* @remarks.
* The flag in PKSDATAFORMAT is not really document, 
* to find it u need api mointor allot api and figout
* how it works, only flag I have found is the 
* KSDATAFORMAT_ATTRIBUTES flag, it doing a Align
* of LONLONG size, it also round up it.
*
*--*/

KSDDKAPI
NTSTATUS
NTAPI
KsCreatePin(HANDLE FilterHandle,
            PKSPIN_CONNECT Connect,
            ACCESS_MASK DesiredAccess,
            PHANDLE  ConnectionHandle)
{
    ULONG BufferSize = sizeof(KSPIN_CONNECT);
    PKSDATAFORMAT DataFormat = ((PKSDATAFORMAT) ( ((ULONG)Connect) + ((ULONG)sizeof(KSPIN_CONNECT)) ) );

    if (DataFormat->Flags &  KSDATAFORMAT_ATTRIBUTES)
    {
        BufferSize += (ROUND_UP(DataFormat->FormatSize,sizeof(LONGLONG)) + DataFormat->FormatSize);
    }

    return KsiCreateObjectType(FilterHandle,
                               KSSTRING_Pin,
                               Connect,
                               BufferSize,
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
