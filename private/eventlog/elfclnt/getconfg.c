/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    getconfg.c

Abstract:

    This routine calls GetComputerName[A,W] to obtain the computer name
    in both Ansi and Unicode

Author:

    Dan Lafferty (danl)     09-Apr-1991

Environment:

    User Mode -Win32 (also uses nt RTL routines)

Revision History:

    09-Apr-1991     danl
        created

--*/

#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>
#include <ntdef.h>
#include <windef.h>
#include <winbase.h>    // LocalAlloc



DWORD
ElfpGetComputerName (
    OUT  LPSTR   *ComputerNamePtrA,
    OUT  LPWSTR  *ComputerNamePtrW
    )
/*++

Routine Description:

    This routine obtains the computer name from a persistent database,
    by calling the GetcomputerName[A,W] Win32 Base APIs

    This routine assumes the length of the computername is no greater
    than MAX_COMPUTERNAME_LENGTH, space for which it allocates using
    LocalAlloc.  It is necessary for the user to free that space using
    LocalFree when finished.

Arguments:

    ComputerNamePtrA - Pointer to the location of the Ansi computer name
    ComputerNamePtrW - Pointer to the location of the Unicode computer name

Return Value:

    NO_ERROR - If the operation was successful.

    Any other Win32 error if unsuccessful

--*/
{
    DWORD dwError = NO_ERROR;
    DWORD nSize   = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // Allocate a buffer to hold the largest possible computer name.
    //

    *ComputerNamePtrA = LocalAlloc(LMEM_ZEROINIT, nSize);
    *ComputerNamePtrW = LocalAlloc(LMEM_ZEROINIT, nSize * sizeof(WCHAR));

    if (*ComputerNamePtrA == NULL || *ComputerNamePtrW == NULL) {
        goto CleanExit;
    }

    //
    // Get the computer name string into the locally allocated buffers
    // by calling the Win32 GetComputerName[A,W] APIs.
    //

    if (!GetComputerNameA(*ComputerNamePtrA, &nSize)) {
        goto CleanExit;
    }

    //
    // GetComputerName always updates nSize
    //

    nSize = MAX_COMPUTERNAME_LENGTH + 1;

    if (!GetComputerNameW(*ComputerNamePtrW, &nSize)) {
        goto CleanExit;
    }

    return (NO_ERROR);

CleanExit:

    dwError = GetLastError();
    LocalFree(*ComputerNamePtrA);
    LocalFree(*ComputerNamePtrW);
    *ComputerNamePtrA = NULL;
    *ComputerNamePtrW = NULL;
    return (dwError);
}
