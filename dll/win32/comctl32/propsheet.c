/*
 * Property Sheets
 *
 * Copyright 1998 Francis Beaudet
 * Copyright 1999 Thuy Nguyen
 * Copyright 2004 Maxime Bellenge
 * Copyright 2004 Filip Navara
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
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 12, 2004, by Filip Navara.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 *
 * TODO:
 *   - Tab order
 *   - Wizard 97 header resizing
 *   - Enforcing of minimal wizard size
 *   - Messages:
 *     o PSM_RECALCPAGESIZES
 *     o WM_HELP
 *     o WM_CONTEXTMENU
 *   - Notifications:
 *     o PSN_GETOBJECT
 *     o PSN_QUERYINITIALFOCUS
 *     o PSN_TRANSLATEACCELERATOR
 *   - Styles:
 *     o PSH_RTLREADING
 *     o PSH_STRETCHWATERMARK
 *     o PSH_USEPAGELANG
 *     o PSH_USEPSTARTPAGE
 *   - Page styles:
 *     o PSP_USEFUSIONCONTEXT
 *     o PSP_USEREFPARENT
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "prsht.h"
#include "comctl32.h"
#include "uxtheme.h"

#include "wine/debug.h"

#define HPROPSHEETPAGE_MAGIC 0x5A9234E3

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

struct _PSP
{
    DWORD magic;
    BOOL unicode;
    union
    {
        PROPSHEETPAGEA pspA;
        PROPSHEETPAGEW pspW;
        BYTE data[1];
    };
};

typedef struct tagPropPageInfo
{
  HPROPSHEETPAGE hpage; /* to keep track of pages not passed to PropertySheet */
  HWND hwndPage;
  BOOL isDirty;
  LPCWSTR pszText;
  BOOL hasHelp;
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
  BOOL hasFinish;
  BOOL usePropPage;
  BOOL useCallback;
  BOOL activeValid;
  PropPageInfo* proppage;
  HFONT hFont;
  HFONT hFontBold;
  int width;
  int height;
  HIMAGELIST hImageList;
  BOOL ended;
  INT result;
} PropSheetInfo;

typedef struct
{
  int x;
  int y;
} PADDING_INFO;

/******************************************************************************
 * Defines and global variables
 */

static const WCHAR PropSheetInfoStr[] = L"PropertySheetInfo";

#define MAX_CAPTION_LENGTH 255
#define MAX_TABTEXT_LENGTH 255
#define MAX_BUTTONTEXT_LENGTH 64

#define INTRNL_ANY_WIZARD (PSH_WIZARD | PSH_WIZARD97_OLD | PSH_WIZARD97_NEW | PSH_WIZARD_LITE)

/* Wizard metrics specified in DLUs */
#define WIZARD_PADDING 7
#define WIZARD_HEADER_HEIGHT 36
                         	
/******************************************************************************
 * Prototypes
 */
static PADDING_INFO PROPSHEET_GetPaddingInfo(HWND hwndDlg);
static void PROPSHEET_SetTitleW(HWND hwndDlg, DWORD dwStyle, LPCWSTR lpszText);
static BOOL PROPSHEET_CanSetCurSel(HWND hwndDlg);
static BOOL PROPSHEET_SetCurSel(HWND hwndDlg,
                                int index,
                                int skipdir,
                                HPROPSHEETPAGE hpage);
static int PROPSHEET_GetPageIndex(HPROPSHEETPAGE hpage, const PropSheetInfo* psInfo, int original_index);
static PADDING_INFO PROPSHEET_GetPaddingInfoWizard(HWND hwndDlg, const PropSheetInfo* psInfo);
static BOOL PROPSHEET_DoCommand(HWND hwnd, WORD wID);
static BOOL PROPSHEET_RemovePage(HWND hwndDlg, int index, HPROPSHEETPAGE hpage);

static INT_PTR CALLBACK
PROPSHEET_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

WINE_DEFAULT_DEBUG_CHANNEL(propsheet);

static char *heap_strdupA(const char *str)
{
    int len = strlen(str) + 1;
    char *ret = Alloc(len);
    return strcpy(ret, str);
}

static WCHAR *heap_strdupW(const WCHAR *str)
{
    int len = lstrlenW(str) + 1;
    WCHAR *ret = Alloc(len * sizeof(WCHAR));
    lstrcpyW(ret, str);
    return ret;
}

static WCHAR *heap_strdupAtoW(const char *str)
{
    WCHAR *ret;
    INT len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, 0, 0);
    ret = Alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static char *heap_strdupWtoA(const WCHAR *str)
{
    char *ret;
    INT len;

    len = WideCharToMultiByte(CP_ACP, 0, str, -1, 0, 0, 0, 0);
    ret = Alloc(len);
    WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, 0, 0);

    return ret;
}

/*
 * Get the size of an in-memory template
 *
 *( Based on the code of PROPSHEET_CollectPageInfo)
 * See also dialog.c/DIALOG_ParseTemplate32().
 */

static UINT get_template_size(const DLGTEMPLATE *template)
{
    const WORD *p = (const WORD *)template;
    BOOL istemplateex = ((const MyDLGTEMPLATEEX *)template)->signature == 0xFFFF;
    WORD nitems;
    UINT ret;

    if (istemplateex)
    {
        /* DLGTEMPLATEEX (not defined in any std. header file) */
        TRACE("is DLGTEMPLATEEX\n");
        p++;       /* dlgVer */
        p++;       /* signature */
        p += 2;    /* help ID */
        p += 2;    /* ext style */
        p += 2;    /* style */
    }
    else
    {
        /* DLGTEMPLATE */
        TRACE("is DLGTEMPLATE\n");
        p += 2;    /* style */
        p += 2;    /* ext style */
    }

    nitems = *p;
    p++;    /* nb items */
    p++;    /* x */
    p++;    /* y */
    p++;    /* width */
    p++;    /* height */

    /* menu */
    switch (*p)
    {
    case 0x0000:
        p++;
        break;
    case 0xffff:
        p += 2;
        break;
    default:
        TRACE("menu %s\n", debugstr_w( p ));
        p += lstrlenW( p ) + 1;
        break;
    }

    /* class */
    switch (*p)
    {
    case 0x0000:
        p++;
        break;
    case 0xffff:
        p += 2; /* 0xffff plus predefined window class ordinal value */
        break;
    default:
        TRACE("class %s\n", debugstr_w( p ));
        p += lstrlenW( p ) + 1;
        break;
    }

    /* title */
    TRACE("title %s\n", debugstr_w( p ));
    p += lstrlenW( p ) + 1;

    /* font, if DS_SETFONT set */
    if ((DS_SETFONT & ((istemplateex) ? ((const MyDLGTEMPLATEEX *)template)->style :
                    template->style)))
    {
        p += istemplateex ? 3 : 1;
        TRACE("font %s\n", debugstr_w( p ));
        p += lstrlenW( p ) + 1; /* the font name */
    }

    /* now process the DLGITEMTEMPLATE(EX) structs (plus custom data)
     * that are following the DLGTEMPLATE(EX) data */
    TRACE("%d items\n", nitems);
    while (nitems > 0)
    {
        p = (WORD*)(((DWORD_PTR)p + 3) & ~3); /* DWORD align */

        /* skip header */
        p += (istemplateex ? sizeof(MyDLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE))
            / sizeof(WORD);

        /* check class */
        switch (*p)
        {
        case 0x0000:
            p++;
            break;
        case 0xffff:
            TRACE("class ordinal %#lx\n", *(const DWORD *)p);
            p += 2;
            break;
        default:
            TRACE("class %s\n", debugstr_w( p ));
            p += lstrlenW( p ) + 1;
            break;
        }

        /* check title text */
        switch (*p)
        {
        case 0x0000:
            p++;
            break;
        case 0xffff:
            TRACE("text ordinal %#lx\n",*(const DWORD *)p);
            p += 2;
            break;
        default:
            TRACE("text %s\n",debugstr_w( p ));
            p += lstrlenW( p ) + 1;
            break;
        }
        p += *p / sizeof(WORD) + 1;    /* Skip extra data */
        --nitems;
    }

    ret = (p - (const WORD *)template) * sizeof(WORD);
    TRACE("%p %p size 0x%08x\n", p, template, ret);
    return ret;
}

static DWORD HPSP_get_flags(HPROPSHEETPAGE hpsp)
{
    if (!hpsp) return 0;
    return hpsp->unicode ? hpsp->pspW.dwFlags : hpsp->pspA.dwFlags;
}

static void HPSP_call_callback(HPROPSHEETPAGE hpsp, UINT msg)
{
    if (hpsp->unicode)
    {
        if (!(hpsp->pspW.dwFlags & PSP_USECALLBACK) || !hpsp->pspW.pfnCallback ||
                (msg == PSPCB_ADDREF && hpsp->pspW.dwSize <= PROPSHEETPAGEW_V1_SIZE))
            return;

        hpsp->pspW.pfnCallback(0, msg, &hpsp->pspW);
    }
    else
    {
        if (!(hpsp->pspA.dwFlags & PSP_USECALLBACK) || !hpsp->pspA.pfnCallback ||
                (msg == PSPCB_ADDREF && hpsp->pspA.dwSize <= PROPSHEETPAGEA_V1_SIZE))
            return;

        hpsp->pspA.pfnCallback(0, msg, &hpsp->pspA);
    }
}

static const DLGTEMPLATE* HPSP_load_template(HPROPSHEETPAGE hpsp, DWORD *size)
{
    HGLOBAL template;
    HINSTANCE hinst;
    HRSRC res;

    if (hpsp->unicode)
    {
        if (hpsp->pspW.dwFlags & PSP_DLGINDIRECT)
        {
            if (size)
                *size = get_template_size(hpsp->pspW.pResource);
            return hpsp->pspW.pResource;
        }

        hinst = hpsp->pspW.hInstance;
        res = FindResourceW(hinst, hpsp->pspW.pszTemplate, (LPWSTR)RT_DIALOG);
    }
    else
    {
        if (hpsp->pspA.dwFlags & PSP_DLGINDIRECT)
        {
            if (size)
                *size = get_template_size(hpsp->pspA.pResource);
            return hpsp->pspA.pResource;
        }

        hinst = hpsp->pspA.hInstance;
        res = FindResourceA(hinst, hpsp->pspA.pszTemplate, (LPSTR)RT_DIALOG);
    }

    if (size)
        *size = SizeofResource(hinst, res);

    template = LoadResource(hinst, res);
    return LockResource(template);
}

static WCHAR* HPSP_get_title(HPROPSHEETPAGE hpsp, const WCHAR *template_title)
{
    const WCHAR *pTitle;
    WCHAR szTitle[256];
    const void *title;
    HINSTANCE hinst;

    if (hpsp->unicode)
    {
        title = hpsp->pspW.pszTitle;
        hinst = hpsp->pspW.hInstance;
    }
    else
    {
        title = hpsp->pspA.pszTitle;
        hinst = hpsp->pspA.hInstance;
    }

    if (IS_INTRESOURCE(title))
    {
        if (LoadStringW(hinst, (DWORD_PTR)title, szTitle, ARRAY_SIZE(szTitle)))
            pTitle = szTitle;
        else if (*template_title)
            pTitle = template_title;
        else
            pTitle = L"(null)";

        return heap_strdupW(pTitle);
    }

    if (hpsp->unicode)
        return heap_strdupW(title);
    return heap_strdupAtoW(title);
}

static HICON HPSP_get_icon(HPROPSHEETPAGE hpsp)
{
    HICON ret;

    if (hpsp->unicode)
    {
        if (hpsp->pspW.dwFlags & PSP_USEICONID)
        {
            int cx = GetSystemMetrics(SM_CXSMICON);
            int cy = GetSystemMetrics(SM_CYSMICON);

            ret = LoadImageW(hpsp->pspW.hInstance, hpsp->pspW.pszIcon, IMAGE_ICON,
                    cx, cy, LR_DEFAULTCOLOR);
        }
        else
        {
            ret = hpsp->pspW.hIcon;
        }
    }
    else
    {
        if (hpsp->pspA.dwFlags & PSP_USEICONID)
        {
            int cx = GetSystemMetrics(SM_CXSMICON);
            int cy = GetSystemMetrics(SM_CYSMICON);

            ret = LoadImageA(hpsp->pspA.hInstance, hpsp->pspA.pszIcon, IMAGE_ICON,
                    cx, cy, LR_DEFAULTCOLOR);
        }
        else
        {
            ret = hpsp->pspA.hIcon;
        }
    }

    return ret;
}

static LRESULT HPSP_get_template(HPROPSHEETPAGE hpsp)
{
    if (hpsp->unicode)
        return (LRESULT)hpsp->pspW.pszTemplate;
    return (LRESULT)hpsp->pspA.pszTemplate;
}

static HWND HPSP_create_page(HPROPSHEETPAGE hpsp, DLGTEMPLATE *template, HWND parent)
{
    HWND hwnd;

    if (hpsp->unicode)
    {
        hwnd = CreateDialogIndirectParamW(hpsp->pspW.hInstance, template,
                parent, hpsp->pspW.pfnDlgProc, (LPARAM)&hpsp->pspW);
    }
    else
    {
        hwnd = CreateDialogIndirectParamA(hpsp->pspA.hInstance, template,
                parent, hpsp->pspA.pfnDlgProc, (LPARAM)&hpsp->pspA);
    }

    return hwnd;
}

static void HPSP_set_header_title(HPROPSHEETPAGE hpsp, const WCHAR *title)
{
    if (hpsp->unicode)
    {
        if (!IS_INTRESOURCE(hpsp->pspW.pszHeaderTitle))
            Free((void *)hpsp->pspW.pszHeaderTitle);

        hpsp->pspW.pszHeaderTitle = heap_strdupW(title);
        hpsp->pspW.dwFlags |= PSP_USEHEADERTITLE;
    }
    else
    {
        if (!IS_INTRESOURCE(hpsp->pspA.pszHeaderTitle))
            Free((void *)hpsp->pspA.pszHeaderTitle);

        hpsp->pspA.pszHeaderTitle = heap_strdupWtoA(title);
        hpsp->pspA.dwFlags |= PSP_USEHEADERTITLE;
    }
}

static void HPSP_set_header_subtitle(HPROPSHEETPAGE hpsp, const WCHAR *subtitle)
{
    if (hpsp->unicode)
    {
        if (!IS_INTRESOURCE(hpsp->pspW.pszHeaderTitle))
            Free((void *)hpsp->pspW.pszHeaderTitle);

        hpsp->pspW.pszHeaderTitle = heap_strdupW(subtitle);
        hpsp->pspW.dwFlags |= PSP_USEHEADERSUBTITLE;
    }
    else
    {
        if (!IS_INTRESOURCE(hpsp->pspA.pszHeaderTitle))
            Free((void *)hpsp->pspA.pszHeaderTitle);

        hpsp->pspA.pszHeaderTitle = heap_strdupWtoA(subtitle);
        hpsp->pspA.dwFlags |= PSP_USEHEADERSUBTITLE;
    }
}

static void HPSP_draw_text(HPROPSHEETPAGE hpsp, HDC hdc, BOOL title, RECT *r, UINT format)
{
    const void *text;

    if (hpsp->unicode)
        text = title ? hpsp->pspW.pszHeaderTitle : hpsp->pspW.pszHeaderSubTitle;
    else
        text = title ? hpsp->pspA.pszHeaderTitle : hpsp->pspA.pszHeaderSubTitle;

    if (IS_INTRESOURCE(text))
    {
        WCHAR buf[256];
        INT len;

        len = LoadStringW(hpsp->unicode ? hpsp->pspW.hInstance : hpsp->pspA.hInstance,
                (UINT_PTR)text, buf, ARRAY_SIZE(buf));
        if (len != 0)
            DrawTextW(hdc, buf, len, r, format);
    }
    else if (hpsp->unicode)
        DrawTextW(hdc, text, -1, r, format);
    else
        DrawTextA(hdc, text, -1, r, format);
}

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
   *  PSH_RTLREADING         0x00000800
   *  PSH_STRETCHWATERMARK   0x00040000
   *  PSH_USEPAGELANG        0x00200000
   */

    add_flag(PSH_RTLREADING);
    add_flag(PSH_STRETCHWATERMARK);
    add_flag(PSH_USEPAGELANG);
    if (string[0] != '\0')
	FIXME("%s\n", string);
}
#undef add_flag

/******************************************************************************
 *            PROPSHEET_GetPageRect
 *
 * Retrieve rect from tab control and map into the dialog for SetWindowPos
 */
static void PROPSHEET_GetPageRect(const PropSheetInfo * psInfo, HWND hwndDlg,
                                  RECT *rc, HPROPSHEETPAGE hpsp)
{
    if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD) {     
        HWND hwndChild;
        RECT r;

        if (((psInfo->ppshheader.dwFlags & (PSH_WIZARD97_NEW | PSH_WIZARD97_OLD)) &&
             (psInfo->ppshheader.dwFlags & PSH_HEADER) &&
             !(HPSP_get_flags(hpsp) & PSP_HIDEHEADER)) ||
            (psInfo->ppshheader.dwFlags & PSH_WIZARD))
        {
            rc->left = rc->top = WIZARD_PADDING;
        }
        else
        {
            rc->left = rc->top = 0;
        }
        rc->right = psInfo->width - rc->left;
        rc->bottom = psInfo->height - rc->top;
        MapDialogRect(hwndDlg, rc);

        if ((psInfo->ppshheader.dwFlags & (PSH_WIZARD97_NEW | PSH_WIZARD97_OLD)) &&
            (psInfo->ppshheader.dwFlags & PSH_HEADER) &&
            !(HPSP_get_flags(hpsp) & PSP_HIDEHEADER))
        {
            hwndChild = GetDlgItem(hwndDlg, IDC_SUNKEN_LINEHEADER);
            GetClientRect(hwndChild, &r);
            MapWindowPoints(hwndChild, hwndDlg, (LPPOINT) &r, 2);
            rc->top += r.bottom + 1;
        }
    } else {
        HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
        GetClientRect(hwndTabCtrl, rc);
        SendMessageW(hwndTabCtrl, TCM_ADJUSTRECT, FALSE, (LPARAM)rc);
        MapWindowPoints(hwndTabCtrl, hwndDlg, (LPPOINT)rc, 2);
    }
}

/******************************************************************************
 *            PROPSHEET_FindPageByResId
 *
 * Find page index corresponding to page resource id.
 */
static INT PROPSHEET_FindPageByResId(const PropSheetInfo * psInfo, LRESULT resId)
{
   INT i;

   for (i = 0; i < psInfo->nPages; i++)
   {
      /* Fixme: if resource ID is a string shall we use strcmp ??? */
      if (HPSP_get_template(psInfo->proppage[i].hpage) == resId)
         break;
   }

   return i;
}

/******************************************************************************
 *            PROPSHEET_CollectSheetInfoCommon
 *
 * Common code for PROPSHEET_CollectSheetInfoA/W
 */
static void PROPSHEET_CollectSheetInfoCommon(PropSheetInfo * psInfo, DWORD dwFlags)
{
  PROPSHEET_UnImplementedFlags(dwFlags);

  psInfo->hasHelp = dwFlags & PSH_HASHELP;
  psInfo->hasApply = !(dwFlags & PSH_NOAPPLYNOW);
  psInfo->hasFinish = dwFlags & PSH_WIZARDHASFINISH;
  psInfo->isModeless = dwFlags & PSH_MODELESS;
  psInfo->usePropPage = dwFlags & PSH_PROPSHEETPAGE;
  if (psInfo->active_page < 0 || psInfo->active_page >= psInfo->nPages)
     psInfo->active_page = 0;

  psInfo->result = 0;
  psInfo->hImageList = 0;
  psInfo->activeValid = FALSE;
}

/******************************************************************************
 *            PROPSHEET_CollectSheetInfoA
 *
 * Collect relevant data.
 */
static void PROPSHEET_CollectSheetInfoA(LPCPROPSHEETHEADERA lppsh,
                                       PropSheetInfo * psInfo)
{
  DWORD dwSize = min(lppsh->dwSize,sizeof(PROPSHEETHEADERA));
  DWORD dwFlags = lppsh->dwFlags;

  psInfo->useCallback = (dwFlags & PSH_USECALLBACK )&& (lppsh->pfnCallback);

  memcpy(&psInfo->ppshheader,lppsh,dwSize);
  TRACE("\n** PROPSHEETHEADER **\ndwSize\t\t%ld\ndwFlags\t\t%#lx\nhwndParent\t%p\nhInstance\t%p\npszCaption\t'%s'\nnPages\t\t%d\npfnCallback\t%p\n",
	lppsh->dwSize, lppsh->dwFlags, lppsh->hwndParent, lppsh->hInstance,
	debugstr_a(lppsh->pszCaption), lppsh->nPages, lppsh->pfnCallback);

  if (lppsh->dwFlags & INTRNL_ANY_WIZARD)
     psInfo->ppshheader.pszCaption = NULL;
  else
  {
     if (!IS_INTRESOURCE(lppsh->pszCaption))
     {
        int len = MultiByteToWideChar(CP_ACP, 0, lppsh->pszCaption, -1, NULL, 0);
        WCHAR *caption = Alloc( len*sizeof (WCHAR) );

        MultiByteToWideChar(CP_ACP, 0, lppsh->pszCaption, -1, caption, len);
        psInfo->ppshheader.pszCaption = caption;
     }
  }
  psInfo->nPages = lppsh->nPages;

  if (dwFlags & PSH_USEPSTARTPAGE)
  {
    TRACE("PSH_USEPSTARTPAGE is on\n");
    psInfo->active_page = 0;
  }
  else
    psInfo->active_page = lppsh->nStartPage;

  PROPSHEET_CollectSheetInfoCommon(psInfo, dwFlags);
}

/******************************************************************************
 *            PROPSHEET_CollectSheetInfoW
 *
 * Collect relevant data.
 */
static void PROPSHEET_CollectSheetInfoW(LPCPROPSHEETHEADERW lppsh,
                                       PropSheetInfo * psInfo)
{
  DWORD dwSize = min(lppsh->dwSize,sizeof(PROPSHEETHEADERW));
  DWORD dwFlags = lppsh->dwFlags;

  psInfo->useCallback = (dwFlags & PSH_USECALLBACK) && (lppsh->pfnCallback);

  memcpy(&psInfo->ppshheader,lppsh,dwSize);
  TRACE("\n** PROPSHEETHEADER **\ndwSize\t\t%ld\ndwFlags\t\t%#lx\nhwndParent\t%p\nhInstance\t%p\npszCaption\t%s\nnPages\t\t%d\npfnCallback\t%p\n",
      lppsh->dwSize, lppsh->dwFlags, lppsh->hwndParent, lppsh->hInstance, debugstr_w(lppsh->pszCaption), lppsh->nPages, lppsh->pfnCallback);

  if (lppsh->dwFlags & INTRNL_ANY_WIZARD)
     psInfo->ppshheader.pszCaption = NULL;
  else
  {
     if (!IS_INTRESOURCE(lppsh->pszCaption))
       psInfo->ppshheader.pszCaption = heap_strdupW( lppsh->pszCaption );
  }
  psInfo->nPages = lppsh->nPages;

  if (dwFlags & PSH_USEPSTARTPAGE)
  {
    TRACE("PSH_USEPSTARTPAGE is on\n");
    psInfo->active_page = 0;
  }
  else
    psInfo->active_page = lppsh->nStartPage;

  PROPSHEET_CollectSheetInfoCommon(psInfo, dwFlags);
}

/******************************************************************************
 *            PROPSHEET_CollectPageInfo
 *
 * Collect property sheet data.
 * With code taken from DIALOG_ParseTemplate32.
 */
static BOOL PROPSHEET_CollectPageInfo(HPROPSHEETPAGE hpsp,
                               PropSheetInfo * psInfo,
                               int index, BOOL resize)
{
  const DLGTEMPLATE* pTemplate;
  const WORD*  p;
  DWORD dwFlags;
  int width, height;

  if (!hpsp)
    return FALSE;

  TRACE("\n");
  psInfo->proppage[index].hpage = hpsp;
  psInfo->proppage[index].hwndPage = 0;
  psInfo->proppage[index].isDirty = FALSE;

  /*
   * Process property page flags.
   */
  dwFlags = HPSP_get_flags(hpsp);
  psInfo->proppage[index].hasHelp = dwFlags & PSP_HASHELP;
  psInfo->proppage[index].hasIcon = dwFlags & (PSP_USEHICON | PSP_USEICONID);

  /* as soon as we have a page with the help flag, set the sheet flag on */
  if (psInfo->proppage[index].hasHelp)
    psInfo->hasHelp = TRUE;

  /*
   * Process page template.
   */
  pTemplate = HPSP_load_template(hpsp, NULL);

  /*
   * Extract the size of the page and the caption.
   */
  if (!pTemplate)
      return FALSE;

  p = (const WORD *)pTemplate;

  if (((const MyDLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF)
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

  if (HPSP_get_flags(hpsp) & (PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE))
    psInfo->ppshheader.dwFlags |= PSH_HEADER;

  /* Special calculation for interior wizard pages so the largest page is
   * calculated correctly. We need to add all the padding and space occupied
   * by the header so the width and height sums up to the whole wizard client
   * area. */
  if ((psInfo->ppshheader.dwFlags & (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW)) &&
      (psInfo->ppshheader.dwFlags & PSH_HEADER) &&
      !(dwFlags & PSP_HIDEHEADER))
  {
      height += 2 * WIZARD_PADDING + WIZARD_HEADER_HEIGHT;
      width += 2 * WIZARD_PADDING;
  }
  if (psInfo->ppshheader.dwFlags & PSH_WIZARD)
  {
      height += 2 * WIZARD_PADDING;
      width += 2 * WIZARD_PADDING;
  }

  /* remember the largest width and height */
  if (resize)
  {
      if (width > psInfo->width)
        psInfo->width = width;

      if (height > psInfo->height)
        psInfo->height = height;
  }

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
      p += lstrlenW( p ) + 1;
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
      p += lstrlenW( p ) + 1;
      break;
  }

  /* Extract the caption */
  psInfo->proppage[index].pszText = p;
  TRACE("Tab %d %s\n",index,debugstr_w( p ));

  if (dwFlags & PSP_USETITLE)
      psInfo->proppage[index].pszText = HPSP_get_title(hpsp, p);

  /*
   * Build the image list for icons
   */
  if ((dwFlags & PSP_USEHICON) || (dwFlags & PSP_USEICONID))
  {
    HICON hIcon;
    int icon_cx = GetSystemMetrics(SM_CXSMICON);
    int icon_cy = GetSystemMetrics(SM_CYSMICON);

    if ((hIcon = HPSP_get_icon(hpsp)))
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
static INT_PTR PROPSHEET_CreateDialog(PropSheetInfo* psInfo)
{
  LRESULT ret;
  LPCVOID template;
  LPVOID temp = 0;
  HRSRC hRes;
  DWORD resSize;
  WORD resID = IDD_PROPSHEET;

  TRACE("(%p)\n", psInfo);
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

  if(!(template = LoadResource(COMCTL32_hModule, hRes)))
    return -1;

  /*
   * Make a copy of the dialog template.
   */
  resSize = SizeofResource(COMCTL32_hModule, hRes);

  temp = Alloc(2 * resSize);

  if (!temp)
    return -1;

  memcpy(temp, template, resSize);

  if (psInfo->ppshheader.dwFlags & PSH_NOCONTEXTHELP)
  {
    if (((MyDLGTEMPLATEEX*)temp)->signature == 0xFFFF)
      ((MyDLGTEMPLATEEX*)temp)->style &= ~DS_CONTEXTHELP;
    else
      ((DLGTEMPLATE*)temp)->style &= ~DS_CONTEXTHELP;
  }
  if ((psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD) &&
      (psInfo->ppshheader.dwFlags & PSH_WIZARDCONTEXTHELP))
  {
    if (((MyDLGTEMPLATEEX*)temp)->signature == 0xFFFF)
      ((MyDLGTEMPLATEEX*)temp)->style |= DS_CONTEXTHELP;
    else
      ((DLGTEMPLATE*)temp)->style |= DS_CONTEXTHELP;
  }

  if (psInfo->useCallback)
    (*(psInfo->ppshheader.pfnCallback))(0, PSCB_PRECREATE, (LPARAM)temp);

  /* NOTE: MSDN states "Returns a positive value if successful, or -1
   * otherwise for modal property sheets.", but this is wrong. The
   * actual return value is either TRUE (success), FALSE (cancel) or
   * -1 (error). */
  if( psInfo->unicode )
  {
    ret = (INT_PTR)CreateDialogIndirectParamW(psInfo->ppshheader.hInstance,
                                          temp, psInfo->ppshheader.hwndParent,
                                          PROPSHEET_DialogProc, (LPARAM)psInfo);
    if ( !ret ) ret = -1;
  }
  else
  {
    ret = (INT_PTR)CreateDialogIndirectParamA(psInfo->ppshheader.hInstance,
                                          temp, psInfo->ppshheader.hwndParent,
                                          PROPSHEET_DialogProc, (LPARAM)psInfo);
    if ( !ret ) ret = -1;
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
static BOOL PROPSHEET_SizeMismatch(HWND hwndDlg, const PropSheetInfo* psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  RECT rcOrigTab, rcPage;

  /*
   * Original tab size.
   */
  GetClientRect(hwndTabCtrl, &rcOrigTab);
  TRACE("orig tab %s\n", wine_dbgstr_rect(&rcOrigTab));

  /*
   * Biggest page size.
   */
  SetRect(&rcPage, 0, 0, psInfo->width, psInfo->height);
  MapDialogRect(hwndDlg, &rcPage);
  TRACE("biggest page %s\n", wine_dbgstr_rect(&rcPage));

  if ( (rcPage.right - rcPage.left) != (rcOrigTab.right - rcOrigTab.left) )
    return TRUE;
  if ( (rcPage.bottom - rcPage.top) != (rcOrigTab.bottom - rcOrigTab.top) )
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
  int buttonHeight;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfo(hwndDlg);
  RECT units;
  LONG style;

  /* Get the height of buttons */
  GetClientRect(hwndButton, &rc);
  buttonHeight = rc.bottom;

  /*
   * Biggest page size.
   */
  SetRect(&rc, 0, 0, psInfo->width, psInfo->height);
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

  rc.right -= rc.left;
  rc.bottom -= rc.top;
  TRACE("setting tab %p, rc (0,0)-(%ld,%ld)\n", hwndTabCtrl, rc.right, rc.bottom);
  SetWindowPos(hwndTabCtrl, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  GetClientRect(hwndTabCtrl, &rc);

  TRACE("tab client rc %s\n", wine_dbgstr_rect(&rc));

  rc.right += (padding.x * 2);
  rc.bottom += buttonHeight + (3 * padding.y);

  style = GetWindowLongW(hwndDlg, GWL_STYLE);
  if (!(style & WS_CHILD))
    AdjustWindowRect(&rc, style, FALSE);

  rc.right -= rc.left;
  rc.bottom -= rc.top;

  /*
   * Resize the property sheet.
   */
  TRACE("setting dialog %p, rc (0,0)-(%ld,%ld)\n", hwndDlg, rc.right, rc.bottom);
  SetWindowPos(hwndDlg, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustSizeWizard
 *
 * Resizes the property sheet to fit the largest page.
 */
static BOOL PROPSHEET_AdjustSizeWizard(HWND hwndDlg, const PropSheetInfo* psInfo)
{
  HWND hwndLine = GetDlgItem(hwndDlg, IDC_SUNKEN_LINE);
  RECT rc, lineRect, dialogRect;

  /* Biggest page size */
  SetRect(&rc, 0, 0, psInfo->width, psInfo->height);
  MapDialogRect(hwndDlg, &rc);

  TRACE("Biggest page %s\n", wine_dbgstr_rect(&rc));

  /* Add space for the buttons row */
  GetWindowRect(hwndLine, &lineRect);
  MapWindowPoints(NULL, hwndDlg, (LPPOINT)&lineRect, 2);
  GetClientRect(hwndDlg, &dialogRect);
  rc.bottom += dialogRect.bottom - lineRect.top - 1;

  /* Convert the client coordinates to window coordinates */
  AdjustWindowRect(&rc, GetWindowLongW(hwndDlg, GWL_STYLE), FALSE);

  /* Resize the property sheet */
  TRACE("setting dialog %p, rc (0,0)-(%ld,%ld)\n", hwndDlg, rc.right, rc.bottom);
  SetWindowPos(hwndDlg, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustButtons
 *
 * Adjusts the buttons' positions.
 */
static BOOL PROPSHEET_AdjustButtons(HWND hwndParent, const PropSheetInfo* psInfo)
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
   * Position OK button and make it default.
   */
  hwndButton = GetDlgItem(hwndParent, IDOK);

  x = rcSheet.right - ((padding.x + buttonWidth) * num_buttons);

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  SendMessageW(hwndParent, DM_SETDEFID, IDOK, 0);


  /*
   * Position Cancel button.
   */
  hwndButton = GetDlgItem(hwndParent, IDCANCEL);

  x += padding.x + buttonWidth;

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Apply button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_APPLY_BUTTON);

  if(psInfo->hasApply)
    x += padding.x + buttonWidth;
  else
    ShowWindow(hwndButton, SW_HIDE);

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  EnableWindow(hwndButton, FALSE);

  /*
   * Position Help button.
   */
  hwndButton = GetDlgItem(hwndParent, IDHELP);

  x += padding.x + buttonWidth;
  SetWindowPos(hwndButton, 0, x, y, 0, 0,
              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  if(!psInfo->hasHelp)
    ShowWindow(hwndButton, SW_HIDE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustButtonsWizard
 *
 * Adjusts the buttons' positions.
 */
static BOOL PROPSHEET_AdjustButtonsWizard(HWND hwndParent,
                                          const PropSheetInfo* psInfo)
{
  HWND hwndButton = GetDlgItem(hwndParent, IDCANCEL);
  HWND hwndLine = GetDlgItem(hwndParent, IDC_SUNKEN_LINE);
  HWND hwndLineHeader = GetDlgItem(hwndParent, IDC_SUNKEN_LINEHEADER);
  RECT rcSheet;
  int x, y;
  int num_buttons = 3;
  int buttonWidth, buttonHeight, lineHeight, lineWidth;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfoWizard(hwndParent, psInfo);

  if (psInfo->hasHelp)
    num_buttons++;
  if (psInfo->hasFinish)
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
   * Position the Back button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_BACK_BUTTON);

  x = rcSheet.right - ((padding.x + buttonWidth) * (num_buttons - 1)) - buttonWidth;

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position the Next button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_NEXT_BUTTON);
  
  x += buttonWidth;
  
  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position the Finish button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_FINISH_BUTTON);
  
  if (psInfo->hasFinish)
    x += padding.x + buttonWidth;

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  if (!psInfo->hasFinish)
    ShowWindow(hwndButton, SW_HIDE);

  /*
   * Position the Cancel button.
   */
  hwndButton = GetDlgItem(hwndParent, IDCANCEL);

  x += padding.x + buttonWidth;

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Help button.
   */
  hwndButton = GetDlgItem(hwndParent, IDHELP);

  if (psInfo->hasHelp)
  {
    x += padding.x + buttonWidth;

    SetWindowPos(hwndButton, 0, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  else
    ShowWindow(hwndButton, SW_HIDE);

  if (psInfo->ppshheader.dwFlags &
      (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW | PSH_WIZARD_LITE)) 
      padding.x = 0;

  /*
   * Position and resize the sunken line.
   */
  x = padding.x;
  y = rcSheet.bottom - ((padding.y * 2) + buttonHeight + lineHeight);

  lineWidth = rcSheet.right - (padding.x * 2);
  SetWindowPos(hwndLine, 0, x, y, lineWidth, 2,
               SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position and resize the header sunken line.
   */
  
  SetWindowPos(hwndLineHeader, 0, 0, 0, rcSheet.right, 2,
	       SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
  if (!(psInfo->ppshheader.dwFlags & (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW)))
      ShowWindow(hwndLineHeader, SW_HIDE);

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
  PADDING_INFO padding;

  GetWindowRect(hwndTab, &rcTab);
  MapWindowPoints( 0, hwndDlg, (POINT *)&rcTab, 2 );

  padding.x = rcTab.left;
  padding.y = rcTab.top;

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
  MapWindowPoints( 0, hwndDlg, (POINT *)&rc, 2 );
  ptButton.x = rc.left;
  ptButton.y = rc.top;

  /* Line */
  hwndControl = GetDlgItem(hwndDlg, IDC_SUNKEN_LINE);
  GetWindowRect(hwndControl, &rc);
  MapWindowPoints( 0, hwndDlg, (POINT *)&rc, 2 );
  ptLine.x = rc.left;
  ptLine.y = rc.bottom;

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
                                       const PropSheetInfo * psInfo)
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

  SendMessageW(hwndTabCtrl, WM_SETREDRAW, 0, 0);
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
    SendMessageW(hwndTabCtrl, TCM_INSERTITEMW, i, (LPARAM)&item);
  }
  SendMessageW(hwndTabCtrl, WM_SETREDRAW, 1, 0);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_WizardSubclassProc
 *
 * Subclassing window procedure for wizard exterior pages to prevent drawing
 * background and so drawing above the watermark.
 */
static LRESULT CALLBACK
PROPSHEET_WizardSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRef)
{
  switch (uMsg)
  {
    case WM_ERASEBKGND:
      return TRUE;

    case WM_CTLCOLORSTATIC:
      SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
      return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
  }

  return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

/******************************************************************************
 *            PROPSHEET_CreatePage
 *
 * Creates a page.
 */
static BOOL PROPSHEET_CreatePage(HWND hwndParent,
                                int index,
                                const PropSheetInfo * psInfo,
                                HPROPSHEETPAGE hpsp)
{
  const DLGTEMPLATE* pTemplate;
  HWND hwndPage;
  DWORD resSize;
  DLGTEMPLATE* pTemplateCopy = NULL;

  TRACE("index %d\n", index);

  if (hpsp == NULL)
  {
    return FALSE;
  }

  pTemplate = HPSP_load_template(hpsp, &resSize);
  pTemplateCopy = Alloc(resSize);
  if (!pTemplateCopy)
    return FALSE;
  
  TRACE("copying pTemplate %p into pTemplateCopy %p (%ld)\n", pTemplate, pTemplateCopy, resSize);
  memcpy(pTemplateCopy, pTemplate, resSize);

  if (((MyDLGTEMPLATEEX*)pTemplateCopy)->signature == 0xFFFF)
  {
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style |= WS_CHILD | WS_TABSTOP | DS_CONTROL;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~DS_MODALFRAME;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~WS_CAPTION;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~WS_SYSMENU;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~WS_POPUP;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~WS_DISABLED;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~WS_VISIBLE;
    ((MyDLGTEMPLATEEX*)pTemplateCopy)->style &= ~WS_THICKFRAME;

    ((MyDLGTEMPLATEEX*)pTemplateCopy)->exStyle |= WS_EX_CONTROLPARENT;
  }
  else
  {
    pTemplateCopy->style |= WS_CHILD | WS_TABSTOP | DS_CONTROL;
    pTemplateCopy->style &= ~DS_MODALFRAME;
    pTemplateCopy->style &= ~WS_CAPTION;
    pTemplateCopy->style &= ~WS_SYSMENU;
    pTemplateCopy->style &= ~WS_POPUP;
    pTemplateCopy->style &= ~WS_DISABLED;
    pTemplateCopy->style &= ~WS_VISIBLE;
    pTemplateCopy->style &= ~WS_THICKFRAME;

    pTemplateCopy->dwExtendedStyle |= WS_EX_CONTROLPARENT;
  }

  HPSP_call_callback(hpsp, PSPCB_CREATE);
  hwndPage = HPSP_create_page(hpsp, pTemplateCopy, hwndParent);
  /* Free a no more needed copy */
  Free(pTemplateCopy);

  if(!hwndPage)
      return FALSE;

  psInfo->proppage[index].hwndPage = hwndPage;

  /* Subclass exterior wizard pages */
  if((psInfo->ppshheader.dwFlags & (PSH_WIZARD97_NEW | PSH_WIZARD97_OLD)) &&
     (psInfo->ppshheader.dwFlags & PSH_WATERMARK) &&
     (HPSP_get_flags(hpsp) & PSP_HIDEHEADER))
  {
      SetWindowSubclass(hwndPage, PROPSHEET_WizardSubclassProc, 1, 0);
  }
  if (!(psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD))
      EnableThemeDialogTexture (hwndPage, ETDT_ENABLETAB);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_LoadWizardBitmaps
 *
 * Loads the watermark and header bitmaps for a wizard.
 */
static VOID PROPSHEET_LoadWizardBitmaps(PropSheetInfo *psInfo)
{
  if (psInfo->ppshheader.dwFlags & (PSH_WIZARD97_NEW | PSH_WIZARD97_OLD))
  {
    /* if PSH_USEHBMWATERMARK is not set, load the resource from pszbmWatermark 
       and put the HBITMAP in hbmWatermark. Thus all the rest of the code always 
       considers hbmWatermark as valid. */
    if ((psInfo->ppshheader.dwFlags & PSH_WATERMARK) &&
        !(psInfo->ppshheader.dwFlags & PSH_USEHBMWATERMARK))
    {
      psInfo->ppshheader.hbmWatermark =
        CreateMappedBitmap(psInfo->ppshheader.hInstance, (INT_PTR)psInfo->ppshheader.pszbmWatermark, 0, NULL, 0);
    }

    /* Same behavior as for watermarks */
    if ((psInfo->ppshheader.dwFlags & PSH_HEADER) &&
        !(psInfo->ppshheader.dwFlags & PSH_USEHBMHEADER))
    {
      psInfo->ppshheader.hbmHeader =
        CreateMappedBitmap(psInfo->ppshheader.hInstance, (INT_PTR)psInfo->ppshheader.pszbmHeader, 0, NULL, 0);
    }
  }
}


/******************************************************************************
 *            PROPSHEET_ShowPage
 *
 * Displays or creates the specified page.
 */
static BOOL PROPSHEET_ShowPage(HWND hwndDlg, int index, PropSheetInfo * psInfo)
{
  HWND hwndTabCtrl;
  HWND hwndLineHeader;
  HWND control;

  TRACE("active_page %d, index %d\n", psInfo->active_page, index);
  if (index == psInfo->active_page)
  {
      if (GetTopWindow(hwndDlg) != psInfo->proppage[index].hwndPage)
          SetWindowPos(psInfo->proppage[index].hwndPage, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
      return TRUE;
  }

  if (psInfo->proppage[index].hwndPage == 0)
  {
     PROPSHEET_CreatePage(hwndDlg, index, psInfo, psInfo->proppage[index].hpage);
  }

  if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
  {
     PROPSHEET_SetTitleW(hwndDlg, psInfo->ppshheader.dwFlags,
                         psInfo->proppage[index].pszText);

     control = GetNextDlgTabItem(psInfo->proppage[index].hwndPage, NULL, FALSE);
     if(control != NULL)
         SetFocus(control);
  }

  if (psInfo->active_page != -1)
     ShowWindow(psInfo->proppage[psInfo->active_page].hwndPage, SW_HIDE);

  ShowWindow(psInfo->proppage[index].hwndPage, SW_SHOW);

  /* Synchronize current selection with tab control
   * It seems to be needed even in case of PSH_WIZARD (no tab controls there) */
  hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  SendMessageW(hwndTabCtrl, TCM_SETCURSEL, index, 0);

  psInfo->active_page = index;
  psInfo->activeValid = TRUE;

  if (psInfo->ppshheader.dwFlags & (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW) )
  {
      hwndLineHeader = GetDlgItem(hwndDlg, IDC_SUNKEN_LINEHEADER);
      
      if ((HPSP_get_flags(psInfo->proppage[index].hpage) & PSP_HIDEHEADER) ||
              (!(psInfo->ppshheader.dwFlags & PSH_HEADER)) )
	  ShowWindow(hwndLineHeader, SW_HIDE);
      else
	  ShowWindow(hwndLineHeader, SW_SHOW);
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Back
 */
static BOOL PROPSHEET_Back(HWND hwndDlg)
{
  PSHNOTIFY psn;
  HWND hwndPage;
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
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
     {
        SetFocus(GetDlgItem(hwndDlg, IDC_BACK_BUTTON));
        SendMessageW(hwndDlg, DM_SETDEFID, IDC_BACK_BUTTON, 0);
        PROPSHEET_SetCurSel(hwndDlg, idx, -1, 0);
     }
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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  int idx;

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return FALSE;

  psn.hdr.code     = PSN_WIZNEXT;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  msgResult = SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  if (msgResult == -1)
    return FALSE;
  else if (msgResult == 0)
     idx = psInfo->active_page + 1;
  else
     idx = PROPSHEET_FindPageByResId(psInfo, msgResult);

  if (idx < psInfo->nPages )
  {
     if (PROPSHEET_CanSetCurSel(hwndDlg) != FALSE)
     {
        SetFocus(GetDlgItem(hwndDlg, IDC_NEXT_BUTTON));
        SendMessageW(hwndDlg, DM_SETDEFID, IDC_NEXT_BUTTON, 0);
        PROPSHEET_SetCurSel(hwndDlg, idx, 1, 0);
     }
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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

  TRACE("active_page %d\n", psInfo->active_page);
  if (psInfo->active_page < 0)
     return FALSE;

  psn.hdr.code     = PSN_WIZFINISH;
  psn.hdr.hwndFrom = hwndDlg;
  psn.hdr.idFrom   = 0;
  psn.lParam       = 0;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  msgResult = SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);

  TRACE("msg result %Id\n", msgResult);

  if (msgResult != 0)
    return FALSE;

  if (psInfo->result == 0)
      psInfo->result = IDOK;
  if (psInfo->isModeless)
    psInfo->activeValid = FALSE;
  else
    psInfo->ended = TRUE;

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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

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

  if (SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn) != FALSE)
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
       switch (SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn))
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
     SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Cancel
 */
static void PROPSHEET_Cancel(HWND hwndDlg, LPARAM lParam)
{
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
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

  if (SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn))
    return;

  psn.hdr.code = PSN_RESET;
  psn.lParam   = lParam;

  for (i = 0; i < psInfo->nPages; i++)
  {
    hwndPage = psInfo->proppage[i].hwndPage;

    if (hwndPage)
       SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
  }

  if (psInfo->isModeless)
  {
     /* makes PSM_GETCURRENTPAGEHWND return NULL */
     psInfo->activeValid = FALSE;
  }
  else
    psInfo->ended = TRUE;
}

/******************************************************************************
 *            PROPSHEET_Help
 */
static void PROPSHEET_Help(HWND hwndDlg)
{
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
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

  SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);
}

/******************************************************************************
 *            PROPSHEET_Changed
 */
static void PROPSHEET_Changed(HWND hwndDlg, HWND hwndDirtyPage)
{
  int i;
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

  TRACE("\n");
  if ( !psInfo ) return;
  for (i = 0; i < psInfo->nPages; i++)
  {
    /* set the specified page as clean */
    if (psInfo->proppage[i].hwndPage == hwndCleanPage)
      psInfo->proppage[i].isDirty = FALSE;

    /* look to see if there are any dirty pages */
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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndPage;
  PSHNOTIFY psn;
  BOOL res = FALSE;

  if (!psInfo)
  {
     res = FALSE;
     goto end;
  }

  TRACE("active_page %d\n", psInfo->active_page);
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

  res = !SendMessageW(hwndPage, WM_NOTIFY, 0, (LPARAM) &psn);

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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndHelp  = GetDlgItem(hwndDlg, IDHELP);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);

  TRACE("index %d, skipdir %d, hpage %p\n", index, skipdir, hpage);

  index = PROPSHEET_GetPageIndex(hpage, psInfo, index);

  if (index < 0 || index >= psInfo->nPages)
  {
    TRACE("Could not find page to select!\n");
    return FALSE;
  }

  /* unset active page while doing this transition. */
  if (psInfo->active_page != -1)
     ShowWindow(psInfo->proppage[psInfo->active_page].hwndPage, SW_HIDE);
  psInfo->active_page = -1;

  while (1) {
    int result;
    PSHNOTIFY psn;
    RECT rc;

    if (hwndTabControl)
	SendMessageW(hwndTabControl, TCM_SETCURSEL, index, 0);

    psn.hdr.code     = PSN_SETACTIVE;
    psn.hdr.hwndFrom = hwndDlg;
    psn.hdr.idFrom   = 0;
    psn.lParam       = 0;

    if (!psInfo->proppage[index].hwndPage) {
      if(!PROPSHEET_CreatePage(hwndDlg, index, psInfo, psInfo->proppage[index].hpage)) {
        PROPSHEET_RemovePage(hwndDlg, index, NULL);

        if (!psInfo->isModeless)
        {
            DestroyWindow(hwndDlg);
            return FALSE;
        }

        if(index >= psInfo->nPages)
          index--;
        if(index < 0)
            return FALSE;
        continue;
      }
    }

    /* Resize the property sheet page to the fit in the Tab control
     * (for regular property sheets) or to fit in the client area (for
     * wizards).
     * NOTE: The resizing happens every time the page is selected and
     * not only when it's created (some applications depend on it). */
    PROPSHEET_GetPageRect(psInfo, hwndDlg, &rc, psInfo->proppage[index].hpage);
    TRACE("setting page %p, rc (%s) w=%ld, h=%ld\n",
          psInfo->proppage[index].hwndPage, wine_dbgstr_rect(&rc),
          rc.right - rc.left, rc.bottom - rc.top);
    SetWindowPos(psInfo->proppage[index].hwndPage, HWND_TOP,
                 rc.left, rc.top,
                 rc.right - rc.left, rc.bottom - rc.top, 0);

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
        WARN("Tried to skip to nonexistent page by res id\n");
        break;
      }
      continue;
    }
  }

  /* Invalidate the header area */
  if ( (psInfo->ppshheader.dwFlags & (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW)) &&
       (psInfo->ppshheader.dwFlags & PSH_HEADER) )
  {
    HWND hwndLineHeader = GetDlgItem(hwndDlg, IDC_SUNKEN_LINEHEADER);
    RECT r;

    GetClientRect(hwndLineHeader, &r);
    MapWindowPoints(hwndLineHeader, hwndDlg, (LPPOINT) &r, 2);
    SetRect(&r, 0, 0, r.right + 1, r.top - 1);

    InvalidateRect(hwndDlg, &r, TRUE);
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
      PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

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
  if(!IS_INTRESOURCE(lpszText))
  {
     WCHAR szTitle[256];
     MultiByteToWideChar(CP_ACP, 0, lpszText, -1, szTitle, ARRAY_SIZE(szTitle));
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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  WCHAR szTitle[256];

  TRACE("%s (style %#lx)\n", debugstr_w(lpszText), dwStyle);
  if (IS_INTRESOURCE(lpszText)) {
    if (!LoadStringW(psInfo->ppshheader.hInstance, LOWORD(lpszText), szTitle, ARRAY_SIZE(szTitle)))
      return;
    lpszText = szTitle;
  }
  if (dwStyle & PSH_PROPTITLE)
  {
    WCHAR* dest;
    int lentitle = lstrlenW(lpszText);
    int lenprop  = lstrlenW(psInfo->strPropertiesFor);

    dest = Alloc( (lentitle + lenprop + 1)*sizeof (WCHAR));
    wsprintfW(dest, psInfo->strPropertiesFor, lpszText);

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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndButton = GetDlgItem(hwndDlg, IDC_FINISH_BUTTON);

  TRACE("'%s'\n", lpszText);
  /* Set text, show and enable the Finish button */
  SetWindowTextA(hwndButton, lpszText);
  ShowWindow(hwndButton, SW_SHOW);
  EnableWindow(hwndButton, TRUE);

  /* Make it default pushbutton */
  SendMessageW(hwndDlg, DM_SETDEFID, IDC_FINISH_BUTTON, 0);

  /* Hide Back button */
  hwndButton = GetDlgItem(hwndDlg, IDC_BACK_BUTTON);
  ShowWindow(hwndButton, SW_HIDE);

  if (!psInfo->hasFinish)
  {
    /* Hide Next button */
    hwndButton = GetDlgItem(hwndDlg, IDC_NEXT_BUTTON);
    ShowWindow(hwndButton, SW_HIDE);
  }
}

/******************************************************************************
 *            PROPSHEET_SetFinishTextW
 */
static void PROPSHEET_SetFinishTextW(HWND hwndDlg, LPCWSTR lpszText)
{
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndButton = GetDlgItem(hwndDlg, IDC_FINISH_BUTTON);

  TRACE("%s\n", debugstr_w(lpszText));
  /* Set text, show and enable the Finish button */
  SetWindowTextW(hwndButton, lpszText);
  ShowWindow(hwndButton, SW_SHOW);
  EnableWindow(hwndButton, TRUE);

  /* Make it default pushbutton */
  SendMessageW(hwndDlg, DM_SETDEFID, IDC_FINISH_BUTTON, 0);

  /* Hide Back button */
  hwndButton = GetDlgItem(hwndDlg, IDC_BACK_BUTTON);
  ShowWindow(hwndButton, SW_HIDE);

  if (!psInfo->hasFinish)
  {
    /* Hide Next button */
    hwndButton = GetDlgItem(hwndDlg, IDC_NEXT_BUTTON);
    ShowWindow(hwndButton, SW_HIDE);
  }
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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

  while ((i < psInfo->nPages) && (msgResult == 0))
  {
    hwndPage = psInfo->proppage[i].hwndPage;
    msgResult = SendMessageW(hwndPage, PSM_QUERYSIBLINGS, wParam, lParam);
    i++;
  }

  return msgResult;
}

/******************************************************************************
 *            PROPSHEET_InsertPage
 */
static BOOL PROPSHEET_InsertPage(HWND hwndDlg, HPROPSHEETPAGE hpageInsertAfter, HPROPSHEETPAGE hpage)
{
  PropSheetInfo *psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  PropPageInfo *ppi, *prev_ppi = psInfo->proppage;
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  TCITEMW item;
  int index;

  TRACE("hwndDlg %p, hpageInsertAfter %p, hpage %p\n", hwndDlg, hpageInsertAfter, hpage);

  if (IS_INTRESOURCE(hpageInsertAfter))
    index = LOWORD(hpageInsertAfter);
  else
  {
    index = PROPSHEET_GetPageIndex(hpageInsertAfter, psInfo, -1);
    if (index < 0)
    {
      TRACE("Could not find page to insert after!\n");
      return FALSE;
    }
    index++;
  }

  if (index > psInfo->nPages)
    index = psInfo->nPages;

  ppi = Alloc(sizeof(PropPageInfo) * (psInfo->nPages + 1));
  if (!ppi)
      return FALSE;

  if (hpage && hpage->magic != HPROPSHEETPAGE_MAGIC)
  {
      if (psInfo->unicode)
          hpage = CreatePropertySheetPageW((const PROPSHEETPAGEW *)hpage);
      else
          hpage = CreatePropertySheetPageA((const PROPSHEETPAGEA *)hpage);
  }

  /*
   * Fill in a new PropPageInfo entry.
   */
  if (index > 0)
    memcpy(ppi, prev_ppi, index * sizeof(PropPageInfo));
  memset(&ppi[index], 0, sizeof(PropPageInfo));
  if (index < psInfo->nPages)
    memcpy(&ppi[index + 1], &prev_ppi[index], (psInfo->nPages - index) * sizeof(PropPageInfo));
  psInfo->proppage = ppi;

  if (!PROPSHEET_CollectPageInfo(hpage, psInfo, index, FALSE))
  {
     psInfo->proppage = prev_ppi;
     Free(ppi);
     return FALSE;
  }

  psInfo->proppage[index].hpage = hpage;

  if (HPSP_get_flags(hpage) & PSP_PREMATURE)
  {
     /* Create the page but don't show it */
     if (!PROPSHEET_CreatePage(hwndDlg, index, psInfo, hpage))
     {
        psInfo->proppage = prev_ppi;
        Free(ppi);
        return FALSE;
     }
  }

  Free(prev_ppi);
  psInfo->nPages++;
  if (index <= psInfo->active_page)
    psInfo->active_page++;

  /*
   * Add a new tab to the tab control.
   */
  item.mask = TCIF_TEXT;
  item.pszText = (LPWSTR) psInfo->proppage[index].pszText;
  item.cchTextMax = MAX_TABTEXT_LENGTH;

  if (psInfo->hImageList)
    SendMessageW(hwndTabControl, TCM_SETIMAGELIST, 0, (LPARAM)psInfo->hImageList);

  if (psInfo->proppage[index].hasIcon)
  {
    item.mask |= TCIF_IMAGE;
    item.iImage = index;
  }

  SendMessageW(hwndTabControl, TCM_INSERTITEMW, index, (LPARAM)&item);

  /* If it is the only page - show it */
  if (psInfo->nPages == 1)
     PROPSHEET_SetCurSel(hwndDlg, 0, 1, 0);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AddPage
 */
static BOOL PROPSHEET_AddPage(HWND hwndDlg, HPROPSHEETPAGE hpage)
{
  PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  TRACE("hwndDlg %p, hpage %p\n", hwndDlg, hpage);
  return PROPSHEET_InsertPage(hwndDlg, UlongToPtr(psInfo->nPages), hpage);
}

/******************************************************************************
 *            PROPSHEET_RemovePage
 */
static BOOL PROPSHEET_RemovePage(HWND hwndDlg,
                                 int index,
                                 HPROPSHEETPAGE hpage)
{
  PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  PropPageInfo* oldPages;

  TRACE("index %d, hpage %p\n", index, hpage);
  if (!psInfo) {
    return FALSE;
  }

  index = PROPSHEET_GetPageIndex(hpage, psInfo, index);

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
         psInfo->ended = TRUE;
         return TRUE;
      }
    }
  }
  else if (index < psInfo->active_page)
    psInfo->active_page--;

  /* Unsubclass the page dialog window */
  if((psInfo->ppshheader.dwFlags & (PSH_WIZARD97_NEW | PSH_WIZARD97_OLD)) &&
     (psInfo->ppshheader.dwFlags & PSH_WATERMARK) &&
     (HPSP_get_flags(psInfo->proppage[index].hpage) & PSP_HIDEHEADER))
  {
     RemoveWindowSubclass(psInfo->proppage[index].hwndPage,
                          PROPSHEET_WizardSubclassProc, 1);
  }

  /* Destroy page dialog window */
  DestroyWindow(psInfo->proppage[index].hwndPage);

  /* Free page resources */
  if(psInfo->proppage[index].hpage)
  {
     if (HPSP_get_flags(psInfo->proppage[index].hpage) & PSP_USETITLE)
        Free ((LPVOID)psInfo->proppage[index].pszText);

     DestroyPropertySheetPage(psInfo->proppage[index].hpage);
  }

  /* Remove the tab */
  SendMessageW(hwndTabControl, TCM_DELETEITEM, index, 0);

  oldPages = psInfo->proppage;
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
  PropSheetInfo* psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
  HWND hwndBack   = GetDlgItem(hwndDlg, IDC_BACK_BUTTON);
  HWND hwndNext   = GetDlgItem(hwndDlg, IDC_NEXT_BUTTON);
  HWND hwndFinish = GetDlgItem(hwndDlg, IDC_FINISH_BUTTON);
  BOOL enable_finish = ((dwFlags & PSWIZB_FINISH) || psInfo->hasFinish) && !(dwFlags & PSWIZB_DISABLEDFINISH);

  TRACE("%lx\n", dwFlags);

  EnableWindow(hwndBack, dwFlags & PSWIZB_BACK);
  EnableWindow(hwndNext, dwFlags & PSWIZB_NEXT);
  EnableWindow(hwndFinish, enable_finish);

  /* set the default pushbutton to an enabled button */
  if (enable_finish)
    SendMessageW(hwndDlg, DM_SETDEFID, IDC_FINISH_BUTTON, 0);
  else if (dwFlags & PSWIZB_NEXT)
    SendMessageW(hwndDlg, DM_SETDEFID, IDC_NEXT_BUTTON, 0);
  else if (dwFlags & PSWIZB_BACK)
    SendMessageW(hwndDlg, DM_SETDEFID, IDC_BACK_BUTTON, 0);
  else
    SendMessageW(hwndDlg, DM_SETDEFID, IDCANCEL, 0);

  if (!psInfo->hasFinish)
  {
    if ((dwFlags & PSWIZB_FINISH) || (dwFlags & PSWIZB_DISABLEDFINISH))
    {
      /* Hide the Next button */
      ShowWindow(hwndNext, SW_HIDE);
      
      /* Show the Finish button */
      ShowWindow(hwndFinish, SW_SHOW);
    }
    else
    {
      /* Hide the Finish button */
      ShowWindow(hwndFinish, SW_HIDE);
      /* Show the Next button */
      ShowWindow(hwndNext, SW_SHOW);
    }
  }
}

/******************************************************************************
 *            PROPSHEET_SetHeaderTitleW
 */
static void PROPSHEET_SetHeaderTitleW(HWND hwndDlg, UINT page_index, const WCHAR *title)
{
    PropSheetInfo *psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

    TRACE("(%p, %u, %s)\n", hwndDlg, page_index, debugstr_w(title));

    if (page_index >= psInfo->nPages)
        return;

    HPSP_set_header_title(psInfo->proppage[page_index].hpage, title);
}

/******************************************************************************
 *            PROPSHEET_SetHeaderTitleA
 */
static void PROPSHEET_SetHeaderTitleA(HWND hwndDlg, UINT page_index, const char *title)
{
    WCHAR *titleW;

    TRACE("(%p, %u, %s)\n", hwndDlg, page_index, debugstr_a(title));

    titleW = heap_strdupAtoW(title);
    PROPSHEET_SetHeaderTitleW(hwndDlg, page_index, titleW);
    Free(titleW);
}

/******************************************************************************
 *            PROPSHEET_SetHeaderSubTitleW
 */
static void PROPSHEET_SetHeaderSubTitleW(HWND hwndDlg, UINT page_index, const WCHAR *subtitle)
{
    PropSheetInfo *psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

    TRACE("(%p, %u, %s)\n", hwndDlg, page_index, debugstr_w(subtitle));

    if (page_index >= psInfo->nPages)
        return;

    HPSP_set_header_subtitle(psInfo->proppage[page_index].hpage, subtitle);
}

/******************************************************************************
 *            PROPSHEET_SetHeaderSubTitleA
 */
static void PROPSHEET_SetHeaderSubTitleA(HWND hwndDlg, UINT page_index, const char *subtitle)
{
    WCHAR *subtitleW;

    TRACE("(%p, %u, %s)\n", hwndDlg, page_index, debugstr_a(subtitle));

    subtitleW = heap_strdupAtoW(subtitle);
    PROPSHEET_SetHeaderSubTitleW(hwndDlg, page_index, subtitleW);
    Free(subtitleW);
}

/******************************************************************************
 *            PROPSHEET_HwndToIndex
 */
static LRESULT PROPSHEET_HwndToIndex(HWND hwndDlg, HWND hPageDlg)
{
    int index;
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

    TRACE("(%p, %p)\n", hwndDlg, hPageDlg);

    for (index = 0; index < psInfo->nPages; index++)
        if (psInfo->proppage[index].hwndPage == hPageDlg)
            return index;
    WARN("%p not found\n", hPageDlg);
    return -1;
}

/******************************************************************************
 *            PROPSHEET_IndexToHwnd
 */
static LRESULT PROPSHEET_IndexToHwnd(HWND hwndDlg, int iPageIndex)
{
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
    TRACE("(%p, %d)\n", hwndDlg, iPageIndex);
    if (!psInfo)
        return 0;
    if (iPageIndex<0 || iPageIndex>=psInfo->nPages) {
        WARN("%d out of range.\n", iPageIndex);
	return 0;
    }
    return (LRESULT)psInfo->proppage[iPageIndex].hwndPage;
}

/******************************************************************************
 *            PROPSHEET_PageToIndex
 */
static LRESULT PROPSHEET_PageToIndex(HWND hwndDlg, HPROPSHEETPAGE hPage)
{
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);

    TRACE("(%p, %p)\n", hwndDlg, hPage);

    return PROPSHEET_GetPageIndex(hPage, psInfo, -1);
}

/******************************************************************************
 *            PROPSHEET_IndexToPage
 */
static LRESULT PROPSHEET_IndexToPage(HWND hwndDlg, int iPageIndex)
{
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
    TRACE("(%p, %d)\n", hwndDlg, iPageIndex);
    if (iPageIndex<0 || iPageIndex>=psInfo->nPages) {
        WARN("%d out of range.\n", iPageIndex);
	return 0;
    }
    return (LRESULT)psInfo->proppage[iPageIndex].hpage;
}

/******************************************************************************
 *            PROPSHEET_IdToIndex
 */
static LRESULT PROPSHEET_IdToIndex(HWND hwndDlg, int iPageId)
{
    int index;
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
    TRACE("(%p, %d)\n", hwndDlg, iPageId);
    for (index = 0; index < psInfo->nPages; index++) {
        if (HPSP_get_template(psInfo->proppage[index].hpage) == iPageId)
            return index;
    }

    return -1;
}

/******************************************************************************
 *            PROPSHEET_IndexToId
 */
static LRESULT PROPSHEET_IndexToId(HWND hwndDlg, int iPageIndex)
{
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
    HPROPSHEETPAGE hpsp;
    LRESULT template;

    TRACE("(%p, %d)\n", hwndDlg, iPageIndex);

    if (iPageIndex<0 || iPageIndex>=psInfo->nPages) {
        WARN("%d out of range.\n", iPageIndex);
	return 0;
    }
    hpsp = psInfo->proppage[iPageIndex].hpage;
    template = HPSP_get_template(hpsp);
    if (HPSP_get_flags(hpsp) & PSP_DLGINDIRECT || !IS_INTRESOURCE(template))
        return 0;
    return template;
}

/******************************************************************************
 *            PROPSHEET_GetResult
 */
static LRESULT PROPSHEET_GetResult(HWND hwndDlg)
{
    PropSheetInfo * psInfo = GetPropW(hwndDlg, PropSheetInfoStr);
    return psInfo->result;
}

/******************************************************************************
 *            PROPSHEET_RecalcPageSizes
 */
static BOOL PROPSHEET_RecalcPageSizes(HWND hwndDlg)
{
    FIXME("(%p): stub\n", hwndDlg);
    return FALSE;
}

/******************************************************************************
 *            PROPSHEET_GetPageIndex
 *
 * Given a HPROPSHEETPAGE, returns the index of the corresponding page from
 * the array of PropPageInfo. If page is not found original index is used
 * (page takes precedence over index).
 */
static int PROPSHEET_GetPageIndex(HPROPSHEETPAGE page, const PropSheetInfo* psInfo, int original_index)
{
    int index;

    TRACE("page %p index %d\n", page, original_index);

    for (index = 0; index < psInfo->nPages; index++)
        if (psInfo->proppage[index].hpage == page)
            return index;

    return original_index;
}

/******************************************************************************
 *            PROPSHEET_CleanUp
 */
static void PROPSHEET_CleanUp(HWND hwndDlg)
{
  int i;
  PropSheetInfo* psInfo = RemovePropW(hwndDlg, PropSheetInfoStr);

  TRACE("\n");
  if (!psInfo) return;
  if (!IS_INTRESOURCE(psInfo->ppshheader.pszCaption))
      Free ((LPVOID)psInfo->ppshheader.pszCaption);

  for (i = 0; i < psInfo->nPages; i++)
  {
     DWORD flags = HPSP_get_flags(psInfo->proppage[i].hpage);

     /* Unsubclass the page dialog window */
     if((psInfo->ppshheader.dwFlags & (PSH_WIZARD97_NEW | PSH_WIZARD97_OLD)) &&
        (psInfo->ppshheader.dwFlags & PSH_WATERMARK) &&
        (flags & PSP_HIDEHEADER))
     {
        RemoveWindowSubclass(psInfo->proppage[i].hwndPage,
                             PROPSHEET_WizardSubclassProc, 1);
     }

     if(psInfo->proppage[i].hwndPage)
        DestroyWindow(psInfo->proppage[i].hwndPage);

     if (flags & PSP_USETITLE)
        Free ((LPVOID)psInfo->proppage[i].pszText);

     DestroyPropertySheetPage(psInfo->proppage[i].hpage);
  }

  DeleteObject(psInfo->hFont);
  DeleteObject(psInfo->hFontBold);
  /* If we created the bitmaps, destroy them */
  if ((psInfo->ppshheader.dwFlags & PSH_WATERMARK) &&
      (!(psInfo->ppshheader.dwFlags & PSH_USEHBMWATERMARK)) )
      DeleteObject(psInfo->ppshheader.hbmWatermark);
  if ((psInfo->ppshheader.dwFlags & PSH_HEADER) &&
      (!(psInfo->ppshheader.dwFlags & PSH_USEHBMHEADER)) )
      DeleteObject(psInfo->ppshheader.hbmHeader);

  Free(psInfo->proppage);
  Free(psInfo->strPropertiesFor);
  ImageList_Destroy(psInfo->hImageList);

  GlobalFree(psInfo);
}

static INT do_loop(const PropSheetInfo *psInfo)
{
    MSG msg = { 0 };
    INT ret = 0;
    HWND hwnd = psInfo->hwnd;
    HWND parent = psInfo->ppshheader.hwndParent;

    while(IsWindow(hwnd) && !psInfo->ended && (ret = GetMessageW(&msg, NULL, 0, 0)))
    {
        if(ret == -1)
            break;

        if(!IsDialogMessageW(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if(ret == 0 && msg.message)
        PostQuitMessage(msg.wParam);

    if(ret != -1)
        ret = psInfo->result;

    if(parent)
        EnableWindow(parent, TRUE);

    DestroyWindow(hwnd);
    return ret;
}

/******************************************************************************
 *            PROPSHEET_PropertySheet
 *
 * Common code between PropertySheetA/W
 */
static INT_PTR PROPSHEET_PropertySheet(PropSheetInfo* psInfo, BOOL unicode)
{
  INT_PTR bRet = 0;
  HWND parent = NULL;
  if (psInfo->active_page >= psInfo->nPages) psInfo->active_page = 0;
  TRACE("startpage: %d of %d pages\n", psInfo->active_page, psInfo->nPages);

  psInfo->unicode = unicode;
  psInfo->ended = FALSE;

  if(!psInfo->isModeless)
  {
      parent = psInfo->ppshheader.hwndParent;
      if (parent) EnableWindow(parent, FALSE);
  }
  bRet = PROPSHEET_CreateDialog(psInfo);
  if(!psInfo->isModeless)
      bRet = do_loop(psInfo);
  return bRet;
}

/******************************************************************************
 *            PropertySheet    (COMCTL32.@)
 *            PropertySheetA   (COMCTL32.@)
 *
 * Creates a property sheet in the specified property sheet header.
 *
 * RETURNS
 *     Modal property sheets: Positive if successful or -1 otherwise.
 *     Modeless property sheets: Property sheet handle.
 *     Or:
 *| ID_PSREBOOTSYSTEM - The user must reboot the computer for the changes to take effect.
 *| ID_PSRESTARTWINDOWS - The user must restart Windows for the changes to take effect.
 */
INT_PTR WINAPI PropertySheetA(LPCPROPSHEETHEADERA lppsh)
{
  PropSheetInfo* psInfo = GlobalAlloc(GPTR, sizeof(PropSheetInfo));
  UINT i, n;
  const BYTE* pByte;

  TRACE("(%p)\n", lppsh);

  PROPSHEET_CollectSheetInfoA(lppsh, psInfo);

  psInfo->proppage = Alloc(sizeof(PropPageInfo) * lppsh->nPages);
  pByte = (const BYTE*) psInfo->ppshheader.ppsp;

  for (n = i = 0; i < lppsh->nPages; i++, n++)
  {
    if (!psInfo->usePropPage)
    {
        if (psInfo->ppshheader.phpage[i] &&
                psInfo->ppshheader.phpage[i]->magic == HPROPSHEETPAGE_MAGIC)
        {
            psInfo->proppage[n].hpage = psInfo->ppshheader.phpage[i];
        }
        else
        {
            psInfo->proppage[n].hpage = CreatePropertySheetPageA(
                    (const PROPSHEETPAGEA *)psInfo->ppshheader.phpage[i]);
        }
    }
    else
    {
       psInfo->proppage[n].hpage = CreatePropertySheetPageA((LPCPROPSHEETPAGEA)pByte);
       pByte += ((LPCPROPSHEETPAGEA)pByte)->dwSize;
    }

    if (!PROPSHEET_CollectPageInfo(psInfo->proppage[n].hpage, psInfo, n, TRUE))
    {
	if (psInfo->usePropPage)
	    DestroyPropertySheetPage(psInfo->proppage[n].hpage);
	n--;
	psInfo->nPages--;
    }
  }

  return PROPSHEET_PropertySheet(psInfo, FALSE);
}

/******************************************************************************
 *            PropertySheetW   (COMCTL32.@)
 *
 * See PropertySheetA.
 */
INT_PTR WINAPI PropertySheetW(LPCPROPSHEETHEADERW lppsh)
{
  PropSheetInfo* psInfo = GlobalAlloc(GPTR, sizeof(PropSheetInfo));
  UINT i, n;
  const BYTE* pByte;

  TRACE("(%p)\n", lppsh);

  PROPSHEET_CollectSheetInfoW(lppsh, psInfo);

  psInfo->proppage = Alloc(sizeof(PropPageInfo) * lppsh->nPages);
  pByte = (const BYTE*) psInfo->ppshheader.ppsp;

  for (n = i = 0; i < lppsh->nPages; i++, n++)
  {
    if (!psInfo->usePropPage)
    {
        if (psInfo->ppshheader.phpage[i] &&
                psInfo->ppshheader.phpage[i]->magic == HPROPSHEETPAGE_MAGIC)
        {
            psInfo->proppage[n].hpage = psInfo->ppshheader.phpage[i];
        }
        else
        {
            psInfo->proppage[n].hpage = CreatePropertySheetPageW(
                    (const PROPSHEETPAGEW *)psInfo->ppshheader.phpage[i]);
        }
    }
    else
    {
       psInfo->proppage[n].hpage = CreatePropertySheetPageW((LPCPROPSHEETPAGEW)pByte);
       pByte += ((LPCPROPSHEETPAGEW)pByte)->dwSize;
    }

    if (!PROPSHEET_CollectPageInfo(psInfo->proppage[n].hpage, psInfo, n, TRUE))
    {
	if (psInfo->usePropPage)
	    DestroyPropertySheetPage(psInfo->proppage[n].hpage);
	n--;
	psInfo->nPages--;
    }
  }

  return PROPSHEET_PropertySheet(psInfo, TRUE);
}

/******************************************************************************
 *            CreatePropertySheetPage    (COMCTL32.@)
 *            CreatePropertySheetPageA   (COMCTL32.@)
 *
 * Creates a new property sheet page.
 *
 * RETURNS
 *     Success: Handle to new property sheet page.
 *     Failure: NULL.
 *
 * NOTES
 *     An application must use the PSM_ADDPAGE message to add the new page to
 *     an existing property sheet.
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(
                          LPCPROPSHEETPAGEA lpPropSheetPage)
{
    PROPSHEETPAGEA *ppsp;
    HPROPSHEETPAGE ret;

    if (lpPropSheetPage->dwSize < PROPSHEETPAGEA_V1_SIZE)
        return NULL;

    ret = Alloc(FIELD_OFFSET(struct _PSP, data[lpPropSheetPage->dwSize]));
    ret->magic = HPROPSHEETPAGE_MAGIC;
    ppsp = &ret->pspA;
    memcpy(ppsp, lpPropSheetPage, lpPropSheetPage->dwSize);

    if ( !(ppsp->dwFlags & PSP_DLGINDIRECT) )
    {
        if (!IS_INTRESOURCE( ppsp->pszTemplate ))
            ppsp->pszTemplate = heap_strdupA( lpPropSheetPage->pszTemplate );
    }

    if (ppsp->dwFlags & PSP_USEICONID)
    {
        if (!IS_INTRESOURCE( ppsp->pszIcon ))
            ppsp->pszIcon = heap_strdupA( lpPropSheetPage->pszIcon );
    }

    if (ppsp->dwFlags & PSP_USETITLE)
    {
        if (!IS_INTRESOURCE( ppsp->pszTitle ))
            ppsp->pszTitle = heap_strdupA( lpPropSheetPage->pszTitle );
    }

    if (ppsp->dwFlags & PSP_HIDEHEADER)
        ppsp->dwFlags &= ~(PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE);

    if (ppsp->dwFlags & PSP_USEHEADERTITLE)
    {
        if (!IS_INTRESOURCE( ppsp->pszHeaderTitle ))
            ppsp->pszHeaderTitle = heap_strdupA( lpPropSheetPage->pszHeaderTitle );
    }

    if (ppsp->dwFlags & PSP_USEHEADERSUBTITLE)
    {
        if (!IS_INTRESOURCE( ppsp->pszHeaderSubTitle ))
            ppsp->pszHeaderSubTitle = heap_strdupA( lpPropSheetPage->pszHeaderSubTitle );
    }

    HPSP_call_callback(ret, PSPCB_ADDREF);
    return ret;
}

/******************************************************************************
 *            CreatePropertySheetPageW   (COMCTL32.@)
 *
 * See CreatePropertySheetA.
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW lpPropSheetPage)
{
    PROPSHEETPAGEW *ppsp;
    HPROPSHEETPAGE ret;

    if (lpPropSheetPage->dwSize < PROPSHEETPAGEW_V1_SIZE)
        return NULL;

    ret = Alloc(FIELD_OFFSET(struct _PSP, data[lpPropSheetPage->dwSize]));
    ret->magic = HPROPSHEETPAGE_MAGIC;
    ret->unicode = TRUE;
    ppsp = &ret->pspW;
    memcpy(ppsp, lpPropSheetPage, lpPropSheetPage->dwSize);

    if ( !(ppsp->dwFlags & PSP_DLGINDIRECT) )
    {
        if (!IS_INTRESOURCE( ppsp->pszTemplate ))
            ppsp->pszTemplate = heap_strdupW( lpPropSheetPage->pszTemplate );
    }

    if ( ppsp->dwFlags & PSP_USEICONID )
    {
        if (!IS_INTRESOURCE( ppsp->pszIcon ))
            ppsp->pszIcon = heap_strdupW( lpPropSheetPage->pszIcon );
    }

    if (ppsp->dwFlags & PSP_USETITLE)
    {
        if (!IS_INTRESOURCE( ppsp->pszTitle ))
            ppsp->pszTitle = heap_strdupW( lpPropSheetPage->pszTitle );
    }

    if (ppsp->dwFlags & PSP_HIDEHEADER)
        ppsp->dwFlags &= ~(PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE);

    if (ppsp->dwFlags & PSP_USEHEADERTITLE)
    {
        if (!IS_INTRESOURCE( ppsp->pszHeaderTitle ))
            ppsp->pszHeaderTitle = heap_strdupW( ppsp->pszHeaderTitle );
    }

    if (ppsp->dwFlags & PSP_USEHEADERSUBTITLE)
    {
        if (!IS_INTRESOURCE( ppsp->pszHeaderSubTitle ))
            ppsp->pszHeaderSubTitle = heap_strdupW( ppsp->pszHeaderSubTitle );
    }

    HPSP_call_callback(ret, PSPCB_ADDREF);
    return ret;
}

/******************************************************************************
 *            DestroyPropertySheetPage   (COMCTL32.@)
 *
 * Destroys a property sheet page previously created with
 * CreatePropertySheetA() or CreatePropertySheetW() and frees the associated
 * memory.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE hpsp)
{
    if (!hpsp)
        return FALSE;

    HPSP_call_callback(hpsp, PSPCB_RELEASE);

    if (hpsp->unicode)
    {
        PROPSHEETPAGEW *psp = &hpsp->pspW;

        if (!(psp->dwFlags & PSP_DLGINDIRECT) && !IS_INTRESOURCE(psp->pszTemplate))
            Free((void *)psp->pszTemplate);

        if ((psp->dwFlags & PSP_USEICONID) && !IS_INTRESOURCE(psp->pszIcon))
            Free((void *)psp->pszIcon);

        if ((psp->dwFlags & PSP_USETITLE) && !IS_INTRESOURCE(psp->pszTitle))
            Free((void *)psp->pszTitle);

        if ((psp->dwFlags & PSP_USEHEADERTITLE) && !IS_INTRESOURCE(psp->pszHeaderTitle))
            Free((void *)psp->pszHeaderTitle);

        if ((psp->dwFlags & PSP_USEHEADERSUBTITLE) && !IS_INTRESOURCE(psp->pszHeaderSubTitle))
            Free((void *)psp->pszHeaderSubTitle);
    }
    else
    {
        PROPSHEETPAGEA *psp = &hpsp->pspA;

        if (!(psp->dwFlags & PSP_DLGINDIRECT) && !IS_INTRESOURCE(psp->pszTemplate))
            Free((void *)psp->pszTemplate);

        if ((psp->dwFlags & PSP_USEICONID) && !IS_INTRESOURCE(psp->pszIcon))
            Free((void *)psp->pszIcon);

        if ((psp->dwFlags & PSP_USETITLE) && !IS_INTRESOURCE(psp->pszTitle))
            Free((void *)psp->pszTitle);

        if ((psp->dwFlags & PSP_USEHEADERTITLE) && !IS_INTRESOURCE(psp->pszHeaderTitle))
            Free((void *)psp->pszHeaderTitle);

        if ((psp->dwFlags & PSP_USEHEADERSUBTITLE) && !IS_INTRESOURCE(psp->pszHeaderSubTitle))
            Free((void *)psp->pszHeaderSubTitle);
    }

    Free(hpsp);
    return TRUE;
}

/******************************************************************************
 *            PROPSHEET_IsDialogMessage
 */
static BOOL PROPSHEET_IsDialogMessage(HWND hwnd, LPMSG lpMsg)
{
   PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);

   TRACE("\n");
   if (!psInfo || (hwnd != lpMsg->hwnd && !IsChild(hwnd, lpMsg->hwnd)))
      return FALSE;

   if (lpMsg->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000))
   {
      int new_page = 0;
      INT dlgCode = SendMessageW(lpMsg->hwnd, WM_GETDLGCODE, 0, (LPARAM)lpMsg);

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

   return IsDialogMessageW(hwnd, lpMsg);
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
                    PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);

                    /* don't overwrite ID_PSRESTARTWINDOWS or ID_PSREBOOTSYSTEM */
                    if (psInfo->result == 0)
                        psInfo->result = IDOK;

		    if (psInfo->isModeless)
			psInfo->activeValid = FALSE;
		    else
                        psInfo->ended = TRUE;
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
 *            PROPSHEET_Paint
 */
static LRESULT PROPSHEET_Paint(HWND hwnd, HDC hdcParam)
{
    PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);
    PAINTSTRUCT ps;
    HDC hdc, hdcSrc;
    BITMAP bm;
    HBITMAP hbmp;
    HPALETTE hOldPal = 0;
    int offsety = 0;
    HBRUSH hbr;
    RECT r, rzone;
    HPROPSHEETPAGE hpsp;
    DWORD flags;

    hdc = hdcParam ? hdcParam : BeginPaint(hwnd, &ps);
    if (!hdc) return 1;

    hdcSrc = CreateCompatibleDC(0);

    if (psInfo->ppshheader.dwFlags & PSH_USEHPLWATERMARK) 
	hOldPal = SelectPalette(hdc, psInfo->ppshheader.hplWatermark, FALSE);

    if (psInfo->active_page < 0)
        hpsp = NULL;
    else
        hpsp = psInfo->proppage[psInfo->active_page].hpage;
    flags = HPSP_get_flags(hpsp);

    if ( hpsp && !(flags & PSP_HIDEHEADER) &&
	 (psInfo->ppshheader.dwFlags & (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW)) &&
	 (psInfo->ppshheader.dwFlags & PSH_HEADER) ) 
    {
	HWND hwndLineHeader = GetDlgItem(hwnd, IDC_SUNKEN_LINEHEADER);
	HFONT hOldFont;
	COLORREF clrOld = 0;
	int oldBkMode = 0;

        GetClientRect(hwndLineHeader, &r);
        MapWindowPoints(hwndLineHeader, hwnd, (LPPOINT) &r, 2);
        SetRect(&rzone, 0, 0, r.right + 1, r.top - 1);

        hOldFont = SelectObject(hdc, psInfo->hFontBold);

        if (psInfo->ppshheader.dwFlags & PSH_USEHBMHEADER)
        {
            hbmp = SelectObject(hdcSrc, psInfo->ppshheader.hbmHeader);

            GetObjectW(psInfo->ppshheader.hbmHeader, sizeof(BITMAP), &bm);
            if (psInfo->ppshheader.dwFlags & PSH_WIZARD97_OLD)
            {
                /* Fill the unoccupied part of the header with color of the
                 * left-top pixel, but do it only when needed.
                 */
                if (bm.bmWidth < r.right || bm.bmHeight < r.bottom)
                {
                    hbr = CreateSolidBrush(GetPixel(hdcSrc, 0, 0));
                    r = rzone;
                    if (bm.bmWidth < r.right)
                    {
                        r.left = bm.bmWidth;
                        FillRect(hdc, &r, hbr);
                    }
                    if (bm.bmHeight < r.bottom)
                    {
                        r.left = 0;
                        r.top = bm.bmHeight;
                        FillRect(hdc, &r, hbr);
                    }
                    DeleteObject(hbr);
                }

                /* Draw the header itself. */
                BitBlt(hdc, 0, 0, bm.bmWidth, min(bm.bmHeight, rzone.bottom),
                        hdcSrc, 0, 0, SRCCOPY);
            }
            else
            {
                int margin;
                hbr = GetSysColorBrush(COLOR_WINDOW);
                FillRect(hdc, &rzone, hbr);

                /* Draw the header bitmap. It's always centered like a
                 * common 49 x 49 bitmap. */
                margin = (rzone.bottom - 49) / 2;
                BitBlt(hdc, rzone.right - 49 - margin, margin,
                        min(bm.bmWidth, 49), min(bm.bmHeight, 49),
                        hdcSrc, 0, 0, SRCCOPY);

                /* NOTE: Native COMCTL32 draws a white stripe over the bitmap
                 * if its height is smaller than 49 pixels. Because the reason
                 * for this bug is unknown the current code doesn't try to
                 * replicate it. */
            }

            SelectObject(hdcSrc, hbmp);
        }

	clrOld = SetTextColor (hdc, 0x00000000);
	oldBkMode = SetBkMode (hdc, TRANSPARENT); 

	if (flags & PSP_USEHEADERTITLE) {
	    SetRect(&r, 20, 10, 0, 0);
            HPSP_draw_text(hpsp, hdc, TRUE, &r, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);
	}

	if (flags & PSP_USEHEADERSUBTITLE) {
	    SelectObject(hdc, psInfo->hFont);
	    SetRect(&r, 40, 25, rzone.right - 69, rzone.bottom);
            HPSP_draw_text(hpsp, hdc, FALSE, &r, DT_LEFT | DT_WORDBREAK);
	}

	offsety = rzone.bottom + 2;

	SetTextColor(hdc, clrOld);
	SetBkMode(hdc, oldBkMode);
	SelectObject(hdc, hOldFont);
    }

    if ( (flags & PSP_HIDEHEADER) &&
	 (psInfo->ppshheader.dwFlags & (PSH_WIZARD97_OLD | PSH_WIZARD97_NEW)) &&
	 (psInfo->ppshheader.dwFlags & PSH_WATERMARK) ) 
    {
	HWND hwndLine = GetDlgItem(hwnd, IDC_SUNKEN_LINE);	    

	GetClientRect(hwndLine, &r);
	MapWindowPoints(hwndLine, hwnd, (LPPOINT) &r, 2);
        SetRect(&rzone, 0, 0, r.right, r.top - 1);

	hbr = GetSysColorBrush(COLOR_WINDOW);
	FillRect(hdc, &rzone, hbr);

	GetObjectW(psInfo->ppshheader.hbmWatermark, sizeof(BITMAP), &bm);
	hbmp = SelectObject(hdcSrc, psInfo->ppshheader.hbmWatermark);

        /* The watermark is truncated to a width of 164 pixels */
        r.right = min(r.right, 164);
	BitBlt(hdc, 0, offsety, min(bm.bmWidth, r.right),
	       min(bm.bmHeight, r.bottom), hdcSrc, 0, 0, SRCCOPY);

	/* If the bitmap is not big enough, fill the remaining area
	   with the color of pixel (0,0) of bitmap - see MSDN */
	if (r.top > bm.bmHeight) {
	    r.bottom = r.top - 1;
	    r.top = bm.bmHeight;
	    r.left = 0;
	    r.right = bm.bmWidth;
	    hbr = CreateSolidBrush(GetPixel(hdcSrc, 0, 0));
	    FillRect(hdc, &r, hbr);
	    DeleteObject(hbr);
	}

	SelectObject(hdcSrc, hbmp);	    
    }

    if (psInfo->ppshheader.dwFlags & PSH_USEHPLWATERMARK) 
	SelectPalette(hdc, hOldPal, FALSE);

    DeleteDC(hdcSrc);

    if (!hdcParam) EndPaint(hwnd, &ps);

    return 0;
}

/******************************************************************************
 *            PROPSHEET_DialogProc
 */
static INT_PTR CALLBACK
PROPSHEET_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  TRACE("hwnd %p, msg=0x%04x, wparam %Ix, lparam %Ix\n", hwnd, uMsg, wParam, lParam);

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) lParam;
      WCHAR* strCaption = Alloc(MAX_CAPTION_LENGTH*sizeof(WCHAR));
      HWND hwndTabCtrl = GetDlgItem(hwnd, IDC_TABCONTROL);
      int idx;
      LOGFONTW logFont;

      /* Using PropSheetInfoStr to store extra data doesn't match the native
       * common control: native uses TCM_[GS]ETITEM
       */
      SetPropW(hwnd, PropSheetInfoStr, psInfo);

      /*
       * psInfo->hwnd is not being used by WINE code - it exists
       * for compatibility with "real" Windoze. The same about
       * SetWindowLongPtr - WINE is only using the PropSheetInfoStr
       * property.
       */
      psInfo->hwnd = hwnd;
      SetWindowLongPtrW(hwnd, DWLP_USER, (DWORD_PTR)psInfo);

      if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
      {
        /* set up the Next and Back buttons by default */
        PROPSHEET_SetWizButtons(hwnd, PSWIZB_BACK|PSWIZB_NEXT);
      }

      /* Set up fonts */
      SystemParametersInfoW (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
      psInfo->hFont = CreateFontIndirectW (&logFont);
      logFont.lfWeight = FW_BOLD;
      psInfo->hFontBold = CreateFontIndirectW (&logFont);
      
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
                             psInfo->ppshheader.pszIcon,
                             IMAGE_ICON,
                             icon_cx, icon_cy,
                             LR_DEFAULTCOLOR);
        else
          hIcon = psInfo->ppshheader.hIcon;

        SendMessageW(hwnd, WM_SETICON, 0, (LPARAM)hIcon);
      }

      if (psInfo->ppshheader.dwFlags & PSH_USEHICON)
        SendMessageW(hwnd, WM_SETICON, 0, (LPARAM)psInfo->ppshheader.hIcon);

      psInfo->strPropertiesFor = strCaption;

      GetWindowTextW(hwnd, psInfo->strPropertiesFor, MAX_CAPTION_LENGTH);

      PROPSHEET_CreateTabControl(hwnd, psInfo);

      PROPSHEET_LoadWizardBitmaps(psInfo);

      if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
      {
        ShowWindow(hwndTabCtrl, SW_HIDE);
        PROPSHEET_AdjustSizeWizard(hwnd, psInfo);
        PROPSHEET_AdjustButtonsWizard(hwnd, psInfo);
        SetFocus(GetDlgItem(hwnd, IDC_NEXT_BUTTON));
      }
      else
      {
        if (PROPSHEET_SizeMismatch(hwnd, psInfo))
        {
          PROPSHEET_AdjustSize(hwnd, psInfo);
          PROPSHEET_AdjustButtons(hwnd, psInfo);
        }
        SetFocus(GetDlgItem(hwnd, IDOK));
      }

      if (IS_INTRESOURCE(psInfo->ppshheader.pszCaption) &&
              psInfo->ppshheader.hInstance)
      {
         WCHAR szText[256];

         if (LoadStringW(psInfo->ppshheader.hInstance,
                         (UINT_PTR)psInfo->ppshheader.pszCaption, szText, 255))
            PROPSHEET_SetTitleW(hwnd, psInfo->ppshheader.dwFlags, szText);
      }
      else
      {
         PROPSHEET_SetTitleW(hwnd, psInfo->ppshheader.dwFlags,
                         psInfo->ppshheader.pszCaption);
      }


      if (psInfo->useCallback)
             (*(psInfo->ppshheader.pfnCallback))(hwnd, PSCB_INITIALIZED, 0);

      idx = psInfo->active_page;
      psInfo->active_page = -1;

      PROPSHEET_SetCurSel(hwnd, idx, 1, psInfo->proppage[idx].hpage);

      /* doing TCM_SETCURSEL seems to be needed even in case of PSH_WIZARD,
       * as some programs call TCM_GETCURSEL to get the current selection
       * from which to switch to the next page */
      SendMessageW(hwndTabCtrl, TCM_SETCURSEL, psInfo->active_page, 0);

      PROPSHEET_UnChanged(hwnd, NULL);

      /* wizards set their focus during init */
      if (psInfo->ppshheader.dwFlags & INTRNL_ANY_WIZARD)
          return FALSE;

      return TRUE;
    }

    case WM_PRINTCLIENT:
    case WM_PAINT:
      PROPSHEET_Paint(hwnd, (HDC)wParam);
      return TRUE;

    case WM_DESTROY:
      PROPSHEET_CleanUp(hwnd);
      return TRUE;

    case WM_CLOSE:
      PROPSHEET_Cancel(hwnd, 1);
      return FALSE; /* let DefDlgProc post us WM_COMMAND/IDCANCEL */

    case WM_COMMAND:
      if (!PROPSHEET_DoCommand(hwnd, LOWORD(wParam)))
      {
          PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);

          if (!psInfo)
              return FALSE;

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
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, !bRet);
        return TRUE;
      }

      return FALSE;
    }
  
    case WM_SYSCOLORCHANGE:
      COMCTL32_RefreshSysColors();
      return FALSE;

    case PSM_GETCURRENTPAGEHWND:
    {
      PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);
      HWND hwndPage = 0;

      if (!psInfo)
        return FALSE;

      if (psInfo->activeValid && psInfo->active_page != -1)
        hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, (DWORD_PTR)hwndPage);

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

      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, (DWORD_PTR)hwndTabCtrl);

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

      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_CANCELTOCLOSE:
    {
      WCHAR buf[MAX_BUTTONTEXT_LENGTH];
      HWND hwndOK = GetDlgItem(hwnd, IDOK);
      HWND hwndCancel = GetDlgItem(hwnd, IDCANCEL);

      EnableWindow(hwndCancel, FALSE);
      if (LoadStringW(COMCTL32_hModule, IDS_CLOSE, buf, ARRAY_SIZE(buf)))
         SetWindowTextW(hwndOK, buf);

      return FALSE;
    }

    case PSM_RESTARTWINDOWS:
    {
      PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);

      if (!psInfo)
        return FALSE;

      /* reboot system takes precedence over restart windows */
      if (psInfo->result != ID_PSREBOOTSYSTEM)
          psInfo->result = ID_PSRESTARTWINDOWS;

      return TRUE;
    }

    case PSM_REBOOTSYSTEM:
    {
      PropSheetInfo* psInfo = GetPropW(hwnd, PropSheetInfoStr);

      if (!psInfo)
        return FALSE;

      psInfo->result = ID_PSREBOOTSYSTEM;

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

      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_QUERYSIBLINGS:
    {
      LRESULT msgResult = PROPSHEET_QuerySiblings(hwnd, wParam, lParam);

      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);

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

      SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_REMOVEPAGE:
      PROPSHEET_RemovePage(hwnd, (int)wParam, (HPROPSHEETPAGE)lParam);
      return TRUE;

    case PSM_ISDIALOGMESSAGE:
    {
       BOOL msgResult = PROPSHEET_IsDialogMessage(hwnd, (LPMSG)lParam);
       SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
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

    case PSM_INSERTPAGE:
    {
        BOOL msgResult = PROPSHEET_InsertPage(hwnd, (HPROPSHEETPAGE)wParam, (HPROPSHEETPAGE)lParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_SETHEADERTITLEW:
        PROPSHEET_SetHeaderTitleW(hwnd, wParam, (LPCWSTR)lParam);
        return TRUE;

    case PSM_SETHEADERTITLEA:
        PROPSHEET_SetHeaderTitleA(hwnd, wParam, (LPCSTR)lParam);
        return TRUE;

    case PSM_SETHEADERSUBTITLEW:
        PROPSHEET_SetHeaderSubTitleW(hwnd, wParam, (LPCWSTR)lParam);
        return TRUE;

    case PSM_SETHEADERSUBTITLEA:
        PROPSHEET_SetHeaderSubTitleA(hwnd, wParam, (LPCSTR)lParam);
        return TRUE;

    case PSM_HWNDTOINDEX:
    {
        LRESULT msgResult = PROPSHEET_HwndToIndex(hwnd, (HWND)wParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_INDEXTOHWND:
    {
        LRESULT msgResult = PROPSHEET_IndexToHwnd(hwnd, (int)wParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_PAGETOINDEX:
    {
        LRESULT msgResult = PROPSHEET_PageToIndex(hwnd, (HPROPSHEETPAGE)wParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_INDEXTOPAGE:
    {
        LRESULT msgResult = PROPSHEET_IndexToPage(hwnd, (int)wParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_IDTOINDEX:
    {
        LRESULT msgResult = PROPSHEET_IdToIndex(hwnd, (int)lParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_INDEXTOID:
    {
        LRESULT msgResult = PROPSHEET_IndexToId(hwnd, (int)wParam);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_GETRESULT:
    {
        LRESULT msgResult = PROPSHEET_GetResult(hwnd);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    case PSM_RECALCPAGESIZES:
    {
        LRESULT msgResult = PROPSHEET_RecalcPageSizes(hwnd);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, msgResult);
        return TRUE;
    }

    default:
      return FALSE;
  }
}
