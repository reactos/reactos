/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/logsup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
CcSetAdditionalCacheAttributes(IN PFILE_OBJECT FileObject,
                               IN BOOLEAN DisableReadAhead,
                               IN BOOLEAN DisableWriteBehind)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
CcSetLogHandleForFile(IN PFILE_OBJECT FileObject,
                      IN PVOID LogHandle,
                      IN PFLUSH_TO_LSN FlushToLsnRoutine)
{
    UNIMPLEMENTED;
    while (TRUE);
}

LARGE_INTEGER
NTAPI
CcGetDirtyPages(IN PVOID LogHandle,
                IN PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
                IN PVOID Context1,
                IN PVOID Context2)
{
    LARGE_INTEGER Result = {{0}};
    UNIMPLEMENTED;
    while (TRUE);
    return Result;
}

BOOLEAN
NTAPI
CcIsThereDirtyData(IN PVPB Vpb)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

LARGE_INTEGER
NTAPI
CcGetLsnForFileObject(IN PFILE_OBJECT FileObject,
                      OUT OPTIONAL PLARGE_INTEGER OldestLsn)
{
    LARGE_INTEGER Result = {{0}};
    UNIMPLEMENTED;
    while (TRUE);
    return Result;
}

/* EOF */
