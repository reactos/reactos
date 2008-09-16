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
    KeBugCheck(FILE_SYSTEM);
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
    KeBugCheck(FILE_SYSTEM);
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
    KeBugCheck(FILE_SYSTEM);
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
    KeBugCheck(FILE_SYSTEM);
    return FALSE;
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
FsRtlInitializeTunnelCache(IN PTUNNEL Cache)
{
    /* Unimplemented */
    KeBugCheck(FILE_SYSTEM);
}

/* EOF */
