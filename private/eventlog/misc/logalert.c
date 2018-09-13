/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    LOGALERT.C

Abstract:

    This file contains the routine to log a Cairo alert in the system log.


Author:

    Ravi Rudrappa  (ravir)    18-Jan-1995


Revision History:

    18-Jan-1995         RaviR
        Created

--*/

#ifdef _CAIRO_

#include <elfclntp.h>


NTSTATUS
ElfLogCairoAlertInSystemLog(
    IN      HANDLE              hLogHandle,
    IN      LARGE_INTEGER       liEventTime,
    IN      USHORT              usEventType,
    IN      USHORT              usEventCategory,
    IN      ULONG               ulEventID,
    IN      USHORT              usNumStrings,
    IN      ULONG               ulDataSize,
    IN      WCHAR              *pwszComputerName,
    IN      WCHAR             **ppwszStrings,
    IN      PBYTE               pbData
    )
/*++

Routine Description:

  This routine logs the given Cairo Alert in the given log, through the
  ElfrReportEventW function.

Arguments:


Return Value:

    Returns an NTSTATUS code.

Note:


--*/
{
    NTSTATUS            s                       = STATUS_SUCCESS;
    ULONG               ulEventTime;
    PRPC_SID            psidUser                = NULL;
    USHORT              usFlags;
    PUNICODE_STRING     pusComputerName         = NULL;
    PUNICODE_STRING    *ppusStrings             = NULL;
    ULONG               i;
    ULONG               ulNumofAllocatedStrings = 0;
    LPBYTE              pbString                = NULL;

    //
    //  parameter validation
    //

    if ((hLogHandle == NULL) ||
        (pwszComputerName == NULL) ||
        ((usNumStrings > 0) && (ppwszStrings == NULL)) ||
        ((ulDataSize > 0) && (pbData == NULL)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  user Sid
    //

    psidUser = NULL;        // BUGBUG: is this ok?

    //
    //  Map creation time
    //

    RtlTimeToSecondsSince1970(&liEventTime, &ulEventTime);

    //
    // Convert the array of Alert description insert strings
    // to an array of UNICODE_STRINGs.
    //

    if (usNumStrings > 0)
    {
        LPBYTE pbPtr;
        //
        //  allocate memory in one shot.
        //

        pbPtr = pbString = MIDL_user_allocate(
                sizeof(PUNICODE_STRING) * usNumStrings +
                sizeof(UNICODE_STRING) * usNumStrings);

        if (pbString == NULL)
        {
            s = STATUS_NO_MEMORY;
            goto LCleanUp;
        }

        ppusStrings = (PUNICODE_STRING *)pbPtr;

        pbPtr += sizeof(PUNICODE_STRING) * usNumStrings;

        //
        // For each string passed in, allocate a UNICODE_STRING structure
        // and set it to the matching string.
        //
        for (i = 0; i < usNumStrings; i++)
        {
            ppusStrings[i] = (PUNICODE_STRING)pbPtr;

            pbPtr += sizeof(UNICODE_STRING);

            RtlInitUnicodeString(ppusStrings[i], ppwszStrings[i]);
        }
    }

    //
    //  Map the ComputerName to UNICODE_STRING.
    //

    pusComputerName = MIDL_user_allocate(sizeof(UNICODE_STRING));

    if (pusComputerName == NULL)
    {
        s = STATUS_NO_MEMORY;
        goto LCleanUp;
    }

    RtlInitUnicodeString(pusComputerName, pwszComputerName);


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        s = ElfrReportEventW (
                (IELF_HANDLE)hLogHandle,
                ulEventTime,
                usEventType,
                usEventCategory,
                ulEventID,
                usNumStrings,
                ulDataSize,
                (PRPC_UNICODE_STRING)pusComputerName,
                psidUser,
                (PRPC_UNICODE_STRING *)ppusStrings,
                pbData,
                0,              // Flags,
                NULL,           // RecordNumber,
                NULL);          // TimeWritten

    }
    RpcExcept (1) {

        s = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

LCleanUp:

    //
    // Free the space allocated for the inserts
    // and then free the space for ComputerName.
    //
    if (pbString)
    {
        MIDL_user_free(pbString);
    }

    if (pusComputerName)
    {
        MIDL_user_free(pusComputerName);
    }

    return (s);

}

#endif // _CAIRO_
