/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/filter.c
 * PURPOSE:         Provides support for usage of SEH inside File System Drivers
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#undef FsRtlAllocatePoolWithQuotaTag
#undef FsRtlAllocatePoolWithTag

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlIsTotalDeviceFailure
 * @implemented NT 4.0
 *
 *     The FsRtlIsTotalDeviceFailure routine checks if an NTSTATUS error code
 *     represents a disk hardware failure.
 *
 * @param NtStatus
 *        The NTSTATUS Code to Test
 *
 * @return TRUE in case of Hardware Failure, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(IN NTSTATUS NtStatus)
{
    return((NT_SUCCESS(NtStatus)) ||
           (STATUS_CRC_ERROR == NtStatus) ||
           (STATUS_DEVICE_DATA_ERROR == NtStatus) ? FALSE : TRUE);
}

/*++
 * @name FsRtlIsNtstatusExpected
 * @implemented NT 4.0
 *
 *     The FsRtlIsNtstatusExpected routine checks if an NTSTATUS error code
 *     is expected by the File System Support Library.
 *
 * @param NtStatus
 *        The NTSTATUS Code to Test
 *
 * @return TRUE if the Value is Expected, FALSE otherwise.
 *
 * @remarks None.
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsNtstatusExpected(IN NTSTATUS NtStatus)
{
    return((STATUS_DATATYPE_MISALIGNMENT == NtStatus) ||
           (STATUS_ACCESS_VIOLATION == NtStatus) ||
           (STATUS_ILLEGAL_INSTRUCTION == NtStatus) ||
           (STATUS_INSTRUCTION_MISALIGNMENT == NtStatus)) ? FALSE : TRUE;
}

/*++
 * @name FsRtlNormalizeNtstatus
 * @implemented NT 4.0
 *
 *     The FsRtlNormalizeNtstatus routine normalizes an NTSTATUS error code.
 *
 * @param NtStatusToNormalize
 *        The NTSTATUS error code to Normalize.
 *
 * @param NormalizedNtStatus
 *        The NTSTATUS error code to return if the NtStatusToNormalize is not
 *        a proper expected error code by the File System Library.
 *
 * @return NtStatusToNormalize if it is an expected value, otherwise
 *         NormalizedNtStatus.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
FsRtlNormalizeNtstatus(IN NTSTATUS NtStatusToNormalize,
                       IN NTSTATUS NormalizedNtStatus)
{
    return(TRUE == FsRtlIsNtstatusExpected(NtStatusToNormalize)) ?
           NtStatusToNormalize : NormalizedNtStatus;
}

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
 * @remarks The pool tag used is "FSrt".
 *
 *--*/
PVOID
NTAPI
FsRtlAllocatePool(IN POOL_TYPE PoolType,
                  IN ULONG NumberOfBytes)
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
 * @remarks The pool tag used is "FSrt".
 *
 *--*/
PVOID
NTAPI
FsRtlAllocatePoolWithQuota(IN POOL_TYPE PoolType,
                           IN ULONG NumberOfBytes)
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
PVOID
NTAPI
FsRtlAllocatePoolWithQuotaTag (IN POOL_TYPE PoolType,
                               IN ULONG NumberOfBytes,
                               IN ULONG Tag)
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
PVOID
NTAPI
FsRtlAllocatePoolWithTag(IN POOL_TYPE PoolType,
                         IN ULONG NumberOfBytes,
                         IN ULONG Tag)
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

