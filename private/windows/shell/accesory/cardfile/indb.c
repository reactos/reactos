/* Dialog Procedures */

#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

int DBcmd;
BOOL fRecursive;        /* Avoid recursive IDD_BUTTON calls */
BOOL fValidDB;          /* Are we validating on save, or not?  local to DB */
HWND hwndLinkWait;      /* Waits for "Set Link" callback */
BOOL fAutomatic;        /* Did the link get changed to Automatic? */

BOOL DlgProc(
    HWND hDB,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR *pResultBuf;
    TCHAR *pInit;
    int    len;

    switch (message)
    {
        case WM_INITDIALOG:
            switch(DBcmd)
            {
                case DTHEADER:
                    pInit = CurCardHead.line;
                    break;

                default:
                    pInit = TEXT("");
            }
            SendDlgItemMessage(hDB, ID_EDIT, EM_LIMITTEXT, LINELENGTH, 0L);
            SetDlgItemText(hDB, ID_EDIT, pInit);

            SetFocus(GetDlgItem(hDB, ID_EDIT));
            return(TRUE);

        case WM_COMMAND:
            pResultBuf = NULL;
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if ((len = GetWindowTextLength(GetDlgItem(hDB, ID_EDIT))) || DBcmd == DTHEADER || DBcmd == DTADD)
                        if (pResultBuf = (TCHAR *) LocalAlloc (LPTR, ByteCountOf(++len)))
                            GetDlgItemText(hDB, ID_EDIT, pResultBuf, len);
                    break;
                case IDCANCEL:
                    break;
                default:
                    return(FALSE);
            }
            EndDialog(hDB, (int)pResultBuf);     /* return pointer to buffer */
            return(TRUE);
            break;
        default:
            return(FALSE);
    }
}

/* Ole2Native - converts OLECHAR data to native character code.
                does it in-place, so you better have enough room.
 *
 * called with: Ole2Native( szBuf, num )
 *
 * szBuf - input string 
 * num   - number of zero terminators in string 
 *
 * returns: ptr to TCHAR string allocated from heap.
            caller must free.
*           on error, returns NULL
 *
 */

TCHAR* Ole2Native( OLECHAR* szBuf, INT num )
{
#ifdef OLE_20
    return szBuf;
#else
    INT      iWSize;                            // characters output
    INT      iOleSize;                          // input size
    PTCHAR   pszResult;                         // returned result
    OLECHAR* pszTemp;                           // for scanning for nulls
    INT      i;                                 // ditto

    pszTemp= szBuf;
    for( i=0; i<num; i++ )
    {
        while( *pszTemp )
        {
            pszTemp++;
        }
        pszTemp++;
    }
    iOleSize= (pszTemp-szBuf);                  // chars to convert including nulls

    iWSize= MultiByteToWideChar(
                         CP_ACP,                // codepage
                         MB_PRECOMPOSED,        // character type options
                         szBuf,                 // string to convert
                         iOleSize,              // length to convert
                         NULL,                  // don't actually convert
                         0);                    // length of output buffer

    pszResult= LocalAlloc( LPTR, iWSize*sizeof(TCHAR) );

    if( pszResult )
    {
        INT iResSize;
        iResSize= MultiByteToWideChar(
                         CP_ACP,                // code page
                         MB_PRECOMPOSED,        // character type options
                         szBuf,                 // string to map
                         iOleSize,              // length in chars to convert
                         pszResult,             // resultant unicode characters
                         iWSize);
        if( iResSize != iWSize )
        {
             pszResult= NULL;
        }
    }

    return( pszResult );

#endif
}

/* LinksDlg... dialog */
BOOL fnLinksDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    OLEOPT_UPDATE fUpdate;
    HANDLE hData = NULL;
    LPTSTR lpstrData = NULL;
    OLECHAR* pszOleData;            // pointer to links format string

    switch (message)
    {
        case WM_INITDIALOG:         /* Clone the object */
            /*
             * Two ways to implement undo:  Either save when the dialog pops up,
             * or save when an action is taken.  I choose to save when the
             * dialog pops up.
             */
            hwndError = hDlg;
            fAutomatic = FALSE;
            PicSaveUndo(&CurCard);

        case WM_REDRAWDIALOG:
            if (OLE_OK != OleGetLinkUpdateOptions(CurCard.lpObject, &fUpdate))
                ErrorMessage(W_IMPROPER_LINK_OPTIONS);
            else
            {
                if (fUpdate == oleupdate_always)
                    CheckRadioButton(hDlg, IDD_AUTO, IDD_MANUAL, IDD_AUTO);
                else if (fUpdate == oleupdate_oncall)
                    CheckRadioButton(hDlg, IDD_AUTO, IDD_MANUAL, IDD_MANUAL);
                else
                    ErrorMessage(W_IMPROPER_LINK_OPTIONS);
            }

            if (OLE_OK != OleGetData(CurCard.lpObject, vcfLink, &hData)) {
                ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                return TRUE;
            }

            /* link formats :  "szClass0szDocument0szItem00" */
            if (pszOleData = GlobalLock(hData))
            {
                LPTSTR lpstrTemp;
                LPTSTR pszOrigData;
                pszOrigData= lpstrData= Ole2Native( pszOleData,3 );    // convert to native code

                if( !lpstrData )
                    break;

                GetClassId(GetDlgItem(hDlg, IDD_CLASSID), lpstrData);
                /* display the Document and Item names */

                // scan off class
                while (*lpstrData++)
                    continue;
                /* Strip off the path name and drive letter */
                lpstrTemp = lpstrData;
                while (*lpstrTemp) {
                    if (*lpstrTemp == TEXT('\\') || *lpstrTemp == TEXT(':'))
                        lpstrData = lpstrTemp + 1;
                    lpstrTemp++;
                }

                // display the filename sans path information
                SetWindowText(GetDlgItem(hDlg, IDD_DOCINFO), lpstrData);

                // scan off filename and point to item information
                while (*lpstrData++)
                    continue;

                SetWindowText(GetDlgItem(hDlg, IDD_ITEMINFO), lpstrData);
                GlobalUnlock(hData);
                LocalFree( pszOrigData );
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_FREEZE:        /* Cancel Link */
                case IDD_UPDATE:        /* Update the Link */

                    /* Only update menus if successful */
                    if (SendMessage (hIndexWnd, WM_COMMAND, wParam, 0L)
                         && LOWORD(wParam) == IDD_FREEZE)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDD_UPDATE), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDD_CHANGE), FALSE);
                        CheckDlgButton(hDlg, IDD_AUTO, FALSE);
                        CheckDlgButton(hDlg, IDD_MANUAL, FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDD_AUTO), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDD_MANUAL), FALSE);
                    }

                    EnableWindow(GetDlgItem(hDlg, IDCANCEL), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDD_EDIT), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDD_PLAY), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDD_FREEZE), FALSE);
                    break;

                case IDD_CHANGE:
                    if (GetNewLinkName(hDlg, &CurCard))
                        PostMessage(hDlg, WM_REDRAWDIALOG, wParam, 0L);
                    else
                    {
                        SendMessage (hIndexWnd, WM_COMMAND, UNDO, 0L);
                        PicSaveUndo (&CurCard);
                    }
                    break;

                case IDD_PLAY:
                case IDD_EDIT:
                    PostMessage(hIndexWnd, WM_COMMAND, wParam, 0L);
                    goto Dismiss;

                case IDD_AUTO:
                case IDD_MANUAL:
                    if ((IsDlgButtonChecked(hDlg, IDD_AUTO) && LOWORD(wParam) == IDD_AUTO)
                         || (IsDlgButtonChecked(hDlg, IDD_MANUAL) && LOWORD(wParam) == IDD_MANUAL)
                         || fRecursive)
                        break;

                    fRecursive = TRUE;

                    switch (OleSetLinkUpdateOptions(CurCard.lpObject,
                        wParam == IDD_AUTO ? oleupdate_always : oleupdate_oncall))
                    {
                        case OLE_WAIT_FOR_RELEASE:
                            hwndLinkWait = hDlg;
                            break;

                        case OLE_OK:
                            fRecursive = FALSE;
                            goto LinkDone;
                            break;

                        case OLE_BUSY:
                            ErrorMessage(E_SERVER_BUSY);
                            /* Fall through */

                        default:
                            ErrorMessage(E_IMPROPER_LINK_OPTIONS);
                            break;
                    }
                    fRecursive = FALSE;
                    break;

                case IDD_LINKDONE:
LinkDone:
                    if (IsDlgButtonChecked(hDlg, IDD_MANUAL))
                    {
                        fAutomatic = TRUE;
                        CheckRadioButton(hDlg, IDD_AUTO, IDD_MANUAL, IDD_AUTO);
                    }
                    else
                    {
                        CheckRadioButton(hDlg, IDD_AUTO, IDD_MANUAL, IDD_MANUAL);
                    }
                    hwndLinkWait = NULL;
                    return FALSE;

                case IDCANCEL:
                    PostMessage(hIndexWnd, WM_COMMAND, UNDO, 0L);
                    goto Dismiss;

                case IDOK:
                    /* fAutomatic is set only if the button changed to Automatic */
                    if (fAutomatic && IsDlgButtonChecked(hDlg, IDD_AUTO))
                        PostMessage(hIndexWnd, WM_COMMAND, IDD_UPDATE, 0L);
                    OleError(OleClose(lpObjectUndo));

Dismiss:
                    hwndError = hIndexWnd;
                    EndDialog(hDlg, TRUE);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/* Invalid Link dialog */
int fnInvalidLink(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowText(hDlg, szWarning);
            return TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDD_LINK:
                    EndDialog(hDlg, wParam);
            }
    }
    return FALSE;
}

BOOL HookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG: {
            TCHAR    szValidate[160];

            LoadString(hIndexInstance, IDS_VALIDATE, szValidate, CharSizeOf(szValidate));
            SetDlgItemText(hDlg, chx1, szValidate);
            CheckDlgButton(hDlg, chx1, fValidDB);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == chx1)
            {
                fValidDB = !fValidDB;
                return TRUE;
            }
            break;

        default:
            break;
    }
    return FALSE;
}
