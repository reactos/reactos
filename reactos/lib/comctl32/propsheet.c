/*
 * Property Sheets
 *
 * Copyright 1998 Francis Beaudet
 * Copyright 1999 Thuy Nguyen
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
 *
 * TODO:
 *   - Tab order
 *   - Unicode property sheets
 */

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "prsht.h"
#include "comctl32.h"

#include "wine/debug.h"
#include "wine/unicode.h"

/******************************************************************************
 * Data structures
 */
#include "pshpack2.h"

typedef struct
{
  WORD dlgVer;
  WORD signature;
  DWORD helpID;
  DWORD exStyle;
  DWORD style;
} MyDLGTEMPLATEEX;

typedef struct
{
  DWORD helpid;
  DWORD exStyle;
  DWORD style;
  short x;
  short y;
  short cx;
  short cy;
  DWORD id;
} MyDLGITEMTEMPLATEEX;
#include "poppack.h"

typedef struct tagPropPageInfo
{
  HPROPSHEETPAGE hpage; /* to keep track of pages not passed to PropertySheet */
  HWND hwndPage;
  BOOL isDirty;
  LPCWSTR pszText;
  BOOL hasHelp;
  BOOL useCallback;
  BOOL hasIcon;
} PropPageInfo;

typedef struct tagPropSheetInfo
{
  HWND hwnd;
  PROPSHEETHEADERW ppshheader;
  BOOL unicode;
  LPWSTR strPropertiesFor;
  int nPages;
  int active_page;
  BOOL isModeless;
  BOOL hasHelp;
  BOOL hasApply;
  BOOL useCallback;
  BOOL restartWindows;
  BOOL rebootSystem;
  BOOL activeValid;
  PropPageInfo* proppage;
  int x;
  int y;
  int width;
  int height;
  HIMAGELIST hImageList;
} PropSheetInfo;

typedef struct
{
  int x;
  int y;
} PADDING_INFO;

/******************************************************************************
 * Defines and global variables
 */

const WCHAR PropSheetInfoStr[] =
    {'P','r','o','p','e','r','t','y','S','h','e','e','t','I','n','f','o',0 };

#define PSP_INTERNAL_UNICODE 0x80000000

#define MAX_CAPTION_LENGTH 255
#define MAX_TABTEXT_LENGTH 255
#define MAX_BUTTONTEXT_LENGTH 64

#define INTRNL_ANY_WIZARD (PSH_WIZARD | PSH_WIZARD97_OLD | PSH_WIZARD97_NEW | PSH_WIZARD_LITE)

/******************************************************************************
 * Prototypes
 */
static int PROPSHEET_CreateDialog(PropSheetInfo* psInfo);
static BOOL PROPSHEET_SizeMismatch(HWND hwndDlg, PropSheetInfo* psInfo);
static BOOL PROPSHEET_AdjustSize(HWND hwndDlg, PropSheetInfo* psInfo);
static BOOL PROPSHEET_AdjustButtons(HWND hwndParent, PropSheetInfo* psInfo);
static BOOL PROPSHEET_CollectSheetInfoA(LPCPROPSHEETHEADERA lppsh,
                                       PropSheetInfo * psInfo);
static BOOL PROPSHEET_CollectSheetInfoW(LPCPROPSHEETHEADERW lppsh,
                                       PropSheetInfo * psInfo);
static BOOL PROPSHEET_CollectPageInfo(LPCPROPSHEETPAGEW lppsp,
                                      PropSheetInfo * psInfo,
                                      int index);
static BOOL PROPSHEET_CreateTabControl(HWND hwndParent,
                                       PropSheetInfo * psInfo);
static BOOL PROPSHEET_CreatePage(HWND hwndParent, int index,
                                const PropSheetInfo * psInfo,
                                LPCPROPSHEETPAGEW ppshpage);
static BOOL PROPSHEET_ShowPage(HWND hwndDlg, int index, PropSheetInfo * psInfo);
static PADDING_INFO PROPSHEET_GetPaddingInfo(HWND hwndDlg);
static BOOL PROPSHEET_Back(HWND hwndDlg);
static BOOL PROPSHEET_Next(HWND hwndDlg);
static BOOL PROPSHEET_Finish(HWND hwndDlg);
static BOOL PROPSHEET_Apply(HWND hwndDlg, LPARAM lParam);
static void PROPSHEET_Cancel(HWND hwndDlg, LPARAM lParam);
static void PROPSHEET_Help(HWND hwndDlg);
static void PROPSHEET_Changed(HWND hwndDlg, HWND hwndDirtyPage);
static void PROPSHEET_UnChanged(HWND hwndDlg, HWND hwndCleanPage);
static void PROPSHEET_PressButton(HWND hwndDlg, int buttonID);
static void PROPSHEET_SetFinishTextA(HWND hwndDlg, LPCSTR lpszText);
static void PROPSHEET_SetFinishTextW(HWND hwndDlg, LPCWSTR lpszText);
static void PROPSHEET_SetTitleA(HWND hwndDlg, DWORD dwStyle, LPCSTR lpszText);
static void PROPSHEET_SetTitleW(HWND hwndDlg, DWORD dwStyle, LPCWSTR lpszText);
static BOOL PROPSHEET_CanSetCurSel(HWND hwndDlg);
static BOOL PROPSHEET_SetCurSel(HWND hwndDlg,
                                int index,
                                int skipdir,
                                HPROPSHEETPAGE hpage);
static void PROPSHEET_SetCurSelId(HWND hwndDlg, int id);
static LRESULT PROPSHEET_QuerySiblings(HWND hwndDlg,
                                       WPARAM wParam, LPARAM lParam);
static BOOL PROPSHEET_AddPage(HWND hwndDlg,
                              HPROPSHEETPAGE hpage);

static BOOL PROPSHEET_RemovePage(HWND hwndDlg,
                                 int index,
                                 HPROPSHEETPAGE hpage);
static void PROPSHEET_CleanUp();
static int PROPSHEET_GetPageIndex(HPROPSHEETPAGE hpage, PropSheetInfo* psInfo);
static void PROPSHEET_SetWizButtons(HWND hwndDlg, DWORD dwFlags);
static PADDING_INFO PROPSHEET_GetPaddingInfoWizard(HWND hwndDlg, const PropSheetInfo* psInfo);
static BOOL PROPSHEET_IsDialogMessage(HWND hwnd, LPMSG lpMsg);
static BOOL PROPSHEET_DoCommand(HWND hwnd, WORD wID);

INT_PTR CALLBACK
PROPSHEET_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

WINE_DEFAULT_DEBUG_CHANNEL(propsheet);

#define add_flag(a) if (dwFlags & a) {strcat(string, #a );strcat(string," ");}
/******************************************************************************
 *            PROPSHEET_UnImplementedFlags
 *
 * Document use of flags we don't implement yet.
 */
static VOID PROPSHEET_UnImplementedFlags(DWORD dwFlags)
{
    CHAR string[256];

    string[0] = '\0';

  /*
   * unhandled header flags:
   *  PSH_DEFAULT            0x00000000
   *  PSH_WIZARDHASFINISH    0x00000010
   *  PSH_RTLREADING         0x00000800
   *  PSH_WIZARDCONTEXTHELP  0x00001000
   *  PSH_WIZARD97           0x00002000  (pre IE 5)
   *  PSH_WATERMARK          0x00008000
   *  PSH_USEHBMWATERMARK    0x00010000
   *  PSH_USEHPLWATERMARK    0x00020000
   *  PSH_STRETCHWATERMARK   0x00040000
   *  PSH_HEADER             0x00080000
   *  PSH_USEHBMHEADER       0x00100000
   *  PSH_USEPAGELANG        0x00200000
   *  PSH_WIZARD_LITE        0x00400000      also not in .h
   *  PSH_WIZARD97           0x01000000  (IE 5 and above)
   *  PSH_NOCONTEXTHELP      0x02000000      also not in .h
   */

    add_flag(PSH_WIZARDHASFINISH);
    add_flag(PSH_RTLREADING);
    add_flag(PSH_WIZARDCONTEXTHELP);
    add_flag(PSH_WIZARD97_OLD);
    add_flag(PSH_WATERMARK);
    add_flag(PSH_USEHBMWATERMARK);
    add_flag(PSH_USEHPLWATERMARK);
    add_flag(PSH_STRETCHWATERMARK);
    add_flag(PSH_HEADER);
    add_flag(PSH_USEHBMHEADER);
    add_flag(PSH_USEPAGELANG);
    add_flag(PSH_WIZARD_LITE);
    add_flag(PSH_WIZARD97_NEW);
    add_flag(PSH_NOCONTEXTHELP);
    if (string[0] != '\0')
	FIXME("%s\n", string);
}
#undef add_flag

/******************************************************************************
 *            PROPSHEET_GetPageRect
 *
 * Retrieve rect from tab control and map into the dialog for SetWindowPos
 */
static void PROPSHEET_GetPageRect(const PropSheetInfo * psInfo, HWND hwndDlg, RECT *rc)
{
    HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);

    GetClientRect(hwndTabCtrl, rc);
    SendMessageW(hwndTabCtrl, TCM_ADJUSTRECT, FALSE, (LPARAM)rc);
    MapWindowPoints(hwndTabCtrl, hwndDlg, (LPPOINT)rc, 2);
}

/******************************************************************************
 *            PROPSHEET_FindPageByResId
 *
 * Find page index corresponding to page resource id.
 */
INT PROPSHEET_FindPageByResId(PropSheetInfo * psInfo, LRESULT resId)
{
   INT i;

   for (i = 0; i < psInfo->nPages; i++)
   {
      LPCPROPSHEETPAGEA lppsp = (LPCPROPSHEETPAGEA)psInfo->proppage[i].hpage;

      /* Fixme: if resource ID is a string shall we use strcmp ??? */
      if (lppsp->u.pszTemplate == (LPVOID)resId)
         break;
   }

   return i;
}

/******************************************************************************
 *            PROPSHEET_AtoW
 *
 * Convert ASCII to Unicode since all data is saved as Unicode.
 */
static void PROPSHEET_AtoW(LPCWSTR *tostr, LPCSTR frstr)
{
    INT len;

    TRACE("<%s>\n", frstr);
    len = MultiByteToWideChar(CP_ACP, 0, frstr, -1, 0, 0);
    *tostr = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, frstr, -1, (LPWSTR)*tostr, len);
}

/******************************************************************************
 *            PROPSHEET_CollectSheetInfoA
 *
 * Collect relevant data.
 */
static BOOL PROPSHEET_CollectSheetInfoA(LPCPROPSHEETHEADERA lppsh,
                                       PropSheetInfo * psInfo)
{
  DWORD dwSize = min(lppsh->dwSize,sizeof(PROPSHEETHEADERA));
  DWORD dwFlags = lppsh->dwFlags;

  psInfo->hasHelp = dwFlags & PSH_HASHELP;
  psInfo->hasApply = !(dwFlags & PSH_NOAPPLYNOW);
  psInfo->useCallback = (dwFlags & PSH_USECALLBACK )&& (lppsh->pfnCallback);
  psInfo->isModeless = dwFlags & PSH_MODELESS;

  memcpy(&psInfo->ppshheader,lppsh,dwSize);
  TRACE("\n** PROPSHEETHEADER **\ndwSize\t\t%ld\ndwFlags\t\t%08lx\nhwndParent\t%p\nhInstance\t%p\npszCaption\t'%s'\nnPages\t\t%d\npfnCallback\t%p\n",
	lppsh->dwSize, lppsh->dwFlags, lppsh->hwndParent, lppsh->hInstance,
	debugstr_a(lppsh->pszCaption), lppsh->nPages, lppsh->pfnCallback);

  PROPSHEET_UnImplementedFlags(lppsh->dwFlags);

  if (lppsh->dwFlags & INTRNL_ANY_WIZARD)
     psInfo->ppshheader.pszCaption = NULL;
  else
  {
     if (HIWORD(lppsh->pszCaption))
     {
        int len = strlen(lppsh->pszCaption);
        psInfo->ppshheader.pszCaption = HeapAlloc( GetProcessHeap(), 0, (len+1)*sizeof (WCHAR) );
        MultiByteToWideChar(CP_ACP, 0, lppsh->pszCaption, -1, (LPWSTR) psInfo->ppshheader.pszCaption, len+1);
        /* strcpy( (char *)psInfo->ppshheader.pszCaption, lppsh->pszCaption ); */
     }
  }
  psInfo->nPages = lppsh->nPages;

  if (dwFlags & PSH_USEPSTARTPAGE)
  {
    TRACE("PSH_USEPSTARTPAGE is on\n");
    psInfo->active_page = 0;
  }
  else
    psInfo->active_page = lppsh->u2.nStartPage;

  if (psInfo->active_page < 0 || psInfo->active_page >= psInfo->nPages)
     psInfo->active_page = 0;

  psInfo->restartWindows = FALSE;
  psInfo->rebootSystem = FALSE;
  psInfo->hImageList = 0;
  psInfo->activeValid = FALSE;

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_CollectSheetInfoW
 *
 * Collect relevant data.
 */
static BOOL PROPSHEET_CollectSheetInfoW(LPCPROPSHEETHEADERW lppsh,
                                       PropSheetInfo * psInfo)
{
  DWORD dwSize = min(lppsh->dwSize,sizeof(PROPSHEETHEADERW));
  DWORD dwFlags = lppsh->dwFlags;

  psInfo->hasHelp = dwFlags & PSH_HASHELP;
  psInfo->hasApply = !(dwFlags & PSH_NOAPPLYNOW);
  psInfo->useCallback = (dwFlags & PSH_USECALLBACK) && (lppsh->pfnCallback);
  psInfo->isModeless = dwFlags & PSH_MODELESS;

  memcpy(&psInfo->ppshheader,lppsh,dwSize);
  TRACE("\n** PROPSHEETHEADER **\ndwSize\t\t%ld\ndwFlags\t\t%08lx\nhwndParent\t%p\nhInstance\t%p\npszCaption\t'%s'\nnPages\t\t%d\npfnCallback\t%p\n",
      lppsh->dwSize, lppsh->dwFlags, lppsh->hwndParent, lppsh->hInstance, debugstr_w(lppsh->pszCaption), lppsh->nPages, lppsh->pfnCallback);

  PROPSHEET_UnImplementedFlags(lppsh->dwFlags);

  if (lppsh->dwFlags & INTRNL_ANY_WIZARD)
     psInfo->ppshheader.pszCaption = NULL;
  else
  {
     if (!(lppsh->dwFlags & INTRNL_ANY_WIZARD) && HIWORD(lppsh->pszCaption))
     {
        int len = strlenW(lppsh->pszCaption);
        psInfo->ppshheader.pszCaption = HeapAlloc( GetProcessHeap(), 0, (len+1)*sizeof(WCHAR) );
        strcpyW( (WCHAR *)psInfo->ppshheader.pszCaption, lppsh->pszCaption );
     }
  }
  psInfo->nPages = lppsh->nPages;

  if (dwFlags & PSH_USEPSTARTPAGE)
  {
    TRACE("PSH_USEPSTARTPAGE is on\n");
    psInfo->active_page = 0;
  }
  else
    psInfo->active_page = lppsh->u2.nStartPage;

  if (psInfo->active_page < 0 || psInfo->active_page >= psInfo->nPages)
     psInfo->active_page = 0;

  psInfo->restartWindows = FALSE;
  psInfo->rebootSystem = FALSE;
  psInfo->hImageList = 0;
  psInfo->activeValid = FALSE;

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_CollectPageInfo
 *
 * Collect property sheet data.
 * With code taken from DIALOG_ParseTemplate32.
 */
BOOL PROPSHEET_CollectPageInfo(LPCPROPSHEETPAGEW lppsp,
                               PropSheetInfo * psInfo,
                               int index)
{
  DLGTEMPLATE* pTemplate;
  const WORD*  p;
  DWORD dwFlags;
  int width, height;

  TRACE("\n");
  psInfo->proppage[index].hpage = (HPROPSHEETPAGE)lppsp;
  psInfo->proppage[index].hwndPage = 0;
  psInfo->proppage[index].isDirty = FALSE;

  /*
   * Process property page flags.
   */
  dwFlags = lppsp->dwFlags;
  psInfo->proppage[index].useCallback = (dwFlags & PSP_USECALLBACK) && (lppsp->pfnCallback);
  psInfo->proppage[index].hasHelp = dwFlags & PSP_HASHELP;
  psInfo->proppage[index].hasIcon = dwFlags & (PSP_USEHICON | PSP_USEICONID);

  /* as soon as we have a page with the help flag, set the sheet flag on */
  if (psInfo->proppage[index].hasHelp)
    psInfo->hasHelp = TRUE;

  /*
   * Process page template.
   */
  if (dwFlags & PSP_DLGINDIRECT)
    pTemplate = (DLGTEMPLATE*)lppsp->u.pResource;
  else if(dwFlags & PSP_INTERNAL_UNICODE )
  {
    HRSRC hResource = FindResourceW(lppsp->hInstance,
                                    lppsp->u.pszTemplate,
                                    (LPWSTR)RT_DIALOG);
    HGLOBAL hTemplate = LoadResource(lppsp->hInstance,
                                     hResource);
    pTemplate = (LPDLGTEMPLATEW)LockResource(hTemplate);
  }
  else
  {
    HRSRC hResource = FindResourceA(lppsp->hInstance,
                                    (LPSTR)lppsp->u.pszTemplate,
                                    (LPSTR)RT_DIALOG);
    HGLOBAL hTemplate = LoadResource(lppsp->hInstance,
                                     hResource);
    pTemplate = (LPDLGTEMPLATEA)LockResource(hTemplate);
  }

  /*
   * Extract the size of the page and the caption.
   */
  if (!pTemplate)
      return FALSE;

  p = (const WORD *)pTemplate;

  if (((MyDLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF)
  {
    /* DLGTEMPLATEEX (not defined in any std. header file) */

    p++;       /* dlgVer    */
    p++;       /* signature */
    p += 2;    /* help ID   */
    p += 2;    /* ext style */
    p += 2;    /* style     */
  }
  else
  {
    /* DLGTEMPLATE */

    p += 2;    /* style     */
    p += 2;    /* ext style */
  }

  p++;    /* nb items */
  p++;    /*   x      */
  p++;    /*   y      */
  width  = (WORD)*p; p++;
  height = (WORD)*p; p++;

  /* remember the largest width and height */
  if (width > psInfo->width)
    psInfo->width = width;

  if (height > psInfo->height)
    psInfo->height = height;

  /* menu */
  switch ((WORD)*p)
  {
    case 0x0000:
      p++;
      break;
    case 0xffff:
      p += 2;
      break;
    default:
      p += lstrlenW( (LPCWSTR)p ) + 1;
      break;
  }

  /* class */
  switch ((WORD)*p)
  {
    case 0x0000:
      p++;
      break;
    case 0xffff:
      p += 2;
      break;
    default:
      p += lstrlenW( (LPCWSTR)p ) + 1;
      break;
  }

  /* Extract the caption */
  psInfo->proppage[index].pszText = (LPCWSTR)p;
  TRACE("Tab %d %s\n",index,debugstr_w((LPCWSTR)p));
  p += lstrlenW((LPCWSTR)p) + 1;

  if (dwFlags & PSP_USETITLE)
  {
    WCHAR szTitle[256];
    const WCHAR *pTitle;
    static WCHAR pszNull[] = { '(','n','u','l','l',')',0 };
    int len;

    if ( !HIWORD( lppsp->pszTitle ) )
    {
      if (!LoadStringW( lppsp->hInstance, (UINT)lppsp->pszTitle,szTitle,sizeof(szTitle) ))
      {
        pTitle = pszNull;
	FIXME("Could not load resource #%04x?\n",LOWORD(lppsp->pszTitle));
      }
      else
        pTitle = szTitle;
    }
    else
      pTitle = lppsp->pszTitle;

    len = strlenW(pTitle);
    psInfo->proppage[index].pszText = Alloc( (len+1)*sizeof (WCHAR) );
    strcpyW( (LPWSTR)psInfo->proppage[index].pszText,pTitle);
  }

  /*
   * Build the image list for icons
   */
  if ((dwFlags & PSP_USEHICON) || (dwFlags & PSP_USEICONID))
  {
    HICON hIcon;
    int icon_cx = GetSystemMetrics(SM_CXSMICON);
    int icon_cy = GetSystemMetrics(SM_CYSMICON);

    if (dwFlags & PSP_USEICONID)
      hIcon = LoadImageW(lppsp->hInstance, lppsp->u2.pszIcon, IMAGE_ICON,
                         icon_cx, icon_cy, LR_DEFAULTCOLOR);
    else
      hIcon = lppsp->u2.hIcon;

    if ( hIcon )
    {
      if (psInfo->hImageList == 0 )
	psInfo->hImageList = ImageList_Create(icon_cx, icon_cy, ILC_COLOR, 1, 1);

      ImageList_AddIcon(psInfo->hImageList, hIcon);
    }

  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_CreateDialog
 *
 * Creates the actual property sheet.
 */
int PROPSHEET_CreateDialog(PropSheetInfo* psInfo)
{
  LRESULT ret;
  LPCVOID template;
  LPVOID temp = 0;
  HRSRC hRes;
  DWORD resSize;
  WORD resID = IDD_PROPSHEET;

  TRACE("\n");
  if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
    resID = IDD_WIZARD;

  if( psInfo->unicode )
  {
    if(!(hRes = FindResourceW(COMCTL32_hModule,
                            MAKEINTRESOURCEW(resID),
                            (LPWSTR)RT_DIALOG)))
      return -1;
  }
  else
  {
    if(!(hRes = FindResourceA(COMCTL32_hModule,
                            MAKEINTRESOURCEA(resID),
                            (LPSTR)RT_DIALOG)))
      return -1;
  }

  if(!(template = (LPVOID)LoadResource(COMCTL32_hModule, hRes)))
    return -1;

  /*
   * Make a copy of the dialog template.
   */
  resSize = SizeofResource(COMCTL32_hModule, hRes);

  temp = Alloc(resSize);

  if (!temp)
    return -1;

  memcpy(temp, template, resSize);

  if (psInfo->useCallback)
    (*(psInfo->ppshheader.pfnCallback))(0, PSCB_PRECREATE, (LPARAM)temp);

  if( psInfo->unicode )
  {
    if (!(psInfo->ppshheader.dwFlags & PSH_MODELESS))
      ret = DialogBoxIndirectParamW(psInfo->ppshheader.hInstance,
                                    (LPDLGTEMPLATEW) temp,
                                    psInfo->ppshheader.hwndParent,
                                    PROPSHEET_DialogProc,
                                    (LPARAM)psInfo);
    else
    {
      ret = (int)CreateDialogIndirectParamW(psInfo->ppshheader.hInstance,
                                            (LPDLGTEMPLATEW) temp,
                                            psInfo->ppshheader.hwndParent,
                                            PROPSHEET_DialogProc,
                                            (LPARAM)psInfo);
      if ( !ret ) ret = -1;
    }
  }
  else
  {
    if (!(psInfo->ppshheader.dwFlags & PSH_MODELESS))
      ret = DialogBoxIndirectParamA(psInfo->ppshheader.hInstance,
                                    (LPDLGTEMPLATEA) temp,
                                    psInfo->ppshheader.hwndParent,
                                    PROPSHEET_DialogProc,
                                    (LPARAM)psInfo);
    else
    {
      ret = (int)CreateDialogIndirectParamA(psInfo->ppshheader.hInstance,
                                            (LPDLGTEMPLATEA) temp,
                                            psInfo->ppshheader.hwndParent,
                                            PROPSHEET_DialogProc,
                                            (LPARAM)psInfo);
      if ( !ret ) ret = -1;
    }
  }

  Free(temp);

  return ret;
}

/******************************************************************************
 *            PROPSHEET_SizeMismatch
 *
 *     Verify that the tab control and the "largest" property sheet page dlg. template
 *     match in size.
 */
static BOOL PROPSHEET_SizeMismatch(HWND hwndDlg, PropSheetInfo* psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  RECT rcOrigTab, rcPage;

  /*
   * Original tab size.
   */
  GetClientRect(hwndTabCtrl, &rcOrigTab);
  TRACE("orig tab %ld %ld %ld %ld\n", rcOrigTab.left, rcOrigTab.top,
        rcOrigTab.right, rcOrigTab.bottom);

  /*
   * Biggest page size.
   */
  rcPage.left   = psInfo->x;
  rcPage.top    = psInfo->y;
  rcPage.right  = psInfo->width;
  rcPage.bottom = psInfo->height;

  MapDialogRect(hwndDlg, &rcPage);
  TRACE("biggest page %ld %ld %ld %ld\n", rcPage.left, rcPage.top,
        rcPage.right, rcPage.bottom);

  if ( (rcPage.right - rcPage.left) != (rcOrigTab.right - rcOrigTab.left) )
    return TRUE;
  if ( (rcPage.bottom - rcPage.top) != (rcOrigTab.bottom - rcOrigTab.top) )
    return TRUE;

  return FALSE;
}

/******************************************************************************
 *            PROPSHEET_IsTooSmallWizard
 *
 * Verify that the default property sheet is big enough.
 */
static BOOL PROPSHEET_IsTooSmallWizard(HWND hwndDlg, PropSheetInfo* psInfo)
{
  RECT rcSheetRect, rcPage, rcLine, rcSheetClient;
  HWND hwndLine = GetDlgItem(hwndDlg, IDC_SUNKEN_LINE);
  PADDING_INFO padding = PROPSHEET_GetPaddingInfoWizard(hwndDlg, psInfo);

  GetClientRect(hwndDlg, &rcSheetClient);
  GetWindowRect(hwndDlg, &rcSheetRect);
  GetWindowRect(hwndLine, &rcLine);

  /* Remove the space below the sunken line */
  rcSheetClient.bottom -= (rcSheetRect.bottom - rcLine.top);

  /* Remove the buffer zone all around the edge */
  rcSheetClient.bottom -= (padding.y * 2);
  rcSheetClient.right -= (padding.x * 2);

  /*
   * Biggest page size.
   */
  rcPage.left   = psInfo->x;
  rcPage.top    = psInfo->y;
  rcPage.right  = psInfo->width;
  rcPage.bottom = psInfo->height;

  MapDialogRect(hwndDlg, &rcPage);
  TRACE("biggest page %ld %ld %ld %ld\n", rcPage.left, rcPage.top,
        rcPage.right, rcPage.bottom);

  if (rcPage.right > rcSheetClient.right)
    return TRUE;

  if (rcPage.bottom > rcSheetClient.bottom)
    return TRUE;

  return FALSE;
}

/******************************************************************************
 *            PROPSHEET_AdjustSize
 *
 * Resizes the property sheet and the tab control to fit the largest page.
 */
static BOOL PROPSHEET_AdjustSize(HWND hwndDlg, PropSheetInfo* psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  HWND hwndButton = GetDlgItem(hwndDlg, IDOK);
  RECT rc,tabRect;
  int tabOffsetX, tabOffsetY, buttonHeight;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfo(hwndDlg);
  RECT units;

  /* Get the height of buttons */
  GetClientRect(hwndButton, &rc);
  buttonHeight = rc.bottom;

  /*
   * Biggest page size.
   */
  rc.left   = psInfo->x;
  rc.top    = psInfo->y;
  rc.right  = psInfo->width;
  rc.bottom = psInfo->height;

  MapDialogRect(hwndDlg, &rc);

  /* retrieve the dialog units */
  units.left = units.right = 4;
  units.top = units.bottom = 8;
  MapDialogRect(hwndDlg, &units);

  /*
   * Resize the tab control.
   */
  GetClientRect(hwndTabCtrl,&tabRect);

  SendMessageW(hwndTabCtrl, TCM_ADJUSTRECT, FALSE, (LPARAM)&tabRect);

  if ((rc.bottom - rc.top) < (tabRect.bottom - tabRect.top))
  {
      rc.bottom = rc.top + tabRect.bottom - tabRect.top;
      psInfo->height = MulDiv((rc.bottom - rc.top),8,units.top);
  }

  if ((rc.right - rc.left) < (tabRect.right - tabRect.left))
  {
      rc.right = rc.left + tabRect.right - tabRect.left;
      psInfo->width  = MulDiv((rc.right - rc.left),4,units.left);
  }

  SendMessageW(hwndTabCtrl, TCM_ADJUSTRECT, TRUE, (LPARAM)&rc);

  tabOffsetX = -(rc.left);
  tabOffsetY = -(rc.top);

  rc.right -= rc.left;
  rc.bottom -= rc.top;
  TRACE("setting tab %08lx, rc (0,0)-(%ld,%ld)\n",
        (DWORD)hwndTabCtrl, rc.right, rc.bottom);
  SetWindowPos(hwndTabCtrl, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  GetClientRect(hwndTabCtrl, &rc);

  TRACE("tab client rc %ld %ld %ld %ld\n",
        rc.left, rc.top, rc.right, rc.bottom);

  rc.right += ((padding.x * 2) + tabOffsetX);
  rc.bottom += (buttonHeight + (3 * padding.y) + tabOffsetY);

  /*
   * Resize the property sheet.
   */
  TRACE("setting dialog %08lx, rc (0,0)-(%ld,%ld)\n",
        (DWORD)hwndDlg, rc.right, rc.bottom);
  SetWindowPos(hwndDlg, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustSizeWizard
 *
 * Resizes the property sheet to fit the largest page.
 */
static BOOL PROPSHEET_AdjustSizeWizard(HWND hwndDlg, PropSheetInfo* psInfo)
{
  HWND hwndButton = GetDlgItem(hwndDlg, IDCANCEL);
  HWND hwndLine = GetDlgItem(hwndDlg, IDC_SUNKEN_LINE);
  HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  RECT rc,tabRect;
  int buttonHeight, lineHeight;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfoWizard(hwndDlg, psInfo);
  RECT units;

  /* Get the height of buttons */
  GetClientRect(hwndButton, &rc);
  buttonHeight = rc.bottom;

  GetClientRect(hwndLine, &rc);
  lineHeight = rc.bottom;

  /* retrieve the dialog units */
  units.left = units.right = 4;
  units.top = units.bottom = 8;
  MapDialogRect(hwndDlg, &units);

  /*
   * Biggest page size.
   */
  rc.left   = psInfo->x;
  rc.top    = psInfo->y;
  rc.right  = psInfo->width;
  rc.bottom = psInfo->height;

  MapDialogRect(hwndDlg, &rc);

  GetClientRect(hwndTabCtrl,&tabRect);

  if ((rc.bottom - rc.top) < (tabRect.bottom - tabRect.top))
  {
      rc.bottom = rc.top + tabRect.bottom - tabRect.top;
      psInfo->height = MulDiv((rc.bottom - rc.top), 8, units.top);
  }

  if ((rc.right - rc.left) < (tabRect.right - tabRect.left))
  {
      rc.right = rc.left + tabRect.right - tabRect.left;
      psInfo->width  = MulDiv((rc.right - rc.left), 4, units.left);
  }

  TRACE("Biggest page %ld %ld %ld %ld\n", rc.left, rc.top, rc.right, rc.bottom);
  TRACE("   constants padx=%d, pady=%d, butH=%d, lH=%d\n",
	padding.x, padding.y, buttonHeight, lineHeight);

  /* Make room */
  rc.right += (padding.x * 2);
  rc.bottom += (buttonHeight + (5 * padding.y) + lineHeight);

  /*
   * Resize the property sheet.
   */
  TRACE("setting dialog %08lx, rc (0,0)-(%ld,%ld)\n",
        (DWORD)hwndDlg, rc.right, rc.bottom);
  SetWindowPos(hwndDlg, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustButtons
 *
 * Adjusts the buttons' positions.
 */
static BOOL PROPSHEET_AdjustButtons(HWND hwndParent, PropSheetInfo* psInfo)
{
  HWND hwndButton = GetDlgItem(hwndParent, IDOK);
  RECT rcSheet;
  int x, y;
  int num_buttons = 2;
  int buttonWidth, buttonHeight;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfo(hwndParent);

  if (psInfo->hasApply)
    num_buttons++;

  if (psInfo->hasHelp)
    num_buttons++;

  /*
   * Obtain the size of the buttons.
   */
  GetClientRect(hwndButton, &rcSheet);
  buttonWidth = rcSheet.right;
  buttonHeight = rcSheet.bottom;

  /*
   * Get the size of the property sheet.
   */
  GetClientRect(hwndParent, &rcSheet);

  /*
   * All buttons will be at this y coordinate.
   */
  y = rcSheet.bottom - (padding.y + buttonHeight);

  /*
   * Position OK button.
   */
  hwndButton = GetDlgItem(hwndParent, IDOK);

  x = rcSheet.right - ((padding.x + buttonWidth) * num_buttons);

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Cancel button.
   */
  hwndButton = GetDlgItem(hwndParent, IDCANCEL);

  x = rcSheet.right - ((padding.x + buttonWidth) * (num_buttons - 1));

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Apply button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_APPLY_BUTTON);

  if (psInfo->hasApply)
  {
    if (psInfo->hasHelp)
      x = rcSheet.right - ((padding.x + buttonWidth) * 2);
    else
      x = rcSheet.right - (padding.x + buttonWidth);

    SetWindowPos(hwndButton, 0, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    EnableWindow(hwndButton, FALSE);
  }
  else
    ShowWindow(hwndButton, SW_HIDE);

  /*
   * Position Help button.
   */
  hwndButton = GetDlgItem(hwndParent, IDHELP);

  if (psInfo->hasHelp)
  {
    x = rcSheet.right - (padding.x + buttonWidth);

    SetWindowPos(hwndButton, 0, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  else
    ShowWindow(hwndButton, SW_HIDE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustButtonsWizard
 *
 * Adjusts the buttons' positions.
 */
static BOOL PROPSHEET_AdjustButtonsWizard(HWND hwndParent,
                                          PropSheetInfo* psInfo)
{
  HWND hwndButton = GetDlgItem(hwndParent, IDCANCEL);
  HWND hwndLine = GetDlgItem(hwndParent, IDC_SUNKEN_LINE);
  RECT rcSheet;
  int x, y;
  int num_buttons = 3;
  int buttonWidth, buttonHeight, lineHeight, lineWidth;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfoWizard(hwndParent, psInfo);

  if (psInfo->hasHelp)
    num_buttons++;

  /*
   * Obtain the size of the buttons.
   */
  GetClientRect(hwndButton, &rcSheet);
  buttonWidth = rcSheet.right;
  buttonHeight = rcSheet.bottom;

  GetClientRect(hwndLine, &rcSheet);
  lineHeight = rcSheet.bottom;

  /*
   * Get the size of the property sheet.
   */
  GetClientRect(hwndParent, &rcSheet);

  /*
   * All buttons will be at this y coordinate.
   */
  y = rcSheet.bottom - (padding.y + buttonHeight);

  /*
   * Position the Next and the Finish buttons.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_NEXT_BUTTON);

  x = rcSheet.right - ((padding.x + buttonWidth) * (num_buttons - 1));

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  hwndButton = GetDlgItem(hwndParent, IDC_FINISH_BUTTON);

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  ShowWindow(hwndButton, SW_HIDE);

  /*
   * Position the Back button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_BACK_BUTTON);

  x -= buttonWidth;

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position the Cancel button.
   */
  hwndButton = GetDlgItem(hwndParent, IDCANCEL);

  x = rcSheet.right - ((padding.x + buttonWidth) * (num_buttons - 2));

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Help button.
   */
  hwndButton = GetDlgItem(hwndParent, IDHELP);

  if (psInfo->hasHelp)
  {
    x = rcSheet.right - (padding.x + buttonWidth);

    SetWindowPos(hwndButton, 0, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  else
    ShowWindow(hwndButton, SW_HIDE);

  /*
   * Position and resize the sunken line.
   */
  x = padding.x;
  y = rcSheet.bottom - ((padding.y * 2) + buttonHeight + lineHeight);

  GetClientRect(hwndParent, &rcSheet);
  lineWidth = rcSheet.right - (padding.x * 2);

  SetWindowPos(hwndLine, 0, x, y, lineWidth, 2,
               SWP_NOZORDER | SWP_NOACTIVATE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_GetPaddingInfo
 *
 * Returns the layout information.
 */
static PADDING_INFO PROPSHEET_GetPaddingInfo(HWND hwndDlg)
{
  HWND hwndTab = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  RECT rcTab;
  POINT tl;
  PADDING_INFO padding;

  GetWindowRect(hwndTab, &rcTab);

  tl.x = rcTab.left;
  tl.y = rcTab.top;

  ScreenToClient(hwndDlg, &tl);

  padding.x = tl.x;
  padding.y = tl.y;

  return padding;
}

/******************************************************************************
 *            PROPSHEET_GetPaddingInfoWizard
 *
 * Returns the layout information.
 * Vertical spacing is the distance between the line and the buttons.
 * Do NOT use the Help button to gather padding information when it isn't mapped
 * (PSH_HASHELP), as app writers aren't forced to supply correct coordinates
 * for it in this case !
 * FIXME: I'm not sure about any other coordinate problems with these evil
 * buttons. Fix it in case additional problems appear or maybe calculate
 * a padding in a completely different way, as this is somewhat messy.
 */
static PADDING_INFO PROPSHEET_GetPaddingInfoWizard(HWND hwndDlg, const PropSheetInfo*
 psInfo)
{
  PADDING_INFO padding;
  RECT rc;
  HWND hwndControl;
  INT idButton;
  POINT ptButton, ptLine;

  TRACE("\n");
  if (psInfo->hasHelp)
  {
	idButton = IDHELP;
  }
  else
  {
    if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
    {
	idButton = IDC_NEXT_BUTTON;
    }
    else
    {
	/* hopefully this is ok */
	idButton = IDCANCEL;
    }
  }

  hwndControl = GetDlgItem(hwndDlg, idButton);
  GetWindowRect(hwndControl, &rc);

  ptButton.x = rc.left;
  ptButton.y = rc.top;

  ScreenToClient(hwndDlg, &ptButton);

  /* Line */
  hwndControl = GetDlgItem(hwndDlg, IDC_SUNKEN_LINE);
  GetWindowRect(hwndControl, &rc);

  ptLine.x = 0;
  ptLine.y = rc.bottom;

  ScreenToClient(hwndDlg, &ptLine);

  padding.y = ptButton.y - ptLine.y;

  if (padding.y < 0)
	  ERR("padding negative ! Please report this !\n");

  /* this is most probably not correct, but the best we have now */
  padding.x = padding.y;
  return padding;
}

/******************************************************************************
 *            PROPSHEET_CreateTabControl
 *
 * Insert the tabs in the tab control.
 */
static BOOL PROPSHEET_CreateTabControl(HWND hwndParent,
                                       PropSheetInfo * psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndParent, IDC_TABCONTROL);
  TCITEMW item;
  int i, nTabs;
  int iImage = 0;

  TRACE("\n");
  item.mask = TCIF_TEXT;
  item.cchTextMax = MAX_TABTEXT_LENGTH;

  nTabs = psInfo->nPages;

  /*
   * Set the image list for icons.
   */
  if (psInfo->hImageList)
  {
    SendMessageW(hwndTabCtrl, TCM_SETIMAGELIST, 0, (LPARAM)psInfo->hImageList);
  }

  for (i = 0; i < nTabs; i++)
  {
    if ( psInfo->proppage[i].hasIcon )
    {
      item.mask |= TCIF_IMAGE;
      item.iImage = iImage++;
    }
    else
    {
      item.mask &= ~TCIF_IMAGE;
    }

    item.pszText = (LPWSTR) psInfo->proppage[i].pszText;
    SendMessageW(hwndTabCtrl, TCM_INSERTITEMW, (WPARAM)i, (LPARAM)&item);
  }

  return TRUE;
}
/*
 * Get the size of an in-memory Template
 *
 *( Based on the code of PROPSHEET_CollectPageInfo)
 * See also dialog.c/DIALOG_ParseTemplate32().
 */

static UINT GetTemplateSize(DLGTEMPLATE* pTemplate)

{
  const WORD*  p = (const WORD *)pTemplate;
  BOOL  istemplateex = (((MyDLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF);
  WORD nrofitems;

  if (istemplateex)
  {
    /* DLGTEMPLATEEX (not defined in any std. header file) */

    TRACE("is DLGTEMPLATEEX\n");
    p++;       /* dlgVer    */
    p++;       /* signature */
    p += 2;    /* help ID   */
    p += 2;    /* ext style */
    p += 2;    /* style     */
  }
  else
  {
    /* DLGTEMPLATE */

    TRACE("is DLGTEMPLATE\n");
    p += 2;    /* style     */
    p += 2;    /* ext style */
  }

  nrofitems =   (WORD)*p; p++;    /* nb items */
  p++;    /*   x      */
  p++;    /*   y      */
  p++;    /*   width  */
  p++;    /*   height */

  /* menu */
  switch ((WORD)*p)
  {
    case 0x0000:
      p++;
      break;
    case 0xffff:
      p += 2;
      break;
    default:
      TRACE("menu %s\n",debugstr_w((LPCWSTR)p));
      p += lstrlenW( (LPCWSTR)p ) + 1;
      break;
  }

  /* class */
  switch ((WORD)*p)
  {
    case 0x0000:
      p++;
      break;
    case 0xffff:
      p += 2; /* 0xffff plus predefined window class ordinal value */
      break;
    default:
      TRACE("class %s\n",debugstr_w((LPCWSTR)p));
      p += lstrlenW( (LPCWSTR)p ) + 1;
      break;
  }

  /* title */
  TRACE("title %s\n",debugstr_w((LPCWSTR)p));
  p += lstrlenW((LPCWSTR)p) + 1;

  /* font, if DS_SETFONT set */
  if ((DS_SETFONT & ((istemplateex)?  ((MyDLGTEMPLATEEX*)pTemplate)->style :
		     pTemplate->style)))
    {
      p+=(istemplateex)?3:1;
      TRACE("font %s\n",debugstr_w((LPCWSTR)p));
      p += lstrlenW( (LPCWSTR)p ) + 1; /* the font name */
    }

  /* now process the DLGITEMTEMPLATE(EX) structs (plus custom data)
   * that are following the DLGTEMPLATE(EX) data */
  TRACE("%d items\n",nrofitems);
  while (nrofitems > 0)
    {
      p = (WORD*)(((DWORD)p + 3) & ~3); /* DWORD align */
      
      /* skip header */
      p += (istemplateex ? sizeof(MyDLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE))/sizeof(WORD);
      
      /* check class */
      switch ((WORD)*p)
	{
	case 0x0000:
	  p++;
	  break;
	case 0xffff:
	  TRACE("class ordinal 0x%08lx\n",*(DWORD*)p);
	  p += 2;
	  break;
	default:
	  TRACE("class %s\n",debugstr_w((LPCWSTR)p));
	  p += lstrlenW( (LPCWSTR)p ) + 1;
	  break;
	}

      /* check title text */
      switch ((WORD)*p)
	{
	case 0x0000:
	  p++;
	  break;
	case 0xffff:
	  TRACE("text ordinal 0x%08lx\n",*(DWORD*)p);
	  p += 2;
	  break;
	default:
	  TRACE("text %s\n",debugstr_w((LPCWSTR)p));
	  p += lstrlenW( (LPCWSTR)p ) + 1;
	  break;
	}
      p += *p + 1;    /* Skip extra data */
      --nrofitems;
    }
  
  TRACE("%p %p size 0x%08x\n",p, (WORD*)pTemplate,sizeof(WORD)*(p - (WORD*)pTemplate));
  return (p - (WORD*)pTemplate)*sizeof(WORD);
  
}

/******************************************************************************
 *            PROPSHEET_CreatePage
 *
 * Creates a page.
 */
static BOOL PROPSHEET_CreatePage(HWND hwndParent,
                                int index,
                                const PropSheetInfo * psInfo,
                                LPCPROPSHEETPAGEW ppshpage)
{
  DLGTEMPLATE* pTemplate;
  HWND hwndPage;
  RECT rc;
  PropPageInfo* ppInfo = psInfo->proppage;
  PADDING_INFO padding;
  UINT pageWidth,pageHeight;
  DWORD resSize;
  LPVOID temp = NULL;

  TRACE("index %d\n", index);

  if (ppshpage == NULL)
  {
    return FALSE;
  }

  if (ppshpage->dwFlags & PSP_DLGINDIRECT)
    {
      pTemplate = (DLGTEMPLATE*)ppshpage->u.pResource;
      resSize = GetTemplateSize(pTemplate);
    }
  else if(ppshpage->dwFlags & PSP_INTERNAL_UNICODE)
  {
    HRSRC hResource;
    HANDLE hTemplate;

    hResource = FindResourceW(ppshpage->hInstance,
                                    ppshpage->u.pszTemplate,
                                    (LPWSTR)RT_DIALOG);
    if(!hResource)
	return FALSE;

    resSize = SizeofResource(ppshpage->hInstance, hResource);

    hTemplate = LoadResource(ppshpage->hInstance, hResource);
    if(!hTemplate)
	return FALSE;

    pTemplate = (LPDLGTEMPLATEW)LockResource(hTemplate);
    /*
     * Make a copy of the dialog template to make it writable
     */
  }
  else
  {
    HRSRC hResource;
    HANDLE hTemplate;

    hResource = FindResourceA(ppshpage->hInstance,
                                    (LPSTR)ppshpage->u.pszTemplate,
                                    (LPSTR)RT_DIALOG);
    if(!hResource)
	return FALSE;

    resSize = SizeofResource(ppshpage->hInstance, hResource);

    hTemplate = LoadResource(ppshpage->hInstance, hResource);
    if(!hTemplate)
	return FALSE;

    pTemplate = (LPDLGTEMPLATEA)LockResource(hTemplate);
    /*
     * Make a copy of the dialog template to make it writable
     */
  }
  temp = Alloc(resSize);
  if (!temp)
    return FALSE;
  
  TRACE("copying pTemplate %p into temp %p (%ld)\n", pTemplate, temp, resSize);
  memcpy(temp, pTemplate, resSize);
  pTemplate = temp;

  if (((MyDLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF)
  {
    ((MyDLGTEMPLATEEX*)pTemplate)->style |= WS_CHILD | DS_CONTROL;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~DS_MODALFRAME;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_CAPTION;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_SYSMENU;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_POPUP;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_DISABLED;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_VISIBLE;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_THICKFRAME;
  }
  else
  {
    pTemplate->style |= WS_CHILD | DS_CONTROL;
    pTemplate->style &= ~DS_MODALFRAME;
    pTemplate->style &= ~WS_CAPTION;
    pTemplate->style &= ~WS_SYSMENU;
    pTemplate->style &= ~WS_POPUP;
    pTemplate->style &= ~WS_DISABLED;
    pTemplate->style &= ~WS_VISIBLE;
    pTemplate->style &= ~WS_THICKFRAME;
  }

  if (psInfo->proppage[index].useCallback)
    (*(ppshpage->pfnCallback))(0, PSPCB_CREATE,
                               (LPPROPSHEETPAGEW)ppshpage);

  if(ppshpage->dwFlags & PSP_INTERNAL_UNICODE)
     hwndPage = CreateDialogIndirectParamW(ppshpage->hInstance,
					pTemplate,
					hwndParent,
					ppshpage->pfnDlgProc,
					(LPARAM)ppshpage);
  else
     hwndPage = CreateDialogIndirectParamA(ppshpage->hInstance,
					pTemplate,
					hwndParent,
					ppshpage->pfnDlgProc,
					(LPARAM)ppshpage);
  /* Free a no more needed copy */
  if(temp)
      Free(temp);

  ppInfo[index].hwndPage = hwndPage;

  if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD) {
      /* FIXME: This code may no longer be correct.
       *        It was not for the non-wizard path.  (GLA 6/02)
       */
      rc.left = psInfo->x;
      rc.top = psInfo->y;
      rc.right = psInfo->width;
      rc.bottom = psInfo->height;

      MapDialogRect(hwndParent, &rc);

      pageWidth = rc.right - rc.left;
      pageHeight = rc.bottom - rc.top;

      padding = PROPSHEET_GetPaddingInfoWizard(hwndParent, psInfo);
      TRACE("setting page %08lx, rc (%ld,%ld)-(%ld,%ld) w=%d, h=%d, padx=%d, pady=%d\n",
	    (DWORD)hwndPage, rc.left, rc.top, rc.right, rc.bottom,
	    pageWidth, pageHeight, padding.x, padding.y);
      SetWindowPos(hwndPage, HWND_TOP,
		   rc.left + padding.x/2,
		   rc.top + padding.y/2,
		   pageWidth, pageHeight, 0);
  }
  else {
      /*
       * Ask the Tab control to reduce the client rectangle to that
       * it has available.
       */
      PROPSHEET_GetPageRect(psInfo, hwndParent, &rc);
      pageWidth = rc.right - rc.left;
      pageHeight = rc.bottom - rc.top;
      TRACE("setting page %08lx, rc (%ld,%ld)-(%ld,%ld) w=%d, h=%d\n",
	    (DWORD)hwndPage, rc.left, rc.top, rc.right, rc.bottom,
	    pageWidth, pageHeight);
      SetWindowPos(hwndPage, HWND_TOP,
		   rc.left, rc.top,
		   pageWidth, pageHeight, 0);
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_ShowPage
 *
 * Displays or creates the specified page.
 */
static BOOL PROPSHEET_ShowPage(HWND hwndDlg, int index, PropSheetInfo * psInfo)
{
  HWND hwndTabCtrl;

  TRACE("active_page %d, index %d\n", psInfo->active_page, index);
  if (index == psInfo->active_page)
  {
      if (GetTopWindow(hwndDlg) != psInfo->proppage[index].hwndPage)
          SetWindowPos(psInfo->proppage[index].hwndPage, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
      return TRUE;
  }

  if (psInfo->proppage[index].hwndPage == 0)
  {
     LPCPROPSHEETPAGEW ppshpage;

     ppshpage = (LPCPROPSHEETPAGEW)psInfo->proppage[index].hpage;
     PROPSHEET_CreatePage(hwndDlg, index, psInfo, ppshpage);
  }

  PROPSHEET_SetTitleW(hwndDlg, psInfo->ppshheader.dwFlags,
                      psInfo->proppage[index].pszText);

  if (psInfo->active_page != -1)
     ShowWindow(psInfo->proppage[psInfo->active_page].hwndPage, SW_HIDE);

  ShowWindow(psInfo->proppage[index].hwndPage, SW_SHOW);

  /* Synchronize current selection with tab control
   * It seems to be needed even in case of PSH_WIZARD (no tab controls there) */
  hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  SendMessageW(hwndTabCtrl, TCM_SETCURSEL, index, 0);

  psInfo->active_page = index;
  psInfo->activeValid = TRUE;

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Back
 */
static BOOL PROPSHEET_Back(HWND hwndDlg)
{
  PSHNOTIFY psn;
  HWND hwndPage;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);
  LRESULT result;
  int idx;

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return FALSE;

  psn.hdr.code     = PSN_WIZBACK;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  result = SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  if (result == -1)
    return FALSE;
  else if (result == 0)
     idx = psInfo->active_page - 1;
  else
     idx = PROPSHEET_FindPageByResId(psInfo, result);

  if (idx >= 0 && idx < psInfo->nPages)
  {
     if (PROPSHEET_CanSetCurSel(hwndDlg))
        PROPSHEET_SetCurSel(hwndDlg, idx, -1, 0);
  }
  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Next
 */
static BOOL PROPSHEET_Next(HWND hwndDlg)
{
  PSHNOTIFY psn;
  HWND hwndPage;
  LRESULT msgResult = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);
  int idx;

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return FALSE;

  psn.hdr.code     = PSN_WIZNEXT;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  msgResult = SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  if (msgResult == -1)
    return FALSE;
  else if (msgResult == 0)
     idx = psInfo->active_page + 1;
  else
     idx = PROPSHEET_FindPageByResId(psInfo, msgResult);

  if (idx < psInfo->nPages )
  {
     if (PROPSHEET_CanSetCurSel(hwndDlg) != FALSE)
        PROPSHEET_SetCurSel(hwndDlg, idx, 1, 0);
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Finish
 */
static BOOL PROPSHEET_Finish(HWND hwndDlg)
{
  PSHNOTIFY psn;
  HWND hwndPage;
  LRESULT msgResult = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return FALSE;

  psn.hdr.code     = PSN_WIZFINISH;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  msgResult = SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);

  TRACE("msg result %ld\n", msgResult);

  if (msgResult != 0)
    return FALSE;

  if (psInfo->isModeless)
    psInfo->activeValid = FALSE;
  else
    EndDialog(hwndDlg, TRUE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Apply
 */
static BOOL PROPSHEET_Apply(HWND hwndDlg, LPARAM lParam)
{
  int i;
  HWND hwndPage;
  PSHNOTIFY psn;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return FALSE;

  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;


  /*
   * Send PSN_KILLACTIVE to the current page.
   */
  psn.hdr.code = PSN_KILLACTIVE;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  if (SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn) != FALSE)
    return FALSE;

  /*
   * Send PSN_APPLY to all pages.
   */
  psn.hdr.code = PSN_APPLY;
  psn.lParam   = lParam;

  for (i = 0; i < psInfo->nPages; i++)
  {
    hwndPage = psInfo->proppage[i].hwndPage;
    if (hwndPage)
    {
       switch (SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn))
       {
       case PSNRET_INVALID:
           PROPSHEET_ShowPage(hwndDlg, i, psInfo);
           /* fall through */
       case PSNRET_INVALID_NOCHANGEPAGE:
           return FALSE;
       }
    }
  }

  if(lParam)
  {
     psInfo->activeValid = FALSE;
  }
  else if(psInfo->active_page >= 0)
  {
     psn.hdr.code = PSN_SETACTIVE;
     psn.lParam   = 0;
     hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
     SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Cancel
 */
static void PROPSHEET_Cancel(HWND hwndDlg, LPARAM lParam)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);
  HWND hwndPage;
  PSHNOTIFY psn;
  int i;

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
  psn.hdr.code     = PSN_QUERYCANCEL;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  if (SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn))
    return;

  psn.hdr.code = PSN_RESET;
  psn.lParam   = lParam;

  for (i = 0; i < psInfo->nPages; i++)
  {
    hwndPage = psInfo->proppage[i].hwndPage;

    if (hwndPage)
       SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  }

  if (psInfo->isModeless)
  {
     /* makes PSM_GETCURRENTPAGEHWND return NULL */
     psInfo->activeValid = FALSE;
  }
  else
    EndDialog(hwndDlg, FALSE);
}

/******************************************************************************
 *            PROPSHEET_Help
 */
static void PROPSHEET_Help(HWND hwndDlg)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);
  HWND hwndPage;
  PSHNOTIFY psn;

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
  psn.hdr.code     = PSN_HELP;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
}

/******************************************************************************
 *            PROPSHEET_Changed
 */
static void PROPSHEET_Changed(HWND hwndDlg, HWND hwndDirtyPage)
{
  int i;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);

  TRACE("\n");
  if (!psInfo) return;
  /*
   * Set the dirty flag of this page.
   */
  for (i = 0; i < psInfo->nPages; i++)
  {
    if (psInfo->proppage[i].hwndPage == hwndDirtyPage)
      psInfo->proppage[i].isDirty = TRUE;
  }

  /*
   * Enable the Apply button.
   */
  if (psInfo->hasApply)
  {
    HWND hwndApplyBtn = GetDlgItem(hwndDlg, IDC_APPLY_BUTTON);

    EnableWindow(hwndApplyBtn, TRUE);
  }
}

/******************************************************************************
 *            PROPSHEET_UnChanged
 */
static void PROPSHEET_UnChanged(HWND hwndDlg, HWND hwndCleanPage)
{
  int i;
  BOOL noPageDirty = TRUE;
  HWND hwndApplyBtn = GetDlgItem(hwndDlg, IDC_APPLY_BUTTON);
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);

  TRACE("\n");
  if ( !psInfo ) return;
  for (i = 0; i < psInfo->nPages; i++)
  {
    /* set the specified page as clean */
    if (psInfo->proppage[i].hwndPage == hwndCleanPage)
      psInfo->proppage[i].isDirty = FALSE;

    /* look to see if there's any dirty pages */
    if (psInfo->proppage[i].isDirty)
      noPageDirty = FALSE;
  }

  /*
   * Disable Apply button.
   */
  if (noPageDirty)
    EnableWindow(hwndApplyBtn, FALSE);
}

/******************************************************************************
 *            PROPSHEET_PressButton
 */
static void PROPSHEET_PressButton(HWND hwndDlg, int buttonID)
{
  TRACE("buttonID %d\n", buttonID);
  switch (buttonID)
  {
    case PSBTN_APPLYNOW:
      PROPSHEET_DoCommand(hwndDlg, IDC_APPLY_BUTTON);
      break;
    case PSBTN_BACK:
      PROPSHEET_Back(hwndDlg);
      break;
    case PSBTN_CANCEL:
      PROPSHEET_DoCommand(hwndDlg, IDCANCEL);
      break;
    case PSBTN_FINISH:
      PROPSHEET_Finish(hwndDlg);
      break;
    case PSBTN_HELP:
      PROPSHEET_DoCommand(hwndDlg, IDHELP);
      break;
    case PSBTN_NEXT:
      PROPSHEET_Next(hwndDlg);
      break;
    case PSBTN_OK:
      PROPSHEET_DoCommand(hwndDlg, IDOK);
      break;
    default:
      FIXME("Invalid button index %d\n", buttonID);
  }
}


/*************************************************************************
 * BOOL PROPSHEET_CanSetCurSel [Internal]
 *
 * Test whether the current page can be changed by sending a PSN_KILLACTIVE
 *
 * PARAMS
 *     hwndDlg        [I] handle to a Dialog hWnd
 *
 * RETURNS
 *     TRUE if Current Selection can change
 *
 * NOTES
 */
static BOOL PROPSHEET_CanSetCurSel(HWND hwndDlg)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);
  HWND hwndPage;
  PSHNOTIFY psn;
  BOOL res = FALSE;

  TRACE("active_page %d\n", psInfo->active_page);
  if (!psInfo)
  {
     res = FALSE;
     goto end;
  }

  if (psInfo->active_page < 0)
  {
     res = TRUE;
     goto end;
  }

  /*
   * Notify the current page.
   */
  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
  psn.hdr.code     = PSN_KILLACTIVE;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  res = !SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);

end:
  TRACE("<-- %d\n", res);
  return res;
}

/******************************************************************************
 *            PROPSHEET_SetCurSel
 */
static BOOL PROPSHEET_SetCurSel(HWND hwndDlg,
                                int index,
				int skipdir,
                                HPROPSHEETPAGE hpage
				)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndHelp  = GetDlgItem(hwndDlg, IDHELP);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);

  TRACE("index %d, skipdir %d, hpage %p\n", index, skipdir, hpage);
  /* hpage takes precedence over index */
  if (hpage != NULL)
    index = PROPSHEET_GetPageIndex(hpage, psInfo);

  if (index < 0 || index >= psInfo->nPages)
  {
    TRACE("Could not find page to select!\n");
    return FALSE;
  }

  while (1) {
    int result;
    PSHNOTIFY psn;

    if (hwndTabControl)
	SendMessageW(hwndTabControl, TCM_SETCURSEL, index, 0);

    psn.hdr.code     = PSN_SETACTIVE;
    psn.hdr.hwndFrom = hwndDlg;
    psn.hdr.idFrom   = 0;
    psn.lParam       = 0;

    if (!psInfo->proppage[index].hwndPage) {
      LPCPROPSHEETPAGEW ppshpage = (LPCPROPSHEETPAGEW)psInfo->proppage[index].hpage;
      PROPSHEET_CreatePage(hwndDlg, index, psInfo, ppshpage);
    }

    result = SendMessageW(psInfo->proppage[index].hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
    if (!result)
      break;
    if (result == -1) {
      index+=skipdir;
      if (index < 0) {
	index = 0;
	WARN("Tried to skip before first property sheet page!\n");
	break;
      }
      if (index >= psInfo->nPages) {
	WARN("Tried to skip after last property sheet page!\n");
	index = psInfo->nPages-1;
	break;
      }
    }
    else if (result != 0)
    {
      int old_index = index;
      index = PROPSHEET_FindPageByResId(psInfo, result);
      if(index >= psInfo->nPages) {
        index = old_index;
        WARN("Tried to skip to nonexistant page by res id\n");
        break;
      }
      continue;
    }
  }
  /*
   * Display the new page.
   */
  PROPSHEET_ShowPage(hwndDlg, index, psInfo);

  if (psInfo->proppage[index].hasHelp)
    EnableWindow(hwndHelp, TRUE);
  else
    EnableWindow(hwndHelp, FALSE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_SetCurSelId
 *
 * Selects the page, specified by resource id.
 */
static void PROPSHEET_SetCurSelId(HWND hwndDlg, int id)
{
      int idx;
      PropSheetInfo* psInfo =
          (PropSheetInfo*) GetPropW(hwndDlg, PropSheetInfoStr);

      idx = PROPSHEET_FindPageByResId(psInfo, id);
      if (idx < psInfo->nPages )
      {
          if (PROPSHEET_CanSetCurSel(hwndDlg) != FALSE)
              PROPSHEET_SetCurSel(hwndDlg, idx, 1, 0);
      }
}

/******************************************************************************
 *            PROPSHEET_SetTitleA
 */
static void PROPSHEET_SetTitleA(HWND hwndDlg, DWORD dwStyle, LPCSTR lpszText)
{
  if(HIWORD(lpszText))
  {
     WCHAR szTitle[256];
     MultiByteToWideChar(CP_ACP, 0, lpszText, -1,
			     szTitle, sizeof(szTitle));
     PROPSHEET_SetTitleW(hwndDlg, dwStyle, szTitle);
  }
  else
  {
     PROPSHEET_SetTitleW(hwndDlg, dwStyle, (LPCWSTR)lpszText);
  }
}

/******************************************************************************
 *            PROPSHEET_SetTitleW
 */
static void PROPSHEET_SetTitleW(HWND hwndDlg, DWORD dwStyle, LPCWSTR lpszText)
{
  PropSheetInfo*	psInfo = (PropSheetInfo*) GetPropW(hwndDlg, PropSheetInfoStr);
  WCHAR			szTitle[256];

  TRACE("'%s' (style %08lx)\n", debugstr_w(lpszText), dwStyle);
  if (HIWORD(lpszText) == 0) {
    if (!LoadStringW(psInfo->ppshheader.hInstance,
                     LOWORD(lpszText), szTitle, sizeof(szTitle)-sizeof(WCHAR)))
      return;
    lpszText = szTitle;
  }
  if (dwStyle & PSH_PROPTITLE)
  {
    WCHAR* dest;
    int lentitle = strlenW(lpszText);
    int lenprop  = strlenW(psInfo->strPropertiesFor);

    dest = Alloc( (lentitle + lenprop + 1)*sizeof (WCHAR));
    strcpyW(dest, psInfo->strPropertiesFor);
    strcatW(dest, lpszText);

    SetWindowTextW(hwndDlg, dest);
    Free(dest);
  }
  else
    SetWindowTextW(hwndDlg, lpszText);
}

/******************************************************************************
 *            PROPSHEET_SetFinishTextA
 */
static void PROPSHEET_SetFinishTextA(HWND hwndDlg, LPCSTR lpszText)
{
  HWND hwndButton = GetDlgItem(hwndDlg, IDC_FINISH_BUTTON);

  TRACE("'%s'\n", lpszText);
  /* Set text, show and enable the Finish button */
  SetWindowTextA(hwndButton, lpszText);
  ShowWindow(hwndButton, SW_SHOW);
  EnableWindow(hwndButton, TRUE);

  /* Make it default pushbutton */
  SendMessageA(hwndDlg, DM_SETDEFID, IDC_FINISH_BUTTON, 0);

  /* Hide Back button */
  hwndButton = GetDlgItem(hwndDlg, IDC_BACK_BUTTON);
  ShowWindow(hwndButton, SW_HIDE);

  /* Hide Next button */
  hwndButton = GetDlgItem(hwndDlg, IDC_NEXT_BUTTON);
  ShowWindow(hwndButton, SW_HIDE);
}

/******************************************************************************
 *            PROPSHEET_SetFinishTextW
 */
static void PROPSHEET_SetFinishTextW(HWND hwndDlg, LPCWSTR lpszText)
{
  HWND hwndButton = GetDlgItem(hwndDlg, IDC_FINISH_BUTTON);

  TRACE("'%s'\n", debugstr_w(lpszText));
  /* Set text, show and enable the Finish button */
  SetWindowTextW(hwndButton, lpszText);
  ShowWindow(hwndButton, SW_SHOW);
  EnableWindow(hwndButton, TRUE);

  /* Make it default pushbutton */
  SendMessageW(hwndDlg, DM_SETDEFID, IDC_FINISH_BUTTON, 0);

  /* Hide Back button */
  hwndButton = GetDlgItem(hwndDlg, IDC_BACK_BUTTON);
  ShowWindow(hwndButton, SW_HIDE);

  /* Hide Next button */
  hwndButton = GetDlgItem(hwndDlg, IDC_NEXT_BUTTON);
  ShowWindow(hwndButton, SW_HIDE);
}

/******************************************************************************
 *            PROPSHEET_QuerySiblings
 */
static LRESULT PROPSHEET_QuerySiblings(HWND hwndDlg,
                                       WPARAM wParam, LPARAM lParam)
{
  int i = 0;
  HWND hwndPage;
  LRESULT msgResult = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                    PropSheetInfoStr);

  while ((i < psInfo->nPages) && (msgResult == 0))
  {
    hwndPage = psInfo->proppage[i].hwndPage;
    msgResult = SendMessageA(hwndPage, PSM_QUERYSIBLINGS, wParam, lParam);
    i++;
  }

  return msgResult;
}


/******************************************************************************
 *            PROPSHEET_AddPage
 */
static BOOL PROPSHEET_AddPage(HWND hwndDlg,
                              HPROPSHEETPAGE hpage)
{
  PropSheetInfo * psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                     PropSheetInfoStr);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  TCITEMW item;
  LPCPROPSHEETPAGEW ppsp = (LPCPROPSHEETPAGEW)hpage;

  TRACE("hpage %p\n", hpage);
  /*
   * Allocate and fill in a new PropPageInfo entry.
   */
  psInfo->proppage = (PropPageInfo*) ReAlloc(psInfo->proppage,
                                                      sizeof(PropPageInfo) *
                                                      (psInfo->nPages + 1));
  if (!PROPSHEET_CollectPageInfo(ppsp, psInfo, psInfo->nPages))
      return FALSE;

  psInfo->proppage[psInfo->nPages].hpage = hpage;

  if (ppsp->dwFlags & PSP_PREMATURE)
  {
     /* Create the page but don't show it */
     PROPSHEET_CreatePage(hwndDlg, psInfo->nPages, psInfo, ppsp);
  }

  /*
   * Add a new tab to the tab control.
   */
  item.mask = TCIF_TEXT;
  item.pszText = (LPWSTR) psInfo->proppage[psInfo->nPages].pszText;
  item.cchTextMax = MAX_TABTEXT_LENGTH;

  if (psInfo->hImageList)
  {
    SendMessageW(hwndTabControl, TCM_SETIMAGELIST, 0, (LPARAM)psInfo->hImageList);
  }

  if ( psInfo->proppage[psInfo->nPages].hasIcon )
  {
    item.mask |= TCIF_IMAGE;
    item.iImage = psInfo->nPages;
  }

  SendMessageW(hwndTabControl, TCM_INSERTITEMW, psInfo->nPages + 1,
               (LPARAM)&item);

  psInfo->nPages++;

  /* If it is the only page - show it */
  if(psInfo->nPages == 1)
     PROPSHEET_SetCurSel(hwndDlg, 0, 1, 0);
  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_RemovePage
 */
static BOOL PROPSHEET_RemovePage(HWND hwndDlg,
                                 int index,
                                 HPROPSHEETPAGE hpage)
{
  PropSheetInfo * psInfo = (PropSheetInfo*) GetPropW(hwndDlg,
                                                     PropSheetInfoStr);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  PropPageInfo* oldPages;

  TRACE("index %d, hpage %p\n", index, hpage);
  if (!psInfo) {
    return FALSE;
  }
  oldPages = psInfo->proppage;
  /*
   * hpage takes precedence over index.
   */
  if (hpage != 0)
  {
    index = PROPSHEET_GetPageIndex(hpage, psInfo);
  }

  /* Make sure that index is within range */
  if (index < 0 || index >= psInfo->nPages)
  {
      TRACE("Could not find page to remove!\n");
      return FALSE;
  }

  TRACE("total pages %d removing page %d active page %d\n",
        psInfo->nPages, index, psInfo->active_page);
  /*
   * Check if we're removing the active page.
   */
  if (index == psInfo->active_page)
  {
    if (psInfo->nPages > 1)
    {
      if (index > 0)
      {
        /* activate previous page  */
        PROPSHEET_SetCurSel(hwndDlg, index - 1, -1, 0);
      }
      else
      {
        /* activate the next page */
        PROPSHEET_SetCurSel(hwndDlg, index + 1, 1, 0);
        psInfo->active_page = index;
      }
    }
    else
    {
      psInfo->active_page = -1;
      if (!psInfo->isModeless)
      {
         EndDialog(hwndDlg, FALSE);
         return TRUE;
      }
    }
  }
  else if (index < psInfo->active_page)
    psInfo->active_page--;

  /* Destroy page dialog window */
  DestroyWindow(psInfo->proppage[index].hwndPage);

  /* Free page resources */
  if(psInfo->proppage[index].hpage)
  {
     PROPSHEETPAGEW* psp = (PROPSHEETPAGEW*)psInfo->proppage[index].hpage;

     if ((psp->dwFlags & PSP_USETITLE) && psInfo->proppage[index].pszText)
        HeapFree(GetProcessHeap(), 0, (LPVOID)psInfo->proppage[index].pszText);

     DestroyPropertySheetPage(psInfo->proppage[index].hpage);
  }

  /* Remove the tab */
  SendMessageW(hwndTabControl, TCM_DELETEITEM, index, 0);

  psInfo->nPages--;
  psInfo->proppage = Alloc(sizeof(PropPageInfo) * psInfo->nPages);

  if (index > 0)
    memcpy(&psInfo->proppage[0], &oldPages[0], index * sizeof(PropPageInfo));

  if (index < psInfo->nPages)
    memcpy(&psInfo->proppage[index], &oldPages[index + 1],
           (psInfo->nPages - index) * sizeof(PropPageInfo));

  Free(oldPages);

  return FALSE;
}

/******************************************************************************
 *            PROPSHEET_SetWizButtons
 *
 * This code will work if (and assumes that) the Next button is on top of the
 * Finish button. ie. Finish comes after Next in the Z order.
 * This means make sure the dialog template reflects this.
 *
 */
static void PROPSHEET_SetWizButtons(HWND hwndDlg, DWORD dwFlags)
{
  HWND hwndBack   = GetDlgItem(hwndDlg, IDC_BACK_BUTTON);
  HWND hwndNext   = GetDlgItem(hwndDlg, IDC_NEXT_BUTTON);
  HWND hwndFinish = GetDlgItem(hwndDlg, IDC_FINISH_BUTTON);

  TRACE("%ld\n", dwFlags);

  EnableWindow(hwndBack, FALSE);
  EnableWindow(hwndNext, FALSE);
  EnableWindow(hwndFinish, FALSE);

  if (dwFlags & PSWIZB_BACK)
    EnableWindow(hwndBack, TRUE);

  if (dwFlags & PSWIZB_NEXT)
  {
    /* Hide the Finish button */
    ShowWindow(hwndFinish, SW_HIDE);

    /* Show and enable the Next button */
    ShowWindow(hwndNext, SW_SHOW);
    EnableWindow(hwndNext, TRUE);

    /* Set the Next button as the default pushbutton  */
    SendMessageA(hwndDlg, DM_SETDEFID, IDC_NEXT_BUTTON, 0);
  }

  if ((dwFlags & PSWIZB_FINISH) || (dwFlags & PSWIZB_DISABLEDFINISH))
  {
    /* Hide the Next button */
    ShowWindow(hwndNext, SW_HIDE);

    /* Show the Finish button */
    ShowWindow(hwndFinish, SW_SHOW);

    if (dwFlags & PSWIZB_FINISH)
      EnableWindow(hwndFinish, TRUE);

    /* Set the Finish button as the default pushbutton  */
    SendMessageA(hwndDlg, DM_SETDEFID, IDC_FINISH_BUTTON, 0);
  }
}

/******************************************************************************
 *            PROPSHEET_GetPageIndex
 *
 * Given a HPROPSHEETPAGE, returns the index of the corresponding page from
 * the array of PropPageInfo.
 */
static int PROPSHEET_GetPageIndex(HPROPSHEETPAGE hpage, PropSheetInfo* psInfo)
{
  BOOL found = FALSE;
  int index = 0;

  TRACE("hpage %p\n", hpage);
  while ((index < psInfo->nPages) && (found == FALSE))
  {
    if (psInfo->proppage[index].hpage == hpage)
      found = TRUE;
    else
      index++;
  }

  if (found == FALSE)
    index = -1;

  return index;
}

/******************************************************************************
 *            PROPSHEET_CleanUp
 */
static void PROPSHEET_CleanUp(HWND hwndDlg)
{
  int i;
  PropSheetInfo* psInfo = (PropSheetInfo*) RemovePropW(hwndDlg,
                                                       PropSheetInfoStr);

  TRACE("\n");
  if (!psInfo) return;
  if (HIWORD(psInfo->ppshheader.pszCaption))
      HeapFree(GetProcessHeap(), 0, (LPVOID)psInfo->ppshheader.pszCaption);

  for (i = 0; i < psInfo->nPages; i++)
  {
     PROPSHEETPAGEA* psp = (PROPSHEETPAGEA*)psInfo->proppage[i].hpage;

     if(psInfo->proppage[i].hwndPage)
        DestroyWindow(psInfo->proppage[i].hwndPage);

     if(psp)
     {
        if ((psp->dwFlags & PSP_USETITLE) && psInfo->proppage[i].pszText)
           HeapFree(GetProcessHeap(), 0, (LPVOID)psInfo->proppage[i].pszText);

        DestroyPropertySheetPage(psInfo->proppage[i].hpage);
     }
  }

  Free(psInfo->proppage);
  Free(psInfo->strPropertiesFor);
  ImageList_Destroy(psInfo->hImageList);

  GlobalFree((HGLOBAL)psInfo);
}

/******************************************************************************
 *            PropertySheet    (COMCTL32.@)
 *            PropertySheetA   (COMCTL32.@)
 */
INT WINAPI PropertySheetA(LPCPROPSHEETHEADERA lppsh)
{
  int bRet = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GlobalAlloc(GPTR,
                                                       sizeof(PropSheetInfo));
  UINT i, n;
  BYTE* pByte;

  TRACE("(%p)\n", lppsh);

  PROPSHEET_CollectSheetInfoA(lppsh, psInfo);

  psInfo->proppage = (PropPageInfo*) Alloc(sizeof(PropPageInfo) *
                                                    lppsh->nPages);
  pByte = (BYTE*) psInfo->ppshheader.u3.ppsp;

  for (n = i = 0; i < lppsh->nPages; i++, n++)
  {
    if (!(lppsh->dwFlags & PSH_PROPSHEETPAGE))
      psInfo->proppage[n].hpage = psInfo->ppshheader.u3.phpage[i];
    else
    {
       psInfo->proppage[n].hpage = CreatePropertySheetPageA((LPCPROPSHEETPAGEA)pByte);
       pByte += ((LPPROPSHEETPAGEA)pByte)->dwSize;
    }

    if (!PROPSHEET_CollectPageInfo((LPCPROPSHEETPAGEW)psInfo->proppage[n].hpage,
                               psInfo, n))
    {
	if (lppsh->dwFlags & PSH_PROPSHEETPAGE)
	    DestroyPropertySheetPage(psInfo->proppage[n].hpage);
	n--;
	psInfo->nPages--;
    }
  }

  psInfo->unicode = FALSE;
  bRet = PROPSHEET_CreateDialog(psInfo);

  return bRet;
}

/******************************************************************************
 *            PropertySheetW   (COMCTL32.@)
 */
INT WINAPI PropertySheetW(LPCPROPSHEETHEADERW lppsh)
{
  int bRet = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GlobalAlloc(GPTR,
                                                       sizeof(PropSheetInfo));
  UINT i, n;
  BYTE* pByte;

  TRACE("(%p)\n", lppsh);

  PROPSHEET_CollectSheetInfoW(lppsh, psInfo);

  psInfo->proppage = (PropPageInfo*) Alloc(sizeof(PropPageInfo) *
                                                    lppsh->nPages);
  pByte = (BYTE*) psInfo->ppshheader.u3.ppsp;

  for (n = i = 0; i < lppsh->nPages; i++, n++)
  {
    if (!(lppsh->dwFlags & PSH_PROPSHEETPAGE))
      psInfo->proppage[n].hpage = psInfo->ppshheader.u3.phpage[i];
    else
    {
       psInfo->proppage[n].hpage = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)pByte);
       pByte += ((LPPROPSHEETPAGEW)pByte)->dwSize;
    }

    if (!PROPSHEET_CollectPageInfo((LPCPROPSHEETPAGEW)psInfo->proppage[n].hpage,
                               psInfo, n))
    {
	if (lppsh->dwFlags & PSH_PROPSHEETPAGE)
	    DestroyPropertySheetPage(psInfo->proppage[n].hpage);
	n--;
	psInfo->nPages--;
    }
  }

  psInfo->unicode = TRUE;
  bRet = PROPSHEET_CreateDialog(psInfo);

  return bRet;
}

/******************************************************************************
 *            CreatePropertySheetPage    (COMCTL32.@)
 *            CreatePropertySheetPageA   (COMCTL32.@)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(
                          LPCPROPSHEETPAGEA lpPropSheetPage)
{
  PROPSHEETPAGEW* ppsp = Alloc(sizeof(PROPSHEETPAGEW));

  memcpy(ppsp,lpPropSheetPage,min(lpPropSheetPage->dwSize,sizeof(PROPSHEETPAGEA)));

  ppsp->dwFlags &= ~ PSP_INTERNAL_UNICODE;
  if ( !(ppsp->dwFlags & PSP_DLGINDIRECT) && HIWORD( ppsp->u.pszTemplate ) )
  {
     int len = strlen(lpPropSheetPage->u.pszTemplate);

     ppsp->u.pszTemplate = HeapAlloc( GetProcessHeap(),0,len+1 );
     strcpy( (LPSTR)ppsp->u.pszTemplate, lpPropSheetPage->u.pszTemplate );
  }
  if ( (ppsp->dwFlags & PSP_USEICONID) && HIWORD( ppsp->u2.pszIcon ) )
  {
      PROPSHEET_AtoW(&ppsp->u2.pszIcon, lpPropSheetPage->u2.pszIcon);
  }

  if ((ppsp->dwFlags & PSP_USETITLE) && HIWORD( ppsp->pszTitle ))
  {
      PROPSHEET_AtoW(&ppsp->pszTitle, lpPropSheetPage->pszTitle);
  }
  else if ( !(ppsp->dwFlags & PSP_USETITLE) )
      ppsp->pszTitle = NULL;

  return (HPROPSHEETPAGE)ppsp;
}

/******************************************************************************
 *            CreatePropertySheetPageW   (COMCTL32.@)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW lpPropSheetPage)
{
  PROPSHEETPAGEW* ppsp = Alloc(sizeof(PROPSHEETPAGEW));

  memcpy(ppsp,lpPropSheetPage,min(lpPropSheetPage->dwSize,sizeof(PROPSHEETPAGEW)));

  ppsp->dwFlags |= PSP_INTERNAL_UNICODE;

  if ( !(ppsp->dwFlags & PSP_DLGINDIRECT) && HIWORD( ppsp->u.pszTemplate ) )
  {
    int len = strlenW(lpPropSheetPage->u.pszTemplate);

    ppsp->u.pszTemplate = HeapAlloc( GetProcessHeap(),0,(len+1)*sizeof (WCHAR) );
    strcpyW( (WCHAR *)ppsp->u.pszTemplate, lpPropSheetPage->u.pszTemplate );
  }
  if ( (ppsp->dwFlags & PSP_USEICONID) && HIWORD( ppsp->u2.pszIcon ) )
  {
      int len = strlenW(lpPropSheetPage->u2.pszIcon);
      ppsp->u2.pszIcon = HeapAlloc( GetProcessHeap(), 0, (len+1)*sizeof (WCHAR) );
      strcpyW( (WCHAR *)ppsp->u2.pszIcon, lpPropSheetPage->u2.pszIcon );
  }

  if ((ppsp->dwFlags & PSP_USETITLE) && HIWORD( ppsp->pszTitle ))
  {
      int len = strlenW(lpPropSheetPage->pszTitle);
      ppsp->pszTitle = HeapAlloc( GetProcessHeap(), 0, (len+1)*sizeof (WCHAR) );
      strcpyW( (WCHAR *)ppsp->pszTitle, lpPropSheetPage->pszTitle );
  }
  else if ( !(ppsp->dwFlags & PSP_USETITLE) )
      ppsp->pszTitle = NULL;

  return (HPROPSHEETPAGE)ppsp;
}

/******************************************************************************
 *            DestroyPropertySheetPage   (COMCTL32.@)
 */
BOOL WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE hPropPage)
{
  PROPSHEETPAGEW *psp = (PROPSHEETPAGEW *)hPropPage;

  if (!psp)
     return FALSE;

  if ( !(psp->dwFlags & PSP_DLGINDIRECT) && HIWORD( psp->u.pszTemplate ) )
     HeapFree(GetProcessHeap(), 0, (LPVOID)psp->u.pszTemplate);

  if ( (psp->dwFlags & PSP_USEICONID) && HIWORD( psp->u2.pszIcon ) )
     HeapFree(GetProcessHeap(), 0, (LPVOID)psp->u2.pszIcon);

  if ((psp->dwFlags & PSP_USETITLE) && HIWORD( psp->pszTitle ))
      HeapFree(GetProcessHeap(), 0, (LPVOID)psp->pszTitle);

  Free(hPropPage);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_IsDialogMessage
 */
static BOOL PROPSHEET_IsDialogMessage(HWND hwnd, LPMSG lpMsg)
{
   PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwnd, PropSheetInfoStr);

   TRACE("\n");
   if (!psInfo || (hwnd != lpMsg->hwnd && !IsChild(hwnd, lpMsg->hwnd)))
      return FALSE;

   if (lpMsg->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000))
   {
      int new_page = 0;
      INT dlgCode = SendMessageA(lpMsg->hwnd, WM_GETDLGCODE, 0, (LPARAM)lpMsg);

      if (!(dlgCode & DLGC_WANTMESSAGE))
      {
         switch (lpMsg->wParam)
         {
            case VK_TAB:
               if (GetKeyState(VK_SHIFT) & 0x8000)
                   new_page = -1;
                else
                   new_page = 1;
               break;

            case VK_NEXT:   new_page = 1;  break;
            case VK_PRIOR:  new_page = -1; break;
         }
      }

      if (new_page)
      {
         if (PROPSHEET_CanSetCurSel(hwnd) != FALSE)
         {
            new_page += psInfo->active_page;

            if (new_page < 0)
               new_page = psInfo->nPages - 1;
            else if (new_page >= psInfo->nPages)
               new_page = 0;

            PROPSHEET_SetCurSel(hwnd, new_page, 1, 0);
         }

         return TRUE;
      }
   }

   return IsDialogMessageA(hwnd, lpMsg);
}

/******************************************************************************
 *            PROPSHEET_DoCommand
 */
static BOOL PROPSHEET_DoCommand(HWND hwnd, WORD wID)
{

    switch (wID) {

    case IDOK:
    case IDC_APPLY_BUTTON:
	{
	    HWND hwndApplyBtn = GetDlgItem(hwnd, IDC_APPLY_BUTTON);

	    if (PROPSHEET_Apply(hwnd, wID == IDOK ? 1: 0) == FALSE)
		break;

	    if (wID == IDOK)
		{
		    PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwnd,
								      PropSheetInfoStr);
		    int result = TRUE;

		    if (psInfo->restartWindows)
			result = ID_PSRESTARTWINDOWS;

		    /* reboot system takes precedence over restart windows */
		    if (psInfo->rebootSystem)
			result = ID_PSREBOOTSYSTEM;

		    if (psInfo->isModeless)
			psInfo->activeValid = FALSE;
		    else
			EndDialog(hwnd, result);
		}
	    else
		EnableWindow(hwndApplyBtn, FALSE);

	    break;
	}

    case IDC_BACK_BUTTON:
	PROPSHEET_Back(hwnd);
	break;

    case IDC_NEXT_BUTTON:
	PROPSHEET_Next(hwnd);
	break;

    case IDC_FINISH_BUTTON:
	PROPSHEET_Finish(hwnd);
	break;

    case IDCANCEL:
	PROPSHEET_Cancel(hwnd, 0);
	break;

    case IDHELP:
	PROPSHEET_Help(hwnd);
	break;

    default:
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 *            PROPSHEET_DialogProc
 */
INT_PTR CALLBACK
PROPSHEET_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  TRACE("hwnd=%p msg=0x%04x wparam=%x lparam=%lx\n",
	hwnd, uMsg, wParam, lParam);

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) lParam;
      WCHAR* strCaption = (WCHAR*)Alloc(MAX_CAPTION_LENGTH*sizeof(WCHAR));
      HWND hwndTabCtrl = GetDlgItem(hwnd, IDC_TABCONTROL);
      LPCPROPSHEETPAGEW ppshpage;
      int idx;

      SetPropW(hwnd, PropSheetInfoStr, (HANDLE)psInfo);

      /*
       * psInfo->hwnd is not being used by WINE code - it exists
       * for compatibility with "real" Windoze. The same about
       * SetWindowLong - WINE is only using the PropSheetInfoStr
       * property.
       */
      psInfo->hwnd = hwnd;
      SetWindowLongW(hwnd,DWL_USER,(LONG)psInfo);

      /* set up the Next and Back buttons by default */
      PROPSHEET_SetWizButtons(hwnd, PSWIZB_BACK|PSWIZB_NEXT);

      /*
       * Small icon in the title bar.
       */
      if ((psInfo->ppshheader.dwFlags & PSH_USEICONID) ||
          (psInfo->ppshheader.dwFlags & PSH_USEHICON))
      {
        HICON hIcon;
        int icon_cx = GetSystemMetrics(SM_CXSMICON);
        int icon_cy = GetSystemMetrics(SM_CYSMICON);

        if (psInfo->ppshheader.dwFlags & PSH_USEICONID)
          hIcon = LoadImageW(psInfo->ppshheader.hInstance,
                             psInfo->ppshheader.u.pszIcon,
                             IMAGE_ICON,
                             icon_cx, icon_cy,
                             LR_DEFAULTCOLOR);
        else
          hIcon = psInfo->ppshheader.u.hIcon;

        SendMessageW(hwnd, WM_SETICON, 0, (LPARAM)hIcon);
      }

      if (psInfo->ppshheader.dwFlags & PSH_USEHICON)
        SendMessageW(hwnd, WM_SETICON, 0, (LPARAM)psInfo->ppshheader.u.hIcon);

      psInfo->strPropertiesFor = strCaption;

      GetWindowTextW(hwnd, psInfo->strPropertiesFor, MAX_CAPTION_LENGTH);

      PROPSHEET_CreateTabControl(hwnd, psInfo);

      if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
      {
        if (PROPSHEET_IsTooSmallWizard(hwnd, psInfo))
        {
          PROPSHEET_AdjustSizeWizard(hwnd, psInfo);
          PROPSHEET_AdjustButtonsWizard(hwnd, psInfo);
        }
      }
      else
      {
        if (PROPSHEET_SizeMismatch(hwnd, psInfo))
        {
          PROPSHEET_AdjustSize(hwnd, psInfo);
          PROPSHEET_AdjustButtons(hwnd, psInfo);
        }
      }

      if (psInfo->useCallback)
             (*(psInfo->ppshheader.pfnCallback))(hwnd,
					      PSCB_INITIALIZED, (LPARAM)0);

      idx = psInfo->active_page;
      ppshpage = (LPCPROPSHEETPAGEW)psInfo->proppage[idx].hpage;
      psInfo->active_page = -1;

      PROPSHEET_SetCurSel(hwnd, idx, 1, psInfo->proppage[idx].hpage);

      /* doing TCM_SETCURSEL seems to be needed even in case of PSH_WIZARD,
       * as some programs call TCM_GETCURSEL to get the current selection
       * from which to switch to the next page */
      SendMessageW(hwndTabCtrl, TCM_SETCURSEL, psInfo->active_page, 0);

      if (!HIWORD(psInfo->ppshheader.pszCaption) &&
              psInfo->ppshheader.hInstance)
      {
         WCHAR szText[256];

         if (LoadStringW(psInfo->ppshheader.hInstance,
                 (UINT)psInfo->ppshheader.pszCaption, szText, 255))
            PROPSHEET_SetTitleW(hwnd, psInfo->ppshheader.dwFlags, szText);
      }
      else
      {
         PROPSHEET_SetTitleW(hwnd, psInfo->ppshheader.dwFlags,
                         psInfo->ppshheader.pszCaption);
      }

      return TRUE;
    }

    case WM_DESTROY:
      PROPSHEET_CleanUp(hwnd);
      return TRUE;

    case WM_CLOSE:
      PROPSHEET_Cancel(hwnd, 1);
      return TRUE;

    case WM_COMMAND:
      if (!PROPSHEET_DoCommand(hwnd, LOWORD(wParam)))
      {
          PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwnd, PropSheetInfoStr);

          /* No default handler, forward notification to active page */
          if (psInfo->activeValid && psInfo->active_page != -1)
          {
             HWND hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
             SendMessageW(hwndPage, WM_COMMAND, wParam, lParam);
          }
      }
      return TRUE;

    case WM_NOTIFY:
    {
      NMHDR* pnmh = (LPNMHDR) lParam;

      if (pnmh->code == TCN_SELCHANGE)
      {
        int index = SendMessageW(pnmh->hwndFrom, TCM_GETCURSEL, 0, 0);
        PROPSHEET_SetCurSel(hwnd, index, 1, 0);
      }

      if(pnmh->code == TCN_SELCHANGING)
      {
        BOOL bRet = PROPSHEET_CanSetCurSel(hwnd);
        SetWindowLongW(hwnd, DWL_MSGRESULT, !bRet);
        return TRUE;
      }

      return FALSE;
    }

    case PSM_GETCURRENTPAGEHWND:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwnd,
                                                        PropSheetInfoStr);
      HWND hwndPage = 0;

      if (psInfo->activeValid && psInfo->active_page != -1)
        hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

      SetWindowLongW(hwnd, DWL_MSGRESULT, (LONG)hwndPage);

      return TRUE;
    }

    case PSM_CHANGED:
      PROPSHEET_Changed(hwnd, (HWND)wParam);
      return TRUE;

    case PSM_UNCHANGED:
      PROPSHEET_UnChanged(hwnd, (HWND)wParam);
      return TRUE;

    case PSM_GETTABCONTROL:
    {
      HWND hwndTabCtrl = GetDlgItem(hwnd, IDC_TABCONTROL);

      SetWindowLongW(hwnd, DWL_MSGRESULT, (LONG)hwndTabCtrl);

      return TRUE;
    }

    case PSM_SETCURSEL:
    {
      BOOL msgResult;

      msgResult = PROPSHEET_CanSetCurSel(hwnd);
      if(msgResult != FALSE)
      {
        msgResult = PROPSHEET_SetCurSel(hwnd,
                                       (int)wParam,
				       1,
                                       (HPROPSHEETPAGE)lParam);
      }

      SetWindowLongW(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_CANCELTOCLOSE:
    {
      WCHAR buf[MAX_BUTTONTEXT_LENGTH];
      HWND hwndOK = GetDlgItem(hwnd, IDOK);
      HWND hwndCancel = GetDlgItem(hwnd, IDCANCEL);

      EnableWindow(hwndCancel, FALSE);
      if (LoadStringW(COMCTL32_hModule, IDS_CLOSE, buf, sizeof(buf)))
         SetWindowTextW(hwndOK, buf);

      return FALSE;
    }

    case PSM_RESTARTWINDOWS:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwnd,
                                                        PropSheetInfoStr);

      psInfo->restartWindows = TRUE;
      return TRUE;
    }

    case PSM_REBOOTSYSTEM:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) GetPropW(hwnd,
                                                        PropSheetInfoStr);

      psInfo->rebootSystem = TRUE;
      return TRUE;
    }

    case PSM_SETTITLEA:
      PROPSHEET_SetTitleA(hwnd, (DWORD) wParam, (LPCSTR) lParam);
      return TRUE;

    case PSM_SETTITLEW:
      PROPSHEET_SetTitleW(hwnd, (DWORD) wParam, (LPCWSTR) lParam);
      return TRUE;

    case PSM_APPLY:
    {
      BOOL msgResult = PROPSHEET_Apply(hwnd, 0);

      SetWindowLongW(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_QUERYSIBLINGS:
    {
      LRESULT msgResult = PROPSHEET_QuerySiblings(hwnd, wParam, lParam);

      SetWindowLongW(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_ADDPAGE:
    {
      /*
       * Note: MSVC++ 6.0 documentation says that PSM_ADDPAGE does not have
       *       a return value. This is not true. PSM_ADDPAGE returns TRUE
       *       on success or FALSE otherwise, as specified on MSDN Online.
       *       Also see the MFC code for
       *       CPropertySheet::AddPage(CPropertyPage* pPage).
       */

      BOOL msgResult = PROPSHEET_AddPage(hwnd, (HPROPSHEETPAGE)lParam);

      SetWindowLongW(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_REMOVEPAGE:
      PROPSHEET_RemovePage(hwnd, (int)wParam, (HPROPSHEETPAGE)lParam);
      return TRUE;

    case PSM_ISDIALOGMESSAGE:
    {
       BOOL msgResult = PROPSHEET_IsDialogMessage(hwnd, (LPMSG)lParam);
       SetWindowLongA(hwnd, DWL_MSGRESULT, msgResult);
       return TRUE;
    }

    case PSM_PRESSBUTTON:
      PROPSHEET_PressButton(hwnd, (int)wParam);
      return TRUE;

    case PSM_SETFINISHTEXTA:
      PROPSHEET_SetFinishTextA(hwnd, (LPCSTR) lParam);
      return TRUE;

    case PSM_SETWIZBUTTONS:
      PROPSHEET_SetWizButtons(hwnd, (DWORD)lParam);
      return TRUE;

    case PSM_SETCURSELID:
        PROPSHEET_SetCurSelId(hwnd, (int)lParam);
        return TRUE;

    case PSM_SETFINISHTEXTW:
        PROPSHEET_SetFinishTextW(hwnd, (LPCWSTR) lParam);
        return FALSE;

    default:
      return FALSE;
  }

  return FALSE;
}
