/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmwrapr2.c

Abstract:

    This module contains the source for wrapper routines called by the
    hive code, which in turn call the appropriate NT routines.  But not
    callable from user mode.

Author:

    Steven R. Wood (stevewo) 21-Apr-1992

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFileSetSize)
#endif

extern  REGISTRY_COMMAND    CommandArea;
extern  KEVENT StartRegistryCommand;
extern  KEVENT EndRegistryCommand;

//
// Write-Control:
//  CmpNoWrite is initially true.  When set this way write and flush
//  do nothing, simply returning success.  When cleared to FALSE, I/O
//  is enabled.  This change is made after the I/O system is started
//  AND autocheck (chkdsk) has done its thing.
//

extern  BOOLEAN CmpNoWrite;


BOOLEAN
CmpFileSetSize(
    PHHIVE      Hive,
    ULONG       FileType,
    ULONG       FileSize
    )
/*++

Routine Description:

    This routine sets the size of a file.  It must not return until
    the size is guaranteed, therefore, it does a flush.

    It is environment specific.

    This routine will force execution to the correct thread context.

Arguments:

    Hive - Hive we are doing I/O for

    FileType - which supporting file to use

    FileSize - 32 bit value to set the file's size to

Return Value:

    FALSE if failure
    TRUE if success

--*/
{
    NTSTATUS    status;
    REGISTRY_COMMAND Command;

    ASSERT(FIELD_OFFSET(CMHIVE, Hive) == 0);

    //
    // Wake up worker thread to do real work for us.
    //
    Command.Command  = REG_CMD_FILE_SET_SIZE;
    Command.Hive     = Hive;
    Command.FileType = FileType;
    Command.FileSize = FileSize;
    CmpLockRegistryExclusive();
    CmpWorker(&Command);
    status = Command.Status;
    CmpUnlockRegistry();

    if (!NT_SUCCESS(status)) {
        CMLOG(CML_MAJOR, CMS_IO_ERROR) {
            KdPrint(("CmpFileSetSize:\n\t"));
            KdPrint(("Failure: status = %08lx ", status));
        }
        return FALSE;
    }

    return TRUE;
}
