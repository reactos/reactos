/*
 * Copyright 2005 James Hawkins
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#ifndef HHCTRL_H
#define HHCTRL_H

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winuser.h"
#include "winnls.h"
#include "htmlhelp.h"
#include "ole2.h"
#include "exdisp.h"
#include "mshtmhst.h"
#include "commctrl.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "wine/itss.h"
#include "wine/list.h"

#define WB_GOBACK     0
#define WB_GOFORWARD  1
#define WB_GOHOME     2
#define WB_SEARCH     3
#define WB_REFRESH    4
#define WB_STOP       5
#define WB_PRINT      6

typedef struct {
    LPWSTR chm_file;
    LPWSTR chm_index;
} ChmPath;

typedef struct ContentItem {
    struct ContentItem *parent;
    struct ContentItem *child;
    struct ContentItem *next;

    HTREEITEM id;

    LPWSTR name;
    LPWSTR local;
    ChmPath merge;
} ContentItem;

typedef struct IndexSubItem {
    LPWSTR name;
    LPWSTR local;
} IndexSubItem;

typedef struct IndexItem {
    struct IndexItem *next;

    HTREEITEM id;
    LPWSTR keyword;
    ChmPath merge;

    int nItems;
    int itemFlags;
    int indentLevel;
    IndexSubItem *items;
} IndexItem;

typedef struct SearchItem {
    struct SearchItem *next;

    HTREEITEM id;
    LPWSTR title;
    LPWSTR filename;
} SearchItem;

typedef struct CHMInfo
{
    IITStorage *pITStorage;
    IStorage *pStorage;
    WCHAR *szFile;

    IStream *strings_stream;
    char **strings;
    DWORD strings_size;

    WCHAR *compiledFile;
    WCHAR *defWindow;
    WCHAR *defTopic;
    WCHAR *defTitle;
    WCHAR *defToc;

    UINT codePage;
} CHMInfo;

#define TAB_CONTENTS   0
#define TAB_INDEX      1
#define TAB_SEARCH     2
#define TAB_FAVORITES  3
#define TAB_NUMTABS    TAB_FAVORITES

typedef struct {
    HWND hwnd;
    DWORD id;
} HHTab;

typedef struct {
    HWND hwndList;
    HWND hwndPopup;
    HWND hwndCallback;
} IndexPopup;

typedef struct {
    SearchItem *root;
    HWND hwndEdit;
    HWND hwndList;
    HWND hwndContainer;
} SearchTab;

typedef struct {
    HIMAGELIST hImageList;
} ContentsTab;

struct wintype_stringsW {
    WCHAR *pszType;
    WCHAR *pszCaption;
    WCHAR *pszToc;
    WCHAR *pszIndex;
    WCHAR *pszFile;
    WCHAR *pszHome;
    WCHAR *pszJump1;
    WCHAR *pszJump2;
    WCHAR *pszUrlJump1;
    WCHAR *pszUrlJump2;
    WCHAR *pszCustomTabs;
};

struct wintype_stringsA {
    char *pszType;
    char *pszCaption;
    char *pszToc;
    char *pszIndex;
    char *pszFile;
    char *pszHome;
    char *pszJump1;
    char *pszJump2;
    char *pszUrlJump1;
    char *pszUrlJump2;
    char *pszCustomTabs;
};

typedef struct {
    IOleClientSite IOleClientSite_iface;
    IOleInPlaceSite IOleInPlaceSite_iface;
    IOleInPlaceFrame IOleInPlaceFrame_iface;
    IDocHostUIHandler IDocHostUIHandler_iface;

    LONG ref;

    IOleObject *ole_obj;
    IWebBrowser2 *web_browser;
    HWND hwndWindow;
} WebBrowserContainer;

typedef struct {
    WebBrowserContainer *web_browser;

    HH_WINTYPEW WinType;

    struct wintype_stringsA stringsA;
    struct wintype_stringsW stringsW;

    struct list entry;
    CHMInfo *pCHMInfo;
    ContentItem *content;
    IndexItem *index;
    IndexPopup popup;
    SearchTab search;
    ContentsTab contents;
    HWND hwndTabCtrl;
    HWND hwndSizeBar;
    HFONT hFont;

    HHTab tabs[TAB_FAVORITES+1];
    int viewer_initialized;
    DWORD current_tab;
} HHInfo;

BOOL InitWebBrowser(HHInfo*,HWND);
void ReleaseWebBrowser(HHInfo*);
void ResizeWebBrowser(HHInfo*,DWORD,DWORD);
void DoPageAction(WebBrowserContainer*,DWORD);

void InitContent(HHInfo*);
void ReleaseContent(HHInfo*);
void ActivateContentTopic(HWND,LPCWSTR,ContentItem *);

void InitIndex(HHInfo*);
void ReleaseIndex(HHInfo*);

CHMInfo *OpenCHM(LPCWSTR szFile);
BOOL LoadWinTypeFromCHM(HHInfo *info);
CHMInfo *CloseCHM(CHMInfo *pCHMInfo);
void SetChmPath(ChmPath*,LPCWSTR,LPCWSTR);
IStream *GetChmStream(CHMInfo*,LPCWSTR,ChmPath*);
LPWSTR FindContextAlias(CHMInfo*,DWORD);
WCHAR *GetDocumentTitle(CHMInfo*,LPCWSTR);

extern struct list window_list;
HHInfo *CreateHelpViewer(HHInfo*,LPCWSTR,HWND);
void ReleaseHelpViewer(HHInfo*);
#ifdef __REACTOS__
LPWSTR HH_LoadString(DWORD dwID);
#endif
BOOL NavigateToUrl(HHInfo*,LPCWSTR);
BOOL NavigateToChm(HHInfo*,LPCWSTR,LPCWSTR);
void MergeChmProperties(HH_WINTYPEW*,HHInfo*,BOOL);
void UpdateHelpWindow(HHInfo *info);

void InitSearch(HHInfo *info, const char *needle);
void ReleaseSearch(HHInfo *info);

LPCWSTR skip_schema(LPCWSTR url);
void wintype_stringsA_free(struct wintype_stringsA *stringsA);
void wintype_stringsW_free(struct wintype_stringsW *stringsW);
WCHAR *decode_html(const char *html_fragment, int html_fragment_len, UINT code_page);
HHInfo *find_window(const WCHAR *window);

/* memory allocation functions */

static inline LPWSTR strdupnAtoW(LPCSTR str, LONG lenA)
{
    LPWSTR ret;
    DWORD len;

    if(!str)
        return NULL;

    if (lenA > 0)
    {
        /* find length of string */
        LPCSTR eos = memchr(str, 0, lenA);
	if (eos) lenA = eos - str;
    }

    len = MultiByteToWideChar(CP_ACP, 0, str, lenA, NULL, 0)+1; /* +1 for null pad */
    ret = malloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, lenA, ret, len);
    ret[len-1] = 0;

    return ret;
}

static inline LPWSTR strdupAtoW(LPCSTR str)
{
    return strdupnAtoW(str, -1);
}

static inline LPSTR strdupWtoA(LPCWSTR str)
{
    LPSTR ret;
    DWORD len;

    if(!str)
        return NULL;

    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ret = malloc(len);
    WideCharToMultiByte(CP_ACP, 0, str, -1, ret, len, NULL, NULL);
    return ret;
}


extern HINSTANCE hhctrl_hinstance;
extern BOOL hh_process;

#endif
