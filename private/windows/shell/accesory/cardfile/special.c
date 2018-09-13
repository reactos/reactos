#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/* OLE - Paste special support                                          */
/************************************************************************/

NOEXPORT BOOL GetClipObjectInfo(LPTSTR szClass, LPTSTR szItem);
NOEXPORT BOOL SetupFormatsList(HWND hwndList);

#define MAX_CLIPFORMATS     20

int Formats[MAX_CLIPFORMATS];
int nFormats, iFormats;
BOOL fPaste;
int ClipFormat;


void DoPasteSpecial(void)
{
    int Result;

    if (EditMode == I_TEXT)
        return;

    Result= DialogBox(          hIndexInstance,
                      (LPTSTR)  MAKEINTRESOURCE(DLGPASTESPECIAL), 
                                hIndexWnd,
                      (DLGPROC) PasteSpecialDlgProc);
    if (Result == -1)
        return; /* probably error msg , out of mem */
    else if (Result == 0)
        return;   /* dlg cancelled */

    PicPaste( &CurCard, fPaste, (WORD) ((ClipFormat == vcfOwnerLink) ?
                                    0 : ClipFormat) );
}

int FAR PASCAL PasteSpecialDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    int i;
    static BOOL fLinkAvail, fEmbAvail, fObjAvail;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR szClassName[KEYNAMESIZE];
            TCHAR szItemName[PATHMAX];
            HWND hwndList;

            hwndList = GetDlgItem(hDlg,IDD_LISTBOX);
            szClassName[0] = szItemName[0] = TEXT('\0');
            iFormats = 0; /* always start with 0 */
            ClipFormat = 0;  /* no formats have been selected */ // lhb tracks

            fEmbAvail  = OleQueryCreateFromClip(szPStdFile, olerender_draw, 0) == OLE_OK;
            fLinkAvail = OleQueryLinkFromClip(szPStdFile, olerender_draw, 0) == OLE_OK;
            fObjAvail = fEmbAvail || fLinkAvail;

            // Show ole object first if available
            if (fObjAvail)
            {
               TCHAR szTmp[OBJSTRINGSMAX];
               TCHAR szListItem[OBJSTRINGSMAX+KEYNAMESIZE]; // hope this is big enough!

                GetClipObjectInfo(szClassName, szItemName);
                SetWindowText(GetDlgItem(hDlg,IDD_CLIPOWNER), szClassName);
                SetWindowText(GetDlgItem(hDlg,IDD_ITEM), szItemName);
                LoadString(hIndexInstance, IDS_OBJECTFMT, szTmp, CharSizeOf(szTmp));
                wsprintf(szListItem,TEXT("%s %s"), szClassName, szTmp);
                SendMessage(hwndList, LB_INSERTSTRING, iFormats, (LPARAM) szClassName);
                Formats[iFormats++] = vcfOwnerLink;
            }
            else
                ShowWindow(GetDlgItem(hDlg,IDD_SOURCE), SW_HIDE);

            SetupFormatsList(hwndList);

            if( !iFormats )     // if nothing to do, exit (how did we get here?)
            {
                EndDialog(hDlg, FALSE );
                return FALSE;
            }

            /* Choose the first one by default */
            SendMessage(hwndList, LB_SETCURSEL, 0, 0L);
            EnableWindow(GetDlgItem(hDlg, IDD_PASTELINK), fLinkAvail);

            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_LISTBOX:
                    switch (HIWORD(lParam))
                    {
                        case LBN_DBLCLK:
                            SendMessage (hDlg, WM_COMMAND, IDD_PASTE, 0L);
                            return TRUE;

                        default:
                            break;
                    }
                    break;

                case IDD_PASTE:
                case IDD_PASTELINK:
                    i = (WORD)SendDlgItemMessage(hDlg, IDD_LISTBOX, LB_GETCURSEL, 0, 0L);
                    if (i == LB_ERR)
                        break;

                    fPaste = (LOWORD(wParam) == IDD_PASTE);
                    ClipFormat = Formats[i];
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

/* fill hwndList with all the formats available on clipboard */
NOEXPORT BOOL SetupFormatsList(HWND hwndList)
{
   TCHAR szPicture[OBJSTRINGSMAX];
   TCHAR szBitmap[OBJSTRINGSMAX];
   WORD cfFormat = 0; // lhb tracks

    if (!OpenClipboard(hIndexWnd))
        return FALSE;

    LoadString(hIndexInstance, IDS_PICTUREFMT, szPicture, CharSizeOf(szPicture));
    LoadString(hIndexInstance, IDS_BITMAPFMT, szBitmap, CharSizeOf(szBitmap));

    while (cfFormat = EnumClipboardFormats(cfFormat))
    {
        switch(cfFormat)
        {
            case CF_BITMAP:
                SendMessage(hwndList, LB_INSERTSTRING, iFormats, (LPARAM) szBitmap);
                Formats[iFormats++] = cfFormat;
                break;

            case CF_METAFILEPICT:
                SendMessage(hwndList, LB_INSERTSTRING, iFormats, (LPARAM) szPicture);
                Formats[iFormats++] = cfFormat;
                break;

            default:
                break; //do nothing
        } //end switch
    }

    CloseClipboard();
    return iFormats;
}

/* get the classname, item name for the owner of the clipboard */
/* return TRUE if successful */
/* only gets ownerlink class, assumes its available */
NOEXPORT BOOL GetClipObjectInfo(LPTSTR szClass, LPTSTR szItem)
{
    HANDLE hData = INVALID_HANDLE_VALUE;
    LPTSTR lpData = NULL;
    TCHAR szFullItem[PATHMAX],*pch;
    DWORD dwSize = KEYNAMESIZE;
    TCHAR szShortClass[PATHMAX];
    INT   iDataSize;

        if (!OpenClipboard(hIndexWnd))
        return FALSE;

    if (!(hData = GetClipboardData(vcfOwnerLink)) &&
        !(hData = GetClipboardData(vcfLink)))
    {
        CloseClipboard();
        return FALSE;
    }

    if (!(lpData = GlobalLock(hData)))
    {
        CloseClipboard();
        return FALSE;
    }

    // convert to unicode if need be
    iDataSize= GlobalSize( hData );

    if( IsTextUnicode( lpData,GlobalSize(hData),NULL ) )
    {
        CopyMemory( szShortClass, lpData, iDataSize );
    }
    else
    {
        MultiByteToWideChar( CP_ACP, 
                             MB_PRECOMPOSED, 
                             (LPCSTR) lpData,
                             iDataSize,
                             szShortClass,
                             CharSizeOf(szShortClass) );
    }


    /**** get szClass ****/
    if (RegQueryValue(HKEY_CLASSES_ROOT, szShortClass, szClass, &dwSize))
            lstrcpy(szClass, szShortClass);

    lpData= (LPTSTR) &szShortClass;
    /**** get szName ****/
    while(*lpData++); // skip class key

    pch = szFullItem;

    /* first doc name */
    do
    {
       *pch++ = *lpData;
    } while(*lpData++);

    /* second item name (if there) */
    if (*lpData)
    {
        *(pch-1) = TEXT(' ');
        do
        {
            *pch++ = *lpData;
        } while(*lpData++);
    }

    /* get rid of path.  pch now points to \0 */
    --pch;
    while (pch != szFullItem)
    {
        if ((*(pch-1) == TEXT('\\')) || (*(pch-1) == TEXT(':')))
            break;
        else
            --pch;
    }

    lstrcpy(szItem, pch);

    GlobalUnlock(hData);
        CloseClipboard();

    return TRUE;
}
