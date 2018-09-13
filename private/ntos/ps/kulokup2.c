
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kulookup.c

Abstract:

    xxxx processor version of PspLookupKernelUserEntryPoints

Author:


Revision History:

--*/

#include    "psp.h"


NTSTATUS
PspLookupKernelUserEntryPoints( VOID)

/*++

Routine Description:

    The function locates user mode code that the kernel dispatches
    to, and stores the addresses of that code in global kernel variables.

    Which procedures are of interest is machine dependent.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    //
    // NOTE WELL - This is a dummy stub to make the system build.
    //		   Replace it with proper code
    //

    return STATUS_SUCCESS;
}
