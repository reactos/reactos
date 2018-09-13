/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    font.c

Abstract:

    This module implements the Win32 font dialog.

Revision History:

--*/



//
//  Include Files.
//

#define NOCOMM
#define NOWH

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <port1632.h>

#include "privcomd.h"
#include "font.h"





#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ChooseFontA
//
//  ANSI entry point for ChooseFont when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseFontA(
    LPCHOOSEFONTA pCFA)
{
    LPCHOOSEFONTW pCFW;
    BOOL result;
    LPBYTE pStrMem;
    UNICODE_STRING usStyle;
    ANSI_STRING asStyle;
    INT cchTemplateName;
    FONTINFO FI;

    ZeroMemory(&FI, sizeof(FONTINFO));

    // sanity
    if (!pCFA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pCFA->lStructSize != sizeof(CHOOSEFONTA))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    //
    //  Setup and allocate CHOOSEFONTW structure.
    //
    if (!pCFA->lpLogFont && (pCFA->Flags & CF_INITTOLOGFONTSTRUCT))
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (!(pCFW = (LPCHOOSEFONTW)LocalAlloc(
                                LPTR,
                                sizeof(CHOOSEFONTW) + sizeof(LOGFONTW) )))
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    pCFW->lStructSize = sizeof(CHOOSEFONTW);

    pCFW->lpLogFont = (LPLOGFONTW)((LPCHOOSEFONTW)pCFW + 1);

    if (pCFA->Flags & CF_ENABLETEMPLATE)
    {
        if (HIWORD(pCFA->lpTemplateName))
        {
            cchTemplateName = (lstrlenA(pCFA->lpTemplateName) + 1) *
                              sizeof(WCHAR);
            if (!(pCFW->lpTemplateName = (LPWSTR)LocalAlloc( LPTR,
                                                             cchTemplateName)))
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                return (FALSE);
            }
            else
            {
                MultiByteToWideChar( CP_ACP,
                                     0,
                                     pCFA->lpTemplateName,
                                     -1,
                                     (LPWSTR)pCFW->lpTemplateName,
                                     cchTemplateName );
            }
        }
        else
        {
            (DWORD)pCFW->lpTemplateName = (DWORD)pCFA->lpTemplateName;
        }
    }
    else
    {
        pCFW->lpTemplateName = NULL;
    }

    if ((pCFA->Flags & CF_USESTYLE) && (HIWORD(pCFA->lpszStyle)))
    {
        asStyle.MaximumLength = LF_FACESIZE;
        asStyle.Length = (lstrlenA(pCFA->lpszStyle));
        if (asStyle.Length >= asStyle.MaximumLength)
        {
            asStyle.MaximumLength = asStyle.Length;
        }
    }
    else
    {
        asStyle.Length = usStyle.Length = 0;
        asStyle.MaximumLength = LF_FACESIZE;
    }
    usStyle.MaximumLength = asStyle.MaximumLength * sizeof(WCHAR);
    usStyle.Length = asStyle.Length * sizeof(WCHAR);

    if (!(pStrMem = (LPBYTE)LocalAlloc( LPTR,
                                        asStyle.MaximumLength +
                                            usStyle.MaximumLength )))
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    asStyle.Buffer = pStrMem;
    pCFW->lpszStyle = usStyle.Buffer =
        (LPWSTR)(asStyle.Buffer + asStyle.MaximumLength);

    if ((pCFA->Flags & CF_USESTYLE) && (HIWORD(pCFA->lpszStyle)))
    {
        lstrcpyA(asStyle.Buffer, pCFA->lpszStyle);
    }

    FI.pCF = pCFW;
    FI.pCFA = pCFA;
    FI.ApiType = COMDLG_ANSI;
    FI.pasStyle = &asStyle;
    FI.pusStyle = &usStyle;

    ThunkChooseFontA2W(&FI);

    if (result = ChooseFontX(&FI))
    {
        ThunkChooseFontW2A(&FI);

        //
        //  Doesn't say how many characters there are here.
        //
        if ((pCFA->Flags & CF_USESTYLE) && (HIWORD(pCFA->lpszStyle)))
        {
            LPSTR psz = pCFA->lpszStyle;
            LPSTR pszT = asStyle.Buffer;

            try
            {
                while (*psz++ = *pszT++);
            }
            except (EXCEPTION_ACCESS_VIOLATION)
            {
                //
                //  Not enough space in the passed in string.
                //
                *--psz = '\0';
            }
        }
    }

    LocalFree(pCFW);
    LocalFree(pStrMem);

    return (result);
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  ChooseFontW
//
//  Stub UNICODE function for ChooseFont when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseFontW(
   LPCHOOSEFONTW lpCFW)
{
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  ChooseFont
//
//  The ChooseFont function creates a system-defined dialog box from which
//  the user can select a font, a font style (such as bold or italic), a
//  point size, an effect (such as strikeout or underline), and a text
//  color.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI ChooseFont(
   LPCHOOSEFONT lpCF)
{
    FONTINFO FI;

    ZeroMemory(&FI, sizeof(FONTINFO));

    FI.pCF = lpCF;
    FI.ApiType = COMDLG_WIDE;

    return ( ChooseFontX(&FI) );
}


////////////////////////////////////////////////////////////////////////////
//
//  ChooseFontX
//
//  Invokes the font picker dialog, which lets the user specify common
//  character format attributes: facename, point size, text color and
//  attributes (bold, italic, strikeout or underline).
//
//  lpCF    - ptr to structure that will hold character attributes
//  ApiType - api type (COMDLG_WIDE or COMDLG_ANSI) so that the dialog
//            can remember which message to send to the user.
//
//  Returns:   TRUE  - user pressed IDOK
//             FALSE - user pressed IDCANCEL
//
////////////////////////////////////////////////////////////////////////////

BOOL ChooseFontX(
    PFONTINFO pFI)
{
    INT iRet;                    // font picker dialog return value
    HANDLE hDlgTemplate;         // handle to loaded dialog resource
    HANDLE hRes;                 // handle of res. block with dialog
    INT id;
    LPCHOOSEFONT lpCF = pFI->pCF;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    StoreExtendedError(0);
    bUserPressedCancel = FALSE;

    // sanity
    if (!lpCF)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (lpCF->lStructSize != sizeof(CHOOSEFONT))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (!lpCF->lpLogFont)
    {
        if (!(lpCF->lpLogFont = (LPLOGFONT)LocalAlloc(LPTR, sizeof(LOGFONT))))
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
            return (FALSE);
        }
    }

    //
    //  Verify that lpfnHook is not null if CF_ENABLEHOOK is specified.
    //
    if (lpCF->Flags & CF_ENABLEHOOK)
    {
        if (!lpCF->lpfnHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        lpCF->lpfnHook = NULL;
    }

    if (lpCF->Flags & CF_ENABLETEMPLATE)
    {
        //
        //  Both custom instance handle and the dialog template name are
        //  user specified. Locate the dialog resource in the specified
        //  instance block and load it.
        //
        if (!(hRes = FindResource(lpCF->hInstance, lpCF->lpTemplateName, RT_DIALOG)))
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if (!(hDlgTemplate = LoadResource(lpCF->hInstance, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }
    else if (lpCF->Flags & CF_ENABLETEMPLATEHANDLE)
    {
        //
        //  A handle to the pre-loaded resource has been specified.
        //
        hDlgTemplate = lpCF->hInstance;
    }
    else
    {
        id = FORMATDLGORD31;

        if (!(hRes = FindResource(g_hinst, MAKEINTRESOURCE(id), RT_DIALOG)))
        {
            StoreExtendedError(CDERR_FINDRESFAILURE);
            return (FALSE);
        }
        if (!(hDlgTemplate = LoadResource(g_hinst, hRes)))
        {
            StoreExtendedError(CDERR_LOADRESFAILURE);
            return (FALSE);
        }
    }

    if (LockResource(hDlgTemplate))
    {
        if (lpCF->Flags & CF_ENABLEHOOK)
        {
            glpfnFontHook = lpCF->lpfnHook;
        }

#ifdef UNICODE
        if (lpCF->Flags & CD_WOWAPP)
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        iRet = DialogBoxIndirectParamAorW( g_hinst,
                                           (LPDLGTEMPLATE)hDlgTemplate,
                                           lpCF->hwndOwner,
                                           FormatCharDlgProc,
                                           (LPARAM)pFI,
                                           uiWOWFlag );
#else
        iRet = DialogBoxIndirectParam( g_hinst,
                                       (LPDLGTEMPLATE)hDlgTemplate,
                                       lpCF->hwndOwner,
                                       FormatCharDlgProc,
                                       (LPARAM)pFI );
#endif

        glpfnFontHook = 0;

        if ((iRet == 0) && (!bUserPressedCancel) && (!GetStoredExtendedError()))
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
    }

    return (iRet == IDOK);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetStyleSelection
//
////////////////////////////////////////////////////////////////////////////

VOID SetStyleSelection(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    BOOL bInit)
{
    if (!(lpcf->Flags & CF_NOSTYLESEL))
    {
        if (bInit && (lpcf->Flags & CF_USESTYLE))
        {
            PLOGFONT plf;
            INT iSel;

            iSel = CBSetSelFromText(GetDlgItem(hDlg, cmb2), lpcf->lpszStyle);
            if (iSel >= 0)
            {
                LPITEMDATA lpItemData =
                     (LPITEMDATA)SendDlgItemMessage( hDlg,
                                                     cmb2,
                                                     CB_GETITEMDATA,
                                                     iSel,
                                                     0L );
                if (lpItemData)
                {
                    plf = lpItemData->pLogFont;

                    lpcf->lpLogFont->lfWeight = plf->lfWeight;
                    lpcf->lpLogFont->lfItalic = plf->lfItalic;
                }
                else
                {
                    lpcf->lpLogFont->lfWeight = FW_NORMAL;
                    lpcf->lpLogFont->lfItalic = 0;
                }
            }
            else
            {
                lpcf->lpLogFont->lfWeight = FW_NORMAL;
                lpcf->lpLogFont->lfItalic = 0;
            }
        }
        else
        {
            SelectStyleFromLF(GetDlgItem(hDlg, cmb2), lpcf->lpLogFont);
        }

        CBSetTextFromSel(GetDlgItem(hDlg, cmb2));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  HideDlgItem
//
////////////////////////////////////////////////////////////////////////////

VOID HideDlgItem(
    HWND hDlg,
    INT id)
{
    EnableWindow(GetDlgItem(hDlg, id), FALSE);
    ShowWindow(GetDlgItem(hDlg, id), SW_HIDE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FixComboHeights
//
//  Fixes the ownerdraw combo boxes to match the height of the non
//  ownerdraw combo boxes.
//
////////////////////////////////////////////////////////////////////////////

VOID FixComboHeights(
    HWND hDlg)
{
    INT height;

    height = SendDlgItemMessage(hDlg, cmb2, CB_GETITEMHEIGHT, (WPARAM)-1, 0L);
    SendDlgItemMessage(hDlg, cmb1, CB_SETITEMHEIGHT, (WPARAM)-1, (LONG)height);
    SendDlgItemMessage(hDlg, cmb3, CB_SETITEMHEIGHT, (WPARAM)-1, (LONG)height);
}


////////////////////////////////////////////////////////////////////////////
//
//  FormatCharDlgProc
//
//  Message handler for font dlg
//
//  chx1 - "underline" checkbox
//  chx2 - "strikeout" checkbox
//  psh4 - "help" pushbutton
//
//  On WM_INITDIALOG message, the choosefont is accessed via lParam,
//  and stored in the window's prop list.  If a hook function has been
//  specified, it is invoked AFTER the current function has processed
//  WM_INITDIALOG.
//
//  For all other messages, control is passed directly to the hook
//  function first.  Depending on the latter's return value, the message
//  is processed by this function.
//
////////////////////////////////////////////////////////////////////////////

BOOL FormatCharDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PFONTINFO pFI;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;
    HDC hDC;                      // handle to screen DC
    LPCHOOSEFONT pCF = NULL;      // ptr to struct passed to ChooseFont()
    HWND hWndHelp;                // handle to Help... pushbutton
    short nIndex;                 // at init, see if color matches
    TCHAR szPoints[10];
    HDC hdc;
    HFONT hFont;
    DWORD dw;
    BOOL bRet;
    LPTSTR lpRealFontName, lpSubFontName;
    INT iResult;
    BOOL bContinueChecking;


    //
    //  If CHOOSEFONT struct has already been accessed and if a hook
    //  function is specified, let it do the processing first.
    //
    if (pFI = (PFONTINFO)GetProp(hDlg, FONTPROP))
    {
        if ((pCF = (LPCHOOSEFONT)pFI->pCF) &&
            (pCF->lpfnHook) &&
            (bRet = (*pCF->lpfnHook)(hDlg, wMsg, wParam, lParam)))
        {
            if ((wMsg == WM_COMMAND) &&
                (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL))
            {
                //
                //  Set global flag stating that the user pressed cancel.
                //
                bUserPressedCancel = TRUE;
            }
            return (bRet);
        }
    }
    else
    {
        if (glpfnFontHook &&
            (wMsg != WM_INITDIALOG) &&
            (bRet = (* glpfnFontHook)(hDlg, wMsg, wParam, lParam)))
        {
            return (bRet);
        }
    }

    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            if (!LoadString(g_hinst, iszRegular, (LPTSTR)szRegular, CCHSTYLE) ||
                !LoadString(g_hinst, iszBold, (LPTSTR)szBold, CCHSTYLE)       ||
                !LoadString(g_hinst, iszItalic, (LPTSTR)szItalic, CCHSTYLE)   ||
                !LoadString(g_hinst, iszBoldItalic, (LPTSTR)szBoldItalic, CCHSTYLE))
            {
                StoreExtendedError(CDERR_LOADSTRFAILURE);
                EndDialog(hDlg, FALSE);
                return (FALSE);
            }

            pFI = (PFONTINFO)lParam;
            pCF = pFI->pCF;
            if ((pCF->Flags & CF_LIMITSIZE) &&
                (pCF->nSizeMax < pCF->nSizeMin))
            {
                StoreExtendedError(CFERR_MAXLESSTHANMIN);
                EndDialog(hDlg, FALSE);
                return (FALSE);
            }

            //
            //  Save ptr to CHOOSEFONT struct in the dialog's prop list.
            //  Alloc a temp LOGFONT struct to be used for the length of
            //  the dialog session, the contents of which will be copied
            //  over to the final LOGFONT (pointed to by CHOOSEFONT)
            //  only if <OK> is selected.
            //
            SetProp(hDlg, FONTPROP, (HANDLE)pFI);
            glpfnFontHook = 0;

            hDlgFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);

            if (!hbmFont)
            {
                hbmFont = LoadBitmaps(BMFONT);
            }

            if (!(pCF->Flags & CF_APPLY))
            {
                HideDlgItem(hDlg, psh3);
            }

            if (!(pCF->Flags & CF_EFFECTS))
            {
                HideDlgItem(hDlg, stc4);
                HideDlgItem(hDlg, cmb4);
            }
            else
            {
                //
                //  Fill color list.
                //
                FillColorCombo(hDlg);
                for (nIndex = CCHCOLORS - 1; nIndex > 0; nIndex--)
                {
                    dw = SendDlgItemMessage( hDlg,
                                             cmb4,
                                             CB_GETITEMDATA,
                                             nIndex,
                                             0L );
                    if (pCF->rgbColors == dw)
                    {
                        break;
                    }
                }
                SendDlgItemMessage(hDlg, cmb4, CB_SETCURSEL, nIndex, 0L);
            }

            ComputeSampleTextRectangle(hDlg);
            FixComboHeights(hDlg);

            //
            //  Init our LOGFONT.
            //
            if (!(pCF->Flags & CF_INITTOLOGFONTSTRUCT))
            {
                InitLF(pCF->lpLogFont);
            }

            //
            //  Init effects.
            //
            if (!(pCF->Flags & CF_EFFECTS))
            {
                HideDlgItem(hDlg, grp1);
                HideDlgItem(hDlg, chx1);
                HideDlgItem(hDlg, chx2);
            }
            else
            {
                CheckDlgButton(hDlg, chx1, pCF->lpLogFont->lfStrikeOut);
                CheckDlgButton(hDlg, chx2, pCF->lpLogFont->lfUnderline);
            }

            nLastFontType = 0;

            if (!GetFontFamily(hDlg, pCF->hDC, pCF->Flags))
            {
                StoreExtendedError(CFERR_NOFONTS);
                if (pCF->Flags & CF_ENABLEHOOK)
                {
                    glpfnFontHook = pCF->lpfnHook;
                }
                EndDialog(hDlg, FALSE);
                return (FALSE);
            }

            if (!(pCF->Flags & CF_NOFACESEL) && *pCF->lpLogFont->lfFaceName)
            {
                //
                //  We want to select the font the user has requested.
                //
                iResult = CBSetSelFromText( GetDlgItem(hDlg, cmb1),
                                            pCF->lpLogFont->lfFaceName );

                //
                //  If iResult == CB_ERR, then we could be working with a
                //  font subsitution name (eg: MS Shell Dlg).
                //
                if (iResult == CB_ERR)
                {
                    lpSubFontName = pCF->lpLogFont->lfFaceName;
                }

                //
                //  Allocate a buffer to store the real font name in.
                //
                lpRealFontName = GlobalAlloc (GPTR, MAX_PATH * sizeof(TCHAR));

                if (!lpRealFontName)
                {
                    StoreExtendedError(CDERR_MEMALLOCFAILURE);
                    EndDialog(hDlg, FALSE);
                    return (FALSE);
                }

                //
                //  The while loop is necessary in order to resolve
                //  substitions pointing to subsitutions.
                //     eg:  Helv->MS Shell Dlg->MS Sans Serif
                //
                bContinueChecking = TRUE;
                while ((iResult == CB_ERR) && bContinueChecking)
                {
                    bContinueChecking = LookUpFontSubs( lpSubFontName,
                                                        lpRealFontName );

                    //
                    //  If bContinueChecking is TRUE, then we have a font
                    //  name.  Try to select that in the list.
                    //
                    if (bContinueChecking)
                    {
                        iResult = CBSetSelFromText( GetDlgItem(hDlg, cmb1),
                                                    lpRealFontName );
                    }

                    lpSubFontName = lpRealFontName;
                }

                //
                //  Free our buffer.
                //
                GlobalFree(lpRealFontName);

                //
                // Set the edit control text if appropriate.
                //
                if (iResult != CB_ERR)
                {
                    CBSetTextFromSel(GetDlgItem(hDlg, cmb1));
                }
            }

            GetFontStylesAndSizes(hDlg, pCF, TRUE);

            if (!(pCF->Flags & CF_NOSTYLESEL))
            {
                SetStyleSelection(hDlg, pCF, TRUE);
            }

            if (!(pCF->Flags & CF_NOSIZESEL) && pCF->lpLogFont->lfHeight)
            {
                hdc = GetDC(NULL);
                GetPointString(szPoints, hdc, pCF->lpLogFont->lfHeight);
                ReleaseDC(NULL, hdc);
                CBSetSelFromText(GetDlgItem(hDlg, cmb3), szPoints);
                SetDlgItemText(hDlg, cmb3, szPoints);
            }

            //
            //  Hide the help button if it isn't needed.
            //
            if (!(pCF->Flags & CF_SHOWHELP))
            {
                ShowWindow (hWndHelp = GetDlgItem(hDlg, pshHelp), SW_HIDE);
                EnableWindow (hWndHelp, FALSE);
            }

            SendDlgItemMessage(hDlg, cmb1, CB_LIMITTEXT, LF_FACESIZE - 1, 0L);
            SendDlgItemMessage(hDlg, cmb2, CB_LIMITTEXT, LF_FACESIZE - 1, 0L);
            SendDlgItemMessage(hDlg, cmb3, CB_LIMITTEXT, 4, 0L);

            //
            //  If hook function has been specified, let it do any additional
            //  processing of this message.
            //
            if (pCF->lpfnHook)
            {
#ifdef UNICODE
                if (pFI->ApiType == COMDLG_ANSI)
                {
                    ThunkChooseFontW2A(pFI);
                    bRet = (*pCF->lpfnHook)( hDlg,
                                             wMsg,
                                             wParam,
                                             (LPARAM)pFI->pCFA );
                    ThunkChooseFontA2W(pFI);
                }
                else
#endif
                {
                    bRet = (*pCF->lpfnHook)( hDlg,
                                             wMsg,
                                             wParam,
                                             (LPARAM)pCF );
                }
                return (bRet);
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            break;
        }
        case ( WM_DESTROY ) :
        {
            if (pCF)
            {
                RemoveProp(hDlg, FONTPROP);
            }
            break;
        }
        case ( WM_PAINT ) :
        {
            if (BeginPaint(hDlg, &ps))
            {
                DrawSampleText(hDlg, pCF, ps.hdc);
                EndPaint(hDlg, &ps);
            }
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            hDC = GetDC(hDlg);
            hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);
            if (hFont)
            {
                hFont = SelectObject(hDC, hFont);
            }
            GetTextMetrics(hDC, &tm);
            if (hFont)
            {
                SelectObject(hDC, hFont);
            }
            ReleaseDC(hDlg, hDC);

            if (((LPMEASUREITEMSTRUCT)lParam)->itemID != -1)
            {
                ((LPMEASUREITEMSTRUCT)lParam)->itemHeight =
                       max(tm.tmHeight, DY_BITMAP);
            }
            else
            {
                //
                //  This is for 3.0 only.  In 3.1, the CB_SETITEMHEIGHT
                //  will fix this.  Note, this is off by one on 8514.
                //
                ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = tm.tmHeight + 1;
            }

            break;
        }

        case ( WM_DRAWITEM ) :
        {
#define lpdis ((LPDRAWITEMSTRUCT)lParam)

            if (lpdis->itemID == (UINT)-1)
            {
                break;
            }

            if (lpdis->CtlID == cmb4)
            {
                DrawColorComboItem(lpdis);
            }
            else if (lpdis->CtlID == cmb1)
            {
                DrawFamilyComboItem(lpdis);
            }
            else
            {
                DrawSizeComboItem(lpdis);
            }
            break;

#undef lpdis
        }
        case ( WM_SYSCOLORCHANGE ) :
        {
            DeleteObject(hbmFont);
            hbmFont = LoadBitmaps(BMFONT);
            break;
        }
        case ( WM_COMMAND ) :
        {
            return (ProcessDlgCtrlCommand(hDlg, pFI, wParam, lParam));
            break;
        }
        default :
        {
            if ((wMsg == WM_CHOOSEFONT_GETLOGFONT) ||
                (wMsg == msgWOWCHOOSEFONT_GETLOGFONT))
            {
#ifdef UNICODE
                if (pFI->ApiType == COMDLG_ANSI)
                {
                    LOGFONT lf;
                    BOOL bRet;

                    bRet = FillInFont(hDlg, pCF, &lf, TRUE);

                    ThunkLogFontW2A(&lf, (LPLOGFONTA)lParam);

                    return (bRet);
                }
                else
#endif
                {
                    return (FillInFont(hDlg, pCF, (LPLOGFONT)lParam, TRUE));
                }
            }

            return (FALSE);
        }
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SelectStyleFromLF
//
//  Given a logfont, selects the closest match in the style list.
//
////////////////////////////////////////////////////////////////////////////

void SelectStyleFromLF(
    HWND hwnd,
    LPLOGFONT lplf)
{
    int i, count, iSel;
    PLOGFONT plf;
    int weight_delta, best_weight_delta = 1000;
    BOOL bIgnoreItalic;
    LPITEMDATA lpItemData;


    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
    iSel = 0;
    bIgnoreItalic = FALSE;

TryAgain:
    for (i = 0; i < count; i++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, i, 0L);

        if (lpItemData)
        {
            plf = lpItemData->pLogFont;

            if (bIgnoreItalic ||
                (plf->lfItalic && lplf->lfItalic) ||
                (!plf->lfItalic && !lplf->lfItalic))
            {
                weight_delta = lplf->lfWeight - plf->lfWeight;
                if (weight_delta < 0)
                {
                    weight_delta = -weight_delta;
                }

                if (weight_delta < best_weight_delta)
                {
                    best_weight_delta = weight_delta;
                    iSel = i;
                }
            }
        }
    }
    if (!bIgnoreItalic && iSel == 0)
    {
        bIgnoreItalic = TRUE;
        goto TryAgain;
    }

    SendMessage(hwnd, CB_SETCURSEL, iSel, 0L);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBSetTextFromSel
//
//  Makes the currently selected item the edit text for a combo box.
//
////////////////////////////////////////////////////////////////////////////

INT CBSetTextFromSel(
    HWND hwnd)
{
    INT iSel;
    TCHAR szFace[LF_FACESIZE];

    iSel = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0L);
    if (iSel >= 0)
    {
        SendMessage(hwnd, CB_GETLBTEXT, iSel, (LONG)(LPTSTR)szFace);
        SetWindowText(hwnd, szFace);
    }
    return (iSel);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBSetSelFromText
//
//  Sets the selection based on lpszString.  Sends notification messages
//  if bNotify is TRUE.
//
////////////////////////////////////////////////////////////////////////////

INT CBSetSelFromText(
    HWND hwnd,
    LPTSTR lpszString)
{
    INT iInd;

    iInd = CBFindString(hwnd, lpszString);

    if (iInd >= 0)
    {
        SendMessage(hwnd, CB_SETCURSEL, iInd, 0L);
    }
    return (iInd);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBGetTextAndData
//
//  Returns the text and item data for a combo box based on the current
//  edit text.  If the current edit text does not match anything in the
//  listbox, then CB_ERR is returned.
//
////////////////////////////////////////////////////////////////////////////

INT CBGetTextAndData(
    HWND hwnd,
    LPTSTR lpszString,
    INT iSize,
    LPDWORD lpdw)
{
    INT iSel;

    GetWindowText(hwnd, lpszString, iSize);
    iSel = CBFindString(hwnd, lpszString);
    if (iSel < 0)
    {
        return (iSel);
    }

    *lpdw = SendMessage(hwnd, CB_GETITEMDATA, iSel, 0L);
    return (iSel);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBFindString
//
//  Does an exact string find and returns the index.
//
////////////////////////////////////////////////////////////////////////////

INT CBFindString(
    HWND hwnd,
    LPTSTR lpszString)
{
    return (SendMessage( hwnd,
                         CB_FINDSTRINGEXACT,
                         (WPARAM)-1,
                         (LPARAM)(LPCSTR)lpszString ));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPointSizeInRange
//
//  Ensures that the point size edit field is in range.
//
//  Returns:  Point Size - of the edit field limitted by MIN/MAX size
//            0          - if the field is empty
//
////////////////////////////////////////////////////////////////////////////

#define GPS_COMPLAIN    0x0001
#define GPS_SETDEFSIZE  0x0002

BOOL GetPointSizeInRange(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPINT pts,
    WORD wFlags)
{
    TCHAR szBuffer[90];
    TCHAR szTitle[90];
    int nTmp;
    BOOL bOK;

    *pts = 0;

    if (GetDlgItemText(hDlg, cmb3, szBuffer, sizeof(szBuffer) / sizeof(TCHAR)))
    {
        nTmp = GetDlgItemInt(hDlg, cmb3, &bOK, TRUE);
        if (!bOK)
        {
            nTmp = 0;
        }
    }
    else if (wFlags & GPS_SETDEFSIZE)
    {
        nTmp = DEF_POINT_SIZE;
        bOK = TRUE;
    }
    else
    {
        //
        //  We're just returning with 0 in *pts.
        //
        return (FALSE);
    }

    //
    //  Check that we got a number in range.
    //
    if (wFlags & GPS_COMPLAIN)
    {
        if ((lpcf->Flags & CF_LIMITSIZE) &&
            (!bOK || nTmp > lpcf->nSizeMax || nTmp < lpcf->nSizeMin))
        {
            bOK = FALSE;
            LoadString( g_hinst,
                        iszSizeRange,
                        szTitle,
                        sizeof(szTitle) / sizeof(TCHAR) );

            wsprintf( (LPTSTR)szBuffer,
                      (LPTSTR)szTitle,
                      lpcf->nSizeMin,
                      lpcf->nSizeMax );
        }
        else if (!bOK)
        {
            LoadString( g_hinst,
                        iszSizeNumber,
                        szBuffer,
                        sizeof(szBuffer) / sizeof(TCHAR) );
        }

        if (!bOK)
        {
            GetWindowText(hDlg, szTitle, sizeof(szTitle) / sizeof(TCHAR));
            MessageBox(hDlg, szBuffer, szTitle, MB_OK | MB_ICONINFORMATION);
            return (FALSE);
        }
    }

    *pts = nTmp;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ProcessDlgCtrlCommand
//
//  Handles all WM_COMMAND messages for the font dialog.
//
//  cmb1 - ID of font facename combobox
//  cmb2 - style
//  cmb3 - size
//  chx1 - "Underline" checkbox
//  chx2 - "Strikeout" checkbox
//  stc5 - frame around text preview area
//  psh4 - button that invokes the Help application
//  IDOK - OK button to end dialog, retaining information
//  IDCANCEL - button to cancel dialog, not doing anything
//
//  Returns:   TRUE    - if message is processed successfully
//             FALSE   - otherwise
//
////////////////////////////////////////////////////////////////////////////

BOOL ProcessDlgCtrlCommand(
    HWND hDlg,
    PFONTINFO pFI,
    WPARAM wParam,
    LPARAM lParam)
{
    INT iSel;
    LPCHOOSEFONT pCF = (pFI ? pFI->pCF : NULL);
    TCHAR szPoints[10];
    TCHAR szStyle[LF_FACESIZE];
    LPITEMDATA lpItemData;
    WORD wCmbId;
    TCHAR szMsg[160], szTitle[160];


    if (pCF)
    {
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case ( IDABORT ) :
            {
                //
                //  This is how a hook can cause the dialog to go away.
                //
                FreeFonts(GetDlgItem(hDlg, cmb2));
                if (pCF->Flags & CF_ENABLEHOOK)
                {
                    glpfnFontHook = pCF->lpfnHook;
                }
                EndDialog(hDlg, (BOOL)GET_WM_COMMAND_HWND(wParam, lParam));
                break;
            }
            case ( IDOK ) :
            {
                //
                //  Make sure the focus is set to the OK button.  Must do
                //  this so that when the user presses Enter from one of
                //  the combo boxes, the kill focus processing is done
                //  before the data is captured.
                //
                SetFocus(GetDlgItem(hDlg, IDOK));

                if (!GetPointSizeInRange( hDlg,
                                          pCF,
                                          &iSel,
                                          GPS_COMPLAIN | GPS_SETDEFSIZE))
                {
                    PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, cmb3), 1L);
                    break;
                }
                pCF->iPointSize = iSel * 10;

                FillInFont(hDlg, pCF, pCF->lpLogFont, TRUE);

                if (pCF->Flags & CF_FORCEFONTEXIST)
                {
                    if (pCF->Flags & CF_NOFACESEL)
                    {
                        wCmbId = cmb1;
                    }
                    else if (pCF->Flags & CF_NOSTYLESEL)
                    {
                        wCmbId = cmb2;
                    }
                    else
                    {
                        wCmbId = 0;
                    }

                    if (wCmbId)
                    {
                        //
                        //  Error found.
                        //
                        LoadString( g_hinst,
                                    (wCmbId == cmb1)
                                        ? iszNoFaceSel
                                        : iszNoStyleSel,
                                    szMsg,
                                    sizeof(szMsg) / sizeof(TCHAR) );
                        GetWindowText( hDlg,
                                       szTitle,
                                       sizeof(szTitle) / sizeof(TCHAR) );
                        MessageBox( hDlg,
                                    szMsg,
                                    szTitle,
                                    MB_OK | MB_ICONINFORMATION );
                        PostMessage( hDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hDlg, wCmbId),
                                     1L );
                        break;
                    }
                }

                if (pCF->Flags & CF_EFFECTS)
                {
                    //
                    //  Get currently selected item in color combo box and
                    //  the 32 bit color rgb value associated with it.
                    //
                    iSel = (int)SendDlgItemMessage( hDlg,
                                                    cmb4,
                                                    CB_GETCURSEL,
                                                    0,
                                                    0L );
                    pCF->rgbColors = SendDlgItemMessage( hDlg,
                                                         cmb4,
                                                         CB_GETITEMDATA,
                                                         iSel,
                                                         0L );
                }

                iSel = CBGetTextAndData( GetDlgItem(hDlg, cmb2),
                                         szStyle,
                                         sizeof(szStyle) / sizeof(TCHAR),
                                         (LPDWORD)&lpItemData );
                if (iSel >=0 && lpItemData)
                {
                    pCF->nFontType = (WORD)lpItemData->nFontType;
                }
                else
                {
                    pCF->nFontType = 0;
                }

                if (pCF->Flags & CF_USESTYLE)
                {
                    lstrcpy(pCF->lpszStyle, szStyle);
                }

                goto LeaveDialog;
            }
            case ( IDCANCEL ) :
            {
                bUserPressedCancel = TRUE;

LeaveDialog:
                FreeFonts(GetDlgItem(hDlg, cmb2));
                if (pCF->Flags & CF_ENABLEHOOK)
                {
                    glpfnFontHook = pCF->lpfnHook;
                }
                EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam) == IDOK);
                break;
            }
            case ( cmb1 ) :                 // facenames combobox
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case ( CBN_SELCHANGE ) :
                    {
                        CBSetTextFromSel(GET_WM_COMMAND_HWND(wParam, lParam));
FillStyles:
                        //
                        //  Try to maintain the current point size and style.
                        //
                        GetDlgItemText( hDlg,
                                        cmb3,
                                        szPoints,
                                        sizeof(szPoints) / sizeof(TCHAR) );
                        GetFontStylesAndSizes(hDlg, pCF, FALSE);
                        SetStyleSelection(hDlg, pCF, FALSE);

                        //
                        //  Preserve the point size selection or put it in
                        //  the edit control if it is not in the list for
                        //  this font.
                        //
                        iSel = CBFindString(GetDlgItem(hDlg, cmb3), szPoints);
                        if (iSel < 0)
                        {
                            SetDlgItemText(hDlg, cmb3, szPoints);
                        }
                        else
                        {
                            SendDlgItemMessage( hDlg,
                                                cmb3,
                                                CB_SETCURSEL,
                                                iSel,
                                                0L );
                        }

                        goto DrawSample;
                        break;
                    }
                    case ( CBN_EDITUPDATE ) :
                    {
                        PostMessage( hDlg,
                                     WM_COMMAND,
                                     GET_WM_COMMAND_MPS(
                                         GET_WM_COMMAND_ID(wParam, lParam),
                                         GET_WM_COMMAND_HWND(wParam, lParam),
                                         CBN_MYEDITUPDATE ) );
                        break;
                    }
                    case ( CBN_MYEDITUPDATE ) :
                    {
                        GetWindowText( GET_WM_COMMAND_HWND(wParam, lParam),
                                       szStyle,
                                       sizeof(szStyle) / sizeof(TCHAR) );
                        iSel = CBFindString( GET_WM_COMMAND_HWND(wParam, lParam),
                                             szStyle );
                        if (iSel >= 0)
                        {
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETCURSEL,
                                         (WPARAM)iSel,
                                         0L );
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETEDITSEL,
                                         0,
                                         0x0000FFFF );
                            goto FillStyles;
                        }
                        break;
                    }
                }
                break;
            }
            case ( cmb2 ) :                 // styles combobox
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case ( CBN_EDITUPDATE ) :
                    {
                        PostMessage( hDlg,
                                     WM_COMMAND,
                                     GET_WM_COMMAND_MPS(
                                         GET_WM_COMMAND_ID(wParam, lParam),
                                         GET_WM_COMMAND_HWND(wParam, lParam),
                                         CBN_MYEDITUPDATE ) );
                        break;
                    }
                    case CBN_MYEDITUPDATE:
                    {
                        GetWindowText( GET_WM_COMMAND_HWND(wParam, lParam),
                                       szStyle,
                                       sizeof(szStyle) / sizeof(TCHAR) );
                        iSel = CBFindString( GET_WM_COMMAND_HWND(wParam, lParam),
                                             szStyle );
                        if (iSel >= 0)
                        {
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETCURSEL,
                                         iSel,
                                         0L );
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETEDITSEL,
                                         0,
                                         0x0000FFFF );
                            goto DrawSample;
                        }
                        break;
                    }
                    case ( CBN_SELCHANGE ) :
                    {
                        iSel = CBSetTextFromSel(GET_WM_COMMAND_HWND(wParam, lParam));

                        //
                        //  Make the style selection stick.
                        //
                        if (iSel >= 0)
                        {
                            LPITEMDATA lpItemData;
                            PLOGFONT plf;

                            lpItemData = (LPITEMDATA)SendMessage(
                                            GET_WM_COMMAND_HWND(wParam, lParam),
                                            CB_GETITEMDATA,
                                            iSel,
                                            0L );

                            if (lpItemData)
                            {
                                plf = lpItemData->pLogFont;
                                pCF->lpLogFont->lfWeight = plf->lfWeight;
                                pCF->lpLogFont->lfItalic = plf->lfItalic;
                            }
                            else
                            {
                                pCF->lpLogFont->lfWeight = FW_NORMAL;
                                pCF->lpLogFont->lfItalic = 0;
                            }
                        }

                        goto DrawSample;
                    }
                    case ( CBN_KILLFOCUS ) :
                    {
DrawSample:
#ifdef UNICODE
                        if (pFI->ApiType == COMDLG_ANSI)
                        {
                            //
                            //  Send special WOW message to indicate the
                            //  font style has changed.
                            //
                            FillInFont(hDlg, pCF, pCF->lpLogFont, TRUE);
                            ThunkLogFontW2A(pCF->lpLogFont, pFI->pCFA->lpLogFont);
                            SendMessage(hDlg, msgWOWLFCHANGE, 0, (LPARAM)(LPLOGFONT)pFI->pCFA->lpLogFont);
                        }
#endif

                        //
                        //  Force redraw of preview text for any size change.
                        //
                        InvalidateRect(hDlg, &rcText, FALSE);
                        UpdateWindow(hDlg);
                    }
                }
                break;
            }
            case ( cmb3 ) :                 // point sizes combobox
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case ( CBN_EDITUPDATE ) :
                    {
                        PostMessage( hDlg,
                                     WM_COMMAND,
                                     GET_WM_COMMAND_MPS(
                                         GET_WM_COMMAND_ID(wParam, lParam),
                                         GET_WM_COMMAND_HWND(wParam, lParam),
                                         CBN_MYEDITUPDATE ) );
                        break;
                    }
                    case ( CBN_MYEDITUPDATE ) :
                    {
                        GetWindowText( GET_WM_COMMAND_HWND(wParam, lParam),
                                       szStyle,
                                       sizeof(szStyle) / sizeof(TCHAR) );
                        iSel = CBFindString( GET_WM_COMMAND_HWND(wParam, lParam),
                                             szStyle );
                        if (iSel >= 0)
                        {
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETCURSEL,
                                         iSel,
                                         0L );
                            SendMessage( GET_WM_COMMAND_HWND(wParam, lParam),
                                         CB_SETEDITSEL,
                                         0,
                                         0x0000FFFF );
                            goto DrawSample;
                        }
                        break;
                    }
                    case ( CBN_SELCHANGE ) :
                    {
                        iSel = CBSetTextFromSel(GET_WM_COMMAND_HWND(wParam, lParam));
                        goto DrawSample;
                    }
                    case ( CBN_KILLFOCUS ) :
                    {
                        goto DrawSample;
                    }
                }
                break;
            }
            case ( cmb4 ) :
            {
                if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_SELCHANGE)
                {
                    break;
                }

                // fall thru...
            }
            case ( chx1 ) :                 // bold
            case ( chx2 ) :                 // italic
            {
                goto DrawSample;
            }
            case ( pshHelp ) :              // help
            {
#ifdef UNICODE
                if (pFI->ApiType == COMDLG_ANSI)
                {
                    if (msgHELPA && pCF->hwndOwner)
                    {
                        SendMessage( pCF->hwndOwner,
                                     msgHELPA,
                                     (WPARAM)hDlg,
                                     (LPARAM)pCF );
                    }
                }
                else
#endif
                {
                    if (msgHELPW && pCF->hwndOwner)
                    {
                        SendMessage( pCF->hwndOwner,
                                     msgHELPW,
                                     (WPARAM)hDlg,
                                     (LPARAM)pCF );
                    }
                }
                break;
            }
            default :
            {
                return (FALSE);
            }
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CmpFontType
//
//  Compares two font types.  The values of the font type bits are
//  monotonic except the low bit (RASTER_FONTTYPE).  After flipping
//  that bit the words can be compared directly.
//
//  Returns the best of the two.
//
////////////////////////////////////////////////////////////////////////////

INT CmpFontType(
    DWORD ft1,
    DWORD ft2)
{
    ft1 &= ~(SCREEN_FONTTYPE | PRINTER_FONTTYPE);
    ft2 &= ~(SCREEN_FONTTYPE | PRINTER_FONTTYPE);

    //
    //  Flip the RASTER_FONTTYPE bit so we can compare.
    //
    ft1 ^= RASTER_FONTTYPE;
    ft2 ^= RASTER_FONTTYPE;

    return ( (INT)ft1 - (INT)ft2 );
}


////////////////////////////////////////////////////////////////////////////
//
//  FontFamilyEnumProc
//
//  nFontType bits
//
//  SCALABLE DEVICE RASTER
//     (TT)  (not GDI) (not scalable)
//      0       0       0       vector, ATM screen
//      0       0       1       GDI raster font
//      0       1       0       PS/LJ III, ATM printer, ATI/LaserMaster
//      0       1       1       non scalable device font
//      1       0       x       TT screen font
//      1       1       x       TT dev font
//
////////////////////////////////////////////////////////////////////////////

INT FontFamilyEnumProc(
    LPLOGFONT lplf,
    LPTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData)
{
    INT iItem;
    DWORD nOldType, nNewType;
    LPITEMDATA lpItemData;

    //
    //  Bounce non TT fonts.
    //
    if ((lpData->dwFlags & CF_TTONLY) &&
        !(nFontType & TRUETYPE_FONTTYPE))
    {
        return (TRUE);
    }

    //
    //  Bounce non scalable fonts.
    //
    if ((lpData->dwFlags & CF_SCALABLEONLY) &&
        (nFontType & RASTER_FONTTYPE))
    {
        return (TRUE);
    }

    //
    //  Bounce non ANSI fonts.
    //
    if ((lpData->dwFlags & CF_ANSIONLY) &&
        (lplf->lfCharSet != ANSI_CHARSET))
    {
        return (TRUE);
    }

    //
    //  Bounce proportional fonts.
    //
    if ((lpData->dwFlags & CF_FIXEDPITCHONLY) &&
        (lplf->lfPitchAndFamily & VARIABLE_PITCH))
    {
        return (TRUE);
    }

    //
    //  Bounce vector fonts.
    //
    if ((lpData->dwFlags & CF_NOVECTORFONTS) &&
        (lplf->lfCharSet == OEM_CHARSET))
    {
        return (TRUE);
    }

    if (lpData->bPrinterFont)
    {
        nFontType |= PRINTER_FONTTYPE;
    }
    else
    {
        nFontType |= SCREEN_FONTTYPE;
    }

    //
    //  Test for a name collision.
    //
    iItem = CBFindString(lpData->hwndFamily, lplf->lfFaceName);
    if (iItem >= 0)
    {
        lpItemData = (LPITEMDATA)SendMessage( lpData->hwndFamily,
                                              CB_GETITEMDATA,
                                              iItem,
                                              0L );
        if (lpItemData)
        {
            nOldType = lpItemData->nFontType;
        }
        else
        {
            nOldType = 0;
        }

        //
        //  If we don't want screen fonts, but do want printer fonts,
        //  the old font is a screen font and the new font is a
        //  printer font, take the new font regardless of other flags.
        //  Note that this means if a printer wants TRUETYPE fonts, it
        //  should enumerate them.
        //
        if (!(lpData->dwFlags & CF_SCREENFONTS)  &&
             (lpData->dwFlags & CF_PRINTERFONTS) &&
             (nFontType & PRINTER_FONTTYPE)      &&
             (nOldType & SCREEN_FONTTYPE))
        {
            nOldType = 0;                   // for setting nNewType below
            goto SetNewType;
        }

        if (CmpFontType(nFontType, nOldType) > 0)
        {
SetNewType:
            nNewType = nFontType;
            SendMessage( lpData->hwndFamily,
                         CB_INSERTSTRING,
                         iItem,
                         (LONG)(LPTSTR)lplf->lfFaceName );
            SendMessage( lpData->hwndFamily,
                         CB_DELETESTRING,
                         iItem + 1,
                         0L );
        }
        else
        {
            nNewType = nOldType;
        }

        //
        //  Accumulate the printer/screen ness of these fonts.
        //
        nNewType |= (nFontType | nOldType) &
                    (SCREEN_FONTTYPE | PRINTER_FONTTYPE);

        lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
        if (!lpItemData)
        {
            return (FALSE);
        }
        lpItemData->pLogFont = 0L;
        lpItemData->nFontType = nNewType;
        SendMessage( lpData->hwndFamily,
                     CB_SETITEMDATA,
                     iItem,
                     (LONG)lpItemData );

        return (TRUE);
    }

    iItem = (int)SendMessage( lpData->hwndFamily,
                              CB_ADDSTRING,
                              0,
                              (LONG)(LPTSTR)lplf->lfFaceName );
    if (iItem < 0)
    {
        return (FALSE);
    }

    lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
    if (!lpItemData)
    {
        return (FALSE);
    }
    lpItemData->pLogFont = 0L;
    lpItemData->nFontType = nFontType;
    SendMessage(lpData->hwndFamily, CB_SETITEMDATA, iItem, (LONG)lpItemData);

    lptm;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFontFamily
//
//  Fills the screen and/or printer font facenames into the font facenames
//  combobox depending on the CF_?? flags passed in.
//
//  cmb1 is the ID for the font facename combobox
//
//  Both screen and printer fonts are listed into the same combobox
//
//  Returns:   TRUE    if successful
//             FALSE   otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFontFamily(
    HWND hDlg,
    HDC hDC,
    DWORD dwEnumCode)
{
    ENUM_FONT_DATA data;
    INT iItem, iCount;
    DWORD nFontType;
    TCHAR szMsg[100], szTitle[40];
    LPITEMDATA lpItemData;

    data.hwndFamily = GetDlgItem(hDlg, cmb1);
    data.dwFlags = dwEnumCode;

    //
    //  This is a bit strange.  We have to get all the screen fonts
    //  so if they ask for the printer fonts we can tell which
    //  are really printer fonts.  This is so we don't list the
    //  vector and raster fonts as printer device fonts.
    //
    data.hDC = GetDC(NULL);
    data.bPrinterFont = FALSE;
    EnumFontFamilies( data.hDC,
                      NULL,
                      (FONTENUMPROC)FontFamilyEnumProc,
                      (LPARAM)&data );
    ReleaseDC(NULL, data.hDC);

    //
    //  List out printer font facenames.
    //
    if (dwEnumCode & CF_PRINTERFONTS)
    {
        data.hDC = hDC;
        data.bPrinterFont = TRUE;
        EnumFontFamilies( hDC,
                          NULL,
                          (FONTENUMPROC)FontFamilyEnumProc,
                          (LPARAM)&data );
    }

    //
    //  Now we have to remove those screen fonts if they didn't
    //  ask for them.
    //
    if (!(dwEnumCode & CF_SCREENFONTS))
    {
        iCount = (int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L);

        for (iItem = iCount - 1; iItem >= 0; iItem--)
        {
            lpItemData = (LPITEMDATA)SendMessage( data.hwndFamily,
                                                  CB_GETITEMDATA,
                                                  iItem,
                                                  0L );
            if (lpItemData)
            {
                nFontType = lpItemData->nFontType;
            }
            else
            {
                nFontType = 0;
            }

            if ((nFontType & (SCREEN_FONTTYPE |
                              PRINTER_FONTTYPE)) == SCREEN_FONTTYPE)
            {
                SendMessage(data.hwndFamily, CB_DELETESTRING, iItem, 0L);
            }
        }
    }

    //
    //  For WYSIWYG mode we delete all the fonts that don't exist
    //  on the screen and the printer.
    //
    if (dwEnumCode & CF_WYSIWYG)
    {
        iCount = (int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L);

        for (iItem = iCount - 1; iItem >= 0; iItem--)
        {
            nFontType = ((LPITEMDATA)SendMessage( data.hwndFamily,
                                                  CB_GETITEMDATA,
                                                  iItem,
                                                  0L ))->nFontType;

            if ((nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE)) !=
                (SCREEN_FONTTYPE | PRINTER_FONTTYPE))
            {
                SendMessage(data.hwndFamily, CB_DELETESTRING, iItem, 0L);
            }
        }
    }

    if ((int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L) <= 0)
    {
        LoadString( g_hinst,
                    iszNoFontsTitle,
                    szTitle,
                    sizeof(szTitle) / sizeof(TCHAR) );
        LoadString( g_hinst,
                    iszNoFontsMsg,
                    szMsg,
                    sizeof(szMsg) / sizeof(TCHAR) );
        MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_ICONINFORMATION);

        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CBAddSize
//
////////////////////////////////////////////////////////////////////////////

VOID CBAddSize(
    HWND hwnd,
    INT pts,
    LPCHOOSEFONT lpcf)
{
    INT iInd;
    TCHAR szSize[10];
    INT count, test_size;
    LPITEMDATA lpItemData;

    if ((lpcf->Flags & CF_LIMITSIZE) &&
        ((pts > lpcf->nSizeMax) || (pts < lpcf->nSizeMin)))
    {
        return;
    }

    wsprintf(szSize, szPtFormat, pts);

    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

    test_size = -1;

    for (iInd = 0; iInd < count; iInd++)
    {
        lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, iInd, 0L);
        if (lpItemData)
        {
            test_size = (int)lpItemData->nFontType;
        }
        else
        {
            test_size = 0;
        }

        if (pts <= test_size)
        {
            break;
        }
    }

    //
    //  Don't add duplicates.
    //
    if (pts == test_size)
    {
        return;
    }

    iInd = SendMessage(hwnd, CB_INSERTSTRING, iInd, (LONG)(LPTSTR)szSize);
    if (iInd >= 0)
    {
        lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
        if (!lpItemData)
        {
            return;
        }

        lpItemData->nFontType = (DWORD)pts;
        lpItemData->pLogFont = 0L;
        SendMessage(hwnd, CB_SETITEMDATA, iInd, (LONG)lpItemData);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  InsertStyleSorted
//
//  Sort styles by weight first, then by italics.
//
//  Returns the index of the place this was inserted.
//
////////////////////////////////////////////////////////////////////////////

INT InsertStyleSorted(
    HWND hwnd,
    LPTSTR lpszStyle,
    LPLOGFONT lplf)
{
    INT count, i;
    PLOGFONT plf;
    LPITEMDATA lpItemData;

    count = SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

    for (i = 0; i < count; i++)
    {
        if (lpItemData = ((LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, i, 0L)))
        {
            plf = lpItemData->pLogFont;

            if (lplf->lfWeight < plf->lfWeight)
            {
                break;
            }
            else if (lplf->lfWeight == plf->lfWeight)
            {
                if (lplf->lfItalic && !plf->lfItalic)
                {
                    i++;
                }
                break;
            }
        }
    }
    return ((INT)SendMessage(hwnd, CB_INSERTSTRING, i, (LONG)lpszStyle));
}


////////////////////////////////////////////////////////////////////////////
//
//  CBAddStyle
//
////////////////////////////////////////////////////////////////////////////

PLOGFONT CBAddStyle(
    HWND hwnd,
    LPTSTR lpszStyle,
    DWORD nFontType,
    LPLOGFONT lplf)
{
    INT iItem;
    PLOGFONT plf;
    LPITEMDATA lpItemData;

    //
    //  don't add duplicates.
    //
    if (CBFindString(hwnd, lpszStyle) >= 0)
    {
        return (NULL);
    }

    iItem = (int)InsertStyleSorted(hwnd, lpszStyle, lplf);
    if (iItem < 0)
    {
        return (NULL);
    }

    plf = (PLOGFONT)LocalAlloc(LMEM_FIXED, sizeof(LOGFONT));
    if (!plf)
    {
        SendMessage(hwnd, CB_DELETESTRING, iItem, 0L);
        return (NULL);
    }

    *plf = *lplf;

    lpItemData = (LPITEMDATA)LocalAlloc(LMEM_FIXED, sizeof(ITEMDATA));
    if (!lpItemData)
    {
        SendMessage(hwnd, CB_DELETESTRING, iItem, 0L);
        return (NULL);
    }

    lpItemData->pLogFont = plf;
    lpItemData->nFontType = nFontType;
    SendMessage(hwnd, CB_SETITEMDATA, iItem, (LONG)lpItemData);

    return (plf);
}


////////////////////////////////////////////////////////////////////////////
//
//  FillInMissingStyles
//
//  Generates simulated forms from those that we have.
//
//  reg -> bold
//  reg -> italic
//  bold || italic || reg -> bold italic
//
////////////////////////////////////////////////////////////////////////////

VOID FillInMissingStyles(
    HWND hwnd)
{
    PLOGFONT plf, plf_reg, plf_bold, plf_italic;
    DWORD nFontType;
    INT i, count;
    BOOL bBold, bItalic, bBoldItalic;
    LPITEMDATA lpItemData;
    LOGFONT lf;

    bBold = bItalic = bBoldItalic = FALSE;
    plf_reg = plf_bold = plf_italic = NULL;

    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
    for (i = 0; i < count; i++)
    {
        if (lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, i, 0L))
        {
            plf = lpItemData->pLogFont;
            nFontType = lpItemData->nFontType;
        }
        else
        {
            plf = NULL;
            nFontType = 0;
        }

        if ((nFontType & BOLD_FONTTYPE) && (nFontType & ITALIC_FONTTYPE))
        {
            bBoldItalic = TRUE;
        }
        else if (nFontType & BOLD_FONTTYPE)
        {
            bBold = TRUE;
            plf_bold = plf;
        }
        else if (nFontType & ITALIC_FONTTYPE)
        {
            bItalic = TRUE;
            plf_italic = plf;
        }
        else
        {
            plf_reg = plf;
        }
    }

    nFontType |= SIMULATED_FONTTYPE;

    if (!bBold && plf_reg)
    {
        lf = *plf_reg;
        lf.lfWeight = FW_BOLD;
        CBAddStyle(hwnd, szBold, (nFontType | BOLD_FONTTYPE), &lf);
    }

    if (!bItalic && plf_reg)
    {
        lf = *plf_reg;
        lf.lfItalic = TRUE;
        CBAddStyle(hwnd, szItalic, (nFontType | ITALIC_FONTTYPE), &lf);
    }
    if (!bBoldItalic && (plf_bold || plf_italic || plf_reg))
    {
        if (plf_italic)
        {
            plf = plf_italic;
        }
        else if (plf_bold)
        {
            plf = plf_bold;
        }
        else
        {
            plf = plf_reg;
        }

        lf = *plf;
        lf.lfItalic = (BYTE)TRUE;
        lf.lfWeight = FW_BOLD;
        CBAddStyle(hwnd, szBoldItalic, (nFontType | BOLD_FONTTYPE | ITALIC_FONTTYPE), &lf);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FillScalableSizes
//
////////////////////////////////////////////////////////////////////////////

VOID FillScalableSizes(
    HWND hwnd,
    LPCHOOSEFONT lpcf)
{
    CBAddSize(hwnd, 8,  lpcf);
    CBAddSize(hwnd, 9,  lpcf);
    CBAddSize(hwnd, 10, lpcf);
    CBAddSize(hwnd, 11, lpcf);
    CBAddSize(hwnd, 12, lpcf);
    CBAddSize(hwnd, 14, lpcf);
    CBAddSize(hwnd, 16, lpcf);
    CBAddSize(hwnd, 18, lpcf);
    CBAddSize(hwnd, 20, lpcf);
    CBAddSize(hwnd, 22, lpcf);
    CBAddSize(hwnd, 24, lpcf);
    CBAddSize(hwnd, 26, lpcf);
    CBAddSize(hwnd, 28, lpcf);
    CBAddSize(hwnd, 36, lpcf);
    CBAddSize(hwnd, 48, lpcf);
    CBAddSize(hwnd, 72, lpcf);
}


////////////////////////////////////////////////////////////////////////////
//
//  FontStyleEnumProc
//
////////////////////////////////////////////////////////////////////////////

#define GDI_FONTTYPE_STUFF (RASTER_FONTTYPE | DEVICE_FONTTYPE | TRUETYPE_FONTTYPE)

INT FontStyleEnumProc(
    LPLOGFONT lplf,
    LPNEWTEXTMETRIC lptm,
    DWORD nFontType,
    LPENUM_FONT_DATA lpData)
{
    INT height, pts;
    TCHAR buf[10];

    //
    //  Filter for a font type match (the font type of the selected face
    //  must be the same as that enumerated).
    //
    if (nFontType != (DWORD)(GDI_FONTTYPE_STUFF & lpData->nFontType))
    {
        return (TRUE);
    }

    if (!(nFontType & RASTER_FONTTYPE))
    {
        //
        //  Vector or TT font.
        //
        if (lpData->bFillSize &&
            (int)SendMessage(lpData->hwndSizes, CB_GETCOUNT, 0, 0L) == 0)
        {
            FillScalableSizes(lpData->hwndSizes, lpData->lpcf);
        }
    }
    else
    {
        height = lptm->tmHeight - lptm->tmInternalLeading;
        pts = GetPointString(buf, lpData->hDC, height);

        //
        //  Filter devices same size of multiple styles.
        //
        if (CBFindString(lpData->hwndSizes, buf) < 0)
        {
            CBAddSize(lpData->hwndSizes, pts, lpData->lpcf);
        }
    }

    //
    //  Keep the printer/screen bits from the family list here too.
    //
    nFontType |= (lpData->nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE));

    if (nFontType & TRUETYPE_FONTTYPE)
    {
        //
        //  If (lptm->ntmFlags & NTM_REGULAR)
        //
        if (!(lptm->ntmFlags & (NTM_BOLD | NTM_ITALIC)))
        {
            nFontType |= REGULAR_FONTTYPE;
        }

        if (lptm->ntmFlags & NTM_ITALIC)
        {
            nFontType |= ITALIC_FONTTYPE;
        }

        if (lptm->ntmFlags & NTM_BOLD)
        {
            nFontType |= BOLD_FONTTYPE;
        }

        //
        //  After the LOGFONT.lfFaceName there are 2 more names
        //     lfFullName[LF_FACESIZE * 2]
        //     lfStyle[LF_FACESIZE]
        //
        CBAddStyle( lpData->hwndStyle,
                    lplf->lfFaceName + 3 * LF_FACESIZE,
                    nFontType,
                    lplf );
    }
    else
    {
        if ((lplf->lfWeight >= FW_BOLD) && lplf->lfItalic)
        {
            CBAddStyle( lpData->hwndStyle,
                        szBoldItalic,
                        (nFontType | BOLD_FONTTYPE | ITALIC_FONTTYPE),
                        lplf );
        }
        else if (lplf->lfWeight >= FW_BOLD)
        {
            CBAddStyle( lpData->hwndStyle,
                        szBold,
                        (nFontType | BOLD_FONTTYPE),
                        lplf );
        }
        else if (lplf->lfItalic)
        {
            CBAddStyle( lpData->hwndStyle,
                        szItalic,
                        (nFontType | ITALIC_FONTTYPE),
                        lplf );
        }
        else
        {
            CBAddStyle( lpData->hwndStyle,
                        szRegular,
                        (nFontType | REGULAR_FONTTYPE),
                        lplf );
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeFonts
//
////////////////////////////////////////////////////////////////////////////

VOID FreeFonts(
    HWND hwnd)
{
    INT i, count;
    LPITEMDATA lpItemData;

    count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

    for (i = 0; i < count; i++)
    {
        if (lpItemData = (LPITEMDATA)SendMessage(hwnd, CB_GETITEMDATA, i, 0L))
        {
            LocalFree((HANDLE)lpItemData->pLogFont);
            LocalFree((HANDLE)lpItemData);
        }
    }

    SendMessage(hwnd, CB_RESETCONTENT, 0, 0L);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitLF
//
//  Initalize a LOGFONT structure to some base generic regular type font.
//
////////////////////////////////////////////////////////////////////////////

VOID InitLF(
    LPLOGFONT lplf)
{
    HDC hdc;

    lplf->lfEscapement = 0;
    lplf->lfOrientation = 0;
    lplf->lfCharSet = ANSI_CHARSET;
    lplf->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lplf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lplf->lfQuality = DEFAULT_QUALITY;
    lplf->lfPitchAndFamily = DEFAULT_PITCH;
    lplf->lfItalic = 0;
    lplf->lfWeight = FW_NORMAL;
    lplf->lfStrikeOut = 0;
    lplf->lfUnderline = 0;
    lplf->lfWidth = 0;            // otherwise we get independant x-y scaling
    lplf->lfFaceName[0] = 0;
    hdc = GetDC(NULL);
    lplf->lfHeight = -MulDiv( DEF_POINT_SIZE,
                              GetDeviceCaps(hdc, LOGPIXELSY),
                              POINTS_PER_INCH );
    ReleaseDC(NULL, hdc);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetFontStylesAndSizes
//
//  Fills the point sizes combo box with the point sizes for the current
//  selection in the facenames combobox.
//
//  cmb1 is the ID for the font facename combobox.
//
//  Returns:   TRUE    if successful
//             FALSE   otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFontStylesAndSizes(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    BOOL bForceSizeFill)
{
    ENUM_FONT_DATA data;
    TCHAR szFace[LF_FACESIZE];
    INT iSel;
    INT iMapMode;
    SIZE ViewportExt, WindowExt;
    LOGFONT lf;
    LPITEMDATA lpItemData;

    FreeFonts(GetDlgItem(hDlg, cmb2));

    data.hwndStyle = GetDlgItem(hDlg, cmb2);
    data.hwndSizes = GetDlgItem(hDlg, cmb3);
    data.dwFlags   = lpcf->Flags;
    data.lpcf      = lpcf;

    iSel = (int)SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);
    if (iSel < 0)
    {
        //
        //  If we don't have a face name selected we will synthisize
        //  the standard font styles...
        //
        InitLF(&lf);
        CBAddStyle(data.hwndStyle, szRegular, REGULAR_FONTTYPE, &lf);
        lf.lfWeight = FW_BOLD;
        CBAddStyle(data.hwndStyle, szBold, BOLD_FONTTYPE, &lf);
        lf.lfWeight = FW_NORMAL;
        lf.lfItalic = TRUE;
        CBAddStyle(data.hwndStyle, szItalic, ITALIC_FONTTYPE, &lf);
        lf.lfWeight = FW_BOLD;
        CBAddStyle(data.hwndStyle, szBoldItalic, BOLD_FONTTYPE | ITALIC_FONTTYPE, &lf);
        FillScalableSizes(data.hwndSizes, lpcf);

        return (TRUE);
    }

    if (lpItemData = ((LPITEMDATA)SendDlgItemMessage( hDlg,
                                                      cmb1,
                                                      CB_GETITEMDATA,
                                                      iSel,
                                                      0L )))
    {
        data.nFontType  = lpItemData->nFontType;
    }
    else
    {
        data.nFontType  = 0;
    }

    data.bFillSize = TRUE;

    if (data.bFillSize)
    {
        SendMessage(data.hwndSizes, CB_RESETCONTENT, 0, 0L);
        SendMessage(data.hwndSizes, WM_SETREDRAW, FALSE, 0L);
    }

    SendMessage(data.hwndStyle, WM_SETREDRAW, FALSE, 0L);

    GetDlgItemText(hDlg, cmb1, szFace, sizeof(szFace) / sizeof(TCHAR));

    if (lpcf->Flags & CF_SCREENFONTS)
    {
        data.hDC = GetDC(NULL);
        data.bPrinterFont = FALSE;
        EnumFontFamilies( data.hDC,
                          (LPCTSTR)szFace,
                          (FONTENUMPROC)FontStyleEnumProc,
                          (LPARAM)&data );
        ReleaseDC(NULL, data.hDC);
    }

    if (lpcf->Flags & CF_PRINTERFONTS)
    {
        //
        //  Save and restore the DC's mapping mode (and extents if needed)
        //  if it's been set by the app to something other than MM_TEXT.
        //
        if ((iMapMode = GetMapMode(lpcf->hDC)) != MM_TEXT)
        {
            if ((iMapMode == MM_ISOTROPIC) || (iMapMode == MM_ANISOTROPIC))
            {
                GetViewportExtEx(lpcf->hDC, &ViewportExt);
                GetWindowExtEx(lpcf->hDC, &WindowExt);
            }
            SetMapMode(lpcf->hDC, MM_TEXT);
        }

        data.hDC = lpcf->hDC;
        data.bPrinterFont = TRUE;
        EnumFontFamilies( lpcf->hDC,
                          (LPCTSTR)szFace,
                          (FONTENUMPROC)FontStyleEnumProc,
                          (LPARAM)&data );

        if (iMapMode != MM_TEXT)
        {
            SetMapMode(lpcf->hDC, iMapMode);
            if ((iMapMode == MM_ISOTROPIC) || (iMapMode == MM_ANISOTROPIC))
            {
                SetWindowExtEx( lpcf->hDC,
                                WindowExt.cx,
                                WindowExt.cy,
                                &WindowExt );
                SetViewportExtEx( lpcf->hDC,
                                  ViewportExt.cx,
                                  ViewportExt.cy,
                                  &ViewportExt );
            }
        }
    }

    if (!(lpcf->Flags & CF_NOSIMULATIONS))
    {
        FillInMissingStyles(data.hwndStyle);
    }

    SendMessage(data.hwndStyle, WM_SETREDRAW, TRUE, 0L);
    if (wWinVer < 0x030A)
    {
        InvalidateRect(data.hwndStyle, NULL, TRUE);
    }

    if (data.bFillSize)
    {
        SendMessage(data.hwndSizes, WM_SETREDRAW, TRUE, 0L);
        if (wWinVer < 0x030A)
        {
            InvalidateRect(data.hwndSizes, NULL, TRUE);
        }
    }

    bForceSizeFill;
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FillColorCombo
//
//  Adds the color name strings to the colors combobox.
//
//  cmb4 is the ID for the color combobox.
//
//  The color rectangles are drawn later in response to a WM_DRAWITEM msg.
//
////////////////////////////////////////////////////////////////////////////

VOID FillColorCombo(
    HWND hDlg)
{
    INT iT, item;
    TCHAR szT[CCHCOLORNAMEMAX];

    for (iT = 0; iT < CCHCOLORS; ++iT)
    {
        *szT = 0;
        LoadString(g_hinst, iszBlack + iT, szT, sizeof(szT) / sizeof(TCHAR));
        item = SendDlgItemMessage( hDlg,
                                   cmb4,
                                   CB_INSERTSTRING,
                                   iT,
                                   (LONG)(LPTSTR)szT );
        if (item >= 0)
        {
            SendDlgItemMessage(hDlg, cmb4, CB_SETITEMDATA, item, rgbColors[iT]);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ComputeSampleTextRectangle
//
//  Determines the bounding rectangle for the text preview area and fills
//  in rcText.
//
//  stc5 is the ID for the preview text rectangel.
//
//  Coordinates are calculated w.r.t the dialog.
//
////////////////////////////////////////////////////////////////////////////

VOID ComputeSampleTextRectangle(
    HWND hDlg)
{
    GetWindowRect(GetDlgItem (hDlg, stc5), &rcText);
    ScreenToClient(hDlg, (LPPOINT)&rcText.left);
    ScreenToClient(hDlg, (LPPOINT)&rcText.right);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawSizeComboItem
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawSizeComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC;
    DWORD rgbBack, rgbText;
    TCHAR szFace[10];
    HFONT hFont;

    hDC = lpdis->hDC;

    //
    //  We must first select the dialog control font.
    //
    if (hDlgFont)
    {
        hFont = SelectObject(hDC, hDlgFont);
    }

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LONG)(LPTSTR)szFace );

    ExtTextOut( hDC,
                lpdis->rcItem.left + GetSystemMetrics(SM_CXBORDER),
                lpdis->rcItem.top,
                ETO_OPAQUE,
                &lpdis->rcItem,
                szFace,
                lstrlen(szFace),
                NULL );
    //
    //  Reset font.
    //
    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawFamilyComboItem
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawFamilyComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC, hdcMem;
    DWORD rgbBack, rgbText;
    TCHAR szFace[LF_FACESIZE + 10];
    HBITMAP hOld;
    INT dy, x;
    HFONT hFont;

    hDC = lpdis->hDC;

    //
    //  We must first select the dialog control font.
    //
    if (hDlgFont)
    {
        hFont = SelectObject(hDC, hDlgFont);
    }

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    // wsprintf(szFace, "%4.4X", LOWORD(lpdis->itemData));

    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LONG)(LPTSTR)szFace );
    ExtTextOut( hDC,
                lpdis->rcItem.left + DX_BITMAP,
                lpdis->rcItem.top,
                ETO_OPAQUE,
                &lpdis->rcItem,
                szFace,
                lstrlen(szFace),
                NULL );
    //
    //  Reset font.
    //
    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem)
    {
        if (hbmFont)
        {
            LPITEMDATA lpItemData = (LPITEMDATA)lpdis->itemData;

            hOld = SelectObject(hdcMem, hbmFont);

            if (!lpItemData)
            {
                goto SkipBlt;
            }

            if (lpItemData->nFontType & TRUETYPE_FONTTYPE)
            {
                x = 0;
            }
            else
            {
                if ((lpItemData->nFontType & (PRINTER_FONTTYPE |
                                              DEVICE_FONTTYPE))
                  == (PRINTER_FONTTYPE | DEVICE_FONTTYPE))
                {
                    //
                    //  This may be a screen and printer font but
                    //  we will call it a printer font here.
                    //
                    x = DX_BITMAP;
                }
                else
                {
                    goto SkipBlt;
                }
            }

            dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - DY_BITMAP) / 2;

            BitBlt( hDC,
                    lpdis->rcItem.left,
                    lpdis->rcItem.top + dy,
                    DX_BITMAP,
                    DY_BITMAP,
                    hdcMem,
                    x,
                    lpdis->itemState & ODS_SELECTED ? DY_BITMAP : 0,
                    SRCCOPY );

SkipBlt:
            SelectObject(hdcMem, hOld);
        }
        DeleteDC(hdcMem);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawColorComboItem
//
//  Computes and draws the color combo items.
//  Called by main dialog function in response to a WM_DRAWITEM msg.
//
//  All color name strings have already been loaded and filled into
//  the combobox.
//
//  Returns:   TRUE    if succesful
//             FALSE   otherwise.
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawColorComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC;
    HBRUSH hbr;
    INT dx, dy;
    RECT rc;
    TCHAR szColor[CCHCOLORNAMEMAX];
    DWORD rgbBack, rgbText, dw;
    HFONT hFont;

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }
    ExtTextOut( hDC,
                lpdis->rcItem.left,
                lpdis->rcItem.top,
                ETO_OPAQUE,
                &lpdis->rcItem,
                NULL,
                0,
                NULL );

    //
    //  Compute coordinates of color rectangle and draw it.
    //
    dx = GetSystemMetrics(SM_CXBORDER);
    dy = GetSystemMetrics(SM_CYBORDER);
    rc.top    = lpdis->rcItem.top + dy;
    rc.bottom = lpdis->rcItem.bottom - dy;
    rc.left   = lpdis->rcItem.left + dx;
    rc.right  = rc.left + 2 * (rc.bottom - rc.top);

//  if (wWinVer < 0x030A)
    {
        dw = SendMessage(lpdis->hwndItem, CB_GETITEMDATA, lpdis->itemID, 0L);
    }
//  else
//      dw = lpdis->itemData;

    hbr = CreateSolidBrush(dw);
    if (!hbr)
    {
        return (FALSE);
    }

    hbr = SelectObject(hDC, hbr);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    DeleteObject(SelectObject(hDC, hbr));

    //
    //  Shift the color text right by the width of the color rectangle.
    //
    *szColor = 0;
    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LONG)(LPTSTR)szColor );

    //
    //  We must first select the dialog control font.
    //
    if (hDlgFont)
    {
        hFont = SelectObject(hDC, hDlgFont);
    }

    TextOut( hDC,
             2 * dx + rc.right,
             lpdis->rcItem.top,
             szColor,
             lstrlen(szColor) );

    //
    //  Reset font.
    //
    if (hFont)
    {
        SelectObject(hDC, hFont);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawSampleText
//
//  Displays sample text with given attributes.  Assumes rcText holds the
//  coordinates of the area within the frame (relative to dialog client)
//  which text should be drawn in.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSampleText(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    HDC hDC)
{
    DWORD rgbText;
    DWORD rgbBack;
    INT iItem;
    HFONT hFont, hTemp;
    TCHAR szSample[50];
    LOGFONT lf;
    SIZE TextExtent;
    INT len, x, y;
    TEXTMETRIC tm;
    BOOL bCompleteFont;

    bCompleteFont = FillInFont(hDlg, lpcf, &lf, FALSE);

    hFont = CreateFontIndirect(&lf);
    if (!hFont)
    {
        return;
    }

    hTemp = SelectObject(hDC, hFont);

    rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));

    if (lpcf->Flags & CF_EFFECTS)
    {
        iItem = (int)SendDlgItemMessage(hDlg, cmb4, CB_GETCURSEL, 0, 0L);
        if (iItem != CB_ERR)
        {
            rgbText = SendDlgItemMessage(hDlg, cmb4, CB_GETITEMDATA, iItem, 0L);
        }
        else
        {
            goto GetWindowTextColor;
        }
    }
    else
    {
GetWindowTextColor:
        rgbText = GetSysColor(COLOR_WINDOWTEXT);
    }

    rgbText = SetTextColor(hDC, rgbText);

    if (bCompleteFont)
    {
        GetDlgItemText(hDlg, stc5, szSample, sizeof(szSample) / sizeof(TCHAR));
    }
    else
    {
        szSample[0] = 0;
    }

    GetTextMetrics(hDC, &tm);

    len = lstrlen(szSample);
    GetTextExtentPoint(hDC, szSample, len, &TextExtent);
    TextExtent.cy = tm.tmAscent - tm.tmInternalLeading;

    if ((TextExtent.cx >= (rcText.right - rcText.left)) ||
        (TextExtent.cx <= 0))
    {
        x = rcText.left;
    }
    else
    {
        x = rcText.left + ((rcText.right - rcText.left) - TextExtent.cx) / 2;
    }

    y = min( rcText.bottom,
             rcText.bottom - ((rcText.bottom - rcText.top) - TextExtent.cy) / 2);

    ExtTextOut( hDC,
                x,
                y - (tm.tmAscent),
                ETO_OPAQUE | ETO_CLIPPED,
                &rcText,
                szSample,
                len,
                NULL );

    SetBkColor(hDC, rgbBack);
    SetTextColor(hDC, rgbText);

    if (hTemp)
    {
        DeleteObject(SelectObject(hDC, hTemp));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  FillInFont
//
//  Fills in the LOGFONT strucuture based on the current selection.
//
//  bSetBits - if TRUE the Flags fields in the lpcf are set to indicate
//             what parts (face, style, size) are not selected
//
//  lplf     - LOGFONT filled in upon return
//
//  Returns:   TRUE    if there was an unambiguous selection
//                     (the LOGFONT is filled in as per the enumeration)
//             FALSE   there was not a complete selection
//                     (fields set in the LOGFONT with default values)
//
////////////////////////////////////////////////////////////////////////////

BOOL FillInFont(
    HWND hDlg,
    LPCHOOSEFONT lpcf,
    LPLOGFONT lplf,
    BOOL bSetBits)
{
    HDC hdc;
    INT iSel, id, pts;
    LPITEMDATA lpItemData;
    DWORD nFontType;
    PLOGFONT plf;
    TCHAR szStyle[LF_FACESIZE];
    TCHAR szMessage[128];
    BOOL bFontComplete = TRUE;

    InitLF(lplf);

    GetDlgItemText( hDlg,
                    cmb1,
                    lplf->lfFaceName,
                    sizeof(lplf->lfFaceName) / sizeof(TCHAR) );
    if (CBFindString(GetDlgItem(hDlg, cmb1), lplf->lfFaceName) >= 0)
    {
        if (bSetBits)
        {
            lpcf->Flags &= ~CF_NOFACESEL;
        }
    }
    else
    {
        bFontComplete = FALSE;
        if (bSetBits)
        {
            lpcf->Flags |= CF_NOFACESEL;
        }
    }

    iSel = CBGetTextAndData( GetDlgItem(hDlg, cmb2),
                             szStyle,
                             sizeof(szStyle) / sizeof(TCHAR),
                             (LPDWORD)&lpItemData );
    if (iSel >= 0 && lpItemData)
    {
        nFontType = lpItemData->nFontType;
        plf = lpItemData->pLogFont;
        *lplf = *plf;                       // copy the LOGFONT
        lplf->lfWidth = 0;                  // 1:1 x-y scaling
        if (bSetBits)
        {
            lpcf->Flags &= ~CF_NOSTYLESEL;
        }
    }
    else
    {
        bFontComplete = FALSE;
        if (bSetBits)
        {
            lpcf->Flags |= CF_NOSTYLESEL;
        }
        nFontType = 0;
    }

    //
    //  Now make sure the size is in range; pts will be 0 if not.
    //
    GetPointSizeInRange(hDlg, lpcf, &pts, 0);

    hdc = GetDC(NULL);
    if (pts)
    {
        lplf->lfHeight = -MulDiv( pts,
                                  GetDeviceCaps(hdc, LOGPIXELSY),
                                  POINTS_PER_INCH );
        if (bSetBits)
        {
            lpcf->Flags &= ~CF_NOSIZESEL;
        }
    }
    else
    {
        lplf->lfHeight = -MulDiv( DEF_POINT_SIZE,
                                  GetDeviceCaps(hdc, LOGPIXELSY),
                                  POINTS_PER_INCH );
        bFontComplete = FALSE;
        if (bSetBits)
        {
            lpcf->Flags |= CF_NOSIZESEL;
        }
    }
    ReleaseDC(NULL, hdc);

    //
    //  And the attributes we control.
    //
    lplf->lfStrikeOut = (BYTE)IsDlgButtonChecked(hDlg, chx1);
    lplf->lfUnderline = (BYTE)IsDlgButtonChecked(hDlg, chx2);

    if (nFontType != nLastFontType)
    {
        if (lpcf->Flags & CF_PRINTERFONTS)
        {
            if (nFontType & SIMULATED_FONTTYPE)
            {
                id = iszSynth;
            }
            else if (nFontType & TRUETYPE_FONTTYPE)
            {
                id = iszTrueType;
            }
            else if ((nFontType & (PRINTER_FONTTYPE | DEVICE_FONTTYPE)) ==
                     (PRINTER_FONTTYPE | DEVICE_FONTTYPE))
            {
                //
                //  May be both screen and printer (ATM) but we'll just
                //  call this a printer font.
                //
                id = iszPrinterFont;
            }
            else if ((nFontType & (PRINTER_FONTTYPE | SCREEN_FONTTYPE)) ==
                     SCREEN_FONTTYPE)
            {
                id = iszGDIFont;
            }
            else
            {
                szMessage[0] = 0;
                goto SetText;
            }
            LoadString( g_hinst,
                        id,
                        szMessage,
                        sizeof(szMessage) / sizeof(TCHAR) );
SetText:
            SetDlgItemText(hDlg, stc6, szMessage);
        }
    }
    nLastFontType = nFontType;

    return (bFontComplete);
}


////////////////////////////////////////////////////////////////////////////
//
//  TermFont
//
//  Release any data required by functions in this module.
//
////////////////////////////////////////////////////////////////////////////

VOID TermFont()
{
    if (hbmFont)
    {
        DeleteObject(hbmFont);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPointString
//
//  Converts font height into a string of digits representing point size.
//
//  Returns:   Size in points and fills in buffer with string
//
////////////////////////////////////////////////////////////////////////////

INT GetPointString(
    LPTSTR buf,
    HDC hDC,
    INT height)
{
    INT pts;

    pts = MulDiv((height < 0) ? -height : height, 72, GetDeviceCaps(hDC, LOGPIXELSY));
    wsprintf(buf, szPtFormat, pts);
    return (pts);
}


////////////////////////////////////////////////////////////////////////////
//
//  FlipColor
//
////////////////////////////////////////////////////////////////////////////

DWORD FlipColor(
    DWORD rgb)
{
    return ( RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)) );
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadBitmaps
//
//  This routine loads DIB bitmaps, and "fixes up" their color tables
//  so that we get the desired result for the device we are on.
//
//  This routine requires:
//      the DIB is a 16 color DIB authored with the standard windows colors
//      bright blue (00 00 FF) is converted to the background color
//      light grey  (C0 C0 C0) is replaced with the button face color
//      dark grey   (80 80 80) is replaced with the button shadow color
//
//  This means you can't have any of these colors in your bitmap.
//
////////////////////////////////////////////////////////////////////////////

#define BACKGROUND      0x000000FF          // bright blue
#define BACKGROUNDSEL   0x00FF00FF          // bright blue
#define BUTTONFACE      0x00C0C0C0          // bright grey
#define BUTTONSHADOW    0x00808080          // dark grey

HBITMAP LoadBitmaps(
    INT id)
{
    HDC hdc;
    HANDLE h;
    DWORD *p;
    BYTE *lpBits;
    HANDLE hRes;
    LPBITMAPINFOHEADER lpBitmapInfo;
    int numcolors;
    DWORD rgbSelected;
    DWORD rgbUnselected;
    HBITMAP hbm;
    UINT cbBitmapSize;
    LPBITMAPINFOHEADER lpBitmapData;

    rgbSelected = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
    rgbUnselected = FlipColor(GetSysColor(COLOR_WINDOW));

    h = FindResource(g_hinst, MAKEINTRESOURCE(id), RT_BITMAP);
    hRes = LoadResource(g_hinst, h);

    //
    //  Lock the bitmap and get a pointer to the color table.
    //
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
    {
        return FALSE;
    }

    //
    //  Lock the bitmap data and make a copy of it for the mask and the
    //  bitmap.
    //
    cbBitmapSize = SizeofResource(g_hinst, h);

    lpBitmapData = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, cbBitmapSize);

    if (!lpBitmapData)
    {
        return (NULL);
    }

    memcpy((TCHAR *)lpBitmapData, (TCHAR *)lpBitmapInfo, cbBitmapSize);

    p = (DWORD *)((LPTSTR)(lpBitmapData) + lpBitmapData->biSize);

    //
    //  Search for the Solid Blue entry and replace it with the current
    //  background RGB.
    //
    numcolors = 16;

    while (numcolors-- > 0)
    {
        if (*p == BACKGROUND)
        {
            *p = rgbUnselected;
        }
        else if (*p == BACKGROUNDSEL)
        {
            *p = rgbSelected;
        }
#if 0
        else if (*p == BUTTONFACE)
        {
            *p = FlipColor(GetSysColor(COLOR_BTNFACE));
        }
        else if (*p == BUTTONSHADOW)
        {
            *p = FlipColor(GetSysColor(COLOR_BTNSHADOW));
        }
#endif
        p++;
    }

    //
    //  First skip over the header structure.
    //
    lpBits = (BYTE *)(lpBitmapData + 1);

    //
    //  Skip the color table entries, if any.
    //
    lpBits += (1 << (lpBitmapData->biBitCount)) * sizeof(RGBQUAD);

    //
    //  Create a color bitmap compatible with the display device.
    //
    hdc = GetDC(NULL);
    hbm = CreateDIBitmap( hdc,
                          lpBitmapData,
                          (DWORD)CBM_INIT,
                          lpBits,
                          (LPBITMAPINFO)lpBitmapData,
                          DIB_RGB_COLORS );
    ReleaseDC(NULL, hdc);

    LocalFree(lpBitmapData);

    return (hbm);
}


////////////////////////////////////////////////////////////////////////////
//
//  LookUpFontSubs
//
//  Looks in the font substitute list for a real font name.
//
//  lpSubFontName  - substitute font name
//  lpRealFontName - real font name buffer
//
//  Returns:   TRUE    if lpRealFontName is filled in
//             FALSE   if not
//
////////////////////////////////////////////////////////////////////////////

BOOL LookUpFontSubs(
    LPTSTR lpSubFontName,
    LPTSTR lpRealFontName)
{
    LONG lResult;
    HKEY hKey;
    TCHAR szValueName[MAX_PATH];
    TCHAR szValueData[MAX_PATH];
    DWORD dwValueSize;
    DWORD dwIndex = 0;
    DWORD dwType, dwSize;


    //
    //  Open the font substitution's key.
    //
    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            KEY_FONT_SUBS,
                            0,
                            KEY_READ,
                            &hKey );

    if (lResult != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Loop through the values in the key
    //
    dwValueSize = MAX_PATH;
    dwSize = MAX_PATH;
    while (RegEnumValue( hKey,
                         dwIndex,
                         szValueName,
                         &dwValueSize,
                         NULL,
                         &dwType,
                         (LPBYTE)szValueData,
                         &dwSize ) == ERROR_SUCCESS)
    {
        //
        //  If the value name matches the requested font name, then
        //  copy the real font name to the output buffer.
        //
        if (!lstrcmpi(szValueName, lpSubFontName))
        {
            lstrcpy(lpRealFontName, szValueData);
            RegCloseKey(hKey);
            return (TRUE);
        }

        //
        //  Re-initialize for the next time through the loop.
        //
        dwValueSize = MAX_PATH;
        dwSize = MAX_PATH;
        dwIndex++;
    }

    //
    //  Clean up.
    //
    *lpRealFontName = CHAR_NULL;
    RegCloseKey(hKey);
    return (FALSE);
}






/*========================================================================*/
/*                 Ansi->Unicode Thunk routines                           */
/*========================================================================*/

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ThunkChooseFontA2W
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkChooseFontA2W(
    PFONTINFO pFI)
{
    BOOL bRet;
    LPCHOOSEFONTW pCFW = pFI->pCF;
    LPCHOOSEFONTA pCFA = pFI->pCFA;

    pCFW->hwndOwner = pCFA->hwndOwner;
    pCFW->lCustData = pCFA->lCustData;

    pCFW->Flags = pCFA->Flags;

    //
    //  !!! hack, should not be based on flag value, since this could happen
    //  at any time.
    //
    if (pCFA->Flags & CF_INITTOLOGFONTSTRUCT)
    {
        ThunkLogFontA2W(pCFA->lpLogFont, pCFW->lpLogFont);
    }

    pCFW->hInstance = pCFA->hInstance;
    pCFW->lpfnHook = pCFA->lpfnHook;

    if (pCFW->Flags & CF_PRINTERFONTS)
    {
        pCFW->hDC = pCFA->hDC;
    }

    if (pCFW->Flags & CF_USESTYLE)
    {
        bRet = RtlAnsiStringToUnicodeString(pFI->pusStyle, pFI->pasStyle, FALSE);
    }

    pCFW->nSizeMin = pCFA->nSizeMin;
    pCFW->nSizeMax = pCFA->nSizeMax;
    pCFW->rgbColors = pCFA->rgbColors;

    pCFW->iPointSize = pCFA->iPointSize;
    pCFW->nFontType = pCFA->nFontType;

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkChooseFontW2A
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkChooseFontW2A(
    PFONTINFO pFI)
{
    BOOL bRet;
    LPCHOOSEFONTA pCFA = pFI->pCFA;
    LPCHOOSEFONTW pCFW = pFI->pCF;

    ThunkLogFontW2A(pCFW->lpLogFont, pCFA->lpLogFont);

    pCFA->hInstance = pCFW->hInstance;
    pCFA->lpfnHook = pCFW->lpfnHook;

    if (pCFA->Flags & CF_USESTYLE)
    {
        pFI->pusStyle->Length = (lstrlen(pFI->pusStyle->Buffer) + 1) * sizeof(WCHAR);
        bRet = RtlUnicodeStringToAnsiString(pFI->pasStyle, pFI->pusStyle, FALSE);
    }

    pCFA->Flags = pCFW->Flags;
    pCFA->nSizeMin = pCFW->nSizeMin;
    pCFA->nSizeMax = pCFW->nSizeMax;
    pCFA->rgbColors = pCFW->rgbColors;

    pCFA->iPointSize = pCFW->iPointSize;
    pCFA->nFontType = pCFW->nFontType;

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkLogFontA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkLogFontA2W(
    LPLOGFONTA lpLFA,
    LPLOGFONTW lpLFW)
{
    lpLFW->lfHeight = lpLFA->lfHeight;
    lpLFW->lfWidth = lpLFA->lfWidth;
    lpLFW->lfEscapement = lpLFA->lfEscapement;
    lpLFW->lfOrientation = lpLFA->lfOrientation;
    lpLFW->lfWeight = lpLFA->lfWeight;
    lpLFW->lfItalic = lpLFA->lfItalic;
    lpLFW->lfUnderline = lpLFA->lfUnderline;
    lpLFW->lfStrikeOut = lpLFA->lfStrikeOut;
    lpLFW->lfCharSet = lpLFA->lfCharSet;
    lpLFW->lfOutPrecision = lpLFA->lfOutPrecision;
    lpLFW->lfClipPrecision = lpLFA->lfClipPrecision;
    lpLFW->lfQuality = lpLFA->lfQuality;
    lpLFW->lfPitchAndFamily = lpLFA->lfPitchAndFamily;

    MultiByteToWideChar( CP_ACP,
                         0,
                         lpLFA->lfFaceName,
                         -1,
                         lpLFW->lfFaceName,
                         LF_FACESIZE );
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkLogFontW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkLogFontW2A(
    LPLOGFONTW lpLFW,
    LPLOGFONTA lpLFA)
{
    BOOL fDefCharUsed;

    // sanity check
    if (lpLFW && lpLFA)
    {
        lpLFA->lfHeight = lpLFW->lfHeight;
        lpLFA->lfWidth = lpLFW->lfWidth;
        lpLFA->lfEscapement = lpLFW->lfEscapement;
        lpLFA->lfOrientation = lpLFW->lfOrientation;
        lpLFA->lfWeight = lpLFW->lfWeight;
        lpLFA->lfItalic = lpLFW->lfItalic;
        lpLFA->lfUnderline = lpLFW->lfUnderline;
        lpLFA->lfStrikeOut = lpLFW->lfStrikeOut;
        lpLFA->lfCharSet = lpLFW->lfCharSet;
        lpLFA->lfOutPrecision = lpLFW->lfOutPrecision;
        lpLFA->lfClipPrecision = lpLFW->lfClipPrecision;
        lpLFA->lfQuality = lpLFW->lfQuality;
        lpLFA->lfPitchAndFamily = lpLFW->lfPitchAndFamily;

        WideCharToMultiByte( CP_ACP,
                             0,
                             lpLFW->lfFaceName,
                             -1,
                             lpLFA->lfFaceName,
                             LF_FACESIZE,
                             NULL,
                             &fDefCharUsed );
    }
}

#endif
