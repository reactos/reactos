/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    setuplgn.c

Abstract:

    Routines for the special version of winlogon for Setup.

Author:

    Ted Miller (tedm) 4-May-1992

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#include "shlobj.h"
#include "shlobjp.h"


#if DEVL
BOOL bDebugSetup;
#endif


typedef enum _REG_OP {
    RegOpAppendSubString,
    RegOpInsertSubString,
    RegOpRemoveSubString
} REG_OP ;

typedef struct _REG_CHANGE {
    PWSTR   ValueName ;
    PWSTR   SubString ;
    REG_OP  Operation ;
} REG_CHANGE, * PREG_CHANGE ;

//
// This list is order dependent.  They are processed serially.  A replace 
// can be accomplished by a remote followed by an insert or append.
//
REG_CHANGE WinlogonSetupChanges[] = {
    { L"Userinit", L"Nddeagnt.exe", RegOpRemoveSubString },
    { L"userinit", L"nddeagnt", RegOpRemoveSubString },
    { L"Userinit", L"userinit.exe", RegOpRemoveSubString },
    { L"Userinit", L"userinit", RegOpRemoveSubString},
    { L"Userinit", L"%SystemRoot%\\system32\\userinit.exe", RegOpInsertSubString},
    { L"System", L"lsass.exe", RegOpRemoveSubString }
} ;

//
// Handle to the event used by lsa to stall security initialization.
//

HANDLE LsaStallEvent = NULL;

//
// Thread Id of the main thread of setuplgn.
//

DWORD MainThreadId;



DWORD
WaiterThread(
    PVOID hProcess
    );

VOID
SetWin9xUpgradePasswords (
    PTERMINAL pTerm
    );


VOID
UpdateRegistryItem(
    PREG_CHANGE Change
    )
{
    HKEY Key ;
    int err ;
    DWORD Type ;
    DWORD Size ;
    PWSTR String ;
    WCHAR Scratch[ 128 ];
    PWSTR Scan ;
    PWSTR Trail ;
    PWSTR Changed ;
    BOOL StringChanged = FALSE ;
    PWSTR Percent = NULL ;
    PWSTR StoreSubString ;

    Percent = wcschr( Change->SubString, L'%' );

    if ( Percent )
    {
        //
        // Need to expand environment strings in this replacement
        //

        Percent = LocalAlloc( LMEM_FIXED, MAX_PATH * sizeof( WCHAR ) );

        if ( Percent == NULL )
        {
            return ;
        }

        Size = ExpandEnvironmentStrings( Change->SubString,
                                         Percent,
                                         MAX_PATH );

        if ( (Size == 0) || (Size > MAX_PATH) )
        {
            //
            // What?  Fail the change?
            //

            LocalFree( Percent );

            if ( Size == 0 )
            {
                return;
            }

            Percent = LocalAlloc( LMEM_FIXED, (Size + 1) * sizeof( WCHAR ) );

            if ( !Percent )
            {
                return;
            }


            Size = ExpandEnvironmentStrings( Change->SubString,
                                             Percent,
                                             Size );

            if ( Size == 0  )
            {
                LocalFree( Percent );

                return ;
            }
        }

        StoreSubString = Change->SubString ;
        Change->SubString = Percent ;

    }


    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      WINLOGON_KEY,
                      0,
                      KEY_READ | KEY_WRITE,
                      &Key );

    if ( err )
    {
        return ;
    }

    Size = sizeof( Scratch );
    String = Scratch ;

    err = RegQueryValueEx(
                Key,
                Change->ValueName,
                0,
                &Type,
                (PUCHAR) String,
                &Size );

    if ( err )
    {
        if ( ( err == ERROR_PATH_NOT_FOUND ) ||
             ( err == ERROR_FILE_NOT_FOUND ) )
        {
            if ( ( Change->Operation == RegOpAppendSubString ) ||
                 ( Change->Operation == RegOpInsertSubString ) )
            {
                RegSetValueEx(
                    Key,
                    Change->ValueName,
                    0,
                    REG_SZ,
                    (PUCHAR) Change->SubString,
                    ( wcslen( Change->SubString ) + 1 ) * sizeof( WCHAR ) );
            }
            RegCloseKey( Key );
            return;
        }

        String = LocalAlloc( LMEM_FIXED, Size );

        if ( !String )
        {
            RegCloseKey( Key );
            return ;
        }
        
        err = RegQueryValueEx(
                    Key,
                    Change->ValueName,
                    0,
                    &Type,
                    (PUCHAR) String,
                    &Size );

        if ( err )
        {
            goto RegUpdate_Cleanup ;
        }
    }

    Scan = String ;
    Trail = String ;

    while ( Scan && (*Scan != L'\0') )
    {
        Scan = wcschr( Scan, L',' );

        if ( Scan )
        {
            *Scan = L'\0' ;
        }

        if ( _wcsicmp( Trail, Change->SubString ) == 0 )
        {
            //
            // Found it, now what are we supposed to do?
            // 

            if ( ( Change->Operation == RegOpAppendSubString ) ||
                 ( Change->Operation == RegOpInsertSubString ) )
            {
                //
                // we were supposed to add this string, but it is already present.
                // we're done, bail out.
                //

                goto RegUpdate_Cleanup ;
            }

            if ( Change->Operation == RegOpRemoveSubString )
            {
                //
                // Need to remove this one.
                //

                if ( Scan )
                {
                    RtlMoveMemory( Trail, 
                                   Scan + 1 , 
                                   ( wcslen( Scan + 1 ) + 1 ) * sizeof( WCHAR ) );
                }
                else 
                {
                    //
                    // Easy case:
                    //

                    *Trail = L'\0' ; 
                }

                StringChanged = TRUE ;

                continue;
            }
        }

        if ( Scan )
        {
            *Scan = L',';
            Scan++ ;
            Trail = Scan ;
        }
    }

    if ( ( Change->Operation == RegOpInsertSubString ) ||
         ( Change->Operation == RegOpAppendSubString ) )
    {
        Changed = LocalAlloc( LMEM_FIXED, 
                              Size + ( wcslen( Change->SubString ) + 2 ) * sizeof( WCHAR ) );

        if ( Changed )
        {
            *Changed = L'\0';

            if ( Change->Operation == RegOpInsertSubString )
            {
                wcscpy( Changed,
                        Change->SubString );
                wcscat( Changed, L"," );

            }

            wcscat( Changed, String );

            if ( Change->Operation == RegOpAppendSubString )
            {
                wcscat( Changed, L"," );
                wcscat( Changed, Change->SubString );
            }

            if ( String != Scratch )
            {
                LocalFree( String );
            }

            String = Changed ;

            StringChanged = TRUE ;
        }
    }

    if ( StringChanged )
    {
        RegSetValueEx(
            Key,
            Change->ValueName,
            0,
            REG_SZ,
            (PUCHAR) String,
            (wcslen( String ) + 1 ) * sizeof( WCHAR ) );
    }

RegUpdate_Cleanup:

    if ( String != Scratch )
    {
        LocalFree( String );
    }
    if ( Percent )
    {
        LocalFree( Percent );
        Change->SubString = StoreSubString ;
    }

    RegCloseKey( Key );

}

VOID
FixupRegistry(
    VOID
    )
{
    ULONG i ;

    for ( i = 0 ; i < sizeof( WinlogonSetupChanges ) / sizeof( REG_CHANGE ) ; i++ )
    {
        UpdateRegistryItem( &WinlogonSetupChanges[ i ] );
    }
}


VOID
CreateLsaStallEvent(
    VOID
    )

/*++

Routine Description:

    Create the event used by lsa to stall security initialization.

Arguments:

    None.

Return Value:

    None.

--*/

{
    OBJECT_ATTRIBUTES EventAttributes;
    NTSTATUS Status;
    UNICODE_STRING EventName;
    HANDLE EventHandle;

    RtlInitUnicodeString(&EventName,TEXT("\\INSTALLATION_SECURITY_HOLD"));
    InitializeObjectAttributes(&EventAttributes,&EventName,0,0,NULL);

    Status = NtCreateEvent( &EventHandle,
                            0,
                            &EventAttributes,
                            NotificationEvent,
                            FALSE
                          );

    if(NT_SUCCESS(Status)) {
        LsaStallEvent = EventHandle;
    } else {
        DebugLog((DEB_ERROR, "Couldn't create lsa stall event (status = %lx)",Status));
    }
}


DWORD
CheckSetupType (
   VOID
   )
/*++

Routine Description:

    See if the value "SetupType" exists under the Winlogon key in WIN.INI;
    return its value.  Return SETUPTYPE_NONE if not found.

Arguments:

    None.

Return Value:

    SETUPTYPE_xxxx (see SETUPLGN.H).

--*/

{
   DWORD SetupType = SETUPTYPE_NONE ;
   DWORD dwType, dwSize;
   HKEY hKeySetup;


   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, KEYNAME_SETUP, 0,
                     KEY_READ, &hKeySetup) == ERROR_SUCCESS) {

       dwSize = sizeof(SetupType);
       RegQueryValueEx (hKeySetup, VARNAME_SETUPTYPE, NULL,
                        &dwType, (LPBYTE) &SetupType, &dwSize);

       RegCloseKey (hKeySetup);
   }

   return SetupType ;
}

BOOL
SetSetupType (
   DWORD dwType
   )
/*++

Routine Description:

    Set the "SetupType" value in the Registry.

Arguments:

    DWORD type  (see SETUPLGN.H)

Return Value:

    TRUE if operation successful.

--*/

{
   HKEY hKeySetup;
   LONG lResult = ERROR_INVALID_PARAMETER ;


   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, KEYNAME_SETUP, 0,
                     KEY_WRITE, &hKeySetup) == ERROR_SUCCESS) {

       lResult = RegSetValueEx (hKeySetup, VARNAME_SETUPTYPE, 0,
                                REG_DWORD, (LPBYTE) &dwType, sizeof(dwType));

       RegCloseKey (hKeySetup);
   }

   return (lResult == ERROR_SUCCESS) ? TRUE : FALSE;
}



VOID
ExecuteSetup(
    PTERMINAL pTerm
    )

/*++

Routine Description:

    Execute setup.exe.  The command line to be passed to setup is obtained
    from HKEY_LOCAL_MACHINE\system\setup:cmdline.  Wait for setup to complete
    before returning.

Arguments:

    pTerm - terminal data structure

Return Value:

    None.

--*/

{
    TCHAR CmdLineBuffer[1024];
    TCHAR DebugCmdLineBuffer[1024];
    PWCHAR CmdLine;
    USER_PROCESS_DATA UserProcessData;
    PROCESS_INFORMATION ProcessInformation;
    HANDLE hProcess;
    HANDLE hThread;
    ULONG ThreadId;
    HKEY hKey;
    ULONG Result;
    ULONG DataSize;
    ULONG ExitCode;
    MSG Msg;
    USEROBJECTFLAGS uof;
    USHORT LShift ;
    USHORT RShift ;


    //
    // Get the Setup command line from the registry
    //

    FixupRegistry();

    if((Result = RegOpenKey( HKEY_LOCAL_MACHINE,
                              KEYNAME_SETUP,
                              &hKey)) == NO_ERROR) {

        DataSize = sizeof(CmdLineBuffer);

        Result = RegQueryValueEx( hKey,
                                   VARNAME_SETUPCMD,
                                   NULL,
                                   NULL,
                                   (LPBYTE)CmdLineBuffer,
                                   &DataSize
                                 );

        if(Result == NO_ERROR) {
            // DebugLog((DEB_ERROR, "Setup cmd line is '%s'",CmdLineBuffer));
        } else {
            DebugLog((DEB_ERROR, "error %u querying CmdLine value from \\system\\setup",Result));
        }
        RegCloseKey(hKey);

    } else {
        DebugLog((DEB_ERROR, "error %u opening \\system\\setup key for CmdLine (2)",Result));
    }

    //  Alter "SetupType" to indicate setup is no long in progress.

    SetSetupType( SETUPTYPE_NONE ) ;

    //  Delete "AutoAdminLogon" from WIN.INI if present
    //  except in the retail upgrade case.

    if(g_uSetupType != SETUPTYPE_UPGRADE) {
        WriteProfileString( APPNAME_WINLOGON, VARNAME_AUTOLOGON, NULL );
    }



    RtlZeroMemory(&UserProcessData,sizeof(UserProcessData));

    //
    // Make windowstation and desktop handles inheritable.
    //
    GetUserObjectInformation(pTerm->pWinStaWinlogon->hwinsta, UOI_FLAGS, &uof, sizeof(uof), NULL);
    uof.fInherit = TRUE;
    SetUserObjectInformation(pTerm->pWinStaWinlogon->hwinsta, UOI_FLAGS, &uof, sizeof(uof));
    GetUserObjectInformation(pTerm->pWinStaWinlogon->hdeskApplication, UOI_FLAGS, &uof, sizeof(uof), NULL);
    uof.fInherit = TRUE;
    SetUserObjectInformation(pTerm->pWinStaWinlogon->hdeskApplication, UOI_FLAGS, &uof, sizeof(uof));

    SetActiveDesktop(pTerm, Desktop_Application);

    CmdLine = CmdLineBuffer;

    LShift = GetAsyncKeyState( VK_LSHIFT );
    RShift = GetAsyncKeyState( VK_RSHIFT );

    if ( (LShift | RShift) & 0x8000 )
    {
        wsprintf( DebugCmdLineBuffer,
                  TEXT( "ntsd %s %s" ),
                  (RShift & 0x8000) ? TEXT("-d") : TEXT(""),
                  CmdLine );

        CmdLine = DebugCmdLineBuffer;
    }

    if(StartSystemProcess(  CmdLine,
                            TEXT("winsta0\\Default"),
                            0,
                            0, // Normal startup feedback
                            NULL,
                            FALSE,
                            &hProcess,
                            NULL) )
    {
        if (hProcess)
        {


            //
            // Create a second thread to wait on the setup process.
            // When setup terminates, the second thread will send us
            // a special message.  When we receive the special message,
            // exit the dispatch loop.
            //
            // Do this to allow us to respond to messages sent by the
            // system, thus preventing the system from hanging.
            //

            MainThreadId = GetCurrentThreadId();

            hThread = CreateThread( NULL,
                                    0,
                                    WaiterThread,
                                    (LPVOID)hProcess,
                                    0,
                                    &ThreadId
                                  );
            if(hThread) {

                while(GetMessage(&Msg,NULL,0,0)) {
                    DispatchMessage(&Msg);
                }

                CloseHandle(hThread);
                GetExitCodeProcess(hProcess,&ExitCode);

                // BUGBUG look at exit code; may have to restart machine.

            } else {
                DebugLog((DEB_ERROR, "couldn't start waiter thread"));
            }

            CloseHandle(hProcess);

        } else {
            DebugLog((DEB_ERROR, "couldn't get handle to setup process, error = %u",GetLastError()));
        }
    } else {
        DebugLog((DEB_ERROR, "couldn't exec '%ws'",CmdLine));
    }

    SetActiveDesktop(pTerm, Desktop_Winlogon);

}


DWORD
WaiterThread(
    PVOID hProcess
    )
{
    WaitForSingleObject(hProcess,(DWORD)(-1));

    PostThreadMessage(MainThreadId,WM_QUIT,0,0);

    ExitThread(0);

    return(0);      // prevent compiler warning
}



VOID
CheckForIncompleteSetup (
   PTERMINAL pTerm
   )
/*++

Routine Description:

    Checks to see if setup started but never completed.
    Do this by checking to see if the SetupInProgress value has been
    reset to 0. This value is set to 1 in the default hives that run setup.
    Setup.exe resets it to 0 on successful completion.

Arguments:

    None.

Return Value:

    None

--*/

{
   DWORD SetupInProgress = 0 ;
   DWORD dwType, dwSize;
   HKEY hKeySetup;


   if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, KEYNAME_SETUP, 0,
                     KEY_READ, &hKeySetup) == ERROR_SUCCESS) {

       dwSize = sizeof(SetupInProgress);
       RegQueryValueEx (hKeySetup, VARNAME_SETUPINPROGRESS, NULL,
                        &dwType, (LPBYTE) &SetupInProgress, &dwSize);

       RegCloseKey (hKeySetup);
   }


    //
    // If setup did not complete then make them reboot
    //

    if (SetupInProgress) {

        TimeoutMessageBox(pTerm,
                          NULL,
                          IDS_SETUP_INCOMPLETE,
                          IDS_WINDOWS_MESSAGE,
                          MB_ICONSTOP | MB_OK
                         );

#if DBG
        //
        // On debug builds let them continue if they hold down Ctrl
        //

        if ((GetKeyState(VK_LCONTROL) < 0) ||
            (GetKeyState(VK_RCONTROL) < 0)) {

            return;
        }
#endif

        //
        // Reboot time
        //

        RebootMachine(pTerm);

   }
}


//
// This function checks if the "Repair" value is set.  If so, then
// it loads syssetup.dll and calls RepairStartMenuItems
//

VOID
CheckForRepairRequest (void)
{
    HKEY hkeyWinlogon;
    LONG lResult;
    DWORD dwSize, dwType;
    BOOL bRunRepair = FALSE;
    HINSTANCE hSysSetup;
    REPAIRSTARTMENUITEMS RepairStartMenuItems;


    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY,
                      0, KEY_READ | KEY_WRITE, &hkeyWinlogon) == ERROR_SUCCESS) {

        dwSize = sizeof(bRunRepair);

        if (RegQueryValueEx (hkeyWinlogon, L"Repair",
                             NULL, &dwType, (LPBYTE) &bRunRepair,
                             &dwSize) == ERROR_SUCCESS) {

            RegDeleteValue (hkeyWinlogon, L"Repair");
        }

        RegCloseKey (hkeyWinlogon);
    }


    if (bRunRepair) {

        hSysSetup = LoadLibrary (L"syssetup.dll");

        if (hSysSetup) {

            RepairStartMenuItems = (REPAIRSTARTMENUITEMS)GetProcAddress(hSysSetup,
                                                           "RepairStartMenuItems");

            if (RepairStartMenuItems) {
                RepairStartMenuItems();
            }

            FreeLibrary (hSysSetup);
        }
    }

}


//
// Check to see we should run the net access wizard
//

extern HRESULT PAWaitForSamService();

BOOL
CheckForNetAccessWizard (
    PTERMINAL pTerm
    )
{
    BOOL fReboot = FALSE;
    HKEY hkeyWinlogon = NULL;
    LONG lResult;
    DWORD dwSize, dwType;
    ULONG uWizardType = NAW_NETID;          // NAW_NETID is not valid
    HINSTANCE hNetPlWiz;
    LPNETACCESSWIZARD pfnNetAccessWiz;
    DWORD dwWin9xUpgrade = 0;

    //
    // Do we have a post setup wizard entry?  Or a Win9x upgrade entry?
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY,
                      0, KEY_READ | KEY_WRITE, &hkeyWinlogon) == ERROR_SUCCESS) {

        dwSize = sizeof(uWizardType);

        RegQueryValueEx (
            hkeyWinlogon,
            L"RunNetAccessWizard",
            NULL,
            &dwType,
            (LPBYTE) &uWizardType,
            &dwSize
            );

        dwSize = sizeof(dwWin9xUpgrade);

        RegQueryValueEx (
            hkeyWinlogon,
            L"SetWin9xUpgradePasswords",
            NULL,
            &dwType,
            (LPBYTE) &dwWin9xUpgrade,
            &dwSize
            );

    }

    //
    // check the wizard type, if its != NAW_NETID then we assume that we should
    // be starting the wizard for post setup.
    //

    if ( uWizardType != NAW_NETID || dwWin9xUpgrade )
    {
        //
        // Wait on the font loading thread here, so we don't inadvertantly re-enter
        // the stuff in user that gets confused.
        //

        if ( hFontThread )
        {
            WaitForSingleObject( hFontThread, INFINITE );
            CloseHandle( hFontThread );
            hFontThread = NULL;
        }

        //
        // wait for SAM to get its act together...
        //

        PAWaitForSamService();


        //
        // Get rid of the status UI so we don't overlap
        //

        RemoveStatusMessage(TRUE);


        if ( !dwWin9xUpgrade ) {
            //
            // lets load the network account wizard and set it off
            //

            hNetPlWiz = LoadLibrary(L"netplwiz.dll");
            if ( hNetPlWiz )
            {
                pfnNetAccessWiz = (LPNETACCESSWIZARD)GetProcAddress(hNetPlWiz, "NetAccessWizard");
                if ( pfnNetAccessWiz )
                    (*pfnNetAccessWiz)(NULL, uWizardType, &fReboot);

                FreeLibrary(hNetPlWiz);

                //
                // The wizard has completed, remove the value that
                // caused it to run
                //

                RegDeleteValue (hkeyWinlogon, L"RunNetAccessWizard");
            }

        } else {
            //
            // Launch the Win9x upgrade password UI
            //

            SetWin9xUpgradePasswords (pTerm);
        }
    }

    if ( hkeyWinlogon ) {
        RegCloseKey (hkeyWinlogon);
    }

    return fReboot;
}


VOID
SetWin9xUpgradePasswords (
    PTERMINAL pTerm
    )
{
    HKEY Key;
    DWORD Size;
    LONG rc;
    WCHAR CmdLine[MAX_PATH];
    WCHAR SysDir[MAX_PATH];
    BOOL ProcessResult;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    //
    // Sanity test...
    //
    // Verify migpwd is in the Run key as well.  Setup puts it there
    // in all cases.  In an auto stress launched from Win9x, migpwd.exe
    // is not started in winlogon.
    //

    rc = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_READ,
            &Key
            );

    if (rc == ERROR_SUCCESS) {
        rc = RegQueryValueEx (
                Key,
                L"MigPwd",
                NULL,
                NULL,
                NULL,
                &Size
                );

        if (rc == ERROR_MORE_DATA) {
            rc = ERROR_SUCCESS;
        }

        RegCloseKey (Key);
    }

    if (rc == ERROR_SUCCESS) {
        //
        // Launch migpwd.exe
        //

        WlxSetTimeout (pTerm, TIMEOUT_NONE);

        ZeroMemory (&si, sizeof (si));
        si.cb = sizeof (si);
        si.dwFlags = STARTF_FORCEOFFFEEDBACK;
        si.lpDesktop = TEXT("Winsta0\\") WINLOGON_DESKTOP_NAME;

        GetSystemDirectory (SysDir, MAX_PATH);
        lstrcpy (CmdLine, SysDir);
        lstrcat (CmdLine, L"\\migpwd.exe");

        ProcessResult = CreateProcess (
                            CmdLine,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_DEFAULT_ERROR_MODE,
                            NULL,
                            SysDir,
                            &si,
                            &pi
                            );

        if (ProcessResult) {

            //
            // Wait for the process to complete.  The machine stays
            // awake until it is done.
            //

            CloseHandle (pi.hThread);

            rc = WaitForSingleObject (pi.hProcess, INFINITE);
            if (rc != WAIT_OBJECT_0) {
                TerminateProcess (pi.hProcess, 0);
            }

            CloseHandle (pi.hProcess);

            //
            // Process must be successful before it will terminate.
            // Now remove the value that caused it to run.
            //

            rc = RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    WINLOGON_KEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &Key
                    );

            if (rc == ERROR_SUCCESS) {
                RegDeleteValue (Key, L"SetWin9xUpgradePasswords");
                RegCloseKey (Key);
            }

            //
            // Remove migpwd.exe from the system, it is not needed anymore.
            //

            DeleteFile (CmdLine);

            WlxSetTimeout (pTerm, 120);
        }
    }
}

