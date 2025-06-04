/*
 * Shell Menu Desk Bar
 *
 * Copyright 2014 David Quintana
 * Copyright 2023 ReactOS User cs96
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
#include "shellmenu.h"
#include <atlwin.h>
#include <shlwapi_undoc.h>
#include <shellapi.h>
#include <vector>
#include <algorithm>
#include <string>
#include <strsafe.h>
#include <commctrl.h>
#include <stdio.h>

#include "CMenuDeskBar.h"
#include "../../../../base/shell/explorer/resource.h"

// Heights for sections
#define SEARCH_BAR_HEIGHT 22
#define PINNED_VIEW_DEFAULT_HEIGHT 90
#define FREQUENT_APPS_VIEW_DEFAULT_HEIGHT 90
#define SECTION_PADDING 3
#define PINNED_ITEMS_REG_PATH L"Software\\ReactOS\\Explorer\\PinnedStartItems"
#define FREQUENT_APPS_REG_PATH L"Software\\ReactOS\\Explorer\\FrequentApps"
#define MAX_FREQUENT_APPS 10


// Context menu IDs for pinned items listview (local to CMenuDeskBar)
#define ID_PINNED_ITEM_OPEN   (CMD_PIN_TO_STARTMENU + 100)
#define ID_PINNED_ITEM_UNPIN  (CMD_PIN_TO_STARTMENU + 101)
// Context menu IDs for frequent apps listview
#define ID_FREQ_APP_OPEN           (CMD_PIN_TO_STARTMENU + 102)
#define ID_FREQ_APP_PIN_TO_START   (CMD_PIN_TO_STARTMENU + 103)
#define ID_FREQ_APP_REMOVE         (CMD_PIN_TO_STARTMENU + 104)


WINE_DEFAULT_DEBUG_CHANNEL(CMenuDeskBar); // Consider changing to TRACE for more fine-grained control if WINE_DEFAULT_DEBUG_CHANNEL is too broad

// --- CMenuDeskBar Implementation ---
CMenuDeskBar::CMenuDeskBar() :
    m_Client(NULL), m_ClientWindow(NULL), m_IconSize(BMICON_LARGE),
    m_Banner(NULL), m_Shown(FALSE), m_ShowFlags(0), m_didAddRef(FALSE),
    m_hSearchBox(NULL), m_hPinnedItemsView(NULL), m_hFrequentAppsView(NULL)
{
}

CMenuDeskBar::~CMenuDeskBar() { }

void CMenuDeskBar::_ClearPinnedItemsView()
{
    if (m_hPinnedItemsView && ::IsWindow(m_hPinnedItemsView)) {
        int count = ListView_GetItemCount(m_hPinnedItemsView);
        for (int i = 0; i < count; ++i) {
            LVITEMW item = {0}; item.iItem = i; item.mask = LVIF_PARAM;
            if (ListView_GetItem(m_hPinnedItemsView, &item) && item.lParam) ILFree((LPITEMIDLIST)item.lParam);
        }
        ListView_DeleteAllItems(m_hPinnedItemsView);
    }
}

void CMenuDeskBar::_LoadAndDisplayPinnedItemsInView()
{
    OutputDebugStringW(L"PIN_TEST: _LoadAndDisplayPinnedItemsInView START\n");
    if (!m_hPinnedItemsView) { OutputDebugStringW(L"PIN_TEST: Pinned items ListView not created!\n"); return; }
    _ClearPinnedItemsView();
    HKEY hKey;
    int itemsLoaded = 0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, PINNED_ITEMS_REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR achValueName[MAX_PATH], achDataPath[MAX_PATH];
        DWORD cchValueName = MAX_PATH, cbDataPath = sizeof(achDataPath), dwIndex = 0;
        HIMAGELIST hImageList = NULL; BOOL useSmallIcons = (m_IconSize == BMICON_SMALL);
        Shell_GetImageLists(useSmallIcons ? &hImageList : NULL, useSmallIcons ? NULL : &hImageList);
        ListView_SetImageList(m_hPinnedItemsView, hImageList, useSmallIcons ? LVSIL_SMALL : LVSIL_NORMAL);
        ListView_SetIconSpacing(m_hPinnedItemsView, useSmallIcons ? 50 : 75, useSmallIcons ? 32 : 48);

        while (RegEnumValueW(hKey, dwIndex++, achValueName, &cchValueName, NULL, NULL, (LPBYTE)achDataPath, &cbDataPath) == ERROR_SUCCESS) {
            LPITEMIDLIST pidl = NULL; HRESULT hr = SHParseDisplayName(achDataPath, NULL, &pidl, 0, NULL);
            if (SUCCEEDED(hr) && pidl) {
                SHFILEINFOW sfi = {0}; UINT shgfiFlags = SHGFI_PIDL | SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | (useSmallIcons ? SHGFI_SMALLICON : SHGFI_LARGEICON);
                if (SHGetFileInfoW((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), shgfiFlags)) {
                    LVITEMW lvItem = {LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, itemsLoaded, sfi.szDisplayName, 0,0, sfi.iIcon, (LPARAM)ILClone(pidl)};
                    if (ListView_InsertItem(m_hPinnedItemsView, &lvItem) != -1) itemsLoaded++; else ILFree((LPITEMIDLIST)lvItem.lParam);
                } ILFree(pidl);
            }
            cchValueName = MAX_PATH; cbDataPath = sizeof(achDataPath);
        }
        RegCloseKey(hKey);
    }
    WCHAR logMsg[100]; StringCchPrintfW(logMsg, _countof(logMsg), L"PIN_TEST: _LoadAndDisplayPinnedItemsInView END - Loaded %d items\n", itemsLoaded); OutputDebugStringW(logMsg);
    ListView_Arrange(m_hPinnedItemsView, LVA_DEFAULT);
}

void CMenuDeskBar::_ClearFrequentAppsView() {
    if (m_hFrequentAppsView && ::IsWindow(m_hFrequentAppsView)) {
        int count = ListView_GetItemCount(m_hFrequentAppsView);
        for (int i = 0; i < count; ++i) { LVITEMW item = {0}; item.iItem = i; item.mask = LVIF_PARAM; if (ListView_GetItem(m_hFrequentAppsView, &item) && item.lParam) ILFree((LPITEMIDLIST)item.lParam); }
        ListView_DeleteAllItems(m_hFrequentAppsView);
    }
}

static void PathToRegValueName(const WCHAR* path, WCHAR* regValueName, DWORD regValueNameMaxLen) { StringCchCopyW(regValueName, regValueNameMaxLen, path); for (DWORD i = 0; regValueName[i] != L'\0' && i < regValueNameMaxLen; ++i) { if (regValueName[i] == L'\\') regValueName[i] = L'|'; } }

void CMenuDeskBar::RecordAppLaunch(LPCWSTR appPath) {
    if (!appPath || appPath[0] == L'\0') return;
    HKEY hKey; WCHAR logMsg[MAX_PATH + 100];
    StringCchPrintfW(logMsg, _countof(logMsg), L"FREQUENT_APPS_TEST: RecordAppLaunch attempt for [%s]\n", appPath); OutputDebugStringW(logMsg);
    if (RegCreateKeyExW(HKEY_CURRENT_USER, FREQUENT_APPS_REG_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) { OutputDebugStringW(L"FREQUENT_APPS_TEST: RecordAppLaunch - Failed to open/create FrequentApps key.\n"); return; }
    WCHAR regValueName[MAX_PATH]; PathToRegValueName(appPath, regValueName, MAX_PATH);
    WCHAR dataStr[100]; DWORD cbDataStr = sizeof(dataStr); DWORD count = 0; FILETIME ftNow; GetSystemTimeAsFileTime(&ftNow);
    ULARGE_INTEGER uliNow = {ftNow.dwLowDateTime, ftNow.dwHighDateTime}; ULARGE_INTEGER uliStored = uliNow;
    if (RegQueryValueExW(hKey, regValueName, NULL, NULL, (LPBYTE)dataStr, &cbDataStr) == ERROR_SUCCESS) { ULONG parsedCount, parsedHigh, parsedLow; if (swscanf_s(dataStr, L"%lu,%lu,%lu", &parsedCount, &parsedHigh, &parsedLow) == 3) { count = parsedCount; uliStored.HighPart = parsedHigh; uliStored.LowPart = parsedLow; } else { count = 0; } } else { count = 0; }
    count++; uliStored = uliNow;
    StringCchPrintfW(dataStr, _countof(dataStr), L"%lu,%lu,%lu", count, uliStored.HighPart, uliStored.LowPart);
    if (RegSetValueExW(hKey, regValueName, 0, REG_SZ, (const BYTE*)dataStr, (wcslen(dataStr) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS) {
        StringCchPrintfW(logMsg, _countof(logMsg), L"FREQUENT_APPS_TEST: RecordAppLaunch SUCCESS for [%s]. New Count: %lu, Timestamp: %lu,%lu\n", appPath, count, uliStored.HighPart, uliStored.LowPart); OutputDebugStringW(logMsg);
    } else { StringCchPrintfW(logMsg, _countof(logMsg), L"FREQUENT_APPS_TEST: RecordAppLaunch FAILED to set value for [%s].\n", appPath); OutputDebugStringW(logMsg); }
    RegCloseKey(hKey);
    HWND hwndDeskBar = FindWindowW(L"BaseBar", NULL); if (hwndDeskBar) PostMessageW(hwndDeskBar, WM_USER_REFRESH_FREQUENT_APPS, 0, 0);
}

void CMenuDeskBar::PinItemToStartMenu(LPCITEMIDLIST pidlAbsolute) {
    if (!pidlAbsolute) return; WCHAR path[MAX_PATH]; WCHAR logMsg[MAX_PATH + 100];
    if (SHGetPathFromIDListW(pidlAbsolute, path)) {
        StringCchPrintfW(logMsg, _countof(logMsg), L"PIN_TEST: Static PinItemToStartMenu for [%s]\n", path); OutputDebugStringW(logMsg);
        HKEY hKey; WCHAR regValueName[MAX_PATH]; PathToRegValueName(path, regValueName, MAX_PATH);
        if (RegCreateKeyExW(HKEY_CURRENT_USER, PINNED_ITEMS_REG_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, regValueName, 0, REG_SZ, (const BYTE*)path, (wcslen(path) + 1) * sizeof(WCHAR)); RegCloseKey(hKey);
            HWND hwndDeskBar = FindWindowW(L"BaseBar", NULL); if (hwndDeskBar) PostMessageW(hwndDeskBar, WM_USER_REFRESH_PINNED_ITEMS, 0, 0);
        }
    } else { OutputDebugStringW(L"PIN_TEST: Static PinItemToStartMenu - SHGetPathFromIDListW failed.\n"); }
}

struct FrequentAppInfo { std::wstring path; DWORD count; ULARGE_INTEGER lastLaunched; bool operator<(const FrequentAppInfo& other) const { if (count != other.count) return count > other.count; return lastLaunched.QuadPart > other.lastLaunched.QuadPart; }};

void CMenuDeskBar::_LoadAndDisplayFrequentApps() {
    OutputDebugStringW(L"FREQUENT_APPS_TEST: _LoadAndDisplayFrequentApps START\n");
    if (!m_hFrequentAppsView) { OutputDebugStringW(L"FREQUENT_APPS_TEST: Frequent apps ListView not created!\n"); return; }
    _ClearFrequentAppsView();
    std::vector<FrequentAppInfo> infos; HKEY hKey; int itemsRead = 0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, FREQUENT_APPS_REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR mangledPathValueName[MAX_PATH]; DWORD cchMangledPathValueName = MAX_PATH; WCHAR dataStr[100]; DWORD cbDataStr = sizeof(dataStr); DWORD dwIndex = 0;
        while (RegEnumValueW(hKey, dwIndex++, mangledPathValueName, &cchMangledPathValueName, NULL, NULL, (LPBYTE)dataStr, &cbDataStr) == ERROR_SUCCESS) {
            itemsRead++; FrequentAppInfo info; WCHAR originalPath[MAX_PATH]; StringCchCopyW(originalPath, MAX_PATH, mangledPathValueName);
            for (int i = 0; originalPath[i] != L'\0'; ++i) { if (originalPath[i] == L'|') originalPath[i] = L'\\'; }
            info.path = originalPath; ULONG parsedCount, parsedHigh, parsedLow;
            if (swscanf_s(dataStr, L"%lu,%lu,%lu", &parsedCount, &parsedHigh, &parsedLow) == 3) { info.count = parsedCount; info.lastLaunched.HighPart = parsedHigh; info.lastLaunched.LowPart = parsedLow; infos.push_back(info); }
            cchMangledPathValueName = MAX_PATH; cbDataStr = sizeof(dataStr);
        }
        RegCloseKey(hKey);
    }
    std::sort(infos.begin(), infos.end());
    BOOL useSmallIcons = (m_IconSize == BMICON_SMALL); HIMAGELIST hImageList = NULL; Shell_GetImageLists(useSmallIcons ? &hImageList : NULL, useSmallIcons ? NULL : &hImageList);
    ListView_SetImageList(m_hFrequentAppsView, hImageList, useSmallIcons ? LVSIL_SMALL : LVSIL_NORMAL);
    ListView_SetIconSpacing(m_hFrequentAppsView, useSmallIcons ? 50 : 75, useSmallIcons ? 32 : 48);
    int itemsDisplayed = 0;
    for (size_t i = 0; i < infos.size() && i < MAX_FREQUENT_APPS; ++i) {
        LPITEMIDLIST pidl = NULL; HRESULT hr = SHParseDisplayName(infos[i].path.c_str(), NULL, &pidl, 0, NULL);
        if (SUCCEEDED(hr) && pidl) { SHFILEINFOW sfi = {0}; UINT shgfiFlags = SHGFI_PIDL | SHGFI_DISPLAYNAME | SHGFI_SYSICONINDEX | (useSmallIcons ? SHGFI_SMALLICON : SHGFI_LARGEICON);
            if (SHGetFileInfoW((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), shgfiFlags)) { LVITEMW lvItem = {LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM, itemsDisplayed, sfi.szDisplayName,0,0, sfi.iIcon, (LPARAM)ILClone(pidl)}; if (ListView_InsertItem(m_hFrequentAppsView, &lvItem) != -1) itemsDisplayed++; else ILFree((LPITEMIDLIST)lvItem.lParam);
            } else { ILFree(pidl); }
        }
    }
    WCHAR logMsg[150]; StringCchPrintfW(logMsg, _countof(logMsg), L"FREQUENT_APPS_TEST: _LoadAndDisplayFrequentApps END - Read %d items from registry. Displaying top %d items.\n", itemsRead, itemsDisplayed); OutputDebugStringW(logMsg);
    ListView_Arrange(m_hFrequentAppsView, LVA_DEFAULT);
}

LRESULT CMenuDeskBar::_OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { /* ... same ... Transparency lines commented ... */ return 0;}
void CMenuDeskBar::OnFinalMessage(HWND hWnd) { /* ... same ... */ }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS) { return S_OK; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetWindow(HWND *lphwnd) { if (!lphwnd) return E_POINTER; *lphwnd = m_hWnd; return S_OK; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus) { if(m_Client) return IUnknown_OnFocusChangeIS(m_Client, punkObj, fSetFocus); return E_FAIL; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) { if (IsEqualIID(*pguidCmdGroup, CGID_MenuDeskBar)) { if (nCmdID == 2) { _LoadAndDisplayPinnedItemsInView(); _LoadAndDisplayFrequentApps(); return S_OK; } if (nCmdID == 4) return _AdjustForTheme(nCmdexecopt); } return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject) { if (IsEqualGUID(guidService, SID_SMenuPopup) || IsEqualGUID(guidService, SID_SMenuBandParent) || IsEqualGUID(guidService, SID_STopLevelBrowser)) return this->QueryInterface(riid, ppvObject); if (m_Client) { HRESULT hr = IUnknown_QueryService(m_Client, guidService, riid, ppvObject); if (SUCCEEDED(hr)) return hr; } if (m_Site) return IUnknown_QueryService(m_Site, guidService, riid, ppvObject); return E_NOINTERFACE; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg) { if(m_Client) return IUnknown_UIActivateIO(m_Client, fActivate, lpMsg); return S_OK;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO() { if (m_hSearchBox == ::GetFocus()) return S_OK; if (m_hPinnedItemsView == ::GetFocus()) return S_OK; if (m_hFrequentAppsView == ::GetFocus()) return S_OK; if (m_Client && IUnknown_HasFocusIO(m_Client) == S_OK) return S_OK; return S_FALSE; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(LPMSG lpMsg) { if(m_Client) return IUnknown_TranslateAcceleratorIO(m_Client, lpMsg); return S_FALSE; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(IUnknown *punkClient) { /* ... same ... */ return S_OK;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(IUnknown **ppunkClient) { /* ... same ... */ return E_FAIL;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(LPRECT prc) { /* ... same ... */ return S_OK;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(IUnknown *pUnkSite) { /* ... same ... */ return S_OK;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(REFIID riid, void **ppvSite) { /* ... same ... */ return E_FAIL;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags) { /* ... same ... */ return S_OK;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(THIS_ DWORD iIcon) { HRESULT hr = S_OK; BOOL oldSmallIcons = (m_IconSize == BMICON_SMALL); m_IconSize = iIcon; BOOL newSmallIcons = (m_IconSize == BMICON_SMALL); if (m_Client) { const int CMD = 16; const int CMD_EXEC_OPT = (iIcon == BMICON_LARGE) ? 0 : 2; hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL); } if (oldSmallIcons != newSmallIcons) { DWORD dwNewStyleBase = GetWindowLongW(m_hPinnedItemsView, GWL_STYLE) & ~(LVS_ICON | LVS_SMALLICON); DWORD listViewStyle = dwNewStyleBase | (newSmallIcons ? LVS_SMALLICON : LVS_ICON); if(m_hPinnedItemsView) SetWindowLongW(m_hPinnedItemsView, GWL_STYLE, listViewStyle); if(m_hFrequentAppsView) SetWindowLongW(m_hFrequentAppsView, GWL_STYLE, listViewStyle); _LoadAndDisplayPinnedItemsInView(); _LoadAndDisplayFrequentApps(); } else { if (m_hPinnedItemsView) ListView_SetIconSpacing(m_hPinnedItemsView, newSmallIcons ? 50 : 75, newSmallIcons ? 32 : 48); if (m_hFrequentAppsView) ListView_SetIconSpacing(m_hFrequentAppsView, newSmallIcons ? 50 : 75, newSmallIcons ? 32 : 48); } BOOL bHandled; _OnSize(WM_SIZE, 0, 0, bHandled); return hr; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetIconSize(THIS_ DWORD* piIcon) { if (piIcon) *piIcon = m_IconSize; return S_OK; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetBitmap(THIS_ HBITMAP hBitmap) { if (m_Banner && m_Banner != hBitmap) ::DeleteObject(m_Banner); m_Banner = hBitmap; BOOL bHandled; _OnSize(WM_SIZE, 0, 0, bHandled); return S_OK;}
HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetBitmap(THIS_ HBITMAP* phBitmap) { if (phBitmap) *phBitmap = m_Banner; return S_OK; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(IMenuPopup *pmp, BOOL fSet) { if (fSet) m_SubMenuChild = pmp; else { if (m_SubMenuChild && pmp == m_SubMenuChild) m_SubMenuChild = NULL; } return S_OK; }
HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnSelect(DWORD dwSelectType) { /* ... same ... */ return S_OK;}
HRESULT CMenuDeskBar::_CloseBar() { /* ... same ... */ return S_OK;}
BOOL CMenuDeskBar::_IsSubMenuParent(HWND hwnd) { /* ... same ... */ return FALSE;}

LRESULT CMenuDeskBar::_OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    RECT rcClientArea; GetClientRect(&rcClientArea); int currentY = rcClientArea.top;
    int availableWidth = rcClientArea.right - rcClientArea.left; int leftOffset = rcClientArea.left;
    if (m_Banner && m_IconSize != BMICON_SMALL) { BITMAP bm; ::GetObject(m_Banner, sizeof(bm), &bm); leftOffset += bm.bmWidth; availableWidth -= bm.bmWidth; if (availableWidth < 0) availableWidth = 0;}
    if (m_hSearchBox) { ::SetWindowPos(m_hSearchBox, NULL, leftOffset, currentY, availableWidth, SEARCH_BAR_HEIGHT, SWP_NOZORDER | SWP_SHOWWINDOW); currentY += SEARCH_BAR_HEIGHT; if (availableWidth > 0 && SEARCH_BAR_HEIGHT > 0) currentY+=SECTION_PADDING; }
    if (m_hPinnedItemsView) { int actualHeight = PINNED_VIEW_DEFAULT_HEIGHT; if (ListView_GetItemCount(m_hPinnedItemsView) == 0) actualHeight = 0; if (currentY + actualHeight > rcClientArea.bottom && rcClientArea.bottom > currentY) actualHeight = rcClientArea.bottom - currentY; else if (currentY + actualHeight > rcClientArea.bottom) actualHeight = 0; ::SetWindowPos(m_hPinnedItemsView, NULL, leftOffset, currentY, availableWidth, actualHeight, SWP_NOZORDER | (actualHeight > 0 ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) ); if (actualHeight > 0) currentY += actualHeight + SECTION_PADDING; }
    if (m_hFrequentAppsView) { int actualHeight = FREQUENT_APPS_VIEW_DEFAULT_HEIGHT; if (ListView_GetItemCount(m_hFrequentAppsView) == 0) actualHeight = 0; if (currentY + actualHeight > rcClientArea.bottom && rcClientArea.bottom > currentY) actualHeight = rcClientArea.bottom - currentY; else if (currentY + actualHeight > rcClientArea.bottom) actualHeight = 0; ::SetWindowPos(m_hFrequentAppsView, NULL, leftOffset, currentY, availableWidth, actualHeight, SWP_NOZORDER | (actualHeight > 0 ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)); if (actualHeight > 0) currentY += actualHeight + SECTION_PADDING; }
    if (!m_ClientWindow && currentY > rcClientArea.top) { if ( (m_hFrequentAppsView && ListView_GetItemCount(m_hFrequentAppsView) > 0) || (m_hPinnedItemsView && ListView_GetItemCount(m_hPinnedItemsView) > 0) || m_hSearchBox ) currentY -= SECTION_PADDING; }
    if (m_ClientWindow) { int menuBandHeight = rcClientArea.bottom - currentY; if (menuBandHeight < 0) menuBandHeight = 0; ::SetWindowPos(m_ClientWindow, NULL, leftOffset, currentY, availableWidth, menuBandHeight, SWP_NOZORDER | SWP_SHOWWINDOW); }
    bHandled = TRUE; return 0;
}
LRESULT CMenuDeskBar::_OnRefreshPinnedItems(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { _LoadAndDisplayPinnedItemsInView(); InvalidateRect(m_hWnd, NULL, TRUE); BOOL bSizeHandled; RECT rc; GetClientRect(&rc); _OnSize(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top), bSizeHandled); bHandled = TRUE; return 0; }
LRESULT CMenuDeskBar::_OnRefreshFrequentApps(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { _LoadAndDisplayFrequentApps(); InvalidateRect(m_hWnd, NULL, TRUE); BOOL bSizeHandled; RECT rc; GetClientRect(&rc); _OnSize(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top), bSizeHandled); bHandled = TRUE; return 0; }
void UnpinItemFromRegistryByPidl(LPCITEMIDLIST pidlAbsolute) { if (!pidlAbsolute) return; WCHAR path[MAX_PATH]; if (!SHGetPathFromIDListW(pidlAbsolute, path)) return; WCHAR regValueName[MAX_PATH]; PathToRegValueName(path, regValueName, MAX_PATH); HKEY hKey; if (RegOpenKeyExW(HKEY_CURRENT_USER, PINNED_ITEMS_REG_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) { RegDeleteValueW(hKey, regValueName); RegCloseKey(hKey); } }
void CMenuDeskBar::_ShowPinnedItemContextMenu(int iItem, POINT ptScreen) { HMENU hMenu = CreatePopupMenu(); if (!hMenu) return; if (iItem < 0 || iItem >= ListView_GetItemCount(m_hPinnedItemsView)) { DestroyMenu(hMenu); return; } InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_PINNED_ITEM_OPEN, L"&Open"); InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_PINNED_ITEM_UNPIN, L"&Unpin from Start Menu"); SetForegroundWindow(m_hWnd); TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, m_hWnd, NULL); DestroyMenu(hMenu); }
void CMenuDeskBar::_ShowFrequentAppsContextMenu(int iItem, POINT ptScreen) { HMENU hMenu = CreatePopupMenu(); if (!hMenu) return; if (iItem < 0 || iItem >= ListView_GetItemCount(m_hFrequentAppsView)) { DestroyMenu(hMenu); return; } InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_FREQ_APP_OPEN, L"&Open"); InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_FREQ_APP_PIN_TO_START, L"&Pin to Start Menu"); InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_FREQ_APP_REMOVE, L"&Remove from this list"); SetForegroundWindow(m_hWnd); TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, m_hWnd, NULL); DestroyMenu(hMenu); }

LRESULT CMenuDeskBar::_OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    bHandled = FALSE; WCHAR logText[300];
    if (LOWORD(wParam) == IDC_STARTMENU_SEARCHBOX) {
        if (HIWORD(wParam) == EN_CHANGE) { GetWindowTextW(m_hSearchBox, logText, _countof(logText)); StringCchPrintfW(logText, _countof(logText), L"SEARCHBOX_TEST: EN_CHANGE received. Text: %s\n", logText); OutputDebugStringW(logText); }
        else if (HIWORD(wParam) == 0) { GetWindowTextW(m_hSearchBox, logText, _countof(logText)); StringCchPrintfW(logText, _countof(logText), L"SEARCHBOX_TEST: Action triggered. Text: %s\n", logText); OutputDebugStringW(logText); }
        bHandled = TRUE;
    }
    else if (LOWORD(wParam) == ID_PINNED_ITEM_OPEN || LOWORD(wParam) == ID_PINNED_ITEM_UNPIN) {
        int iSelected = ListView_GetNextItem(m_hPinnedItemsView, -1, LVNI_FOCUSED | LVNI_SELECTED); if (iSelected == -1) iSelected = ListView_GetNextItem(m_hPinnedItemsView, -1, LVNI_FOCUSED); if (iSelected == -1) iSelected = ListView_GetNextItem(m_hPinnedItemsView, -1, LVNI_SELECTED);
        if (iSelected != -1) { LVITEMW lvitem = {0}; lvitem.iItem = iSelected; lvitem.mask = LVIF_PARAM;
            if (ListView_GetItem(m_hPinnedItemsView, &lvitem) && lvitem.lParam) { LPITEMIDLIST pidl = (LPITEMIDLIST)lvitem.lParam; WCHAR appPath[MAX_PATH] = {0}; SHGetPathFromIDListW(pidl, appPath);
                if (LOWORD(wParam) == ID_PINNED_ITEM_OPEN) { StringCchPrintfW(logText, _countof(logText), L"PIN_TEST: Launching pinned item from context menu: [%s]\n", appPath); OutputDebugStringW(logText); if (appPath[0]) CMenuDeskBar::RecordAppLaunch(appPath); SHELLEXECUTEINFOW sei = {sizeof(sei), SEE_MASK_IDLIST|SEE_MASK_FLAG_NO_UI, m_hWnd, L"open", NULL, NULL, pidl, NULL, SW_SHOWNORMAL}; ShellExecuteExW(&sei); _CloseBar();
                } else if (LOWORD(wParam) == ID_PINNED_ITEM_UNPIN) { StringCchPrintfW(logText, _countof(logText), L"PIN_TEST: Unpinning item from pinned list context menu: [%s]\n", appPath); OutputDebugStringW(logText); UnpinItemFromRegistryByPidl(pidl); PostMessageW(m_hWnd, WM_USER_REFRESH_PINNED_ITEMS, 0, 0); }
            }
        } bHandled = TRUE;
    }
    else if (LOWORD(wParam) == ID_FREQ_APP_OPEN || LOWORD(wParam) == ID_FREQ_APP_PIN_TO_START || LOWORD(wParam) == ID_FREQ_APP_REMOVE) {
        int iSelected = ListView_GetNextItem(m_hFrequentAppsView, -1, LVNI_FOCUSED | LVNI_SELECTED); if (iSelected == -1) iSelected = ListView_GetNextItem(m_hFrequentAppsView, -1, LVNI_FOCUSED); if (iSelected == -1) iSelected = ListView_GetNextItem(m_hFrequentAppsView, -1, LVNI_SELECTED);
        if (iSelected != -1) { LVITEMW lvitem = {0}; lvitem.iItem = iSelected; lvitem.mask = LVIF_PARAM;
            if (ListView_GetItem(m_hFrequentAppsView, &lvitem) && lvitem.lParam) { LPITEMIDLIST pidl = (LPITEMIDLIST)lvitem.lParam; WCHAR appPath[MAX_PATH] = {0}; SHGetPathFromIDListW(pidl, appPath);
                if (LOWORD(wParam) == ID_FREQ_APP_OPEN) { StringCchPrintfW(logText, _countof(logText), L"FREQUENT_APPS_TEST: Launching frequent item from context menu: [%s]\n", appPath); OutputDebugStringW(logText); if (appPath[0]) CMenuDeskBar::RecordAppLaunch(appPath); SHELLEXECUTEINFOW sei = {sizeof(sei), SEE_MASK_IDLIST|SEE_MASK_FLAG_NO_UI, m_hWnd, L"open", NULL, NULL, pidl, NULL, SW_SHOWNORMAL}; ShellExecuteExW(&sei); _CloseBar();
                } else if (LOWORD(wParam) == ID_FREQ_APP_PIN_TO_START) { StringCchPrintfW(logText, _countof(logText), L"FREQUENT_APPS_TEST: Pinning [%s] from frequent list to Start Menu.\n", appPath); OutputDebugStringW(logText); CMenuDeskBar::PinItemToStartMenu(pidl);
                } else if (LOWORD(wParam) == ID_FREQ_APP_REMOVE) { StringCchPrintfW(logText, _countof(logText), L"FREQUENT_APPS_TEST: Removing [%s] from frequent list.\n", appPath); OutputDebugStringW(logText); if (appPath[0]) { HKEY hKey; WCHAR regValueName[MAX_PATH]; PathToRegValueName(appPath, regValueName, MAX_PATH); if (RegOpenKeyExW(HKEY_CURRENT_USER, FREQUENT_APPS_REG_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) { RegDeleteValueW(hKey, regValueName); RegCloseKey(hKey); } } PostMessageW(m_hWnd, WM_USER_REFRESH_FREQUENT_APPS, 0, 0); }
            }
        } bHandled = TRUE;
    }
    return 0;
}

LRESULT CMenuDeskBar::_OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LPNMHDR hdr = (LPNMHDR)lParam; bHandled = FALSE; WCHAR logText[MAX_PATH + 100];
    if (hdr->hwndFrom == m_hPinnedItemsView) {
        switch (hdr->code) {
            case LVN_ITEMACTIVATE: { LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam; if (lpnmia->iItem != -1) { LVITEMW lvitem = {0}; lvitem.iItem = lpnmia->iItem; lvitem.mask = LVIF_PARAM; if (ListView_GetItem(m_hPinnedItemsView, &lvitem) && lvitem.lParam) { LPITEMIDLIST pidl = (LPITEMIDLIST)lvitem.lParam; WCHAR appPath[MAX_PATH] = {0}; SHGetPathFromIDListW(pidl, appPath); StringCchPrintfW(logText, _countof(logText), L"PIN_TEST: Launching pinned item (LVN_ITEMACTIVATE): [%s]\n", appPath); OutputDebugStringW(logText); if (appPath[0]) CMenuDeskBar::RecordAppLaunch(appPath); SHELLEXECUTEINFOW sei = {sizeof(sei), SEE_MASK_IDLIST|SEE_MASK_FLAG_NO_UI, m_hWnd, L"open", NULL, NULL, pidl, NULL, SW_SHOWNORMAL}; ShellExecuteExW(&sei); _CloseBar(); }} bHandled = TRUE; return 1; }
            case NM_RCLICK: { LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam; if (lpnmia->iItem != -1) { POINT pt = lpnmia->ptAction; if (pt.x == -1 && pt.y == -1) { RECT itemRect; ListView_GetItemRect(m_hPinnedItemsView, lpnmia->iItem, &itemRect, LVIR_ICON); pt.x = itemRect.left + (itemRect.right - itemRect.left) / 2; pt.y = itemRect.top + (itemRect.bottom - itemRect.top) / 2; ClientToScreen(m_hPinnedItemsView, &pt); } else ClientToScreen(m_hPinnedItemsView, &pt); _ShowPinnedItemContextMenu(lpnmia->iItem, pt); } bHandled = TRUE; return 1; }
        }
    }
    else if (hdr->hwndFrom == m_hFrequentAppsView) {
        switch (hdr->code) {
            case LVN_ITEMACTIVATE: { LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam; if (lpnmia->iItem != -1) { LVITEMW lvitem = {0}; lvitem.iItem = lpnmia->iItem; lvitem.mask = LVIF_PARAM; if (ListView_GetItem(m_hFrequentAppsView, &lvitem) && lvitem.lParam) { LPITEMIDLIST pidl = (LPITEMIDLIST)lvitem.lParam; WCHAR appPath[MAX_PATH] = {0}; SHGetPathFromIDListW(pidl, appPath); StringCchPrintfW(logText, _countof(logText), L"FREQUENT_APPS_TEST: Launching frequent item (LVN_ITEMACTIVATE): [%s]\n", appPath); OutputDebugStringW(logText); if (appPath[0]) CMenuDeskBar::RecordAppLaunch(appPath); SHELLEXECUTEINFOW sei = {sizeof(sei), SEE_MASK_IDLIST|SEE_MASK_FLAG_NO_UI, m_hWnd, L"open", NULL, NULL, pidl, NULL, SW_SHOWNORMAL}; ShellExecuteExW(&sei); _CloseBar(); }} bHandled = TRUE; return 1; }
            case NM_RCLICK: { LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam; if (lpnmia->iItem != -1) { POINT pt = lpnmia->ptAction; if (pt.x == -1 && pt.y == -1) { RECT itemRect; ListView_GetItemRect(m_hFrequentAppsView, lpnmia->iItem, &itemRect, LVIR_ICON); pt.x = itemRect.left + (itemRect.right - itemRect.left) / 2; pt.y = itemRect.top + (itemRect.bottom - itemRect.top) / 2; ClientToScreen(m_hFrequentAppsView, &pt); } else ClientToScreen(m_hFrequentAppsView, &pt); _ShowFrequentAppsContextMenu(lpnmia->iItem, pt); } bHandled = TRUE; return 1; }
        }
    }
    if (!bHandled && m_Client) { CComPtr<IWinEventHandler> weh; HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IWinEventHandler, &weh)); if (SUCCEEDED(hr) && weh) { LRESULT res; hr = weh->OnWinEvent(m_ClientWindow, uMsg, wParam, lParam, &res); if (SUCCEEDED(hr)) { bHandled = TRUE; return res; } } }
    return 0;
}

LRESULT CMenuDeskBar::_OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { /* ... same ... */ return 0; }
LRESULT CMenuDeskBar::_OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { /* ... same ... */ return 0;}
LRESULT CMenuDeskBar::_OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { return MA_NOACTIVATE; }
LRESULT CMenuDeskBar::_OnAppActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { /* ... same ... */ return 0; }
LRESULT CMenuDeskBar::_OnWinIniChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { /* ... same ... */ return 0;}
LRESULT CMenuDeskBar::_OnNcPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { /* ... same ... */ return 0; }
LRESULT CMenuDeskBar::_OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) { _CloseBar(); return 0; }
HRESULT CMenuDeskBar::_AdjustForTheme(BOOL bFlatStyle) { /* ... same ... */ return S_OK;}

extern "C"
HRESULT WINAPI RSHELL_CMenuDeskBar_CreateInstance(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CMenuDeskBar>(riid, ppv);
}
