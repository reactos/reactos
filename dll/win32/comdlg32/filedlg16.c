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
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "wine/debug.h"
#include "cderr.h"
#include "commdlg.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "cdlg16.h"
#include "filedlg31.h"

typedef struct tagFD16_PRIVATE
{
    HANDLE16 hDlgTmpl16; /* handle for resource 16 */
    HANDLE16 hResource16; /* handle for allocated resource 16 */
    HANDLE16 hGlobal16; /* 16 bits mem block (resources) */
    OPENFILENAME16 *ofn16; /* original structure if 16 bits dialog */
} FD16_PRIVATE, *PFD16_PRIVATE;

/************************************************************************
 *                              FD16_MapOfnStruct16          [internal]
 *      map a 16 bits structure to an Unicode one
 */
static void FD16_MapOfnStruct16(LPOPENFILENAME16 ofn16, LPOPENFILENAMEW ofnW, BOOL open)
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
    FD31_MapOfnStructA(&ofnA, ofnW, open);
}

/***********************************************************************
 *           FD16_GetTemplate                                [internal]
 *
 * Get a template (FALSE if failure) when 16 bits dialogs are used
 * by a 16 bits application
 *
 */
static BOOL FD16_GetTemplate(PFD31_DATA lfs)
{
    PFD16_PRIVATE priv = (PFD16_PRIVATE) lfs->private1632;
    LPOPENFILENAME16 ofn16 = priv->ofn16;
    LPCVOID template;
    HGLOBAL16 hGlobal16 = 0;

    if (ofn16->Flags & OFN_ENABLETEMPLATEHANDLE)
        priv->hDlgTmpl16 = ofn16->hInstance;
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
	if (!(priv->hDlgTmpl16 = LoadResource16( ofn16->hInstance, hResInfo )))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        priv->hResource16 = priv->hDlgTmpl16;
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
        priv->hDlgTmpl16 = hGlobal16;
        priv->hGlobal16 = hGlobal16;
    }
    return TRUE;
}

/************************************************************************
 *                              FD16_Init          [internal]
 *      called from the common 16/32 code to initialize 16 bit data
 */
static BOOL CALLBACK FD16_Init(LPARAM lParam, PFD31_DATA lfs, DWORD data)
{
    PFD16_PRIVATE priv;

    priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FD16_PRIVATE));
    lfs->private1632 = priv;
    if (NULL == lfs->private1632) return FALSE;

    priv->ofn16 = MapSL(lParam);
    if (priv->ofn16->Flags & OFN_ENABLEHOOK)
        if (priv->ofn16->lpfnHook)
            lfs->hook = TRUE;

    lfs->ofnW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*lfs->ofnW));
    FD16_MapOfnStruct16(priv->ofn16, lfs->ofnW, lfs->open);

    if (! FD16_GetTemplate(lfs)) return FALSE;

    return TRUE;
}

/***********************************************************************
 *                              FD16_CallWindowProc          [internal]
 *
 *      called from the common 16/32 code to call the appropriate hook
 */
static BOOL CALLBACK FD16_CallWindowProc(PFD31_DATA lfs, UINT wMsg, WPARAM wParam,
                                 LPARAM lParam)
{
    PFD16_PRIVATE priv = (PFD16_PRIVATE) lfs->private1632;

    if (priv->ofn16)
    {
        return (BOOL16) CallWindowProc16(
          (WNDPROC16)priv->ofn16->lpfnHook, HWND_16(lfs->hwnd),
          (UINT16)wMsg, (WPARAM16)wParam, lParam);
    }
    return FALSE;
}


/***********************************************************************
 *                              FD31_UpdateResult            [internal]
 *          update the real client structures
 */
static void CALLBACK FD16_UpdateResult(PFD31_DATA lfs)
{
    PFD16_PRIVATE priv = (PFD16_PRIVATE) lfs->private1632;
    LPOPENFILENAMEW ofnW = lfs->ofnW;

    if (priv->ofn16)
    { /* we have to convert to short (8.3) path */
	char tmp[1024]; /* MAX_PATHNAME_LEN */
	LPOPENFILENAME16 ofn16 = priv->ofn16;
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
}


/***********************************************************************
 *                              FD16_UpdateFileTitle         [internal]
 *          update the real client structures
 */
static void CALLBACK FD16_UpdateFileTitle(PFD31_DATA lfs)
{
    PFD16_PRIVATE priv = (PFD16_PRIVATE) lfs->private1632;
    LPOPENFILENAMEW ofnW = lfs->ofnW;

    if (priv->ofn16)
    {
        char *dest = MapSL(priv->ofn16->lpstrFileTitle);
        if (!WideCharToMultiByte( CP_ACP, 0, ofnW->lpstrFileTitle, -1,
                                  dest, ofnW->nMaxFileTitle, NULL, NULL ))
            dest[ofnW->nMaxFileTitle-1] = 0;
    }
}


/***********************************************************************
 *                              FD16_SendLbGetCurSel         [internal]
 *          retrieve selected listbox item
 */
static LRESULT CALLBACK FD16_SendLbGetCurSel(PFD31_DATA lfs)
{
    return SendDlgItemMessageW(lfs->hwnd, lst1, LB_GETCURSEL16, 0, 0);
}


/************************************************************************
 *                              FD16_Destroy          [internal]
 *      called from the common 16/32 code to cleanup 32 bit data
 */
static void CALLBACK FD16_Destroy(PFD31_DATA lfs)
{
    PFD16_PRIVATE priv = (PFD16_PRIVATE) lfs->private1632;

    /* free resources for a 16 bits dialog */
    if (NULL != priv)
    {
        if (priv->hResource16) FreeResource16(priv->hResource16);
        if (priv->hGlobal16)
        {
            GlobalUnlock16(priv->hGlobal16);
            GlobalFree16(priv->hGlobal16);
        }
        FD31_FreeOfnW(lfs->ofnW);
        HeapFree(GetProcessHeap(), 0, lfs->ofnW);
    }
}

static void FD16_SetupCallbacks(PFD31_CALLBACKS callbacks)
{
    callbacks->Init = FD16_Init;
    callbacks->CWP = FD16_CallWindowProc;
    callbacks->UpdateResult = FD16_UpdateResult;
    callbacks->UpdateFileTitle = FD16_UpdateFileTitle;
    callbacks->SendLbGetCurSel = FD16_SendLbGetCurSel;
    callbacks->Destroy = FD16_Destroy;
}

/***********************************************************************
 *                              FD16_MapDrawItemStruct       [internal]
 *      map a 16 bits drawitem struct to 32
 */
static void FD16_MapDrawItemStruct(LPDRAWITEMSTRUCT16 lpdis16, LPDRAWITEMSTRUCT lpdis)
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


/***********************************************************************
 *                              FD16_WMMeasureItem16         [internal]
 */
static LONG FD16_WMMeasureItem(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT16 lpmeasure;

    lpmeasure = MapSL(lParam);
    lpmeasure->itemHeight = FD31_GetFldrHeight();
    return TRUE;
}

/* ------------------ Dialog procedures ---------------------- */

/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    HWND hWnd = HWND_32(hWnd16);
    PFD31_DATA lfs = (PFD31_DATA)GetPropA(hWnd,FD31_OFN_PROP);
    DRAWITEMSTRUCT dis;

    TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
        {
            LRESULT lRet = (BOOL16)FD31_CallWindowProc(lfs, wMsg, wParam, lParam);
            if (lRet)
                return lRet;         /* else continue message processing */
        }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FD31_WMInitDialog(hWnd, wParam, lParam);

    case WM_MEASUREITEM:
        return FD16_WMMeasureItem(hWnd16, wParam, lParam);

    case WM_DRAWITEM:
        FD16_MapDrawItemStruct(MapSL(lParam), &dis);
        return FD31_WMDrawItem(hWnd, wParam, lParam, FALSE, &dis);

    case WM_COMMAND:
        return FD31_WMCommand(hWnd, lParam, HIWORD(lParam),wParam, lfs);
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
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
 HWND hWnd = HWND_32(hWnd16);
 PFD31_DATA lfs = (PFD31_DATA)GetPropA(hWnd,FD31_OFN_PROP);
 DRAWITEMSTRUCT dis;

 TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
 if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
  {
   LRESULT  lRet;
   lRet = (BOOL16)FD31_CallWindowProc(lfs, wMsg, wParam, lParam);
   if (lRet)
    return lRet;         /* else continue message processing */
  }
  switch (wMsg) {
   case WM_INITDIALOG:
      return FD31_WMInitDialog(hWnd, wParam, lParam);

   case WM_MEASUREITEM:
      return FD16_WMMeasureItem(hWnd16, wParam, lParam);

   case WM_DRAWITEM:
      FD16_MapDrawItemStruct(MapSL(lParam), &dis);
      return FD31_WMDrawItem(hWnd, wParam, lParam, TRUE, &dis);

   case WM_COMMAND:
      return FD31_WMCommand(hWnd, lParam, HIWORD(lParam), wParam, lfs);
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
    PFD31_DATA lfs;
    FARPROC16 ptr;
    FD31_CALLBACKS callbacks;
    PFD16_PRIVATE priv;

    if (!lpofn || !FD31_Init()) return FALSE;

    FD16_SetupCallbacks(&callbacks);
    lfs = FD31_AllocPrivate((LPARAM) ofn, OPEN_DIALOG, &callbacks, 0);
    if (lfs)
    {
        priv = (PFD16_PRIVATE) lfs->private1632;
        hInst = GetWindowLongPtrA( HWND_32(lpofn->hwndOwner), GWLP_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 6);
        bRet = DialogBoxIndirectParam16( hInst, priv->hDlgTmpl16, lpofn->hwndOwner,
                                         (DLGPROC16) ptr, (LPARAM) lfs);
        FD31_DestroyPrivate(lfs);
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
    PFD31_DATA lfs;
    FARPROC16 ptr;
    FD31_CALLBACKS callbacks;
    PFD16_PRIVATE priv;

    if (!lpofn || !FD31_Init()) return FALSE;

    FD16_SetupCallbacks(&callbacks);
    lfs = FD31_AllocPrivate((LPARAM) ofn, SAVE_DIALOG, &callbacks, 0);
    if (lfs)
    {
        priv = (PFD16_PRIVATE) lfs->private1632;
        hInst = GetWindowLongPtrA( HWND_32(lpofn->hwndOwner), GWLP_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 7);
        bRet = DialogBoxIndirectParam16( hInst, priv->hDlgTmpl16, lpofn->hwndOwner,
                                         (DLGPROC16) ptr, (LPARAM) lfs);
        FD31_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", (char *)MapSL(lpofn->lpstrFile));
    return bRet;
}
