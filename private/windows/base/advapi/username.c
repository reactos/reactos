/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    USERNAME.C

Abstract:

    This module contains the GetUserName API.

Author:

    Dave Snipp (DaveSn)    27-May-1992


Revision History:


--*/

#include <advapi.h>
#include <stdlib.h>
#include <ntlsa.h>


//
// UNICODE APIs
//

BOOL
WINAPI
GetUserNameInternal (
    LPWSTR pBuffer,
    LPDWORD pcbBuffer,
    BOOLEAN fUnicode
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    DWORD   ReturnedNameSize;
    BOOL    ReturnValue=FALSE;
    PUNICODE_STRING UserName = NULL;
    NTSTATUS Status;

    //
    // Get the user name from the LSA
    //

    Status = LsaGetUserName(
                &UserName,
                NULL
                );

    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError( Status );
        return( FALSE );
    }

    if (fUnicode) {
        ReturnedNameSize = (UserName->Length / sizeof(WCHAR)) + 1;
    } else {
        RtlUnicodeToMultiByteSize(&ReturnedNameSize,
                                  UserName->Buffer,
                                  UserName->Length);
        //
        // Add one for a null terminator
        //

        ReturnedNameSize++;
    }

    //
    // Check that the buffer is big enough. If it not, let the caller
    // know how big it should be.
    //

    if (ReturnedNameSize > *pcbBuffer) {
        *pcbBuffer = ReturnedNameSize;
        BaseSetLastNTError( STATUS_BUFFER_TOO_SMALL );
        ReturnValue = FALSE;

    } else {

        CopyMemory(
            pBuffer,
            UserName->Buffer,
            UserName->Length
            );

        //
        // Zero terminate the returned buffer.
        //

        pBuffer[UserName->Length / sizeof(WCHAR)] = L'\0';

        //
        // Return the length of the name in characters.
        //

        *pcbBuffer = ReturnedNameSize;
        ReturnValue = TRUE;

    }

    LsaFreeMemory(UserName->Buffer);
    LsaFreeMemory(UserName);

    return ReturnValue;
}

BOOL
WINAPI
GetUserNameW (
    LPWSTR pBuffer,
    LPDWORD pcbBuffer
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    return(GetUserNameInternal(
            pBuffer,
            pcbBuffer,
            TRUE                // Unicode
            ));
}



//
// ANSI APIs
//

BOOL
WINAPI
GetUserNameA (
    LPSTR pBuffer,
    LPDWORD pcbBuffer
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{

    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, *pcbBuffer * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *pcbBuffer;
    AnsiString.Length = 0;
    AnsiString.Buffer = pBuffer;

    //
    // Call the UNICODE version to do the work
    //

    if (!GetUserNameInternal(UnicodeBuffer, pcbBuffer, FALSE)) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);

    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *pcbBuffer = AnsiString.Length + 1;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);

}
