/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    feclient.cpp

Abstract:

    This module implements stubs to call EFS Api

Author:

    Robert Reichel (RobertRe)
    Robert Gu (RobertG)

Revision History:

--*/

//
// Turn off lean and mean so we get wincrypt.h and winefs.h included
//

#undef WIN32_LEAN_AND_MEAN

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <feclient.h>
#include <efsstruc.h>
#include <userenv.h>


//
// Constants used in export\import file
//

#define FILE_SIGNATURE    L"ROBS"
#define STREAM_SIGNATURE  L"NTFS"
#define DATA_SIGNATURE    L"GURE"
#define DEFAULT_STREAM    L"::$DATA"
#define DEF_STR_LEN       14

#define PROPERTY_SET     L":$PROPERTY_SET"
#define INISECTIONNAME   L"Encryption"
#define INIKEYNAME       L"Disable"
#define INIFILENAME      L"\\Desktop.ini"
#define PROPERTY_SET_LEN    wcslen(PROPERTY_SET)

#if DBG

ULONG DebugLevel = 0;

#endif


//
// External prototypes
//
extern "C" {
DWORD
EfsReadFileRawRPCClient(
    IN      PFE_EXPORT_FUNC ExportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    );

DWORD
EfsWriteFileRawRPCClient(
    IN      PFE_IMPORT_FUNC ImportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    );

DWORD
EfsAddUsersRPCClient(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_LIST pEncryptionCertificates
    );


DWORD
EfsRemoveUsersRPCClient(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashes
    );

DWORD
EfsQueryRecoveryAgentsRPCClient(
    IN LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pRecoveryAgents
    );


DWORD
EfsQueryUsersRPCClient(
    IN LPCWSTR lpFileName,
    OUT PENCRYPTION_CERTIFICATE_HASH_LIST * pUsers
    );

DWORD
EfsSetEncryptionKeyRPCClient(
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    );

DWORD
EfsDuplicateEncryptionInfoRPCClient(
    IN LPCWSTR lpSrcFileName,
    IN LPCWSTR lpDestFileName
    );
}




//
// Internal function prototypes
//

NTSTATUS
GetStreamInformation(
    IN HANDLE SourceFile,
    OUT PFILE_STREAM_INFORMATION * StreamInfoBase,
    PULONG StreamInfoSize
    );

DWORD
OpenFileStreams(
    IN HANDLE hSourceFile,
    IN ULONG ShareMode,
    IN PFILE_STREAM_INFORMATION StreamInfoBase,
    IN ULONG FileAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOption,
    OUT PUNICODE_STRING * StreamNames,
    OUT PHANDLE * StreamHandles,
    OUT PULONG StreamCount
    );

VOID
CleanupOpenFileStreams(
    IN PHANDLE Handles OPTIONAL,
    IN PUNICODE_STRING StreamNames OPTIONAL,
    IN PFILE_STREAM_INFORMATION StreamInfoBase OPTIONAL,
    IN HANDLE HSourceFile OPTIONAL,
    IN ULONG StreamCount
    );

ULONG
CheckSignature(
    IN void *Signature
    );

//
// Exported function prototypes
//

DWORD
EfsClientEncryptFile(
    IN LPCWSTR      FileName
    );

DWORD
EfsClientDecryptFile(
    IN LPCWSTR      FileName,
    IN DWORD        Recovery
    );

BOOL
EfsClientFileEncryptionStatus(
    IN LPCWSTR      FileName,
    OUT LPDWORD     lpStatus
    );

DWORD
EfsClientOpenFileRaw(
    IN      LPCWSTR         lpFileName,
    IN      ULONG           Flags,
    OUT     PVOID *         Context
    );

DWORD
EfsClientReadFileRaw(
    IN      PFE_EXPORT_FUNC    ExportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    );

DWORD
EfsClientWriteFileRaw(
    IN      PFE_IMPORT_FUNC    ImportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    );

VOID
EfsClientCloseFileRaw(
    IN      PVOID           Context
    );

DWORD
EfsClientAddUsers(
    IN LPCTSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_LIST pEncryptionCertificates
    );

DWORD
EfsClientRemoveUsers(
    IN LPCTSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashes
    );

DWORD
EfsClientQueryRecoveryAgents(
    IN      LPCTSTR                             lpFileName,
    OUT     PENCRYPTION_CERTIFICATE_HASH_LIST * pRecoveryAgents
    );

DWORD
EfsClientQueryUsers(
    IN      LPCTSTR                             lpFileName,
    OUT     PENCRYPTION_CERTIFICATE_HASH_LIST * pUsers
    );

DWORD
EfsClientSetEncryptionKey(
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    );

VOID
EfsClientFreeHashList(
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashList
    );

DWORD
EfsClientDuplicateEncryptionInfo(
    IN LPCTSTR lpSrcFile,
    IN LPCTSTR lpDestFile
    );

BOOL
EfsClientEncryptionDisable(
    IN LPCWSTR DirPath,
    IN BOOL Disable
	);

FE_CLIENT_DISPATCH_TABLE DispatchTable = {  EfsClientEncryptFile,
                                            EfsClientDecryptFile,
                                            EfsClientFileEncryptionStatus,
                                            EfsClientOpenFileRaw,
                                            EfsClientReadFileRaw,
                                            EfsClientWriteFileRaw,
                                            EfsClientCloseFileRaw,
                                            EfsClientAddUsers,
                                            EfsClientRemoveUsers,
                                            EfsClientQueryRecoveryAgents,
                                            EfsClientQueryUsers,
                                            EfsClientSetEncryptionKey,
                                            EfsClientFreeHashList,
                                            EfsClientDuplicateEncryptionInfo,
                                            EfsClientEncryptionDisable
                                            };


FE_CLIENT_INFO ClientInfo = {
                            FE_REVISION_1_0,
                            &DispatchTable
                            };

//
// Internal function prototypes
//


BOOL
TranslateFileName(
    IN LPCWSTR FileName,
    OUT PUNICODE_STRING FullFileNameU
    );

BOOL
RemoteFile(
    IN LPCWSTR FileName
    );

extern "C"
BOOL
EfsClientInit(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{
    return( TRUE );
}

extern "C"
BOOL
FeClientInitialize(
    IN     DWORD           dwFeRevision,
    OUT    LPFE_CLIENT_INFO       *lpFeInfo
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    dwFeRevision - Is the revision of the current FEAPI interface.

    lpFeInfo - On successful return, must contain a pointer to a structure
         describing the FE Client Interface.  Once returned, the FE Client
         must assume that the caller will continue to reference this table until
         an unload call has been made.  Any changes to this information, or
         deallocation of the memory containing the information may result in
         system corruptions.


Return Value:

    TRUE - Indicates the Client DLL successfully initialized.

    FALSE - Indicates the client DLL has not loaded.  More information may be
         obtained by calling GetLastError().

--*/

{

    *lpFeInfo = &ClientInfo;

    return( TRUE );
}

BOOL
TranslateFileName(
    IN LPCWSTR FileName,
    OUT PUNICODE_STRING FullFileNameU
    )

/*++

Routine Description:

    This routine takes the filename passed by the user and converts
    it to a fully qualified pathname in the passed Unicode string.

Arguments:

    FileName - Supplies the user-supplied file name.

    FullFileNameU - Returns the fully qualified pathname of the passed file.
        The buffer in this string is allocated out of heap memory and
        must be freed by the caller.

Return Value:

    TRUE on success, FALSE otherwise.

--*/


//
// Note: need to free the buffer of the returned string
//
{

    UNICODE_STRING FileNameU;
    BOOLEAN TranslationStatus;
    LPWSTR SrcFileName = (LPWSTR)FileName;
    WCHAR   Sep = L'\\';
    int  SrcLen =wcslen(FileName);

    if (0 == SrcLen) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }
    FullFileNameU->Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0, MAX_PATH * sizeof( WCHAR ));

    if (FullFileNameU->Buffer == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return( FALSE );
    }

    if ((SrcLen >= 5) && (FileName[0] == Sep) && (FileName[1] == Sep) && (FileName[2] == L'?') && (FileName[3] == Sep)){

       if (FileName[5] == L':') {
          SrcFileName = (LPWSTR)&FileName[4];
       } else if ((SrcLen >= 7) && (FileName[4] == L'U') && (FileName[5] == L'N') && (FileName[6] == L'C') && (FileName[7] == Sep)){
          SrcFileName = (LPWSTR)&FileName[6];
          SrcFileName[0] = Sep;
       }

    }

    if ( SrcFileName != (LPWSTR)FileName ){

        //
        // User passed in a FULL path with \\?\ format.
        // RtlGetFullPathName_U may fail if we pass in a long file name without \\?\
        //

        FullFileNameU->Length = wcslen(SrcFileName)*sizeof( WCHAR );
        if ( FullFileNameU->Length >= MAX_PATH * sizeof( WCHAR ) ){
            RtlFreeHeap( RtlProcessHeap(), 0, FullFileNameU->Buffer );
            FullFileNameU->Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0, FullFileNameU->Length + sizeof(WCHAR));

            if (FullFileNameU->Buffer == NULL) {
                if ((SrcLen >= 7) && (SrcFileName == &FileName[6])) {
                    SrcFileName[0] = L'C';
                }
                return( FALSE );
            }
            FullFileNameU->MaximumLength = FullFileNameU->Length + sizeof(WCHAR);
        }
        RtlCopyMemory(FullFileNameU->Buffer, SrcFileName, FullFileNameU->Length + sizeof(WCHAR));

    } else {

        FullFileNameU->MaximumLength = MAX_PATH * sizeof( WCHAR );

        FullFileNameU->Length = (USHORT)RtlGetFullPathName_U(
                                             SrcFileName,
                                             FullFileNameU->MaximumLength,
                                             FullFileNameU->Buffer,
                                             NULL
                                             );

        //
        // The return value is supposed to be the length of the filename, without counting
        // the trailing NULL character.  MAX_PATH is supposed be long enough to contain
        // the length of the file name and the trailing NULL, so what we get back had
        // better be less than MAX_PATH wchars.
        //

        if ( FullFileNameU->Length >= FullFileNameU->MaximumLength ){

            RtlFreeHeap( RtlProcessHeap(), 0, FullFileNameU->Buffer );
            FullFileNameU->Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0, FullFileNameU->Length + sizeof(WCHAR));

            if (FullFileNameU->Buffer == NULL) {
                if (SrcFileName == &FileName[6]) {
                    SrcFileName[0] = L'C';
                }
                return( FALSE );
            }
            FullFileNameU->MaximumLength = FullFileNameU->Length + sizeof(WCHAR);

            FullFileNameU->Length = (USHORT)RtlGetFullPathName_U(
                                                SrcFileName,
                                                FullFileNameU->MaximumLength,
                                                FullFileNameU->Buffer,
                                                NULL
                                                );
        }
    }

    if ((SrcLen >= 7) && (SrcFileName == &FileName[6])) {
        SrcFileName[0] = L'C';
    }

    if (FullFileNameU->Length == 0) {
        //
        // We failed for some reason
        //

        RtlFreeHeap( RtlProcessHeap(), 0, FullFileNameU->Buffer );
        return( FALSE );
    }

    return( TRUE );

}

BOOL
WriteEfsIni(
    IN LPCWSTR SectionName,
	IN LPCWSTR KeyName,
	IN LPCWSTR WriteValue,
	IN LPCWSTR IniFileName
	)
/*++

Routine Description:

    This routine writes to the ini file. A wrap of WritePrivateProfileString
    
Arguments:

    SectionName - Section name (Encryption).

    KeyName - Key name (Disable).
    
    WriteValue - The value to be write (1).
    
    IniFileName - The path for ini file (dir\desktop.ini).

Return Value:

    TRUE on success

--*/
{
    BOOL bRet;

	bRet = WritePrivateProfileString(
                SectionName,
                KeyName,
                WriteValue,
                IniFileName
                );

    //
    // If SetFileAttributes fails, life should go on.
    //

    SetFileAttributes(IniFileName, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN );

    return bRet;
}


BOOL
EfsClientEncryptionDisable(
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
    LPWSTR IniFilePath;
    WCHAR  WriteValue[2];
    BOOL   bRet = FALSE;

    if (DirPath) {

        IniFilePath = (LPWSTR)RtlAllocateHeap( 
                                RtlProcessHeap(), 
                                0,
                                (wcslen(DirPath)+1+wcslen(INIFILENAME))*sizeof(WCHAR) 
                                );
        if (IniFilePath) {
            if (Disable) {
                wcscpy(WriteValue, L"1");
            } else {
                wcscpy(WriteValue, L"0");
            }
    
            wcscpy(IniFilePath, DirPath);
            wcscat(IniFilePath, INIFILENAME);
            bRet = WriteEfsIni(INISECTIONNAME, INIKEYNAME, WriteValue, IniFilePath);
            RtlFreeHeap( RtlProcessHeap(), 0, IniFilePath );
    
        }

    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRet;
}

BOOL
EfsDisabled(
    IN LPCWSTR SectionName,
	IN LPCWSTR KeyName,
	IN LPCWSTR IniFileName
	)
/*++

Routine Description:

    This routine checks if the encryption has been turned off for the ini file.
        
Arguments:

    SectionName - Section name (Encryption).

    KeyName - Key name (Disable).
    
    IniFileName - The path for ini file (dir\desktop.ini).

Return Value:

    TRUE for disabled

--*/
{
    DWORD ValueLength;
    WCHAR ResultString[4];

    memset( ResultString, 0, 4 );

    ValueLength = GetPrivateProfileString(
                      SectionName,
                      KeyName,
                      L"0",
                      ResultString,
                      sizeof(ResultString),
                      IniFileName
                      );

    //
    // If GetPrivateProfileString failed, EFS will be enabled
    //

    return (!wcscmp(L"1", ResultString));
}

BOOL
DirEfsDisabled(
    IN LPCWSTR  DirName
    )
/*++

Routine Description:

    This routine checks if the encryption has been turned off for the dir.
        
Arguments:

    SectionName - Section name (Encryption).

    KeyName - Key name (Disable).
    
    IniFileName - The path for ini file (dir\desktop.ini).

Return Value:

    TRUE for disabled

--*/
{
    LPWSTR FileName;
    DWORD  FileLength = (wcslen(INIFILENAME)+wcslen(DirName)+1)*sizeof (WCHAR);
    BOOL   bRet = FALSE;

    FileName = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0, FileLength );
    if (FileName) {
        wcscpy( FileName, DirName );
        wcscat( FileName, INIFILENAME );
        bRet = EfsDisabled( INISECTIONNAME, INIKEYNAME, FileName );
        RtlFreeHeap( RtlProcessHeap(), 0, FileName );
    }

    return bRet;
}

BOOL
RemoteFile(
    IN LPCWSTR FileName
    )
/*++

Routine Description:

    This routine checks if the file is a local file.
    BUGBUG. If a UNC name is passed in, it assumes a remote file.

Arguments:

    FileName - Supplies the user-supplied file name.

Return Value:

    TRUE for remote file.

--*/

{

    if ( FileName[1] == L':' ){

        WCHAR DriveLetter[3];
        DWORD BufferLength = 3;
        DWORD RetCode = ERROR_SUCCESS;

        DriveLetter[0] = FileName[0];
        DriveLetter[1] = FileName[1];
        DriveLetter[2] = 0;

        RetCode = WNetGetConnectionW(
                                DriveLetter,
                                DriveLetter,
                                &BufferLength
                                );

        if (RetCode == ERROR_NOT_CONNECTED) {
            return FALSE;
        } else {
            return TRUE;
        }

    } else {
        return TRUE;
    }

}

DWORD
EfsClientEncryptFile(
    IN LPCWSTR      FileName
    )
{
    DWORD           rc;
    BOOL            Result;


    UNICODE_STRING FullFileNameU;

    if (NULL == FileName) {
       return ERROR_INVALID_PARAMETER;
    }
    Result = TranslateFileName( FileName, &FullFileNameU );

    if (Result) {

        //
        // Call the server
        //

        rc = EfsEncryptFileRPCClient( &FullFileNameU );
        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {
        rc = GetLastError();
    }

    return( rc );
}

DWORD
EfsClientDecryptFile(
    IN LPCWSTR      FileName,
    IN DWORD        dwRecovery
    )
{
    DWORD           rc;
    BOOL            Result;

    UNICODE_STRING FullFileNameU;

    if (NULL == FileName) {
       return ERROR_INVALID_PARAMETER;
    }
    Result = TranslateFileName( FileName, &FullFileNameU );

    if (Result) {

        //
        // Call the server
        //

        //rc = EfsDecryptFileAPI( &FullFileNameU, dwRecovery );
        rc = EfsDecryptFileRPCClient( &FullFileNameU, dwRecovery );
        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {
        rc = GetLastError();
    }


    return( rc );
}

BOOL
EfsClientFileEncryptionStatus(
    IN LPCWSTR      FileName,
    OUT LPDWORD      lpStatus
    )
/*++

Routine Description:

    This routine checks if a file is encryptable or not.

    BUGBUG, We do not test the NTFS Volume 5 for the reason of performance
                      at this version. We disable the encryption from %windir% down.
                      We might change these features later.

Arguments:

    FileName - The file to be checked.

    lpStatus - The encryption status of the file. Error code if the return value is
                    FALSE.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    BOOL            Result;
    DWORD        FileAttributes;

    UNICODE_STRING FullFileNameU;

    if ((NULL == FileName) || ( NULL == lpStatus)) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    //
    // GetFileAttributes should use the name before TanslateFileName
    // in case the passed in name is longer than MAX_PATH and using the
    // format \\?\
    //

    FileAttributes = GetFileAttributes( FileName );

    if (FileAttributes == -1){
        *lpStatus = GetLastError();
        return FALSE;
    }

    Result = TranslateFileName( FileName, &FullFileNameU );

    ASSERT(FullFileNameU.Buffer[FullFileNameU.Length / 2] == 0);

    if (Result) {

        if ( (FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) ||
             (FileAttributes & FILE_ATTRIBUTE_SYSTEM) ) {

            //
            // File not encryptable. Either it is encypted or a system file.
            //

            if ( FileAttributes & FILE_ATTRIBUTE_ENCRYPTED ){

                *lpStatus = FILE_IS_ENCRYPTED;

            } else {

                *lpStatus = FILE_SYSTEM_ATTR ;

            }

        } else {

            LPWSTR  TmpBuffer;
            LPWSTR  FullPathName;
            UINT    TmpBuffLen;
            UINT    FullPathLen;
            UINT    PathLength;
            BOOL    GotRoot;
            BOOL    EfsDisabled = FALSE;

            //
            // Check if it is the root.
            //

            if ( FullFileNameU.Length >= MAX_PATH * sizeof(WCHAR)){

                //
                // We need to put back the \\?\ or \\?\UNC\ to use the
                // Win 32 API
                //

                FullPathLen = FullFileNameU.Length + 7 * sizeof(WCHAR);
                TmpBuffLen = FullPathLen;
                FullPathName = (LPWSTR)RtlAllocateHeap(
                                            RtlProcessHeap(),
                                            0,
                                            FullPathLen
                                            );
                TmpBuffer = (LPWSTR)RtlAllocateHeap(
                                            RtlProcessHeap(),
                                            0,
                                            TmpBuffLen
                                            );

                if ((FullPathName == NULL) || (TmpBuffer == NULL)){
                    RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);
                    if (FullPathName){
                        RtlFreeHeap(RtlProcessHeap(), 0, FullPathName);
                    }
                    if (TmpBuffer){
                        RtlFreeHeap(RtlProcessHeap(), 0, TmpBuffer);
                    }
                    *lpStatus = ERROR_OUTOFMEMORY;
                    return FALSE;
                }

                if ( FullFileNameU.Buffer[0] == L'\\' ){

                    //
                    // Put back the \\?\UNC\
                    //

                    wcscpy(FullPathName, L"\\\\?\\UNC");
                    wcscat(FullPathName, &(FullFileNameU.Buffer[1]));
                    FullPathLen = FullFileNameU.Length + 6 * sizeof(WCHAR);

                } else {

                    //
                    // Put back the \\?\
                    //

                    wcscpy(FullPathName, L"\\\\?\\");
                    wcscat(FullPathName, FullFileNameU.Buffer);
                    FullPathLen = FullFileNameU.Length + 4 * sizeof(WCHAR);
                }

            } else {
                TmpBuffLen = MAX_PATH * sizeof(WCHAR);
                TmpBuffer = (LPWSTR)RtlAllocateHeap(
                                            RtlProcessHeap(),
                                            0,
                                            TmpBuffLen
                                            );
                if (TmpBuffer == NULL){
                    RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);
                    *lpStatus = ERROR_OUTOFMEMORY;
                    return FALSE;
                }

                FullPathName = FullFileNameU.Buffer;
                FullPathLen = FullFileNameU.Length;
            }

            //
            // Check desktop.ini here
            //


            wcscpy(TmpBuffer, FullFileNameU.Buffer); 
            if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                //
                // This is a file. Get the DIR path
                //

                int ii;

                ii = wcslen(TmpBuffer) - 1;
                while ((ii >= 0) && (TmpBuffer[ii] != L'\\')) {
                    ii--;
                }
                if (ii>=0) {
                    TmpBuffer[ii] = 0;
                }

            }

            EfsDisabled = DirEfsDisabled( TmpBuffer );

            if (EfsDisabled) {
               *lpStatus = FILE_DIR_DISALLOWED;
            } else if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (FileAttributes & FILE_ATTRIBUTE_READONLY)){

                //
                // Read only file
                //

                *lpStatus = FILE_READ_ONLY;
            } else {
                GotRoot = GetVolumePathName(
                                    FullPathName,
                                    TmpBuffer,
                                    TmpBuffLen
                                    );
    
                if ( GotRoot ){
    
                    DWORD RootLength = wcslen(TmpBuffer) - 1;
                    TmpBuffer[RootLength] = NULL;
                    if ( (FullPathLen == RootLength * sizeof (WCHAR))
                           && !wcscmp(TmpBuffer, FullPathName)){
    
                        //
                        // It is the root
                        //
    
                        *lpStatus = FILE_ROOT_DIR;
    
                    } else {
    
                        //
                        // Check if it is the Windows\system32 directories
                        //
    
                        PathLength = GetSystemWindowsDirectory( TmpBuffer, TmpBuffLen );                    
                        //PathLength = GetWindowsDirectory( TmpBuffer, TmpBuffLen );
                        //PathLength = GetSystemDirectory( TmpBuffer, TmpBuffLen );
    
                        ASSERT(PathLength <= TmpBuffLen);
    
                        if ( PathLength > TmpBuffLen ) {
    
                            //
                            // BUGBUG. Unlikely, but possible. Ignore now.
                            //
    
                            *lpStatus = FILE_UNKNOWN ;
    
                        } else {
    
                            if ( ( FullFileNameU.Length < PathLength * sizeof (WCHAR) ) ||
                                  ( ( FullFileNameU.Buffer[PathLength] ) &&
                                    ( FullFileNameU.Buffer[PathLength] != L'\\') )){
    
                                //
                                // Check if a remote file
                                //
    
                                if ( RemoteFile( FullFileNameU.Buffer ) ){
    
                                    *lpStatus = FILE_UNKNOWN;
    
                                } else {
    
                                    *lpStatus = FILE_ENCRYPTABLE;
    
                                }
    
                            } else {
    
                                if ( _wcsnicmp(TmpBuffer, FullFileNameU.Buffer, PathLength)){
    
                                    //
                                    // Not under %SystemRoot%
                                    //
    
                                    if ( RemoteFile( FullFileNameU.Buffer ) ){
    
                                        *lpStatus = FILE_UNKNOWN;
    
                                    } else {
    
                                        *lpStatus = FILE_ENCRYPTABLE;
    
                                    }
                                } else {
    
                                    //
                                    // In windows root directory. WINNT
                                    //
    
                                    BOOL bRet;
                                    DWORD allowPathLen;
    
                                    //
                                    // Check for allow lists
                                    //
    
                                    allowPathLen = (DWORD) TmpBuffLen;
                                    bRet = GetProfilesDirectory(TmpBuffer, &allowPathLen);
                                    if (!bRet){
                                        RtlFreeHeap(RtlProcessHeap(), 0, TmpBuffer);
                                        TmpBuffer = (LPWSTR)RtlAllocateHeap(
                                                                RtlProcessHeap(),
                                                                0,
                                                                allowPathLen
                                                                );
                                        if (TmpBuffer){
                                            bRet = GetProfilesDirectory(TmpBuffer, &allowPathLen);
                                        } else {
                                            *lpStatus = ERROR_OUTOFMEMORY;
                                            Result = FALSE;
                                        }
                                    }
                                    if (bRet){
    
                                        //
                                        // Check for Profiles directory
                                        //
    
                                        if (!_wcsnicmp(TmpBuffer, FullFileNameU.Buffer, allowPathLen - 1)){
                                            *lpStatus = FILE_ENCRYPTABLE;
                                        } else {
    
                                            //
                                            // Under %windir% but not profiles
                                            //
    
                                            *lpStatus = FILE_SYSTEM_DIR;
                                        }
                                    } else {
    
                                        if ( *lpStatus != ERROR_OUTOFMEMORY){
    
                                            //
                                            // This should not happen, unless a bug in GetProfilesDirectoryEx()
                                            //
                                            ASSERT(FALSE);
    
                                            *lpStatus = FILE_UNKNOWN;
                                        }
    
                                    }
                                }
                            }
                        }
                    }
                } else {
    
                    //
                    // BUGBUG Cannot get the root, should we fail the call?
                    //
    
                    *lpStatus = FILE_UNKNOWN ;
    
                }
            }

            if ((FullPathName != FullFileNameU.Buffer) && FullPathName){
                RtlFreeHeap(RtlProcessHeap(), 0, FullPathName);
            }

            if (TmpBuffer){
                RtlFreeHeap(RtlProcessHeap(), 0, TmpBuffer);
            }

        }

        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {
        *lpStatus = GetLastError();
    }

    return  Result;
}

DWORD
EfsClientOpenFileRaw(
    IN      LPCWSTR         FileName,
    IN      ULONG           Flags,
    OUT     PVOID *         Context
    )

/*++

Routine Description:

    This routine is used to open an encrypted file. It opens the file and
    prepares the necessary context to be used in ReadRaw data and WriteRaw
    data.


Arguments:

    FileName  --  File name of the file to be exported

    Flags -- Indicating if open for export or import; for directory or file.

    Context - Export context to be used by READ operation later. Caller should
              pass this back in ReadRaw().


Return Value:

    Result of the operation.

--*/

{
    DWORD        rc;
    BOOL            Result;
    UNICODE_STRING FullFileNameU;

    if ((NULL == FileName) || ( NULL == Context)) {
       return ERROR_INVALID_PARAMETER;
    }

    Result = TranslateFileName( FileName, &FullFileNameU );

    if (Result) {

        rc =  (EfsOpenFileRawRPCClient(
                        FullFileNameU.Buffer,
                        Flags,
                        Context
                        )
                    );

        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {
        rc = GetLastError();
    }

    return rc;

}

DWORD
EfsClientReadFileRaw(
    IN      PFE_EXPORT_FUNC ExportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    )
/*++

Routine Description:

    This routine is used to read encrypted file's raw data. It uses
    NTFS FSCTL to get the data.

Arguments:

    ExportCallback --  Caller supplied callback function to process the
                       raw data.

    CallbackContext -- Caller's context passed back in ExportCallback.

    Context - Export context created in the CreateRaw.

Return Value:

    Result of the operation.

--*/

{
    return ( EfsReadFileRawRPCClient(
                    ExportCallback,
                    CallbackContext,
                    Context
                    ));
}

DWORD
EfsClientWriteFileRaw(
    IN      PFE_IMPORT_FUNC ImportCallback,
    IN      PVOID           CallbackContext,
    IN      PVOID           Context
    )

/*++

Routine Description:

    This routine is used to write encrypted file's raw data. It uses
    NTFS FSCTL to write the data.

Arguments:

    ImportCallback --  Caller supplied callback function to provide the
                       raw data.

    CallbackContext -- Caller's context passed back in ImportCallback.

    Context - Import context created in the CreateRaw.

Return Value:

    Result of the operation.

--*/

{

    return ( EfsWriteFileRawRPCClient(
                    ImportCallback,
                    CallbackContext,
                    Context
                    ));
}

VOID
EfsClientCloseFileRaw(
    IN      PVOID           Context
    )
/*++

Routine Description:

    This routine frees the resources allocated by the CreateRaw

Arguments:

    Context - Created by the CreateRaw.

Return Value:

    NO.

--*/
{
    if ( !Context ){
        return;
    }

    EfsCloseFileRawRPCClient( Context );
}


//
// Beta 2 API
//

DWORD
EfsClientAddUsers(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_LIST pEncryptionCertificates
    )
/*++

Routine Description:

    Calls client stub for AddUsersToFile EFS API.

Arguments:

    lpFileName - Supplies the name of the file to be modified.

    nUsers - Supplies the number of entries in teh pEncryptionCertificates array

    pEncryptionCertificates - Supplies an array of pointers to PENCRYPTION_CERTIFICATE
        structures.  Length of array is given in nUsers parameter.

Return Value:

--*/
{
    DWORD        rc;
    UNICODE_STRING FullFileNameU;


    if ((NULL == lpFileName) || (NULL == pEncryptionCertificates)) {
       return ERROR_INVALID_PARAMETER;
    }
    if (TranslateFileName( lpFileName, &FullFileNameU )) {

        rc = EfsAddUsersRPCClient( FullFileNameU.Buffer, pEncryptionCertificates );

        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {

        rc = GetLastError();
    }

    return rc;
}

DWORD
EfsClientRemoveUsers(
    IN LPCWSTR lpFileName,
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashes
    )
/*++

Routine Description:

    Calls client stub for RemoveUsersFromFile EFS API

Arguments:

    lpFileName - Supplies the name of the file to be modified.

    pHashes - Supplies a structure containing a list of PENCRYPTION_CERTIFICATE_HASH
        structures, each of which represents a user to remove from the specified file.

Return Value:

--*/
{
    DWORD        rc;
    UNICODE_STRING FullFileNameU;


    if ((NULL == lpFileName) || (NULL == pHashes) || (pHashes->pUsers == NULL)) {
       return ERROR_INVALID_PARAMETER;
    }
    if (TranslateFileName( lpFileName, &FullFileNameU )) {

        rc =  EfsRemoveUsersRPCClient( FullFileNameU.Buffer, pHashes );

        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {

        rc = GetLastError();
    }

    return rc;
}

DWORD
EfsClientQueryRecoveryAgents(
    IN      LPCWSTR                             lpFileName,
    OUT     PENCRYPTION_CERTIFICATE_HASH_LIST * pRecoveryAgents
    )
/*++

Routine Description:

    Calls client stub for QueryRecoveryAgents EFS API

Arguments:

    lpFileName - Supplies the name of the file to be modified.

    pRecoveryAgents - Returns a pointer to a structure containing a list
        of PENCRYPTION_CERTIFICATE_HASH structures, each of which represents
        a recovery agent on the file.

Return Value:

--*/
{
    DWORD        rc;
    UNICODE_STRING FullFileNameU;


    if ((NULL == lpFileName) || (NULL == pRecoveryAgents)) {
       return ERROR_INVALID_PARAMETER;
    }
    if (TranslateFileName( lpFileName, &FullFileNameU )) {

        rc =  EfsQueryRecoveryAgentsRPCClient( FullFileNameU.Buffer, pRecoveryAgents );

        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {

        rc = GetLastError();
    }

    return rc;
}

DWORD
EfsClientQueryUsers(
    IN      LPCWSTR                             lpFileName,
    OUT     PENCRYPTION_CERTIFICATE_HASH_LIST * pUsers
    )
/*++

Routine Description:

    Calls client stub for QueryUsersOnFile EFS API

Arguments:

    lpFileName - Supplies the name of the file to be modified.

    pUsers - Returns a pointer to a structure containing a list
        of PENCRYPTION_CERTIFICATE_HASH structures, each of which represents
        a user of this file (that is, someone who can decrypt the file).

Return Value:

--*/
{
    DWORD        rc;
    UNICODE_STRING FullFileNameU;

    if ((NULL == lpFileName) || (NULL == pUsers)) {
       return ERROR_INVALID_PARAMETER;
    }
    if (TranslateFileName( lpFileName, &FullFileNameU )) {

        rc =  EfsQueryUsersRPCClient( FullFileNameU.Buffer, pUsers );

        RtlFreeHeap(RtlProcessHeap(), 0, FullFileNameU.Buffer);

    } else {

        rc = GetLastError();
    }

    return rc;
}


DWORD
EfsClientSetEncryptionKey(
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    )
/*++

Routine Description:

    Calls client stub for SetFileEncryptionKey EFS API

Arguments:

    pEncryptionCertificate - Supplies a pointer to an EFS certificate
        representing the public key to use for future encryption operations.

Return Value:

--*/
{
    /*
    if ((NULL == pEncryptionCertificate) || ( NULL == pEncryptionCertificate->pCertBlob)) {
       return ERROR_INVALID_PARAMETER;
    }
    */

    if ( pEncryptionCertificate && ( NULL == pEncryptionCertificate->pCertBlob)) {
       return ERROR_INVALID_PARAMETER;
    }

    DWORD rc =  EfsSetEncryptionKeyRPCClient( pEncryptionCertificate );

    return( rc );
}

VOID
EfsClientFreeHashList(
    IN PENCRYPTION_CERTIFICATE_HASH_LIST pHashList
    )
/*++

Routine Description:

    This routine frees the memory allocated by a call to
    QueryUsersOnEncryptedFile and QueryRecoveryAgentsOnEncryptedFile

Arguments:

    pHashList - Supplies the hash list to be freed.

Return Value:

    None.  Faults in user's context if passed bogus data.

--*/

{
    if (NULL == pHashList) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return;
    }

    for (DWORD i=0; i<pHashList->nCert_Hash ; i++) {

         PENCRYPTION_CERTIFICATE_HASH pHash = pHashList->pUsers[i];

         if (pHash->lpDisplayInformation) {
             MIDL_user_free( pHash->lpDisplayInformation );
         }

         if (pHash->pUserSid) {
             MIDL_user_free( pHash->pUserSid );
         }

         MIDL_user_free( pHash->pHash->pbData );
         MIDL_user_free( pHash->pHash );
         MIDL_user_free( pHash );
    }

    MIDL_user_free( pHashList->pUsers );
    MIDL_user_free( pHashList );

    return;
}

DWORD
EfsClientDuplicateEncryptionInfo(
    IN LPCWSTR lpSrcFile,
    IN LPCWSTR lpDestFile
    )
{
    DWORD rc;

    UNICODE_STRING SrcFullFileNameU;
    UNICODE_STRING DestFullFileNameU;

    if (TranslateFileName( lpSrcFile, &SrcFullFileNameU )) {

        if (TranslateFileName( lpDestFile, &DestFullFileNameU )) {

            rc = EfsDuplicateEncryptionInfoRPCClient(
                    SrcFullFileNameU.Buffer,
                    DestFullFileNameU.Buffer
                    );

            RtlFreeHeap(RtlProcessHeap(), 0, DestFullFileNameU.Buffer);

        } else {

            rc = GetLastError();
        }

        RtlFreeHeap(RtlProcessHeap(), 0, SrcFullFileNameU.Buffer);

    } else {

        rc = GetLastError();
    }

    return( rc );

}
