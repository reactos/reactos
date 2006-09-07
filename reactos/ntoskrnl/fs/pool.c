/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fs/pool.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*++
 * @name FsRtlAllocatePool
 * @implemented
 *
 * FILLME
 *
 * @param PoolType
 *        FILLME
 *
 * @param NumberOfBytes
 *        FILLME
 *
 * @return None
 *
 * @remarks IFS_POOL_TAG is "FSrt" in mem view.
 *
 *--*/
PVOID
NTAPI
FsRtlAllocatePool(IN POOL_TYPE PoolType,
                  IN ULONG     NumberOfBytes)
{
    PVOID   Address;

    Address = ExAllocatePoolWithTag(PoolType,
                                    NumberOfBytes,
                                    IFS_POOL_TAG);

    if (NULL == Address)
    {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }

    return Address;
}

/*++
 * @name FsRtlAllocatePoolWithQuota
 * @implemented
 *
 * FILLME
 *
 * @param PoolType
 *        FILLME
 *
 * @param NumberOfBytes
 *        FILLME
 *
 * @return None
 *
 * @remarks IFS_POOL_TAG is "FSrt" in mem view.
 *
 *--*/
PVOID
NTAPI
FsRtlAllocatePoolWithQuota(IN POOL_TYPE PoolType,
                           IN ULONG     NumberOfBytes)
{
    PVOID	Address;

    Address = ExAllocatePoolWithQuotaTag(PoolType,
                                         NumberOfBytes,
                                         IFS_POOL_TAG);
    if (NULL == Address)
    {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
    return Address;
}

/*++
 * @name FsRtlAllocatePoolWithQuotaTag
 * @implemented
 *
 * FILLME
 *
 * @param PoolType
 *        FILLME
 *
 * @param NumberOfBytes
 *        FILLME
 *
 * @param Tag
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
#undef FsRtlAllocatePoolWithQuotaTag
PVOID
NTAPI
FsRtlAllocatePoolWithQuotaTag (IN POOL_TYPE PoolType,
                               IN ULONG     NumberOfBytes,
                               IN ULONG     Tag)
{
    PVOID   Address;

    Address = ExAllocatePoolWithQuotaTag(PoolType,
                                         NumberOfBytes,
                                         Tag);

    if (NULL == Address)
    {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }

    return Address;
}

/*++
 * @name FsRtlAllocatePoolWithTag
 * @implemented
 *
 * FILLME
 *
 * @param PoolType
 *        FILLME
 *
 * @param NumberOfBytes
 *        FILLME
 *
 * @param Tag
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
#undef FsRtlAllocatePoolWithTag
PVOID
NTAPI
FsRtlAllocatePoolWithTag(IN POOL_TYPE   PoolType,
                         IN ULONG       NumberOfBytes,
                         IN ULONG       Tag)
{
    PVOID   Address;

    Address = ExAllocatePoolWithTag(PoolType,
                                    NumberOfBytes,
                                    Tag);

    if (NULL == Address)
    {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }

    return Address;
}


/* EOF */
