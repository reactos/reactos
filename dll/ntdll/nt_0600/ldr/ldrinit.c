#include "ntdll_vista.h"

PVOID LdrpHeap;
#include <debug.h>


/* These APIs are very commonly used in modern apps + needed for kernelbase, But these stubs will work for now. */
NTSTATUS WINAPI LdrGetDllDirectory( UNICODE_STRING *dir )
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS WINAPI LdrSetDllDirectory( const UNICODE_STRING *dir )
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LdrAddDllDirectory( const UNICODE_STRING *dir, void **cookie )
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI LdrRemoveDllDirectory( void *cookie )
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LdrSetDefaultDllDirectories( ULONG flags )
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}