/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efs.c

Abstract:

    EFS (Encrypting File System) API Interfaces

Author:

    Robert Reichel      (RobertRe)
    Robert Gu           (RobertG)

Environment:

Revision History:

--*/

#undef WIN32_LEAN_AND_MEAN

#include "advapi.h"
#include <windows.h>
#include <feclient.h>

#define FE_CLIENT_DLL      L"feclient.dll"


//
// Global Variables
//

LPFE_CLIENT_INFO    FeClientInfo   = NULL;
HMODULE             FeClientModule = NULL;
CRITICAL_SECTION    FeClientLoadCritical;


LPWSTR
GetFeClientDll(
    VOID
    )
/*++

Routine Description:

    This routine obtains the name of the currently installed client
    encryption dll (which is currently hardcoded).

Arguments:

    None.

Return Value:

    Returns the name of the current DLL, or NULL on error.

--*/

{
    return( FE_CLIENT_DLL );
}


BOOL
LoadAndInitFeClient(
    VOID
    )

/*++

Routine Description:

    This routine finds the name of the proper client dll (by some as of
    yet unspecified means) and proceeds to load it and initialize it.

Arguments:

    None.

Return Value:

    TRUE on success, FALSE on failure.  Callers may call GetLastError()
    for more error information.

--*/
{
    LPWSTR FeClientDllName;
    LPFEAPI_CLIENT_INITIALIZE ClientInitRoutine;
    BOOL Inited;

    //
    // BUGBUG Free this when the time comes, right now it's
    // static.
    //


    FeClientDllName = GetFeClientDll();

    EnterCriticalSection(&FeClientLoadCritical);
    if (FeClientInfo) {
       LeaveCriticalSection(&FeClientLoadCritical);
       return( TRUE );
    }
    if (FeClientDllName) {
        FeClientModule = LoadLibraryW( FeClientDllName );
        if (FeClientModule == NULL) {
            DbgPrint("Unable to load client dll, error = %d\n",GetLastError());
            LeaveCriticalSection(&FeClientLoadCritical);
            return( FALSE );
        }
    }

    ClientInitRoutine = (LPFEAPI_CLIENT_INITIALIZE) GetProcAddress( FeClientModule, (LPCSTR)"FeClientInitialize");



    if (NULL == ClientInitRoutine) {
        FreeLibrary( FeClientModule );
        DbgPrint("Unable to locate init routine, error = %d\n",GetLastError());
        LeaveCriticalSection(&FeClientLoadCritical);
        return( FALSE );
    }

    Inited = (*ClientInitRoutine)( FE_REVISION_1_0, &FeClientInfo );

    LeaveCriticalSection(&FeClientLoadCritical);
    if (!Inited) {
        FreeLibrary( FeClientModule );
        return( FALSE );
    }

    return( TRUE );
}

BOOL
WINAPI
EncryptFileA (
    LPCSTR lpFileName
    )
/*++

Routine Description:

    ANSI Stub to EncryptFileW

Arguments:

    lpFileName - The name of the file to be encrypted.

Return Value:

    TRUE on success, FALSE on failure.  Callers may call GetLastError()
    for more information.

--*/
{
    UNICODE_STRING Unicode;
    WCHAR UnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode.Length = 0;
    Unicode.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof( WCHAR );
    Unicode.Buffer = UnicodeBuffer;

    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return ( EncryptFileW( Unicode.Buffer ));

}


BOOL
WINAPI
EncryptFileW (
    LPCWSTR lpFileName
    )
/*++

Routine Description:

    Win32 EncryptFile API

Arguments:

    lpFileName - Supplies the name of the file to be encrypted.

Return Value:

    TRUE on success, FALSE on failure.  Callers may call GetLastError()
    for more information.

--*/
{
    BOOL rc;
    DWORD Result;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    Result = FeClientInfo->lpServices->EncryptFile( lpFileName );

    if (ERROR_SUCCESS != Result) {
        SetLastError( Result );
        return( FALSE );
    }

    return( TRUE );
}

BOOL
WINAPI
DecryptFileA (
    IN LPCSTR lpFileName,
    IN DWORD  dwRecovery
    )
/*++

Routine Description:

    ANSI Stub for the DecryptFileW API

Arguments:

    lpFileName - Supplies the name of the file to be decrypted.

    dwRecover - Supplies whether this is a recovery operation or a
        normal decryption operation.

Return Value:

    TRUE on success, FALSE on failure.  Callers may call GetLastError()
    for more information.

--*/
{
    UNICODE_STRING Unicode;
    WCHAR UnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode.Length = 0;
    Unicode.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof( WCHAR );
    Unicode.Buffer = UnicodeBuffer;

    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return ( DecryptFileW( Unicode.Buffer, dwRecovery ));
}


BOOL
WINAPI
DecryptFileW (
    IN LPCWSTR lpFileName,
    IN DWORD   dwRecovery
    )
/*++

Routine Description:

    Win32 DecryptFile API

Arguments:

    lpFileName - Supplies the name of the file to be encrypted.

Return Value:

    TRUE on success, FALSE on failure.  Callers may call GetLastError()
    for more information.

--*/
{
    BOOL rc;
    DWORD Result;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    Result = FeClientInfo->lpServices->DecryptFile( lpFileName, dwRecovery );

    if (ERROR_SUCCESS != Result) {
        SetLastError( Result );
        return( FALSE );
    }

    return( TRUE );

}

BOOL
WINAPI
FileEncryptionStatusA (
    LPCSTR    lpFileName,
    LPDWORD   lpStatus
    )
/*++

Routine Description:

    ANSI Stub to FileEncryptionStatusW

Arguments:

    lpFileName - The name of the file to be checked.
    lpStatus - The status of the file.

Return Value:

    TRUE on success, FALSE on failure. Callers may call GetLastError() for more information.

--*/
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING Unicode;
    WCHAR UnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];

    Unicode.Length = 0;
    Unicode.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof( WCHAR );
    Unicode.Buffer = UnicodeBuffer;

    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return ( FileEncryptionStatusW( Unicode.Buffer, lpStatus ));

}

BOOL
WINAPI
FileEncryptionStatusW (
    LPCWSTR    lpFileName,
    LPDWORD    lpStatus
    )
/*++

Routine Description:

    Win32 FileEncryptionStatus API

Arguments:

    lpFileName - Supplies the name of the file to be encrypted.
    lpStatus - The status of the file.

Return Value:

    TRUE on success, FALSE on failure. Callers may call GetLastError()
    for more information.

--*/
{

    BOOL rc;
    DWORD Result;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    return (FeClientInfo->lpServices->FileEncryptionStatus( lpFileName, lpStatus ));

}

DWORD
WINAPI
OpenEncryptedFileRawA(
    LPCSTR          lpFileName,
    ULONG           Flags,
    PVOID *         Context
    )
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING Unicode;
    WCHAR UnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];

    Unicode.Length = 0;
    Unicode.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof( WCHAR );
    Unicode.Buffer = UnicodeBuffer;

    RtlInitAnsiString(&AnsiString,lpFileName);
    Status = RtlAnsiStringToUnicodeString(&Unicode,&AnsiString,FALSE);

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            BaseSetLastNTError(Status);
        }
        return FALSE;
    }

    return ( OpenEncryptedFileRawW( Unicode.Buffer, Flags, Context ));
}




DWORD
WINAPI
OpenEncryptedFileRawW(
    LPCWSTR         lpFileName,
    ULONG           Flags,
    PVOID *         Context
    )
{
    BOOL rc;
    DWORD Result;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(GetLastError());
        }
    }

    return (FeClientInfo->lpServices->OpenFileRaw( lpFileName, Flags, Context ));
}


DWORD
WINAPI
ReadEncryptedFileRaw(
    PFE_EXPORT_FUNC ExportCallback,
    PVOID           CallbackContext,
    PVOID           Context
    )
{
    //
    // It doesn't make sense to call this before calling OpenRaw, so don't
    // bother checking to see if the module is loaded or not.  We'll fault
    // in the user process if it isn't.
    //

    return (FeClientInfo->lpServices->ReadFileRaw( ExportCallback, CallbackContext, Context ));
}

DWORD
WINAPI
WriteEncryptedFileRaw(
    PFE_IMPORT_FUNC ImportCallback,
    PVOID           CallbackContext,
    PVOID           Context
    )
{
    //
    // It doesn't make sense to call this before calling OpenRaw, so don't
    // bother checking to see if the module is loaded or not.  We'll fault
    // in the user process if it isn't.
    //

    return (FeClientInfo->lpServices->WriteFileRaw( ImportCallback, CallbackContext, Context ));
}

VOID
WINAPI
CloseEncryptedFileRaw(
    PVOID           Context
    )
{
    FeClientInfo->lpServices->CloseFileRaw( Context );

    return;
}


DWORD
QueryUsersOnEncryptedFile(
    IN  LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pUsers
    )
/*++

Routine Description:

    Win32 interface for adding users to an encrypted file.

Arguments:

    lpFileName - Supplies the name of the file to be modified.

    pUsers - Returns a list of users on the file.  This parameter
        must be passed to FreeEncryptionCertificateHashList() when
        no longer needed.

Return Value:

    Win32 error.

--*/

{
    DWORD rc;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    if ((lpFileName != NULL) && (pUsers != NULL)) {
        return(FeClientInfo->lpServices->QueryUsers( lpFileName, pUsers ));
    } else {
        return( ERROR_INVALID_PARAMETER );
    }
}


VOID
FreeEncryptionCertificateHashList(
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pUsers
    )
/*++

Routine Description:

    Frees a certificate hash list as returned by QueryUsersOnEncryptedFile()
    and QueryRecoveryAgentsOnEncryptedFile().

Arguments:

    Supplies a list of users returned from QueryUsersOnEncryptedFile().

Return Value:

    Win32 error.

--*/
{

    //
    // It is probably safe to assume that feclient.dll is loaded,
    // since we wouldn't have one of these structures to free
    // if it weren't.
    //

    if (pUsers != NULL) {
        FeClientInfo->lpServices->FreeCertificateHashList( pUsers );
    } else {

        //
        // nothing to do
        //
    }

    return;
}


DWORD
QueryRecoveryAgentsOnEncryptedFile(
    IN  LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pRecoveryAgents
    )
/*++

Routine Description:

    This routine returns a list of recovery agents on an encrypted
    file.

Arguments:

    lpFileName - Supplies the name of the file to be examined.

    pRecoveryAgents - Returns a list of recovery agents, represented
        by certificate hashes on the file.  This list should be freed
        by calling FreeEncryptionCertificateHashList().

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    DWORD rc;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    if ((lpFileName != NULL) && (pRecoveryAgents != NULL)) {
        return(FeClientInfo->lpServices->QueryRecoveryAgents( lpFileName, pRecoveryAgents ));
    } else {
        return( ERROR_INVALID_PARAMETER );
    }
}


DWORD
RemoveUsersFromEncryptedFile(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashes
    )
/*++

Routine Description:

    Takes a list of certificate hashes to be removed
    from the passed file.  Any that are found are removed,
    the rest are ignored with no error return.

Arguments:

    lpFileName - Supplies the name of the file to be modified.

    pHashes - Supplies the list of hashes to be removed.

Return Value:

    Win32 Error

--*/
{
    DWORD rc;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    if ((lpFileName != NULL) && (pHashes != NULL)) {
        return(FeClientInfo->lpServices->RemoveUsers( lpFileName, pHashes ));
    } else {
        return( ERROR_INVALID_PARAMETER );
    }
}

DWORD
AddUsersToEncryptedFile(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_LIST pEncryptionCertificates
    )
/*++

Routine Description:

    This routine adds user keys to the passed encrypted file.

Arguments:

    lpFileName - Supplies the name of the file to be encrypted.

    pEncryptionCertificates - Supplies the list of certificates for
        new users to be added to the file.

Return Value:

    Win32 Error

--*/
{
    DWORD rc;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    if ((lpFileName != NULL) && (pEncryptionCertificates != NULL)) {
        return(FeClientInfo->lpServices->AddUsers( lpFileName, pEncryptionCertificates ));
    } else {
        return( ERROR_INVALID_PARAMETER );
    }
}

DWORD
SetUserFileEncryptionKey(
    PENCRYPTION_CERTIFICATE pEncryptionCertificate
    )
/*++

Routine Description:

    This routine will set the user's current EFS key to the one
    contained in the passed certificate.  If no certificate is
    passed, a new key will be generated automatically.

Arguments:

    pEncryptionCertificate - Optionally supplies the certificate
        containing the new public key.

Return Value:

    Win32 error

--*/
{
    DWORD rc;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    return(FeClientInfo->lpServices->SetKey( pEncryptionCertificate ));

    /*
    if (pEncryptionCertificate != NULL) {
        return(FeClientInfo->lpServices->SetKey( pEncryptionCertificate ));
    } else {
        return( ERROR_INVALID_PARAMETER );
    }*/
}

DWORD
DuplicateEncryptionInfoFile(
    LPCWSTR lpSrcFileName,
    LPCWSTR lpDestFileName
    )
/*++

Routine Description:

    This routine duplicates the encryption information from the source file to the
    destination file.

    The destination file is overwritten.

Arguments:

    lpSrcFileName - Supplies the source of the encryption information.

    lpDestFileName - Supplies the target file.

Return Value:

    Win32 error on failure.

--*/

{
    DWORD rc;

    if (FeClientModule == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    if (lpSrcFileName && lpDestFileName) {
        return(FeClientInfo->lpServices->DuplicateEncryptionInfo( lpSrcFileName, lpDestFileName ));
    } else {
        return( ERROR_INVALID_PARAMETER );
    }
}


BOOL
WINAPI
EncryptionDisable(
    IN LPCWSTR DirPath,
    IN BOOL Disable
    )

/*++

Routine Description:

    This routine disable and enable EFS in the directory DirPath.
        
Arguments:

    DirPath - Directory path.

    Disable - TRUE to disable
    

Return Value:

    TRUE for SUCCESS

--*/
{
    DWORD rc;

    //
    // See if the module has been loaded, and if not, load it into this
    // process.
    //

    if (FeClientInfo == NULL) {
        rc = LoadAndInitFeClient();
        if (!rc) {
            return(rc);
        }
    }

    return(FeClientInfo->lpServices->DisableDir( DirPath, Disable ));

}
