/* dlgprocs.c - Packager-specific dialog routines.
 */

#include "packager.h"
#include <shellapi.h>
#include <commdlg.h>
#include "dialogs.h"
// #include "..\..\library\shell.h"

// HACK: Copied from shsemip.h
#ifdef WINNT
    WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPWSTR pwszIconPath, UINT cchIconPath, int *piIconIndex);
    int  PkgPickIconDlg(HWND hwnd, LPSTR pszIconPath, UINT cbIconPath, int *piIconIndex);
#else
    WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPSTR pwszIconPath, UINT cchIconPath, int *piIconIndex);
#   define PkgPickIconDlg(h, s, c, p)  PickIconDlg(h, s, c, p)
#endif

#define CBCUSTFILTER 40

static CHAR szPathField[CBPATHMAX];
static CHAR szDirField[CBPATHMAX];
static CHAR szStarDotEXE[] = "*.EXE";
static CHAR szShellDll[] = "shell32.dll";
static CHAR szCommand[CBCMDLINKMAX];
static CHAR szIconText[CBPATHMAX];



/*--------------------------------------------------------------------------*/
/*                                      */
/*  MyDialogBox() -                             */
/*                                      */
/*--------------------------------------------------------------------------*/

INT_PTR MyDialogBox(
    UINT idd,
    HWND hwndParent,
    DLGPROC lpfnDlgProc
    )
{
    return DialogBoxAfterBlock(MAKEINTRESOURCE(idd), hwndParent, lpfnDlgProc);
}



#ifdef WINNT
/*
 * NT's PickIconDlg is UNICODE only, so thunk it here
 */

/* PkgPickIconDlg() -
 *
 *  hwnd        - window
 *  pszIconPath - ANSI path for icon suggested icon file (also output buffer that holds real icon file)
 *  cchIconPath - size of the buffer in chars pointed to pszIconPath. NOT the string length!
 *  piIconIndex - receives the index of the icon
 *
 */
int  PkgPickIconDlg(HWND hwnd, LPSTR pszIconPath, UINT cchIconPath, int *piIconIndex) {
    WCHAR wszPath[MAX_PATH+1];
    int iRet;

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszIconPath, -1, wszPath, sizeof(wszPath)/sizeof(TCHAR) );

    iRet = PickIconDlg(hwnd, wszPath, cchIconPath, piIconIndex);
    wszPath[MAX_PATH] = L'\0'; // Make sure text is zero terminated, even if it is garbage

    WideCharToMultiByte( CP_ACP, 0, wszPath, -1, pszIconPath, cchIconPath, NULL, NULL );

    return iRet;
}
#endif

/* IconDialog() -
 *
 */
BOOL
IconDialog(
    LPIC lpic
    )
{
    char szIconPath[MAX_PATH];
    int iDlgIcon = lpic->iDlgIcon;
    lstrcpy(szIconPath, (*lpic->szIconPath) ? lpic->szIconPath : szShellDll);

    if (PkgPickIconDlg(ghwndPane[APPEARANCE],
                    szIconPath, sizeof(szIconPath)/sizeof(char), &iDlgIcon))
    {
        lstrcpy(lpic->szIconPath, szIconPath);
        lpic->iDlgIcon = iDlgIcon;
        GetCurrentIcon(lpic);
        return TRUE;
    }

    return FALSE;
}



/* ChangeCmdLine() - Summons the Command Line... dialog.
 *
 */
BOOL
ChangeCmdLine(
    LPCML lpcml
    )
{
    lstrcpy(szCommand, lpcml->szCommand);

    if (DialogBoxAfterBlock(MAKEINTRESOURCE(DTCHANGECMDTEXT),
        ghwndPane[CONTENT], fnChangeCmdText) != IDOK)
        return FALSE;

    lstrcpy(lpcml->szCommand, szCommand);
    CmlFixBounds(lpcml);

    return TRUE;
}



/* ChangeLabel() - Summons the Label... dialog.
 *
 */
VOID
ChangeLabel(
    LPIC lpic
    )
{
    INT iPane = APPEARANCE;

    lstrcpy(szIconText, lpic->szIconText);

    if (DialogBoxAfterBlock(MAKEINTRESOURCE(DTCHANGETEXT),
        ghwndPane[iPane], fnChangeText)
        && lstrcmp(lpic->szIconText, szIconText))
    {
        // label has changed, set the undo object.
        if (glpobjUndo[iPane])
            DeletePaneObject(glpobjUndo[iPane], gptyUndo[iPane]);

        gptyUndo[iPane]  = ICON;
        glpobjUndo[iPane] = IconClone (lpic);
        lstrcpy(lpic->szIconText, szIconText);
    }
}



/**************************** Dialog Functions ****************************/
/* fnChangeCmdText() - Command Line... dialog procedure.
 */
INT_PTR CALLBACK
fnChangeCmdText(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPSTR psz;

    switch (msg)
    {
        case WM_INITDIALOG:
            SetDlgItemText(hDlg, IDD_COMMAND, szCommand);
            SendDlgItemMessage(hDlg, IDD_COMMAND, EM_LIMITTEXT, CBCMDLINKMAX - 1, 0L);
            PostMessage(hDlg, WM_NEXTDLGCTL,
                (WPARAM)GetDlgItem(hDlg, IDD_COMMAND), 1L);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_LABEL:
                    PostMessage(hDlg, WM_NEXTDLGCTL,
                        (WPARAM)GetDlgItem(hDlg, IDD_COMMAND), 1L);
                    break;

                case IDOK:
                    GetDlgItemText(hDlg, IDD_COMMAND, szCommand, CBCMDLINKMAX);
                    /*
                     * Eat leading spaces to make Afrikaners in high places
                     * happy.
                     */
                    psz = szCommand;
                    while(*psz == CHAR_SPACE)
                        psz++;

                    if( psz != szCommand ) {
                        LPSTR pszDst = szCommand;

                        while(*psz) {
                            *pszDst++ = *psz++;
                        }

                        /* copy null across */
                        *pszDst = *psz;
                    }

                // FALL THROUGH TO IDCANCEL

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
            }
    }

    return FALSE;
}



/* fnProperties() - Link Properties... dialog
 */
INT_PTR CALLBACK
fnProperties(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndLB = GetDlgItem(hDlg, IDD_LISTBOX);

    switch (msg)
    {
    case WM_REDRAW:
        SendMessage(hwndLB, WM_SETREDRAW, 0, 0L);

    case WM_INITDIALOG:
        {
            BOOL fChangeLink = TRUE;
            HANDLE hData = NULL;
            LONG otFocus;
            LPSTR lpstrData = NULL;
            LPSTR lpstrTemp;
            LPOLEOBJECT lpObject;
            LPVOID lpobjFocus;
            LPVOID lpobjFocusUndo;
            OLEOPT_UPDATE update;
            CHAR szType[CBMESSAGEMAX];
            CHAR szFull[CBMESSAGEMAX * 4];
            INT idButton;
            INT iPane;

            iPane = (GetTopWindow(ghwndFrame) == ghwndPane[CONTENT]);
            lpobjFocus = glpobj[iPane];
            lpobjFocusUndo = glpobjUndo[iPane];
            lpObject = ((LPPICT)lpobjFocus)->lpObject;

            // Reset the list box
            SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

            if (msg == WM_INITDIALOG)
            {
                // If it wasn't a link it doesn't belong
                OleQueryType(lpObject, &otFocus);

                if (otFocus != OT_LINK)
                {
                    ghwndError = ghwndFrame;
                    EndDialog(hDlg, TRUE);
                    break;
                }

                PicSaveUndo(lpobjFocus);
                ghwndError = hDlg;
            }

            //
            // Redrawing the string, get the update options and
            // the button state for IDD_AUTO/IDD_MANUAL.
            //
            Error(OleGetLinkUpdateOptions(lpObject, &update));

            switch (update)
            {
            case oleupdate_always:
                LoadString(ghInst, IDS_AUTO, szType, CBMESSAGEMAX);
                idButton    = IDD_AUTO;
                break;

            case oleupdate_oncall:
                LoadString(ghInst, IDS_MANUAL, szType, CBMESSAGEMAX);
                idButton    = IDD_MANUAL;
                break;

            default:
                LoadString(ghInst, IDS_CANCELED, szType, CBMESSAGEMAX);
                idButton = -1;

                // Disable the change link button
                fChangeLink = FALSE;
            }

            //
            // Retrieve the server name (try it from Undo
            // if the object has been frozen)
            //
            if (Error(OleGetData(lpObject, gcfLink, &hData)) || !hData)
            {
                OleQueryType(lpObject, &otFocus);
                if (otFocus != OT_STATIC)
                {
                    ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                    return TRUE;
                }

                if (gptyUndo[iPane] == PICTURE &&
                    (Error(OleGetData(((LPPICT)lpobjFocusUndo)->lpObject,
                    gcfLink, &hData)) || !hData))
                {
                    ErrorMessage(E_GET_FROM_CLIPBOARD_FAILED);
                    return TRUE;
                }
            }

            // The link format is:  "szClass0szDocument0szItem00"
            if (hData && (lpstrData = GlobalLock(hData)))
            {
                // Retrieve the server's class ID
                RegGetClassId(szFull, lpstrData);
                lstrcat(szFull, "\t");

                // Display the Document and Item names
                while (*lpstrData++)
                    ;

                // Strip off the path name and drive letter
                lpstrTemp = lpstrData;
                while (*lpstrTemp)
                {
                    if (*lpstrTemp == '\\' || *lpstrTemp == ':')
                        lpstrData = lpstrTemp + 1;

                    if (gbDBCS)
                    {
                        lpstrTemp = CharNext(lpstrTemp);
                    }
                    else
                    {
                        lpstrTemp++;
                    }
                }

                // Append the file name
                lstrcat(szFull, lpstrData);
                lstrcat(szFull, "\t");

                // Append the item name
                while (*lpstrData++)
                    ;

                lstrcat(szFull, lpstrData);
                lstrcat(szFull, "\t");

                GlobalUnlock(hData);
            }
            else
            {
                lstrcpy(szFull, "\t\t\t");
            }

            // Append the type of link
            lstrcat(szFull, szType);

            // Draw the link in the list box
            SendMessage(hwndLB, LB_INSERTSTRING, (WPARAM) - 1, (LPARAM)szFull);

            if (msg == WM_REDRAW)
            {
                SendMessage(hwndLB, WM_SETREDRAW, 1, 0L);
                InvalidateRect(hwndLB, NULL, TRUE);
                Dirty();
            }

            // Uncheck those buttons that shouldn't be checked
            if (IsDlgButtonChecked(hDlg, IDD_AUTO) && (idButton != IDD_AUTO))
                CheckDlgButton(hDlg, IDD_AUTO, FALSE);

            if (IsDlgButtonChecked(hDlg, IDD_MANUAL) && (idButton != IDD_MANUAL))
                CheckDlgButton(hDlg, IDD_MANUAL, FALSE);

            // Check the dialog button, as appropriate
            if ((idButton == IDD_AUTO) || (idButton == IDD_MANUAL))
                CheckDlgButton(hDlg, idButton, TRUE);

            // Enable the other buttons appropriately
            EnableWindow(GetDlgItem(hDlg, IDD_CHANGE),
                ((otFocus != OT_STATIC) && fChangeLink));
            EnableWindow(GetDlgItem(hDlg, IDD_EDIT), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_PLAY), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_UPDATE), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_CHANGE), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_MANUAL), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_AUTO), (otFocus != OT_STATIC));
            EnableWindow(GetDlgItem(hDlg, IDD_FREEZE), (otFocus != OT_STATIC));

            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDCANCEL:
                PostMessage(ghwndFrame, WM_COMMAND, IDM_UNDO, 0L);

            case IDOK:
                ghwndError = ghwndFrame;
                EndDialog(hDlg, TRUE);
                return TRUE;

            default:
                break;
        }

        SendMessage(ghwndPane[GetTopWindow(ghwndFrame) == ghwndPane[CONTENT]],
            WM_COMMAND, wParam, 0L);

        switch (LOWORD(wParam))
        {
            // Dismiss the dialog on Edit/Activate
            case IDD_EDIT:
            case IDD_PLAY:
                ghwndError = ghwndFrame;
                EndDialog(hDlg, TRUE);
                return TRUE;

            default:
                break;
        }

        break;
    }

    return FALSE;
}



/* fnChangeText() - Label... dialog
 */
INT_PTR CALLBACK
fnChangeText(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDD_ICONTEXT, szIconText);
        SendDlgItemMessage(hDlg, IDD_ICONTEXT, EM_LIMITTEXT, 39, 0L);
        PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDD_ICONTEXT),
             1L);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_LABEL:
            PostMessage(hDlg, WM_NEXTDLGCTL,
                (WPARAM)GetDlgItem(hDlg, IDD_ICONTEXT), 1L);
            break;

        case IDOK:
            GetDlgItemText(hDlg, IDD_ICONTEXT, szIconText, CBMESSAGEMAX);
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        }
    }

    return FALSE;
}



/* fnInvalidLink() - Invalid Link dialog
 *
 * This is the two button "Link unavailable" dialog box.
 */
INT_PTR CALLBACK
fnInvalidLink(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDD_CHANGE:
            EndDialog(hDlg, LOWORD(wParam));
        }
    }

    return FALSE;
}
