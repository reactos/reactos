#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/* OLE - Insert New Object support                                      */
/************************************************************************/

NOEXPORT int NEAR InitClassNamesList(
    HWND hwndList);

NOEXPORT BOOL NEAR CreateObjectInDoc(
    PCARD pCard,
    TCHAR szClass[]);

NOEXPORT BOOL NEAR ServerNameFromClass(
    TCHAR szClass[],
    TCHAR szServer[]);

NOEXPORT BOOL NEAR GetSelectedClassName(
    HWND hwndList,
    TCHAR szSelectedClass[]);
/* set to FALSE at the beginning of Insertobject,
 * set to TRUE in Ole CallBack function on OLE_SAVE/OLE_CHANGE,
 * If FALSE when OLE_CLOSE received, then delete the Object(server closed without
 * updating the embedded object).
 */
BOOL fInsertComplete=TRUE;

// Class name picked up from dialog
static TCHAR szClassName[KEYNAMESIZE];

void InsertObject(
    void)
{
    int Result;

    if (EditMode == I_TEXT)
        return;

    Result = DialogBox( hIndexInstance,
                        (LPTSTR) MAKEINTRESOURCE(DLGINSERTOBJECT),
                        hIndexWnd,
                        (WNDPROC) InsertObjectDlgProc);

    if (Result == -1)
        return; /* probably error msg , out of mem */
    else if (Result == 0 || szClassName[0] == 0)
        return;   /* dlg cancelled */

    PicSaveUndo(&CurCard);
    CreateObjectInDoc(&CurCard, szClassName);
}

/* Insert New... dialog */
int FAR PASCAL InsertObjectDlgProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndList;

    switch (msg)
    {
        case WM_INITDIALOG:
            *szClassName = (TCHAR) 0;
            hwndList = GetDlgItem(hDlg, IDD_LISTBOX);
            if (!InitClassNamesList(hwndList))
            {
                TCHAR szMsg[50];

                LoadString(hIndexInstance, IDS_NOOLESERVERS, szMsg, CharSizeOf(szMsg));
                MessageBox(hIndexWnd, szMsg, szCardfile, MB_OK );
                EndDialog(hDlg, FALSE);
            }
            SendMessage(hwndList, LB_SETCURSEL, 0, 0L);
            SetFocus(hwndList);
            return FALSE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_LISTBOX:
#if !defined(WIN32)
                    if (HIWORD(lParam) != LBN_DBLCLK)
#else
                    if (HIWORD(wParam) != LBN_DBLCLK)
#endif
                        break;
                    /* fall through */
                case IDOK:
                    hwndList = GetDlgItem(hDlg, IDD_LISTBOX);
                    if (!GetSelectedClassName(hwndList, szClassName))
                        EndDialog(hDlg, FALSE); /* error, similar to Cancel dlg */
                    else
                        EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;
            }
            break;
    }
    return FALSE;
}

// do something to get rid of warnings
#ifndef OLE_20
#define OLECHAR CHAR
#else
#define OLECHAR TCHAR
#endif

NOEXPORT BOOL NEAR CreateObjectInDoc(
    PCARD pCard,
    TCHAR szClass[])
{
    LPOLEOBJECT lpObject;
    OBJECTTYPE otObject;
    WORD fOleErrRet;
    OLECHAR szOleClass[KEYNAMESIZE];    // class name in ascii for ole

    // 
    // Convert class name to ascii if we need to
    //

#ifndef OLE_20
    WideCharToMultiByte(CP_ACP,
                        0,
                        szClass,
                        -1,   // api calculates length
                        szOleClass,
                        sizeof(szOleClass),
                        NULL,
                        NULL );
#else
    lstrcpy( szOleClass, szClass );
#endif

#ifndef OLE_20
    wsprintfA (szObjectName, szObjFormat, idObjectMax + 1);
#else
    wsprintfW (szObjectName, szObjFormat, idObjectMax + 1);
#endif

    fInsertComplete = FALSE;
    if (fOleErrRet = OleError(
                     OleCreate(szPStdFile,
                               lpclient,
                               szOleClass,
                               lhcdoc,
                               szObjectName,
                               &lpObject,
                               olerender_draw,
                               0)))
    {
        if (fOleErrRet == FOLEERROR_NOTGIVEN)
            ErrorMessage(E_CREATEOBJECTFAILED);
        return FALSE;
    }
    else
        otObject = EMBEDDED;

    DoSetHostNames(lpObject, otObject);

    if (lpObject)
    {
        if (pCard->lpObject)
            PicDelete(pCard);
        pCard->lpObject = lpObject;
        pCard->otObject = otObject;
        pCard->idObject = idObjectMax++;
        CurCardHead.flags |= FDIRTY;
    }

    return TRUE;
}

/* InitClassNamesList() - Fills the list box with possible server names. */
NOEXPORT int NEAR InitClassNamesList(
    HWND hwndList)
{
    TCHAR   szClass[KEYNAMESIZE];
    TCHAR   szServer[KEYNAMESIZE];
    HKEY    hkeyRoot;
    int     i;

    if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hkeyRoot))
        return FALSE;

    SendMessage(hwndList, LB_RESETCONTENT, 0, 0L);
    for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, KEYNAMESIZE); i++)
    {
        if (ServerNameFromClass(szClass, szServer))
            SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)szServer);
    }

    RegCloseKey(hkeyRoot);
    return SendMessage(hwndList, LB_GETCOUNT, 0, 0L);
}

/* for a given classname, it returns the server name if one exists.
 * returns TRUE on success, FALSE if given classname is not a server */
NOEXPORT BOOL NEAR ServerNameFromClass(
    TCHAR szClass[],
    TCHAR szServer[])
{
    DWORD   dwSize = KEYNAMESIZE;
    HKEY    hkeyTemp = NULL;
    TCHAR    szExec[KEYNAMESIZE];
    int     Result;

    if (*szClass != TEXT('.'))           /* Not default extension... */
    {
        /* See if this class really refers to a server */
        wsprintf(szExec, TEXT("%s\\protocol\\StdFileEditing\\server"), (LPTSTR)szClass);
        if (!RegOpenKey(HKEY_CLASSES_ROOT, szExec, &hkeyTemp))
        {
            /* get the class name string */
            Result = RegQueryValue(HKEY_CLASSES_ROOT, szClass, szServer, (LONG FAR *)&dwSize);
            RegCloseKey(hkeyTemp);
            if (Result == 0)
                return TRUE;
        }
    }
    return FALSE;
}

/* GetSelectedClassName() - Returns the class id from the listbox.
 */
NOEXPORT BOOL NEAR GetSelectedClassName(
    HWND hwndList,
    TCHAR szSelectedClass[])
{
    TCHAR    szKey[KEYNAMESIZE];
    TCHAR    szClass[KEYNAMESIZE];
    TCHAR    szServer[KEYNAMESIZE];
    int     i;
    int     Sel;
    HKEY    hkeyRoot;

    if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hkeyRoot))
        return FALSE;

    Sel = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0L);
    SendMessage(hwndList, LB_GETTEXT, Sel, (LPARAM)szKey);

    for (i = 0;
         !RegEnumKey(HKEY_CLASSES_ROOT, i, szClass, KEYNAMESIZE); i++)
    {
        if (ServerNameFromClass(szClass, szServer) &&   /* if it is a server */
            lstrcmp(szServer, szKey) == 0)              /* and it is the one we want */
        {
            lstrcpy(szSelectedClass, szClass);
            RegCloseKey(hkeyRoot);
            return TRUE;
        }
    }

    RegCloseKey(hkeyRoot);
    return FALSE;
}
