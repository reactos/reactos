/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    find.c

Abstract:

    This module implements the Win32 find dialog.

Revision History:

--*/



//
//  Include Files.
//

#include "comdlg32.h"
#include "find.h"





#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  FindTextA
//
//  ANSI entry point for FindText when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

HWND WINAPI FindTextA(
    LPFINDREPLACEA pFRA)
{
    return (CreateFindReplaceDlg((LPFINDREPLACEW)pFRA, DLGT_FIND, COMDLG_ANSI));
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  FindTextW
//
//  Stub UNICODE function for FindText when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

HWND WINAPI FindTextW(
    LPFINDREPLACEW pFRW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif



////////////////////////////////////////////////////////////////////////////
//
//  FindText
//
//  The FindText function creates a system-defined modeless dialog box
//  that enables the user to find text within a document.
//
////////////////////////////////////////////////////////////////////////////

HWND WINAPI FindText(
    LPFINDREPLACE pFR)
{
    return ( CreateFindReplaceDlg(pFR, DLGT_FIND, COMDLG_WIDE) );
}


#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ReplaceTextA
//
//  ANSI entry point for ReplaceText when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

HWND WINAPI ReplaceTextA(
    LPFINDREPLACEA pFRA)
{
    return (CreateFindReplaceDlg((LPFINDREPLACEW)pFRA, DLGT_REPLACE, COMDLG_ANSI));
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  ReplaceTextW
//
//  Stub UNICODE function for ReplaceText when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

HWND WINAPI ReplaceTextW(
    LPFINDREPLACEW pFRW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  ReplaceText
//
//  The ReplaceText function creates a system-defined modeless dialog box
//  that enables the user to find and replace text within a document.
//
////////////////////////////////////////////////////////////////////////////

HWND WINAPI ReplaceText(
    LPFINDREPLACE pFR)
{
    return ( CreateFindReplaceDlg(pFR, DLGT_REPLACE, COMDLG_WIDE) );
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateFindReplaceDlg
//
//  Creates FindText modeless dialog.
//
//  pFR     - ptr to FINDREPLACE structure set up by user
//  DlgType - type of dialog to create (DLGT_FIND, DLGT_REPLACE)
//  ApiType - type of FINDREPLACE ptr (COMDLG_ANSI or COMDLG_WIDE)
//
//  Returns   success => HANDLE to created dlg
//            failure => HNULL = ((HANDLE) 0)
//
////////////////////////////////////////////////////////////////////////////

HWND CreateFindReplaceDlg(
    LPFINDREPLACE pFR,
    UINT DlgType,
    UINT ApiType)
{
    HWND hWndDlg;                      // handle to created modeless dialog
    HANDLE hDlgTemplate;               // handle to loaded dialog resource
    LPCDLGTEMPLATE lpDlgTemplate;      // pointer to loaded resource block
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif

    if (!pFR)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (!SetupOK(pFR, DlgType, ApiType))
    {
        return (HNULL);
    }

    if (!(hDlgTemplate = GetDlgTemplate(pFR, DlgType, ApiType)))
    {
        return (FALSE);
    }

    if (lpDlgTemplate = (LPCDLGTEMPLATE)LockResource(hDlgTemplate))
    {
        PFINDREPLACEINFO pFRI;

        if (pFRI = (PFINDREPLACEINFO)LocalAlloc(LPTR, sizeof(FINDREPLACEINFO)))
        {
            //
            //  CLEAR extended error on new instantiation.
            //
            StoreExtendedError(0);

            if (pFR->Flags & FR_ENABLEHOOK)
            {
                glpfnFindHook = GETHOOKFN(pFR);
            }

            pFRI->pFR = pFR;
            pFRI->ApiType = ApiType;
            pFRI->DlgType = DlgType;

#ifdef UNICODE
            if (IS16BITWOWAPP(pFR))
            {
                uiWOWFlag = SCDLG_16BIT;
            }

            hWndDlg = CreateDialogIndirectParamAorW( g_hinst,
                                                     lpDlgTemplate,
                                                     pFR->hwndOwner,
                                                     FindReplaceDlgProc,
                                                     (LPARAM)pFRI,
                                                     uiWOWFlag );
#else
            hWndDlg = CreateDialogIndirectParam( g_hinst,
                                                 lpDlgTemplate,
                                                 pFR->hwndOwner,
                                                 FindReplaceDlgProc,
                                                 (LPARAM)pFRI );
#endif
            if (!hWndDlg)
            {
                glpfnFindHook = 0;
                LocalFree(pFRI);
            }
        }
        else
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            return (NULL);
        }
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
        return (HNULL);
    }

    return (hWndDlg);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetupOK
//
//  Checks setup for unmet preconditions.
//
//  pFR       ptr to FINDREPLACE structure
//  DlgType   dialog type (either FIND or REPLACE)
//  ApiType   findreplace type (either COMDLG_ANSI or COMDLG_UNICODE)
//
//  Returns   TRUE   - success
//            FALSE  - failure
//
////////////////////////////////////////////////////////////////////////////

BOOL SetupOK(
   LPFINDREPLACE pFR,
   UINT DlgType,
   UINT ApiType)
{
    LANGID LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    //
    //  Sanity
    //
    if (!pFR)
    {
        return (FALSE);
    }

    if (pFR->lStructSize != sizeof(FINDREPLACE))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    //
    //  Verify window handle and text pointers.
    //
    if (!IsWindow(pFR->hwndOwner))
    {
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return (FALSE);
    }

    if (!pFR->lpstrFindWhat ||
        ((DlgType == DLGT_REPLACE) && !pFR->lpstrReplaceWith) ||
        !pFR->wFindWhatLen)
    {
        StoreExtendedError(FRERR_BUFFERLENGTHZERO);
        return (FALSE);
    }

    //
    //  Verify lpfnHook has a ptr if ENABLED.
    //
    if (pFR->Flags & FR_ENABLEHOOK)
    {
        if (!pFR->lpfnHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pFR->lpfnHook = 0;
    }

    //
    // Get LangID 
    //
    if (
        !(pFR->Flags & FR_ENABLETEMPLATE) &&
        !(pFR->Flags & FR_ENABLETEMPLATEHANDLE) )
    {
        LangID = GetDialogLanguage(pFR->hwndOwner, NULL);
    }

    //
    // Warning! Warning! Warning!
    //
    // We have to set g_tlsLangID before any call for CDLoadString
    //
    TlsSetValue(g_tlsLangID, (LPVOID) LangID);

    //
    //  Load "CLOSE" text for Replace.
    //
    if ((DlgType == DLGT_REPLACE) &&
        !CDLoadString(g_hinst, iszClose, (LPTSTR)szClose, CCHCLOSE))
    {
        StoreExtendedError(CDERR_LOADSTRFAILURE);
        return (FALSE);
    }


    //
    //  Setup unique msg# for talking to hwndOwner.
    //
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        if (!(wFRMessage = RegisterWindowMessageA((LPCSTR)FINDMSGSTRINGA)))
        {
            StoreExtendedError(CDERR_REGISTERMSGFAIL);
            return (FALSE);
        }
    }
    else
#endif
    {
        if (!(wFRMessage = RegisterWindowMessage((LPCTSTR)FINDMSGSTRING)))
        {
            StoreExtendedError(CDERR_REGISTERMSGFAIL);
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDlgTemplate
//
//  Finds and loads the dialog template.
//
//  pFR       ptr to FINDREPLACE structure
//  ApiType   findreplace type (either COMDLG_ANSI or COMDLG_UNICODE)
//
//  Returns   handle to dialog template   - success
//            HNULL = ((HANDLE) 0)        - failure
//
////////////////////////////////////////////////////////////////////////////

HANDLE GetDlgTemplate(
    LPFINDREPLACE pFR,
    UINT DlgType,
    UINT ApiType)
{
    HANDLE hRes;                 // handle of res. block with dialog
    HANDLE hDlgTemplate;         // handle to loaded dialog resource
    LANGID LangID;

    if (pFR->Flags & FR_ENABLETEMPLATE)
    {
        //
        //  Find/Load TEMP NAME and INSTANCE from pFR.
        //
#ifdef UNICODE
        if (ApiType == COMDLG_ANSI)
        {
            hRes = FindResourceA( (HMODULE)pFR->hInstance,
                                  (LPCSTR)pFR->lpTemplateName,
                                  (LPCSTR)RT_DIALOG );
        }
        else
#endif
        {
            hRes = FindResource( pFR->hInstance,
                                 (LPCTSTR)pFR->lpTemplateName,
                                 (LPCTSTR)RT_DIALOG );
        }
        if (!hRes)
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (HNULL);
        }
        if (!(hDlgTemplate = LoadResource(pFR->hInstance, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (HNULL);
        }
    }
    else if (pFR->Flags & FR_ENABLETEMPLATEHANDLE)
    {
        //
        //  Get whole PRELOADED resource handle from user.
        //
        if (!(hDlgTemplate = pFR->hInstance))
        {
            StoreExtendedError(CDERR_NOHINSTANCE);
            return (HNULL);
        }
    }
    else
    {
        //
        //  Get STANDARD dialog from DLL instance block.
        //
        LangID = (LANGID) TlsGetValue(g_tlsLangID);

        if (DlgType == DLGT_FIND)
        {
            hRes = FindResourceEx( g_hinst,
                                   RT_DIALOG, 
                                   (LPCTSTR)MAKELONG(FINDDLGORD, 0),
                                   LangID);
        }
        else
        {
            hRes = FindResourceEx( g_hinst,
                                   RT_DIALOG, 
                                   (LPCTSTR)MAKELONG(REPLACEDLGORD, 0),
                                   LangID);
        }

        //
        //  !!!!!  definitely ORD here?
        //
        if (!hRes)
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (HNULL);
        }
        if (!(hDlgTemplate = LoadResource(g_hinst, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (HNULL);
        }
    }

    return (hDlgTemplate);
}


////////////////////////////////////////////////////////////////////////////
//
//  FindReplaceDlgProc
//
//  Handles messages to FindText/ReplaceText dialogs.
//
//  hDlg   -  handle to dialog
//  wMsg   -  window message
//  wParam -  w parameter of message
//  lParam -  l parameter of message
//
//  Note: lparam contains ptr to FINDREPLACEINITPROC upon
//        initialization from CreateDialogIndirectParam...
//
//  Returns:   TRUE (or dlg fcn return vals) - success
//             FALSE                         - failure
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK FindReplaceDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PFINDREPLACEINFO pFRI;
    LPFINDREPLACE pFR;
    BOOL_PTR bRet;

    //
    //  If a hook exists, let hook function do procing.
    //
    if (pFRI = (PFINDREPLACEINFO)GetProp(hDlg, FINDREPLACEPROP))
    {
        if ((pFR = (LPFINDREPLACE)pFRI->pFR) &&
            (pFR->Flags & FR_ENABLEHOOK))
        {
            LPFRHOOKPROC lpfnHook = GETHOOKFN(pFR);

            if ((bRet = (*lpfnHook)(hDlg, wMsg, wParam, lParam)))
            {
                return (bRet);
            }
        }
    }
    else if (glpfnFindHook &&
             (wMsg != WM_INITDIALOG) &&
             (bRet = (* glpfnFindHook)(hDlg, wMsg, wParam, lParam)))
    {
        return (bRet);
    }

    //
    //  Dispatch MSG to appropriate HANDLER.
    //
    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            //
            //  Set Up P-Slot.
            //
            pFRI = (PFINDREPLACEINFO)lParam;
            SetProp(hDlg, FINDREPLACEPROP, (HANDLE)pFRI);

            glpfnFindHook = 0;

            //
            //  Init dlg controls accordingly.
            //
            pFR = pFRI->pFR;
            InitControlsWithFlags(hDlg, pFR, pFRI->DlgType, pFRI->ApiType);

            //
            //  If Hook function, do extra processing.
            //
            if (pFR->Flags & FR_ENABLEHOOK)
            {
                LPFRHOOKPROC lpfnHook = GETHOOKFN(pFR);

                bRet = (*lpfnHook)(hDlg, wMsg, wParam, (LPARAM)pFR);
            }
            else
            {
                bRet = TRUE;
            }

            if (bRet)
            {
                //
                //  If the hook function returns FALSE, then we must call
                //  these functions here.
                //
                ShowWindow(hDlg, SW_SHOWNORMAL);
                UpdateWindow(hDlg);
            }

            return (bRet);
            break;
        }
        case ( WM_COMMAND ) :
        {
            if (!pFRI || !pFR)
            {
                return (FALSE);
            }

            switch (GET_WM_COMMAND_ID (wParam, lParam))
            {
                //
                //  FIND NEXT button clicked.
                //
                case ( IDOK ) :
                {
                    UpdateTextAndFlags( hDlg,
                                        pFR,
                                        FR_FINDNEXT,
                                        pFRI->DlgType,
                                        pFRI->ApiType );
                    NotifyUpdateTextAndFlags(pFR);
                    break;
                }
                case ( IDCANCEL ) :
                case ( IDABORT ) :
                {
                    EndDlgSession(hDlg, pFR);
                    LocalFree(pFRI);
                    break;
                }
                case ( psh1 ) :
                case ( psh2 ) :
                {
                    UpdateTextAndFlags( hDlg,
                                        pFR,
                                        (wParam == psh1)
                                            ? FR_REPLACE
                                            : FR_REPLACEALL,
                                        pFRI->DlgType,
                                        pFRI->ApiType );
                    if (NotifyUpdateTextAndFlags(pFR) == TRUE)
                    {
                        //
                        //  Change <Cancel> button to <Close> if function
                        //  returns TRUE.
                        //  IDCANCEL instead of psh1.
                        SetWindowText( GetDlgItem(hDlg, IDCANCEL),
                                       (LPTSTR)szClose );
                    }
                    break;
                }
                case ( pshHelp ) :
                {
                    //
                    //  Call HELP app.
                    //
#ifdef UNICODE
                    if (pFRI->ApiType == COMDLG_ANSI)
                    {
                        if (msgHELPA && pFR->hwndOwner)
                        {
                            SendMessage( pFR->hwndOwner,
                                         msgHELPA,
                                         (WPARAM)hDlg,
                                         (LPARAM)pFR );
                        }
                    }
                    else
#endif
                    {
                        if (msgHELPW && pFR->hwndOwner)
                        {
                            SendMessage( pFR->hwndOwner,
                                         msgHELPW,
                                         (WPARAM)hDlg,
                                         (LPARAM)pFR );
                        }
                    }
                    break;
                }
                case ( edt1 ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        BOOL fAnythingToFind =
                            (BOOL)SendDlgItemMessage( hDlg,
                                                      edt1,
                                                      WM_GETTEXTLENGTH,
                                                      0,
                                                      0L );
                        EnableWindow(GetDlgItem(hDlg, IDOK), fAnythingToFind);
                        if (pFRI->DlgType == DLGT_REPLACE)
                        {
                            EnableWindow(GetDlgItem(hDlg, psh1), fAnythingToFind);
                            EnableWindow(GetDlgItem(hDlg, psh2), fAnythingToFind);
                        }
                    }

                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        EnableWindow( GetDlgItem(hDlg, IDOK),
                                      (BOOL)SendDlgItemMessage(
                                                   hDlg,
                                                   edt1,
                                                   WM_GETTEXTLENGTH,
                                                   0,
                                                   0L ));
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_HELP ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)aFindReplaceHelpIDs );
            }
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)aFindReplaceHelpIDs );
            }
            break;
        }
        case ( WM_CLOSE ) :
        {
            SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDCANCEL, 0, 0));
            return (TRUE);
            break;
        }
        default:
        {
            return (FALSE);
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EndDlgSession
//
//  Cleans up upon destroying the dialog.
//
////////////////////////////////////////////////////////////////////////////

VOID EndDlgSession(
   HWND hDlg,
   LPFINDREPLACE pFR)
{
    //
    //  Need to terminate regardless of app testing order ... so:
    //

    //
    //  No SUCCESS on termination.
    //
    pFR->Flags &= ~((DWORD)(FR_REPLACE | FR_FINDNEXT | FR_REPLACEALL));

    //
    //  Tell caller dialog is about to terminate.
    //
    pFR->Flags |= FR_DIALOGTERM;
    NotifyUpdateTextAndFlags(pFR);

    if (IS16BITWOWAPP(pFR))
    {
        if ((pFR->Flags & FR_ENABLEHOOK) && (pFR->lpfnHook))
        {
            (*pFR->lpfnHook)(hDlg, WM_DESTROY, 0, 0);
        }
    }

    //
    //  Free property slots.
    //
    RemoveProp(hDlg, FINDREPLACEPROP);
    DestroyWindow(hDlg);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitControlsWithFlags
//
////////////////////////////////////////////////////////////////////////////

VOID InitControlsWithFlags(
   HWND hDlg,
   LPFINDREPLACE pFR,
   UINT DlgType,
   UINT ApiType)
{
    HWND hCtl;

    //
    //  Set EDIT control to FindText.
    //
#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        SetDlgItemTextA(hDlg, edt1, (LPSTR)pFR->lpstrFindWhat);
    }
    else
#endif
    {
        SetDlgItemText(hDlg, edt1, (LPTSTR)pFR->lpstrFindWhat);
    }
    SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(edt1, 0, EN_CHANGE));

    //
    //  Set HELP push button state.
    //
    if (!(pFR->Flags & FR_SHOWHELP))
    {
        ShowWindow(hCtl = GetDlgItem(hDlg, pshHelp), SW_HIDE);
        EnableWindow(hCtl, FALSE);
    }

    //
    //  Dis/Enable check state of WHOLE WORD control.
    //
    if (pFR->Flags & FR_HIDEWHOLEWORD)
    {
        ShowWindow(hCtl = GetDlgItem(hDlg, chx1), SW_HIDE);
        EnableWindow(hCtl, FALSE);
    }
    else if (pFR->Flags & FR_NOWHOLEWORD)
    {
        EnableWindow(GetDlgItem(hDlg, chx1), FALSE);
    }
    CheckDlgButton(hDlg, chx1, (pFR->Flags & FR_WHOLEWORD) ? TRUE: FALSE);

    //
    //  Dis/Enable check state of MATCH CASE control.
    //
    if (pFR->Flags & FR_HIDEMATCHCASE)
    {
        ShowWindow(hCtl = GetDlgItem(hDlg, chx2), SW_HIDE);
        EnableWindow(hCtl, FALSE);
    }
    else if (pFR->Flags & FR_NOMATCHCASE)
    {
        EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
    }
    CheckDlgButton(hDlg, chx2, (pFR->Flags & FR_MATCHCASE) ? TRUE: FALSE);

    //
    //  Dis/Enable check state of UP/DOWN buttons.
    //
    if (pFR->Flags & FR_HIDEUPDOWN)
    {
        ShowWindow(GetDlgItem(hDlg, grp1), SW_HIDE);
        ShowWindow(hCtl = GetDlgItem(hDlg, rad1), SW_HIDE);
        EnableWindow(hCtl, FALSE);
        ShowWindow(hCtl = GetDlgItem(hDlg, rad2), SW_HIDE);
        EnableWindow(hCtl, FALSE);
    }
    else if (pFR->Flags & FR_NOUPDOWN)
    {
        EnableWindow(GetDlgItem(hDlg, rad1), FALSE);
        EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
    }

    if (DlgType == DLGT_FIND)
    {
        //
        //  Find Text only search direction setup.
        //
        CheckRadioButton( hDlg,
                          rad1,
                          rad2,
                          (pFR->Flags & FR_DOWN ? rad2 : rad1) );
    }
    else
    {
        //
        //  Replace Text only operations.
        //
#ifdef UNICODE
        if (ApiType == COMDLG_ANSI)
        {
             SetDlgItemTextA(hDlg, edt2, (LPSTR)pFR->lpstrReplaceWith);
        }
        else
#endif
        {
             SetDlgItemText(hDlg, edt2, pFR->lpstrReplaceWith);
        }
        SendMessage( hDlg,
                     WM_COMMAND,
                     GET_WM_COMMAND_MPS(edt2, 0, EN_CHANGE) );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateTextAndFlags
//
//  chx1 is whether or not to match entire words
//  chx2 is whether or not case is relevant
//  chx3 is whether or not to wrap scans
//
////////////////////////////////////////////////////////////////////////////

VOID UpdateTextAndFlags(
    HWND hDlg,
    LPFINDREPLACE pFR,
    DWORD dwActionFlag,
    UINT DlgType,
    UINT ApiType)
{
    //
    //  Only clear flags that this routine sets.  The hook and template
    //  flags should not be anded off here.
    //
    pFR->Flags &= ~((DWORD)(FR_WHOLEWORD | FR_MATCHCASE | FR_REPLACE |
                            FR_FINDNEXT | FR_REPLACEALL | FR_DOWN));
    if (IsDlgButtonChecked(hDlg, chx1))
    {
        pFR->Flags |= FR_WHOLEWORD;
    }

    if (IsDlgButtonChecked(hDlg, chx2))
    {
        pFR->Flags |= FR_MATCHCASE;
    }

    //
    //  Set ACTION flag FR_{REPLACE,FINDNEXT,REPLACEALL}.
    //
    pFR->Flags |= dwActionFlag;

#ifdef UNICODE
    if (ApiType == COMDLG_ANSI)
    {
        GetDlgItemTextA(hDlg, edt1, (LPSTR)pFR->lpstrFindWhat, pFR->wFindWhatLen);
    }
    else
#endif
    {
        GetDlgItemText(hDlg, edt1, pFR->lpstrFindWhat, pFR->wFindWhatLen);
    }

    if (DlgType == DLGT_FIND)
    {
        //
        //  Assume searching down.  Check if UP button is NOT pressed, rather
        //  than if DOWN button IS.  So, if buttons have been hidden or
        //  disabled, FR_DOWN flag will be set correctly.
        //
        if (!IsDlgButtonChecked(hDlg, rad1))
        {
            pFR->Flags |= FR_DOWN;
        }
    }
    else
    {
#ifdef UNICODE
        if (ApiType == COMDLG_ANSI)
        {
            GetDlgItemTextA( hDlg,
                             edt2,
                             (LPSTR)pFR->lpstrReplaceWith,
                             pFR->wReplaceWithLen );
        }
        else
#endif
        {
            GetDlgItemText( hDlg,
                            edt2,
                            pFR->lpstrReplaceWith,
                            pFR->wReplaceWithLen );
        }
        pFR->Flags |= FR_DOWN;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  NotifyUpdateTextAndFlags
//
////////////////////////////////////////////////////////////////////////////

LRESULT NotifyUpdateTextAndFlags(
    LPFINDREPLACE pFR)
{
    if (IS16BITWOWAPP(pFR))
    {
        return ( SendMessage( pFR->hwndOwner,
                              WM_NOTIFYWOW,
                              WMNW_UPDATEFINDREPLACE,
                              (DWORD_PTR)pFR ) );
    }
    return ( SendMessage(pFR->hwndOwner, wFRMessage, 0, (DWORD_PTR)pFR) );
}
