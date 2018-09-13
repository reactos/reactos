//*************************************************************
//  File name: winlogon.c
//
//  Description:  Main entry point
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1996
//  All rights reserved
//
//*************************************************************

#include "precomp.h"
#pragma hdrstop

//
// Global variables
//
#if !defined(_WIN64)
#include <delayimp.h>

FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );


PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;

#endif

ULONG   g_SessionId;
int     g_Console;
BOOL    g_IsTerminalServer;

HANDLE  hFontThread = NULL;

HINSTANCE g_hInstance = NULL;
PTERMINAL g_pTerminals = NULL;
UINT      g_uSetupType  = SETUPTYPE_NONE;
BOOL      g_fExecuteSetup = FALSE;
PSID      g_WinlogonSid;
BOOL      g_fAllowStatusUI = TRUE;


HANDLE NetworkProviderEvent ;
BOOL KernelDebuggerPresent ;

ULONG g_BreakinProcessId=0;


//
// The default desktop section sizes in kb
//

DWORD     g_dwScreenSaverDSSize = 64;
DWORD     g_dwApplicationDSSize = 0;
DWORD     g_dwWinlogonDSSize    = 128;


//
// Local function proto-types
//

BOOL InitializeGlobals (HINSTANCE hInstance);
BOOL CreatePrimaryTerminal (void);
VOID MiscInitialization (PTERMINAL pTerm);

//
// application desktop thread declaration
//

VOID StartAppDesktopThread(PTERMINAL pTerm);

DWORD
WINAPI
WinlogonUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    CHAR Text[80];

    DbgPrint( "Unhandled Exception hit in winlogon.exe\n" );
    _snprintf( Text, 80, "first, enter !exr %p for the exception record\n", ExceptionInfo->ExceptionRecord );
    DbgPrint( Text );
    _snprintf( Text, 80, "next, enter !cxr %p for the context\n", ExceptionInfo->ContextRecord );
    DbgPrint( Text );
    DbgPrint( "then !kb to get the faulting stack\n" );
    DebugBreak();

    if ( ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_POSSIBLE_DEADLOCK )
    {
        DbgPrint( "Restarting wait on critsec or resource at %p\n",
                  ExceptionInfo->ExceptionRecord->ExceptionInformation[ 0 ] );
        return EXCEPTION_CONTINUE_EXECUTION ;
    }
    return EXCEPTION_CONTINUE_SEARCH ;
}

//+---------------------------------------------------------------------------
//
//  Function:   UpdateTcpIpParameters
//
//  Synopsis:   Copy non-volatile settings to volatile settings
//
//  Arguments:  (none)
//
//  History:    6-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
UpdateTcpIpParameters(
    VOID
    )
{
    HKEY Key ;
    int err ;
    WCHAR LocalSpace[ 64 ];
    PWSTR Buffer ;
    DWORD Size ;
    DWORD Type ;

    Key = NULL ;

    Buffer = LocalSpace ;
    Size = sizeof( LocalSpace ) ;

    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
            0,
            KEY_READ | KEY_WRITE,
            &Key );

    if ( err == 0 )
    {

        err = RegQueryValueEx(
                    Key,
                    TEXT("NV Hostname"),
                    0,
                    &Type,
                    (PBYTE) Buffer,
                    &Size );

        if ( ( err == ERROR_INSUFFICIENT_BUFFER ) ||
             (err == ERROR_MORE_DATA ))

        {
            Buffer = LocalAlloc( LMEM_FIXED, Size );

            if ( !Buffer )
            {
                goto Update_Exit ;
            }

            err = RegQueryValueEx(
                        Key,
                        TEXT("NV Hostname"),
                        0,
                        &Type,
                        (PBYTE) Buffer,
                        &Size );
        }

        if ( err == 0 )
        {

            RegSetValueEx(
                Key,
                TEXT("Hostname"),
                0,
                REG_SZ,
                (PBYTE) Buffer,
                Size );

        }

        //
        // now, do the same for DnsDomain
        //

        err = RegQueryValueEx(
                    Key,
                    TEXT("NV Domain"),
                    0,
                    &Type,
                    (PBYTE) Buffer,
                    &Size );

        if ( (err == ERROR_INSUFFICIENT_BUFFER) ||
             (err == ERROR_MORE_DATA ))
        {
            if ( Buffer != LocalSpace )
            {
                LocalFree( Buffer );
            }

            Buffer = LocalAlloc( LMEM_FIXED, Size );

            if ( !Buffer )
            {
                goto Update_Exit ;
            }

            err = RegQueryValueEx(
                        Key,
                        TEXT("NV Domain"),
                        0,
                        &Type,
                        (PBYTE) Buffer,
                        &Size );
        }

        if ( err == 0 )
        {
            RegSetValueEx(
                Key,
                TEXT("Domain"),
                0,
                REG_SZ,
                (PBYTE) Buffer,
                Size );
        }

        RegCloseKey( Key );

        Key = NULL ;

    }

    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("Software\\Policies\\Microsoft\\System\\DNSclient"),
            0,
            KEY_READ | KEY_WRITE,
            &Key );

    if ( err == 0 )
    {
        //
        // now, do the same for the suffix that comes down through policy
        //

        err = RegQueryValueEx(
                    Key,
                    TEXT("NV PrimaryDnsSuffix"),
                    0,
                    &Type,
                    (PBYTE) Buffer,
                    &Size );

        if ( (err == ERROR_INSUFFICIENT_BUFFER) ||
             (err == ERROR_MORE_DATA ))
        {
            if ( Buffer != LocalSpace )
            {
                LocalFree( Buffer );
            }

            Buffer = LocalAlloc( LMEM_FIXED, Size );

            if ( !Buffer )
            {
                goto Update_Exit ;
            }

            err = RegQueryValueEx(
                        Key,
                        TEXT("NV PrimaryDnsSuffix"),
                        0,
                        &Type,
                        (PBYTE) Buffer,
                        &Size );
        }

        if ( err == 0 )
        {
            RegSetValueEx(
                Key,
                TEXT("PrimaryDnsSuffix"),
                0,
                REG_SZ,
                (PBYTE) Buffer,
                Size );
        }
        else 
        {
            RegDeleteValue(
                Key,
                TEXT("PrimaryDnsSuffix" ) );
        }

        RegCloseKey( Key );

        Key = NULL ;

    }


Update_Exit:

    if ( Key )
    {
        RegCloseKey( Key );
    }

    if ( Buffer != LocalSpace )
    {
        LocalFree( Buffer );
    }


}

//*************************************************************
//
//  InitializeSlowStuff()
//
//  Purpose:    Initializes slow stuff
//
//  Parameters: dummy
//
//  Return:     0
//
//*************************************************************

DWORD InitializeSlowStuff (LPVOID dummy)
{

    //
    // initialize the dll hell watcher
    //

    Sleep(100);

    //
    // call the winmm guys so they can start their message window
    // and whatever else they need.
    //

#if !defined(_WIN64_LOGON)

    try
    {
        waveOutGetNumDevs();
    }
    except ( EXCEPTION_EXECUTE_HANDLER )
    {
        DebugLog(( DEB_ERROR, "Failed to initialize winmm, %x\n", GetExceptionCode() ));
    }


#endif

    if (SfcInitProt( SFC_REGISTRY_DEFAULT, SFC_DISABLE_NORMAL, SFC_SCAN_NORMAL, SFC_QUOTA_DEFAULT, NULL, NULL )) {
#if DBG
        ExitProcess( EXIT_SYSTEM_PROCESS_ERROR );
#endif
    }



    return 0;
}


//*************************************************************
//
//  InitializeGlobals()
//
//  Purpose:    Initialize global variables / environment
//
//
//  Parameters: hInstance   -   Winlogon's instance handle
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL InitializeGlobals(HINSTANCE hInstance)
{
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    ULONG SidLength;
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH+1;
    DWORD dwSize;
    TCHAR szProfile[MAX_PATH];
    PROCESS_SESSION_INFORMATION SessionInfo;
    HKEY hKey;
    DWORD dwType ;
    DWORD dwOptionValue = 0;




    g_IsTerminalServer = !!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer));


    if (g_IsTerminalServer) {

        //
        // Query Winlogon's Session Id
        //

        if (!NT_SUCCESS(NtQueryInformationProcess(
                         NtCurrentProcess(),
                         ProcessSessionInformation,
                         &SessionInfo,
                         sizeof(SessionInfo),
                         NULL
                         ))) {

            ASSERT(FALSE);

            ExitProcess( EXIT_INITIALIZATION_ERROR );

        }

        g_SessionId = SessionInfo.SessionId;

    } else {

        //
        // For Non TerminaServer SessionId is always 0
        //
        g_SessionId = 0;

    }

    if (g_SessionId == 0) {

       g_Console = TRUE;

    }


    if (g_IsTerminalServer) {

        if (!InitializeMultiUserFunctionsPtrs()) {
            ExitProcess( EXIT_INITIALIZATION_ERROR );
        }
    }


    //
    // Register with windows so we can create windowstation etc.
    //

    if (!RegisterLogonProcess(HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess), TRUE)) {
        DebugLog((DEB_ERROR, "could not register itself as logon process\n"));
        return FALSE;
    }


    //
    // Store away our instance handle
    //

    g_hInstance = hInstance;


    //
    // Get our sid so it can be put on object ACLs
    //

    SidLength = RtlLengthRequiredSid(1);
    g_WinlogonSid = (PSID)Alloc(SidLength);
    ASSERTMSG("Winlogon failed to allocate memory for system sid", g_WinlogonSid != NULL);

    RtlInitializeSid(g_WinlogonSid,  &SystemSidAuthority, 1);
    *(RtlSubAuthoritySid(g_WinlogonSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;


    //
    //  Get setup information
    //

    g_uSetupType = CheckSetupType() ;
    g_fExecuteSetup = (g_uSetupType == SETUPTYPE_FULL) ||
                      (g_uSetupType == SETUPTYPE_UPGRADE);


    if (!g_fExecuteSetup) {

        LARGE_INTEGER Time = USER_SHARED_DATA->SystemExpirationDate;

        //
        // Print the license expire time
        //

        if (Time.QuadPart) {
#if DBG
            FILETIME   LocalTime;
            SYSTEMTIME SysTime;

            FileTimeToLocalFileTime((CONST FILETIME*)&Time, &LocalTime);
            FileTimeToSystemTime((CONST FILETIME*)&LocalTime, &SysTime);

            DebugLog((DEB_TRACE,
                "Your NT System License Expires %2d/%2d/%4d @ %d:%d\n",
                SysTime.wMonth,
                SysTime.wDay,
                SysTime.wYear,
                SysTime.wHour,
                SysTime.wMinute));
#endif // DBG
        }


        //
        // Check for verbose status messages
        //

        QueryVerboseStatus();
    }


    //
    // Get a copy of the computer name in *my* environment, so that we
    // can look at it later.
    //

    if (GetComputerName (szComputerName, &dwComputerNameSize)) {
        SetEnvironmentVariable(COMPUTERNAME_VARIABLE, (LPTSTR) szComputerName);
    }


    //
    // Set the default USERPROFILE and ALLUSERSPROFILE locations
    //

    if (g_fExecuteSetup) {
        DetermineProfilesLocation(((g_uSetupType == SETUPTYPE_FULL) ? TRUE : FALSE));
    }

    dwSize = ARRAYSIZE(szProfile);
    if (GetDefaultUserProfileDirectory (szProfile, &dwSize)) {
        SetEnvironmentVariable(USERPROFILE_VARIABLE, szProfile);
    }

    dwSize = ARRAYSIZE(szProfile);
    if (GetAllUsersProfileDirectory (szProfile, &dwSize)) {
        SetEnvironmentVariable(ALLUSERSPROFILE_VARIABLE, szProfile);
    }

    return TRUE;
}


// Lifted the code below from breakin.exe

#define STACKSIZE 32768

void Breakin(ULONG BreakinProcessId)
{
    HANDLE Token ;
    UCHAR Buf[ sizeof( TOKEN_PRIVILEGES ) + sizeof( LUID_AND_ATTRIBUTES ) ];
    UCHAR Buf2[ sizeof( Buf ) ];
    PTOKEN_PRIVILEGES Privs ;
    PTOKEN_PRIVILEGES NewPrivs ;
    DWORD size ;
    LPTHREAD_START_ROUTINE DbgBreakPoint;
    HANDLE ntdll;
    ULONG ThreadId;
    HANDLE Process;
    HANDLE Thread;
    
    if (OpenProcessToken( GetCurrentProcess(),
                      MAXIMUM_ALLOWED,
                      &Token ))

    {
        Privs = (PTOKEN_PRIVILEGES) Buf ;

        Privs->PrivilegeCount = 1 ;
        Privs->Privileges[0].Luid.LowPart = 20L ;
        Privs->Privileges[0].Luid.HighPart = 0 ;
        Privs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED ;

        NewPrivs = (PTOKEN_PRIVILEGES) Buf2 ;

        if (AdjustTokenPrivileges( Token,
                               FALSE,
                               Privs,
                               sizeof( Buf2 ),
                               NewPrivs,
                               &size )) {

        
            Process = OpenProcess(
                                   PROCESS_ALL_ACCESS,
                                   FALSE,
                                   BreakinProcessId
                                );
            if (Process) {

                //
                // Looking at the source code, it doesn't need to be freed.
                // Check in the debugger.
                //

                ntdll = GetModuleHandle(L"ntdll.dll");

                if (ntdll) {

                    DbgBreakPoint = (LPTHREAD_START_ROUTINE)GetProcAddress(ntdll, "DbgBreakPoint");

                    Thread = CreateRemoteThread(
                                        Process,
                                        NULL,
                                        STACKSIZE,
                                        DbgBreakPoint,
                                        NULL,
                                        CREATE_SUSPENDED,
                                        &ThreadId
                                        );

                    if (Thread) {
                        SetThreadPriority(Thread, THREAD_PRIORITY_HIGHEST);
                        ResumeThread(Thread);
                        CloseHandle(Thread);
                    }
                }

                CloseHandle(Process);
            }

            //
            // Once the remote thread is started, return to the old privileges
            // so that nothing else gets screwed.
            //

            AdjustTokenPrivileges( Token,
                                   FALSE,
                                   NewPrivs,
                                   0,
                                   NULL,
                                   NULL );
        }

        CloseHandle( Token );
    }
}



ULONG
NTAPI
WlpPeriodicBreak(
    PVOID Param,
    BOOLEAN Timeout
    )
{
    LARGE_INTEGER ZeroExpiration;
    HANDLE hToken;
    DWORD ReturnLength;
    TOKEN_STATISTICS TokenStats;
    NTSTATUS Status;

    ZeroExpiration.QuadPart = 0;

    
    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY,
                 &hToken
                 );

    if (NT_SUCCESS( Status )) {

        Status = NtQueryInformationToken (
                     hToken,
                     TokenStatistics,
                     &TokenStats,
                     sizeof( TOKEN_STATISTICS ),
                     &ReturnLength
                     );

        if (NT_SUCCESS( Status )) {

            if (RtlLargeIntegerEqualTo( TokenStats.ExpirationTime, ZeroExpiration )) {

                DbgBreakPoint();
            }
        }

        NtClose( hToken );
    }


    if (g_BreakinProcessId != 0) {
        Breakin(g_BreakinProcessId);    
    }    

    return(0);
}

//*************************************************************
//
//  WinMain ()
//
//  Purpose:    Main entry point
//
//  Parameters: hInstance       -   Instance handle
//              hPrevInstance   -   Previous instance handle
//              lpCmdLine       -   Command line
//              nCmdShow        -   ShowWindow argument
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
    PTERMINAL pTerm;
    LPWSTR    pszGinaName;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo ;
    HKEY hKey;
    DWORD dwType, dwSize;
    DWORD dwOptionValue = 0;
    HANDLE hThread;
    DWORD dwThreadID;

    NtQuerySystemInformation(
        SystemKernelDebuggerInformation,
        &KdInfo,
        sizeof( KdInfo ),
        NULL );

    if ( KdInfo.KernelDebuggerEnabled || NtCurrentPeb()->BeingDebugged )
    {
        SetUnhandledExceptionFilter( WinlogonUnhandledExceptionFilter );

        KernelDebuggerPresent = TRUE ;

        SetTimerQueueTimer( NULL,
                            WlpPeriodicBreak,
                            NULL,
                            60 * 1000,
                            60 * 1000,
                            FALSE );
    }


    //
    // Initialize debug support and logging
    //

    InitDebugSupport();


    //
    // Make ourselves more important
    //

    if (!SetProcessPriority())
    {
        ExitProcess( EXIT_INITIALIZATION_ERROR );
    }

    //
    // Map the TCPIP information
    //

    UpdateTcpIpParameters();


    //
    // Initialize the globals
    //

    if ( !InitializeGlobals(hInstance) )
    {
        ExitProcess( EXIT_INITIALIZATION_ERROR );
    }


    //
    // Check the pagefile
    //

    if (!g_fExecuteSetup)
    {
        CreateTemporaryPageFile();
    }

    //
    // Initialize security
    //

    if (!InitializeSecurity ())
    {
        ExitProcess( EXIT_SECURITY_INIT_ERROR );
    }

    //
    // Create the primary terminal.
    //

    if (!CreatePrimaryTerminal())
    {
        DebugLog((DEB_TRACE_INIT, "CreatePrimaryTerminal failed\n"));
        ExitProcess( EXIT_PRIMARY_TERMINAL_ERROR );
    }

    pTerm = g_pTerminals ;


    //
    // Check for safemode:
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, SAFEBOOT_OPTION_KEY, 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(dwOptionValue);
        RegQueryValueEx (hKey, OPTION_VALUE, NULL, &dwType,
                         (LPBYTE) &dwOptionValue, &dwSize);

        RegCloseKey (hKey);


        //
        // Check if the options value is minimal, if so exit now
        // so Group Policy is not applied.

        if (dwOptionValue == SAFEBOOT_MINIMAL) {

            pTerm->SafeMode = TRUE ;
        }
    }


    //
    //  Set this thread to winlogon's desktop
    //

    SetThreadDesktop( pTerm->pWinStaWinlogon->hdeskWinlogon );

    //
    // Change user to 'system'
    //
    if (!SecurityChangeUser(pTerm, NULL, NULL, g_WinlogonSid, FALSE)) {
        DebugLog((DEB_ERROR, "failed to set user to system\n"));
    }

    //
    // For the console winlogon, set the event that is toggled
    // when the network provider list changes
    //

    if (g_Console) {

        CreateNetworkProviderEvent();

    }

    if ( !WlpInitializeNotifyList( pTerm ) ||
         !InitializeJobControl() ||
         !LogoffLockInit() )
    {
        ExitProcess( EXIT_NO_MEMORY );
    }

    SetThreadDesktop(pTerm->pWinStaWinlogon->hdeskWinlogon);

    DebugLog((DEB_TRACE_INIT, "Boot Password Check\n" ));

    if (g_Console) {

        SbBootPrompt();

    }

    if ( !ScInit() )
    {
        ExitProcess( EXIT_NO_MEMORY );
    }


    //
    // Start the system processes
    //

    DebugLog((DEB_TRACE_INIT, "Execute system processes:\n"));

    if (!ExecSystemProcesses())
    {
        DebugLog((DEB_TRACE_INIT, "ExecSystemProcesses failed\n"));
        ExitProcess( EXIT_SYSTEM_PROCESS_ERROR );
    }

    pTerm->ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS |
                                     SEM_NOOPENFILEERRORBOX );

    DebugLog((DEB_TRACE_INIT, "Done with system processes:\n"));

    if (g_Console) {

        DebugLog(( DEB_TRACE_INIT, "Sync with thread started by SbBootPrompt\n"));

        SbSyncWithKeyThread();

    }

    //
    // Start the ApplicationDesktopThread now before we first log on
    //

#ifndef _WIN64

    StartAppDesktopThread(pTerm);

#endif // _WIN64

    //
    // Finish some misc initialization.  Note:  This call can drop into setup
    //

    MiscInitialization(pTerm);


    //
    // Kick off a thread to watch the system dlls, initialize winmm, etc
    //

    if (g_Console) {
        hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) InitializeSlowStuff,
                                NULL, 0, &dwThreadID);

        if (hThread) {
            SetThreadPriority (hThread, THREAD_PRIORITY_IDLE);
            CloseHandle (hThread);
        } else {
            InitializeSlowStuff (NULL);
        }
    }


    //
    // Load a GINA DLL for this terminal
    //

    pszGinaName = (LPWSTR)LocalAlloc (LPTR, sizeof(WCHAR) * MAX_PATH);

    if ( !pszGinaName )
    {
        ExitProcess( EXIT_NO_MEMORY );
    }

    if ( pTerm->SafeMode )
    {
        ExpandEnvironmentStringsW(
                    TEXT("%SystemRoot%\\system32\\msgina.dll"),
                    pszGinaName,
                    MAX_PATH );
    }
    else
    {
        GetProfileString(
            APPLICATION_NAME,
            GINA_KEY,
            TEXT("msgina.dll"),
            pszGinaName,
            MAX_PATH);

    }


    if (!LoadGinaDll (pTerm, pszGinaName)) {
        DebugLog((DEB_TRACE_INIT, "Failed to load gina\n"));
        ExitProcess( EXIT_GINA_ERROR );
    }

    LocalFree (pszGinaName);

    //
    // Initialize the secure attention sequence
    //

    if (!SASInit(pTerm))
    {
        DebugLog((DEB_TRACE_INIT, "Failed to create sas window\n"));
        ExitProcess( EXIT_SAS_WINDOW_ERROR );
    }

    //
    // Initialize GPO support
    //


    InitializeGPOSupport( pTerm );

    InitializeAutoEnrollmentSupport ();

    WlAddInternalNotify(
                PokeComCtl32,
                WL_NOTIFY_LOGOFF,
                FALSE,
                FALSE,
                TEXT("Reset ComCtl32"),
                15 );


    //
    // Main loop
    //

    MainLoop (pTerm);

    //
    // Shutdown the machine
    //

    if (g_IsTerminalServer) {

        //
        // Standard NT never exits the MainLoop above unless shutdown
        // is requested.  HYDRA exits MainLoop for all non-console
        // WinStations, therefore we must check if shutdown is desired.
        //

        if ( IsShutdown(pTerm->LastGinaRet) )
            ShutdownMachine(pTerm, pTerm->LastGinaRet);

         //
         // If its the console, and another WinStation did the shutdown,
         // we must wait for the systems demise. If we exit here, we bluescreen
         // while the shutdown is in process.
         //
         if( g_Console ) {
             SleepEx((DWORD)-1, FALSE);
         }

    } else {

        ShutdownMachine(pTerm, pTerm->LastGinaRet);

        //
        // Should never get here
        //

        DebugLog((DEB_ERROR, "ShutdownMachine failed!\n"));
        ASSERT(!"ShutdownMachine failed!");

    }

    ExitProcess( EXIT_SHUTDOWN_FAILURE );

    return( 0 );
}

//*************************************************************
//
//  CreatePrimaryTerminal()
//
//  Purpose:    Creates the primary terminal
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

extern PTERMINAL pShutDownTerm;

BOOL CreatePrimaryTerminal (void)
{
    PTERMINAL       pTerm = NULL;
    PWINDOWSTATION  pWS   = NULL;
    NTSTATUS Status ;

    //
    // Allocate space for a new terminal
    //

    pTerm = LocalAlloc (LPTR, sizeof(TERMINAL) +
                        (lstrlenW(DEFAULT_TERMINAL_NAME) + 1) * sizeof(WCHAR));
    if (!pTerm) {
        DebugLog((DEB_ERROR, "Could not allocate terminal structure\n"));
        return FALSE;
    }

    //
    // Check mark
    //
    pTerm->CheckMark = TERMINAL_CHECKMARK;

    ZeroMemory(pTerm->Mappers, sizeof(WindowMapper) * MAX_WINDOW_MAPPERS);
    pTerm->cActiveWindow  = 0;
    pTerm->PendingSasHead = 0;
    pTerm->PendingSasTail = 0;

    //
    // Wait here for the connection
    //
    if ( !g_Console  ) {
        if ( !gpfnWinStationWaitForConnect() ) {
            DebugLog((DEB_ERROR, "wait for connect failed\n"));
            return(FALSE);
        }
    }

    //
    // For non-console winlogon's, make them point to the session specific
    // DeviceMap.
    //
    if (!g_Console) {
        if (!NT_SUCCESS(SetWinlogonDeviceMap(g_SessionId))) {
            ExitProcess( EXIT_DEVICE_MAP_ERROR );
        }
    }

    //
    // Create interactive window station
    //

    //
    // Allocate space for a new WindowStation
    //
    pWS = LocalAlloc (LPTR, sizeof(WINDOWSTATION) +
                     (lstrlenW(WINDOW_STATION_NAME) + 1) * sizeof(WCHAR));
    if (!pWS) {
        DebugLog((DEB_ERROR, "Could not allocate windowstation structure\n"));
        goto failCreateTerminal;
    }

    //
    // Save the name
    //
    pWS->lpWinstaName = (LPWSTR)((LPBYTE) pWS + sizeof(WINDOWSTATION));
    lstrcpyW (pWS->lpWinstaName, WINDOW_STATION_NAME);

    Status = RtlInitializeCriticalSection( &pWS->UserProcessData.Lock );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "Could not create critical section\n" ));
        goto failCreateTerminal ;
    }

    pWS->UserProcessData.Ref = 0 ;

    //
    // Create the window station
    //
    pWS->hwinsta = CreateWindowStationW (WINDOW_STATION_NAME, 0, MAXIMUM_ALLOWED, NULL);
    if (!pWS->hwinsta) {
        DebugLog((DEB_ERROR, "Could not create the interactive windowstation\n"));
        goto failCreateTerminal;
    }

    SetProcessWindowStation(pWS->hwinsta);

    InitializeWinstaSecurity(pWS);

    pTerm->pWinStaWinlogon = pWS;

    //
    // Create winlogon's desktop
    //
    pWS->hdeskWinlogon = CreateDesktopW (WINLOGON_DESKTOP_NAME,
                                         NULL, NULL, 0, MAXIMUM_ALLOWED, NULL);
    if (!pWS->hdeskWinlogon) {
        DebugLog((DEB_ERROR, "Could not create winlogon's desktop\n"));
        goto failCreateTerminal;
    }

    //
    // Create the application desktop
    //
    pWS->hdeskApplication = CreateDesktopW (APPLICATION_DESKTOP_NAME,
                                            NULL, NULL, 0, MAXIMUM_ALLOWED, NULL);
    if (!pWS->hdeskApplication) {
        DebugLog((DEB_ERROR, "Could not create application's desktop\n"));
        goto failCreateTerminal;
    }

    //
    // Set desktop security (no user access yet)
    //
    if (!SetWinlogonDesktopSecurity(pWS->hdeskWinlogon, g_WinlogonSid)) {
        DebugLog((DEB_ERROR, "Failed to set winlogon desktop security\n"));
    }
    if (!SetUserDesktopSecurity(pWS->hdeskApplication, NULL, g_WinlogonSid)) {
        DebugLog((DEB_ERROR, "Failed to set application desktop security\n"));
    }

    //
    // Switch to the winlogon desktop
    //
    SetActiveDesktop(pTerm, Desktop_Winlogon);

    //
    // Save this terminal in the global list
    //
    pTerm->pNext = g_pTerminals;
    g_pTerminals = pTerm;

    //
    // Set the shutdown terminal now so we won't AV when upgrade.
    //
    pShutDownTerm = pTerm;

    //
    // Initialize Multi-User Globals
    //
    RtlZeroMemory( &pTerm->MuGlobals, sizeof(pTerm->MuGlobals));
    // We have not retrieved the USERCONFIG yet.
    pTerm->MuGlobals.ConfigQueryResult = ERROR_INVALID_DATA;

    if (g_IsTerminalServer) {
        //
        // Enable WinStation logons during console initilization.
        //
        if ( g_SessionId == 0 ) {
            (VOID) WriteProfileString( APPNAME_WINLOGON, WINSTATIONS_DISABLED, TEXT("0") );
        }
    }

    return TRUE;

failCreateTerminal:

    //
    // Cleanup
    //
    if (pWS) {
        if (pWS->hdeskApplication)
            CloseDesktop(pWS->hdeskApplication);

        if (pWS->hdeskWinlogon)
            CloseDesktop(pWS->hdeskWinlogon);

        if (pWS->hwinsta)
            CloseWindowStation (pWS->hwinsta);

        LocalFree (pWS);
    }
    if (pTerm) {
        LocalFree (pTerm);
    }

    return FALSE;
}

//*************************************************************
//
//  MiscInitialization()
//
//  Purpose:    Misc initialization that needs to be done after
//              the first terminal is created.
//
//  Parameters: pTerm   -   Terminal info
//
//  Return:     void
//
//*************************************************************

VOID MiscInitialization (PTERMINAL pTerm)
{
    DWORD Win31MigrationFlags;


    //
    // Decide what to do about setup
    //

    if (g_fExecuteSetup)
    {
        //
        // Init winmm
        //

#if !defined(_WIN64_LOGON)

        try
        {
            waveOutGetNumDevs();
        }
        except ( EXCEPTION_EXECUTE_HANDLER )
        {
            DebugLog(( DEB_ERROR, "Failed to initialize winmm, %x\n", GetExceptionCode() ));
        }

#endif

        //
        // Run setup and reboot
        //

        ExecuteSetup(pTerm);
        EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
        NtShutdownSystem(ShutdownReboot);

    }
    else
    {
        //
        // In the case of running setup in mini-setup mode,
        // we want to be able to continue through and let the
        // user logon.
        //
        if(g_uSetupType == SETUPTYPE_NOREBOOT) {

            //
            // go execute setup
            //
            ExecuteSetup(pTerm);
        }

        //
        // Don't go any further if setup didn't complete fully.  If this
        // machine has not completed setup correctly, this will not return.
        //

        CheckForIncompleteSetup(g_pTerminals);
    }


    if (!IsWin9xUpgrade()) {
        //
        // Check to see if there is any WIN.INI or REG.DAT to migrate into
        // Windows/NT registry.
        //
        // This code is skipped when the previous OS was Win9x.
        //

        Win31MigrationFlags = QueryWindows31FilesMigration( Win31SystemStartEvent );
        if (Win31MigrationFlags != 0) {
            SynchronizeWindows31FilesAndWindowsNTRegistry( Win31SystemStartEvent,
                                                           Win31MigrationFlags,
                                                           NULL,
                                                           NULL
                                                         );
            InitSystemFontInfo();
        }
    }

#ifdef _X86_

    //
    // Do OS/2 Subsystem boot-time migration.
    // Only applicable to x86 builds.
    //

    Os2MigrationProcedure();

#endif


    //
    // Load those pesky fonts:
    //

    hFontThread = StartLoadingFonts();



    //
    // Check if we need to run setup's GUI repair code
    //

    CheckForRepairRequest ();
}
