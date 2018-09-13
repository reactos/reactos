//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wlxutil.c
//
//  Contents:   WLX helper functions
//
//  Classes:
//
//  Functions:
//
//  History:    8-24-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

extern DWORD g_dwApplicationDSSize;
extern BOOL ReturnFromPowerState ;
BOOL g_bVerboseStatus = FALSE;

#if DBG
extern char * SASTypes[] ;

#define SASName(x)  (x <= WLX_SAS_TYPE_SC_REMOVE ? SASTypes[x] : "User Defined")
char * StateNames[] = {"Preload", "Initialize", "NoOne", "NoOne_Display",
                        "NoOne_SAS", "LoggedOnUser_StartShell", "LoggedOnUser",
                        "LoggedOn_SAS", "Locked", "Locked_Display", "Locked_SAS",
                        "WaitForLogoff", "WaitForShutdown", "Shutdown" };
#endif


WLX_DISPATCH_VERSION_1_3 WlxDispatchTable = {
    WlxUseCtrlAltDel,
    WlxSetContextPointer,
    WlxSasNotify,
    WlxSetTimeout,
    WlxAssignShellProtection,
    WlxMessageBox,
    WlxDialogBox,
    WlxDialogBoxParam,
    WlxDialogBoxIndirect,
    WlxDialogBoxIndirectParam,
    WlxSwitchDesktopToUser,
    WlxSwitchDesktopToWinlogon,
    WlxChangePasswordNotify,
    WlxGetSourceDesktop,
    WlxSetReturnDesktop,
    WlxCreateUserDesktop,
    WlxChangePasswordNotifyEx,
    WlxCloseUserDesktop,
    WlxSetOption,
    WlxGetOption,
    WlxWin31Migrate,
    WlxQueryClientCredentials,
    WlxQueryInetConnectorCredentials,
    WlxDisconnect,
    WlxQueryTerminalServicesData
};



typedef struct _WindowMapperTerminal {
        PTERMINAL pTerm;
        PWindowMapper pMap;
} WindowMapperTerminal, *PWindowMapperTerminal;

#define MAPPERFLAG_ACTIVE       1
#define MAPPERFLAG_DIALOG       2
#define MAPPERFLAG_SAS          4
#define MAPPERFLAG_WINLOGON     8



PWindowMapper
LocateTopMappedWindow(PTERMINAL pTerm)
{
    int i;
    for (i = 0; i < MAX_WINDOW_MAPPERS ; i++ )
    {
        if (pTerm->Mappers[i].fMapper & MAPPERFLAG_SAS)
        {
            return(&pTerm->Mappers[i]);
        }
    }

    return(NULL);

}


PWindowMapper
AllocWindowMapper(PTERMINAL pTerm)
{
    int i;
    PWindowMapper   pMap;

    for (i = 0 ; i < MAX_WINDOW_MAPPERS ; i++ )
    {
        if ((pTerm->Mappers[i].fMapper & MAPPERFLAG_ACTIVE) == 0)
        {
            pTerm->cActiveWindow ++;
            pMap = LocateTopMappedWindow(pTerm);
            if (pMap)
            {
                FLAG_OFF(pMap->fMapper, MAPPERFLAG_SAS);
            }

            pTerm->Mappers[i].hWnd = NULL;
            FLAG_ON(pTerm->Mappers[i].fMapper, MAPPERFLAG_ACTIVE | MAPPERFLAG_SAS);
            pTerm->Mappers[i].pPrev = pMap;

            return(&pTerm->Mappers[i]);
        }
    }
    return(NULL);
}


PWindowMapper
LocateWindowMapper(HWND hWnd, PTERMINAL pTerm)
{
    int i;

    for (i = 0; i < MAX_WINDOW_MAPPERS ; i++ )
    {
        if (pTerm->Mappers[i].hWnd == hWnd)
        {
            return(&pTerm->Mappers[i]);
        }
    }

    return(NULL);
}


void
FreeWindowMapper(PWindowMapper  pMap, PTERMINAL pTerm)
{
    pMap->hWnd = NULL;
    pMap->DlgProc = NULL;
    if (pMap->fMapper & MAPPERFLAG_SAS)
    {
        if (pMap->pPrev)
        {
            FLAG_ON(pMap->pPrev->fMapper, MAPPERFLAG_SAS);
        }
    }
    pMap->fMapper = 0;
    pMap->pPrev = NULL;
    pTerm->cActiveWindow--;
}


HWND
LocateTopWindow(PTERMINAL pTerm)
{
    PWindowMapper   pMap;

    pMap = LocateTopMappedWindow(pTerm);
    if (pMap)
    {
        return(pMap->hWnd);
    }
    return(NULL);
}


BOOL
SetMapperFlag(
    HWND    hWnd,
    DWORD   Flag,
    PTERMINAL pTerm
    )
{
    PWindowMapper   pMap;

    ASSERT(pTerm);
    if (!pTerm)
        return FALSE;

    pMap = LocateWindowMapper(hWnd, pTerm);
    if (!pMap)
    {
        return(FALSE);
    }

    pMap->fMapper |= Flag;

    return(TRUE);

}


BOOL
QueueSasEvent(
    DWORD     dwSasType,
    PTERMINAL pTerm)
{
    if (((pTerm->PendingSasTail + 1) % MAX_WINDOW_MAPPERS) == pTerm->PendingSasHead)
    {
        return(FALSE);
    }

    pTerm->PendingSasEvents[pTerm->PendingSasTail] = dwSasType;
    pTerm->PendingSasTail ++;
    pTerm->PendingSasTail %= MAX_WINDOW_MAPPERS;

    return(TRUE);
}


BOOL
FetchPendingSas(
    PDWORD  pSasType,
    PTERMINAL pTerm)
{
    if (pTerm->PendingSasHead == pTerm->PendingSasTail)
    {
        return(FALSE);
    }
    *pSasType = pTerm->PendingSasEvents[pTerm->PendingSasHead++];
    pTerm->PendingSasHead %= MAX_WINDOW_MAPPERS;
    return(TRUE);
}


VOID
EnableSasMessages(HWND  hWnd, PTERMINAL pTerm)
{
    DWORD   SasType;
    SasMessages = TRUE;

    ASSERT(pTerm);
    if (!pTerm)
        return;

    while (FetchPendingSas(&SasType, pTerm))
    {
        if (hWnd)
        {
#if DBG
            DebugLog((DEB_TRACE, "Posting queued Sas %d to window %x\n",
                        SasType, hWnd ));
#endif

            PostMessage(hWnd, WLX_WM_SAS, (WPARAM) SasType, 0);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   RootWndProc
//
//  Synopsis:   This is the base window proc for all testgina windows.
//
//  Arguments:  [hWnd]    --
//              [Message] --
//              [wParam]  --
//              [lParam]  --
//
//  History:    7-18-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INT_PTR
CALLBACK
RootDlgProc(
    HWND    hWnd,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    PWindowMapper   pMap;
    PWindowMapperTerminal  pMapTerminal;
    int res;
    INT_PTR bRet;
    PTERMINAL pTerm;

    //
    // If this is a WM_INITDIALOG message, then the parameter is the mapping,
    // which needs to have a hwnd associated with it.  Otherwise, do the normal
    // preprocessing.
    //
    if (Message == WM_INITDIALOG)
    {
        pMapTerminal = (PWindowMapperTerminal) lParam;
        pMap = pMapTerminal->pMap;
        pTerm = pMapTerminal->pTerm;

        // Save the pTerm in the Dialog
        SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)pTerm);

        pMap->hWnd = hWnd;
        SetTopTimeout(hWnd);
        lParam = pMap->InitialParameter;
        //
        // Now that everything is done, enable sas messages again.  This
        // protects us from people pounding on the c-a-d keys, when our response
        // time is slow, e.g. due to stress.  We also drain the queue of pending
        // SAS events.
        //
        EnableSasMessages(hWnd, pTerm);
    }
    else
    {
        pTerm = (PTERMINAL)GetWindowLongPtr(hWnd, DWLP_USER);
        if ( !pTerm ) {
            return FALSE;
        }

        pMap = LocateWindowMapper(hWnd, pTerm);
        if (!pMap)
        {
            return(FALSE);
        }
    }

    if (Message == WLX_WM_SAS &&
        ((pMap->fMapper & MAPPERFLAG_WINLOGON) == 0))
    {
        if ((wParam == WLX_SAS_TYPE_TIMEOUT) ||
            (wParam == WLX_SAS_TYPE_SCRNSVR_TIMEOUT) )
        {
            DebugLog((DEB_TRACE, "Sending timeout to top window.\n"));
        }
    }

    bRet = pMap->DlgProc(hWnd, Message, wParam, lParam);
    if (!bRet)
    {
        if (Message == WM_INITDIALOG)
        {
            return(bRet);
        }
        if (Message == WLX_WM_SAS)
        {
            if ((pMap->fMapper & MAPPERFLAG_WINLOGON) == 0)
            {
                //
                // Re-enable the messages
                EnableSasMessages(pMap->hWnd, pTerm);

            }
            switch (wParam)
            {
                case WLX_SAS_TYPE_CTRL_ALT_DEL:
                default:
                    res = WLX_DLG_SAS;
                    break;

                case WLX_SAS_TYPE_TIMEOUT:
                    res = WLX_DLG_INPUT_TIMEOUT;
                    break;
                case WLX_SAS_TYPE_SCRNSVR_TIMEOUT:
                    res = WLX_DLG_SCREEN_SAVER_TIMEOUT;
                    break;
                case WLX_SAS_TYPE_USER_LOGOFF:
                    res = WLX_DLG_USER_LOGOFF;
                    break;
            }
            if (res)
            {
                EndDialog(hWnd, res);
                bRet = TRUE;
            }
        }
    }
    else
    {
        if (Message == WLX_WM_SAS &&
            ((pMap->fMapper & MAPPERFLAG_WINLOGON) == 0))
        {
            //
            // Re-enable the messages
            //
            EnableSasMessages(pMap->hWnd, pTerm);

            switch (wParam)
            {
                case WLX_SAS_TYPE_TIMEOUT:
                    res = WLX_DLG_INPUT_TIMEOUT;
                    break;

                case WLX_SAS_TYPE_SCRNSVR_TIMEOUT:
                    res = WLX_DLG_SCREEN_SAVER_TIMEOUT;
                    break;

                case WLX_SAS_TYPE_USER_LOGOFF:
                    res = WLX_DLG_USER_LOGOFF;
                    break;

                default:
                    res = 0;
                    break;
            }

            if (res)
            {
                DebugLog((DEB_TRACE, "Gina ate the SAS (%d) message, but ending it anyway.\n", wParam));
                EndDialog(hWnd, res);
            }

        }
    }

    return(bRet);

}

VOID
ChangeStateForSAS(PTERMINAL  pTerm)
{
#if DBG
    WinstaState State = pTerm->WinlogonState;
#endif
    switch (pTerm->WinlogonState)
    {
        case Winsta_NoOne:
        case Winsta_NoOne_Display:
            pTerm->WinlogonState = Winsta_NoOne_SAS;
            break;

        case Winsta_Locked:
        case Winsta_Locked_Display:
            pTerm->WinlogonState = Winsta_Locked_SAS;
            break;

        case Winsta_LoggedOnUser:
        case Winsta_LoggedOnUser_StartShell:
            pTerm->WinlogonState = Winsta_LoggedOn_SAS;
            break;

        case Winsta_WaitForLogoff:
        case Winsta_WaitForShutdown:
            break;

        default:
            DebugLog((DEB_ERROR, "Don't know how to get to next state from %d, %s\n",
                    pTerm->WinlogonState, GetState(pTerm->WinlogonState)));
    }
#if DBG
    DebugLog((DEB_TRACE, "ChangeStateForSAS: Went from %d (%s) to %d (%s)\n",
                    State, GetState(State), pTerm->WinlogonState,
                    GetState(pTerm->WinlogonState) ));
#endif
}


BOOL
SendSasToTopWindow(
    PTERMINAL   pTerm,
    DWORD       SasType)
{
    PWindowMapper   pMap;
#if DBG
    WCHAR           WindowName[32];
#endif

    if (pTerm->cActiveWindow)
    {
        pMap = LocateTopMappedWindow(pTerm);

        if (!pMap)
        {
            return(FALSE);
        }

        if ( ( SasType > WLX_SAS_TYPE_MAX_MSFT_VALUE) ||
             ( SasType == WLX_SAS_TYPE_SCRNSVR_TIMEOUT) ||
             ( SasType == WLX_SAS_TYPE_TIMEOUT) )
        {
            //
            // Either a timeout (which we have to forward), or a private
            // which we have to forward.  Kill any message boxes
            //
            if (KillMessageBox( SasType ))
            {
                DebugLog((DEB_TRACE, "Killed a pending message box\n"));
            }
        }

#if DBG
        GetWindowText( pMap->hWnd, WindowName, 32 );
        DebugLog((DEB_TRACE, "Sending SAS code %d (%s) to window %x (%ws) \n",
            SasType, SASName( SasType ), pMap->hWnd, WindowName ));

#endif


        PostMessage(pMap->hWnd, WLX_WM_SAS, (WPARAM) SasType, 0);

        //
        // This will cause them to be queued, and then handled later.
        //
        if ( !ReturnFromPowerState )
        {
            DisableSasMessages();
        }

        return(TRUE);
    }
    else
    {
        //
        // No windows active.  See if there is a single message box going currently,
        // like maybe a legal notice box.  If there is, kill it for any SAS
        //

        if ( pTerm->MessageBoxActive )
        {
            KillMessageBox( SasType );
        }
    }

    return(FALSE);
}


VOID
DestroyMprInfo(
    PWLX_MPR_NOTIFY_INFO    pMprInfo)
{
    if (pMprInfo->pszUserName)
    {
        LocalFree(pMprInfo->pszUserName);
    }

    if (pMprInfo->pszDomain)
    {
        LocalFree(pMprInfo->pszDomain);
    }

    if (pMprInfo->pszPassword)
    {
        ZeroMemory(pMprInfo->pszPassword, wcslen(pMprInfo->pszPassword) * 2);
        LocalFree(pMprInfo->pszPassword);
    }

    if (pMprInfo->pszOldPassword)
    {
        ZeroMemory(pMprInfo->pszOldPassword, wcslen(pMprInfo->pszOldPassword) * 2);
        LocalFree(pMprInfo->pszOldPassword);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxUseCtrlAltDel
//
//  Synopsis:   Enable CAD events to the GINA.  Obsoleted by WlxSetOption
//
//  Arguments:  [hWlx] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxUseCtrlAltDel(
    HANDLE  hWlx)
{

    WlxSetOption( hWlx,
                  WLX_OPTION_USE_CTRL_ALT_DEL,
                  TRUE,
                  NULL );

}


//+---------------------------------------------------------------------------
//
//  Function:   WlxSetContextPointer
//
//  Synopsis:   Set the pointer used in calls to this GINA.  Obsoleted by
//              WlxSetOption
//
//  Arguments:  [hWlx]        --
//              [pWlxContext] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxSetContextPointer(
    HANDLE  hWlx,
    PVOID   pWlxContext
    )
{
    WlxSetOption( hWlx,
                  WLX_OPTION_CONTEXT_POINTER,
                  (ULONG_PTR) pWlxContext,
                  NULL );

}


//+---------------------------------------------------------------------------
//
//  Function:   WlxSasNotify
//
//  Synopsis:   Allows a GINA to raise a SAS event.
//
//  Arguments:  [hWlx]    --
//              [SasType] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
WINAPI
WlxSasNotify(
    HANDLE  hWlx,
    DWORD   SasType
    )
{
    PTERMINAL    pTerm;

    if (pTerm = VerifyHandle(hWlx))
    {
        switch (SasType)
        {
            case WLX_SAS_TYPE_USER_LOGOFF:
            case WLX_SAS_TYPE_TIMEOUT:
            case WLX_SAS_TYPE_SCRNSVR_TIMEOUT:
                DebugLog((DEB_ERROR, "Illegal SAS Type (%d) passed to WlxSasNotify\n", SasType ));
                return;

            default:
                SASRouter(pTerm, SasType);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxSetTimeout
//
//  Synopsis:   Sets the current timeout for dialog windows
//
//  Arguments:  [hWlx]    --
//              [Timeout] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxSetTimeout(
    HANDLE  hWlx,
    DWORD   Timeout
    )
{
    PTERMINAL    pTerm;

    if (pTerm = VerifyHandle(hWlx))
    {
        if ((pTerm->WinlogonState == Winsta_NoOne_Display) ||
            (pTerm->WinlogonState == Winsta_Locked_Display) )
        {
            if (Timeout)
            {
                SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_ERROR);
                return(FALSE);
            }
        }
        pTerm->Gina.cTimeout = Timeout;
        TimeoutUpdateTopTimeout( Timeout );
        return(TRUE);
    }

    SetLastErrorEx(ERROR_INVALID_HANDLE, SLE_ERROR);
    return(FALSE);

}


//+---------------------------------------------------------------------------
//
//  Function:   WlxAssignShellProtection
//
//  Synopsis:   Allows a GINA to establish the correct shell protection for
//              a process.  Superceded by the Win32 API, CreateProcessAsUser.
//
//  Arguments:  [hWlx]     --
//              [hToken]   --
//              [hProcess] --
//              [hThread]  --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
WINAPI
WlxAssignShellProtection(
    HANDLE  hWlx,
    HANDLE  hToken,
    HANDLE  hProcess,
    HANDLE  hThread
    )
{
    PTERMINAL               pTerm;
    PTOKEN_DEFAULT_DACL     pDefDacl;
    DWORD                   cDefDacl = 0;
    NTSTATUS                Status;
    PSECURITY_DESCRIPTOR    psd;
    unsigned char           buf[SECURITY_DESCRIPTOR_MIN_LENGTH];
    BOOL                    Success;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        return(ERROR_INVALID_HANDLE);
    }

    Status = NtQueryInformationToken(hToken, TokenDefaultDacl, NULL, 0, &cDefDacl);
    if (!NT_SUCCESS(Status) && ( Status != STATUS_BUFFER_TOO_SMALL ))
    {
        return(RtlNtStatusToDosError(Status));
    }


    pDefDacl = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cDefDacl);
    if (!pDefDacl)
    {
        return(ERROR_OUTOFMEMORY);
    }

    Status = NtQueryInformationToken(hToken, TokenDefaultDacl,
                                    pDefDacl, cDefDacl, &cDefDacl);
    if (!NT_SUCCESS(Status))
    {
        LocalFree(pDefDacl);
        return(RtlNtStatusToDosError(Status));
    }

    psd = (PSECURITY_DESCRIPTOR) buf;
    InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(psd, TRUE, pDefDacl->DefaultDacl, FALSE);

    Success = SetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, psd);

    LocalFree(pDefDacl);

    if (Success)
    {
        if (SetProcessToken(pTerm, hProcess, hThread, hToken))
            return(0);
    }

    return(GetLastError());
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxMessageBox
//
//  Synopsis:   Allows a GINA to raise a message box.
//
//  Arguments:  [hWlx]  --
//              [hWnd]  --
//              [lpsz1] --
//              [lpsz2] --
//              [fmb]   --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI
WlxMessageBox(
    HANDLE      hWlx,
    HWND        hWnd,
    LPWSTR      lpsz1,
    LPWSTR      lpsz2,
    UINT        fmb)
{
    PTERMINAL    pTerm;
    int ret ;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastErrorEx(ERROR_INVALID_HANDLE, SLE_ERROR);
        return(-1);
    }

    pTerm->MessageBoxActive = TRUE ;

    ret = TimeoutMessageBoxlpstr(pTerm, hWnd, lpsz1, lpsz2, fmb,
                                pTerm->Gina.cTimeout | TIMEOUT_SS_NOTIFY );

    pTerm->MessageBoxActive = FALSE ;

    return ret;
}


int WINAPI
WlxDialogBox(
    HANDLE      hWlx,
    HANDLE      hInstance,
    LPWSTR      lpsz1,
    HWND        hWnd,
    DLGPROC     dlgproc)
{
    return(WlxDialogBoxParam(hWlx, hInstance, lpsz1, hWnd, dlgproc, 0));
}


int WINAPI
WlxDialogBoxIndirect(
    HANDLE          hWlx,
    HANDLE          hInstance,
    LPCDLGTEMPLATE  lpTemplate,
    HWND            hWnd,
    DLGPROC         dlgproc)
{
    return(WlxDialogBoxIndirectParam(hWlx, hInstance, lpTemplate, hWnd, dlgproc, 0));
}


int WINAPI
WlxDialogBoxParam(
    HANDLE          hWlx,
    HANDLE          hInstance,
    LPWSTR          lpsz1,
    HWND            hWnd,
    DLGPROC         dlgproc,
    LPARAM          lParam)
{
    PWindowMapper   pMap;
    PTERMINAL       pTerm;
    int res;
    WindowMapperTerminal  MapTerminal;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastErrorEx(ERROR_INVALID_HANDLE, SLE_ERROR);
        return(-1);
    }

    pMap = AllocWindowMapper(pTerm);
    if (!pMap)
    {
        ASSERTMSG("Too many nested windows?  send mail to richardw", pMap);
        DebugLog((DEB_ERROR, "Too many nested windows?!?\n"));
        SetLastError(ERROR_OUTOFMEMORY);
        return(-1);
    }

    pMap->InitialParameter = lParam;
    pMap->DlgProc = dlgproc;
    pMap->fMapper |= MAPPERFLAG_DIALOG;

    MapTerminal.pMap = pMap;
    MapTerminal.pTerm = pTerm;
    //res = DialogBoxParam(hInstance, lpsz1, hWnd, RootDlgProc, (LPARAM) pMap);
    res = TimeoutDialogBoxParam(pTerm, hInstance, lpsz1, hWnd,
                            RootDlgProc, (LPARAM) &MapTerminal,
                            pTerm->Gina.cTimeout | TIMEOUT_SS_NOTIFY);

    FreeWindowMapper(pMap, pTerm);

    return(res);
}


int WINAPI
WlxDialogBoxIndirectParam(
    HANDLE          hWlx,
    HANDLE  hInstance,
    LPCDLGTEMPLATE  lpTemplate,
    HWND    hWnd,
    DLGPROC dlgproc,
    LPARAM  lParam)
{
    PWindowMapper   pMap;
    int res;
    PTERMINAL        pTerm;
    WindowMapperTerminal  MapTerminal;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastErrorEx(ERROR_INVALID_HANDLE, SLE_ERROR);
        return(-1);
    }

    pMap = AllocWindowMapper(pTerm);
    if (!pMap)
    {
        ASSERTMSG("Too many nested windows?  send mail to richardw", pMap);
        DebugLog((DEB_ERROR, "Too many nested windows?!?\n"));
        SetLastError(ERROR_OUTOFMEMORY);
        return(-1);
    }

    pMap->InitialParameter = lParam;
    pMap->DlgProc = dlgproc;
    pMap->fMapper |= MAPPERFLAG_DIALOG;

    MapTerminal.pMap = pMap;
    MapTerminal.pTerm = pTerm;
    res = (int)DialogBoxIndirectParam(hInstance, lpTemplate, hWnd, RootDlgProc, (LPARAM) &MapTerminal);

    FreeWindowMapper(pMap, pTerm);

    return(res);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxSwitchDesktopToUser
//
//  Synopsis:   Allows a GINA to arbitrarily switch the active desktop from
//              winlogon to previous.
//
//  Arguments:  [hWlx] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI
WlxSwitchDesktopToUser(
    HANDLE          hWlx)
{
    PTERMINAL pTerm;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        return(ERROR_INVALID_HANDLE);
    }

    SetActiveDesktop(pTerm, Desktop_Application);
    SetThreadDesktop(pTerm->pWinStaWinlogon->hdeskApplication);

    return(0);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxSwitchDesktopToWinlogon
//
//  Synopsis:   Allows the GINA to switch the desktop from default to winlogon
//
//  Arguments:  [hWlx] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI
WlxSwitchDesktopToWinlogon(
    HANDLE          hWlx)
{
    PTERMINAL    pTerm;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        return(ERROR_INVALID_HANDLE);
    }

    SetActiveDesktop(pTerm, Desktop_Winlogon);
    SetThreadDesktop(pTerm->pWinStaWinlogon->hdeskWinlogon);

    return(0);
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxChangePasswordNotify
//
//  Synopsis:   Notify of a changed password.  Obsoleted by
//              WlxChangePasswordNotifyEx
//
//  Arguments:  [hWlx]         --
//              [pMprInfo]     --
//              [dwChangeInfo] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI
WlxChangePasswordNotify(
    HANDLE                  hWlx,
    PWLX_MPR_NOTIFY_INFO    pMprInfo,
    DWORD                   dwChangeInfo)
{
    PTERMINAL    pTerm;
    int         Result;

    return WlxChangePasswordNotifyEx( hWlx, pMprInfo, dwChangeInfo, NULL, NULL );
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxChangePasswordNotifyEx
//
//  Synopsis:   Allows a GINA to notify all or one network provider of a
//              password change.
//
//  Arguments:  [hWlx]         --
//              [pMprInfo]     --
//              [dwChangeInfo] --
//              [pszProvider]  --
//              [pvReserved]   --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int WINAPI
WlxChangePasswordNotifyEx(
    HANDLE                  hWlx,
    PWLX_MPR_NOTIFY_INFO    pMprInfo,
    DWORD                   dwChangeInfo,
    PWSTR                   pszProvider,
    PVOID                   pvReserved)
{
    PTERMINAL    pTerm;
    int         Result;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        return(ERROR_INVALID_HANDLE);
    }

    Result = MprChangePasswordNotify(
                pTerm,
                LocateTopWindow(pTerm),
                pszProvider,
                pMprInfo->pszUserName,
                pMprInfo->pszDomain,
                pMprInfo->pszPassword,
                pMprInfo->pszOldPassword,
                dwChangeInfo,
                FALSE);

    DestroyMprInfo(pMprInfo);

    if (Result == DLG_SUCCESS)
    {
        return(0);
    }
    else
        return(ERROR_INVALID_PARAMETER);

}


//+---------------------------------------------------------------------------
//
//  Function:   WlxGetSourceDesktop
//
//  Synopsis:   Returns a WLX_DESKTOP describing the originating desktop
//
//  Arguments:  [hWlx]      --
//              [ppDesktop] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxGetSourceDesktop(
    HANDLE              hWlx,
    PWLX_DESKTOP *      ppDesktop)
{
    PTERMINAL       pTerm;
    DWORD           len;
    PWLX_DESKTOP    pDesktop;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return( FALSE );
    }

    if (pTerm->pszDesktop)
    {
        len = (wcslen(pTerm->pszDesktop) + 1) * sizeof(WCHAR);
    }
    else
    {
        len = 0;
    }

    pDesktop = LocalAlloc( LMEM_FIXED, sizeof(WLX_DESKTOP) + len );

    if (!pDesktop)
    {
        return( FALSE );
    }

    pDesktop->Size = sizeof(WLX_DESKTOP);
    pDesktop->Flags = WLX_DESKTOP_NAME;
    pDesktop->hDesktop = NULL;
    pDesktop->pszDesktopName = (PWSTR) (pDesktop + 1);
    if (len)
    {
        wcscpy( pDesktop->pszDesktopName, pTerm->pszDesktop );
    }

    *ppDesktop = pDesktop;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   WlxSetReturnDesktop
//
//  Synopsis:   Allows a GINA to specify the desktop that winlogon should
//              return to at the end of the call.
//
//  Arguments:  [hWlx]     --
//              [pDesktop] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxSetReturnDesktop(
    HANDLE              hWlx,
    PWLX_DESKTOP        pDesktop)
{
    PTERMINAL        pTerm;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return( FALSE );
    }

    if ((pDesktop->Size != sizeof(WLX_DESKTOP)) ||
        ((pDesktop->Flags & (WLX_DESKTOP_HANDLE | WLX_DESKTOP_NAME)) == 0) )
    {
        DebugLog((DEB_ERROR, "Invalid desktop\n"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }


    return( SetReturnDesktop( pTerm->pWinStaWinlogon, pDesktop ) );

}

//+---------------------------------------------------------------------------
//
//  Function:   WlxCreateUserDesktop
//
//  Synopsis:   Allows a GINA to create a desktop for use by user processes
//
//  Arguments:  [hWlx]           --
//              [hToken]         --
//              [Flags]          --
//              [pszDesktopName] --
//              [ppDesktop]      --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxCreateUserDesktop(
    HANDLE              hWlx,
    HANDLE              hToken,
    DWORD               Flags,
    PWSTR               pszDesktopName,
    PWLX_DESKTOP *      ppDesktop)
{
    PTERMINAL       pTerm;
    PTOKEN_GROUPS   pGroups;
    PTOKEN_USER     pUser;
    DWORD           Needed;
    NTSTATUS        Status;
    DWORD           i;
    PSID            pSid;
    PWLX_DESKTOP    pDesktop = NULL ;


    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return( FALSE );
    }

    if (((Flags & (WLX_CREATE_INSTANCE_ONLY | WLX_CREATE_USER)) == 0 ) ||
        ((Flags & (WLX_CREATE_INSTANCE_ONLY | WLX_CREATE_USER)) ==
                   (WLX_CREATE_INSTANCE_ONLY | WLX_CREATE_USER) ) )
    {
        DebugLog((DEB_ERROR, "Invalid flags\n"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return( FALSE );
    }

    pGroups = NULL;
    pUser = NULL;
    pSid = NULL;

    if ( Flags & WLX_CREATE_INSTANCE_ONLY )
    {
        Status = NtQueryInformationToken(   hToken,
                                            TokenGroups,
                                            NULL,
                                            0,
                                            &Needed );

        if ( Status != STATUS_BUFFER_TOO_SMALL )
        {
            SetLastError( RtlNtStatusToDosError( Status ) );
            return( FALSE );
        }

        pGroups = (PTOKEN_GROUPS) LocalAlloc( LMEM_FIXED, Needed );

        if ( !pGroups )
        {
            return( FALSE );
        }

        Status = NtQueryInformationToken(   hToken,
                                            TokenGroups,
                                            pGroups,
                                            Needed,
                                            &Needed );

        if ( !NT_SUCCESS( Status ) )
        {
            LocalFree( pGroups );
            SetLastError( RtlNtStatusToDosError( Status ) );
            return( FALSE );
        }

        for (i = 0 ; i < pGroups->GroupCount ; i++ )
        {
            if ( pGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID )
            {
                 pSid = pGroups->Groups[i].Sid;
                 break;
            }
        }

    }
    else
    {
        Status = NtQueryInformationToken(   hToken,
                                            TokenUser,
                                            NULL,
                                            0,
                                            &Needed );

        if ( Status != STATUS_BUFFER_TOO_SMALL )
        {
            SetLastError( RtlNtStatusToDosError( Status ) );
            return( FALSE );
        }

        pUser = (PTOKEN_USER) LocalAlloc( LMEM_FIXED, Needed );

        if ( !pUser )
        {
            return( FALSE );
        }

        Status = NtQueryInformationToken(   hToken,
                                            TokenUser,
                                            pUser,
                                            Needed,
                                            &Needed );

        if ( !NT_SUCCESS( Status ) )
        {
            LocalFree( pUser );
            SetLastError( RtlNtStatusToDosError( Status ) );
            return( FALSE );
        }

        pSid = pUser->User.Sid;

    }

    if ( !pSid )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto CleanUp;
    }

    //
    // Okay, we have the right SID now, so create the desktop.
    //

    Needed = sizeof( WLX_DESKTOP ) + (wcslen( pszDesktopName ) + 1 ) * sizeof(WCHAR);

    pDesktop = (PWLX_DESKTOP) LocalAlloc( LMEM_FIXED, Needed );

    if ( !pDesktop )
    {
        goto CleanUp;
    }

    pDesktop->Size = sizeof( WLX_DESKTOP );
    pDesktop->Flags = WLX_DESKTOP_NAME;
    pDesktop->hDesktop = NULL;
    pDesktop->pszDesktopName = (PWSTR) (pDesktop + 1);

    wcscpy( pDesktop->pszDesktopName, pszDesktopName );

    pDesktop->hDesktop = CreateDesktopW (pszDesktopName,
                                         NULL, NULL, 0, MAXIMUM_ALLOWED, NULL);

    if ( !pDesktop->hDesktop )
    {
        goto CleanUp;
    }

    if (!SetUserDesktopSecurity(pDesktop->hDesktop,
                                pSid, g_WinlogonSid ) )
    {
        goto CleanUp;
    }

    if (!AddUserToWinsta( pTerm->pWinStaWinlogon,
                          pSid,
                          hToken ) )
    {
        goto CleanUp;
    }

    *ppDesktop = pDesktop;
    pDesktop->Flags |= WLX_DESKTOP_HANDLE;

    if ( pGroups )
    {
        LocalFree( pGroups );
    }

    if ( pUser )
    {
        LocalFree( pUser );
    }

    return( TRUE );


CleanUp:

    if ( pDesktop )
    {
        if ( pDesktop->hDesktop )
        {
            CloseDesktop( pDesktop->hDesktop );
        }

        LocalFree( pDesktop );
    }

    if ( pGroups )
    {
        LocalFree( pGroups );
    }

    if ( pUser )
    {
        LocalFree( pUser );
    }

    return( FALSE );

}


//+---------------------------------------------------------------------------
//
//  Function:   WlxCloseUserDesktop
//
//  Synopsis:   Allows the GINA to close and clean up after creating a user
//              desktop.
//
//  Arguments:  [hWlx]     --
//              [pDesktop] --
//              [hToken]   --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxCloseUserDesktop(
    HANDLE          hWlx,
    PWLX_DESKTOP    pDesktop,
    HANDLE          hToken )
{
    PTERMINAL        pTerm;

    if (!(pTerm = VerifyHandle(hWlx)))
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return( FALSE );
    }

    if ( RemoveUserFromWinsta( pTerm->pWinStaWinlogon, hToken ) )
    {
        return( CloseDesktop( pDesktop->hDesktop ) );
    }

    return( FALSE );
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxSetOption
//
//  Synopsis:   Sets options
//
//  Arguments:  [hWlx]     --
//              [Option]   --
//              [Value]    --
//              [OldValue] --
//
//  History:    10-14-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxSetOption(
    HANDLE hWlx,
    DWORD Option,
    ULONG_PTR Value,
    ULONG_PTR * OldValue
    )
{
    PTERMINAL pTerm ;
    ULONG * Item ;
    BOOL Res = TRUE ;

    if ( !( pTerm = VerifyHandle( hWlx ) ) )
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return( FALSE );
    }

    Item = NULL ;

    //
    // Be careful when adding elements to this switch statement.  When
    // make sure the correct data size is saved.
    //

    switch ( Option )
    {
        C_ASSERT(sizeof(pTerm->ForwardCAD) == sizeof(ULONG));
        C_ASSERT(sizeof(pTerm->Gina.pGinaContext) == sizeof(ULONG_PTR));
        C_ASSERT(sizeof(pTerm->EnableSC) == sizeof(ULONG));

        case WLX_OPTION_USE_CTRL_ALT_DEL:
            Item = &pTerm->ForwardCAD ;
            break;

        case WLX_OPTION_CONTEXT_POINTER:
            if ( OldValue )
            {
                *OldValue = (ULONG_PTR) pTerm->Gina.pGinaContext;
            }
    
            pTerm->Gina.pGinaContext = (PVOID) Value;

            return(Res);

        case WLX_OPTION_USE_SMART_CARD:
            Item = &pTerm->EnableSC ;
            if ( Value )
            {
                Res = StartListeningForSC( pTerm );
                if ( !Res )
                {
                    Value = 0 ;
                }
            }
            else
            {
                Res = StopListeningForSC( pTerm );
            }
            break;

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;
    }

    if ( Item )
    {
        if ( OldValue )
        {
            *OldValue = *Item ;
        }

        *Item = (ULONG) Value ;
    }

    return Res ;
}

//+---------------------------------------------------------------------------
//
//  Function:   WlxGetOption
//
//  Synopsis:   Gets the current value for a variety of options
//
//  Arguments:  [hWlx]     --
//              [Option]   --
//              [Value]    --
//              [OldValue] --
//
//  History:    12-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
WlxGetOption(
    HANDLE hWlx,
    DWORD Option,
    ULONG_PTR * Value
    )
{
    PTERMINAL pTerm ;
    ULONG * Item ;
    ULONG Local ;

    if ( !( pTerm = VerifyHandle( hWlx ) ) )
    {
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return( FALSE );
    }

    Item = NULL ;

    switch ( Option )
    {
        C_ASSERT(sizeof(pTerm->ForwardCAD) == sizeof(ULONG));
        C_ASSERT(sizeof(pTerm->Gina.pGinaContext) == sizeof(ULONG_PTR));
        C_ASSERT(sizeof(pTerm->EnableSC) == sizeof(ULONG));

        case WLX_OPTION_USE_CTRL_ALT_DEL:
            Item = &pTerm->ForwardCAD ;
            break;

        case WLX_OPTION_CONTEXT_POINTER:
            if (Value) {
                *Value  = (ULONG_PTR) pTerm->Gina.pGinaContext ;
                return TRUE;
            }

            break;

        case WLX_OPTION_USE_SMART_CARD:
            Item = &pTerm->EnableSC ;
            break;

        case WLX_OPTION_SMART_CARD_PRESENT:

            Item = &Local ;

            Local = IsSmartCardReaderPresent( pTerm );

            break;

        case WLX_OPTION_SMART_CARD_INFO:

            if (Value) {
                *Value  = (ULONG_PTR) pTerm->CurrentScEvent ;
                return TRUE;
            }

            break;

        case WLX_OPTION_DISPATCH_TABLE_SIZE:
            Item = &Local ;

            Local = sizeof( WlxDispatchTable ) ;

            break;

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE ;
    }

    if ( Item )
    {
        if ( Value )
        {
            *Value = *Item ;
            return TRUE ;
        }

    }

    return FALSE ;
}


/*****************************************************************************
 *
 *  WlxQueryClientCredentials
 *
 *   Query credentials from a network WinStation.
 *
 *   This allows GINA's to fill in defaults in their dialogs
 *   with the user.
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
WlxQueryClientCredentials(
    PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pInfo
    )
{
    ULONG Length;
    WINSTATIONCONFIG ConfigData;
    BOOL ret = FALSE;

    // Console does not have a client
    if( g_Console ) {
        return( FALSE );
    }

    if( pInfo == NULL ) {
        return( FALSE );
    }

    //
    // See if autologon info is defined for the WinStation.
    //
    if ( !gpfnWinStationQueryInformation( SERVERNAME_CURRENT,
                                     LOGONID_CURRENT,
                                     WinStationConfiguration,
                                     &ConfigData,
                                     sizeof(ConfigData),
                                     &Length ) ) {
        return( FALSE );
    }

    if ( ConfigData.User.UserName[0] || ConfigData.User.Domain[0] ) {
        ret = InitializeAutoLogonInfo(
                pInfo,
                ConfigData.User.UserName,
                ConfigData.User.Domain,
                ConfigData.User.Password,
                ConfigData.User.fPromptForPassword
                );
    }

    return( ret );
}

/*****************************************************************************
 *
 *  WlxQueryInternetConnectorCredentials
 *
 *   Query credentials from a network WinStation.
 *
 *   This allows GINA's to fill in defaults in their dialogs
 *   with the user.
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
WlxQueryInetConnectorCredentials(
    PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pInfo
    )
{
    ULONG Length;
    WINSTATIONCONFIG ConfigData;
    BOOL ret = FALSE;

    // Console does not have a client
    if( g_Console ) {
        return( FALSE );
    }

    if( pInfo == NULL ) {
        return( FALSE );
    }

    if ( NULL == gpfnServerQueryInetConnectorInformation) {
        return( FALSE );
    }

    //
    // Get Internet Connector information.
    //
    if ( !gpfnServerQueryInetConnectorInformation(
                SERVERNAME_CURRENT,
                &ConfigData,
                sizeof(ConfigData),
                &Length ) ) {
        return( FALSE );
    }

    if ( ConfigData.User.UserName[0] || ConfigData.User.Domain[0] ) {
        ret = InitializeAutoLogonInfo(
                pInfo,
                ConfigData.User.UserName,
                ConfigData.User.Domain,
                ConfigData.User.Password,
                ConfigData.User.fPromptForPassword
                );
    }

    return( ret );
}

/*****************************************************************************
 *
 *  WlxDisconnect
 *
 *   Disconnect from a network WinStation
 *
 *   This allows GINA's to optionally support a Disconnect... dialog
 *   item.
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
WlxDisconnect( void )
{
    BOOL rc;

    if (!g_IsTerminalServer) {

        return FALSE;
    }

    rc = gpfnWinStationDisconnect(
             SERVERNAME_CURRENT,
             LOGONID_CURRENT,
             TRUE
             );

    return( rc );
}

/*****************************************************************************
 *
 *  WlxWin31Migrate
 *
 *   Perform per user Win31 application migration
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
WINAPI
WlxWin31Migrate(
    HANDLE  hWlx)
{

    TCHAR szAdminName[ MAX_STRING_BYTES ];
    PTERMINAL    pTerm;

    if (pTerm = VerifyHandle(hWlx))
    {
        //
        // If not logging in as Guest, System or Administrator then check for
        // migration of Windows 3.1 configuration information.
        //

        LoadString(NULL, IDS_ADMIN_ACCOUNT_NAME, szAdminName, sizeof(szAdminName));
        if (!IsUserAGuest(pTerm->pWinStaWinlogon) &&
            _wcsicmp(pTerm->pWinStaWinlogon->UserName, szAdminName)
           )
        {
            Windows31Migration(pTerm);
        }
    }

}

/******************************************************************************
 *
 *  InitializeAutoLogonInfo
 *      Saves autologon info into global structure.
 *
 *   initialize autologon info.
 *
 *  ENTRY:
 *     pUserName (input)
 *        pointer to username string
 *     pDomain (input)
 *        pointer to domain string (may be empty)
 *     pPassword (input)
 *        pointer to password string (may be empty)
 *
 *  EXIT:
 *     pointer to LocalAlloc()'d structure
 *
 *****************************************************************************/

BOOL
InitializeAutoLogonInfo(
    PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pAutoLogon,
    LPTSTR pUserName,
    LPTSTR pDomain,
    LPTSTR pPassword,
    BOOL fPromptForPassword
    )
{
    pAutoLogon->pszUserName = AllocAndDuplicateString( pUserName );
    pAutoLogon->pszDomain =   AllocAndDuplicateString( pDomain );
    pAutoLogon->pszPassword = AllocAndDuplicateString( pPassword );
    pAutoLogon->fPromptForPassword = fPromptForPassword;

    return( TRUE );
}

//
// This function spawns an application on behalf of a user.
// Note that the environment variables are duplicated before
// passing to the gina.  This is due to msgina updating the
// user environment variables which could cause the environment
// block to move but the pointer change isn't passed back to winlogon
//
// In theory, msgina shouldn't be touching the environment block
// passed in.  Winlogon should do all the environment variable
// management.  When this happens, this wrapper api can be removed.
//

BOOL
StartApplication(
    PTERMINAL               pTerm,
    PWSTR                   pszDesktop,
    PVOID                   pEnvironment,
    PWSTR                   pszCmdLine
    )
{
    BOOL  bResult;
    PVOID pNewEnvironment;
    UINT ErrorMode ;

    if (!pEnvironment) {
        return FALSE;
    }

    pNewEnvironment = CopyEnvironment(pEnvironment);

    ErrorMode = SetErrorMode( pTerm->ErrorMode );

    bResult = pTerm->Gina.pWlxStartApplication(pTerm->Gina.pGinaContext,
                                               pszDesktop,
                                               pNewEnvironment, pszCmdLine);

    SetErrorMode( ErrorMode );

    VirtualFree(pNewEnvironment, 0, MEM_RELEASE);

    return bResult;
}

VOID
QueryVerboseStatus(
    VOID
    )
{
    HKEY hKey;
    DWORD dwType, dwSize;
    BOOL bVerbose = FALSE;


    //
    // First, check for a machine preference
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bVerbose);
        RegQueryValueEx (hKey, VERBOSE_STATUS, NULL, &dwType,
                         (LPBYTE) &bVerbose, &dwSize);
        RegCloseKey (hKey);
    }


    //
    // Second, check for machine policy
    //

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0,
                            KEY_READ, &hKey) == ERROR_SUCCESS) {

        dwSize = sizeof(bVerbose);
        RegQueryValueEx (hKey, VERBOSE_STATUS, NULL, &dwType,
                         (LPBYTE) &bVerbose, &dwSize);

        RegCloseKey (hKey);
    }


    //
    // Save the results
    //

    g_bVerboseStatus = bVerbose;

}

VOID
StatusMessage(
    BOOL bVerbose,
    DWORD dwOptions,
    UINT idMsg,
    ...
    )
{
    TCHAR szMsg[MAX_PATH];
    LPTSTR lpMessage;
    va_list marker;
    HDESK hDesktop;
    BOOL bCloseWhenDone;
    BOOL bLocked;


    //
    // Check if status UI is allowed
    //

    if (!g_fAllowStatusUI) {
        return;
    }


    //
    // If this is a verbose message and we are not in verbose
    // mode, exit now
    //

    if (bVerbose && !g_bVerboseStatus) {
        return;
    }


    //
    // Check if the gina is ready for status messages
    //

    if (!g_pTerminals || !g_pTerminals->Gina.pGinaContext) {
        return;
    }


    //
    // Load the message
    //

    if (!LoadString (NULL, idMsg, szMsg, ARRAYSIZE(szMsg))) {
        return;
    }


    //
    // Allocate space for the message
    //

    lpMessage = LocalAlloc (LPTR, (2 * MAX_PATH) * sizeof(TCHAR));

    if (!lpMessage) {
        return;
    }


    //
    // Plug in the arguments
    //

    va_start(marker, idMsg);
    wvsprintf(lpMessage, szMsg, marker);
    va_end(marker);


    //
    // Get the active desktop
    //

    hDesktop = GetActiveDesktop(g_pTerminals, &bCloseWhenDone, &bLocked);


    //
    // Update the status message
    //

    g_pTerminals->Gina.pWlxDisplayStatusMessage(g_pTerminals->Gina.pGinaContext,
                                                hDesktop, dwOptions, NULL, lpMessage);


    //
    // Close the desktop handle if appropriate
    //

    if (bCloseWhenDone)
    {
        CloseDesktop(hDesktop);
    }

    LocalFree (lpMessage);
}

DWORD
StatusMessage2(
    BOOL bVerbose,
    LPWSTR lpMessage
    )
{
    HDESK hDesktop;
    BOOL bCloseWhenDone;
    BOOL bLocked;


    //
    // Check if status UI is allowed
    //

    if (!g_fAllowStatusUI) {
        return ERROR_SUCCESS;
    }


    //
    // If this is a verbose message and we are not in verbose
    // mode, exit now
    //

    if (bVerbose && !g_bVerboseStatus) {
        return ERROR_SUCCESS;
    }


    //
    // Check if the gina is ready for status messages
    //

    if (!g_pTerminals || !g_pTerminals->Gina.pGinaContext) {
        return ERROR_SUCCESS;
    }


    //
    // Get the active desktop
    //

    hDesktop = GetActiveDesktop(g_pTerminals, &bCloseWhenDone, &bLocked);


    //
    // Update the status message
    //

    g_pTerminals->Gina.pWlxDisplayStatusMessage(g_pTerminals->Gina.pGinaContext,
                                                hDesktop, 0, NULL, lpMessage);


    //
    // Close the desktop handle if appropriate
    //

    if (bCloseWhenDone)
    {
        CloseDesktop(hDesktop);
    }

    return ERROR_SUCCESS;
}


VOID
RemoveStatusMessage(
    BOOL bForce
    )
{

    //
    // Check if the gina is ready
    //

    if (!g_pTerminals || !g_pTerminals->Gina.pGinaContext) {
        return;
    }


    //
    // Remove the status message under these conditions:
    //
    // 1)  We are not in verbose status message mode
    // 2)  We are in verbose status message mode and bForce is true
    //

    if (!g_bVerboseStatus || (g_bVerboseStatus && bForce)) {
        g_pTerminals->Gina.pWlxRemoveStatusMessage(g_pTerminals->Gina.pGinaContext);
    }

}


//*************************************************************
//
//  WaitForServiceToStart()
//
//  Purpose:    Waits for the specified service to start
//
//  Parameters: dwMaxWait  -  Max wait time
//
//
//  Return:     TRUE if the network is started
//              FALSE if not
//
//*************************************************************
BOOL WaitForServiceToStart (LPTSTR lpServiceName, DWORD dwMaxWait)
{
    BOOL bStarted = FALSE;
    DWORD dwSize = 512;
    DWORD StartTickCount;
    DWORD dwOldCheckPoint = (DWORD)-1;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;

    //
    // OpenSCManager and the rpcss service
    //
    hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hScManager) {
        goto Exit;
    }

    hService = OpenService(hScManager, lpServiceName,
                           SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
    if (!hService) {
        goto Exit;
    }

    //
    // Query if the service is going to start
    //
    lpServiceConfig = LocalAlloc (LPTR, dwSize);
    if (!lpServiceConfig) {
        goto Exit;
    }

    if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto Exit;
        }

        LocalFree (lpServiceConfig);

        lpServiceConfig = LocalAlloc (LPTR, dwSize);

        if (!lpServiceConfig) {
            goto Exit;
        }

        if (!QueryServiceConfig (hService, lpServiceConfig, dwSize, &dwSize)) {
            goto Exit;
        }
    }

    if (lpServiceConfig->dwStartType != SERVICE_AUTO_START) {
        goto Exit;
    }

    //
    // Loop until the service starts or we think it never will start
    // or we've exceeded our maximum time delay.
    //

    StartTickCount = GetTickCount();

    while (!bStarted) {

        if ((GetTickCount() - StartTickCount) > dwMaxWait) {
            break;
        }

        if (!QueryServiceStatus(hService, &ServiceStatus )) {
            break;
        }

        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            if (ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {
                Sleep(500);
            } else {
                break;
            }
        } else if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
                    (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

            bStarted = TRUE;

        } else if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING) {
            Sleep(500);
        } else {
            Sleep(500);
        }
    }


Exit:

    if (lpServiceConfig) {
        LocalFree (lpServiceConfig);
    }

    if (hService) {
        CloseServiceHandle(hService);
    }

    if (hScManager) {
        CloseServiceHandle(hScManager);
    }

    return bStarted;
}


//*************************************************************
//
//  WaitForMUP()
//
//  Purpose:    Waits for the MUP to finish initializing
//
//  Parameters: dwMaxWait     -  Max wait time
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************
BOOL WaitForMUP (DWORD dwMaxWait)
{
    HANDLE hEvent;
    BOOL bResult;
    INT i = 0;

    //
    // Try to open the event
    //
    do {
        hEvent = OpenEvent (SYNCHRONIZE, FALSE,
                            TEXT("wkssvc:  MUP finished initializing event"));
        if (hEvent) {
            break;
        }

        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            break;
        }

        Sleep(500);
        i++;
    } while (i < 20);

    if (!hEvent) {
        DebugLog((DEB_ERROR, "Failed to open MUP event with %d\n", GetLastError()));
        return FALSE;
    }

    //
    // Wait for the event to be signalled
    //
    bResult = (WaitForSingleObject (hEvent, dwMaxWait) == WAIT_OBJECT_0);

    //
    // Clean up
    //
    CloseHandle (hEvent);

    return bResult;
}


//*************************************************************
//
//  WaitForDS()
//
//  Purpose:    Waits for the DS service to start if this is a DC
//
//  Parameters: dwMaxWait     -  Max wait time
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************
BOOL WaitForDS (DWORD dwMaxWait)
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBasic;
    DWORD   dwResult;
    BOOL    bRetVal = FALSE;
    TCHAR   szPath[512];
    DWORD   dwCount = 0, dwMax;
    HANDLE  hFile;
    WIN32_FIND_DATA fd;

    //
    // Ask for the role of this machine
    //

    dwResult = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic,
                                                 (PBYTE *)&pBasic);

    if (dwResult == ERROR_SUCCESS) {

        //
        // Check if this is a DC
        //

        if ((pBasic->MachineRole == DsRole_RoleBackupDomainController) ||
            (pBasic->MachineRole == DsRole_RolePrimaryDomainController)) {

            HANDLE hEvent;

            hEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE,
                                TEXT("NtdsDelayedStartupCompletedEvent") );

            if (hEvent) {
                WaitForSingleObject (hEvent, dwMaxWait);
                CloseHandle (hEvent);
            }
        }

        //
        // If this is a PDC, check HKLM\SYSTEM\CurrentControlSet\Services\netlogon\parameters\SysVolReady
        // If this is absent or, the value is 0 wait till SysVol is ready on the PDC
        //

        if ( pBasic->MachineRole == DsRole_RolePrimaryDomainController ) {
            HKEY hKeyNetLogonParams;

            dwResult = RegOpenKeyEx(    HKEY_LOCAL_MACHINE,
                                        TEXT("SYSTEM\\CurrentControlSet\\Services\\netlogon\\parameters"),
                                        0,
                                        KEY_READ,
                                        &hKeyNetLogonParams );

            if ( dwResult == ERROR_SUCCESS ) {

                //
                // value exists
                //

                DWORD   dwSysVolReady = 0,
                        dwSize = sizeof( DWORD ),
                        dwType = REG_DWORD;

                dwResult = RegQueryValueEx( hKeyNetLogonParams,
                                            TEXT("SysVolReady"),
                                            0,
                                            &dwType,
                                            (LPBYTE) &dwSysVolReady,
                                            &dwSize );

                if ( dwResult == ERROR_SUCCESS ) {

                    //
                    // SysVolReady?
                    //

                    if ( dwSysVolReady == 0 ) {

                        HANDLE hEvent;

                        //
                        // wait for SysVol to become ready
                        //

                        hEvent = CreateEvent( 0, TRUE, FALSE, TEXT("SysVolReadyEvent") );

                        if ( hEvent ) {

                            dwResult = RegNotifyChangeKeyValue( hKeyNetLogonParams, FALSE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE );

                            if ( dwResult == ERROR_SUCCESS ) {


                                const DWORD dwMaxCount = 3;

                                do {

                                    //
                                    // wait for SysVolReady to change
                                    // hEvent is signaled for any changes in hKeyNetLogonParams
                                    // not just the SysVolReady value.
                                    //

                                    WaitForSingleObject (hEvent, dwMaxWait / dwMaxCount );

                                    dwResult = RegQueryValueEx( hKeyNetLogonParams,
                                                                TEXT("SysVolReady"),
                                                                0,
                                                                &dwType,
                                                                (LPBYTE) &dwSysVolReady,
                                                                &dwSize );

                                } while ( dwSysVolReady == 0 && ++dwCount < dwMaxCount );

                                if ( dwSysVolReady ) {

                                    bRetVal = TRUE;

                                }
                            }

                            CloseHandle( hEvent );

                        }

                    }

                } else  {

                    //
                    // value is non-existent, SysVol is assumed to be ready
                    //

                    if ( dwResult == ERROR_FILE_NOT_FOUND ) {
                        bRetVal = TRUE;
                    }
                }

                RegCloseKey( hKeyNetLogonParams );
            }


        } else {

            //
            // machine is not a PDC
            //

            bRetVal = TRUE;
        }


        //
        // Test the sysvol
        //

        if ((pBasic->MachineRole == DsRole_RoleBackupDomainController) ||
            (pBasic->MachineRole == DsRole_RolePrimaryDomainController)) {

            wsprintf (szPath, TEXT("\\\\%s\\sysvol\\*.*"), pBasic->DomainNameDns);

            DebugLog((DEB_TRACE, "Testing sysvol path of %S\n", szPath));

            dwCount = 0;
            dwMax =  GetProfileInt (WINLOGON, MAX_RETRY_SYSVOL_ACCESS, 180);

            while (TRUE) {

                hFile = FindFirstFile (szPath, &fd);

                if (hFile != INVALID_HANDLE_VALUE) {
                    FindClose (hFile);
                    break;
                }

                DebugLog((DEB_TRACE, "FindFirstFile failed with %d\n", GetLastError()));

                dwCount++;

                if (dwCount > dwMax) {
                    break;
                }

                Sleep(1000);
            }
        }


        DsRoleFreeMemory (pBasic);
    }

    return bRetVal;
}



VOID WaitForServices(
    PTERMINAL pTerm
    )
{

    HANDLE hDsReindexEvent ;
    ULONG SamWaitTime = 15000 ;

    if ( pTerm->SafeMode )
    {
        SamWaitTime = 120000 ;
    }

    StatusMessage(FALSE, 0, IDS_STATUS_SYSTEM_STARTUP );
    WaitForServiceToStart( TEXT("SamSs"), SamWaitTime);

    if ( pTerm->SafeMode )
    {
        //
        // In safe mode, no sense waiting for services that
        // are disabled to start.
        //

        return;
    }
    hDsReindexEvent = OpenEvent( SYNCHRONIZE,
                                 FALSE,
                                 L"NTDS.IndexRecreateEvent" );

    if ( hDsReindexEvent ) {
        StatusMessage( FALSE, 0, IDS_STATUS_DS_REINDEX );

        WaitForSingleObject( hDsReindexEvent, INFINITE );

        CloseHandle( hDsReindexEvent );
    }

    //
    // Wait for the network to start
    //

    StatusMessage (FALSE, 0, IDS_STATUS_NET_START);
    WaitForServiceToStart (SERVICE_NETLOGON, 120000);

    StatusMessage (TRUE, 0, IDS_STATUS_RPCSS_START);
    WaitForServiceToStart (TEXT("RpcSs"), 120000);

    StatusMessage (TRUE, 0, IDS_STATUS_MUP_START);
    WaitForMUP (120000);

    StatusMessage (TRUE, 0, IDS_STATUS_DS_START);
    WaitForDS (120000);
}


// This function is passed as a utility function to the GINA.
// It is used in the WlxLoggedOutSAS() context, after the username and domain
// are retrieved on a successful logon, to perform off-machine lookups
// of the entire Terminal Services USERCONFIG private struct.
// The components that the GINA needs are the copied from the
// USERCONFIG into *pTSData. The USERCONFIG is preserved in the MuGlobals
// of the TERMINAL for after WlxLoggedOutSAS() returns, to send the
// information to TermSrv.
//
// Returns an ERROR_XXX code.
DWORD WINAPI WlxQueryTerminalServicesData(
        HANDLE hWlx,
        PWLX_TERMINAL_SERVICES_DATA pTSData,
        WCHAR *UserName,
        WCHAR *Domain)
{
    PTERMINAL pTerm;
    DWORD rc;

    pTerm = VerifyHandle(hWlx);
    if (pTerm != NULL) {
        rc = QueryTerminalServicesDataWorker(pTerm, UserName, Domain);
        if (rc == STATUS_SUCCESS) {
            // Copy to *pTSData the elements of the USERCONFIG needed by the GINA.
            wcscpy(pTSData->ProfilePath,
                    pTerm->MuGlobals.UserConfig.WFProfilePath);
            wcscpy(pTSData->HomeDir,
                    pTerm->MuGlobals.UserConfig.WFHomeDir);
            wcscpy(pTSData->HomeDirDrive,
                    pTerm->MuGlobals.UserConfig.WFHomeDirDrive);
        }
    }
    else {
        // If the GINA cannot give us a valid handle, mark that we have not
        // queried the data yet so we'll do it later.
        DebugLog((DEB_ERROR, "Invalid hWlx handle\n"));
        rc = ERROR_INVALID_DATA;
    }

    pTerm->MuGlobals.ConfigQueryResult = rc;
    return rc;
}

// Real worker function -- allows us to call from within WinLogon itself.
DWORD WINAPI QueryTerminalServicesDataWorker(
        PTERMINAL pTerm,
        WCHAR *UserName,
        WCHAR *Domain)
{
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR DcName[MAX_COMPUTERNAME_LENGTH + 3];
    ULONG Length;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    PWSTR DcNameBuffer = NULL;
    DWORD rc = ERROR_SUCCESS;
    ULONG Error;
    HANDLE dllHandle = NULL;
    PREGUSERCONFIGQUERY        pfnRegUserConfigQuery = NULL;
    PREGDEFAULTUSERCONFIGQUERY pfnRegDefaultUserConfigQuery = NULL;

    // Grab the local computer name.
    Length = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(ComputerName, &Length)) {
        ComputerName[0] = L'\0';
    }

    // If we are not logging in locally, we need to grab the domain controller
    // computer name 
    if (_wcsicmp(ComputerName, Domain)) {
        __try {
            // Delay load netapi32.dll. We need a try except around this
            // call in case the LoadLibrary/GetProcAddress fails during
            // runtime
            Error = DsGetDcName(NULL, Domain, NULL, NULL, 0, &DcInfo);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Error == NO_ERROR) {
            DcNameBuffer = DcInfo->DomainControllerName;
        }
    }
    else {
        if (ComputerName[0] != L'\0') {
            // Prepend 
            DcName[0] = DcName[1] = L'\\';
            wcscpy(DcName + 2, ComputerName);
            DcNameBuffer = DcName;
        }
    }

    // Load regapi.dll
    dllHandle = LoadLibraryW(L"regapi.dll");
    if (dllHandle != NULL) {
        pfnRegUserConfigQuery = (PREGUSERCONFIGQUERY)GetProcAddress(
                dllHandle, "RegUserConfigQuery");
        pfnRegDefaultUserConfigQuery = (PREGDEFAULTUSERCONFIGQUERY)
                GetProcAddress(dllHandle, "RegDefaultUserConfigQueryW");

        if (pfnRegUserConfigQuery == NULL ||
                pfnRegDefaultUserConfigQuery == NULL) {
            DebugLog((DEB_ERROR, "WlxQueryTSData: Could not get "
                    "RegUserConfigQuery or RegDefaultUserConfigQuery\n"));
            rc = ERROR_GEN_FAILURE;
            goto Cleanup;
        }
    }
    else {
        DebugLog((DEB_ERROR, "WlxQueryTSData: Could not load regapi.dll\n"));
        rc = ERROR_GEN_FAILURE;
        goto Cleanup;
    }

    // Get Domain Controller name and userconfig data.
    // If no userconfig data for user then get default values.
    DebugLog((DEB_TRACE,"WlxQueryTSData: RegUserConfigQuery: \\\\%S\\%S, "
            "server %S\n", Domain, UserName, DcNameBuffer));
    Error = pfnRegUserConfigQuery(DcNameBuffer, UserName,
            &pTerm->MuGlobals.UserConfig, sizeof(USERCONFIG), &Length);
    if (Error != ERROR_SUCCESS) {
        DebugLog((DEB_TRACE,"WlxQueryTSData: RegUserConfigQuery returned %u\n",
                Error));
        Error = pfnRegDefaultUserConfigQuery(DcNameBuffer,
                &pTerm->MuGlobals.UserConfig, sizeof(USERCONFIG), &Length);
        DebugLog((DEB_TRACE,"WlxQueryTSData: RegDefaultUserConfigQuery "
                "error=%u\n", Error));
    }

Cleanup:
    if (dllHandle != NULL)
        FreeLibrary(dllHandle);

    if (DcInfo)
        NetApiBufferFree(DcInfo);

    // Store the return value in MuGlobals for later consumption
    // after return from the GINA's WlxLoggedOutSAS() function.
    pTerm->MuGlobals.ConfigQueryResult = rc;

    return rc;
}
