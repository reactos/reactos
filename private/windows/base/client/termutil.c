/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    termutil.c

Abstract:

    Terminal server support functions and inifile syncing/merging code

Author:


Revision History:

--*/

#include "basedll.h"
#include "hydra\regapi.h"

#define TERMSRV_INIFILE_TIMES L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\IniFile Times"


BOOL IsTerminalServerCompatible(VOID)
{

PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader( NtCurrentPeb()->ImageBaseAddress );

    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL IsTSAppCompatEnabled(VOID)
{

   NTSTATUS NtStatus;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UniString;
   HKEY   hKey = 0;
   ULONG  ul, ulcbuf;
   PKEY_VALUE_PARTIAL_INFORMATION pKeyValInfo = NULL;

   BOOL retval = TRUE;


   RtlInitUnicodeString(&UniString,REG_NTAPI_CONTROL_TSERVER);



   // Determine the value info buffer size
   ulcbuf = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR) + 
            sizeof(ULONG);

   pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                 0, 
                                 ulcbuf);

   // Did everything initialize OK?
   if (UniString.Buffer && pKeyValInfo) {

       InitializeObjectAttributes(&ObjectAttributes,
                                  &UniString,
                                  OBJ_CASE_INSENSITIVE,
                                  NULL,
                                  NULL
                                 );
   
       NtStatus = NtOpenKey(&hKey, KEY_READ, &ObjectAttributes);
   
       if (NT_SUCCESS(NtStatus)) {
   
           RtlInitUnicodeString(&UniString, 
                               L"TSAppCompat");
           NtStatus = NtQueryValueKey(hKey,
                                      &UniString,
                                      KeyValuePartialInformation,
                                      pKeyValInfo,
                                      ulcbuf,
                                      &ul);

           if (NT_SUCCESS(NtStatus) && (REG_DWORD == pKeyValInfo->Type)) {

               if ((*(PULONG)pKeyValInfo->Data) == 0) {
                  retval = FALSE;
               }

           }

           NtClose(hKey);
       }
   }

   // Free up the buffers we allocated
   // Need to zero out the buffers, because some apps (MS Internet Assistant)
   // won't install if the heap is not zero filled.
   if (pKeyValInfo) {
       memset(pKeyValInfo, 0, ulcbuf);
       RtlFreeHeap( RtlProcessHeap(), 0, pKeyValInfo );
   }

   return(retval);

}

BOOL IsSystemLUID(VOID)
{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ sizeof( TOKEN_STATISTICS ) ];
    ULONG       ReturnLength;
    LUID        CurrentLUID = { 0, 0 };
    LUID        SystemLUID = SYSTEM_LUID;
    NTSTATUS Status;

    if ( CurrentLUID.LowPart == 0 && CurrentLUID.HighPart == 0 ) {

        Status = NtOpenProcessToken( NtCurrentProcess(),
                                     TOKEN_QUERY,
                                     &TokenHandle );
        if ( !NT_SUCCESS( Status ) )
            return(TRUE);

        NtQueryInformationToken( TokenHandle, TokenStatistics, &TokenInformation,
                                 sizeof(TokenInformation), &ReturnLength );
        NtClose( TokenHandle );

        RtlCopyLuid(&CurrentLUID,
                    &(((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId));
    }

    if (RtlEqualLuid(&CurrentLUID, &SystemLUID)) {
        return(TRUE);
    } else {
        return(FALSE );
    }
}


void InitializeTermsrvFpns(void)
{

    HANDLE          dllHandle;

    if (IsTerminalServerCompatible() ||
       (IsSystemLUID()) ||
       (!IsTSAppCompatEnabled())) {
        return;
    }

    //
    // Load Terminal Server application compatibility dll
    //
    dllHandle = LoadLibrary("tsappcmp.dll");

    if (dllHandle) {

        gpTermsrvFormatObjectName =
            (PTERMSRVFORMATOBJECTNAME)GetProcAddress(dllHandle,"TermsrvFormatObjectName");

        gpTermsrvGetComputerName =
            (PTERMSRVGETCOMPUTERNAME)GetProcAddress(dllHandle,"TermsrvGetComputerName");

        gpTermsrvAdjustPhyMemLimits =
                (PTERMSRVADJUSTPHYMEMLIMITS)GetProcAddress(dllHandle,"TermsrvAdjustPhyMemLimits");

        gpTermsrvGetWindowsDirectoryA =
                (PTERMSRVGETWINDOWSDIRECTORYA)GetProcAddress(dllHandle,"TermsrvGetWindowsDirectoryA");

        gpTermsrvGetWindowsDirectoryW =
                (PTERMSRVGETWINDOWSDIRECTORYW)GetProcAddress(dllHandle,"TermsrvGetWindowsDirectoryW");

        gpTermsrvConvertSysRootToUserDir =
                (PTERMSRVCONVERTSYSROOTTOUSERDIR)GetProcAddress(dllHandle,"TermsrvConvertSysRootToUserDir");

        gpTermsrvBuildIniFileName =
                (PTERMSRVBUILDINIFILENAME)GetProcAddress(dllHandle,"TermsrvBuildIniFileName");

        gpTermsrvCORIniFile =
                (PTERMSRVCORINIFILE)GetProcAddress(dllHandle,"TermsrvCORIniFile");

        gpGetTermsrCompatFlags =
                (PGETTERMSRCOMPATFLAGS)GetProcAddress(dllHandle,"GetTermsrCompatFlags");

        gpTermsrvBuildSysIniPath =
                (PTERMSRVBUILDSYSINIPATH)GetProcAddress(dllHandle,"TermsrvBuildSysIniPath");

        gpTermsrvCopyIniFile =
        (PTERMSRVCOPYINIFILE)GetProcAddress(dllHandle,"TermsrvCopyIniFile");

        gpTermsrvGetString =
        (PTERMSRVGETSTRING)GetProcAddress(dllHandle,"TermsrvGetString");

        gpTermsrvLogInstallIniFile =
        (PTERMSRVLOGINSTALLINIFILE)GetProcAddress(dllHandle,"TermsrvLogInstallIniFile");


    }
}


/*****************************************************************************
 *
 *  TermsrvAppInstallMode
 *
 *   Returns whether the system is in Install mode or not
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

WINBASEAPI
BOOL
WINAPI
TermsrvAppInstallMode( VOID )
{

    if ( BaseStaticServerData->fTermsrvAppInstallMode )
        return( TRUE );

    return( FALSE );
}


/*****************************************************************************
 *
 *  SetTermsrvAppInstallMode
 *
 *   Turns App install mode on or off. Default is off
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

WINBASEAPI
BOOL
WINAPI
SetTermsrvAppInstallMode( BOOL bState )
{
    NTSTATUS Status;

#if defined(BUILD_WOW6432)
    Status = CsrBasepSetTermsrvAppInstallMode(bState);
#else
    BASE_API_MSG m;
    PBASE_SET_TERMSRVAPPINSTALLMODE c= (PBASE_SET_TERMSRVAPPINSTALLMODE)&m.u.SetTermsrvAppInstallMode;

    c->bState = bState;
    Status = CsrClientCallServer((PCSR_API_MSG)&m, NULL,
                                 CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                                     BasepSetTermsrvAppInstallMode),
                                 sizeof( *c ));
#endif

    if ( !NT_SUCCESS( Status ) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return( FALSE );
    }


    if (IsTerminalServer() && IsTSAppCompatEnabled()) {
        //
        // Load tsappcmp.dll
        //
        if (gpTermsrvUpdateAllUserMenu == NULL) {
            HANDLE          dllHandle;
    
            //
            // Load Terminal Server application compatibility dll
            //
            dllHandle = LoadLibrary("tsappcmp.dll");
    
            if (dllHandle) {
                gpTermsrvUpdateAllUserMenu =
                        (PTERMSRVUPDATEALLUSERMENU)GetProcAddress(dllHandle,"TermsrvUpdateAllUserMenu");
            }
        }
    
        if (gpTermsrvUpdateAllUserMenu) {
            gpTermsrvUpdateAllUserMenu( bState == TRUE ? 0 : 1 );
        }
    }

    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/*
  Ini File syncing/merging code

*/
//////////////////////////////////////////////////////////////////////////////
/* External Functions */
NTSTATUS
BaseDllOpenIniFileOnDisk(
    PINIFILE_PARAMETERS a
    );

NTSTATUS
BaseDllWriteKeywordValue(
    IN PINIFILE_PARAMETERS a,
    IN PUNICODE_STRING VariableName OPTIONAL
    );

NTSTATUS
BaseDllCloseIniFileOnDisk(
    IN PINIFILE_PARAMETERS a
    );

NTSTATUS
BaseDllFindSection(
    IN PINIFILE_PARAMETERS a
    );

NTSTATUS
BaseDllFindKeyword(
    IN PINIFILE_PARAMETERS a
    );

NTSTATUS
TermsrvIniSyncLoop( HANDLE SrcHandle,
                PINIFILE_PARAMETERS a,
                PBOOLEAN pfIniUpdated
              );
NTSTATUS
TermsrvGetSyncTime( PUNICODE_STRING pSysIniPath,
                PUNICODE_STRING pUserBasePath,
                PLARGE_INTEGER  pLastSyncTime
              );

NTSTATUS
TermsrvPutSyncTime( PUNICODE_STRING pSysIniPath,
                PUNICODE_STRING pUserBasePath,
                PLARGE_INTEGER  pLastSyncTime
              );


/*****************************************************************************
 *
 *  TermsrvGetSyncTime
 *
 *  This routine will get the time of the system ini file that the user ini
 *  file was last sync'd with.
 *
 * ENTRY:
 *   PUNICODE_STRING pSysIniPath (In) - NT fully qualified system ini path
 *   PUNICODE_STRING pUserBasePath (In) - NT fully qualified user directory path
 *   PLARGE_INTEGER pLastSyncTime (OUT) - ptr to return last sync time
 *
 * EXIT:
 *   STATUS_SUCCESS - successfully retrieved the last sync time from infile.upd
 *
 ****************************************************************************/

NTSTATUS
TermsrvGetSyncTime(
    PUNICODE_STRING pSysIniPath,
    PUNICODE_STRING pUserBasePath,
    PLARGE_INTEGER  pLastSyncTime)
{
    NTSTATUS Status;
    HANDLE   hUpdate = NULL;
    OBJECT_ATTRIBUTES ObjAUpd;
    IO_STATUS_BLOCK   Iosb;
    FILE_STANDARD_INFORMATION StandardInfo;
    WCHAR             wcUpdateFile[MAX_PATH+1];
    UNICODE_STRING    UniUpdateName = {0,
                                       sizeof(wcUpdateFile),
                                       wcUpdateFile};
    PCHAR             pBuff = NULL, pBuffEnd;
    PWCH              pwch;
    SIZE_T            ulBuffSize;
    LONG              lresult;

    wcscpy(wcUpdateFile, pUserBasePath->Buffer);
    wcscat(wcUpdateFile, L"\\inifile.upd");
    UniUpdateName.Length = wcslen(wcUpdateFile)*sizeof(WCHAR);
    pLastSyncTime->LowPart = 0;
    pLastSyncTime->HighPart = 0;

    InitializeObjectAttributes( &ObjAUpd,
                                &UniUpdateName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    // Open the update log
    Iosb.Status = STATUS_SUCCESS;
    Status = NtOpenFile( &hUpdate,
                         FILE_GENERIC_READ,
                         &ObjAUpd,
                         &Iosb,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT    // OpenOptions
                       );

    // Get the size of the file
    if (NT_SUCCESS( Status )) {
        Status = NtQueryInformationFile( hUpdate,
                                         &Iosb,
                                         &StandardInfo,
                                         sizeof(StandardInfo),
                                         FileStandardInformation
                                       );
        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_SUCCESS;
        }
#if DBG
        else if (!NT_SUCCESS( Status )) {
            DbgPrint( "TermsrvGetSyncTime: Unable to QueryInformation for %wZ - Status == %x\n", &UniUpdateName, Status );
        }
#endif
    }

    if (NT_SUCCESS( Status )) {
        ulBuffSize = StandardInfo.EndOfFile.LowPart + 4 * sizeof(WCHAR);
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &pBuff,
                                          0,
                                          &ulBuffSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE
                                        );
    }

    if (NT_SUCCESS( Status )) {
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &pBuff,
                                          0,
                                          &ulBuffSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
    }

    if (NT_SUCCESS( Status )) {
        Status = NtReadFile( hUpdate,
                             NULL,
                             NULL,
                             NULL,
                             &Iosb,
                             pBuff,
                             StandardInfo.EndOfFile.LowPart,
                             NULL,
                             NULL
                           );

        if ( Status == STATUS_PENDING ) {
            Status = NtWaitForSingleObject( hUpdate, FALSE, NULL );
        }

        if ( NT_SUCCESS(Status) ) {
            // Get final I/O status
            Status = Iosb.Status;
        }
    }

    // Look for this ini file in the list
    if (NT_SUCCESS(Status)) {

        pwch = (PWCHAR)pBuff;
        pBuffEnd = pBuff + StandardInfo.EndOfFile.LowPart;

        // Look for the file in the sorted list
        while ((pwch < (PWCHAR)pBuffEnd) &&
               ((lresult = _wcsicmp(pwch, pSysIniPath->Buffer)) < 0)) {
            pwch += wcslen(pwch) + sizeof(LARGE_INTEGER)/sizeof(WCHAR) + 1;
        }

        if ((pwch < (PWCHAR)pBuffEnd) && (lresult == 0)) {
            pwch += wcslen(pwch) + 1;
            pLastSyncTime->LowPart = ((PLARGE_INTEGER)pwch)->LowPart;
            pLastSyncTime->HighPart = ((PLARGE_INTEGER)pwch)->HighPart;
        }
    }

    if (NT_SUCCESS(Status) ) {
        // Get final I/O status
        Status = Iosb.Status;
    }

    if (pBuff) {
        NtFreeVirtualMemory( NtCurrentProcess(),
                             &pBuff,
                             &ulBuffSize,
                             MEM_RELEASE
                           );
    }

    if (hUpdate) {
        Status = NtClose( hUpdate );
    }
    return(Status);
}


/*****************************************************************************
 *
 *  TermsrvPutSyncTime
 *
 *  This routine will write the time of the system ini file that the user ini
 *  file was last sync'd with.
 *
 * ENTRY:
 *   PUNICODE_STRING pSysIniPath (In) - NT fully qualified system ini path
 *   PUNICODE_STRING pUserBasePath (In) - NT fully qualified user directory path
 *   PLARGE_INTEGER pLastSyncTime (OUT) - ptr to return last sync time
 *
 * EXIT:
 *   STATUS_SUCCESS - successfully stored the last sync time in infile.upd
 *
 ****************************************************************************/

NTSTATUS
TermsrvPutSyncTime(
    PUNICODE_STRING pSysIniPath,
    PUNICODE_STRING pUserBasePath,
    PLARGE_INTEGER  pLastSyncTime)
{
    NTSTATUS Status;
    HANDLE   hUpdate = NULL;
    OBJECT_ATTRIBUTES ObjAUpd;
    IO_STATUS_BLOCK   Iosb;
    FILE_STANDARD_INFORMATION StandardInfo;
    WCHAR             wcUpdateFile[MAX_PATH+1];
    UNICODE_STRING    UniUpdateName = {0,
                                       sizeof(wcUpdateFile),
                                       wcUpdateFile};
    PCHAR             pBuff = NULL, pBuffEnd;
    PWCH              pwch;
    SIZE_T            ulBuffSize;
    ULONG             ulLength;
    SIZE_T            ulRegionSize;
    LONG              lresult;
    LARGE_INTEGER     FileLength;
    FILE_POSITION_INFORMATION CurrentPos;

    wcscpy(wcUpdateFile, pUserBasePath->Buffer);
    wcscat(wcUpdateFile, L"\\inifile.upd");
    UniUpdateName.Length = wcslen(wcUpdateFile)*sizeof(WCHAR);

    InitializeObjectAttributes( &ObjAUpd,
                                &UniUpdateName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    // Open the update log
    Iosb.Status = STATUS_SUCCESS;
    Status = NtCreateFile( &hUpdate,
                             FILE_READ_DATA | FILE_WRITE_DATA |
                               FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                             &ObjAUpd,
                             &Iosb,
                           NULL,                  // Allocation size
                           FILE_ATTRIBUTE_NORMAL, // dwFlagsAndAttributes
                             FILE_SHARE_WRITE,      // dwShareMode
                           FILE_OPEN_IF,          // CreateDisposition
                             FILE_SYNCHRONOUS_IO_NONALERT |
                               FILE_NON_DIRECTORY_FILE, // CreateFlags
                           NULL, // EaBuffer
                           0     // EaLength
                           );

    if (NT_SUCCESS( Status )) {
        Status = NtQueryInformationFile( hUpdate,
                                         &Iosb,
                                         &StandardInfo,
                                         sizeof(StandardInfo),
                                         FileStandardInformation
                                       );
        if (Status == STATUS_BUFFER_OVERFLOW) {
            Status = STATUS_SUCCESS;
        }
#if DBG
        else if (!NT_SUCCESS( Status )) {
            DbgPrint( "TermsrvGetLastSyncTime: Unable to QueryInformation for %wZ - Status == %x\n", &UniUpdateName, Status );
        }
#endif
    }

    if (NT_SUCCESS( Status )) {
        ulBuffSize = StandardInfo.EndOfFile.LowPart + 4 * sizeof(WCHAR);
        ulRegionSize = ulBuffSize + 0x1000; // Room for 4K of growth
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &pBuff,
                                          0,
                                          &ulRegionSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE
                                        );
    }

    if (NT_SUCCESS( Status )) {
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &pBuff,
                                          0,
                                          &ulBuffSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
    }

    if (NT_SUCCESS( Status ) && StandardInfo.EndOfFile.LowPart) {
        Status = NtReadFile( hUpdate,
                             NULL,
                             NULL,
                             NULL,
                             &Iosb,
                             pBuff,
                             StandardInfo.EndOfFile.LowPart,
                             NULL,
                             NULL
                           );

        if ( Status == STATUS_PENDING ) {
            Status = NtWaitForSingleObject( hUpdate, FALSE, NULL );
        }

        if ( NT_SUCCESS(Status) ) {
            // Get final I/O status
            Status = Iosb.Status;
        }
    }

    // Look for this ini file in the list
    if (NT_SUCCESS(Status)) {

        pwch = (PWCHAR)pBuff;
        pBuffEnd = pBuff + StandardInfo.EndOfFile.LowPart;

        // Look for the file in the list
        while ((pwch < (PWCHAR)pBuffEnd) &&
               ((lresult = _wcsicmp(pwch, pSysIniPath->Buffer)) < 0)) {
            pwch += wcslen(pwch) + (sizeof(LARGE_INTEGER)/sizeof(WCHAR)) + 1;
        }

        // If the ini file is already in the file, just update the time
        if ((pwch < (PWCHAR)pBuffEnd) && (lresult == 0)) {
            pwch += wcslen(pwch) + 1;
            ((PLARGE_INTEGER)pwch)->LowPart = pLastSyncTime->LowPart;
            ((PLARGE_INTEGER)pwch)->HighPart = pLastSyncTime->HighPart;

        } else {                    // Ini file not in list

            // Figure out the size to grow the file
            ulLength = (pSysIniPath->Length + 2) + sizeof(LARGE_INTEGER);
            ulBuffSize += ulLength;

            // Grow the memory region
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &pBuff,
                                              0,
                                              &ulBuffSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );

            if (NT_SUCCESS(Status)) {
                // figure out where the entry goes in the file
                if (pwch < (PWCHAR)pBuffEnd) {
                    RtlMoveMemory( pwch+(ulLength/sizeof(WCHAR)),
                                   pwch,
                                   pBuffEnd - (PCHAR)pwch
                                 );
                }

                pBuffEnd += ulLength;
                wcscpy(pwch, pSysIniPath->Buffer);
                pwch += (pSysIniPath->Length + 2)/sizeof(WCHAR);
                ((PLARGE_INTEGER)pwch)->LowPart = pLastSyncTime->LowPart;
                ((PLARGE_INTEGER)pwch)->HighPart = pLastSyncTime->HighPart;
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        CurrentPos.CurrentByteOffset.LowPart = 0;
        CurrentPos.CurrentByteOffset.HighPart = 0;
        Status = NtSetInformationFile( hUpdate,
                                       &Iosb,
                                       &CurrentPos,
                                       sizeof(CurrentPos),
                                       FilePositionInformation
                                     );

        Status = NtWriteFile( hUpdate,
                              NULL,
                              NULL,
                              NULL,
                              &Iosb,
                              pBuff,
                              (ULONG)(pBuffEnd - pBuff + 1),
                              NULL,
                              NULL
                            );

        if( Status == STATUS_PENDING ) {
            Status = NtWaitForSingleObject( hUpdate, FALSE, NULL );
        }

        if( NT_SUCCESS(Status) ) {
            // Get final I/O status
            Status = Iosb.Status;
        }
    }

    if (NT_SUCCESS( Status )) {
        FileLength.LowPart = (ULONG)(pBuffEnd - pBuff);
        FileLength.HighPart = 0;
        Status = NtSetInformationFile( hUpdate,
                                       &Iosb,
                                       &FileLength,
                                       sizeof( FileLength ),
                                       FileEndOfFileInformation
                                     );
    }

    if (pBuff) {
        NtFreeVirtualMemory( NtCurrentProcess(),
                             &pBuff,
                             &ulRegionSize,
                             MEM_RELEASE
                           );
    }

    if (hUpdate) {
        Status = NtClose( hUpdate );
    }

    return(Status);
}


/*****************************************************************************
 *
 *  TermsrvCheckIniSync
 *
 *  This routine will get the time of the system ini file that the user ini
 *  file was last sync'd with.
 *
 * ENTRY:
 *   PUNICODE_STRING pSysIniPath (In) - NT fully qualified system ini path
 *   PUNICODE_STRING pUserBasePath (In) - NT fully qualified user directory path
 *   BOOLEAN fGet (In) - TRUE means to get last sync time, FALSE means to write it
 *   PLARGE_INTEGER pLastSyncTime (OUT) - ptr to return last sync time
 *
 * EXIT:
 *   TRUE  - User ini file should be sync'd
 *   FALSE - User ini file should be sync'd
 *
 ****************************************************************************/

BOOLEAN
TermsrvCheckIniSync(
    PUNICODE_STRING pSysIniPath,
    PUNICODE_STRING pUserBasePath)
{
    LARGE_INTEGER          LastSyncTime;
    OBJECT_ATTRIBUTES      objaIni;
    FILE_NETWORK_OPEN_INFORMATION BasicInfo;
    NTSTATUS               Status;

    // Get the last sync time of the ini file from the inifile.upd file
    TermsrvGetSyncTime(pSysIniPath, pUserBasePath, &LastSyncTime);

    // Get the last write time of the system ini file
    InitializeObjectAttributes(
        &objaIni,
        pSysIniPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    // Now query it
    Status = NtQueryFullAttributesFile( &objaIni, &BasicInfo );

    // If we couldn't get the time or the system ini file has been updated
    // since we last sync'd, return TRUE
    if (!NT_SUCCESS(Status) ||
        ((BasicInfo.LastWriteTime.HighPart > LastSyncTime.HighPart) ||
         ((BasicInfo.LastWriteTime.HighPart == LastSyncTime.HighPart) &&
         (BasicInfo.LastWriteTime.LowPart > LastSyncTime.LowPart)))) {
        return(TRUE);
    }
    return(FALSE);
}
/*****************************************************************************
 *
 *  TermsrvDoesFileExist
 *
 *   Returns whether the file exists or not.
 *
 *   Must use NT, not WIN32 pathnames.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
TermsrvDoesFileExist(
    PUNICODE_STRING pFileName
    )
{
    NTSTATUS Status;
    FILE_BASIC_INFORMATION BasicInfo;
    OBJECT_ATTRIBUTES Obja;

    InitializeObjectAttributes(
        &Obja,
        pFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    /*
     * Now query it
     */
    Status = NtQueryAttributesFile( &Obja, &BasicInfo );

    if( NT_SUCCESS( Status ) ) {
        return( TRUE );
    }

    return( FALSE );
}



/*****************************************************************************
 *
 *  TermsrvSyncUserIniFile
 *
 *   This routine will check that the user's ini file is "sync'd" with the
 *   system version of the ini file.  This means that it walks through the
 *   system ini file and checks that there is a corresponding entry in the
 *   user's ini file.
 *
 * ENTRY:
 *   IN PINIFILE_PARAMETERS a - ptr to inifile structure
 *
 * EXIT:
 *   True  - Ini file updated
 *   False - User Ini file was unchanged
 *
 ****************************************************************************/
BOOL TermsrvSyncUserIniFile(PINIFILE_PARAMETERS a)
{
    WCHAR          wcIniPath[MAX_PATH+1];
    UNICODE_STRING IniFilePath = {MAX_PATH,
                                  MAX_PATH+1,
                                  wcIniPath};
    PWCH           pwch, pwcIniName;
    UNICODE_STRING UniSysPath;
    UNICODE_STRING UserBasePath;
    NTSTATUS       Status;
    HANDLE         SrcHandle;
    ULONG          ulCompatFlags;
    OBJECT_ATTRIBUTES SrcObja;
    IO_STATUS_BLOCK   SrcIosb;
    INIFILE_OPERATION OrigOperation;
    BOOLEAN           OrigWrite, OrigMultiValue, OrigUnicode,
                      OrigWriteOperation, fIniUpdated = FALSE;
    ANSI_STRING       OrigAppName, OrigVarName;
    ULONG             OrigResultChars, OrigResultMaxChars;
    LPSTR             OrigResultBuffer;
    OBJECT_ATTRIBUTES      objaIni;
    FILE_NETWORK_OPEN_INFORMATION BasicInfo;

    // If INI file mapping is not on, return
    if (IsSystemLUID() || TermsrvAppInstallMode()) {
        return(FALSE);
    }

    // Build full system path to the Ini file, and get BasePath to user dir
    if ((gpTermsrvBuildSysIniPath == NULL) || !(gpTermsrvBuildSysIniPath(&a->NtFileName, &UniSysPath, &UserBasePath))) {
        #if DBG
        //DbgPrint("TermsrvSyncUserIniFile: Error building Sys Ini Path!\n");
        #endif
        return(FALSE);
    }

    // Get the ini file name
    pwch = wcsrchr(a->NtFileName.Buffer, L'\\') + 1;
    pwcIniName = RtlAllocateHeap( RtlProcessHeap(),
                                  0,
                                  (wcslen(pwch) + 1)*sizeof(WCHAR));
    wcscpy(pwcIniName, pwch);
    pwch = wcsrchr(pwcIniName, L'.');
    if (pwch) {
        *pwch = L'\0';
    }

    if (gpGetTermsrCompatFlags) {
        gpGetTermsrCompatFlags(pwcIniName, &ulCompatFlags, CompatibilityIniFile);
    } else {
        return FALSE;
    }

    // If the INISYNC compatibility flag is set in the registry and the
    // system version of the ini file exists, sync up the user version
    if (((ulCompatFlags & (TERMSRV_COMPAT_INISYNC | TERMSRV_COMPAT_WIN16)) ==
         (TERMSRV_COMPAT_INISYNC | TERMSRV_COMPAT_WIN16)) &&
        TermsrvDoesFileExist(&UniSysPath) &&
        TermsrvCheckIniSync(&UniSysPath, &UserBasePath)) {

        // Create a backup copy of the original file (inifile.ctx)
        wcscpy(wcIniPath, UserBasePath.Buffer);
        if (UserBasePath.Buffer[UserBasePath.Length/sizeof(WCHAR) - 1] != L'\\')
            wcscat(wcIniPath, L"\\");
        wcscat(wcIniPath, pwcIniName);
        wcscat(wcIniPath, L".ctx");
        IniFilePath.Length = wcslen(wcIniPath)*sizeof(WCHAR);

        if (gpTermsrvCopyIniFile) {
            Status = gpTermsrvCopyIniFile(&a->NtFileName, NULL, &IniFilePath);
    #if DBG
            if (!NT_SUCCESS(Status)) {
                DbgPrint("TermsrvSyncUserIniFile: Error 0x%x creating backup ini file %ws\n",
                         Status,
                         wcIniPath);
            }
    #endif
        } else {
            return FALSE;
            }

        // Check that each entry in the system version is in the user's version
        InitializeObjectAttributes(&SrcObja,
                                   &UniSysPath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        // Open the src
        SrcIosb.Status = STATUS_SUCCESS;
        Status = NtOpenFile(&SrcHandle,
                             FILE_GENERIC_READ,
                            &SrcObja,
                            &SrcIosb,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT);

        if( NT_SUCCESS(Status) ) {
            // Get final I/O status
                  Status = SrcIosb.Status;
        }

        if( !NT_SUCCESS(Status) ) {
#if DBG
            DbgPrint("TermsrvSyncUserIniFile: Error 0x%x opening SrcFile %ws\n",
                     Status,
                     &UniSysPath.Buffer);
#endif
            goto Cleanup;
        }

        // Save the original values
        OrigOperation = a->Operation;
        OrigMultiValue = a->MultiValueStrings;
        OrigAppName = a->ApplicationName;
        OrigVarName = a->VariableName;
        OrigResultChars = a->ResultChars;
        OrigResultMaxChars = a->ResultMaxChars;
        OrigResultBuffer = a->ResultBuffer;
        OrigUnicode = a->Unicode;
        OrigWriteOperation = a->WriteOperation;

        // Set up the open for writes
        a->WriteOperation = TRUE;
        a->Operation = WriteKeyValue;
        a->MultiValueStrings = FALSE;
        a->Unicode = FALSE;

        Status = BaseDllOpenIniFileOnDisk( a );

        if( !NT_SUCCESS(Status) ) {
#if DBG
            DbgPrint("TermsrvSyncUserIniFile: Error 0x%x opening DestFile %ws\n",
                     Status,
                     &a->NtFileName.Buffer);
#endif
            NtClose( SrcHandle );
                goto Cleanup;
        }

        // set the data up for writing
        a->TextEnd = (PCHAR)a->IniFile->BaseAddress +
                            a->IniFile->EndOfFile;
        a->TextCurrent = a->IniFile->BaseAddress;

        // Make sure entries in system ini file are in user ini file
        Status = TermsrvIniSyncLoop( SrcHandle, a, &fIniUpdated );
#if DBG
        if( !NT_SUCCESS(Status) ) {
            DbgPrint("TermsrvSyncUserIniFile: Error 0x%x Doing sync loop\n",Status);
        }
#endif

        // Close the file handles
        NtClose( SrcHandle );
        BaseDllCloseIniFileOnDisk( a );

        // Restore the variables in the ini file structure
        a->Operation = OrigOperation;
        a->MultiValueStrings = OrigMultiValue;
        a->ApplicationName = OrigAppName;
        a->VariableName = OrigVarName;
        a->ResultChars = OrigResultChars;
        a->ResultMaxChars = OrigResultMaxChars;
        a->ResultBuffer = OrigResultBuffer;
        a->WriteOperation = FALSE;
        a->Unicode = OrigUnicode;
        a->WriteOperation = OrigWriteOperation;

        // Get the last write time of the system ini file
        InitializeObjectAttributes( &objaIni,
                                    &UniSysPath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                  );

        // Now query it
        Status = NtQueryFullAttributesFile( &objaIni, &BasicInfo );

        // Update the sync time in the inisync file
        if (NT_SUCCESS(Status)) {
            TermsrvPutSyncTime( &UniSysPath,
                            &UserBasePath,
                            &BasicInfo.LastWriteTime
                          );
        }
    }

Cleanup:
    // Free the unicode buffers
    RtlFreeHeap( RtlProcessHeap(), 0, UniSysPath.Buffer );
    RtlFreeHeap( RtlProcessHeap(), 0, UserBasePath.Buffer );
    RtlFreeHeap( RtlProcessHeap(), 0, pwcIniName);

    return(fIniUpdated);
}


/*****************************************************************************
 *
 *  TermsrvIniSyncLoop
 *
 *  This routine will verify that there's a corresponding entry in the user's
 *  ini file for each entry in the system ini file.
 *
 * ENTRY:
 *   HANDLE SrcHandle (INPUT)  - Handle to system ini file
 *   PINIFILE_PARAMETERS a (INPUT) - pointer to current ini file structure
 *   PBOOLEAN pfIniUpdated (OUTPUT) - Returns TRUE if user ini file is modified
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
TermsrvIniSyncLoop(HANDLE SrcHandle,
               PINIFILE_PARAMETERS a,
               PBOOLEAN pfIniUpdated)
{
    PCHAR pStr;
    NTSTATUS Status;
    ULONG StringSize;
    CHAR  IOBuf[512];
    ULONG IOBufSize = 512;
    ULONG IOBufIndex = 0;
    ULONG IOBufFillSize = 0;
    ANSI_STRING AnsiSection;
    PCH pch;
    PVOID pSection, origbase;

    AnsiSection.Buffer = NULL;
    *pfIniUpdated = FALSE;

    while( 1 ) {

        pStr = NULL;
        StringSize = 0;

        if (gpTermsrvGetString == NULL) {
            return STATUS_UNSUCCESSFUL;
        }

        // Get a string from the source ini file
        Status = gpTermsrvGetString(SrcHandle,
                               &pStr,
                               &StringSize,
                               IOBuf,
                               IOBufSize,
                              &IOBufIndex,
                               &IOBufFillSize);

        if( !NT_SUCCESS(Status) ) {

            ASSERT( pStr == NULL );

            if( Status == STATUS_END_OF_FILE ) {
                Status = STATUS_SUCCESS;
            }
            if (AnsiSection.Buffer) {
                RtlFreeHeap( RtlProcessHeap(), 0, AnsiSection.Buffer );
            }

            a->IniFile->UpdateEndOffset = a->IniFile->EndOfFile;
            return( Status );
        }

        // Make sure we got some actual data
        ASSERT( pStr != NULL );

        // Is this a section name?
        if (*pStr == '[') {
            if (AnsiSection.Buffer) {
                RtlFreeHeap( RtlProcessHeap(), 0, AnsiSection.Buffer );
                AnsiSection.Buffer = NULL;
            }
            pch = strrchr(pStr, ']');
            if (pch) {
                AnsiSection.MaximumLength = (USHORT)(pch - pStr);
                *pch = '\0';
            } else {
                AnsiSection.Length = strlen(pStr);
            }
            AnsiSection.Length = AnsiSection.MaximumLength - 1;
            AnsiSection.Buffer = RtlAllocateHeap(RtlProcessHeap(),
                                                 0,
                                                 AnsiSection.MaximumLength);
            strcpy(AnsiSection.Buffer, pStr+1);
            a->ApplicationName = AnsiSection;

            a->TextCurrent = a->IniFile->BaseAddress;   // reset file pointer

            // See if the section already exists, if so save the start of it
            Status = BaseDllFindSection( a );
            if (NT_SUCCESS(Status)) {
                pSection = a->TextCurrent;
            } else {
                pSection = NULL;
            }

        // If it's not a comment, see if the entry is in the user's ini file
        } else if (*pStr != ';') {

            pch = strchr(pStr, '=');
            if (pch) {
                a->VariableName.Length = a->VariableName.MaximumLength =
                    (USHORT)(pch - pStr);
                a->VariableName.Buffer = pStr;
                a->ValueBuffer = (++pch);
                a->ValueLength = 0;
                while (*pch && (*pch != 0xa) && (*pch != 0xd)) {
                    pch++;
                    a->ValueLength++;
                }


                // If the section exists, check for the keyword in user's ini
                if (pSection) {
                    a->TextCurrent = pSection;
                    Status = BaseDllFindKeyword( a );
                }

                // If variable isn't found, write it out
                if (!pSection || !NT_SUCCESS( Status )) {

                    origbase = a->TextCurrent = a->IniFile->BaseAddress;
                    Status = BaseDllWriteKeywordValue( a, NULL );
                    a->TextEnd = (PCHAR)a->IniFile->BaseAddress +
                                        a->IniFile->EndOfFile;
                    if (!NT_SUCCESS(Status)) {
                              #if DBG
                              DbgPrint("TermsrvIniSyncLoop: Error 0x%x write Key Value\n",
                                  Status);
                              #endif
                        a->IniFile->UpdateEndOffset = a->IniFile->EndOfFile;
                        RtlFreeHeap( RtlProcessHeap(), 0, pStr );
                        if (AnsiSection.Buffer) {
                            RtlFreeHeap(RtlProcessHeap(),
                                        0,
                                        AnsiSection.Buffer);
                        }
                        return(Status);
                    }
                    *pfIniUpdated = TRUE;
                    if (origbase != a->IniFile->BaseAddress) {
                        a->TextCurrent = a->IniFile->BaseAddress;
                        Status = BaseDllFindSection( a );
                        if (NT_SUCCESS(Status)) {
                            pSection = a->TextCurrent;
                        } else {
                            pSection = NULL;
                        }
                    }
                }
            }
        }


    } // end while(1)
}


