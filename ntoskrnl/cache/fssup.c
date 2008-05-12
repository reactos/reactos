/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/fssup.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PFSN_PREFETCHER_GLOBALS CcPfGlobals;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CcInitializeCacheManager(VOID)
{
    /* Nothing to do */
    return TRUE;
}

VOID
NTAPI
CcPfInitializePrefetcher(VOID)
{
    /* Notify debugger */
    DbgPrintEx(DPFLTR_PREFETCHER_ID,
               DPFLTR_TRACE_LEVEL,
               "CCPF: InitializePrefetecher()\n");

    /* Setup the Prefetcher Data */
    InitializeListHead(&CcPfGlobals.ActiveTraces);
    InitializeListHead(&CcPfGlobals.CompletedTraces);
    ExInitializeFastMutex(&CcPfGlobals.CompletedTracesLock);

    /* FIXME: Setup the rest of the prefetecher */
}

VOID
NTAPI
CcInitializeCacheMap(IN PFILE_OBJECT FileObject,
                     IN PCC_FILE_SIZES FileSizes,
                     IN BOOLEAN PinAccess,
                     IN PCACHE_MANAGER_CALLBACKS Callbacks,
                     IN PVOID LazyWriteContext)
{
    /* Setup a fake cache map pointer */
    FileObject->SectionObjectPointer->SharedCacheMap = (PVOID)0xBAADF00D;
    return;
}

BOOLEAN
NTAPI
CcUninitializeCacheMap(IN PFILE_OBJECT FileObject,
                       IN OPTIONAL PLARGE_INTEGER TruncateSize,
                       IN OPTIONAL PCACHE_UNINITIALIZE_EVENT UninitializeEvent)
{
    ASSERT(UninitializeEvent == NULL);

    /* Clear the cache map */
    FileObject->SectionObjectPointer->SharedCacheMap = NULL;
    return TRUE;
}

VOID
NTAPI
CcSetFileSizes(IN PFILE_OBJECT FileObject,
               IN PCC_FILE_SIZES FileSizes)
{
    /* Nothing to do */
}

BOOLEAN
NTAPI
CcPurgeCacheSection(IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                    IN OPTIONAL PLARGE_INTEGER FileOffset,
                    IN ULONG Length,
                    IN BOOLEAN UninitializeCacheMaps)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
CcSetDirtyPageThreshold(IN PFILE_OBJECT FileObject,
                        IN ULONG DirtyPageThreshold)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
CcZeroData(IN PFILE_OBJECT FileObject,
           IN PLARGE_INTEGER StartOffset,
           IN PLARGE_INTEGER EndOffset,
           IN BOOLEAN Wait)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs(IN PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NULL;
}

PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb(IN PVOID Bcb)
{
    UNIMPLEMENTED;
    while (TRUE);
    return NULL;
}

/* EOF */
