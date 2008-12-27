/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "winreg.h"
#include "winternl.h"
#include "commdlg.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "filedlg31.h"

#define BUFFILE 512
#define BUFFILEALLOC 512 * sizeof(WCHAR)

static const WCHAR FILE_star[] = {'*','.','*', 0};
static const WCHAR FILE_bslash[] = {'\\', 0};
static const WCHAR FILE_specc[] = {'%','c',':', 0};
static const int fldrHeight = 16;
static const int fldrWidth = 20;

static HICON hFolder = 0;
static HICON hFolder2 = 0;
static HICON hFloppy = 0;
static HICON hHDisk = 0;
static HICON hCDRom = 0;
static HICON hNet = 0;

/***********************************************************************
 * 				FD31_Init			[internal]
 */
BOOL FD31_Init(void)
{
    static BOOL initialized = 0;

    if (!initialized) {
        hFolder  = LoadImageA( COMDLG32_hInstance, "FOLDER", IMAGE_ICON, 16, 16, LR_SHARED );
        hFolder2 = LoadImageA( COMDLG32_hInstance, "FOLDER2", IMAGE_ICON, 16, 16, LR_SHARED );
        hFloppy  = LoadImageA( COMDLG32_hInstance, "FLOPPY", IMAGE_ICON, 16, 16, LR_SHARED );
        hHDisk   = LoadImageA( COMDLG32_hInstance, "HDISK", IMAGE_ICON, 16, 16, LR_SHARED );
        hCDRom   = LoadImageA( COMDLG32_hInstance, "CDROM", IMAGE_ICON, 16, 16, LR_SHARED );
        hNet     = LoadImageA( COMDLG32_hInstance, "NETWORK", IMAGE_ICON, 16, 16, LR_SHARED );
	if (hFolder == 0 || hFolder2 == 0 || hFloppy == 0 ||
	    hHDisk == 0 || hCDRom == 0 || hNet == 0)
	{
	    ERR("Error loading icons !\n");
	    return FALSE;
	}
	initialized = TRUE;
    }
    return TRUE;
}

/***********************************************************************
 *                              FD31_StripEditControl        [internal]
 * Strip pathnames off the contents of the edit control.
 */
static void FD31_StripEditControl(HWND hwnd)
{
    WCHAR temp[BUFFILE], *cp;

    GetDlgItemTextW( hwnd, edt1, temp, sizeof(temp)/sizeof(WCHAR));
    cp = strrchrW(temp, '\\');
    if (cp != NULL) {
	strcpyW(temp, cp+1);
    }
    cp = strrchrW(temp, ':');
    if (cp != NULL) {
	strcpyW(temp, cp+1);
    }
    /* FIXME: shouldn't we do something with the result here? ;-) */
}

/***********************************************************************
 *                              FD31_CallWindowProc          [internal]
 *
 *      Call the appropriate hook
 */
BOOL FD31_CallWindowProc(const FD31_DATA *lfs, UINT wMsg, WPARAM wParam,
                         LPARAM lParam)
{
    return lfs->callbacks->CWP(lfs, wMsg, wParam, lParam);
}

/***********************************************************************
 * 				FD31_GetFileType		[internal]
 */
static LPCWSTR FD31_GetFileType(LPCWSTR cfptr, LPCWSTR fptr, const WORD index)
{
  int n, i;
  i = 0;
  if (cfptr)
    for ( ;(n = lstrlenW(cfptr)) != 0; i++)
      {
	cfptr += n + 1;
	if (i == index)
	  return cfptr;
	cfptr += lstrlenW(cfptr) + 1;
      }
  if (fptr)
    for ( ;(n = lstrlenW(fptr)) != 0; i++)
      {
	fptr += n + 1;
	if (i == index)
	  return fptr;
	fptr += lstrlenW(fptr) + 1;
    }
  return FILE_star; /* FIXME */
}

/***********************************************************************
 * 				FD31_ScanDir                 [internal]
 */
static BOOL FD31_ScanDir(const OPENFILENAMEW *ofn, HWND hWnd, LPCWSTR newPath)
{
    WCHAR   buffer[BUFFILE];
    HWND    hdlg;
    LRESULT lRet = TRUE;
    HCURSOR hCursorWait, oldCursor;

    TRACE("Trying to change to %s\n", debugstr_w(newPath));
    if  ( newPath[0] && !SetCurrentDirectoryW( newPath ))
        return FALSE;

    /* get the list of spec files */
    lstrcpynW(buffer, FD31_GetFileType(ofn->lpstrCustomFilter,
              ofn->lpstrFilter, ofn->nFilterIndex - 1), BUFFILE);

    hCursorWait = LoadCursorA(0, (LPSTR)IDC_WAIT);
    oldCursor = SetCursor(hCursorWait);

    /* list of files */
    if ((hdlg = GetDlgItem(hWnd, lst1)) != 0) {
        WCHAR*	scptr; /* ptr on semi-colon */
	WCHAR*	filter = buffer;

	TRACE("Using filter %s\n", debugstr_w(filter));
	SendMessageW(hdlg, LB_RESETCONTENT, 0, 0);
	while (filter) {
	    scptr = strchrW(filter, ';');
	    if (scptr)	*scptr = 0;
            while (*filter == ' ') filter++;
	    TRACE("Using file spec %s\n", debugstr_w(filter));
	    SendMessageW(hdlg, LB_DIR, 0, (LPARAM)filter);
	    if (scptr) *scptr = ';';
	        filter = (scptr) ? (scptr + 1) : 0;
	 }
    }

    /* list of directories */
    strcpyW(buffer, FILE_star);

    if (GetDlgItem(hWnd, lst2) != 0) {
        lRet = DlgDirListW(hWnd, buffer, lst2, stc1, DDL_EXCLUSIVE | DDL_DIRECTORY);
    }
    SetCursor(oldCursor);
    return lRet;
}

/***********************************************************************
 *                              FD31_WMDrawItem              [internal]
 */
LONG FD31_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
       int savedlg, const DRAWITEMSTRUCT *lpdis)
{
    WCHAR *str;
    HICON hIcon;
    COLORREF oldText = 0, oldBk = 0;

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1)
    {
        if (!(str = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC))) return FALSE;
	SendMessageW(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID,
                      (LPARAM)str);

	if ((lpdis->itemState & ODS_SELECTED) && !savedlg)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	if (savedlg)
	    SetTextColor(lpdis->hDC,GetSysColor(COLOR_GRAYTEXT) );

	ExtTextOutW(lpdis->hDC, lpdis->rcItem.left + 1,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, lstrlenW(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	    DrawFocusRect( lpdis->hDC, &(lpdis->rcItem) );

	if ((lpdis->itemState & ODS_SELECTED) && !savedlg)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
        HeapFree(GetProcessHeap(), 0, str);
	return TRUE;
    }

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst2)
    {
        if (!(str = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC)))
            return FALSE;
	SendMessageW(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID,
                      (LPARAM)str);

	if (lpdis->itemState & ODS_SELECTED)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	ExtTextOutW(lpdis->hDC, lpdis->rcItem.left + fldrWidth,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, lstrlenW(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	    DrawFocusRect( lpdis->hDC, &(lpdis->rcItem) );

	if (lpdis->itemState & ODS_SELECTED)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
	DrawIcon(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hFolder);
        HeapFree(GetProcessHeap(), 0, str);
	return TRUE;
    }
    if (lpdis->CtlType == ODT_COMBOBOX && lpdis->CtlID == cmb2)
    {
        char root[] = "a:";
        if (!(str = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC)))
            return FALSE;
	SendMessageW(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID,
                      (LPARAM)str);
        root[0] += str[2] - 'a';
        switch(GetDriveTypeA(root))
        {
        case DRIVE_REMOVABLE: hIcon = hFloppy; break;
        case DRIVE_CDROM:     hIcon = hCDRom; break;
        case DRIVE_REMOTE:    hIcon = hNet; break;
        case DRIVE_FIXED:
        default:           hIcon = hHDisk; break;
        }
	if (lpdis->itemState & ODS_SELECTED)
	{
	    oldBk = SetBkColor( lpdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
	    oldText = SetTextColor( lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	ExtTextOutW(lpdis->hDC, lpdis->rcItem.left + fldrWidth,
                  lpdis->rcItem.top + 1, ETO_OPAQUE | ETO_CLIPPED,
                  &(lpdis->rcItem), str, lstrlenW(str), NULL);

	if (lpdis->itemState & ODS_SELECTED)
	{
	    SetBkColor( lpdis->hDC, oldBk );
	    SetTextColor( lpdis->hDC, oldText );
	}
	DrawIcon(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon);
        HeapFree(GetProcessHeap(), 0, str);
	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *                              FD31_UpdateResult            [internal]
 *      update the displayed file name (with path)
 */
static void FD31_UpdateResult(const FD31_DATA *lfs, const WCHAR *tmpstr)
{
    int lenstr2;
    LPOPENFILENAMEW ofnW = lfs->ofnW;
    WCHAR tmpstr2[BUFFILE];
    WCHAR *p;

    TRACE("%s\n", debugstr_w(tmpstr));
    if(ofnW->Flags & OFN_NOVALIDATE)
        tmpstr2[0] = '\0';
    else
        GetCurrentDirectoryW(BUFFILE, tmpstr2);
    lenstr2 = strlenW(tmpstr2);
    if (lenstr2 > 3)
        tmpstr2[lenstr2++]='\\';
    lstrcpynW(tmpstr2+lenstr2, tmpstr, BUFFILE-lenstr2);
    if (!ofnW->lpstrFile)
        return;

    lstrcpynW(ofnW->lpstrFile, tmpstr2, ofnW->nMaxFile);

    /* set filename offset */
    p = PathFindFileNameW(ofnW->lpstrFile);
    ofnW->nFileOffset = (p - ofnW->lpstrFile);

    /* set extension offset */
    p = PathFindExtensionW(ofnW->lpstrFile);
    ofnW->nFileExtension = (*p) ? (p - ofnW->lpstrFile) + 1 : 0;

    TRACE("file %s, file offset %d, ext offset %d\n",
          debugstr_w(ofnW->lpstrFile), ofnW->nFileOffset, ofnW->nFileExtension);

    /* update the real client structures if any */
    lfs->callbacks->UpdateResult(lfs);
}

/***********************************************************************
 *                              FD31_UpdateFileTitle         [internal]
 *      update the displayed file name (without path)
 */
static void FD31_UpdateFileTitle(const FD31_DATA *lfs)
{
  LONG lRet;
  LPOPENFILENAMEW ofnW = lfs->ofnW;
  if (ofnW->lpstrFileTitle != NULL)
  {
    lRet = SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETTEXT, lRet,
                             (LPARAM)ofnW->lpstrFileTitle );
    lfs->callbacks->UpdateFileTitle(lfs);
  }
}

/***********************************************************************
 *                              FD31_DirListDblClick         [internal]
 */
static LRESULT FD31_DirListDblClick( const FD31_DATA *lfs )
{
  LONG lRet;
  HWND hWnd = lfs->hwnd;
  LPWSTR pstr;
  WCHAR tmpstr[BUFFILE];

  /* get the raw string (with brackets) */
  lRet = SendDlgItemMessageW(hWnd, lst2, LB_GETCURSEL, 0, 0);
  if (lRet == LB_ERR) return TRUE;
  pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC);
  SendDlgItemMessageW(hWnd, lst2, LB_GETTEXT, lRet,
		     (LPARAM)pstr);
  strcpyW( tmpstr, pstr );
  HeapFree(GetProcessHeap(), 0, pstr);
  /* get the selected directory in tmpstr */
  if (tmpstr[0] == '[')
    {
      tmpstr[lstrlenW(tmpstr) - 1] = 0;
      strcpyW(tmpstr,tmpstr+1);
    }
  strcatW(tmpstr, FILE_bslash);

  FD31_ScanDir(lfs->ofnW, hWnd, tmpstr);
  /* notify the app */
  if (lfs->hook)
    {
      if (FD31_CallWindowProc(lfs, lfs->lbselchstring, lst2,
              MAKELONG(lRet,CD_LBSELCHANGE)))
        return TRUE;
    }
  return TRUE;
}

/***********************************************************************
 *                              FD31_FileListSelect         [internal]
 *    called when a new item is picked in the file list
 */
static LRESULT FD31_FileListSelect( const FD31_DATA *lfs )
{
    LONG lRet;
    HWND hWnd = lfs->hwnd;
    LPWSTR pstr;

    lRet = lfs->callbacks->SendLbGetCurSel(lfs);
    if (lRet == LB_ERR)
        return TRUE;

    /* set the edit control to the choosen file */
    if ((pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC)))
    {
        SendDlgItemMessageW(hWnd, lst1, LB_GETTEXT, lRet,
                       (LPARAM)pstr);
        SetDlgItemTextW( hWnd, edt1, pstr );
        HeapFree(GetProcessHeap(), 0, pstr);
    }
    if (lfs->hook)
    {
        FD31_CallWindowProc(lfs, lfs->lbselchstring, lst1,
                           MAKELONG(lRet,CD_LBSELCHANGE));
    }
    /* FIXME: for OFN_ALLOWMULTISELECT we need CD_LBSELSUB, CD_SELADD,
           CD_LBSELNOITEMS */
    return TRUE;
}

/***********************************************************************
 *                              FD31_TestPath      [internal]
 *      before accepting the file name, test if it includes wild cards
 *      tries to scan the directory and returns TRUE if no error.
 */
static LRESULT FD31_TestPath( const FD31_DATA *lfs, LPWSTR path )
{
    HWND hWnd = lfs->hwnd;
    LPWSTR pBeginFileName, pstr2;
    WCHAR tmpstr2[BUFFILE];

    pBeginFileName = strrchrW(path, '\\');
    if (pBeginFileName == NULL)
	pBeginFileName = strrchrW(path, ':');

    if (strchrW(path,'*') != NULL || strchrW(path,'?') != NULL)
    {
        /* edit control contains wildcards */
        if (pBeginFileName != NULL)
        {
	    lstrcpynW(tmpstr2, pBeginFileName + 1, BUFFILE);
	    *(pBeginFileName + 1) = 0;
	}
	else
	{
	    strcpyW(tmpstr2, path);
            if(!(lfs->ofnW->Flags & OFN_NOVALIDATE))
                *path = 0;
        }

        TRACE("path=%s, tmpstr2=%s\n", debugstr_w(path), debugstr_w(tmpstr2));
        SetDlgItemTextW( hWnd, edt1, tmpstr2 );
        FD31_ScanDir(lfs->ofnW, hWnd, path);
        return (lfs->ofnW->Flags & OFN_NOVALIDATE) ? TRUE : FALSE;
    }

    /* no wildcards, we might have a directory or a filename */
    /* try appending a wildcard and reading the directory */

    pstr2 = path + lstrlenW(path);
    if (pBeginFileName == NULL || *(pBeginFileName + 1) != 0)
        strcatW(path, FILE_bslash);

    /* if ScanDir succeeds, we have changed the directory */
    if (FD31_ScanDir(lfs->ofnW, hWnd, path))
        return FALSE; /* and path is not a valid file name */

    /* if not, this must be a filename */

    *pstr2 = 0; /* remove the wildcard added before */

    if (pBeginFileName != NULL)
    {
        /* strip off the pathname */
        *pBeginFileName = 0;
        SetDlgItemTextW( hWnd, edt1, pBeginFileName + 1 );

        lstrcpynW(tmpstr2, pBeginFileName + 1, sizeof(tmpstr2)/sizeof(WCHAR) );
        /* Should we MessageBox() if this fails? */
        if (!FD31_ScanDir(lfs->ofnW, hWnd, path))
        {
            return FALSE;
        }
        strcpyW(path, tmpstr2);
    }
    else
        SetDlgItemTextW( hWnd, edt1, path );
    return TRUE;
}

/***********************************************************************
 *                              FD31_Validate               [internal]
 *   called on: click Ok button, Enter in edit, DoubleClick in file list
 */
static LRESULT FD31_Validate( const FD31_DATA *lfs, LPCWSTR path, UINT control, INT itemIndex,
                                 BOOL internalUse )
{
    LONG lRet;
    HWND hWnd = lfs->hwnd;
    OPENFILENAMEW ofnsav;
    LPOPENFILENAMEW ofnW = lfs->ofnW;
    WCHAR filename[BUFFILE];

    ofnsav = *ofnW; /* for later restoring */

    /* get current file name */
    if (path)
        lstrcpynW(filename, path, sizeof(filename)/sizeof(WCHAR));
    else
        GetDlgItemTextW( hWnd, edt1, filename, sizeof(filename)/sizeof(WCHAR));

    TRACE("got filename = %s\n", debugstr_w(filename));
    /* if we did not click in file list to get there */
    if (control != lst1)
    {
        if (!FD31_TestPath( lfs, filename) )
           return FALSE;
    }
    FD31_UpdateResult(lfs, filename);

    if (internalUse)
    { /* called internally after a change in a combo */
        if (lfs->hook)
        {
             FD31_CallWindowProc(lfs, lfs->lbselchstring, control,
                             MAKELONG(itemIndex,CD_LBSELCHANGE));
        }
        return TRUE;
    }

    FD31_UpdateFileTitle(lfs);
    if (lfs->hook)
    {
        lRet = FD31_CallWindowProc(lfs, lfs->fileokstring,
                  0, lfs->lParam );
        if (lRet)
        {
            *ofnW = ofnsav; /* restore old state */
            return FALSE;
        }
    }
    if ((ofnW->Flags & OFN_ALLOWMULTISELECT) && (ofnW->Flags & OFN_EXPLORER))
    {
        if (ofnW->lpstrFile)
        {
            LPWSTR str = ofnW->lpstrFile;
            LPWSTR ptr = strrchrW(str, '\\');
	    str[lstrlenW(str) + 1] = '\0';
	    *ptr = 0;
        }
    }
    return TRUE;
}

/***********************************************************************
 *                              FD31_DiskChange             [internal]
 *    called when a new item is picked in the disk selection combo
 */
static LRESULT FD31_DiskChange( const FD31_DATA *lfs )
{
    LONG lRet;
    HWND hWnd = lfs->hwnd;
    LPWSTR pstr;
    WCHAR diskname[BUFFILE];

    FD31_StripEditControl(hWnd);
    lRet = SendDlgItemMessageW(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
    if (lRet == LB_ERR)
        return 0;
    pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC);
    SendDlgItemMessageW(hWnd, cmb2, CB_GETLBTEXT, lRet,
                         (LPARAM)pstr);
    wsprintfW(diskname, FILE_specc, pstr[2]);
    HeapFree(GetProcessHeap(), 0, pstr);

    return FD31_Validate( lfs, diskname, cmb2, lRet, TRUE );
}

/***********************************************************************
 *                              FD31_FileTypeChange         [internal]
 *    called when a new item is picked in the file type combo
 */
static LRESULT FD31_FileTypeChange( const FD31_DATA *lfs )
{
    LONG lRet;
    LPWSTR pstr;

    lRet = SendDlgItemMessageW(lfs->hwnd, cmb1, CB_GETCURSEL, 0, 0);
    if (lRet == LB_ERR)
        return TRUE;
    pstr = (LPWSTR)SendDlgItemMessageW(lfs->hwnd, cmb1, CB_GETITEMDATA, lRet, 0);
    TRACE("Selected filter : %s\n", debugstr_w(pstr));

    return FD31_Validate( lfs, NULL, cmb1, lRet, TRUE );
}

/***********************************************************************
 *                              FD31_WMCommand               [internal]
 */
LRESULT FD31_WMCommand(HWND hWnd, LPARAM lParam, UINT notification,
       UINT control, const FD31_DATA *lfs )
{
    switch (control)
    {
        case lst1: /* file list */
        FD31_StripEditControl(hWnd);
        if (notification == LBN_DBLCLK)
        {
            return SendMessageW(hWnd, WM_COMMAND, IDOK, 0);
        }
        else if (notification == LBN_SELCHANGE)
            return FD31_FileListSelect( lfs );
        break;

        case lst2: /* directory list */
        FD31_StripEditControl(hWnd);
        if (notification == LBN_DBLCLK)
            return FD31_DirListDblClick( lfs );
        break;

        case cmb1: /* file type drop list */
        if (notification == CBN_SELCHANGE)
            return FD31_FileTypeChange( lfs );
        break;

        case chx1:
        break;

        case pshHelp:
        break;

        case cmb2: /* disk dropdown combo */
        if (notification == CBN_SELCHANGE)
            return FD31_DiskChange( lfs );
        break;

        case IDOK:
        TRACE("OK pressed\n");
        if (FD31_Validate( lfs, NULL, control, 0, FALSE ))
            EndDialog(hWnd, TRUE);
        return TRUE;

        case IDCANCEL:
        EndDialog(hWnd, FALSE);
        return TRUE;

        case IDABORT: /* can be sent by the hook procedure */
        EndDialog(hWnd, TRUE);
        return TRUE;
    }
    return FALSE;
}

/************************************************************************
 *                              FD31_MapStringPairsToW       [internal]
 *      map string pairs to Unicode
 */
static LPWSTR FD31_MapStringPairsToW(LPCSTR strA, UINT size)
{
    LPCSTR s;
    LPWSTR x;
    unsigned int n, len;

    s = strA;
    while (*s)
        s = s+strlen(s)+1;
    s++;
    n = s + 1 - strA; /* Don't forget the other \0 */
    if (n < size) n = size;

    len = MultiByteToWideChar( CP_ACP, 0, strA, n, NULL, 0 );
    x = HeapAlloc(GetProcessHeap(),0, len * sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, strA, n, x, len );
    return x;
}


/************************************************************************
 *                              FD31_DupToW                  [internal]
 *      duplicates an Ansi string to unicode, with a buffer size
 */
static LPWSTR FD31_DupToW(LPCSTR str, DWORD size)
{
    LPWSTR strW = NULL;
    if (str && (size > 0))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        if (strW) MultiByteToWideChar( CP_ACP, 0, str, -1, strW, size );
    }
    return strW;
}

/************************************************************************
 *                              FD31_MapOfnStructA          [internal]
 *      map a 32 bits Ansi structure to a Unicode one
 */
void FD31_MapOfnStructA(const OPENFILENAMEA *ofnA, LPOPENFILENAMEW ofnW, BOOL open)
{
    UNICODE_STRING usBuffer;

    ofnW->lStructSize = sizeof(OPENFILENAMEW);
    ofnW->hwndOwner = ofnA->hwndOwner;
    ofnW->hInstance = ofnA->hInstance;
    if (ofnA->lpstrFilter)
        ofnW->lpstrFilter = FD31_MapStringPairsToW(ofnA->lpstrFilter, 0);

    if ((ofnA->lpstrCustomFilter) && (*(ofnA->lpstrCustomFilter)))
        ofnW->lpstrCustomFilter = FD31_MapStringPairsToW(ofnA->lpstrCustomFilter, ofnA->nMaxCustFilter);
    ofnW->nMaxCustFilter = ofnA->nMaxCustFilter;
    ofnW->nFilterIndex = ofnA->nFilterIndex;
    ofnW->nMaxFile = ofnA->nMaxFile;
    ofnW->lpstrFile = FD31_DupToW(ofnA->lpstrFile, ofnW->nMaxFile);
    ofnW->nMaxFileTitle = ofnA->nMaxFileTitle;
    ofnW->lpstrFileTitle = FD31_DupToW(ofnA->lpstrFileTitle, ofnW->nMaxFileTitle);
    if (ofnA->lpstrInitialDir)
    {
        RtlCreateUnicodeStringFromAsciiz (&usBuffer,ofnA->lpstrInitialDir);
        ofnW->lpstrInitialDir = usBuffer.Buffer;
    }
    if (ofnA->lpstrTitle) {
        RtlCreateUnicodeStringFromAsciiz (&usBuffer, ofnA->lpstrTitle);
        ofnW->lpstrTitle = usBuffer.Buffer;
    } else {
        WCHAR buf[16];
        LPWSTR title_tmp;
        int len;
        LoadStringW(COMDLG32_hInstance, open ? IDS_OPEN_FILE : IDS_SAVE_AS,
                    buf, sizeof(buf)/sizeof(WCHAR));
        len = lstrlenW(buf)+1;
        title_tmp = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        memcpy(title_tmp, buf, len * sizeof(WCHAR));
        ofnW->lpstrTitle = title_tmp;
    }
    ofnW->Flags = ofnA->Flags;
    ofnW->nFileOffset = ofnA->nFileOffset;
    ofnW->nFileExtension = ofnA->nFileExtension;
    ofnW->lpstrDefExt = FD31_DupToW(ofnA->lpstrDefExt, 3);
    if ((ofnA->Flags & OFN_ENABLETEMPLATE) && (ofnA->lpTemplateName))
    {
        if (HIWORD(ofnA->lpTemplateName))
        {
            RtlCreateUnicodeStringFromAsciiz (&usBuffer,ofnA->lpTemplateName);
            ofnW->lpTemplateName = usBuffer.Buffer;
        }
        else /* numbered resource */
            ofnW->lpTemplateName = (LPCWSTR) ofnA->lpTemplateName;
    }
}


/************************************************************************
 *                              FD31_FreeOfnW          [internal]
 *      Undo all allocations done by FD31_MapOfnStructA
 */
void FD31_FreeOfnW(OPENFILENAMEW *ofnW)
{
   HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrFilter);
   HeapFree(GetProcessHeap(), 0, ofnW->lpstrCustomFilter);
   HeapFree(GetProcessHeap(), 0, ofnW->lpstrFile);
   HeapFree(GetProcessHeap(), 0, ofnW->lpstrFileTitle);
   HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrInitialDir);
   HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrTitle);
   if (HIWORD(ofnW->lpTemplateName))
       HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpTemplateName);
}

/************************************************************************
 *                              FD31_DestroyPrivate            [internal]
 *      destroys the private object
 */
void FD31_DestroyPrivate(PFD31_DATA lfs)
{
    HWND hwnd;
    if (!lfs) return;
    hwnd = lfs->hwnd;
    TRACE("destroying private allocation %p\n", lfs);
    lfs->callbacks->Destroy(lfs);
    HeapFree(GetProcessHeap(), 0, lfs);
    RemovePropA(hwnd, FD31_OFN_PROP);
}

/************************************************************************
 *                              FD31_AllocPrivate            [internal]
 *      allocate a private object to hold 32 bits Unicode
 *      structure that will be used throughout the calls, while
 *      keeping available the original structures and a few variables
 *      On entry : type = dialog procedure type (16,32A,32W)
 *                 dlgType = dialog type (open or save)
 */
PFD31_DATA FD31_AllocPrivate(LPARAM lParam, UINT dlgType,
                             PFD31_CALLBACKS callbacks, DWORD data)
{
    PFD31_DATA lfs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FD31_DATA));

    TRACE("alloc private buf %p\n", lfs);
    if (!lfs) return NULL;
    lfs->hook = FALSE;
    lfs->lParam = lParam;
    lfs->open = (dlgType == OPEN_DIALOG);
    lfs->callbacks = callbacks;
    if (! lfs->callbacks->Init(lParam, lfs, data))
    {
        FD31_DestroyPrivate(lfs);
        return NULL;
    }
    lfs->lbselchstring = RegisterWindowMessageA(LBSELCHSTRINGA);
    lfs->fileokstring = RegisterWindowMessageA(FILEOKSTRINGA);

    return lfs;
}

/***********************************************************************
 *                              FD31_WMInitDialog            [internal]
 */

LONG FD31_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i, n;
  WCHAR tmpstr[BUFFILE];
  LPWSTR pstr, old_pstr;
  LPOPENFILENAMEW ofn;
  PFD31_DATA lfs = (PFD31_DATA) lParam;

  if (!lfs) return FALSE;
  SetPropA(hWnd, FD31_OFN_PROP, (HANDLE)lfs);
  lfs->hwnd = hWnd;
  ofn = lfs->ofnW;

  TRACE("flags=%x initialdir=%s\n", ofn->Flags, debugstr_w(ofn->lpstrInitialDir));

  SetWindowTextW( hWnd, ofn->lpstrTitle );
  /* read custom filter information */
  if (ofn->lpstrCustomFilter)
    {
      pstr = ofn->lpstrCustomFilter;
      n = 0;
      TRACE("lpstrCustomFilter = %p\n", pstr);
      while(*pstr)
	{
	  old_pstr = pstr;
          i = SendDlgItemMessageW(hWnd, cmb1, CB_ADDSTRING, 0,
                                   (LPARAM)(ofn->lpstrCustomFilter) + n );
          n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	  TRACE("add str=%s associated to %s\n",
                debugstr_w(old_pstr), debugstr_w(pstr));
          SendDlgItemMessageW(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
          n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	}
    }
  /* read filter information */
  if (ofn->lpstrFilter) {
	pstr = (LPWSTR) ofn->lpstrFilter;
	n = 0;
	while(*pstr) {
	  old_pstr = pstr;
	  i = SendDlgItemMessageW(hWnd, cmb1, CB_ADDSTRING, 0,
				       (LPARAM)(ofn->lpstrFilter + n) );
	  n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	  TRACE("add str=%s associated to %s\n",
                debugstr_w(old_pstr), debugstr_w(pstr));
	  SendDlgItemMessageW(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
	  n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	}
  }
  /* set default filter */
  if (ofn->nFilterIndex == 0 && ofn->lpstrCustomFilter == NULL)
  	ofn->nFilterIndex = 1;
  SendDlgItemMessageW(hWnd, cmb1, CB_SETCURSEL, ofn->nFilterIndex - 1, 0);
  if (ofn->lpstrFile && ofn->lpstrFile[0])
  {
    TRACE( "SetText of edt1 to %s\n", debugstr_w(ofn->lpstrFile) );
    SetDlgItemTextW( hWnd, edt1, ofn->lpstrFile );
  }
  else
  {
    lstrcpynW(tmpstr, FD31_GetFileType(ofn->lpstrCustomFilter,
	     ofn->lpstrFilter, ofn->nFilterIndex - 1),BUFFILE);
    TRACE("nFilterIndex = %d, SetText of edt1 to %s\n",
  			ofn->nFilterIndex, debugstr_w(tmpstr));
    SetDlgItemTextW( hWnd, edt1, tmpstr );
  }
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBoxW(hWnd, tmpstr, cmb2, 0, DDL_DRIVES | DDL_EXCLUSIVE);
  /* read initial directory */
  /* FIXME: Note that this is now very version-specific (See MSDN description of
   * the OPENFILENAME structure).  For example under 2000/XP any path in the
   * lpstrFile overrides the lpstrInitialDir, but not under 95/98/ME
   */
  if (ofn->lpstrInitialDir != NULL)
    {
      int len;
      lstrcpynW(tmpstr, ofn->lpstrInitialDir, 511);
      len = lstrlenW(tmpstr);
      if (len > 0 && tmpstr[len-1] != '\\'  && tmpstr[len-1] != ':') {
        tmpstr[len]='\\';
        tmpstr[len+1]='\0';
      }
    }
  else
    *tmpstr = 0;
  if (!FD31_ScanDir(ofn, hWnd, tmpstr)) {
    *tmpstr = 0;
    if (!FD31_ScanDir(ofn, hWnd, tmpstr))
      WARN("Couldn't read initial directory %s!\n", debugstr_w(tmpstr));
  }
  /* select current drive in combo 2, omit missing drives */
  {
      char dir[MAX_PATH];
      char str[4] = "a:\\";
      GetCurrentDirectoryA( sizeof(dir), dir );
      for(i = 0, n = -1; i < 26; i++)
      {
          str[0] = 'a' + i;
          if (GetDriveTypeA(str) > DRIVE_NO_ROOT_DIR) n++;
          if (toupper(str[0]) == toupper(dir[0])) break;
      }
  }
  SendDlgItemMessageW(hWnd, cmb2, CB_SETCURSEL, n, 0);
  if (!(ofn->Flags & OFN_SHOWHELP))
    ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
  if (ofn->Flags & OFN_HIDEREADONLY)
    ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
  if (lfs->hook)
      return FD31_CallWindowProc(lfs, WM_INITDIALOG, wParam, lfs->lParam);
  return TRUE;
}

int FD31_GetFldrHeight(void)
{
  return fldrHeight;
}
