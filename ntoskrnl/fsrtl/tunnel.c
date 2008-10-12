/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/tunnel.c
 * PURPOSE:         Provides the Tunnel Cache implementation for file system drivers.
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlAddToTunnelCache
 * @unimplemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @param DirectoryKey
 *        FILLME
 *
 * @param ShortName
 *        FILLME
 *
 * @param LongName
 *        FILLME
 *
 * @param KeyByShortName
 *        FILLME
 *
 * @param DataLength
 *        FILLME
 *
 * @param Data
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlAddToTunnelCache(IN PTUNNEL Cache,
                      IN ULONGLONG DirectoryKey,
                      IN PUNICODE_STRING ShortName,
                      IN PUNICODE_STRING LongName,
                      IN BOOLEAN KeyByShortName,
                      IN ULONG DataLength,
                      IN PVOID Data)
{
    /* Unimplemented */
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlDeleteKeyFromTunnelCache
 * @unimplemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @param DirectoryKey
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlDeleteKeyFromTunnelCache(IN PTUNNEL Cache,
                              IN ULONGLONG DirectoryKey)
{
    /* Unimplemented */
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlDeleteTunnelCache
 * @unimplemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlDeleteTunnelCache(IN PTUNNEL Cache)
{
    /* Unimplemented */
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlFindInTunnelCache
 * @unimplemented
 *
 * FILLME
 *
 * @param Cache
 *        FILLME
 *
 * @param DirectoryKey
 *        FILLME
 *
 * @param ShortName
 *        FILLME
 *
 * @param LongName
 *        FILLME
 *
 * @param KeyByShortName
 *        FILLME
 *
 * @param DataLength
 *        FILLME
 *
 * @param Data
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlFindInTunnelCache(IN PTUNNEL Cache,
                       IN ULONGLONG DirectoryKey,
                       IN PUNICODE_STRING Name,
                       OUT PUNICODE_STRING ShortName,
                       OUT PUNICODE_STRING LongName,
                       IN OUT PULONG DataLength,
                       OUT PVOID Data)
{
    /* Unimplemented */
    KEBUGCHECK(0);
    return FALSE;
}

/*++
 * @name FsRtlDeleteTunnelCache
 * @implemented
 *
 * Initialize a tunnel cache
 *
 * @param Cache
 *        Pointer to an allocated TUNNEL structure
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlInitializeTunnelCache(IN PTUNNEL Cache)
{
    ExInitializeFastMutex(&(Cache->Mutex));
    Cache->Cache = 0;
    InitializeListHead(&(Cache->TimerQueue));
    Cache->NumEntries = 0;
}

/* EOF */
