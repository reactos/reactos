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
    HWND hDlgCtrl;
    BOOL bNoOpen;
    UINT idCmd;
} OPEN_WITH_CONTEXT, *POPEN_WITH_CONTEXT;

#define MANUFACTURER_NAME_SIZE 100

typedef struct
{
    HICON hIcon;
    WCHAR wszAppPath[MAX_PATH];
    WCHAR wszManufacturer[MANUFACTURER_NAME_SIZE];
} OPEN_ITEM_CONTEXT, *POPEN_ITEM_CONTEXT;

typedef struct _LANGANDCODEPAGE_
{
    WORD lang;
    WORD code;
} LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

static HANDLE OpenMRUList(HKEY hKey);

static VOID LoadItemFromHKCU(POPEN_WITH_CONTEXT pContext, LPCWSTR pwszExt);
static VOID LoadItemFromHKCR(POPEN_WITH_CONTEXT pContext, LPCWSTR pwszExt);
static VOID InsertOpenWithItem(POPEN_WITH_CONTEXT pContext, LPCWSTR pwszAppName);
static VOID FreeMenuItemContext(HMENU hMenu);

COpenWithMenu::COpenWithMenu()
{
    m_idCmdFirst = 0;
    m_idCmdLast = 0;
}

COpenWithMenu::~COpenWithMenu()
{
    TRACE("Destroying IContextMenu(%p)\n", this);
    if (m_hSubMenu)
        FreeMenuItemContext(m_hSubMenu);
}

static VOID
FreeMenuItemContext(HMENU hMenu)
{
    INT Count, Index;
    MENUITEMINFOW mii;

    /* get item count */
    Count = GetMenuItemCount(hMenu);
    if (Count == -1)
        return;

    /* setup menuitem info */
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_CHECKMARKS;

    for(Index = 0; Index < Count; Index++)
    {
        if (GetMenuItemInfoW(hMenu, Index, TRUE, &mii))
        {
            if (mii.dwItemData)
                HeapFree(GetProcessHeap(), 0, (LPVOID)mii.dwItemData);
            if (mii.hbmpChecked)
                DeleteObject(mii.hbmpChecked);
        }
    }
}

static HBITMAP
IconToBitmap(HICON hIcon)
{
    HDC hdc, hdcScr;
    HBITMAP hbm, hbmOld;
    RECT rc;

    hdcScr = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScr);
    SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK));
    hbm = CreateCompatibleBitmap(hdcScr, rc.right, rc.bottom);
    ReleaseDC(NULL, hdcScr);

    hbmOld = (HBITMAP)SelectObject(hdc, hbm);
    FillRect(hdc, &rc, (HBRUSH)(COLOR_MENU + 1));
    if(!DrawIconEx(hdc, 0, 0, hIcon, rc.right, rc.bottom, 0, NULL, DI_NORMAL))
        ERR("DrawIcon failed: %x\n", GetLastError());
    SelectObject(hdc, hbmOld);

    DeleteDC(hdc);

    return hbm;
}

static VOID
AddChooseProgramItem(HMENU hMenu, UINT idCmd)
{
    MENUITEMINFOW mii;
    WCHAR wszBuf[128];

    TRACE("hMenu %p idCmd %u\n", hMenu, idCmd);

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, -1, TRUE, &mii);

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
    {
        ERR("Failed to load string\n");
        return;
    }

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = idCmd;
    mii.dwTypeData = (LPWSTR)wszBuf;
    mii.cch = wcslen(wszBuf);

    InsertMenuItemW(hMenu, -1, TRUE, &mii);
}

static VOID
LoadOpenWithItems(POPEN_WITH_CONTEXT pContext, LPCWSTR szName)
{
    const WCHAR *pwszExt;
    WCHAR wszPath[100];
    DWORD dwSize;

    pwszExt = wcsrchr(szName, '.');
    if (!pwszExt)
    {
        /* FIXME: show default list of available programs */
        return;
    }

    /* load programs directly associated from HKCU */
    LoadItemFromHKCU(pContext, pwszExt);

    /* load programs associated from HKCR\Extension */
    LoadItemFromHKCR(pContext, pwszExt);

    /* load programs referenced from HKCR\ProgId */
    dwSize = sizeof(wszPath);
    if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, NULL, RRF_RT_REG_SZ, NULL, wszPath, &dwSize) == ERROR_SUCCESS)
        LoadItemFromHKCR(pContext, wszPath);
}

HRESULT WINAPI COpenWithMenu::QueryContextMenu(
    HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    MENUITEMINFOW mii;
    WCHAR wszBuf[100];
    INT DefaultPos;
    HMENU hSubMenu;
    OPEN_WITH_CONTEXT Context;

    TRACE("hMenu %p indexMenu %u idFirst %u idLast %u uFlags %u\n", hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
    {
        ERR("Failed to load string\n");
        return E_FAIL;
    }

    hSubMenu = CreatePopupMenu();

    /* set up context */
    ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
    Context.bMenu = TRUE;
    Context.hMenu = hSubMenu;
    Context.idCmd = idCmdFirst;

    /* load items */
    LoadOpenWithItems(&Context, m_wszPath);
    if (Context.idCmd == idCmdFirst)
    {
        /* No items has been added */
        DestroyMenu(hSubMenu);
        m_hSubMenu = NULL;
        m_idCmdFirst = idCmdFirst;
        m_idCmdLast = idCmdFirst;
    }
    else
    {
        AddChooseProgramItem(hSubMenu, Context.idCmd);
        m_idCmdFirst = idCmdFirst;
        m_idCmdLast = Context.idCmd;
        m_hSubMenu = hSubMenu;
    }

    DefaultPos = GetMenuDefaultItem(hMenu, TRUE, 0);

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    if (m_hSubMenu)
    {
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = m_hSubMenu;
        mii.wID = -1;
    }
    else
        mii.wID = m_idCmdLast;

    mii.fType = MFT_STRING;
    mii.dwTypeData = (LPWSTR)wszBuf;
    mii.cch = wcslen(wszBuf);

    mii.fState = MFS_ENABLED;
    if (DefaultPos == -1)
        mii.fState |= MFS_DEFAULT;

    InsertMenuItemW(hMenu, DefaultPos + 1, TRUE, &mii);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, m_idCmdLast - idCmdFirst + 1);
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
IsAppHidden(LPCWSTR pwszFileName)
{
    WCHAR wszBuf[100];
    DWORD dwSize = 0;
    LONG result;

    if (FAILED(StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Applications\\%s", pwszFileName)))
    {
        ERR("insufficient buffer\n");
        return FALSE;
    }

    result = RegGetValueW(HKEY_CLASSES_ROOT, wszBuf, L"NoOpenWith", RRF_RT_REG_SZ, NULL, NULL, &dwSize);

    TRACE("result %d wszBuf %s\n", result, wszBuf);

    if (result == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}

static VOID
WriteStaticShellExtensionKey(HKEY hRootKey, LPCWSTR pwszVerb, LPCWSTR pwszFullPath)
{
    HKEY hShell;
    WCHAR wszBuf[MAX_PATH];

    /* construct verb reg path */
    if (FAILED(StringCbPrintfW(wszBuf, sizeof(wszBuf), L"shell\\%s\\command", pwszVerb)))
    {
        ERR("insufficient buffer\n");
        return;
    }

    /* create verb reg key */
    if (RegCreateKeyExW(hRootKey, wszBuf, 0, NULL, 0, KEY_WRITE, NULL, &hShell, NULL) != ERROR_SUCCESS)
        return;

    /* build command buffer */
    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s %%1", pwszFullPath);

    RegSetValueExW(hShell, NULL, 0, REG_SZ, (const BYTE*)wszBuf, (wcslen(wszBuf) + 1) * sizeof(WCHAR));
    RegCloseKey(hShell);
}

static VOID
StoreNewSettings(LPCWSTR pwszFileName, LPCWSTR pwszAppName)
{
    WCHAR wszBuf[100];
    LPCWSTR pwszExt;
    HKEY hKey;
    HANDLE hList;

    /* Get file extension */
    pwszExt = wcsrchr(pwszFileName, L'.');
    if (!pwszExt)
        return;

    /* Build registry key */
    if (FAILED(StringCbPrintfW(wszBuf, sizeof(wszBuf),
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s",
                               pwszExt)))
    {
        ERR("insufficient buffer\n");
        return;
    }

    /* Open base key for this file extension */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, wszBuf, 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;

    /* Open MRU list */
    hList = OpenMRUList(hKey);
    if (hList)
    {
        /* Insert the entry */
        AddMRUStringW(hList, pwszAppName);

        /* Close MRU list */
        FreeMRUList(hList);
    }

    RegCloseKey(hKey);
}

static VOID
SetProgramAsDefaultHandler(LPCWSTR pwszFileName, LPCWSTR pwszAppName)
{
    HKEY hKey;
    HKEY hAppKey;
    DWORD dwDisposition;
    WCHAR wszBuf[100];
    DWORD dwSize;
    BOOL result;
    LPCWSTR pwszExt;
    LPCWSTR pwszAppFileName;

    /* Extract file extension */
    pwszExt = wcsrchr(pwszFileName, L'.');
    if (!pwszExt)
        return;

    /* Create file extension key */
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        return;

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        /* A new entry was created create the prog key id */
        StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s_auto_file", pwszExt + 1);
        if (RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)wszBuf, (wcslen(wszBuf) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return;
        }
    }
    else
    {
        /* Entry already exists fetch prog key id */
        dwSize = sizeof(wszBuf);
        if (RegGetValueW(hKey, NULL, NULL, RRF_RT_REG_SZ, NULL, wszBuf, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return;
        }
    }

    /* Close file extension key */
    RegCloseKey(hKey);

    /* Create prog id key */
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        return;

    /* Check if there already verbs existing for that app */
    pwszAppFileName = wcsrchr(pwszAppName, L'\\');
    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Classes\\Applications\\%s\\shell", pwszAppFileName);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hAppKey) == ERROR_SUCCESS)
    {
        /* Copy static verbs from Classes\Applications key */
        HKEY hDestKey;
        if (RegCreateKeyExW(hKey, L"shell", 0, NULL, 0, KEY_WRITE, NULL, &hDestKey, &dwDisposition) == ERROR_SUCCESS)
        {
            result = RegCopyTreeW(hAppKey, NULL, hDestKey);
            RegCloseKey(hDestKey);
            if (result == ERROR_SUCCESS)
            {
                /* Copied all subkeys, we are done */
                RegCloseKey(hKey);
                RegCloseKey(hAppKey);
                return;
            }
        }
        RegCloseKey(hAppKey);
    }

    /* Write standard static shell extension */
    WriteStaticShellExtensionKey(hKey, L"open", pwszAppName);
    RegCloseKey(hKey);
}

static VOID
BrowseForApplication(HWND hwndDlg)
{
    WCHAR wszTitle[64];
    WCHAR wszFilter[256];
    WCHAR wszPath[MAX_PATH];
    OPENFILENAMEW ofn;
    OPEN_WITH_CONTEXT Context;
    INT iCount;

    /* Initialize OPENFILENAMEW structure */
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize  = sizeof(OPENFILENAMEW);
    ofn.hInstance = shell32_hInstance;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.nMaxFile = (sizeof(wszPath) / sizeof(WCHAR));
    ofn.lpstrFile = wszPath;

    /* Init title */
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, wszTitle, sizeof(wszTitle) / sizeof(WCHAR)))
    {
        ofn.lpstrTitle = wszTitle;
        ofn.nMaxFileTitle = wcslen(wszTitle);
    }

    /* Init the filter string */
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH_FILTER, wszFilter, sizeof(wszFilter) / sizeof(WCHAR)))
        ofn.lpstrFilter = wszFilter;
    ZeroMemory(wszPath, sizeof(wszPath));

    /* Create OpenFile dialog */
    if (!GetOpenFileNameW(&ofn))
        return;

    /* Setup context for insert proc */
    ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
    Context.hDlgCtrl = GetDlgItem(hwndDlg, 14002);
    iCount = SendMessage(Context.hDlgCtrl, LB_GETCOUNT, 0, 0);
    InsertOpenWithItem(&Context, wszPath);

    /* Select new item */
    SendMessage(Context.hDlgCtrl, LB_SETCURSEL, iCount, 0);
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
ExecuteOpenItem(POPEN_ITEM_CONTEXT pItemContext, LPCWSTR pwszPath)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR wszBuf[MAX_PATH * 2 + 8];

    /* setup path with argument */
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);

    /* Build the command line. Don't use applcation name as first parameter of
       CreateProcessW, because it have to be an absolute path. */
    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"\"%s\" \"%s\"", pItemContext->wszAppPath, pwszPath);

    /* Start the application now */
    TRACE("AppName %ls Path %ls\n", pItemContext->wszAppPath, pwszPath);
    if (CreateProcessW(NULL, wszBuf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        SHAddToRecentDocs(SHARD_PATHW, pwszPath);
    }
}

static INT_PTR CALLBACK
OpenWithProgrammDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            OPENASINFO *poainfo;

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
                WCHAR wszBuf[MAX_PATH];
                UINT cchBuf;
                OPEN_WITH_CONTEXT Context;

                cchBuf = GetDlgItemTextW(hwndDlg, 14001, wszBuf, sizeof(wszBuf) / sizeof(wszBuf[0]));
                StringCchCopyW(wszBuf, _countof(wszBuf) - cchBuf, poainfo->pcszFile);
                SetDlgItemTextW(hwndDlg, 14001, wszBuf);

                ZeroMemory(&Context, sizeof(OPEN_WITH_CONTEXT));
                Context.hDlgCtrl = GetDlgItem(hwndDlg, 14002);
                LoadOpenWithItems(&Context, poainfo->pcszFile);

                SendMessage(Context.hDlgCtrl, LB_SETCURSEL, 0, 0);
            }
            return TRUE;
        }
        case WM_MEASUREITEM:
        {
            LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
            lpmis->itemHeight = 64;
            return TRUE;
        }
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
                {
                    POPEN_ITEM_CONTEXT pItemContext;
                    
                    pItemContext = GetCurrentOpenItemContext(hwndDlg);
                    if (pItemContext)
                    {
                        OPENASINFO *poainfo = (OPENASINFO*)GetWindowLongPtr(hwndDlg, DWLP_USER);

                        /* store settings in HKCU path */
                        StoreNewSettings(poainfo->pcszFile, pItemContext->wszAppPath);

                        if (SendDlgItemMessage(hwndDlg, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            /* set programm as default handler */
                            SetProgramAsDefaultHandler(poainfo->pcszFile, pItemContext->wszAppPath);
                        }

                        if (poainfo->oaifInFlags & OAIF_EXEC)
                            ExecuteOpenItem(pItemContext, poainfo->pcszFile);
                    }
                    FreeListItems(hwndDlg);
                    DestroyWindow(hwndDlg);
                    return TRUE;
                }
                case 14006: /* cancel */
                    FreeListItems(hwndDlg);
                    DestroyWindow(hwndDlg);
                    return TRUE;
                default:
                    break;
            }
            break;
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
            TEXTMETRIC mt;
            COLORREF preColor, preBkColor;
            LONG cyOffset;
            POPEN_ITEM_CONTEXT pItemContext;
            INT Index;
            WCHAR wszBuf[MAX_PATH];

            if ((int)lpdis->itemID == -1)
                break;

            switch (lpdis->itemAction)
            {
                case ODA_SELECT:
                case ODA_DRAWENTIRE:
                    Index = SendMessageW(lpdis->hwndItem, LB_GETCURSEL, 0, 0);
                    pItemContext = (POPEN_ITEM_CONTEXT)SendMessage(lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0);

                    if ((int)lpdis->itemID == Index)
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

                    SendMessageW(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)wszBuf);
                    /* paint the icon */
                    DrawIconEx(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, pItemContext->hIcon, 0, 0, 0, NULL, DI_NORMAL);
                    /* get text size */
                    GetTextMetrics(lpdis->hDC, &mt);
                    /* paint app name */
                    cyOffset = lpdis->rcItem.top + mt.tmHeight / 2;
                    TextOutW(lpdis->hDC, 45, cyOffset, wszBuf, wcslen(wszBuf));
                    /* paint manufacturer description */
                    cyOffset += mt.tmHeight + 2;
                    preColor = SetTextColor(lpdis->hDC, RGB(192, 192, 192));
                    if (pItemContext->wszManufacturer[0])
                        TextOutW(lpdis->hDC, 45, cyOffset, pItemContext->wszManufacturer, wcslen(pItemContext->wszManufacturer));
                    else
                        TextOutW(lpdis->hDC, 45, cyOffset, pItemContext->wszAppPath, wcslen(pItemContext->wszAppPath));
                    SetTextColor(lpdis->hDC, preColor);
                    SetBkColor(lpdis->hDC, preBkColor);
                    break;
            }
            break;
        }
        case WM_CLOSE:
            FreeListItems(hwndDlg);
            DestroyWindow(hwndDlg);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

HRESULT WINAPI
COpenWithMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;

    TRACE("This %p idFirst %u idLast %u idCmd %u\n", this, m_idCmdFirst, m_idCmdLast, m_idCmdFirst + LOWORD(lpici->lpVerb));

    if (HIWORD(lpici->lpVerb) == 0 && m_idCmdFirst + LOWORD(lpici->lpVerb) <= m_idCmdLast)
    {
        if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdLast)
        {
            OPENASINFO info;

            info.pcszFile = m_wszPath;
            info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT | OAIF_EXEC;
            info.pcszClass = NULL;
            hr = SHOpenWithDialog(lpici->hwnd, &info);
        }
        else
        {
            /* retrieve menu item info */
            MENUITEMINFOW mii;
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA | MIIM_FTYPE;

            if (GetMenuItemInfoW(m_hSubMenu, LOWORD(lpici->lpVerb), TRUE, &mii) && mii.dwItemData)
            {
                /* launch item with specified app */
                POPEN_ITEM_CONTEXT pItemContext = (POPEN_ITEM_CONTEXT)mii.dwItemData;
                ExecuteOpenItem(pItemContext, m_wszPath);
                hr = S_OK;
            }
        }
    }

    return hr;
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

static VOID
LoadFileInfo(LPCWSTR pwszAppPath, POPEN_ITEM_CONTEXT pContext, LPWSTR pwszAppName, unsigned cchAppName)
{
    UINT cbSize;
    LPVOID pBuf;
    WORD wLang = 0, wCode = 0;
    LPLANGANDCODEPAGE lpLangCode;
    WCHAR wszBuf[100];
    WCHAR *pResult;

    /* Clear manufacturer */
    pContext->wszManufacturer[0] = 0;

    /* query version info size */
    cbSize = GetFileVersionInfoSizeW(pwszAppPath, NULL);
    if (!cbSize)
        return;

    /* allocate buffer */
    pBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbSize);
    if (!pBuf)
        return;

    /* query version info */
    if(!GetFileVersionInfoW(pwszAppPath, 0, cbSize, pBuf))
    {
        HeapFree(GetProcessHeap(), 0, pBuf);
        return;
    }

    /* query lang code */
    if(VerQueryValueW(pBuf, L"VarFileInfo\\Translation", (LPVOID*)&lpLangCode, &cbSize))
    {
        /* FIXME: find language from current locale / if not available,
         * default to english
         * for now default to first available language
         */
        wLang = lpLangCode->lang;
        wCode = lpLangCode->code;
    }

    /* Query name */
    swprintf(wszBuf, L"\\StringFileInfo\\%04x%04x\\FileDescription", wLang, wCode);
    if (VerQueryValueW(pBuf, wszBuf, (LPVOID *)&pResult, &cbSize))
        StringCchCopyNW(pwszAppName, cchAppName, pResult, cbSize);

    /* Query manufacturer */
    swprintf(wszBuf, L"\\StringFileInfo\\%04x%04x\\CompanyName", wLang, wCode);

    if (VerQueryValueW(pBuf, wszBuf, (LPVOID *)&pResult, &cbSize))
        StringCbCopyNW(pContext->wszManufacturer, sizeof(pContext->wszManufacturer), pResult, cbSize);
    HeapFree(GetProcessHeap(), 0, pBuf);
}

static VOID
InsertOpenWithItem(POPEN_WITH_CONTEXT pContext, LPCWSTR pwszAppPath)
{
    POPEN_ITEM_CONTEXT pItemContext;
    WCHAR wszAppName[256];

    /* Allocate new item context */
    pItemContext = (OPEN_ITEM_CONTEXT *)HeapAlloc(GetProcessHeap(), 0, sizeof(OPEN_ITEM_CONTEXT));
    if (!pItemContext)
        return;

    /* Store app path and icon */
    wcscpy(pItemContext->wszAppPath, pwszAppPath);
    pItemContext->hIcon = ExtractIconW(shell32_hInstance, pwszAppPath, 0);
    LoadFileInfo(pwszAppPath, pItemContext, wszAppName, _countof(wszAppName));

    /* Add item to the list */
    if (pContext->bMenu)
    {
        MENUITEMINFOW mii;

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA | MIIM_CHECKMARKS;
        mii.fType = MFT_STRING; //MFT_OWNERDRAW;
        mii.fState = MFS_ENABLED;
        mii.wID = pContext->idCmd;
        mii.dwTypeData = wszAppName;
        mii.cch = wcslen(wszAppName);
        mii.dwItemData = (ULONG_PTR)pItemContext;
        mii.hbmpChecked = mii.hbmpUnchecked = IconToBitmap(pItemContext->hIcon);

        if (InsertMenuItemW(pContext->hMenu, -1, TRUE, &mii))
            pContext->idCmd++;
    }
    else
    {
        LRESULT Index;

        Index = SendMessageW(pContext->hDlgCtrl, LB_ADDSTRING, 0, (LPARAM)wszAppName);
        if (Index != LB_ERR)
            SendMessageW(pContext->hDlgCtrl, LB_SETITEMDATA, Index, (LPARAM)pItemContext);
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
    CREATEMRULISTW Info;

    /* initialize mru list info */
    Info.cbSize = sizeof(Info);
    Info.nMaxItems = 32;
    Info.dwFlags = MRU_STRING;
    Info.hKey = hKey;
    Info.lpszSubKey = L"OpenWithList";
    Info.lpfnCompare = NULL;

    /* load list */
    return CreateMRUListW(&Info);
}

static VOID
AddItemFromMRUList(POPEN_WITH_CONTEXT pContext, HKEY hKey)
{
    HANDLE hList;
    int nItem, nCount, nResult;
    WCHAR wszBuf[MAX_PATH];

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
        nResult = EnumMRUListW(hList, nItem, wszBuf, _countof(wszBuf));
        if (nResult <= 0)
            continue;

        /* insert item */
        if (!IsAppHidden(wszBuf))
            InsertOpenWithItem(pContext, wszBuf);
    }

    /* free the mru list */
    FreeMRUList(hList);
}

static VOID
LoadItemFromHKCR(POPEN_WITH_CONTEXT pContext, LPCWSTR pwszExt)
{
    HKEY hKey;
    HKEY hSubKey;
    WCHAR wszBuf[MAX_PATH], wszBuf2[MAX_PATH];
    DWORD dwSize;

    /* Check if extension exists */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    if (RegGetValueW(hKey, NULL, L"NoOpen", RRF_RT_REG_SZ, NULL, NULL, &dwSize) == ERROR_SUCCESS)
    {
        /* Display warning dialog */
        pContext->bNoOpen = TRUE;
    }

    /* Check if there is a directly available execute key */
    dwSize = sizeof(wszBuf);
    if (RegGetValueW(hKey, L"shell\\open\\command", NULL, RRF_RT_REG_SZ, NULL, (PVOID)wszBuf, &dwSize) == ERROR_SUCCESS)
    {
        WCHAR *pwszSpace = wcsrchr(wszBuf, L' ');
        if (pwszSpace)
        {
            /* Erase %1 or extra arguments */
            *pwszSpace = 0; // FIXME: what about '"'
        }
        if(!IsAppHidden(wszBuf))
            InsertOpenWithItem(pContext, wszBuf);
    }

    /* Load items from HKCR\Ext\OpenWithList */
    if (RegOpenKeyExW(hKey, L"OpenWithList", 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hKey);
        RegCloseKey(hSubKey);
    }

    /* Load items from HKCR\Ext\OpenWithProgIDs */
    if (RegOpenKeyExW(hKey, L"OpenWithProgIDs", 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromProgIDList(pContext, hSubKey);
        RegCloseKey(hSubKey);
    }

    /* Load items from SystemFileAssociations\Ext key */
    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"SystemFileAssociations\\%s", pwszExt);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hSubKey);
        RegCloseKey(hSubKey);
    }

    /* Load additional items from referenced PerceivedType*/
    dwSize = sizeof(wszBuf);
    if (RegGetValueW(hKey, NULL, L"PerceivedType", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }
    RegCloseKey(hKey);

    StringCbPrintfW(wszBuf2, sizeof(wszBuf2), L"SystemFileAssociations\\%s", wszBuf);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf2, 0, KEY_READ | KEY_WRITE, &hSubKey) == ERROR_SUCCESS)
    {
        AddItemFromMRUList(pContext, hSubKey);
        RegCloseKey(hSubKey);
    }
}

static VOID
LoadItemFromHKCU(POPEN_WITH_CONTEXT pContext, LPCWSTR pwszExt)
{
    WCHAR wszBuf[MAX_PATH];
    HKEY hKey;

    /* Handle ProgId lists first */
    StringCbPrintfW(wszBuf, sizeof(wszBuf),
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s\\OpenWithProgIDs",
                    pwszExt);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, wszBuf, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        AddItemFromProgIDList(pContext, hKey);
        RegCloseKey(hKey);
    }

    /* Now handle MRU lists */
    StringCbPrintfW(wszBuf, sizeof(wszBuf),
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s",
                    pwszExt);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, wszBuf, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
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
    LPCWSTR pwszExt;
    
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

    if (!SHGetPathFromIDListW(pidl, m_wszPath))
    {
        IID *iid = _ILGetGUIDPointer(pidl);
        SHFree((void*)pidl);
        ERR("SHGetPathFromIDListW failed %s\n", iid ? shdebugstr_guid(iid) : "");
        return E_FAIL;
    }

    SHFree((void*)pidl);
    TRACE("szPath %s\n", debugstr_w(m_wszPath));

    pwszExt = wcsrchr(m_wszPath, L'.');
    if (pwszExt)
    {
        if (!_wcsicmp(pwszExt, L".exe") || !_wcsicmp(pwszExt, L".lnk"))
        {
            TRACE("file is a executable or shortcut\n");
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

    while (GetMessage(&msg, NULL, 0, 0) && IsWindow(hwnd))
    {
        if (!IsDialogMessage(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return S_OK;
}
