/*
 *	Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <janderwald@reactos.org>
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

///
/// [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\policies\system]
/// "NoInternetOpenWith"=dword:00000001
///

// TODO
// implement duplicate checks in list box
// implement duplicate checks for MRU!
// implement owner drawn menu

typedef struct
{	
    const IContextMenu2Vtbl *lpVtblContextMenu;
	const IShellExtInitVtbl *lpvtblShellExtInit;
    LONG  wId;
    volatile LONG ref;
    BOOL NoOpen;
    UINT count;
    WCHAR szPath[MAX_PATH];
    HMENU hSubMenu;
} SHEOWImpl, *LPSHEOWImpl;

typedef struct
{
    BOOL bMenu;
    HMENU hMenu;
    HWND  hDlgCtrl;
    UINT  Count;
    BOOL NoOpen;
    UINT idCmdFirst;
}OPEN_WITH_CONTEXT, *POPEN_WITH_CONTEXT;

#define MANUFACTURER_NAME_SIZE    100

typedef struct
{
    HICON hIcon;
    WCHAR szAppName[MAX_PATH];
    WCHAR szManufacturer[MANUFACTURER_NAME_SIZE];
}OPEN_ITEM_CONTEXT, *POPEN_ITEM_CONTEXT;


typedef struct _LANGANDCODEPAGE_
  {
    WORD lang;
    WORD code;
} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

typedef struct {
    DWORD cbSize;
    DWORD uMax;
    DWORD fFlags;
    HKEY hKey;
    LPWSTR lpszSubKey;
    PROC lpfnCompare;
} MRUINFO, *LPMRUINFO;

#define MRUF_STRING_LIST 0

typedef int (WINAPI *CREATEMRULISTW)(
    LPMRUINFO lpmi
);

typedef int (WINAPI *ENUMMRULISTW)(
    HANDLE hMRU,
    int nItem,
    void *lpData,
    UINT uLen
);

typedef int (WINAPI *ADDMRUSTRINGW)(
    HANDLE hMRU,
    LPCWSTR szString
);

typedef void (WINAPI *FREEMRULIST)(
    HANDLE hList);

static const IShellExtInitVtbl eivt;
static const IContextMenu2Vtbl cmvt;
static HRESULT WINAPI SHEOWCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj);
static ULONG WINAPI SHEOWCm_fnRelease(IContextMenu2 *iface);

int OpenMRUList(HKEY hKey);
void LoadItemFromHKCU(POPEN_WITH_CONTEXT pContext, WCHAR * szExt);
void LoadItemFromHKCR(POPEN_WITH_CONTEXT pContext, WCHAR * szExt);
void InsertOpenWithItem(POPEN_WITH_CONTEXT pContext, WCHAR * szAppName);

static HMODULE hModule = NULL;
static CREATEMRULISTW CreateMRUListW = NULL;
static ENUMMRULISTW EnumMRUListW = NULL;
static FREEMRULIST FreeMRUList = NULL;
static ADDMRUSTRINGW AddMRUStringW = NULL;


HRESULT WINAPI SHEOW_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv)
{
    SHEOWImpl * ow;
    HRESULT res;

    ow = LocalAlloc(LMEM_ZEROINIT, sizeof(SHEOWImpl));
    if (!ow)
    {
        return E_OUTOFMEMORY;
    }

    ow->ref = 1;
    ow->lpVtblContextMenu = &cmvt;
    ow->lpvtblShellExtInit = &eivt;

    TRACE("(%p)->()\n",ow);

    res = SHEOWCm_fnQueryInterface( (IContextMenu2*)&ow->lpVtblContextMenu, riid, ppv );
    SHEOWCm_fnRelease( (IContextMenu2*)&ow->lpVtblContextMenu );
    return res;
}

static LPSHEOWImpl __inline impl_from_IShellExtInit( IShellExtInit *iface )
{
    return (SHEOWImpl *)((char*)iface - FIELD_OFFSET(SHEOWImpl, lpvtblShellExtInit));
}

static LPSHEOWImpl __inline impl_from_IContextMenu( IContextMenu2 *iface )
{
    return (SHEOWImpl *)((char*)iface - FIELD_OFFSET(SHEOWImpl, lpVtblContextMenu));
}

static HRESULT WINAPI SHEOWCm_fnQueryInterface(IContextMenu2 *iface, REFIID riid, LPVOID *ppvObj)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

     if(IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IContextMenu) ||
        IsEqualIID(riid, &IID_IContextMenu2))
	{
	  *ppvObj = &This->lpVtblContextMenu;
	}
	else if(IsEqualIID(riid, &IID_IShellExtInit))
	{
	  *ppvObj = &This->lpvtblShellExtInit;
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI SHEOWCm_fnAddRef(IContextMenu2 *iface)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static ULONG WINAPI SHEOWCm_fnRelease(IContextMenu2 *iface)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IContextMenu(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

VOID
AddItem(HMENU hMenu, UINT idCmdFirst)
{
    MENUITEMINFOW mii;
    WCHAR szBuffer[MAX_PATH];
    static const WCHAR szChoose[] = { 'C','h','o','o','s','e',' ','P','r','o','g','r','a','m','.','.','.',0 };

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, -1, TRUE, &mii);

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
       wcscpy(szBuffer, szChoose);

    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = idCmdFirst;
    mii.dwTypeData = (LPWSTR)szBuffer;
    mii.cch = wcslen(szBuffer);

    InsertMenuItemW(hMenu, -1, TRUE, &mii);
}

static 
void
LoadOWItems(POPEN_WITH_CONTEXT pContext, LPCWSTR szName)
{
    WCHAR * szExt;
    WCHAR szPath[100];
    DWORD dwPath;

    szExt = wcsrchr(szName, '.');
    if (!szExt)
    {
        /* FIXME
         * show default list of available programs
         */
        return;
    }

    /* load programs directly associated from HKCU */
    LoadItemFromHKCU(pContext, szExt);

    /* load programs associated from HKCR\Extension */
    LoadItemFromHKCR(pContext, szExt);

    /* load programs referenced from HKCR\ProgId */
    dwPath = sizeof(szPath);
    szPath[0] = 0;
    if (RegGetValueW(HKEY_CLASSES_ROOT, szExt, NULL, RRF_RT_REG_SZ, NULL, szPath, &dwPath) == ERROR_SUCCESS)
    {
        szPath[(sizeof(szPath)/sizeof(WCHAR))-1] = L'\0';
        LoadItemFromHKCR(pContext, szPath);
    }
}



static HRESULT WINAPI SHEOWCm_fnQueryContextMenu(
	IContextMenu2 *iface,
	HMENU hmenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    MENUITEMINFOW mii;
    WCHAR szBuffer[100] = {0};
    INT pos;
    HMENU hSubMenu = NULL;
    OPEN_WITH_CONTEXT Context;
    SHEOWImpl *This = impl_from_IContextMenu(iface);
    
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)) < 0)
    {
       TRACE("failed to load string\n");
       return E_FAIL;
    }
    szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';

    hSubMenu = CreatePopupMenu();

    /* set up context */
    ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
    Context.bMenu = TRUE;
    Context.Count = 0;
    Context.hMenu = hSubMenu;
    Context.idCmdFirst = idCmdFirst;
    /* load items */
    LoadOWItems(&Context, This->szPath);
    if (!Context.Count)
    {
        DestroyMenu(hSubMenu);
        hSubMenu = NULL;
        This->wId = 0;
        This->count = 0;
    }
    else
    {
        AddItem(hSubMenu, Context.idCmdFirst++);
        This->count = Context.idCmdFirst - idCmdFirst;
        /* verb start at index zero */
        This->wId = This->count -1;
        This->hSubMenu = hSubMenu;
    }

    pos = GetMenuDefaultItem(hmenu, TRUE, 0) + 1;

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    if (hSubMenu)
    {
       mii.fMask |= MIIM_SUBMENU;
       mii.hSubMenu = hSubMenu;
    }
    mii.dwTypeData = (LPWSTR) szBuffer;
    mii.fState = MFS_ENABLED;
    if (!pos)
    {
        mii.fState |= MFS_DEFAULT;
    }

    mii.wID = Context.idCmdFirst;
    mii.fType = MFT_STRING;
    if (InsertMenuItemW( hmenu, pos, TRUE, &mii))
       Context.Count++;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Context.Count);
}

void
FreeListItems(HWND hwndDlg)
{
    HWND hList;
    LRESULT iIndex, iCount;
    POPEN_ITEM_CONTEXT pContext;

    hList = GetDlgItem(hwndDlg, 14002);
    iCount = SendMessageW(hList, LB_GETCOUNT, 0, 0);
    if (iCount == LB_ERR)
        return;

    for (iIndex = 0; iIndex < iCount; iIndex++)
    {
        pContext = (POPEN_ITEM_CONTEXT)SendMessageW(hList, LB_GETITEMDATA, iIndex, 0);
        if (pContext)
        {
            DestroyIcon(pContext->hIcon);
            SendMessageW(hList, LB_SETITEMDATA, iIndex, (LPARAM)0);
            HeapFree(GetProcessHeap(), 0, pContext);
        }
    }
}

BOOL HideApplicationFromList(WCHAR * pFileName)
{
    WCHAR szBuffer[100] = {'A','p','p','l','i','c','a','t','i','o','n','s','\\',0};
    DWORD dwSize = 0;
    LONG result;

    if (wcslen(pFileName) > (sizeof(szBuffer)/sizeof(WCHAR)) - 14)
    {
        ERR("insufficient buffer\n");
        return FALSE;
    }
    wcscpy(&szBuffer[13], pFileName);

    result = RegGetValueW(HKEY_CLASSES_ROOT, szBuffer, L"NoOpenWith", RRF_RT_REG_SZ, NULL, NULL, &dwSize);

    TRACE("result %d szBuffer %s\n", result, debugstr_w(szBuffer));

    if (result == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}

VOID
WriteStaticShellExtensionKey(HKEY hRootKey, WCHAR * pVerb, WCHAR *pFullPath)
{
    HKEY hShell;
    LONG result;
    WCHAR szBuffer[MAX_PATH+10] = {'s','h','e','l','l','\\', 0 };

    if (wcslen(pVerb) > (sizeof(szBuffer)/sizeof(WCHAR)) - 15 ||
        wcslen(pFullPath) > (sizeof(szBuffer)/sizeof(WCHAR)) - 4)
    {
        ERR("insufficient buffer\n");
        return;
    }

    /* construct verb reg path */
    wcscpy(&szBuffer[6], pVerb);
    wcscat(szBuffer, L"\\command");

    /* create verb reg key */
    if (RegCreateKeyExW(hRootKey, szBuffer, 0, NULL, 0, KEY_WRITE, NULL, &hShell, NULL) != ERROR_SUCCESS)
        return;

    /* build command buffer */
    wcscpy(szBuffer, pFullPath);
    wcscat(szBuffer, L" %1");

    result = RegSetValueExW(hShell, NULL, 0, REG_SZ, (const BYTE*)szBuffer, (wcslen(szBuffer)+1)* sizeof(WCHAR));
    RegCloseKey(hShell);
}

VOID
StoreNewSettings(LPCWSTR szFileName, WCHAR *szAppName)
{
    WCHAR szBuffer[100] = { L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\"};
    WCHAR * pFileExt;
    HKEY hKey;
    LONG result;
    int hList;

    /* get file extension */
    pFileExt = wcsrchr(szFileName, L'.');
    if (wcslen(pFileExt) > (sizeof(szBuffer)/sizeof(WCHAR)) - 60)
    {
        ERR("insufficient buffer\n");
        return;
    }
    wcscpy(&szBuffer[60], pFileExt);
    /* open  base key for this file extension */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, szBuffer, 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;

    /* open mru list */
    hList = OpenMRUList(hKey);

    if (!hList)
    {
        RegCloseKey(hKey);
        return;
    }

    /* insert the entry */
    result = (*AddMRUStringW)((HANDLE)hList, szAppName);

    /* close mru list */
    (*FreeMRUList)((HANDLE)hList);
    /* create mru list key */
    RegCloseKey(hKey);
}

VOID
SetProgrammAsDefaultHandler(LPCWSTR szFileName, WCHAR * szAppName)
{
    HKEY hKey;
    HKEY hAppKey;
    DWORD dwDisposition;
    WCHAR szBuffer[100];
    DWORD dwSize;
    BOOL result;
    WCHAR * pFileExt;
    WCHAR * pFileName;

    /* extract file extension */
    pFileExt = wcsrchr(szFileName, L'.');
    if (!pFileExt)
        return;

    /* create file extension key */
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, pFileExt, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        return;

    if (dwDisposition & REG_CREATED_NEW_KEY)
    {
        /* a new entry was created create the prog key id */
        wcscpy(szBuffer, &pFileExt[1]);
        wcscat(szBuffer, L"_auto_file");
        if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)szBuffer, (wcslen(szBuffer)+1) * sizeof(WCHAR)) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return;
        }
    }
    else
    {
        /* entry already exists fetch prog key id */
        dwSize = sizeof(szBuffer);
        if (RegGetValueW(hKey, NULL, NULL, RRF_RT_REG_SZ, NULL, szBuffer, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return;
        }
    }
    /* close file extension key */
    RegCloseKey(hKey);

    /* create prog id key */
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        return;


    /* check if there already verbs existing for that app */
    pFileName = wcsrchr(szAppName, L'\\');
    wcscpy(szBuffer, L"Classes\\Applications\\");
    wcscat(szBuffer, pFileName);
    wcscat(szBuffer, L"\\shell");
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szBuffer, 0, KEY_READ, &hAppKey) == ERROR_SUCCESS)
    {
        /* copy static verbs from Classes\Applications key */
        HKEY hTemp;
        if (RegCreateKeyExW(hKey, L"shell", 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hTemp, &dwDisposition) == ERROR_SUCCESS)
        {
            result = RegCopyTreeW(hAppKey, NULL, hTemp);
            RegCloseKey(hTemp);
            if (result == ERROR_SUCCESS)
            {
                /* copied all subkeys, we are done */
                RegCloseKey(hKey);
                RegCloseKey(hAppKey);
                return;
            }
        }
        RegCloseKey(hAppKey);
    }
    /* write standard static shell extension */
    WriteStaticShellExtensionKey(hKey, L"open", szAppName);
    RegCloseKey(hKey);
}

void
BrowseForApplication(HWND hwndDlg)
{
    WCHAR szBuffer[30] = {0};
    WCHAR szFilter[30] = {0};
    WCHAR szPath[MAX_PATH];
    OPENFILENAMEW ofn;
    OPEN_WITH_CONTEXT Context;
    INT count;

    /* load resource open with */
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        ofn.lpstrTitle = szBuffer;
        ofn.nMaxFileTitle = wcslen(szBuffer);
    }

    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize  = sizeof(OPENFILENAMEW);
    ofn.hInstance = shell32_hInstance;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.nMaxFile = (sizeof(szPath) / sizeof(WCHAR));
    ofn.lpstrFile = szPath;

    /* load the filter resource string */
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH_FILTER, szFilter, sizeof(szFilter) / sizeof(WCHAR)))
    {
        szFilter[(sizeof(szFilter)/sizeof(WCHAR))-1] = 0;
        ofn.lpstrFilter = szFilter;
    }
    ZeroMemory(szPath, sizeof(szPath));

    /* call openfilename */
    if (!GetOpenFileNameW(&ofn))
        return;

    /* setup context for insert proc */
    ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
    Context.hDlgCtrl = GetDlgItem(hwndDlg, 14002);
    count = SendMessage(Context.hDlgCtrl, LB_GETCOUNT, 0, 0);
    InsertOpenWithItem(&Context, szPath);
    /* select new item */
    SendMessage(Context.hDlgCtrl, LB_SETCURSEL, count, 0);
}

POPEN_ITEM_CONTEXT
GetCurrentOpenItemContext(HWND hwndDlg)
{
     LRESULT result;

    /* get current item */
    result = SendDlgItemMessage(hwndDlg, 14002, LB_GETCURSEL, 0, 0);
    if(result == LB_ERR)
        return NULL;

    /* get item context */
    result = SendDlgItemMessage(hwndDlg, 14002, LB_GETITEMDATA, result, 0);
    if (result == LB_ERR)
        return NULL;

    return (POPEN_ITEM_CONTEXT)result;
}

void
ExecuteOpenItem(POPEN_ITEM_CONTEXT pItemContext, LPCWSTR FileName)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR szPath[(MAX_PATH * 2)];

    /* setup path with argument */
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    wcscpy(szPath, pItemContext->szAppName);
    wcscat(szPath, L" ");
    wcscat(szPath, FileName);

    ERR("path %s\n", debugstr_w(szPath));

    if (CreateProcessW(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        SHAddToRecentDocs(SHARD_PATHW, FileName);
    }
}


static BOOL CALLBACK OpenWithProgrammDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 
    INT index;
    WCHAR szBuffer[MAX_PATH + 30] = { 0 };
    OPENASINFO *poainfo;
    TEXTMETRIC mt;
    COLORREF preColor, preBkColor;
    POPEN_ITEM_CONTEXT pItemContext;
    LONG YOffset;
    OPEN_WITH_CONTEXT Context;

    poainfo = (OPENASINFO*) GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG)lParam);
        poainfo = (OPENASINFO*)lParam;
        if (!(poainfo->oaifInFlags & OAIF_ALLOW_REGISTRATION))
            EnableWindow(GetDlgItem(hwndDlg, 14003), FALSE);
        if (poainfo->oaifInFlags & OAIF_FORCE_REGISTRATION)
            SendDlgItemMessage(hwndDlg, 14003, BM_SETCHECK, BST_CHECKED, 0);
        if (poainfo->oaifInFlags & OAIF_HIDE_REGISTRATION)
            ShowWindow(GetDlgItem(hwndDlg, 14003), SW_HIDE);
        if (poainfo->pcszFile)
        {
             szBuffer[0] = L'\0';
             SendDlgItemMessageW(hwndDlg, 14001, WM_GETTEXT, sizeof(szBuffer), (LPARAM)szBuffer);
             index = wcslen(szBuffer);
             if (index + wcslen(poainfo->pcszFile) + 1 < sizeof(szBuffer)/sizeof(szBuffer[0]))
                 wcscat(szBuffer, poainfo->pcszFile);
             szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
             SendDlgItemMessageW(hwndDlg, 14001, WM_SETTEXT, 0, (LPARAM)szBuffer);
             ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
             Context.hDlgCtrl = GetDlgItem(hwndDlg, 14002);
             LoadOWItems(&Context, poainfo->pcszFile);
             SendMessage(Context.hDlgCtrl, LB_SETCURSEL, 0, 0);
        }
        return TRUE;
    case WM_MEASUREITEM:
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
            lpmis->itemHeight = 64;
            return TRUE; 
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case 14004: /* browse */
            BrowseForApplication(hwndDlg);
            return TRUE;
        case 14002:
            if (HIWORD(wParam) == LBN_SELCHANGE)
                InvalidateRect((HWND)lParam, NULL, TRUE); // FIXME USE UPDATE RECT
            break;
        case 14005: /* ok */
            pItemContext = GetCurrentOpenItemContext(hwndDlg);
            if (pItemContext)
            {
                /* store settings in HKCU path */
                StoreNewSettings(poainfo->pcszFile, pItemContext->szAppName);

                if (SendDlgItemMessage(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    /* set programm as default handler */
                    SetProgrammAsDefaultHandler(poainfo->pcszFile, pItemContext->szAppName);
                }

                if (poainfo->oaifInFlags & OAIF_EXEC)
                    ExecuteOpenItem(pItemContext, poainfo->pcszFile);
            }
            FreeListItems(hwndDlg);
            EndDialog(hwndDlg, 1);
            return TRUE;
        case 14006: /* cancel */
            FreeListItems(hwndDlg);
            EndDialog(hwndDlg, 0);
            return TRUE;
        default:
            break;
        }
        break;
    case WM_DRAWITEM:
        lpdis = (LPDRAWITEMSTRUCT) lParam; 
         if (lpdis->itemID == -1) 
            break; 

         switch (lpdis->itemAction) 
         { 
             case ODA_SELECT: 
             case ODA_DRAWENTIRE:
                 index = SendMessageW(lpdis->hwndItem, LB_GETCURSEL, 0, 0);
                 pItemContext =(POPEN_ITEM_CONTEXT)SendMessage(lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0);

                 if (lpdis->itemID == index)
                 {
                     /* paint focused item with blue background */
                     HBRUSH hBrush;
                     hBrush = CreateSolidBrush(RGB(0, 0, 255));
                     FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
                     DeleteObject(hBrush);
                     preBkColor = SetBkColor(lpdis->hDC, RGB(255, 255, 255));
                 }
                 else
                 {
                     /* paint non focused item with white background */
                     HBRUSH hBrush;
                     hBrush = CreateSolidBrush(RGB(255, 255, 255));
                     FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
                     DeleteObject(hBrush);
                     preBkColor = SetBkColor(lpdis->hDC, RGB(255, 255, 255));
                 }

                 SendMessageW(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM) szBuffer); 
                 /* paint the icon */
                 DrawIconEx(lpdis->hDC, lpdis->rcItem.left,lpdis->rcItem.top, pItemContext->hIcon, 0, 0, 0, NULL, DI_NORMAL);
                 /* get text size */
                 GetTextMetrics(lpdis->hDC, &mt);
                 /* paint app name */
                 YOffset = lpdis->rcItem.top + mt.tmHeight/2;
                 TextOutW(lpdis->hDC, 45, YOffset, szBuffer, wcslen(szBuffer));
                 /* paint manufacturer description */
                 YOffset += mt.tmHeight + 2;
                 preColor = SetTextColor(lpdis->hDC, RGB(192, 192, 192));
                 if (pItemContext->szManufacturer[0])
                     TextOutW(lpdis->hDC, 45, YOffset, pItemContext->szManufacturer, wcslen(pItemContext->szManufacturer));
                 else
                     TextOutW(lpdis->hDC, 45, YOffset, pItemContext->szAppName, wcslen(pItemContext->szAppName));
                 SetTextColor(lpdis->hDC, preColor);
                 SetBkColor(lpdis->hDC, preBkColor);
                 break;
         }
         break;
    default:
        break;
    }
    return FALSE;
}

void
FreeMenuItemContext(HMENU hMenu)
{
    INT Count;
    INT Index;
    MENUITEMINFOW mii;

    /* get item count */
    Count = GetMenuItemCount(hMenu);
    if (Count == -1)
        return;

    /* setup menuitem info */
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA | MIIM_FTYPE;

    for(Index = 0; Index < Count; Index++)
    {
       if (GetMenuItemInfoW(hMenu, Index, TRUE, &mii))
       {
           if ((mii.fType & MFT_SEPARATOR) || mii.dwItemData == 0)
               continue;
           HeapFree(GetProcessHeap(), 0, (LPVOID)mii.dwItemData);
       }
    }
}


static HRESULT WINAPI
SHEOWCm_fnInvokeCommand( IContextMenu2* iface, LPCMINVOKECOMMANDINFO lpici )
{
    MENUITEMINFOW mii;
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    ERR("This %p wId %x count %u verb %x\n", This, This->wId, This->count, LOWORD(lpici->lpVerb));

    if (This->wId < LOWORD(lpici->lpVerb))
        return E_FAIL;

    if (This->wId == LOWORD(lpici->lpVerb))
    {
        OPENASINFO info;

        info.pcszFile = This->szPath;
        info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT | OAIF_EXEC;
        info.pcszClass = NULL;
        FreeMenuItemContext(This->hSubMenu);
        return SHOpenWithDialog(lpici->hwnd, &info);
    }

    /* retrieve menu item info */
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA | MIIM_FTYPE;

    if (GetMenuItemInfoW(This->hSubMenu, LOWORD(lpici->lpVerb), TRUE, &mii))
    {
        POPEN_ITEM_CONTEXT pItemContext = (POPEN_ITEM_CONTEXT)mii.dwItemData;
        if (pItemContext)
        {
            /* launch item with specified app */
            ExecuteOpenItem(pItemContext, This->szPath);
        }
    }
    /* free menu item context */
    FreeMenuItemContext(This->hSubMenu);
    return S_OK;
}

static HRESULT WINAPI
SHEOWCm_fnGetCommandString( IContextMenu2* iface, UINT_PTR idCmd, UINT uType,
                            UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    FIXME("%p %lu %u %p %p %u\n", This,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

static HRESULT WINAPI SHEOWCm_fnHandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    SHEOWImpl *This = impl_from_IContextMenu(iface);

    TRACE("This %p uMsg %x\n",This, uMsg);

    return E_NOTIMPL;
}

static const IContextMenu2Vtbl cmvt =
{
	SHEOWCm_fnQueryInterface,
	SHEOWCm_fnAddRef,
	SHEOWCm_fnRelease,
	SHEOWCm_fnQueryContextMenu,
	SHEOWCm_fnInvokeCommand,
	SHEOWCm_fnGetCommandString,
	SHEOWCm_fnHandleMenuMsg
};

VOID
GetManufacturer(WCHAR * szAppName, POPEN_ITEM_CONTEXT pContext)
{
    UINT VerSize;
    DWORD DummyHandle;
    LPVOID pBuf;
    WORD lang = 0;
    WORD code = 0;
    LPLANGANDCODEPAGE lplangcode;
    WCHAR szBuffer[100];
    WCHAR * pResult;
	BOOL bResult;

    static const WCHAR wFormat[] = L"\\StringFileInfo\\%04x%04x\\CompanyName";
    static const WCHAR wTranslation[] = L"VarFileInfo\\Translation";

    /* query version info size */
    VerSize = GetFileVersionInfoSizeW(szAppName, &DummyHandle);
    if (!VerSize)
    {
        pContext->szManufacturer[0] = 0;
        return;
    }

    /* allocate buffer */
    pBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, VerSize);
    if (!pBuf)
    {
        pContext->szManufacturer[0] = 0;
        return;
    }

    /* query version info */
    if(!GetFileVersionInfoW(szAppName, 0, VerSize, pBuf))
    {
        pContext->szManufacturer[0] = 0;
        HeapFree(GetProcessHeap(), 0, pBuf);
        return;
    }

    /* query lang code */
    if(VerQueryValueW(pBuf, wTranslation, (LPVOID *)&lplangcode, &VerSize))
    {
       /* FIXME find language from current locale / if not available,
        * default to english
        * for now default to first available language
        */
       lang = lplangcode->lang;
       code = lplangcode->code;
    }
    /* set up format */
    swprintf(szBuffer, wFormat, lang, code);
    /* query manufacturer */
     pResult = NULL;
    bResult = VerQueryValueW(pBuf, szBuffer, (LPVOID *)&pResult, &VerSize);

    if (VerSize && bResult && pResult)
        wcscpy(pContext->szManufacturer, pResult);
    else
        pContext->szManufacturer[0] = 0;
    HeapFree(GetProcessHeap(), 0, pBuf);
}




void
InsertOpenWithItem(POPEN_WITH_CONTEXT pContext, WCHAR * szAppName)
{
    MENUITEMINFOW mii;
    POPEN_ITEM_CONTEXT pItemContext;
    LRESULT index;
    WCHAR * Offset;
    WCHAR Buffer[_MAX_FNAME];

    pItemContext = HeapAlloc(GetProcessHeap(), 0, sizeof(OPEN_ITEM_CONTEXT));
    if (!pItemContext)
        return;

    /* store app path */
    wcscpy(pItemContext->szAppName, szAppName);
    /* null terminate it */
    pItemContext->szAppName[MAX_PATH-1] = 0;
    /* extract path name */
    _wsplitpath(szAppName, NULL, NULL, Buffer, NULL);
    Offset = wcsrchr(Buffer, '.');
    if (Offset)
        Offset[0] = L'\0';
    Buffer[0] = towupper(Buffer[0]);

    if (pContext->bMenu)
    {
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
        mii.fType = MFT_STRING; //MFT_OWNERDRAW;
        mii.fState = MFS_ENABLED;
        mii.wID = pContext->idCmdFirst;
        mii.dwTypeData = Buffer;
        mii.cch = wcslen(Buffer);
        mii.dwItemData = (ULONG_PTR)pItemContext;
        wcscpy(pItemContext->szManufacturer, Buffer);
        if (InsertMenuItemW(pContext->hMenu, -1, TRUE, &mii))
        {
            pContext->idCmdFirst++;
            pContext->Count++;
        }
    }
    else
    {
         /* get default icon */
         pItemContext->hIcon = ExtractIconW(shell32_hInstance, szAppName, 0);
         /* get manufacturer */
        GetManufacturer(pItemContext->szAppName, pItemContext);
         index = SendMessageW(pContext->hDlgCtrl, LB_ADDSTRING, 0, (LPARAM)Buffer);
        if (index != LB_ERR)
            SendMessageW(pContext->hDlgCtrl, LB_SETITEMDATA, index, (LPARAM)pItemContext);
    }
}

void
AddItemFromProgIDList(POPEN_WITH_CONTEXT pContext, HKEY hKey)
{
   FIXME("implement me :)))\n");
}

int
OpenMRUList(HKEY hKey)
{
    MRUINFO info;

    if (!hModule)
    {
        WCHAR szPath[MAX_PATH];
        if (!GetSystemDirectoryW(szPath, MAX_PATH))
            return 0;
        PathAddBackslashW(szPath);
        wcscat(szPath, L"comctl32.dll");
        hModule = LoadLibraryExW(szPath, NULL, 0);
    }
    CreateMRUListW = (CREATEMRULISTW)GetProcAddress(hModule, MAKEINTRESOURCEA(400));
    EnumMRUListW = (ENUMMRULISTW)GetProcAddress(hModule, MAKEINTRESOURCEA(403));
    FreeMRUList = (FREEMRULIST)GetProcAddress(hModule, MAKEINTRESOURCEA(152));
    AddMRUStringW = (ADDMRUSTRINGW)GetProcAddress(hModule, MAKEINTRESOURCEA(401));

    if (!CreateMRUListW || !EnumMRUListW || !FreeMRUList || !AddMRUStringW)
        return 0;

    /* initialize mru list info */
    info.cbSize = sizeof(MRUINFO);
    info.uMax = 32;
    info.fFlags = MRUF_STRING_LIST;
    info.hKey = hKey;
    info.lpszSubKey = L"OpenWithList";
    info.lpfnCompare = NULL;

    /* load list */
    return (*CreateMRUListW)(&info);
}

void
AddItemFromMRUList(POPEN_WITH_CONTEXT pContext, HKEY hKey)
{
    int hList;
    int nItem, nCount, nResult;
    WCHAR szBuffer[MAX_PATH];

    /* open mru list */
    hList = OpenMRUList(hKey);
    if (!hList)
        return;

    /* get list count */
    nCount = (*EnumMRUListW)((HANDLE)hList, -1, NULL, 0);

    for(nItem = 0; nItem < nCount; nItem++)
    {
        nResult = (*EnumMRUListW)((HANDLE)hList, nItem, szBuffer, MAX_PATH);
        if (nResult <= 0)
            continue;
        /* make sure its zero terminated */
        szBuffer[min(MAX_PATH-1, nResult)] = '\0';
        /* insert item */
        if (!HideApplicationFromList(szBuffer))
            InsertOpenWithItem(pContext, szBuffer);
    }

    /* free the mru list */
    (*FreeMRUList)((HANDLE)hList);
}



void
LoadItemFromHKCR(POPEN_WITH_CONTEXT pContext, WCHAR * szExt)
{
    HKEY hKey;
    HKEY hSubKey;
    WCHAR szBuffer[MAX_PATH+10];
    WCHAR szResult[100];
    DWORD dwSize;

    static const WCHAR szOpenWithList[] = L"OpenWithList";
    static const WCHAR szOpenWithProgIds[] = L"OpenWithProgIDs";
    static const WCHAR szPerceivedType[] = L"PerceivedType";
    static const WCHAR szSysFileAssoc[] = L"SystemFileAssociations\\%s";

    /* check if extension exists */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szExt, 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return;

    if (RegGetValueW(hKey, NULL, L"NoOpen", RRF_RT_REG_SZ, NULL, NULL, &dwSize) == ERROR_SUCCESS)
    {
        /* display warning dialog */
        pContext->NoOpen = TRUE;
    }

    /* check if there is a directly available execute key */
    if (RegOpenKeyExW(hKey, L"shell\\open\\command", 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        DWORD dwBuffer = sizeof(szBuffer);

        if (RegGetValueW(hSubKey, NULL, NULL, RRF_RT_REG_SZ, NULL, (PVOID)szBuffer, &dwBuffer) == ERROR_SUCCESS)
        {
            WCHAR * Ext = wcsrchr(szBuffer, ' ');
            if (Ext)
            {
                /* erase %1 or extra arguments */
                Ext[0] = 0;
            }
            if(!HideApplicationFromList(szBuffer))
                InsertOpenWithItem(pContext, szBuffer);
        }
        RegCloseKey(hSubKey);
    }

    /* load items from HKCR\Ext\OpenWithList */
    if (RegOpenKeyExW(hKey, szOpenWithList, 0, KEY_READ | KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hKey);
        RegCloseKey(hSubKey);
    }

    /* load items from HKCR\Ext\OpenWithProgIDs */
    if (RegOpenKeyExW(hKey, szOpenWithProgIds, 0, KEY_READ | KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromProgIDList(pContext, hSubKey);
        RegCloseKey(hSubKey);
    }

    /* load items from SystemFileAssociations\Ext key */
    swprintf(szResult, szSysFileAssoc, szExt);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szResult, 0, KEY_READ | KEY_WRITE, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hSubKey);
        RegCloseKey(hSubKey);
    }

    /* load additional items from referenced PerceivedType*/
    dwSize = sizeof(szBuffer);
    if (RegGetValueW(hKey, NULL, szPerceivedType, RRF_RT_REG_SZ, NULL, szBuffer, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }
    RegCloseKey(hKey);

    /* terminate it explictely */
    szBuffer[29] = 0;
    swprintf(szResult, szSysFileAssoc, szBuffer);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szResult, 0, KEY_READ | KEY_WRITE, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hSubKey);
        RegCloseKey(hSubKey);
    }
}

void
LoadItemFromHKCU(POPEN_WITH_CONTEXT pContext, WCHAR * szExt)
{
    WCHAR szBuffer[MAX_PATH];
    HKEY hKey;

    static const WCHAR szOpenWithProgIDs[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s\\OpenWithProgIDs";
    static const WCHAR szOpenWithList[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s";

    /* handle first progid lists */
    swprintf(szBuffer, szOpenWithProgIDs, szExt);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, szBuffer, 0, KEY_READ | KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        AddItemFromProgIDList(pContext, hKey);
        RegCloseKey(hKey);
    }

    /* now handle mru lists */
    swprintf(szBuffer, szOpenWithList, szExt);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, szBuffer, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hKey);
        RegCloseKey(hKey);
    }
}

HRESULT
SHEOW_LoadOpenWithItems(SHEOWImpl *This, IDataObject *pdtobj)
{
    STGMEDIUM medium;
    FORMATETC fmt;
    HRESULT hr;
    LPIDA pida;
    LPCITEMIDLIST pidl_folder;
    LPCITEMIDLIST pidl_child; 
    LPCITEMIDLIST pidl; 
    DWORD dwPath;
    LPWSTR szPtr;
    static const WCHAR szShortCut[] = { '.','l','n','k', 0 };

    fmt.cfFormat = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = IDataObject_GetData(pdtobj, &fmt, &medium);

    if (FAILED(hr))
    {
        ERR("IDataObject_GetData failed with 0x%x\n", hr);
        return hr;
    }

        /*assert(pida->cidl==1);*/
    pida = (LPIDA)GlobalLock(medium.u.hGlobal);

    pidl_folder = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);
    pidl_child = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]);

    pidl = ILCombine(pidl_folder, pidl_child);

    GlobalUnlock(medium.u.hGlobal);
    GlobalFree(medium.u.hGlobal);

    if (!pidl)
    {
        ERR("no mem\n");
        return E_OUTOFMEMORY;
    }
    if (_ILIsDesktop(pidl_child) || _ILIsMyDocuments(pidl_child) || _ILIsControlPanel(pidl_child) || _ILIsNetHood(pidl_child) ||
        _ILIsBitBucket(pidl_child) || _ILIsDrive(pidl_child) || _ILIsCPanelStruct(pidl_child) || _ILIsFolder(pidl_child) || _ILIsControlPanel(pidl_folder))
    {
        TRACE("pidl is a folder\n");
        SHFree((void*)pidl);
        return E_FAIL;
    }

    if (!SHGetPathFromIDListW(pidl, This->szPath))
    {
        SHFree((void*)pidl);
        ERR("SHGetPathFromIDListW failed\n");
        return E_FAIL;
    }
    
    SHFree((void*)pidl);
    TRACE("szPath %s\n", debugstr_w(This->szPath));

    if (GetBinaryTypeW(This->szPath, &dwPath))
    {
        TRACE("path is a executable %x\n", dwPath);
        return E_FAIL;
    }

    szPtr = wcsrchr(This->szPath, '.');
    if (szPtr)
    {
        if (!_wcsicmp(szPtr, szShortCut))
        {
            FIXME("pidl is a shortcut\n");
            return E_FAIL;
        }
    }
    return S_OK;
}

static HRESULT WINAPI
SHEOW_ExtInit_Initialize( IShellExtInit* iface, LPCITEMIDLIST pidlFolder,
                              IDataObject *pdtobj, HKEY hkeyProgID )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);

    TRACE("This %p\n", This);

    return SHEOW_LoadOpenWithItems(This, pdtobj);
}

static ULONG WINAPI SHEOW_ExtInit_AddRef(IShellExtInit *iface)
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}

static ULONG WINAPI SHEOW_ExtInit_Release(IShellExtInit *iface)
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%i)\n", This, refCount + 1);

	if (!refCount)
	{
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

static HRESULT WINAPI
SHEOW_ExtInit_QueryInterface( IShellExtInit* iface, REFIID riid, void** ppvObject )
{
    SHEOWImpl *This = impl_from_IShellExtInit(iface);
    return SHEOWCm_fnQueryInterface((IContextMenu2*)This, riid, ppvObject);
}

static const IShellExtInitVtbl eivt =
{
    SHEOW_ExtInit_QueryInterface,
    SHEOW_ExtInit_AddRef,
    SHEOW_ExtInit_Release,
    SHEOW_ExtInit_Initialize
};


HRESULT WINAPI SHOpenWithDialog(
  HWND hwndParent,
  const OPENASINFO *poainfo
)
{
    MSG msg;
    BOOL bRet;
    HWND hwnd;

    if (poainfo->pcszClass == NULL && poainfo->pcszFile == NULL)
        return E_FAIL;


    hwnd = CreateDialogParam(shell32_hInstance, MAKEINTRESOURCE(OPEN_WITH_PROGRAMM_DLG), hwndParent, OpenWithProgrammDlg, (LPARAM)poainfo);
    if (hwnd == NULL)
    {
        ERR("Failed to create dialog\n");
        return E_FAIL;
    }
    ShowWindow(hwnd, SW_SHOWNORMAL);

    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) 
    { 
        if (!IsWindow(hwnd) || !IsDialogMessage(hwnd, &msg)) 
        {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        }
    }
    return S_OK;
}
