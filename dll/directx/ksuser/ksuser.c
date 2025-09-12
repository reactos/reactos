/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            dll/directx/ksuser/ksuser.c
 * PURPOSE:         KS USER functions
 * PROGRAMMER:      Magnus Olsen and Dmitry Chapyshev and Johannes Anderwald
 */

#include "ksuser.h"
#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
KsiCreateObjectType( HANDLE hHandle,
                     LPWSTR ObjectType,
                     PVOID Buffer,
                     ULONG BufferSize,
                     ACCESS_MASK DesiredAccess,
                     PHANDLE phHandle)
{
    NTSTATUS Status;
    SIZE_T Length;
    SIZE_T TotalSize;
    LPWSTR pStr;
    UNICODE_STRING ObjectName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    /* get length of object type */
    Length = wcslen(ObjectType);

    /* get length for request */
    TotalSize = (Length * sizeof(WCHAR)) + BufferSize;

    /* append space for '\\'*/
    TotalSize += sizeof(WCHAR);

    /* allocate buffer */
    pStr = HeapAlloc(GetProcessHeap(), 0, TotalSize);
    if (!pStr)
    {
        /* out of memory */
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* copy object type */
    wcscpy(pStr, ObjectType);

    /* append slash */
    pStr[Length] = L'\\';

    /* append parameters */
    memcpy(&pStr[Length+1], Buffer, BufferSize);

    /* initialize object name */
    ObjectName.Buffer = pStr;
    ObjectName.Length = ObjectName.MaximumLength = TotalSize;

    /* initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, &ObjectName, OBJ_CASE_INSENSITIVE, hHandle, NULL);

    /* create the object */
    Status = NtCreateFile(phHandle, DesiredAccess, &ObjectAttributes, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, 0, 1, 0, NULL, 0);

    /* free buffer */
    HeapFree(GetProcessHeap(), 0, pStr);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed zero handle */
        *phHandle = INVALID_HANDLE_VALUE;

        /* convert error code */
        Status = RtlNtStatusToDosError(Status);
    }

    /* done */
    return Status;
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
DWORD
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
* parameter to use to create the clock
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
DWORD
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
* Desired access
*
* @param PHANDLE ConnectionHandle
* connection handle passed
*
* @return
* Return NTSTATUS error code or sussess code.
*
* @remarks.
* The flag in PKSDATAFORMAT is not really document,
* to find it u need api monitor allot api and figout
* how it works, only flag I have found is the
* KSDATAFORMAT_ATTRIBUTES flag, it doing a Align
* of LONLONG size, it also round up it.
*
*--*/

KSDDKAPI
DWORD
NTAPI
KsCreatePin(HANDLE FilterHandle,
            PKSPIN_CONNECT Connect,
            ACCESS_MASK DesiredAccess,
            PHANDLE  ConnectionHandle)
{
    ULONG BufferSize = sizeof(KSPIN_CONNECT);
    PKSDATAFORMAT DataFormat = (PKSDATAFORMAT)(Connect + 1);

    BufferSize += DataFormat->FormatSize;

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
* Desired access
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
DWORD
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
