/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    sid.c

Abstract:

    SID management functions

Author:

    (davidc) 26-Aug-1992

--*/
// NT base apis
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

#include "sysdm.h"


LPTSTR 
GetSidString(
    void
)
/*++

Routine Description:

    Allocates and returns a string representing the sid of the current user
    The returned pointer should be freed using DeleteSidString().

Arguments:

    None

Return Value:

    Returns a pointer to the string or NULL on failure.

--*/
{
    NTSTATUS NtStatus;
    PSID UserSid;
    UNICODE_STRING UnicodeString;
    LPTSTR lpEnd;
#ifndef UNICODE
    STRING String;
#endif

    //
    // Get the user sid
    //

    UserSid = GetUserSid();
    if (UserSid == NULL) {
        return NULL;
    }

    //
    // Convert user SID to a string.
    //

    NtStatus = RtlConvertSidToUnicodeString(
                            &UnicodeString,
                            UserSid,
                            (BOOLEAN)TRUE // Allocate
                            );
    //
    // We're finished with the user sid
    //

    DeleteUserSid(UserSid);

    //
    // See if the conversion to a string worked
    //

    if (!NT_SUCCESS(NtStatus)) {
        return NULL;
    }

#ifdef UNICODE


    return(UnicodeString.Buffer);

#else

    //
    // Convert the string to ansi
    //

    NtStatus = RtlUnicodeStringToAnsiString(&String, &UnicodeString, TRUE);
    RtlFreeUnicodeString(&UnicodeString);
    if (!NT_SUCCESS(NtStatus)) {
        return NULL;
    }


    return(String.Buffer);

#endif

}


VOID 
DeleteSidString(
    IN LPTSTR SidString
)
/*++

Routine Description:

    Frees up a sid string previously returned by GetSidString()

Arguments:

    SidString -
        Supplies string to free

Return Value:

    None

--*/
{

#ifdef UNICODE
    UNICODE_STRING String;

    RtlInitUnicodeString(&String, SidString);

    RtlFreeUnicodeString(&String);
#else
    ANSI_STRING String;

    RtlInitAnsiString(&String, SidString);

    RtlFreeAnsiString(&String);
#endif

}


PSID 
GetUserSid(
    void
)
/*++

Routine Description:

    Allocs space for the user sid, fills it in and returns a pointer. Caller
    The sid should be freed by calling DeleteUserSid.

    Note the sid returned is the user's real sid, not the per-logon sid.

Arguments:

    None

Return Value:

    Returns pointer to sid or NULL on failure.

--*/
{
    PTOKEN_USER pUser;
    PSID pSid;
    DWORD BytesRequired = 200;
    NTSTATUS status;
    HANDLE UserToken;


    if (!OpenProcessToken (GetCurrentProcess(), TOKEN_READ, &UserToken)) {
        return NULL;
    }

    //
    // Allocate space for the user info
    //

    pUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, BytesRequired);


    if (pUser == NULL) {
        CloseHandle (UserToken);
        return NULL;
    }


    //
    // Read in the UserInfo
    //

    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Allocate a bigger buffer and try again.
        //

        pUser = LocalReAlloc(pUser, BytesRequired, LMEM_MOVEABLE);
        if (pUser == NULL) {
            CloseHandle (UserToken);
            return NULL;
        }

        status = NtQueryInformationToken(
                     UserToken,             // Handle
                     TokenUser,             // TokenInformationClass
                     pUser,                 // TokenInformation
                     BytesRequired,         // TokenInformationLength
                     &BytesRequired         // ReturnLength
                     );

    }

    if (!NT_SUCCESS(status)) {
        LocalFree(pUser);
        CloseHandle (UserToken);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
    if (pSid == NULL) {
        LocalFree(pUser);
        CloseHandle (UserToken);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if (!NT_SUCCESS(status)) {
        LocalFree(pSid);
        pSid = NULL;
    }

    CloseHandle (UserToken);

    return pSid;
}


VOID 
DeleteUserSid(
    IN PSID Sid
)
/*++

Routine Description:

    Deletes a user sid previously returned by GetUserSid()

Arguments:

    Sid -
        Supplies sid to delete

Return Value:

    None

--*/
{
    LocalFree(Sid);
}
