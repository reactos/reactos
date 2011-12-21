/*
 *    Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <janderwald@reactos.org>
 * Copyright 2009 Andrew Hill
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
    BOOL bMenu;
    HMENU hMenu;
    HWND  hDlgCtrl;
    UINT  Count;
    BOOL NoOpen;
    UINT idCmdFirst;
} OPEN_WITH_CONTEXT, *POPEN_WITH_CONTEXT;

#define MANUFACTURER_NAME_SIZE    100

typedef struct
{
    HICON hIcon;
    WCHAR szAppName[MAX_PATH];
    WCHAR szManufacturer[MANUFACTURER_NAME_SIZE];
} OPEN_ITEM_CONTEXT, *POPEN_ITEM_CONTEXT;


typedef struct _LANGANDCODEPAGE_
{
    WORD lang;
    WORD code;
} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

static HANDLE OpenMRUList(HKEY hKey);

static VOID LoadItemFromHKCU(POPEN_WITH_CONTEXT pContext, const WCHAR * szExt);
static VOID LoadItemFromHKCR(POPEN_WITH_CONTEXT pContext, const WCHAR * szExt);
static VOID InsertOpenWithItem(POPEN_WITH_CONTEXT pContext, WCHAR * szAppName);

COpenWithMenu::COpenWithMenu()
{
    count = 0;
    wId = 0;
}

COpenWithMenu::~COpenWithMenu()
{
    TRACE(" destroying IContextMenu(%p)\n", this);
}

static VOID
AddChooseProgramItem(HMENU hMenu, UINT idCmdFirst)
{
    MENUITEMINFOW mii;
    WCHAR szBuffer[MAX_PATH];

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, -1, TRUE, &mii);

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
        wcscpy(szBuffer, L"Choose Program...");

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_STRING;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = idCmdFirst;
    mii.dwTypeData = (LPWSTR)szBuffer;
    mii.cch = wcslen(szBuffer);

    InsertMenuItemW(hMenu, -1, TRUE, &mii);
}

static VOID
LoadOpenWithItems(POPEN_WITH_CONTEXT pContext, LPCWSTR szName)
{
    const WCHAR * szExt;
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

HRESULT WINAPI COpenWithMenu::QueryContextMenu(
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

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        ERR("failed to load string\n");
        return E_FAIL;
    }

    hSubMenu = CreatePopupMenu();

    /* set up context */
    ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
    Context.bMenu = TRUE;
    Context.Count = 0;
    Context.hMenu = hSubMenu;
    Context.idCmdFirst = idCmdFirst;
    /* load items */
    LoadOpenWithItems(&Context, szPath);
    if (!Context.Count)
    {
        DestroyMenu(hSubMenu);
        hSubMenu = NULL;
        wId = 0;
        count = 0;
    }
    else
    {
        AddChooseProgramItem(hSubMenu, Context.idCmdFirst++);
        count = Context.idCmdFirst - idCmdFirst;
        /* verb start at index zero */
        wId = count - 1;
        hSubMenu = hSubMenu;
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
    if (InsertMenuItemW(hmenu, pos, TRUE, &mii))
        Context.Count++;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Context.Count);
}

static VOID
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

static BOOL
HideApplicationFromList(WCHAR *pFileName)
{
    WCHAR szBuffer[100] = L"Applications\\";
    DWORD dwSize = 0;
    LONG result;

    if (wcslen(pFileName) > (sizeof(szBuffer) / sizeof(WCHAR)) - 14)
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

static VOID
WriteStaticShellExtensionKey(HKEY hRootKey, const WCHAR * pVerb, WCHAR *pFullPath)
{
    HKEY hShell;
    LONG result;
    WCHAR szBuffer[MAX_PATH+10] = L"shell\\";

    if (wcslen(pVerb) > (sizeof(szBuffer) / sizeof(WCHAR)) - 15 ||
        wcslen(pFullPath) > (sizeof(szBuffer) / sizeof(WCHAR)) - 4)
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

    result = RegSetValueExW(hShell, NULL, 0, REG_SZ, (const BYTE*)szBuffer, (wcslen(szBuffer) + 1) * sizeof(WCHAR));
    RegCloseKey(hShell);
}

static VOID
StoreNewSettings(LPCWSTR szFileName, WCHAR *szAppName)
{
    WCHAR szBuffer[100] = { L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\"};
    const WCHAR * pFileExt;
    HKEY hKey;
    LONG result;
    HANDLE hList;

    /* get file extension */
    pFileExt = wcsrchr(szFileName, L'.');
    if (wcslen(pFileExt) > (sizeof(szBuffer) / sizeof(WCHAR)) - 60)
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
    result = AddMRUStringW(hList, szAppName);

    /* close mru list */
    FreeMRUList(hList);
    /* create mru list key */
    RegCloseKey(hKey);
}

static VOID
SetProgrammAsDefaultHandler(LPCWSTR szFileName, WCHAR * szAppName)
{
    HKEY hKey;
    HKEY hAppKey;
    DWORD dwDisposition;
    WCHAR szBuffer[100];
    DWORD dwSize;
    BOOL result;
    const WCHAR * pFileExt;
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
        if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)szBuffer, (wcslen(szBuffer) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
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

static VOID
BrowseForApplication(HWND hwndDlg)
{
    WCHAR szBuffer[64] = {0};
    WCHAR szFilter[256] = {0};
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

static POPEN_ITEM_CONTEXT
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

static VOID
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

static INT_PTR CALLBACK
OpenWithProgrammDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                GetDlgItemTextW(hwndDlg, 14001, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
                index = wcslen(szBuffer);
                if (index + wcslen(poainfo->pcszFile) + 1 < sizeof(szBuffer) / sizeof(szBuffer[0]))
                    wcscat(szBuffer, poainfo->pcszFile);
                szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
                SetDlgItemTextW(hwndDlg, 14001, szBuffer);
                ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
                Context.hDlgCtrl = GetDlgItem(hwndDlg, 14002);
                LoadOpenWithItems(&Context, poainfo->pcszFile);
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
                    DestroyWindow(hwndDlg);
                    return TRUE;
                case 14006: /* cancel */
                    FreeListItems(hwndDlg);
                    DestroyWindow(hwndDlg);
                    return TRUE;
                default:
                    break;
            }
            break;
        case WM_DRAWITEM:
            lpdis = (LPDRAWITEMSTRUCT) lParam;
            if ((int)lpdis->itemID == -1)
                break;

            switch (lpdis->itemAction)
            {
                case ODA_SELECT:
                case ODA_DRAWENTIRE:
                    index = SendMessageW(lpdis->hwndItem, LB_GETCURSEL, 0, 0);
                    pItemContext = (POPEN_ITEM_CONTEXT)SendMessage(lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0);

                    if ((int)lpdis->itemID == index)
                    {
                        /* paint focused item with standard background colour */
                        HBRUSH hBrush;
                        hBrush = CreateSolidBrush(RGB(46, 104, 160));
                        FillRect(lpdis->hDC, &lpdis->rcItem, hBrush);
                        DeleteObject(hBrush);
                        preBkColor = SetBkColor(lpdis->hDC, RGB(46, 104, 160));
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
                    DrawIconEx(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, pItemContext->hIcon, 0, 0, 0, NULL, DI_NORMAL);
                    /* get text size */
                    GetTextMetrics(lpdis->hDC, &mt);
                    /* paint app name */
                    YOffset = lpdis->rcItem.top + mt.tmHeight / 2;
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
        case WM_CLOSE:
            FreeListItems(hwndDlg);
            DestroyWindow(hwndDlg);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

static VOID
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

HRESULT WINAPI
COpenWithMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    MENUITEMINFOW mii;

    ERR("This %p wId %x count %u verb %x\n", this, wId, count, LOWORD(lpici->lpVerb));

    if (HIWORD(lpici->lpVerb) != 0 || LOWORD(lpici->lpVerb) > wId)
        return E_FAIL;

    if (wId == LOWORD(lpici->lpVerb))
    {
        OPENASINFO info;

        info.pcszFile = szPath;
        info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT | OAIF_EXEC;
        info.pcszClass = NULL;
        FreeMenuItemContext(hSubMenu);
        return SHOpenWithDialog(lpici->hwnd, &info);
    }

    /* retrieve menu item info */
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA | MIIM_FTYPE;

    if (GetMenuItemInfoW(hSubMenu, LOWORD(lpici->lpVerb), TRUE, &mii))
    {
        POPEN_ITEM_CONTEXT pItemContext = (POPEN_ITEM_CONTEXT)mii.dwItemData;
        if (pItemContext)
        {
            /* launch item with specified app */
            ExecuteOpenItem(pItemContext, szPath);
        }
    }
    /* free menu item context */
    FreeMenuItemContext(hSubMenu);
    return S_OK;
}

HRESULT WINAPI
COpenWithMenu::GetCommandString(UINT_PTR idCmd, UINT uType,
                                UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

HRESULT WINAPI COpenWithMenu::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE("This %p uMsg %x\n", this, uMsg);

    return E_NOTIMPL;
}

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
    if(VerQueryValueW(pBuf, const_cast<LPWSTR>(wTranslation), (LPVOID *)&lplangcode, &VerSize))
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

static VOID
InsertOpenWithItem(POPEN_WITH_CONTEXT pContext, WCHAR * szAppName)
{
    MENUITEMINFOW mii;
    POPEN_ITEM_CONTEXT pItemContext;
    LRESULT index;
    WCHAR *pwszExt;
    WCHAR Buffer[_MAX_FNAME];

    pItemContext = (OPEN_ITEM_CONTEXT *)HeapAlloc(GetProcessHeap(), 0, sizeof(OPEN_ITEM_CONTEXT));
    if (!pItemContext)
        return;

    /* store app path */
    wcscpy(pItemContext->szAppName, szAppName);
    /* extract path name */
    _wsplitpath(szAppName, NULL, NULL, Buffer, NULL);
    pwszExt = wcsrchr(Buffer, '.');
    if (pwszExt)
        pwszExt[0] = L'\0';
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

static VOID
AddItemFromProgIDList(POPEN_WITH_CONTEXT pContext, HKEY hKey)
{
    FIXME("implement me :)))\n");
}

static HANDLE
OpenMRUList(HKEY hKey)
{
    CREATEMRULISTW info;

    /* initialize mru list info */
    info.cbSize = sizeof(info);
    info.nMaxItems = 32;
    info.dwFlags = MRU_STRING;
    info.hKey = hKey;
    info.lpszSubKey = L"OpenWithList";
    info.lpfnCompare = NULL;

    /* load list */
    return CreateMRUListW(&info);
}

static VOID
AddItemFromMRUList(POPEN_WITH_CONTEXT pContext, HKEY hKey)
{
    HANDLE hList;
    int nItem, nCount, nResult;
    WCHAR szBuffer[MAX_PATH];

    /* open mru list */
    hList = OpenMRUList(hKey);
    if (!hList)
    {
        ERR("OpenMRUList failed\n");
        return;
    }

    /* get list count */
    nCount = EnumMRUListW(hList, -1, NULL, 0);

    for(nItem = 0; nItem < nCount; nItem++)
    {
        nResult = EnumMRUListW(hList, nItem, szBuffer, MAX_PATH);
        if (nResult <= 0)
            continue;
        /* make sure its zero terminated */
        szBuffer[min(MAX_PATH-1, nResult)] = '\0';
        /* insert item */
        if (!HideApplicationFromList(szBuffer))
            InsertOpenWithItem(pContext, szBuffer);
    }

    /* free the mru list */
    FreeMRUList(hList);
}

static VOID
LoadItemFromHKCR(POPEN_WITH_CONTEXT pContext, const WCHAR * szExt)
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
            WCHAR * pszSpace = wcsrchr(szBuffer, ' ');
            if (pszSpace)
            {
                /* erase %1 or extra arguments */
                *pszSpace = 0; // FIXME: what about '"'
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

static VOID
LoadItemFromHKCU(POPEN_WITH_CONTEXT pContext, const WCHAR * szExt)
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

HRESULT WINAPI
COpenWithMenu::Initialize(LPCITEMIDLIST pidlFolder,
                          IDataObject *pdtobj,
                          HKEY hkeyProgID)
{
    STGMEDIUM medium;
    FORMATETC fmt;
    HRESULT hr;
    LPIDA pida;
    LPCITEMIDLIST pidlFolder2;
    LPCITEMIDLIST pidlChild;
    LPCITEMIDLIST pidl;
    LPWSTR pszExt;
    
    TRACE("This %p\n", this);

    if (pdtobj == NULL)
        return E_INVALIDARG;

    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = pdtobj->GetData(&fmt, &medium);

    if (FAILED(hr))
    {
        ERR("IDataObject_GetData failed with 0x%x\n", hr);
        return hr;
    }

    pida = (LPIDA)GlobalLock(medium.hGlobal);
    ASSERT(pida->cidl >= 1);

    pidlFolder2 = (LPCITEMIDLIST) ((LPBYTE)pida + pida->aoffset[0]);
    pidlChild = (LPCITEMIDLIST) ((LPBYTE)pida + pida->aoffset[1]);

    pidl = ILCombine(pidlFolder2, pidlChild);

    GlobalUnlock(medium.hGlobal);
    GlobalFree(medium.hGlobal);

    if (!pidl)
    {
        ERR("no mem\n");
        return E_OUTOFMEMORY;
    }
    if (_ILIsDesktop(pidlChild) || _ILIsMyDocuments(pidlChild) || _ILIsControlPanel(pidlChild) ||
        _ILIsNetHood(pidlChild) || _ILIsBitBucket(pidlChild) || _ILIsDrive(pidlChild) ||
        _ILIsCPanelStruct(pidlChild) || _ILIsFolder(pidlChild) || _ILIsMyComputer(pidlChild))
    {
        TRACE("pidl is a folder\n");
        SHFree((void*)pidl);
        return E_FAIL;
    }

    if (!SHGetPathFromIDListW(pidl, szPath))
    {
        IID *iid = _ILGetGUIDPointer(pidl);
        SHFree((void*)pidl);
        ERR("SHGetPathFromIDListW failed %s\n", iid ? shdebugstr_guid(iid) : "");
        return E_FAIL;
    }

    SHFree((void*)pidl);
    TRACE("szPath %s\n", debugstr_w(szPath));

    pszExt = wcsrchr(szPath, L'.');

    if (pszExt)
    {
        if (!_wcsicmp(pszExt, L".exe") || !_wcsicmp(pszExt, L".lnk"))
        {
            TRACE("path is a executable or shortcut\n");
            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT WINAPI
SHOpenWithDialog(HWND hwndParent,
                 const OPENASINFO *poainfo)
{
    MSG msg;
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

    while (GetMessage(&msg, NULL, 0, 0) != 0 && IsWindow(hwnd))
    {
        if (!IsDialogMessage(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return S_OK;
}
