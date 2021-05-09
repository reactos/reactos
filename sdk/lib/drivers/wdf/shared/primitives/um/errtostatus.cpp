//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "Mx.h"

BOOL
ConvertWinErrorToNtstatus(
    __in    ULONG       WinError,
    __out   NTSTATUS    * Status
    )
{
    ULONG index = 0;
    BOOL found = FALSE;

    *Status = INVALID_STATUS;

    for ( ULONG i = 0; i < ARRAYSIZE(ErrorBucketTable); i++ )
    {
        if (WinError < ErrorBucketTable[i].BaseErrorCode)
        {
            //
            // The error code falls between previous bucket end and current
            // bucket begin, hence there is no mapping for it
            //
            break;
        }

        if ( WinError < (ErrorBucketTable[i].BaseErrorCode +
                                                ErrorBucketTable[i].RunLength) )
        {
            //
            // Index falls within the current bucket
            //
            index += (WinError - ErrorBucketTable[i].BaseErrorCode);
            found = TRUE;
            break;
        }
        else
        {
            //
            // Index is beyond current bucket, continue search
            //
            index += ErrorBucketTable[i].RunLength;
        }
    }

    if (TRUE == found)
    {
        *Status = ErrorTable[index];
        if (INVALID_STATUS == (*Status))
        {
            found = FALSE;
        }
    }

    return found;
}

NTSTATUS
WinErrorToNtStatus(
    __in ULONG WinError
    )
{
    NTSTATUS status;

    if (TRUE == ConvertWinErrorToNtstatus(WinError, &status))
    {
        return status;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}
