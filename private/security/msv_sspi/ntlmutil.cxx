//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ntlmutil.cxx
//
// Contents:    Utility functions for NtLm package:
//                NtLmDuplicateUnicodeString
//                NtLmDuplicateSid
//                NtLmAllocate
//                NtLmFree
//
//
// History:     ChandanS  25-Jul-1996   Stolen from kerberos\client2\kerbutil.cxx
//
//------------------------------------------------------------------------
#include <global.h>


//+-------------------------------------------------------------------------
//
//  Function:   NtLmDuplicateUnicodeString
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too.
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
NtLmDuplicateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    SspPrint((SSP_MISC, "Entering NtLmDuplicateUnicodeString\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    DestinationString->Buffer = NULL;
    DestinationString->Length =
                        DestinationString->MaximumLength =
                        0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {

        DestinationString->Buffer = (LPWSTR) NtLmAllocate(
                       SourceString->Length + sizeof(WCHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = STATUS_NO_MEMORY;
            SspPrint((SSP_CRITICAL, "NtLmDuplicateUnicodeString, NtLmAllocate returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    SspPrint((SSP_MISC, "Leaving NtLmDuplicateUnicodeString\n"));
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmDuplicateString
//
//  Synopsis:   Duplicates a STRING. If the source string buffer is
//              NULL the destionation will be too.
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
NtLmDuplicateString(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    )
{
    SspPrint((SSP_MISC, "Entering NtLmDuplicateString\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    DestinationString->Buffer = NULL;
    DestinationString->Length =
                        DestinationString->MaximumLength =
                        0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {

        DestinationString->Buffer = (LPSTR) NtLmAllocate(
                       SourceString->Length + sizeof(CHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(CHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(CHAR)] = '\0';
        }
        else
        {
            Status = STATUS_NO_MEMORY;
            SspPrint((SSP_CRITICAL, "NtLmDuplicateString, NtLmAllocate returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    SspPrint((SSP_MISC, "Leaving NtLmDuplicateString\n"));
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmDuplicateSid
//
//  Synopsis:   Duplicates a SID
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationSid - Receives a copy of the SourceSid
//              SourceSid - SID to copy
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate memory
//                  failed
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
NtLmDuplicateSid(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    SspPrint((SSP_MISC, "Entering NtLmDuplicateSid\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SidSize;

    ASSERT(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);

    *DestinationSid = (PSID) NtLmAllocate( SidSize );

    if (ARGUMENT_PRESENT(*DestinationSid))
    {
        RtlCopyMemory(
            *DestinationSid,
            SourceSid,
            SidSize
            );
    }
    else
    {
        Status =  STATUS_INSUFFICIENT_RESOURCES;
        SspPrint((SSP_CRITICAL, "NtLmDuplicateSid, NtLmAllocate returns NULL\n"));
        goto CleanUp;
    }

CleanUp:
    SspPrint((SSP_MISC, "Leaving NtLmDuplicateSid\n"));
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmAllocate
//
//  Synopsis:   Allocate memory in either lsa mode or user mode
//
//  Effects:    Allocated chunk is zeroed out
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PVOID
NtLmAllocate(
    IN ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    SspPrint((SSP_MISC, "Entering NtLmAllocate\n"));

    if (NtLmState == NtLmLsaMode)
    {
        Buffer = LsaFunctions->AllocateLsaHeap(BufferSize);
        if (Buffer != NULL)
        {
            RtlZeroMemory(Buffer, BufferSize);
        }
    }
    else
    {
        ASSERT(NtLmState == NtLmUserMode);
        Buffer = LocalAlloc(LPTR, BufferSize);
    }


    SspPrint((SSP_MISC, "Leaving NtLmAllocate\n"));
    return Buffer;
}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmFree
//
//  Synopsis:   Free memory in either lsa mode or user mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
NtLmFree(
    IN PVOID Buffer
    )
{
    SspPrint((SSP_MISC, "Entering NtLmFree\n"));

    if (ARGUMENT_PRESENT(Buffer))
    {
        if (NtLmState == NtLmLsaMode)
        {
            SspPrint((SSP_MISC, "Leaving NtLmFree\n"));
            LsaFunctions->FreeLsaHeap(Buffer);
        }
        else
        {
            ASSERT(NtLmState == NtLmUserMode);
            SspPrint((SSP_MISC, "Leaving NtLmFree\n"));
            LocalFree(Buffer);
        }
    }


}
