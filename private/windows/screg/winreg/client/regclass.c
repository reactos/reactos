/*++




Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regclass.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to open the classes root key for a specified user.

        - RegOpenUserClassesRoot

Author:

    Adam Edwards (adamed) 15-Apr-1998

Notes:

    This API is local only.
    See the notes in server\regkey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"
#include <malloc.h>

#define REG_USER_CLASSES_PREFIX L"\\Registry\\User\\"
#define REG_USER_CLASSES_SUFFIX L"_Classes"

BOOL InitializeClassesEnumTable();
BOOL InitializeClassesNameSpace();

BOOL CleanupClassesEnumTable(DWORD dwCriteria);
BOOL CleanupClassesNameSpace();

#if defined(LEAK_TRACK)
NTSTATUS TrackObject(HKEY hKey);
#endif // defined(LEAK_TRACK)

extern BOOL gbCombinedClasses;


LONG
APIENTRY
RegOpenUserClassesRoot(
    HANDLE hToken,
    DWORD  dwOptions,
    REGSAM samDesired,
    PHKEY  phkResult
    )

/*++

Routine Description:

    Win32 Unicode RPC wrapper for opening the classes root key
    for the use specified by the hToken parameter.

Arguments:

    hToken - token for user whose classes root is to be opened. If 
        this parameter is NULL, we return ERROR_INVALID_PARAMETER

    phkResult - Returns an open handle to the newly opened key.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:


--*/

{
    NTSTATUS            Status;
    UNICODE_STRING      UsersHive;
    BYTE achBuffer[100];
    PTOKEN_USER pTokenInfo = (PTOKEN_USER) &achBuffer;
    DWORD dwBytesRequired;
    LONG Error;

    //
    //  Caller must pass pointer to the variable where the opened handle
    //  will be returned
    //

    if( phkResult == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == hToken) {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwOptions != REG_OPTION_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!gbCombinedClasses) {
        return ERROR_FILE_NOT_FOUND;
    }
    
    //
    // open up the token to get the sid
    //

    if (!GetTokenInformation(
        hToken,                    // Handle
        TokenUser,                 // TokenInformationClass
        pTokenInfo,                // TokenInformation
        sizeof(achBuffer),         // TokenInformationLength
        &dwBytesRequired           // ReturnLength
        )) {

        Error = GetLastError();

        //
        // Try again if the buffer was too small
        //

        if (ERROR_INSUFFICIENT_BUFFER != Error) {
            return Error ;
        }

        //
        // Allocate space for the user info
        //

        pTokenInfo = (PTOKEN_USER) alloca(dwBytesRequired);

        if (!pTokenInfo) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }


        //
        // Read in the UserInfo
        //

        if (!GetTokenInformation(
            hToken,                // Handle
            TokenUser,                 // TokenInformationClass
            pTokenInfo,                // TokenInformation
            dwBytesRequired,           // TokenInformationLength
            &dwBytesRequired           // ReturnLength
            )) {
            return GetLastError();
        }
    }

    //
    //  Change sid to a string
    //

    Status = RtlConvertSidToUnicodeString(
        &UsersHive,
        pTokenInfo->User.Sid,
        TRUE); // allocate the string

    if (NT_SUCCESS(Status)) {
        
        UNICODE_STRING UserClassesString;

        UserClassesString.MaximumLength = UsersHive.Length + 
            sizeof(REG_USER_CLASSES_PREFIX) + 
            sizeof(REG_USER_CLASSES_SUFFIX);

        UserClassesString.Buffer = (WCHAR*) alloca(UserClassesString.MaximumLength);

        if (UserClassesString.Buffer) {

            UNICODE_STRING UserPrefix;

            //
            // construct the name
            //

            RtlInitUnicodeString(&UserPrefix, REG_USER_CLASSES_PREFIX);

            RtlCopyUnicodeString(&UserClassesString, &UserPrefix);

            Status = RtlAppendUnicodeStringToString(&UserClassesString, &UsersHive);

            if (NT_SUCCESS(Status)) {
                Status = RtlAppendUnicodeToString(&UserClassesString, 
                                                  REG_USER_CLASSES_SUFFIX);
            }

            if (NT_SUCCESS(Status)) {

                OBJECT_ATTRIBUTES Obja;

                // open this key
                InitializeObjectAttributes(
                    &Obja,
                    &UserClassesString,
                    OBJ_CASE_INSENSITIVE,
                    NULL, // using absolute path, no hkey
                    NULL);

                Status = NtOpenKey(
                    phkResult,
                    samDesired,
                    &Obja);
            }

        } else {
            Status = STATUS_NO_MEMORY;
        }

        RtlFreeUnicodeString(&UsersHive);

    }

    if (NT_SUCCESS(Status)) {
#if defined(LEAK_TRACK)

        if (g_RegLeakTraceInfo.bEnableLeakTrack) {
            (void) TrackObject(*phkResult);
        }
        
#endif defined(LEAK_TRACK)

        // mark this key as a class key
        TagSpecialClassesHandle(phkResult);
    }

    return RtlNtStatusToDosError(Status);
}

BOOL InitializeClassesRoot() 
{
    if (!InitializeClassesEnumTable()) {
        return FALSE;
    }

    return TRUE;
}

BOOL CleanupClassesRoot(BOOL fOnlyThisThread) 
{
    //
    // Always remove enumeration states for this thread
    //
    return CleanupClassesEnumTable( fOnlyThisThread );
}

