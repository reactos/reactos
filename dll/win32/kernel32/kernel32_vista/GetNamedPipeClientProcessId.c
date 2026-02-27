
#include "k32_vista.h"

#define FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE CTL_CODE(FILE_DEVICE_NAMED_PIPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

/***********************************************************************
 *           GetNamedPipeClientProcessId  (KERNEL32.@)
 */
BOOL WINAPI GetNamedPipeClientProcessId( HANDLE pipe, ULONG *id )
{
    IO_STATUS_BLOCK iosb;

    return set_ntstatus( NtFsControlFile( pipe, NULL, NULL, NULL, &iosb,
                                          FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE, (void *)"ClientProcessId",
                                          sizeof("ClientProcessId"), id, sizeof(*id) ));
}
