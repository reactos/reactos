/****************************** Module Header ******************************\
* Module Name: sysinit.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Winlogon main module
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "shutinit.h"

BOOLEAN PageFilePopup = FALSE;

TCHAR szMemMan[] =
     TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

TCHAR szNoPageFile[] = TEXT("TempPageFile");
WCHAR DontWatchSysProcs[] = L"DontWatchSysProcs" ;

HANDLE  hSystemProcesses[MAXIMUM_WAIT_OBJECTS];
DWORD   cSystemProcesses;
BOOL    IgnoreSystemProcessDeath = FALSE ;

#define USE_RTLTHREADS 1

VOID
SystemProcessDeath(
    PVOID Parameter,
    BOOLEAN Timeout
    );

typedef struct _SYSTEM_PROCESS_DEATH {
    PWSTR Process ;
    HANDLE Handle ;
    PVOID Notify ;
    BOOL Active ;
} SYSTEM_PROCESS_DEATH, * PSYSTEM_PROCESS_DEATH ;

#define DEBUG_COMMAND           TEXT("ntsd -d ")
#define DEBUG_COMMAND_NO_WAIT   TEXT("ntsd -d -g ")
#define SELECT_DEBUG_COMMAND(x) (x & DEB_DEBUG_NOWAIT ? DEBUG_COMMAND_NO_WAIT : DEBUG_COMMAND)



VOID
CreateNetworkProviderEvent(
    VOID
    )
{
    OBJECT_ATTRIBUTES   EventAttr;
    UNICODE_STRING      usName;
    NTSTATUS            Status;
    HANDLE hEvent ;
    UCHAR   Buffer[ 256 ];
    SECURITY_DESCRIPTOR sd ;
    PACL Acl ;

    Acl = (PACL) Buffer ;

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );
    InitializeAcl( Acl, sizeof( Buffer ), ACL_REVISION );
    AddAccessAllowedAce( Acl,
                        ACL_REVISION,
                        EVENT_ALL_ACCESS | STANDARD_RIGHTS_ALL,
                        g_WinlogonSid );

    AddAccessAllowedAce( Acl,
                         ACL_REVISION,
                         EVENT_QUERY_STATE | EVENT_MODIFY_STATE |
                            SYNCHRONIZE | READ_CONTROL,
                         gAdminSid );

    SetSecurityDescriptorDacl( &sd, TRUE, Acl, FALSE );

    RtlInitUnicodeString(&usName, L"\\Security\\NetworkProviderLoad" );

    InitializeObjectAttributes(&EventAttr,
                                   &usName,
                                   0,
                                   NULL,
                                   &sd );

    Status = NtCreateEvent( &hEvent,
                            EVENT_ALL_ACCESS,
                            &EventAttr,
                            SynchronizationEvent,
                            FALSE );

    if ( NT_SUCCESS( Status ) )
    {
        NetworkProviderEvent = hEvent ;
    }
}



//
// Look for autocheck logs, and log them
//

BOOL
GetConfiguredVolumes (
   LPWSTR* ppwszConfiguredVolumes,
   LPWSTR wszConfiguredVolumesBuffer,
   DWORD nConfiguredVolumesBuffer
   )
{
    DWORD  nResult;
    LPWSTR pwszConfiguredVolumes;

    nResult = GetLogicalDriveStringsW(
                 nConfiguredVolumesBuffer,
                 wszConfiguredVolumesBuffer
                 );

    if ( nResult == 0 )
    {
        return( FALSE );
    }
    else if ( nResult <= nConfiguredVolumesBuffer )
    {
        *ppwszConfiguredVolumes = wszConfiguredVolumesBuffer;
        return( TRUE );
    }

    pwszConfiguredVolumes = LocalAlloc(
                                 LPTR,
                                 ( nResult + 1 ) * sizeof( WCHAR )
                                 );

    if ( pwszConfiguredVolumes != NULL )
    {
        if ( GetLogicalDriveStringsW(
                nResult,
                pwszConfiguredVolumes
                ) <= nResult )
        {
            *ppwszConfiguredVolumes = pwszConfiguredVolumes;
            return( TRUE );
        }

        LocalFree( pwszConfiguredVolumes );
    }

    return( FALSE );
}

VOID
FreeConfiguredVolumes (
    LPWSTR pwszConfiguredVolumes,
    LPWSTR wszConfiguredVolumesBuffer
    )
{
    if ( pwszConfiguredVolumes != wszConfiguredVolumesBuffer )
    {
        LocalFree( pwszConfiguredVolumes );
    }
}

BOOL
RetryRegisterEventSource (
     DWORD MaxRetries,
     DWORD SleepBetweenRetries,
     LPWSTR pwszSourceName,
     HANDLE* phEventLog
     )
{
    BOOL   fResult = FALSE;
    DWORD  RetryCount = 0;
    HANDLE hEventLog = NULL;
    DWORD  LastError;

    while ( ( fResult == FALSE ) && ( RetryCount < MaxRetries ) )
    {
        hEventLog = RegisterEventSourceW( NULL, pwszSourceName );

        if ( hEventLog != NULL )
        {
            fResult = TRUE;
        }
        else
        {
            LastError = GetLastError();

            if ( ( LastError == RPC_S_SERVER_UNAVAILABLE ) ||
                 ( LastError = RPC_S_UNKNOWN_IF ) )
            {
                Sleep( SleepBetweenRetries );
                RetryCount += 1;
            }
            else
            {
                RetryCount = MaxRetries;
            }
        }
    }

    if ( fResult == TRUE )
    {
        *phEventLog = hEventLog;
    }

    return( fResult );
}

BOOL
TransferAutochkLogToEventLogIfAvailable (
        LPWSTR pwszAutochkFile
        )
{
    BOOL   fResult = TRUE;
    HANDLE hEventLog = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    ULONG  cbData;
    LPBYTE pbData = NULL;

    fResult = RetryRegisterEventSource( 10, 1000, L"Autochk", &hEventLog );

    if ( fResult == TRUE )
    {
        if ( ( hFile = CreateFileW(
                             pwszAutochkFile,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                             ) ) == INVALID_HANDLE_VALUE )
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        // Note: There is a 32K limit for the data
        cbData = 32 * 1024;
        pbData = LocalAlloc( LMEM_FIXED, cbData );
        if ( pbData != NULL )
        {
            fResult = ReadFile(
                          hFile,
                          pbData,
                          cbData - sizeof( WCHAR ),
                          &cbData,
                          NULL
                          );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        pbData[ cbData ] = 0;
        pbData[ cbData + 1 ] = 0;

        if ( ReportWinlogonEvent(
                   NULL,
                   EVENTLOG_INFORMATION_TYPE,
                   EVENT_AUTOCHK_DATA,
                   0,
                   NULL,
                   1,
                   (WCHAR *)pbData
                   ) != ERROR_SUCCESS )
        {
            fResult = FALSE;
        }
    }

    if ( pbData != NULL )
    {
        LocalFree( pbData );
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    if ( hEventLog != NULL )
    {
        DeregisterEventSource( hEventLog );
    }

    return( fResult );
}


VOID
DealWithAutochkLogs (
    VOID
    )
/*++

Routine Description:

    This routine enumerates the current volumes and attempts to process any AUTOCHK logs
    it finds.

Arguments:

    None.

Return Value:

    None

--*/
{
    WCHAR   pwszBootExFile[MAX_PATH+20];
    WCHAR   pwszVolumeName[MAX_PATH+1];
    HANDLE  h;

    //
    //  Begin enumerating volumes
    //

    h = FindFirstVolume(pwszVolumeName, MAX_PATH+1);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }

    
    //
    //  For each volume we find, process the log file in the root
    //
    
    do {
        if ( GetDriveTypeW( pwszVolumeName ) == DRIVE_FIXED ) {
            wcscpy( pwszBootExFile, pwszVolumeName );
            wcscat( pwszBootExFile, L"bootex.log" );

            if ( TransferAutochkLogToEventLogIfAvailable( pwszBootExFile )) {
                DeleteFileW( pwszBootExFile );
            }
        }
        
    } while ( FindNextVolume( h, pwszVolumeName, MAX_PATH + 1 ));

    FindVolumeClose( h );
    
}

DWORD FontLoaderThread( void  )
{
    LoadLocalFonts();
    ExitThread(0);
    return(0);      // prevent compiler warning
}

HANDLE
StartLoadingFonts(void)
{
    HANDLE  hThread;
    DWORD   ThreadId = 0;

    hThread = CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) FontLoaderThread,
                            0,
                            0,
                            &ThreadId
                          );

    //
    // We don't need this handle (we're not going to wait), so get rid of
    // it now, rather than later.
    //

    return( hThread );
}


BOOL InitSystemFontInfo(void)
{
    TCHAR *FontNames, *FontName;
    TCHAR FontPath[ MAX_PATH ];
    ULONG cb = 63 * 1024;


    FontNames = Alloc( cb );
    ASSERTMSG("Winlogon failed to allocate memory for reading font information", FontNames != NULL);
    if (FontNames == NULL) {
        return FALSE;
    }

    if (GetProfileString( TEXT("Fonts"), NULL, TEXT(""), FontNames, cb )) {
        FontName = FontNames;
        while (*FontName) {
            if (GetProfileString( TEXT("Fonts"), FontName, TEXT(""), FontPath, sizeof( FontPath ) )) {
                switch (AddFontResource( FontPath )) {
                case 0:
                    KdPrint(("WINLOGON: Unable to add new font path: %ws\n", FontPath ));
                    break;

                case 1:
                    KdPrint(("WINLOGON: Found new font path: %ws\n", FontPath ));
                    break;

                default:
                    KdPrint(("WINLOGON: Found existing font path: %ws\n", FontPath ));
                    RemoveFontResource( FontPath );
                    break;
                }
            }
            while (*FontName++) ;
        }
    } else {
        KdPrint(("WINLOGON: Unable to read font info from win.ini - %u\n", GetLastError()));
    }

    Free( FontNames );
    return TRUE;
}


/***************************************************************************\
* SetProcessPriority
*
* Sets the priority of the winlogon process.
*
* History:
* 18-May-1992 Davidc       Created.
\***************************************************************************/
BOOL SetProcessPriority(
    VOID
    )
{
    //
    // Bump us up to the high priority class
    //

    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
        DebugLog((DEB_ERROR, "Failed to raise it's own process priority, error = %d", GetLastError()));
        return(FALSE);
    }

    //
    // Set this thread to high priority since we'll be handling all input
    //

    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
        DebugLog((DEB_ERROR, "Failed to raise main thread priority, error = %d", GetLastError()));
        return(FALSE);
    }

    return(TRUE);
}


VOID
CreateTemporaryPageFile()
{
    LONG FileSizeInMegabytes;
    UNICODE_STRING PagingFileName;
    NTSTATUS st;
    LARGE_INTEGER MinPagingFileSize;
    LARGE_INTEGER MaxPagingFileSize;
    UNICODE_STRING FileName;
    BOOLEAN TranslationStatus;
    TCHAR TemporaryPageFile[MAX_PATH+1];
    NTSTATUS PfiStatus,PiStatus;
    ULONG ReturnLength;
    SYSTEM_PAGEFILE_INFORMATION pfi;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    HKEY hkeyMM;
    DWORD dwRegData = 0;


    GetSystemDirectory(TemporaryPageFile,sizeof(TemporaryPageFile));
    wcscat(TemporaryPageFile,TEXT("\\temppf.sys"));
    DeleteFile(TemporaryPageFile);

    //
    // Check to see if we have a pagefile, warn the user if we don't
    //

    PfiStatus = NtQuerySystemInformation(
                SystemPageFileInformation,
                &pfi,
                sizeof(pfi),
                &ReturnLength
                );

    PiStatus = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );
    //
    // if you have no page file, or your total commit limit is at it's minimum,
    // then create an additional pagefile and tel the user to do something...
    //

    if ( (NT_SUCCESS(PfiStatus) && (ReturnLength == 0)) ||
         (NT_SUCCESS(PiStatus) && PerfInfo.CommitLimit <= 5500 ) ) {

        //
        // Set a flag in registry so USERINIT knows to run VMApp.
        //
        dwRegData = 1;

        PageFilePopup = TRUE;

        //
        // create a temporary pagefile to get us through logon/control
        // panel activation
        //
        //

        GetSystemDirectory(TemporaryPageFile,sizeof(TemporaryPageFile));
        lstrcat(TemporaryPageFile,TEXT("\\temppf.sys"));


        //
        // Start with a 20mb pagefile
        //

        FileSizeInMegabytes = 20;

        RtlInitUnicodeString(&PagingFileName, TemporaryPageFile);

        MinPagingFileSize = RtlEnlargedIntegerMultiply(FileSizeInMegabytes,0x100000);
        MaxPagingFileSize = MinPagingFileSize;


        TranslationStatus = RtlDosPathNameToNtPathName_U(
                                PagingFileName.Buffer,
                                &FileName,
                                NULL,
                                NULL
                                );

        if ( TranslationStatus ) {

retry:
            st = NtCreatePagingFile(
                    (PUNICODE_STRING)&FileName,
                    &MinPagingFileSize,
                    &MaxPagingFileSize,
                    0
                    );

            if (!NT_SUCCESS( st )) {

                if ( FileSizeInMegabytes > 0 ) {
                    FileSizeInMegabytes -= 2;
                    MinPagingFileSize = RtlEnlargedIntegerMultiply(FileSizeInMegabytes,0x100000);
                    MaxPagingFileSize = MinPagingFileSize;
                    goto retry;
                }
            } else {
                MoveFileExW(PagingFileName.Buffer,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);

            }

            RtlFreeHeap(RtlProcessHeap(), 0, FileName.Buffer);

        }
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMemMan, 0,
            KEY_WRITE, &hkeyMM) == ERROR_SUCCESS) {
        if (dwRegData == 1) {
            RegSetValueEx (hkeyMM, szNoPageFile, 0, REG_DWORD,
                    (LPBYTE)&dwRegData, sizeof(dwRegData));
        } else
            RegDeleteValue(hkeyMM, (LPTSTR)szNoPageFile);
        RegCloseKey(hkeyMM);
    }
}


BOOL
StartSystemProcess(
    PWSTR   pszCommandLine,
    PWSTR   pszDesktop,
    DWORD   Flags,
    DWORD   StartupFlags,
    PVOID   pEnvironment,
    DWORD   WlFlags,
    HANDLE *phProcess,
    HANDLE *phThread
    )
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL Result;
    PSYSTEM_PROCESS_DEATH Death ;
    NTSTATUS Status ;
#if DBG
    WCHAR   szExtra[MAX_PATH];
#endif

    //
    // Initialize process startup info
    //
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = pszCommandLine;
    si.lpTitle = pszCommandLine;
    si.dwFlags = StartupFlags;
    si.wShowWindow = SW_SHOW;   // at least let the guy see it
    si.lpDesktop = pszDesktop;

    //
    // Special debug helpers for our friends
    //
#if DBG
    if ((WinlogonInfoLevel & DEB_DEBUG_LSA) &&
        ((wcsncmp(pszCommandLine, TEXT("lsass"), 5) == 0) ||
         (wcsncmp(pszCommandLine, TEXT("spmgr"), 5) == 0) ))
    {
        wcscpy(szExtra, SELECT_DEBUG_COMMAND(WinlogonInfoLevel));
        wcscat(szExtra, pszCommandLine);
        pszCommandLine = szExtra;
    }
    if ((WinlogonInfoLevel & DEB_DEBUG_MPR) &&
        (wcsncmp(pszCommandLine, TEXT("mpnotify"), 8) == 0))
    {
        wcscpy(szExtra, SELECT_DEBUG_COMMAND(WinlogonInfoLevel));
        wcscat(szExtra, pszCommandLine);
        pszCommandLine = szExtra;
    }
    if ((WinlogonInfoLevel & DEB_DEBUG_SERVICES) &&
        (wcsncmp(pszCommandLine, TEXT("services"), 8) == 0))
    {
        wcscpy(szExtra, SELECT_DEBUG_COMMAND(WinlogonInfoLevel));
        wcscat(szExtra, pszCommandLine);
        pszCommandLine = szExtra;
    }
#endif


    //
    // Create the app suspended
    //
    Result = CreateProcess(NULL,
                      pszCommandLine,
                      NULL,
                      NULL,
                      FALSE,
                      Flags | CREATE_UNICODE_ENVIRONMENT,
                      pEnvironment,
                      NULL,
                      &si,
                      &pi);


    if (Result)
    {
        if (!phProcess)
        {
            if ( WlFlags )
            {
                if ( WlFlags & START_SAVE_HANDLE )
                {
                    if (cSystemProcesses < MAXIMUM_WAIT_OBJECTS)
                    {
                        hSystemProcesses[cSystemProcesses++] = pi.hProcess;
                    }
                }

                if ( WlFlags & START_SYSTEM_SERVICE )
                {
                    Death = LocalAlloc( LMEM_FIXED,
                                        sizeof( SYSTEM_PROCESS_DEATH ) );

                    if ( Death )
                    {
                        Death->Process = AllocAndDuplicateString( pszCommandLine );

                        Death->Handle = pi.hProcess ;

                        Status = RtlRegisterWait(
                                    &Death->Notify,
                                    pi.hProcess,
                                    SystemProcessDeath,
                                    Death,
                                    INFINITE,
                                    WT_EXECUTEONLYONCE );

                        if ( !NT_SUCCESS( Status ) )
                        {
                            LocalFree( Death );
                        }

                    }

                }
            }
            else
            {
                CloseHandle(pi.hProcess);
            }
        }
        else
        {
            *phProcess = pi.hProcess;
        }
        if (!phThread)
        {
            CloseHandle(pi.hThread);
        }
        else
        {
            *phThread = pi.hThread;
        }
    }

    return(Result);
}



BOOL
ExecSystemProcesses(
    VOID
    )
{
    BOOL SystemStarted = FALSE ;
    SYSTEM_CRASH_STATE_INFORMATION CrashState;
    PWSTR   pszSystemApp;
    PWSTR   pszStartLine = NULL ;
    PWSTR   pszTok;
    DWORD   dwStarted = 0;
    PVOID   pEnvironment;
    HKEY hKey ;
    DWORD dwType ;
    DWORD dwSize ;
    DWORD dwValue ;
    int err;
    LPWSTR  ServiceName;
    NTSTATUS            Status;

    //
    //  Initialize the shutdown server
    //

    RpcpInitRpcServer();
    if ( !InitializeShutdownModule() ) {
        ASSERT( FALSE );
        DebugLog((DEB_ERROR, "Cannot InitializeShutdownModule."));
    }


    if (CreateUserEnvironment(&pEnvironment))
    {
        SetupBasicEnvironment(&pEnvironment);
    }
    else
    {
        DebugLog((DEB_ERROR, "Failed to create initial environment\n"));

        //
        // Set this to NULL, and let CreateProcess deal with any
        // memory constraints.
        //
        pEnvironment = NULL;
    }

    if ( !g_Console )
    {
        return TRUE ;
    }

    //
    // start the InitShutdown RPC server
    //
    ServiceName = SHUTDOWN_INTERFACE_NAME;
    Status = RpcpStartRpcServer(
                ServiceName,
                InitShutdown_ServerIfHandle
                );
    ASSERT( NT_SUCCESS( Status ));
    if( ! NT_SUCCESS( Status )) {
        DebugLog((DEB_ERROR, "Cannot start InitShutdown server."));
    }


    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        WINLOGON_KEY,
                        0,
                        KEY_READ,
                        &hKey );

    if ( err == 0 )
    {
        dwSize = sizeof( DWORD );

        err = RegQueryValueEx( hKey,
                                DontWatchSysProcs,
                                NULL,
                                &dwType,
                                (LPBYTE) &dwValue,
                                &dwSize );

        if ( err == 0 )
        {
            if ( dwType == REG_DWORD )
            {
                IgnoreSystemProcessDeath = dwValue ;
            }
        }

        RegCloseKey( hKey );
    }

    //
    // must start services.exe server before anything else.  If there is an
    // entry ServiceControllerStart in win.ini, use it as the command.
    //
    pszSystemApp = AllocAndGetProfileString(APPLICATION_NAME,
                                            SERVICE_CONTROLLER_START,
                                            TEXT("%SystemRoot%\\system32\\services.exe"));

    if ( pszSystemApp )
    {
        pszStartLine = AllocAndExpandEnvironmentStrings( pszSystemApp );

        Free( pszSystemApp );
    }
    else
    {
        pszStartLine = NULL ;
    }

    if (!pszStartLine)
    {
        return FALSE ;
    }


    if ( !StartSystemProcess(pszStartLine,
                            L"",
                            0,
                            STARTF_FORCEOFFFEEDBACK,
                            pEnvironment,
                            START_SYSTEM_SERVICE,
                            NULL, NULL))
    {
        DebugLog((DEB_ERROR, "Couldn't start %ws, %d\n", pszStartLine, GetLastError()));
    }

    Free(pszStartLine);

    //
    // If this is standard installation or network installation, we need to
    // create an event to stall lsa security initialization.  In the case of
    // WINNT -> WINNT and AS -> AS upgrade we shouldn't stall LSA.
    //

    if (g_fExecuteSetup && (g_uSetupType != SETUPTYPE_UPGRADE)) {
        CreateLsaStallEvent();
    }

    //
    // If there is a system dump available, start up the save dump process to
    // capture it so that it doesn't use as much paging file so that it is
    // available for system use.
    //

    NtQuerySystemInformation( SystemCrashDumpStateInformation,
                              &CrashState,
                              sizeof( CrashState ),
                              (PULONG) NULL );
    if (CrashState.ValidCrashDump) {
        pszSystemApp = AllocAndGetProfileString(APPLICATION_NAME,
                                                TEXT("SaveDumpStart"),
                                                TEXT("%SystemRoot%\\system32\\savedump.exe"));


        if ( pszSystemApp )
        {
            pszStartLine = AllocAndExpandEnvironmentStrings( pszSystemApp );

            Free( pszSystemApp );
        }
        else
        {
            pszStartLine = NULL ;
        }


        if (!StartSystemProcess(pszStartLine,
                                L"",
                                0,
                                STARTF_FORCEOFFFEEDBACK,
                                pEnvironment,
                                0,
                                NULL, NULL))
        {
            DebugLog((DEB_ERROR, "Couldn't start %ws, %d\n", pszStartLine, GetLastError()));
        }
        Free(pszStartLine);
    }

    pszSystemApp = AllocAndGetProfileString(APPLICATION_NAME,
                                            LSASS_START,
                                            TEXT("%SystemRoot%\\system32\\lsass.exe"));

    if ( pszSystemApp )
    {
        pszStartLine = AllocAndExpandEnvironmentStrings( pszSystemApp );

        Free( pszSystemApp );
    }
    else {
        pszStartLine = NULL ;
    }

    if (!pszStartLine)
    {
        return FALSE ;
    }


    if ( !StartSystemProcess(pszStartLine,
                            L"",
                            0,
                            STARTF_FORCEOFFFEEDBACK,
                            pEnvironment,
                            START_SYSTEM_SERVICE,
                            NULL, NULL))
    {
        DebugLog((DEB_ERROR, "Couldn't start %ws, %d\n", pszStartLine, GetLastError()));
    }

    Free(pszStartLine);


    //
    // Startup system processes
    // These must be started for authentication initialization to succeed
    // because one of the system processes is the LSA server.
    //


    pszStartLine = AllocAndGetProfileString(APPLICATION_NAME,
                                            TEXT("System"),
                                            NULL);

    if ( pszStartLine )
    {
        pszTok = wcstok(pszStartLine, TEXT(","));
        while (pszTok)
        {
            //
            // Skip any blanks...
            //
            if (*pszTok == TEXT(' '))
            {
                while (*pszTok++ == TEXT(' '))
                    ;
            }

            if ( _wcsicmp( pszTok, TEXT("lsass.exe") ) )
            {
                if (StartSystemProcess( pszTok,
                                        L"",
                                        0,
                                        STARTF_FORCEOFFFEEDBACK,
                                        pEnvironment,
                                        0,
                                        NULL, NULL))
                {
                    dwStarted++;
                }
            }

            pszTok = wcstok(NULL, TEXT(","));

        }

        Free(pszStartLine);
    }


    RtlDestroyEnvironment(pEnvironment);

    return TRUE;
}



BOOL
WaitForSystemProcesses(
    VOID)
{
    DWORD   i;
    DWORD   Exit;

    //
    // First, verify all handles:
    //

    for (i = 0; i < cSystemProcesses ; i++ )
    {

WaitLoopTop:

        if (GetExitCodeProcess(hSystemProcesses[i], &Exit))
        {
            if (Exit == STILL_ACTIVE)
            {
                //
                // Ooh, a good one.  Keep it.
                //

                continue;
            }

        }

        //
        // Bad handle, one way or another
        //

        CloseHandle(hSystemProcesses[i]);
        hSystemProcesses[i] = hSystemProcesses[--cSystemProcesses];

        if (i != cSystemProcesses)
        {
            goto WaitLoopTop;   // Retry same index, but do not increment
        }

    }

    if (!cSystemProcesses)
    {
        return(TRUE);
    }

    Exit = WaitForMultipleObjectsEx(    cSystemProcesses,
                                        hSystemProcesses,
                                        FALSE,
                                        4000,
                                        FALSE );


    return(Exit != WAIT_TIMEOUT);
}

DWORD
SystemProcessDeathWorker(
    PSYSTEM_PROCESS_DEATH Death
    )
{
    WCHAR Template[ MAX_PATH ];
    WCHAR Message[ MAX_PATH ];
    UNICODE_STRING String ;
    DWORD ExitCode ;


    DebugLog(( DEB_ERROR, "System Process %ws has terminated prematurely\n",
                Death->Process ));

    if ( IgnoreSystemProcessDeath || g_fExecuteSetup )
    {
        return 0 ;
    }

    GetExitCodeProcess( Death->Handle, &ExitCode );

    if ( !LoadString( NULL, IDS_SYSTEM_PROCESS_DIED, Template, MAX_PATH ) )
    {
        wcscpy( Message, TEXT("System shutting down.") );
    }
    else
    {
        wsprintf( Message, Template, Death->Process, ExitCode );
    }

    RtlInitUnicodeString( &String, Message );

    LocalInitiateSystemShutdown( &String, 60, TRUE, TRUE );

    return 0 ;

}

VOID
SystemProcessDeath(
    PVOID p,
    BOOLEAN Timeout
    )
{
    PSYSTEM_PROCESS_DEATH Death = (PSYSTEM_PROCESS_DEATH) p ;
    HANDLE h ;
    DWORD tid ;

    if ( Timeout )
    {
        return ;
    }

    if ( InterlockedExchange( &Death->Active, 1 ) == 1 )
    {
        return ;
    }

#if !USE_RTLTHREADS
    UnregisterWait( Death->Notify );
#endif

    h = NULL ;

    while ( h == NULL )
    {
        h = CreateThread( NULL, 0,
                          SystemProcessDeathWorker,
                          Death,
                          0,
                          &tid );

        if ( !h )
        {
            SleepEx( 125, TRUE );
            return ;
        }
    }

    CloseHandle( h );


}


/***************************************************************************\
* SetWinlogonDeviceMap
*
* For non-console winlogon's, make them point to the session specific
* DeviceMap.
*
* History:
*  09-November-1997 SalimC     Created
\***************************************************************************/
NTSTATUS
SetWinlogonDeviceMap( ULONG SessionId)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR szSessionString[MAX_SESSION_PATH];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;
    HANDLE DosDevicesDirectory;



    if (SessionId == 0) {

       return STATUS_INVALID_PARAMETER;

    }

    swprintf(szSessionString,L"\\Sessions\\%ld\\DosDevices",SessionId);

    RtlInitUnicodeString( &UnicodeString, szSessionString );


    InitializeObjectAttributes( &Obja,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenDirectoryObject( &DosDevicesDirectory,
                                    DIRECTORY_ALL_ACCESS,
                                    &Obja
                                    );
    if (!NT_SUCCESS( Status )) {

         return Status;

    }



   //
   //  Set the Winlogon's ProcessDeviceMap to the session specific DosDevices Directory
   //

    ProcessDeviceMapInfo.Set.DirectoryHandle = DosDevicesDirectory;

    Status = NtSetInformationProcess( NtCurrentProcess(),
                                      ProcessDeviceMap,
                                      &ProcessDeviceMapInfo.Set,
                                      sizeof( ProcessDeviceMapInfo.Set )
                                    );

    NtClose(DosDevicesDirectory);


    return Status;

}
