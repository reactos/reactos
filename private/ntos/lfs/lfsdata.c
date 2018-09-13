/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LfsData.c

Abstract:

    This module declares the global data used by the Logging File Service.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

//
//  The global Lfs data record
//

LFS_DATA LfsData;

//
//  Various large integer constants.
//

LARGE_INTEGER LfsLi0 = {0x00000000, 0x00000000};
LARGE_INTEGER LfsLi1 = {0x00000001, 0x00000000};

//
//  The following Lsn will never occur in a file, it is used to indicate
//  a non-lsn.
//

LSN LfsZeroLsn = {0x00000000, 0x00000000};

#ifdef LFSDBG

LONG LfsDebugTraceLevel = 0x0000000F;
LONG LfsDebugTraceIndent = 0;

#endif // LFSDBG


LONG
LfsExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It indicates that we should handle
    the exception or bug check the system.

Arguments:

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

#ifdef NTFS_RESTART
    ASSERT( (ExceptionCode != STATUS_DISK_CORRUPT_ERROR) &&
            (ExceptionCode != STATUS_FILE_CORRUPT_ERROR) );
#endif

    //if (ExceptionCode != STATUS_LOG_FILE_FULL) {
    //
    //    DbgPrint("Status not LOGFILE FULL, ExceptionPointers = %08lx\n", ExceptionPointer);
    //    DbgBreakPoint();
    //}

    if (!FsRtlIsNtstatusExpected( ExceptionCode )) {

        return EXCEPTION_CONTINUE_SEARCH;

    } else {

        return EXCEPTION_EXECUTE_HANDLER;
    }
}
