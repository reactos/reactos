/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    share.c

Abstract:

    Implements sharing for input and output handles

Author:

    Therese Stowell (thereses) 11-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

NTSTATUS
ConsoleAddShare(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PCONSOLE_SHARE_ACCESS ShareAccess,
    IN OUT PHANDLE_DATA HandleData
    )

{
    ULONG Ocount;
    ULONG ReadAccess;
    ULONG WriteAccess;
    ULONG SharedRead;
    ULONG SharedWrite;

    //
    // Set the access type in the file object for the current accessor.
    //

    ReadAccess = (DesiredAccess & GENERIC_READ) != 0;
    WriteAccess = (DesiredAccess & GENERIC_WRITE) != 0;

    SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
    SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;

    //
    // Now check to see whether or not the desired accesses are compatible
    // with the way that the file is currently open.
    //

    Ocount = ShareAccess->OpenCount;

    if ( (ReadAccess && (ShareAccess->SharedRead < Ocount))
         ||
         (WriteAccess && (ShareAccess->SharedWrite < Ocount))
         ||
         ((ShareAccess->Readers != 0) && !SharedRead)
         ||
         ((ShareAccess->Writers != 0) && !SharedWrite)
       ) {

        //
        // The check failed.  Simply return to the caller indicating that the
        // current open cannot access the file.
        //

        return STATUS_SHARING_VIOLATION;

    } else {

        //
        // The check was successful.  Update the counter information in the
        // shared access structure for this open request if the caller
        // specified that it should be updated.
        //

        ShareAccess->OpenCount++;

        ShareAccess->Readers += ReadAccess;
        ShareAccess->Writers += WriteAccess;

        ShareAccess->SharedRead += SharedRead;
        ShareAccess->SharedWrite += SharedWrite;
        HandleData->Access = DesiredAccess;
        HandleData->ShareAccess = DesiredShareAccess;

        return STATUS_SUCCESS;
    }
}

NTSTATUS
ConsoleDupShare(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PCONSOLE_SHARE_ACCESS ShareAccess,
    IN OUT PHANDLE_DATA TargetHandleData
    )

{
    ULONG ReadAccess;
    ULONG WriteAccess;
    ULONG SharedRead;
    ULONG SharedWrite;

    //
    // Set the access type in the file object for the current accessor.
    //

    ReadAccess = (DesiredAccess & GENERIC_READ) != 0;
    WriteAccess = (DesiredAccess & GENERIC_WRITE) != 0;

    SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
    SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;

    if (ShareAccess->OpenCount == 0) {
        ASSERT (FALSE);
        return STATUS_SHARING_VIOLATION;
    }

    ShareAccess->OpenCount++;

    ShareAccess->Readers += ReadAccess;
    ShareAccess->Writers += WriteAccess;

    ShareAccess->SharedRead += SharedRead;
    ShareAccess->SharedWrite += SharedWrite;

    TargetHandleData->Access = DesiredAccess;
    TargetHandleData->ShareAccess = DesiredShareAccess;

    return STATUS_SUCCESS;
}

NTSTATUS
ConsoleRemoveShare(
    IN ULONG DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PCONSOLE_SHARE_ACCESS ShareAccess
    )

{
    ULONG ReadAccess;
    ULONG WriteAccess;
    ULONG SharedRead;
    ULONG SharedWrite;

    //
    // Set the access type in the file object for the current accessor.
    //

    ReadAccess = (DesiredAccess & GENERIC_READ) != 0;
    WriteAccess = (DesiredAccess & GENERIC_WRITE) != 0;

    SharedRead = (DesiredShareAccess & FILE_SHARE_READ) != 0;
    SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE) != 0;

    if (ShareAccess->OpenCount == 0) {
        ASSERT (FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    ShareAccess->OpenCount--;

    ShareAccess->Readers -= ReadAccess;
    ShareAccess->Writers -= WriteAccess;

    ShareAccess->SharedRead -= SharedRead;
    ShareAccess->SharedWrite -= SharedWrite;

    return STATUS_SUCCESS;
}
