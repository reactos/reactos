/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    worldmap.c

Abstract:

    This module implements the world map for the Date/Time applet.

Revision History:

--*/



//
//  Include Files.
//

#ifdef WINNT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include "timedate.h"
#include "rc.h"
#include <cpl.h>




//
//  Global Variables.
//

HINSTANCE g_hInst = NULL;




//
//  Function Prototypes.
//

extern BOOL OpenDateTimePropertySheet(HWND hwnd, LPCTSTR cmdline);

BOOL
EnableTimePrivilege(
    PTOKEN_PRIVILEGES *pPreviousState,
    ULONG *pPreviousStateLength);

BOOL
ResetTimePrivilege(
    PTOKEN_PRIVILEGES PreviousState,
    ULONG PreviousStateLength);

int DoMessageBox(
    HWND hWnd,
    DWORD wText,
    DWORD wCaption,
    DWORD wType);





////////////////////////////////////////////////////////////////////////////
//
//  LibMain
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI LibMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            g_hInst = hDll;
            DisableThreadLibraryCalls(hDll);
            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            break;
        }
        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }
        case ( DLL_THREAD_ATTACH ) :
        default :
        {
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CplApplet
//
//  The main applet information manager.
//
////////////////////////////////////////////////////////////////////////////

LONG WINAPI CPlApplet(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    static BOOL fReEntered = FALSE;

    switch (uMsg)
    {
        case ( CPL_INIT ) :
        {
            return (TRUE);
        }
        case ( CPL_GETCOUNT ) :
        {
            //
            //  How many applets are in this DLL?
            //
            return (1);
        }
        case ( CPL_INQUIRE ) :
        {
            //
            //  Fill the CPLINFO with the pertinent information.
            //
            #define lpOldCPlInfo ((LPCPLINFO)lParam2)

            switch (lParam1)
            {
                case ( 0 ) :
                {
                    lpOldCPlInfo->idIcon = IDI_TIMEDATE;
                    lpOldCPlInfo->idName = IDS_TIMEDATE;
                    lpOldCPlInfo->idInfo = IDS_TIMEDATEINFO;
                    break;
                }
            }

            lpOldCPlInfo->lData = 0L;
            return (TRUE);
        }
        case ( CPL_NEWINQUIRE ) :
        {
            #define lpCPlInfo ((LPNEWCPLINFO)lParam2)

            switch (lParam1)
            {
                case ( 0 ) :
                {
                    lpCPlInfo->hIcon = LoadIcon( g_hInst,
                                                 MAKEINTRESOURCE(IDI_TIMEDATE) );
                    LoadString( g_hInst,
                                IDS_TIMEDATE,
                                lpCPlInfo->szName,
                                sizeof(lpCPlInfo->szName) );
                    LoadString( g_hInst,
                                IDS_TIMEDATEINFO,
                                lpCPlInfo->szInfo,
                                sizeof(lpCPlInfo->szInfo) );
                    lpCPlInfo->dwHelpContext = 0;
                    break;
                }
            }

            lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpCPlInfo->lData = 0L;
            lpCPlInfo->szHelpFile[0] = 0;
            return (TRUE);
        }
        case ( CPL_DBLCLK ) :
        {
            lParam2 = (LPARAM)0;

            // fall thru...
        }
        case ( CPL_STARTWPARMS ) :
        {
            //
            //  Do the applet thing.
            //
            switch (lParam1)
            {
                case ( 0 ) :
                {
                    PTOKEN_PRIVILEGES PreviousState;
                    ULONG PreviousStateLength;

                    if (EnableTimePrivilege(&PreviousState, &PreviousStateLength))
                    {
                        OpenDateTimePropertySheet(hwnd, (LPCTSTR)lParam2);
                        ResetTimePrivilege(PreviousState, PreviousStateLength);
                    }
                    else
                    {
                        DoMessageBox( hwnd,
                                      IDS_NOTIMEERROR,
                                      IDS_CAPTION,
                                      MB_OK | MB_ICONINFORMATION );
                    }
                    break;
                }
            }
            break;
        }
        case ( CPL_EXIT ) :
        {
            fReEntered = FALSE;

            //
            //  Free up any allocations of resources made.
            //

            break;
        }
        default :
        {
            return (0L);
        }
    }

    return (1L);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnableTimePrivilege
//
////////////////////////////////////////////////////////////////////////////

BOOL EnableTimePrivilege(
    PTOKEN_PRIVILEGES *pPreviousState,
    ULONG *pPreviousStateLength)
{
#ifndef WINNT
    return TRUE;
#else    
    NTSTATUS NtStatus;
    HANDLE Token;
    LUID SystemTimePrivilege;
    PTOKEN_PRIVILEGES NewState;

    //
    //  Open our own token.
    //
    NtStatus = NtOpenProcessToken( NtCurrentProcess(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   &Token );
    if (!NT_SUCCESS(NtStatus))
    {
        return (FALSE);
    }

    //
    //  Initialize the adjustment structure.
    //
    SystemTimePrivilege = RtlConvertLongToLuid(SE_SYSTEMTIME_PRIVILEGE);

    NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, 100);
    if (NewState == NULL)
    {
        return (FALSE);
    }

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = SystemTimePrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    *pPreviousState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, 100);
    if (*pPreviousState == NULL)
    {
        LocalFree(NewState);
        return (FALSE);
    }

    //
    //  Set the state of the privilege to ENABLED.
    //
    NtStatus = NtAdjustPrivilegesToken( Token,                  // TokenHandle
                                        FALSE,                  // DisableAllPrivileges
                                        NewState,               // NewState
                                        100,                    // BufferLength
                                        *pPreviousState,        // PreviousState (OPTIONAL)
                                        pPreviousStateLength ); // ReturnLength

    //
    //  Clean up some stuff before returning.
    //
    LocalFree(NewState);

    if (NtStatus == STATUS_SUCCESS)
    {
        NtClose(Token);
        return (TRUE);
    }
    else
    {
        LocalFree(*pPreviousState);
        NtClose(Token);
        return (FALSE);
    }
#endif //WINNT
}


////////////////////////////////////////////////////////////////////////////
//
//  ResetTimePrivilege
//
//  Restore previous privilege state for setting system time.
//
////////////////////////////////////////////////////////////////////////////

BOOL ResetTimePrivilege(
    PTOKEN_PRIVILEGES PreviousState,
    ULONG PreviousStateLength)
{
#ifndef WINNT
    return TRUE;
#else
    NTSTATUS NtStatus;
    HANDLE Token;
    LUID SystemTimePrivilege;
    ULONG ReturnLength;

    if (PreviousState == NULL)
    {
        return (FALSE);
    }

    //
    //  Open our own token.
    //
    NtStatus = NtOpenProcessToken( NtCurrentProcess(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   &Token);
    if (!NT_SUCCESS(NtStatus))
    {
        return (FALSE);
    }

    //
    //  Initialize the adjustment structure.
    //
    SystemTimePrivilege = RtlConvertLongToLuid(SE_SYSTEMTIME_PRIVILEGE);

    //
    //  Restore previous state of the privilege.
    //
    NtStatus = NtAdjustPrivilegesToken( Token,               // TokenHandle
                                        FALSE,               // DisableAllPrivileges
                                        PreviousState,       // NewState
                                        PreviousStateLength, // BufferLength
                                        NULL,                // PreviousState (OPTIONAL)
                                        &ReturnLength );     // ReturnLength

    //
    //  Clean up some stuff before returning.
    //
    LocalFree(PreviousState);
    NtClose(Token);

    return (NT_SUCCESS(NtStatus));
#endif //WINNT
}


////////////////////////////////////////////////////////////////////////////
//
//  DoMessageBox
//
////////////////////////////////////////////////////////////////////////////

int DoMessageBox(
    HWND hWnd,
    DWORD wText,
    DWORD wCaption,
    DWORD wType)
{
    TCHAR szText[2 * MAX_PATH];
    TCHAR szCaption[MAX_PATH];

    if (!LoadString(g_hInst, wText, szText, CharSizeOf(szText)))
    {
        return (0);
    }

    if (!LoadString(g_hInst, wCaption, szCaption, CharSizeOf(szCaption)))
    {
        return (0);
    }

    return ( MessageBox(hWnd, szText, szCaption, wType) );
}
