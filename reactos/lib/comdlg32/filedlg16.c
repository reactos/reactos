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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "cderr.h"
#include "winreg.h"
#include "winternl.h"
#include "winuser.h"
#include "commdlg.h"
#include "cderr.h"
#include "winreg.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "cdlg16.h"

#define BUFFILE 512
#define BUFFILEALLOC 512 * sizeof(WCHAR)

struct FSPRIVATE
{
    HWND hwnd; /* file dialog window handle */
    BOOL hook; /* TRUE if the dialog is hooked */
    UINT lbselchstring; /* registered message id */
    UINT fileokstring; /* registered message id */
    LPARAM lParam; /* save original lparam */
    HANDLE16 hDlgTmpl16; /* handle for resource 16 */
    HANDLE16 hResource16; /* handle for allocated resource 16 */
    HANDLE16 hGlobal16; /* 16 bits mem block (resources) */
    LPCVOID template; /* template for 32 bits resource */
    BOOL open; /* TRUE if open dialog, FALSE if save dialog */
    OPENFILENAMEW *ofnW; /* original structure or work struct */
    OPENFILENAMEA *ofnA; /* original structure if 32bits ansi dialog */
    OPENFILENAME16 *ofn16; /* original structure if 16 bits dialog */
};

#define LFSPRIVATE struct FSPRIVATE *
#define LFS16 1
#define LFS32A 2
#define LFS32W 3
#define OFN_PROP "FILEDLG_OFN"

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
 * 				FileDlg_Init			[internal]
 */
static BOOL FileDlg_Init(void)
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
 *           Get32BitsTemplate                                  [internal]
 *
 * Get a template (or FALSE if failure) when 16 bits dialogs are used
 * by a 32 bits application
 *
 */
BOOL Get32BitsTemplate(LFSPRIVATE lfs)
{
    LPOPENFILENAMEW ofnW = lfs->ofnW;
    HANDLE hDlgTmpl;

    if (ofnW->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
	if (!(lfs->template = LockResource( ofnW->hInstance )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
    }
    else if (ofnW->Flags & OFN_ENABLETEMPLATE)
    {
	HRSRC hResInfo;
        if (lfs->ofnA)
	    hResInfo = FindResourceA(lfs->ofnA->hInstance,
				 lfs->ofnA->lpTemplateName,
                                 (LPSTR)RT_DIALOG);
        else
	    hResInfo = FindResourceW(ofnW->hInstance,
				 ofnW->lpTemplateName,
                                 (LPWSTR)RT_DIALOG);
        if (!hResInfo)
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl = LoadResource(ofnW->hInstance,
				hResInfo)) ||
		    !(lfs->template = LockResource(hDlgTmpl)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
    } else { /* get it from internal Wine resource */
	HRSRC hResInfo;
	if (!(hResInfo = FindResourceA(COMDLG32_hInstance,
             lfs->open? "OPEN_FILE":"SAVE_FILE", (LPSTR)RT_DIALOG)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
        }
        if (!(hDlgTmpl = LoadResource(COMDLG32_hInstance, hResInfo )) ||
                !(lfs->template = LockResource( hDlgTmpl )))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *           Get16BitsTemplate                                [internal]
 *
 * Get a template (FALSE if failure) when 16 bits dialogs are used
 * by a 16 bits application
 *
 */
BOOL Get16BitsTemplate(LFSPRIVATE lfs)
{
    LPOPENFILENAME16 ofn16 = lfs->ofn16;
    LPCVOID template;
    HGLOBAL16 hGlobal16 = 0;

    if (ofn16->Flags & OFN_ENABLETEMPLATEHANDLE)
        lfs->hDlgTmpl16 = ofn16->hInstance;
    else if (ofn16->Flags & OFN_ENABLETEMPLATE)
    {
	HANDLE16 hResInfo;
	if (!(hResInfo = FindResource16(ofn16->hInstance,
					MapSL(ofn16->lpTemplateName),
                                        (LPSTR)RT_DIALOG)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(lfs->hDlgTmpl16 = LoadResource16( ofn16->hInstance, hResInfo )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        lfs->hResource16 = lfs->hDlgTmpl16;
    }
    else
    { /* get resource from (32 bits) own Wine resource; convert it to 16 */
	HRSRC hResInfo;
	HGLOBAL hDlgTmpl32;
        LPCVOID template32;
        DWORD size;

	if (!(hResInfo = FindResourceA(COMDLG32_hInstance,
               lfs->open ? "OPEN_FILE":"SAVE_FILE", (LPSTR)RT_DIALOG)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl32 = LoadResource(COMDLG32_hInstance, hResInfo )) ||
	    !(template32 = LockResource( hDlgTmpl32 )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        size = SizeofResource(COMDLG32_hInstance, hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %ld bytes\n", size);
            return FALSE;
        }
        template = GlobalLock16(hGlobal16);
        if (!template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hGlobal16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)template);
        lfs->hDlgTmpl16 = hGlobal16;
        lfs->hGlobal16 = hGlobal16;
    }
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_StripEditControl        [internal]
 * Strip pathnames off the contents of the edit control.
 */
static void FILEDLG_StripEditControl(HWND hwnd)
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
 *                              FILEDLG_CallWindowProc          [internal]
 *
 *      Call the appropriate hook
 */
static BOOL FILEDLG_CallWindowProc(LFSPRIVATE lfs, UINT wMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    if (lfs->ofnA)
    {
        return (BOOL) CallWindowProcA(
          (WNDPROC)lfs->ofnA->lpfnHook, lfs->hwnd,
          wMsg, wParam, lParam);
    }

    if (lfs->ofnW)
    {
        return (BOOL) CallWindowProcW(
          (WNDPROC)lfs->ofnW->lpfnHook, lfs->hwnd,
          wMsg, wParam, lParam);
    }
    return FALSE;
}

/***********************************************************************
 * 				FILEDLG_ScanDir                 [internal]
 */
static BOOL FILEDLG_ScanDir(HWND hWnd, LPWSTR newPath)
{
    WCHAR		buffer[BUFFILE];
    HWND 		hdlg, hdlgDir;
    LRESULT             lRet = TRUE;
    HCURSOR             hCursorWait, oldCursor;

    TRACE("Trying to change to %s\n", debugstr_w(newPath));
    if  ( newPath[0] && !SetCurrentDirectoryW( newPath ))
        return FALSE;
    lstrcpynW(buffer, newPath, sizeof(buffer)/sizeof(WCHAR));

    /* get the list of spec files */
    GetDlgItemTextW(hWnd, edt1, buffer, sizeof(buffer)/sizeof(WCHAR));

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
	    if (SendMessageW(hdlg, LB_DIR, 0, (LPARAM)filter) == LB_ERR)
	        return FALSE;
	    if (scptr) *scptr = ';';
	        filter = (scptr) ? (scptr + 1) : 0;
	 }
    }

    /* list of directories */
    strcpyW(buffer, FILE_star);

    if ((hdlgDir = GetDlgItem(hWnd, lst2)) != 0) {
        lRet = DlgDirListW(hWnd, buffer, lst2, stc1, DDL_EXCLUSIVE | DDL_DIRECTORY);
    }
    SetCursor(oldCursor);
    return lRet;
}

/***********************************************************************
 * 				FILEDLG_GetFileType		[internal]
 */

static LPWSTR FILEDLG_GetFileType(LPWSTR cfptr, LPWSTR fptr, WORD index)
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
  return (LPWSTR) FILE_star; /* FIXME */
}

/***********************************************************************
 *                              FILEDLG_WMDrawItem              [internal]
 */
static LONG FILEDLG_WMDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam,
       int savedlg, LPDRAWITEMSTRUCT lpdis)
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
 *                              FILEDLG_UpdateResult            [internal]
 *      update the displayed file name (with path)
 */
void FILEDLG_UpdateResult(LFSPRIVATE lfs, WCHAR *tmpstr)
{
    int lenstr2;
    LPOPENFILENAMEW ofnW = lfs->ofnW;
    WCHAR tmpstr2[BUFFILE];
    WCHAR *bs;

    TRACE("%s\n", debugstr_w(tmpstr));
    if(ofnW->Flags & OFN_NOVALIDATE)
        tmpstr2[0] = '\0';
    else
        GetCurrentDirectoryW(BUFFILE, tmpstr2);
    lenstr2 = strlenW(tmpstr2);
    if (lenstr2 > 3)
        tmpstr2[lenstr2++]='\\';
    lstrcpynW(tmpstr2+lenstr2, tmpstr, BUFFILE-lenstr2);
    if (ofnW->lpstrFile)
        lstrcpynW(ofnW->lpstrFile, tmpstr2, ofnW->nMaxFile);
    if((bs = strrchrW(tmpstr2, '\\')) != NULL)
        ofnW->nFileOffset = bs - tmpstr2 +1;
    else
        ofnW->nFileOffset = 0;
    ofnW->nFileExtension = 0;
    while(tmpstr2[ofnW->nFileExtension] != '.' && tmpstr2[ofnW->nFileExtension] != '\0')
        ofnW->nFileExtension++;
    if (tmpstr2[ofnW->nFileExtension] == '\0')
        ofnW->nFileExtension = 0;
    else
        ofnW->nFileExtension++;
    /* update the real client structures if any */
    if (lfs->ofn16)
    { /* we have to convert to short (8.3) path */
	char tmp[1024]; /* MAX_PATHNAME_LEN */
	LPOPENFILENAME16 ofn16 = lfs->ofn16;
        char *dest = MapSL(ofn16->lpstrFile);
        char *bs16;
        if (!WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFile, -1,
                                  tmp, sizeof(tmp), NULL, NULL ))
            tmp[sizeof(tmp)-1] = 0;
	GetShortPathNameA(tmp, dest, ofn16->nMaxFile);

	/* the same procedure as every year... */
        if((bs16 = strrchr(dest, '\\')) != NULL)
            ofn16->nFileOffset = bs16 - dest +1;
        else
            ofn16->nFileOffset = 0;
        ofn16->nFileExtension = 0;
        while(dest[ofn16->nFileExtension] != '.' && dest[ofn16->nFileExtension] != '\0')
            ofn16->nFileExtension++;
        if (dest[ofn16->nFileExtension] == '\0')
            ofn16->nFileExtension = 0;
        else
            ofn16->nFileExtension++;
    }
    if (lfs->ofnA)
    {
        if (ofnW->nMaxFile &&
            !WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFile, -1,
                                  lfs->ofnA->lpstrFile, ofnW->nMaxFile, NULL, NULL ))
            lfs->ofnA->lpstrFile[ofnW->nMaxFile-1] = 0;
        lfs->ofnA->nFileOffset = ofnW->nFileOffset;
        lfs->ofnA->nFileExtension = ofnW->nFileExtension;
    }
}

/***********************************************************************
 *                              FILEDLG_UpdateFileTitle         [internal]
 *      update the displayed file name (without path)
 */
void FILEDLG_UpdateFileTitle(LFSPRIVATE lfs)
{
  LONG lRet;
  LPOPENFILENAMEW ofnW = lfs->ofnW;
  if (ofnW->lpstrFileTitle != NULL)
  {
    lRet = SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETCURSEL, 0, 0);
    SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETTEXT, lRet,
                             (LPARAM)ofnW->lpstrFileTitle );
    if (lfs->ofn16)
    {
        char *dest = MapSL(lfs->ofn16->lpstrFileTitle);
        if (!WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFileTitle, -1,
                                  dest, ofnW->nMaxFileTitle, NULL, NULL ))
            dest[ofnW->nMaxFileTitle-1] = 0;
    }
    if (lfs->ofnA)
    {
        if (!WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFileTitle, -1,
                                  lfs->ofnA->lpstrFileTitle, ofnW->nMaxFileTitle, NULL, NULL ))
            lfs->ofnA->lpstrFileTitle[ofnW->nMaxFileTitle-1] = 0;
    }
  }
}

/***********************************************************************
 *                              FILEDLG_DirListDblClick         [internal]
 */
static LRESULT FILEDLG_DirListDblClick( LFSPRIVATE lfs )
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

  FILEDLG_ScanDir(hWnd, tmpstr);
  /* notify the app */
  if (lfs->hook)
    {
      if (FILEDLG_CallWindowProc(lfs, lfs->lbselchstring, lst2,
              MAKELONG(lRet,CD_LBSELCHANGE)))
        return TRUE;
    }
  return TRUE;
}

/***********************************************************************
 *                              FILEDLG_FileListSelect         [internal]
 *    called when a new item is picked in the file list
 */
static LRESULT FILEDLG_FileListSelect( LFSPRIVATE lfs )
{
    LONG lRet;
    HWND hWnd = lfs->hwnd;
    LPWSTR pstr;

    lRet = SendDlgItemMessageW(hWnd, lst1, LB_GETCURSEL16, 0, 0);
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
        FILEDLG_CallWindowProc(lfs, lfs->lbselchstring, lst1,
                           MAKELONG(lRet,CD_LBSELCHANGE));
    }
    /* FIXME: for OFN_ALLOWMULTISELECT we need CD_LBSELSUB, CD_SELADD,
           CD_LBSELNOITEMS */
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_TestPath      [internal]
 *      before accepting the file name, test if it includes wild cards
 *      tries to scan the directory and returns TRUE if no error.
 */
static LRESULT FILEDLG_TestPath( LFSPRIVATE lfs, LPWSTR path )
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
        FILEDLG_ScanDir(hWnd, path);
        return (lfs->ofnW->Flags & OFN_NOVALIDATE) ? TRUE : FALSE;
    }

    /* no wildcards, we might have a directory or a filename */
    /* try appending a wildcard and reading the directory */

    pstr2 = path + lstrlenW(path);
    if (pBeginFileName == NULL || *(pBeginFileName + 1) != 0)
        strcatW(path, FILE_bslash);

    /* if ScanDir succeeds, we have changed the directory */
    if (FILEDLG_ScanDir(hWnd, path))
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
        if (!FILEDLG_ScanDir(hWnd, path))
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
 *                              FILEDLG_Validate               [internal]
 *   called on: click Ok button, Enter in edit, DoubleClick in file list
 */
static LRESULT FILEDLG_Validate( LFSPRIVATE lfs, LPWSTR path, UINT control, INT itemIndex,
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
        if (!FILEDLG_TestPath( lfs, filename) )
           return FALSE;
    }
    FILEDLG_UpdateResult(lfs, filename);

    if (internalUse)
    { /* called internally after a change in a combo */
        if (lfs->hook)
        {
             FILEDLG_CallWindowProc(lfs, lfs->lbselchstring, control,
                             MAKELONG(itemIndex,CD_LBSELCHANGE));
        }
        return TRUE;
    }

    FILEDLG_UpdateFileTitle(lfs);
    if (lfs->hook)
    {
        lRet = (BOOL)FILEDLG_CallWindowProc(lfs, lfs->fileokstring,
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
            LPWSTR str = (LPWSTR)ofnW->lpstrFile;
            LPWSTR ptr = strrchrW(str, '\\');
	    str[lstrlenW(str) + 1] = '\0';
	    *ptr = 0;
        }
    }
    return TRUE;
}

/***********************************************************************
 *                              FILEDLG_DiskChange             [internal]
 *    called when a new item is picked in the disk selection combo
 */
static LRESULT FILEDLG_DiskChange( LFSPRIVATE lfs )
{
    LONG lRet;
    HWND hWnd = lfs->hwnd;
    LPWSTR pstr;
    WCHAR diskname[BUFFILE];

    FILEDLG_StripEditControl(hWnd);
    lRet = SendDlgItemMessageW(hWnd, cmb2, CB_GETCURSEL, 0, 0L);
    if (lRet == LB_ERR)
        return 0;
    pstr = HeapAlloc(GetProcessHeap(), 0, BUFFILEALLOC);
    SendDlgItemMessageW(hWnd, cmb2, CB_GETLBTEXT, lRet,
                         (LPARAM)pstr);
    wsprintfW(diskname, FILE_specc, pstr[2]);
    HeapFree(GetProcessHeap(), 0, pstr);

    return FILEDLG_Validate( lfs, diskname, cmb2, lRet, TRUE );
}

/***********************************************************************
 *                              FILEDLG_FileTypeChange         [internal]
 *    called when a new item is picked in the file type combo
 */
static LRESULT FILEDLG_FileTypeChange( LFSPRIVATE lfs )
{
    LONG lRet;
    WCHAR diskname[BUFFILE];
    LPWSTR pstr;

    diskname[0] = 0;

    lRet = SendDlgItemMessageW(lfs->hwnd, cmb1, CB_GETCURSEL, 0, 0);
    if (lRet == LB_ERR)
        return TRUE;
    pstr = (LPWSTR)SendDlgItemMessageW(lfs->hwnd, cmb1, CB_GETITEMDATA, lRet, 0);
    TRACE("Selected filter : %s\n", debugstr_w(pstr));
    SetDlgItemTextW( lfs->hwnd, edt1, pstr );

    return FILEDLG_Validate( lfs, NULL, cmb1, lRet, TRUE );
}

/***********************************************************************
 *                              FILEDLG_WMCommand               [internal]
 */
static LRESULT FILEDLG_WMCommand(HWND hWnd, LPARAM lParam, UINT notification,
       UINT control, LFSPRIVATE lfs )
{
    switch (control)
    {
        case lst1: /* file list */
        FILEDLG_StripEditControl(hWnd);
        if (notification == LBN_DBLCLK)
        {
            if (FILEDLG_Validate( lfs, NULL, control, 0, FALSE ))
                EndDialog(hWnd, TRUE);
            return TRUE;
        }
        else if (notification == LBN_SELCHANGE)
            return FILEDLG_FileListSelect( lfs );
        break;

        case lst2: /* directory list */
        FILEDLG_StripEditControl(hWnd);
        if (notification == LBN_DBLCLK)
            return FILEDLG_DirListDblClick( lfs );
        break;

        case cmb1: /* file type drop list */
        if (notification == CBN_SELCHANGE)
            return FILEDLG_FileTypeChange( lfs );
        break;

        case chx1:
        break;

        case pshHelp:
        break;

        case cmb2: /* disk dropdown combo */
        if (notification == CBN_SELCHANGE)
            return FILEDLG_DiskChange( lfs );
        break;

        case IDOK:
        TRACE("OK pressed\n");
        if (FILEDLG_Validate( lfs, NULL, control, 0, FALSE ))
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

/***********************************************************************
 *                              FILEDLG_MapDrawItemStruct       [internal]
 *      map a 16 bits drawitem struct to 32
 */
static void FILEDLG_MapDrawItemStruct(LPDRAWITEMSTRUCT16 lpdis16, LPDRAWITEMSTRUCT lpdis)
{
    lpdis->CtlType = lpdis16->CtlType;
    lpdis->CtlID = lpdis16->CtlID;
    lpdis->itemID = lpdis16->itemID;
    lpdis->itemAction = lpdis16->itemAction;
    lpdis->itemState = lpdis16->itemState;
    lpdis->hwndItem = HWND_32(lpdis16->hwndItem);
    lpdis->hDC = HDC_32(lpdis16->hDC);
    lpdis->rcItem.right = lpdis16->rcItem.right;
    lpdis->rcItem.left = lpdis16->rcItem.left;
    lpdis->rcItem.top = lpdis16->rcItem.top;
    lpdis->rcItem.bottom = lpdis16->rcItem.bottom;
    lpdis->itemData = lpdis16->itemData;
}

/************************************************************************
 *                              FILEDLG_MapStringPairsToW       [internal]
 *      map string pairs to Unicode
 */
static LPWSTR FILEDLG_MapStringPairsToW(LPCSTR strA, UINT size)
{
    LPCSTR s;
    LPWSTR x;
    int n, len;

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
 *                              FILEDLG_DupToW                  [internal]
 *      duplicates an Ansi string to unicode, with a buffer size
 */
LPWSTR FILEDLG_DupToW(LPCSTR str, DWORD size)
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
 *                              FILEDLG_MapOfnStructA          [internal]
 *      map a 32 bits Ansi structure to an Unicode one
 */
void FILEDLG_MapOfnStructA(LPOPENFILENAMEA ofnA, LPOPENFILENAMEW ofnW, BOOL open)
{
    LPCSTR str;
    UNICODE_STRING usBuffer;

    ofnW->lStructSize = sizeof(OPENFILENAMEW);
    ofnW->hwndOwner = ofnA->hwndOwner;
    ofnW->hInstance = ofnA->hInstance;
    if (ofnA->lpstrFilter)
        ofnW->lpstrFilter = FILEDLG_MapStringPairsToW(ofnA->lpstrFilter, 0);

    if ((ofnA->lpstrCustomFilter) && (*(ofnA->lpstrCustomFilter)))
        ofnW->lpstrCustomFilter = FILEDLG_MapStringPairsToW(ofnA->lpstrCustomFilter, ofnA->nMaxCustFilter);
    ofnW->nMaxCustFilter = ofnA->nMaxCustFilter;
    ofnW->nFilterIndex = ofnA->nFilterIndex;
    ofnW->nMaxFile = ofnA->nMaxFile;
    ofnW->lpstrFile = FILEDLG_DupToW(ofnA->lpstrFile, ofnW->nMaxFile);
    ofnW->nMaxFileTitle = ofnA->nMaxFileTitle;
    ofnW->lpstrFileTitle = FILEDLG_DupToW(ofnA->lpstrFileTitle, ofnW->nMaxFileTitle);
    if (ofnA->lpstrInitialDir)
    {
        RtlCreateUnicodeStringFromAsciiz (&usBuffer,ofnA->lpstrInitialDir);
        ofnW->lpstrInitialDir = usBuffer.Buffer;
    }
    if (ofnA->lpstrTitle)
        str = ofnA->lpstrTitle;
    else
        /* Allocates default title (FIXME : get it from resource) */
        str = open ? "Open File" : "Save as";
    RtlCreateUnicodeStringFromAsciiz (&usBuffer,str);
    ofnW->lpstrTitle = usBuffer.Buffer;
    ofnW->Flags = ofnA->Flags;
    ofnW->nFileOffset = ofnA->nFileOffset;
    ofnW->nFileExtension = ofnA->nFileExtension;
    ofnW->lpstrDefExt = FILEDLG_DupToW(ofnA->lpstrDefExt, 3);
    if ((ofnA->Flags & OFN_ENABLETEMPLATE) && (ofnA->lpTemplateName))
    {
        if (HIWORD(ofnA->lpTemplateName))
        {
            RtlCreateUnicodeStringFromAsciiz (&usBuffer,ofnA->lpTemplateName);
            ofnW->lpTemplateName = usBuffer.Buffer;
        }
        else /* numbered resource */
            ofnW->lpTemplateName = (LPWSTR) ofnA->lpTemplateName;
    }
}

/************************************************************************
 *                              FILEDLG_MapOfnStruct16          [internal]
 *      map a 16 bits structure to an Unicode one
 */
void FILEDLG_MapOfnStruct16(LPOPENFILENAME16 ofn16, LPOPENFILENAMEW ofnW, BOOL open)
{
    OPENFILENAMEA ofnA;
    /* first convert to linear pointers */
    memset(&ofnA, 0, sizeof(OPENFILENAMEA));
    ofnA.lStructSize = sizeof(OPENFILENAMEA);
    ofnA.hwndOwner = HWND_32(ofn16->hwndOwner);
    ofnA.hInstance = HINSTANCE_32(ofn16->hInstance);
    if (ofn16->lpstrFilter)
        ofnA.lpstrFilter = MapSL(ofn16->lpstrFilter);
    if (ofn16->lpstrCustomFilter)
        ofnA.lpstrCustomFilter = MapSL(ofn16->lpstrCustomFilter);
    ofnA.nMaxCustFilter = ofn16->nMaxCustFilter;
    ofnA.nFilterIndex = ofn16->nFilterIndex;
    ofnA.lpstrFile = MapSL(ofn16->lpstrFile);
    ofnA.nMaxFile = ofn16->nMaxFile;
    ofnA.lpstrFileTitle = MapSL(ofn16->lpstrFileTitle);
    ofnA.nMaxFileTitle = ofn16->nMaxFileTitle;
    ofnA.lpstrInitialDir = MapSL(ofn16->lpstrInitialDir);
    ofnA.lpstrTitle = MapSL(ofn16->lpstrTitle);
    ofnA.Flags = ofn16->Flags;
    ofnA.nFileOffset = ofn16->nFileOffset;
    ofnA.nFileExtension = ofn16->nFileExtension;
    ofnA.lpstrDefExt = MapSL(ofn16->lpstrDefExt);
    if (HIWORD(ofn16->lpTemplateName))
        ofnA.lpTemplateName = MapSL(ofn16->lpTemplateName);
    else
        ofnA.lpTemplateName = (LPSTR) ofn16->lpTemplateName; /* ressource number */
    /* now calls the 32 bits Ansi to Unicode version to complete the job */
    FILEDLG_MapOfnStructA(&ofnA, ofnW, open);
}

/************************************************************************
 *                              FILEDLG_DestroyPrivate            [internal]
 *      destroys the private object
 */
static void FILEDLG_DestroyPrivate(LFSPRIVATE lfs)
{
    HWND hwnd;
    if (!lfs) return;
    hwnd = lfs->hwnd;
    /* free resources for a 16 bits dialog */
    if (lfs->hResource16) FreeResource16(lfs->hResource16);
    if (lfs->hGlobal16)
    {
        GlobalUnlock16(lfs->hGlobal16);
        GlobalFree16(lfs->hGlobal16);
    }
    /* if ofnW has been allocated, have to free everything in it */
    if (lfs->ofn16 || lfs->ofnA)
    {
       LPOPENFILENAMEW ofnW = lfs->ofnW;
       if (ofnW->lpstrFilter) HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrFilter);
       if (ofnW->lpstrCustomFilter) HeapFree(GetProcessHeap(), 0, ofnW->lpstrCustomFilter);
       if (ofnW->lpstrFile) HeapFree(GetProcessHeap(), 0, ofnW->lpstrFile);
       if (ofnW->lpstrFileTitle) HeapFree(GetProcessHeap(), 0, ofnW->lpstrFileTitle);
       if (ofnW->lpstrInitialDir) HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrInitialDir);
       if (ofnW->lpstrTitle) HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpstrTitle);
       if ((ofnW->lpTemplateName) && (HIWORD(ofnW->lpTemplateName)))
           HeapFree(GetProcessHeap(), 0, (LPWSTR) ofnW->lpTemplateName);
       HeapFree(GetProcessHeap(), 0, ofnW);
    }
    TRACE("destroying private allocation %p\n", lfs);
    HeapFree(GetProcessHeap(), 0, lfs);
    RemovePropA(hwnd, OFN_PROP);
}

/************************************************************************
 *                              FILEDLG_AllocPrivate            [internal]
 *      allocate a private object to hold 32 bits Unicode
 *      structure that will be used throughtout the calls, while
 *      keeping available the original structures and a few variables
 *      On entry : type = dialog procedure type (16,32A,32W)
 *                 dlgType = dialog type (open or save)
 */
static LFSPRIVATE FILEDLG_AllocPrivate(LPARAM lParam, int type, UINT dlgType)
{
    LFSPRIVATE lfs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct FSPRIVATE));
    LFSPRIVATE ret;
    TRACE("alloc private buf %p\n", lfs);
    if (!lfs) return NULL;
    lfs->hook = FALSE;
    lfs->lParam = lParam;
    if (dlgType == OPEN_DIALOG)
        lfs->open = TRUE;
    else
        lfs->open = FALSE;
    lfs->lbselchstring = RegisterWindowMessageA(LBSELCHSTRINGA);
    lfs->fileokstring = RegisterWindowMessageA(FILEOKSTRINGA);
    switch(type)
    {
        case LFS16:
        lfs->ofn16 = MapSL(lParam);
        if (lfs->ofn16->Flags & OFN_ENABLEHOOK)
            if (lfs->ofn16->lpfnHook)
                lfs->hook = TRUE;

        break;

        case LFS32A:
        lfs->ofnA = (LPOPENFILENAMEA) lParam;
        if (lfs->ofnA->Flags & OFN_ENABLEHOOK)
            if (lfs->ofnA->lpfnHook)
                lfs->hook = TRUE;
        break;

        case LFS32W:
        lfs->ofnW = (LPOPENFILENAMEW) lParam;
        if (lfs->ofnW->Flags & OFN_ENABLEHOOK)
            if (lfs->ofnW->lpfnHook)
                lfs->hook = TRUE;
        break;
    }
    ret = lfs;
    if (!lfs->ofnW)
    { /* this structure is needed internally, so create it */
        lfs->ofnW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OPENFILENAMEW));
        if (lfs->ofnW)
        {
            if (lfs->ofn16)
                FILEDLG_MapOfnStruct16(lfs->ofn16, lfs->ofnW, lfs->open);
            if (lfs->ofnA)
                FILEDLG_MapOfnStructA(lfs->ofnA, lfs->ofnW, lfs->open);
        }
        else
            ret = NULL;
    }
    if (lfs->ofn16)
    {
        if (!Get16BitsTemplate(lfs)) ret = NULL;
    }
    else
        if (!Get32BitsTemplate(lfs)) ret = NULL;
    if (!ret) FILEDLG_DestroyPrivate(lfs);
    return ret;
}

/***********************************************************************
 *                              FILEDLG_CallWindowProc16          [internal]
 *
 *      Call the appropriate hook
 */
static BOOL FILEDLG_CallWindowProc16(LFSPRIVATE lfs, UINT wMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    if (lfs->ofn16)
    {
        return (BOOL16) CallWindowProc16(
          (WNDPROC16)lfs->ofn16->lpfnHook, HWND_16(lfs->hwnd),
          (UINT16)wMsg, (WPARAM16)wParam, lParam);
    }
    return FALSE;
}

/***********************************************************************
 *                              FILEDLG_WMInitDialog            [internal]
 */

static LONG FILEDLG_WMInitDialog(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i, n;
  WCHAR tmpstr[BUFFILE];
  LPWSTR pstr, old_pstr;
  LPOPENFILENAMEW ofn;
  LFSPRIVATE lfs = (LFSPRIVATE) lParam;

  if (!lfs) return FALSE;
  SetPropA(hWnd, OFN_PROP, (HANDLE)lfs);
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
  lstrcpynW(tmpstr, FILEDLG_GetFileType(ofn->lpstrCustomFilter,
	     (LPWSTR)ofn->lpstrFilter, ofn->nFilterIndex - 1),BUFFILE);
  TRACE("nFilterIndex = %ld, SetText of edt1 to %s\n",
  			ofn->nFilterIndex, debugstr_w(tmpstr));
  SetDlgItemTextW( hWnd, edt1, tmpstr );
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
  if (!FILEDLG_ScanDir(hWnd, tmpstr)) {
    *tmpstr = 0;
    if (!FILEDLG_ScanDir(hWnd, tmpstr))
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
      return (BOOL) FILEDLG_CallWindowProc(lfs, WM_INITDIALOG, wParam, lfs->lParam);
  return TRUE;
}




/***********************************************************************
 *                              FILEDLG_WMInitDialog16            [internal]
 *      The is a duplicate of the 32bit FILEDLG_WMInitDialog function 
 *      The only differnce is that it calls FILEDLG_CallWindowProc16 
 *      for a 16 bit Window Proc.
 */

static LONG FILEDLG_WMInitDialog16(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i, n;
  WCHAR tmpstr[BUFFILE];
  LPWSTR pstr, old_pstr;
  LPOPENFILENAMEW ofn;
  LFSPRIVATE lfs = (LFSPRIVATE) lParam;

  if (!lfs) return FALSE;
  SetPropA(hWnd, OFN_PROP, (HANDLE)lfs);
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
  lstrcpynW(tmpstr, FILEDLG_GetFileType(ofn->lpstrCustomFilter,
	     (LPWSTR)ofn->lpstrFilter, ofn->nFilterIndex - 1),BUFFILE);
  TRACE("nFilterIndex = %ld, SetText of edt1 to %s\n",
  			ofn->nFilterIndex, debugstr_w(tmpstr));
  SetDlgItemTextW( hWnd, edt1, tmpstr );
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
  if (!FILEDLG_ScanDir(hWnd, tmpstr)) {
    *tmpstr = 0;
    if (!FILEDLG_ScanDir(hWnd, tmpstr))
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
      return (BOOL) FILEDLG_CallWindowProc16(lfs, WM_INITDIALOG, wParam, lfs->lParam);
  return TRUE;
}

/***********************************************************************
 *                              FILEDLG_WMMeasureItem           [internal]
 */
static LONG FILEDLG_WMMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmeasure;

    lpmeasure = (LPMEASUREITEMSTRUCT)lParam;
    lpmeasure->itemHeight = fldrHeight;
    return TRUE;
}


/***********************************************************************
 *                              FILEDLG_WMMeasureItem16         [internal]
 */
static LONG FILEDLG_WMMeasureItem16(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT16 lpmeasure;

    lpmeasure = MapSL(lParam);
    lpmeasure->itemHeight = fldrHeight;
    return TRUE;
}

/* ------------------ Dialog procedures ---------------------- */

/***********************************************************************
 *           FileOpenDlgProc                                    [internal]
 *      Used for open and save, in fact.
 */
static INT_PTR CALLBACK FileOpenDlgProc(HWND hWnd, UINT wMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);

    TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
        {
            INT_PTR lRet;
            lRet  = (INT_PTR)FILEDLG_CallWindowProc(lfs, wMsg, wParam, lParam);
            if (lRet)
                return lRet;         /* else continue message processing */
        }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FILEDLG_WMInitDialog(hWnd, wParam, lParam);

    case WM_MEASUREITEM:
        return FILEDLG_WMMeasureItem(hWnd, wParam, lParam);

    case WM_DRAWITEM:
        return FILEDLG_WMDrawItem(hWnd, wParam, lParam, !lfs->open, (DRAWITEMSTRUCT *)lParam);

    case WM_COMMAND:
        return FILEDLG_WMCommand(hWnd, lParam, HIWORD(wParam), LOWORD(wParam), lfs);
#if 0
    case WM_CTLCOLOR:
         SetBkColor((HDC16)wParam, 0x00C0C0C0);
         switch (HIWORD(lParam))
         {
	 case CTLCOLOR_BTN:
	     SetTextColor((HDC16)wParam, 0x00000000);
             return hGRAYBrush;
	case CTLCOLOR_STATIC:
             SetTextColor((HDC16)wParam, 0x00000000);
             return hGRAYBrush;
	}
      break;
#endif
    }
    return FALSE;
}

/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{
    HWND hWnd = HWND_32(hWnd16);
    LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);
    DRAWITEMSTRUCT dis;

    TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
        {
            LRESULT lRet = (BOOL16)FILEDLG_CallWindowProc16(lfs, wMsg, wParam, lParam);
            if (lRet)
                return lRet;         /* else continue message processing */
        }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FILEDLG_WMInitDialog16(hWnd, wParam, lParam);

    case WM_MEASUREITEM:
        return FILEDLG_WMMeasureItem16(hWnd16, wParam, lParam);

    case WM_DRAWITEM:
        FILEDLG_MapDrawItemStruct(MapSL(lParam), &dis);
        return FILEDLG_WMDrawItem(hWnd, wParam, lParam, FALSE, &dis);

    case WM_COMMAND:
        return FILEDLG_WMCommand(hWnd, lParam, HIWORD(lParam),wParam, lfs);
#if 0
    case WM_CTLCOLOR:
         SetBkColor((HDC16)wParam, 0x00C0C0C0);
         switch (HIWORD(lParam))
         {
	 case CTLCOLOR_BTN:
	     SetTextColor((HDC16)wParam, 0x00000000);
             return hGRAYBrush;
	case CTLCOLOR_STATIC:
             SetTextColor((HDC16)wParam, 0x00000000);
             return hGRAYBrush;
	}
      break;
#endif
    }
    return FALSE;
}

/***********************************************************************
 *           FileSaveDlgProc   (COMMDLG.7)
 */
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{
 HWND hWnd = HWND_32(hWnd16);
 LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);
 DRAWITEMSTRUCT dis;

 TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
 if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
  {
   LRESULT  lRet;
   lRet = (BOOL16)FILEDLG_CallWindowProc16(lfs, wMsg, wParam, lParam);
   if (lRet)
    return lRet;         /* else continue message processing */
  }
  switch (wMsg) {
   case WM_INITDIALOG:
      return FILEDLG_WMInitDialog16(hWnd, wParam, lParam);

   case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem16(hWnd16, wParam, lParam);

   case WM_DRAWITEM:
      FILEDLG_MapDrawItemStruct(MapSL(lParam), &dis);
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam, TRUE, &dis);

   case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, lParam, HIWORD(lParam), wParam, lfs);
  }

  /*
  case WM_CTLCOLOR:
   SetBkColor((HDC16)wParam, 0x00C0C0C0);
   switch (HIWORD(lParam))
   {
    case CTLCOLOR_BTN:
     SetTextColor((HDC16)wParam, 0x00000000);
     return hGRAYBrush;
    case CTLCOLOR_STATIC:
     SetTextColor((HDC16)wParam, 0x00000000);
     return hGRAYBrush;
   }
   return FALSE;

   */
  return FALSE;
}



/***********************************************************************
 *           GetFileName31A                                 [internal]
 *
 * Creates a win31 style dialog box for the user to select a file to open/save.
 */
BOOL GetFileName31A(
                    LPOPENFILENAMEA lpofn, /* addess of structure with data*/
                    UINT dlgType /* type dialogue : open/save */
                    )
{
    HINSTANCE hInst;
    BOOL bRet = FALSE;
    LFSPRIVATE lfs;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    TRACE("ofn flags %08lx\n", lpofn->Flags);
    lfs = FILEDLG_AllocPrivate((LPARAM) lpofn, LFS32A, dlgType);
    if (lfs)
    {
        hInst = (HINSTANCE)GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );
        bRet = DialogBoxIndirectParamA( hInst, lfs->template, lpofn->hwndOwner,
                                        FileOpenDlgProc, (LPARAM)lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", lpofn->lpstrFile);
    return bRet;
}

/***********************************************************************
 *           GetFileName31W                                 [internal]
 *
 * Creates a win31 style dialog box for the user to select a file to open/save
 */
BOOL GetFileName31W(
                    LPOPENFILENAMEW lpofn, /* addess of structure with data*/
                    UINT dlgType /* type dialogue : open/save */
                    )
{
    HINSTANCE hInst;
    BOOL bRet = FALSE;
    LFSPRIVATE lfs;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) lpofn, LFS32W, dlgType);
    if (lfs)
    {
        hInst = (HINSTANCE)GetWindowLongA( lpofn->hwndOwner, GWL_HINSTANCE );
        bRet = DialogBoxIndirectParamW( hInst, lfs->template, lpofn->hwndOwner,
                                        FileOpenDlgProc, (LPARAM)lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile=%s !\n", debugstr_w(lpofn->lpstrFile));
    return bRet;
}







/* ------------------ APIs ---------------------- */

/***********************************************************************
 *           GetOpenFileName   (COMMDLG.1)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user selected a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, there are some FIXME's left.
 */
BOOL16 WINAPI GetOpenFileName16(
				SEGPTR ofn /* [in/out] address of structure with data*/
				)
{
    HINSTANCE16 hInst;
    BOOL bRet = FALSE;
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    LFSPRIVATE lfs;
    FARPROC16 ptr;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) ofn, LFS16, OPEN_DIALOG);
    if (lfs)
    {
        hInst = GetWindowWord( HWND_32(lpofn->hwndOwner), GWL_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 6);
        bRet = DialogBoxIndirectParam16( hInst, lfs->hDlgTmpl16, lpofn->hwndOwner,
                                         (DLGPROC16) ptr, (LPARAM) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", (char *)MapSL(lpofn->lpstrFile));
    return bRet;
}

/***********************************************************************
 *           GetSaveFileName   (COMMDLG.2)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown. There are some FIXME's left.
 */
BOOL16 WINAPI GetSaveFileName16(
				SEGPTR ofn /* [in/out] addess of structure with data*/
				)
{
    HINSTANCE16 hInst;
    BOOL bRet = FALSE;
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    LFSPRIVATE lfs;
    FARPROC16 ptr;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) ofn, LFS16, SAVE_DIALOG);
    if (lfs)
    {
        hInst = GetWindowWord( HWND_32(lpofn->hwndOwner), GWL_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 7);
        bRet = DialogBoxIndirectParam16( hInst, lfs->hDlgTmpl16, lpofn->hwndOwner,
                                         (DLGPROC16) ptr, (LPARAM) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", (char *)MapSL(lpofn->lpstrFile));
    return bRet;
}
