/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Filter.c

Abstract:

    The Exception filter is used by the file system and cache manager
    to handle error recovery.  The basic idea is to have the top level
    file system entry points (i.e., the FSD entry points and FSP dispatch
    loop) have a try-except around their code, and then whenever the
    file system or cache manager reach an error case they raise an
    appropriate status.  Then the exception handler catches the exception
    and can either complete the request, send it off to the fsp, verify the
    volume, or bugcheck.  We only bugcheck if the raised exception is
    unexpected (i.e., unhandled).

    This module provides two routines for filtering out exceptions.  The
    first routine is used to normalize status values to be one of the
    value handled by the filter.  That way if we get an exception not handled
    by the filter then we know that the system is in real trouble and we
    just bugcheck the machine.  The second routine is used to ask if
    a status value is within the set of values handled by the filter.

    The value of status handled by this filter are listed in the routine
    FsRtlIsNtstatusExpected.

Author:

    Gary Kimura     [GaryKi]    4-Jan-1991

Revision History:

--*/

#include "FsRtlP.h"

//
//  Trace level for the module
//

#define Dbg                              (0x80000000)


NTSTATUS
FsRtlNormalizeNtstatus (
    IN NTSTATUS Exception,
    IN NTSTATUS GenericException
    )

/*++

Routine Description:

    This routine is used to normalize an NTSTATUS into a status
    that is handled by the file system's top level exception handlers.

Arguments:

    Exception - Supplies the exception being normalized

    GenericException - Supplies a second exception to translate to
        if the first exception is not within the set of exceptions
        handled by the filter

Return Value:

    NTSTATUS - Returns Exception if the value is already handled
        by the filter, and GenericException otherwise.

--*/

{
    return (FsRtlIsNtstatusExpected(Exception) ? Exception : GenericException);
}


BOOLEAN
FsRtlIsNtstatusExpected (
    IN NTSTATUS Exception
    )

/*++

Routine Description:

    This routine is used to decide if a status is within the set of values
    handled by the exception filter.

Arguments:

    Exception - Supplies the exception being queried

Return Value:

    BOOLEAN - Returns TRUE if the value is handled by the filter, and
        FALSE otherwise.

--*/

{
    switch (Exception) {

    case STATUS_DATATYPE_MISALIGNMENT:
    case STATUS_ACCESS_VIOLATION:
    case STATUS_ILLEGAL_INSTRUCTION:
    case STATUS_INSTRUCTION_MISALIGNMENT:

        return FALSE;

    default:

        return TRUE;
    }
}


#undef FsRtlAllocatePool

PVOID
FsRtlAllocatePool (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This routine is used to allocate executive level pool.  It either
    returns a non null pointer to the newly allocated pool or it raises
    a status of insufficient resources.

Arguments:

    PoolType - Supplies the type of executive pool to allocate

    NumberOfBytes - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a non null pointer to the newly allocated pool.

--*/

{
    PVOID p;

    //
    //  Allocate executive pool and if we get back null then raise
    //  a status of insufficient resources
    //

    if ((p = ExAllocatePoolWithTag( PoolType, NumberOfBytes, 'trSF')) == NULL) {

        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    return p;
}

#undef FsRtlAllocatePoolWithQuota


PVOID
FsRtlAllocatePoolWithQuota (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This routine is used to allocate executive level pool with quota.  It
    either returns a non null pointer to the newly allocated pool or it raises
    a status of insufficient resources.

Arguments:

    PoolType - Supplies the type of executive pool to allocate

    NumberOfBytes - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a non null pointer to the newly allocated pool.

--*/

{
    PVOID p;

    //
    //  Allocate executive pool and if we get back null then raise
    //  a status of insufficient resources
    //

    if ((p = ExAllocatePoolWithQuotaTag ( PoolType, NumberOfBytes, 'trSF')) == NULL) {

        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    return p;
}


#undef FsRtlAllocatePoolWithTag

PVOID
FsRtlAllocatePoolWithTag (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine is used to allocate executive level pool with a tag.

Arguments:

    PoolType - Supplies the type of executive pool to allocate

    NumberOfBytes - Supplies the number of bytes to allocate

    Tag - Supplies the tag for the pool block

Return Value:

    PVOID - Returns a non null pointer to the newly allocated pool.

--*/

{
    PVOID p;

    //
    //  Allocate executive pool and if we get back null then raise
    //  a status of insufficient resources
    //

    if ((p = ExAllocatePoolWithTag( PoolType, NumberOfBytes, Tag)) == NULL) {

        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    return p;
}


#undef FsRtlAllocatePoolWithQuotaTag

PVOID
FsRtlAllocatePoolWithQuotaTag (
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine is used to allocate executive level pool with a quota tag.

Arguments:

    PoolType - Supplies the type of executive pool to allocate

    NumberOfBytes - Supplies the number of bytes to allocate

    Tag - Supplies the tag for the pool block

Return Value:

    PVOID - Returns a non null pointer to the newly allocated pool.

--*/

{
    PVOID p;

    //
    //  Allocate executive pool and if we get back null then raise
    //  a status of insufficient resources
    //

    if ((p = ExAllocatePoolWithQuotaTag( PoolType, NumberOfBytes, Tag)) == NULL) {

        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    return p;
}


BOOLEAN
FsRtlIsTotalDeviceFailure(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is given an NTSTATUS value and make a determination as to
    if this value indicates that the complete device has failed and therefore
    should no longer be used, or if the failure is one that indicates that
    continued use of the device is ok (i.e. a sector failure).

Arguments:

    Status - the NTSTATUS value to test.

Return Value:

    TRUE  - The status value given is believed to be a fatal device error.
    FALSE - The status value given is believed to be a sector failure, but not
            a complete device failure.
--*/

{
    if (NT_SUCCESS(Status)) {

        //
        // All warning and informational errors will be resolved here.
        //

        return FALSE;
    }

    switch (Status) {
    case STATUS_CRC_ERROR:
    case STATUS_DEVICE_DATA_ERROR:
        return FALSE;
    default:
        return TRUE;
    }
}
