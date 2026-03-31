/*
 * Win 3.1 Style File Dialogs
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
#include "wine/debug.h"
#include "wine/heap.h"
#include "winreg.h"
#include "winternl.h"
#include "commdlg.h"
#include "shlwapi.h"
#include "cderr.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"

#define BUFFILE 512
#define BUFFILEALLOC 512 * sizeof(WCHAR)

static const int fldrHeight = 16;
static const int fldrWidth = 20;

static HICON hFolder = 0;
static HICON hFolder2 = 0;
static HICON hFloppy = 0;
static HICON hHDisk = 0;
static HICON hCDRom = 0;
static HICON hNet = 0;

#define FD31_OFN_PROP "FILEDLG_OFN"

typedef struct tagFD31_DATA
{
    HWND hwnd; /* file dialog window handle */
    BOOL hook; /* TRUE if the dialog is hooked */
    UINT lbselchstring; /* registered message id */
    UINT fileokstring; /* registered message id */
    LPARAM lParam; /* save original lparam */
    LPCVOID template; /* template for 32 bits resource */
    BOOL open; /* TRUE if open dialog, FALSE if save dialog */
    LPOPENFILENAMEW ofnW; /* pointer either to the original structure or
                             a W copy for A/16 API */
    LPOPENFILENAMEA ofnA; /* original structure if 32bits ansi dialog */
} FD31_DATA, *PFD31_DATA;

/***********************************************************************
 * 				FD31_Init			[internal]
 */
static BOOL FD31_Init(void)
{
    static BOOL initialized = FALSE;

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
	    ERR("Error loading icons!\n");
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

    GetDlgItemTextW( hwnd, edt1, temp, ARRAY_SIZE(temp));
    cp = wcsrchr(temp, '\\');
    if (cp != NULL) {
	lstrcpyW(temp, cp+1);
    }
    cp = wcsrchr(temp, ':');
    if (cp != NULL) {
	lstrcpyW(temp, cp+1);
    }
    /* FIXME: shouldn't we do something with the result here? ;-) */
}

/***********************************************************************
 *                              FD31_CallWindowProc          [internal]
 *
 *      Call the appropriate hook
 */
static BOOL FD31_CallWindowProc(const FD31_DATA *lfs, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL ret;

    if (lfs->ofnA)
    {
        TRACE("Call hookA %p (%p, %04x, %08Ix, %08Ix)\n",
               lfs->ofnA->lpfnHook, lfs->hwnd, wMsg, wParam, lParam);
        ret = lfs->ofnA->lpfnHook(lfs->hwnd, wMsg, wParam, lParam);
        TRACE("ret hookA %p (%p, %04x, %08Ix, %08Ix)\n",
               lfs->ofnA->lpfnHook, lfs->hwnd, wMsg, wParam, lParam);
        return ret;
    }

    TRACE("Call hookW %p (%p, %04x, %08Ix, %08Ix)\n",
           lfs->ofnW->lpfnHook, lfs->hwnd, wMsg, wParam, lParam);
    ret = lfs->ofnW->lpfnHook(lfs->hwnd, wMsg, wParam, lParam);
    TRACE("Ret hookW %p (%p, %04x, %08Ix, %08Ix)\n",
           lfs->ofnW->lpfnHook, lfs->hwnd, wMsg, wParam, lParam);
    return ret;
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
  return L"*.*"; /* FIXME */
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
	    scptr = wcschr(filter, ';');
	    if (scptr)	*scptr = 0;
	    while (*filter == ' ') filter++;
	    TRACE("Using file spec %s\n", debugstr_w(filter));
	    SendMessageW(hdlg, LB_DIR, 0, (LPARAM)filter);
	    if (scptr) *scptr = ';';
	    filter = (scptr) ? (scptr + 1) : 0;
	 }
    }

    /* list of directories */
    lstrcpyW(buffer, L"*.*");

    if (GetDlgItem(hWnd, lst2) != 0) {
        lRet = DlgDirListW(hWnd, buffer, lst2, stc1, DDL_EXCLUSIVE | DDL_DIRECTORY);
    }
    SetCursor(oldCursor);
    return lRet;
}

/***********************************************************************
 *                              FD31_WMDrawItem              [internal]
 */
static LONG FD31_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
			    int savedlg, const DRAWITEMSTRUCT *lpdis)
{
    WCHAR *str;
    HICON hIcon;
    COLORREF oldText = 0, oldBk = 0;

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst1)
    {
        if (!(str = heap_alloc(BUFFILEALLOC))) return FALSE;
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
        heap_free(str);
	return TRUE;
    }

    if (lpdis->CtlType == ODT_LISTBOX && lpdis->CtlID == lst2)
    {
        if (!(str = heap_alloc(BUFFILEALLOC)))
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
	DrawIconEx( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hFolder, 16, 16, 0, 0, DI_NORMAL );
        heap_free(str);
	return TRUE;
    }
    if (lpdis->CtlType == ODT_COMBOBOX && lpdis->CtlID == cmb2)
    {
        char root[] = "a:";
        if (!(str = heap_alloc(BUFFILEALLOC)))
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
	DrawIconEx( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, hIcon, 16, 16, 0, 0, DI_NORMAL );
        heap_free(str);
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
    LPOPENFILENAMEA ofnA = lfs->ofnA;
    WCHAR tmpstr2[BUFFILE];
    WCHAR *p;

    TRACE("%s\n", debugstr_w(tmpstr));
    if(ofnW->Flags & OFN_NOVALIDATE)
        tmpstr2[0] = '\0';
    else
        GetCurrentDirectoryW(BUFFILE, tmpstr2);
    lenstr2 = lstrlenW(tmpstr2);
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
    if (ofnA)
    {
        LPSTR lpszTemp;
        if (ofnW->nMaxFile &&
            !WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFile, -1,
                                  ofnA->lpstrFile, ofnA->nMaxFile, NULL, NULL ))
            ofnA->lpstrFile[ofnA->nMaxFile-1] = 0;

        /* offsets are not guaranteed to be the same in WCHAR to MULTIBYTE conversion */
        /* set filename offset */
        lpszTemp = PathFindFileNameA(ofnA->lpstrFile);
        ofnA->nFileOffset = (lpszTemp - ofnA->lpstrFile);

        /* set extension offset */
        lpszTemp = PathFindExtensionA(ofnA->lpstrFile);
        ofnA->nFileExtension = (*lpszTemp) ? (lpszTemp - ofnA->lpstrFile) + 1 : 0;
    }
}

/***********************************************************************
 *                              FD31_UpdateFileTitle         [internal]
 *      update the displayed file name (without path)
 */
static void FD31_UpdateFileTitle(const FD31_DATA *lfs)
{
  LONG lRet;
  LPOPENFILENAMEW ofnW = lfs->ofnW;
  LPOPENFILENAMEA ofnA = lfs->ofnA;

  if (ofnW->lpstrFileTitle != NULL)
  {
    lRet = SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETTEXT, lRet,
                             (LPARAM)ofnW->lpstrFileTitle );
    if (ofnA)
    {
        if (!WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFileTitle, -1,
                                  ofnA->lpstrFileTitle, ofnA->nMaxFileTitle, NULL, NULL ))
            ofnA->lpstrFileTitle[ofnA->nMaxFileTitle-1] = 0;
    }
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
  pstr = heap_alloc(BUFFILEALLOC);
  SendDlgItemMessageW(hWnd, lst2, LB_GETTEXT, lRet,
		     (LPARAM)pstr);
  lstrcpyW( tmpstr, pstr );
  heap_free(pstr);
  /* get the selected directory in tmpstr */
  if (tmpstr[0] == '[')
    {
      tmpstr[lstrlenW(tmpstr) - 1] = 0;
      lstrcpyW(tmpstr,tmpstr+1);
    }
  lstrcatW(tmpstr, L"\\");

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

    lRet =  SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETCURSEL, 0, 0);
    if (lRet == LB_ERR)
        return TRUE;

    /* set the edit control to the chosen file */
    if ((pstr = heap_alloc(BUFFILEALLOC)))
    {
        SendDlgItemMessageW(hWnd, lst1, LB_GETTEXT, lRet,
                       (LPARAM)pstr);
        SetDlgItemTextW( hWnd, edt1, pstr );
        heap_free(pstr);
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

    pBeginFileName = wcsrchr(path, '\\');
    if (pBeginFileName == NULL)
	pBeginFileName = wcsrchr(path, ':');

    if (wcschr(path,'*') != NULL || wcschr(path,'?') != NULL)
    {
        /* edit control contains wildcards */
        if (pBeginFileName != NULL)
        {
	    lstrcpynW(tmpstr2, pBeginFileName + 1, BUFFILE);
	    *(pBeginFileName + 1) = 0;
	}
	else
	{
	    lstrcpyW(tmpstr2, path);
            if(!(lfs->ofnW->Flags & OFN_NOVALIDATE))
                *path = 0;
        }

        TRACE("path=%s, tmpstr2=%s\n", debugstr_w(path), debugstr_w(tmpstr2));
        SetDlgItemTextW( hWnd, edt1, tmpstr2 );
        FD31_ScanDir(lfs->ofnW, hWnd, path);
        return (lfs->ofnW->Flags & OFN_NOVALIDATE) != 0;
    }

    /* no wildcards, we might have a directory or a filename */
    /* try appending a wildcard and reading the directory */

    pstr2 = path + lstrlenW(path);
    if (pBeginFileName == NULL || *(pBeginFileName + 1) != 0)
        lstrcatW(path, L"\\");

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

        lstrcpynW(tmpstr2, pBeginFileName + 1, ARRAY_SIZE(tmpstr2));
        /* Should we MessageBox() if this fails? */
        if (!FD31_ScanDir(lfs->ofnW, hWnd, path))
        {
            return FALSE;
        }
        lstrcpyW(path, tmpstr2);
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
    int copied_size = min( ofnW->lStructSize, sizeof(ofnsav) );

    memcpy( &ofnsav, ofnW, copied_size ); /* for later restoring */

    /* get current file name */
    if (path)
        lstrcpynW(filename, path, ARRAY_SIZE(filename));
    else
        GetDlgItemTextW( hWnd, edt1, filename, ARRAY_SIZE(filename));

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
            memcpy( ofnW, &ofnsav, copied_size ); /* restore old state */
            return FALSE;
        }
    }
    if ((ofnW->Flags & OFN_ALLOWMULTISELECT) && (ofnW->Flags & OFN_EXPLORER))
    {
        if (ofnW->lpstrFile)
        {
            LPWSTR str = ofnW->lpstrFile;
            LPWSTR ptr = wcsrchr(str, '\\');
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
    pstr = heap_alloc(BUFFILEALLOC);
    SendDlgItemMessageW(hWnd, cmb2, CB_GETLBTEXT, lRet,
                         (LPARAM)pstr);
    wsprintfW(diskname, L"%c:", pstr[2]);
    heap_free(pstr);

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
    lfs->ofnW->nFilterIndex = lRet + 1;
    if (lfs->ofnA)
        lfs->ofnA->nFilterIndex = lRet + 1;
    pstr = (LPWSTR)SendDlgItemMessageW(lfs->hwnd, cmb1, CB_GETITEMDATA, lRet, 0);
    TRACE("Selected filter : %s\n", debugstr_w(pstr));

    return FD31_Validate( lfs, pstr, cmb1, lRet, TRUE );
}

/***********************************************************************
 *                              FD31_WMCommand               [internal]
 */
static LRESULT FD31_WMCommand( HWND hWnd, LPARAM lParam, UINT notification,
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
    x = heap_alloc(len * sizeof(WCHAR));
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
        strW = heap_alloc(size * sizeof(WCHAR));
        if (strW) MultiByteToWideChar( CP_ACP, 0, str, -1, strW, size );
    }
    return strW;
}

/************************************************************************
 *                              FD31_MapOfnStructA          [internal]
 *      map a 32 bits Ansi structure to a Unicode one
 */
static void FD31_MapOfnStructA(const OPENFILENAMEA *ofnA, LPOPENFILENAMEW ofnW, BOOL open)
{
    UNICODE_STRING usBuffer;

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
        LoadStringW(COMDLG32_hInstance, open ? IDS_OPEN_FILE : IDS_SAVE_AS, buf, ARRAY_SIZE(buf));
        len = lstrlenW(buf)+1;
        title_tmp = heap_alloc(len * sizeof(WCHAR));
        memcpy(title_tmp, buf, len * sizeof(WCHAR));
        ofnW->lpstrTitle = title_tmp;
    }
    ofnW->Flags = ofnA->Flags;
    ofnW->nFileOffset = ofnA->nFileOffset;
    ofnW->nFileExtension = ofnA->nFileExtension;
    ofnW->lpstrDefExt = FD31_DupToW(ofnA->lpstrDefExt, 3);
    if ((ofnA->Flags & OFN_ENABLETEMPLATE) && (ofnA->lpTemplateName))
    {
        if (!IS_INTRESOURCE(ofnA->lpTemplateName))
        {
            RtlCreateUnicodeStringFromAsciiz (&usBuffer,ofnA->lpTemplateName);
            ofnW->lpTemplateName = usBuffer.Buffer;
        }
        else /* numbered resource */
            ofnW->lpTemplateName = (LPCWSTR) ofnA->lpTemplateName;
    }
    if (ofnW->lStructSize > OPENFILENAME_SIZE_VERSION_400W)
    {
        ofnW->pvReserved = ofnA->pvReserved;
        ofnW->dwReserved = ofnA->dwReserved;
        ofnW->FlagsEx    = ofnA->FlagsEx;
    }
}


/************************************************************************
 *                              FD31_FreeOfnW          [internal]
 *      Undo all allocations done by FD31_MapOfnStructA
 */
static void FD31_FreeOfnW(OPENFILENAMEW *ofnW)
{
    heap_free((void *)ofnW->lpstrFilter);
    heap_free(ofnW->lpstrCustomFilter);
    heap_free(ofnW->lpstrFile);
    heap_free(ofnW->lpstrFileTitle);
    heap_free((void *)ofnW->lpstrInitialDir);
    heap_free((void *)ofnW->lpstrTitle);
    if (!IS_INTRESOURCE(ofnW->lpTemplateName))
        heap_free((void *)ofnW->lpTemplateName);
}

/************************************************************************
 *                              FD31_DestroyPrivate            [internal]
 *      destroys the private object
 */
static void FD31_DestroyPrivate(PFD31_DATA lfs)
{
    HWND hwnd;
    if (!lfs) return;
    hwnd = lfs->hwnd;
    TRACE("destroying private allocation %p\n", lfs);

    /* if ofnW has been allocated, have to free everything in it */
    if (lfs->ofnA)
    {
        FD31_FreeOfnW(lfs->ofnW);
        heap_free(lfs->ofnW);
    }
    heap_free(lfs);
    RemovePropA(hwnd, FD31_OFN_PROP);
}

/***********************************************************************
 *           FD31_GetTemplate                                  [internal]
 *
 * Get a template (or FALSE if failure) when 16 bits dialogs are used
 * by a 32 bits application
 *
 */
static BOOL FD31_GetTemplate(PFD31_DATA lfs)
{
    LPOPENFILENAMEW ofnW = lfs->ofnW;
    LPOPENFILENAMEA ofnA = lfs->ofnA;
    HANDLE hDlgTmpl;

    if (ofnW->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
        if (!(lfs->template = LockResource( ofnW->hInstance )))
        {
            COMDLG32_SetCommDlgExtendedError( CDERR_LOADRESFAILURE );
            return FALSE;
        }
    }
    else if (ofnW->Flags & OFN_ENABLETEMPLATE)
    {
        HRSRC hResInfo;
        if (ofnA)
            hResInfo = FindResourceA( ofnA->hInstance, ofnA->lpTemplateName, (LPSTR)RT_DIALOG );
        else
            hResInfo = FindResourceW( ofnW->hInstance, ofnW->lpTemplateName, (LPWSTR)RT_DIALOG );
        if (!hResInfo)
        {
            COMDLG32_SetCommDlgExtendedError( CDERR_FINDRESFAILURE );
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource( ofnW->hInstance, hResInfo )) ||
            !(lfs->template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError( CDERR_LOADRESFAILURE );
            return FALSE;
        }
    }
    else /* get it from internal Wine resource */
    {
        HRSRC hResInfo;
        if (!(hResInfo = FindResourceA( COMDLG32_hInstance, lfs->open ? "OPEN_FILE" : "SAVE_FILE", (LPSTR)RT_DIALOG )))
        {
            COMDLG32_SetCommDlgExtendedError( CDERR_FINDRESFAILURE );
            return FALSE;
        }
        if (!(hDlgTmpl = LoadResource( COMDLG32_hInstance, hResInfo )) ||
            !(lfs->template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError( CDERR_LOADRESFAILURE );
            return FALSE;
        }
    }
    return TRUE;
}

/************************************************************************
 *                              FD31_AllocPrivate            [internal]
 *      allocate a private object to hold 32 bits Unicode
 *      structure that will be used throughout the calls, while
 *      keeping available the original structures and a few variables
 *      On entry : type = dialog procedure type (16,32A,32W)
 *                 dlgType = dialog type (open or save)
 */
static PFD31_DATA FD31_AllocPrivate(LPARAM lParam, UINT dlgType, BOOL IsUnicode)
{
    FD31_DATA *lfs = heap_alloc_zero(sizeof(*lfs));

    TRACE("alloc private buf %p\n", lfs);
    if (!lfs) return NULL;
    lfs->hook = FALSE;
    lfs->lParam = lParam;
    lfs->open = (dlgType == OPEN_DIALOG);

    if (IsUnicode)
    {
        lfs->ofnA = NULL;
        lfs->ofnW = (LPOPENFILENAMEW) lParam;
        if (lfs->ofnW->Flags & OFN_ENABLEHOOK)
            if (lfs->ofnW->lpfnHook)
                lfs->hook = TRUE;
    }
    else
    {
        lfs->ofnA = (LPOPENFILENAMEA) lParam;
        if (lfs->ofnA->Flags & OFN_ENABLEHOOK)
            if (lfs->ofnA->lpfnHook)
                lfs->hook = TRUE;
        lfs->ofnW = heap_alloc_zero(lfs->ofnA->lStructSize);
        lfs->ofnW->lStructSize = lfs->ofnA->lStructSize;
        FD31_MapOfnStructA(lfs->ofnA, lfs->ofnW, lfs->open);
    }

    if (! FD31_GetTemplate(lfs))
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
static LONG FD31_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i, n;
  WCHAR tmpstr[BUFFILE];
  LPWSTR pstr, old_pstr;
  LPOPENFILENAMEW ofn;
  PFD31_DATA lfs = (PFD31_DATA) lParam;

  if (!lfs) return FALSE;
  SetPropA(hWnd, FD31_OFN_PROP, lfs);
  lfs->hwnd = hWnd;
  ofn = lfs->ofnW;

  TRACE("flags=%lx initialdir=%s\n", ofn->Flags, debugstr_w(ofn->lpstrInitialDir));

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
    TRACE("nFilterIndex = %ld, SetText of edt1 to %s\n",
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

static int FD31_GetFldrHeight(void)
{
  return fldrHeight;
}

/***********************************************************************
 *                              FD31_WMMeasureItem           [internal]
 */
static LONG FD31_WMMeasureItem(LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmeasure;

    lpmeasure = (LPMEASUREITEMSTRUCT)lParam;
    lpmeasure->itemHeight = FD31_GetFldrHeight();
    return TRUE;
}


/***********************************************************************
 *           FileOpenDlgProc                                    [internal]
 *      Used for open and save, in fact.
 */
static INT_PTR CALLBACK FD31_FileOpenDlgProc(HWND hWnd, UINT wMsg,
                                             WPARAM wParam, LPARAM lParam)
{
    PFD31_DATA lfs = (PFD31_DATA)GetPropA( hWnd, FD31_OFN_PROP );

    TRACE("msg=%x wparam=%Ix lParam=%Ix\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
    {
        INT_PTR lRet;
        lRet  = (INT_PTR)FD31_CallWindowProc( lfs, wMsg, wParam, lParam );
        if (lRet) return lRet;   /* else continue message processing */
    }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FD31_WMInitDialog( hWnd, wParam, lParam );

    case WM_MEASUREITEM:
        return FD31_WMMeasureItem( lParam );

    case WM_DRAWITEM:
        return FD31_WMDrawItem( hWnd, wParam, lParam, !lfs->open, (DRAWITEMSTRUCT *)lParam );

    case WM_COMMAND:
        return FD31_WMCommand( hWnd, lParam, HIWORD(wParam), LOWORD(wParam), lfs );
#if 0
    case WM_CTLCOLOR:
        SetBkColor( (HDC16)wParam, 0x00C0C0C0 );
        switch (HIWORD(lParam))
        {
        case CTLCOLOR_BTN:
            SetTextColor( (HDC16)wParam, 0x00000000 );
            return hGRAYBrush;
        case CTLCOLOR_STATIC:
            SetTextColor( (HDC16)wParam, 0x00000000 );
            return hGRAYBrush;
        }
        break;
#endif
    }
    return FALSE;
}

/***********************************************************************
 *           GetFileName31A                                 [internal]
 *
 * Creates a win31 style dialog box for the user to select a file to open/save.
 */
BOOL GetFileName31A( OPENFILENAMEA *lpofn, UINT dlgType )
{
    BOOL bRet = FALSE;
    PFD31_DATA lfs;

    if (!lpofn || !FD31_Init()) return FALSE;

    TRACE("ofn flags %08lx\n", lpofn->Flags);
    lfs = FD31_AllocPrivate((LPARAM) lpofn, dlgType, FALSE);
    if (lfs)
    {
        bRet = DialogBoxIndirectParamA( COMDLG32_hInstance, lfs->template, lpofn->hwndOwner,
                                        FD31_FileOpenDlgProc, (LPARAM)lfs);
        FD31_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", lpofn->lpstrFile);
    return bRet;
}

/***********************************************************************
 *           GetFileName31W                                 [internal]
 *
 * Creates a win31 style dialog box for the user to select a file to open/save
 */
BOOL GetFileName31W( OPENFILENAMEW *lpofn, UINT dlgType )
{
    BOOL bRet = FALSE;
    PFD31_DATA lfs;

    if (!lpofn || !FD31_Init()) return FALSE;

    lfs = FD31_AllocPrivate((LPARAM) lpofn, dlgType, TRUE);
    if (lfs)
    {
        bRet = DialogBoxIndirectParamW( COMDLG32_hInstance, lfs->template, lpofn->hwndOwner,
                                        FD31_FileOpenDlgProc, (LPARAM)lfs);
        FD31_DestroyPrivate(lfs);
    }

    TRACE("file %s, file offset %d, ext offset %d\n",
          debugstr_w(lpofn->lpstrFile), lpofn->nFileOffset, lpofn->nFileExtension);
    return bRet;
}
