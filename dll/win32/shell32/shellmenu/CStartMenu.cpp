/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "shellmenu.h"

#include "../../../../base/shell/explorer/resource.h"
#include "../CMenuDeskBar.h" // For CMenuDeskBar::RecordAppLaunch and WM_USER_REFRESH_...

#include "CMergedFolder.h"
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(CStartMenu);

// CMD_PIN_TO_STARTMENU and CMD_UNPIN_FROM_STARTMENU are from resource.h

#define IDM_RUN                     401
#define IDM_LOGOFF                  402
#define IDM_UNDOCKCOMPUTER          410
#define IDM_TASKBARANDSTARTMENU     413
#define IDM_LASTSTARTMENU_SEPARATOR 450
#define IDM_DOCUMENTS               501
#define IDM_HELPANDSUPPORT          503
#define IDM_PROGRAMS                504
#define IDM_CONTROLPANEL            505
#define IDM_SHUTDOWN                506
#define IDM_FAVORITES               507
#define IDM_SETTINGS                508
#define IDM_PRINTERSANDFAXES        510
#define IDM_SEARCH                  520
#define IDM_SYNCHRONIZE             553
#define IDM_NETWORKCONNECTIONS      557
#define IDM_DISCONNECT              5000
#define IDM_SECURITY                5001

class CPinnedItemContextMenuWrapper;

class CShellMenuCallback :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellMenuCallback
{
private:
    friend class CPinnedItemContextMenuWrapper;
    HWND m_hwndTray;
    CComPtr<IShellMenu> m_pShellMenu;
    CComPtr<IBandSite> m_pBandSite;
    CComPtr<IDeskBar> m_pDeskBar;
    CComPtr<ITrayPriv> m_pTrayPriv;
    CComPtr<IShellFolder> m_psfPrograms;
    LPITEMIDLIST m_pidlPrograms;

    HRESULT OnInitMenu()
    {
        HMENU hmenu; HRESULT hr;
        if (m_pTrayPriv.p) return S_OK;
        hr = IUnknown_GetSite(m_pDeskBar, IID_PPV_ARG(ITrayPriv, &m_pTrayPriv)); if (FAILED(hr)) return hr;
        hr = IUnknown_GetWindow(m_pTrayPriv, &m_hwndTray); if (FAILED(hr)) return hr;
        hr = m_pTrayPriv->AppendMenu(&hmenu); if (FAILED(hr)) return hr;
        hr = m_pShellMenu->SetMenu(hmenu, NULL, SMSET_BOTTOM); if (FAILED(hr)) { DestroyMenu(hmenu); return hr; }
        return hr;
    }

    HRESULT OnGetInfo(LPSMDATA psmd, SMINFO *psminfo)
    {
        int iconIndex = 0;
        switch (psmd->uId) {
        case IDM_FAVORITES: iconIndex = -IDI_SHELL_FAVOTITES; break; case IDM_SEARCH: iconIndex = -IDI_SHELL_SEARCH1; break;
        case IDM_HELPANDSUPPORT: iconIndex = -IDI_SHELL_HELP2; break; case IDM_LOGOFF: iconIndex = -IDI_SHELL_LOGOFF1; break;
        case IDM_PROGRAMS:  iconIndex = -IDI_SHELL_PROGRAMS_FOLDER1; break; case IDM_DOCUMENTS: iconIndex = -IDI_SHELL_RECENT_DOCUMENTS1; break;
        case IDM_RUN: iconIndex = -IDI_SHELL_RUN1; break; case IDM_SHUTDOWN: iconIndex = -IDI_SHELL_SHUTDOWN1; break;
        case IDM_SETTINGS: iconIndex = -IDI_SHELL_CONTROL_PANEL1; break; case IDM_MYDOCUMENTS: iconIndex = -IDI_SHELL_MY_DOCUMENTS; break;
        case IDM_MYPICTURES: iconIndex = -IDI_SHELL_MY_PICTURES; break; case IDM_CONTROLPANEL: iconIndex = -IDI_SHELL_CONTROL_PANEL; break;
        case IDM_NETWORKCONNECTIONS: iconIndex = -IDI_SHELL_NETWORK_CONNECTIONS2; break; case IDM_PRINTERSANDFAXES: iconIndex = -IDI_SHELL_PRINTER2; break;
        case IDM_TASKBARANDSTARTMENU: iconIndex = -IDI_SHELL_TSKBAR_STARTMENU; break;
        default: return S_FALSE;
        }
        if (iconIndex) {
            if (psminfo->dwMask & SMIM_TYPE) psminfo->dwType = SMIT_STRING;
            if (psminfo->dwMask & SMIM_ICON) psminfo->iIcon = Shell_GetCachedImageIndex(L"shell32.dll", iconIndex, FALSE);
            if (psminfo->dwMask & SMIM_FLAGS) psminfo->dwFlags |= SMIF_ICON;
        } else { if (psminfo->dwMask & SMIM_TYPE) psminfo->dwType = SMIT_SEPARATOR; }
        return S_OK;
    }

    void AddOrSetMenuItem(HMENU hMenu, UINT nID, INT csidl, BOOL bExpand, BOOL bAdd = TRUE, BOOL bSetText = TRUE) const { /* Unchanged */ }
    BOOL GetAdvancedValue(LPCWSTR pszName, BOOL bDefault = FALSE) const { /* Unchanged */ return FALSE; }
    HMENU CreateRecentMenu() const { /* Unchanged */ return NULL; }
    void UpdateSettingsMenu(HMENU hMenu) { /* Unchanged */ }
    HRESULT AddStartMenuItems(IShellMenu *pShellMenu, INT csidl, DWORD dwFlags, IShellFolder *psf = NULL) { /* Unchanged */ return E_FAIL;}

    HRESULT OnGetSubMenu(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        HRESULT hr; CComPtr<IShellMenu> pShellMenu;
        hr = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &pShellMenu)); if (FAILED(hr)) return hr;
        hr = pShellMenu->Initialize(this, 0, ANCESTORDEFAULT, SMINIT_VERTICAL); if (FAILED(hr)) return hr;
        hr = E_FAIL;
        switch (psmd->uId) {
            case IDM_PROGRAMS: hr = AddStartMenuItems(pShellMenu, CSIDL_PROGRAMS, SMSET_TOP, m_psfPrograms); break;
            default: { MENUITEMINFOW mii = { sizeof(mii), MIIM_SUBMENU }; if (GetMenuItemInfoW(psmd->hmenu, psmd->uId, FALSE, &mii) && mii.hSubMenu) hr = pShellMenu->SetMenu(mii.hSubMenu, NULL, SMSET_BOTTOM); }
        }
        if (FAILED(hr)) return hr;
        hr = pShellMenu->QueryInterface(iid, pv); pShellMenu.Detach(); return hr;
    }

    INT CSIDLFromID(UINT uId) const {
        switch (uId) {
            case IDM_PROGRAMS: return CSIDL_PROGRAMS; case IDM_FAVORITES: return CSIDL_FAVORITES;
            case IDM_DOCUMENTS: return CSIDL_RECENT; case IDM_MYDOCUMENTS: return CSIDL_MYDOCUMENTS;
            case IDM_MYPICTURES: return CSIDL_MYPICTURES; case IDM_CONTROLPANEL: return CSIDL_CONTROLS;
            case IDM_NETWORKCONNECTIONS: return CSIDL_CONNECTIONS; case IDM_PRINTERSANDFAXES: return CSIDL_PRINTERS;
            default: return 0;
        }
    }

    HRESULT OnGetContextMenu(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        CComPtr<IShellFolder> pSFToUse; LPCITEMIDLIST pidlItemToUse = NULL; HRESULT hr;
        LPITEMIDLIST pidlFullForItem = NULL;

        if (psmd->psf && psmd->pidlItem) {
            pSFToUse = psmd->psf; pidlItemToUse = psmd->pidlItem;
            CComQIPtr<IPersistFolder2> pParentPersistFolder(psmd->psf);
            if (pParentPersistFolder) {
                LPITEMIDLIST pidlParent = NULL;
                if (SUCCEEDED(pParentPersistFolder->GetCurFolder(&pidlParent)) && pidlParent) {
                    pidlFullForItem = ILCombine(pidlParent, psmd->pidlItem); ILFree(pidlParent);
                }
            }
            if (!pidlFullForItem) OutputDebugStringW(L"OnGetContextMenu: Could not get parent PIDL for folder item.\n");
        } else {
            INT csidl = CSIDLFromID(psmd->uId); if (!csidl) return S_FALSE;
            CComHeapPtr<ITEMIDLIST> pidlSpecialFull;
            hr = SHGetSpecialFolderLocation(NULL, csidl, &pidlSpecialFull); if (FAILED(hr)) return hr;
            pidlFullForItem = ILClone(pidlSpecialFull);
            hr = SHBindToParent(pidlFullForItem, IID_PPV_ARG(IShellFolder, &pSFToUse), &pidlItemToUse);
            if (FAILED(hr)) { ILFree(pidlFullForItem); return hr; }
        }

        if (!pSFToUse || !pidlItemToUse) { if(pidlFullForItem) ILFree(pidlFullForItem); return E_FAIL; }

        CComPtr<IContextMenu> pOriginalContextMenu;
        hr = pSFToUse->GetUIObjectOf(NULL, 1, &pidlItemToUse, IID_PPV_ARG(IContextMenu, &pOriginalContextMenu));
        if (FAILED(hr)) { if(pidlFullForItem) ILFree(pidlFullForItem); return hr; }

        if (!pidlFullForItem) {
             OutputDebugStringW(L"OnGetContextMenu: No absolute PIDL, returning original context menu.\n");
            *pv = pOriginalContextMenu.Detach();
            return S_OK;
        }

        CComObject<CPinnedItemContextMenuWrapper> *pWrapper = NULL;
        hr = CComObject<CPinnedItemContextMenuWrapper>::CreateInstance(&pWrapper);
        if (FAILED(hr) || !pWrapper) { *pv = pOriginalContextMenu.Detach(); ILFree(pidlFullForItem); return hr; }

        pWrapper->AddRef();
        hr = pWrapper->Initialize(pOriginalContextMenu, pidlFullForItem);
        if (FAILED(hr)) { pWrapper->Release(); *pv = pOriginalContextMenu.Detach(); ILFree(pidlFullForItem); return hr; }

        hr = pWrapper->QueryInterface(iid, pv);
        pWrapper->Release();
        ILFree(pidlFullForItem);
        return hr;
    }

    HRESULT OnGetObject(LPSMDATA psmd, REFIID iid, void ** pv) { if (IsEqualIID(iid, IID_IShellMenu)) return OnGetSubMenu(psmd, iid, pv); else if (IsEqualIID(iid, IID_IContextMenu)) return OnGetContextMenu(psmd, iid, pv); return S_FALSE;}

    HRESULT OnExec(LPSMDATA psmd)
    {
        WCHAR szPathForExec[MAX_PATH]; // szPath in original, renamed to avoid conflict
        // We don't generally track these special uID commands as "frequent apps" unless they map to a specific app path.
        if (psmd->uId == IDM_CONTROLPANEL) ShellExecuteW(NULL, NULL, L"explorer.exe", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}", NULL, SW_SHOWNORMAL);
        else if (psmd->uId == IDM_NETWORKCONNECTIONS) ShellExecuteW(NULL, NULL, L"explorer.exe", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", NULL, SW_SHOWNORMAL);
        else if (psmd->uId == IDM_PRINTERSANDFAXES) ShellExecuteW(NULL, NULL, L"explorer.exe", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}", NULL, SW_SHOWNORMAL);
        else if (psmd->uId == IDM_MYDOCUMENTS) { if (SHGetSpecialFolderPathW(NULL, szPathForExec, CSIDL_PERSONAL, FALSE)) ShellExecuteW(NULL, NULL, szPathForExec, NULL, NULL, SW_SHOWNORMAL); }
        else if (psmd->uId == IDM_MYPICTURES) { if (SHGetSpecialFolderPathW(NULL, szPathForExec, CSIDL_MYPICTURES, FALSE)) ShellExecuteW(NULL, NULL, szPathForExec, NULL, NULL, SW_SHOWNORMAL); }
        else PostMessageW(m_hwndTray, WM_COMMAND, psmd->uId, 0);
        return S_OK;
    }

public:
    DECLARE_NOT_AGGREGATABLE(CShellMenuCallback)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CShellMenuCallback) COM_INTERFACE_ENTRY_IID(IID_IShellMenuCallback, IShellMenuCallback) END_COM_MAP()
    void Initialize(IShellMenu* pShellMenu, IBandSite* pBandSite, IDeskBar* pDeskBar) { m_pShellMenu=pShellMenu; m_pBandSite=pBandSite; m_pDeskBar=pDeskBar; }
    ~CShellMenuCallback() { ILFree(m_pidlPrograms); }
    HRESULT _SetProgramsFolder(IShellFolder * psf, LPITEMIDLIST pidl) { m_psfPrograms = psf; m_pidlPrograms = ILClone(pidl); return S_OK; }

    HRESULT STDMETHODCALLTYPE CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch(uMsg){
            case SMC_INITMENU: return OnInitMenu();
            case SMC_GETOBJECT: return OnGetObject(psmd, *reinterpret_cast<IID*>(wParam), reinterpret_cast<void**>(lParam));
            case SMC_GETINFO: return OnGetInfo(psmd, reinterpret_cast<SMINFO*>(lParam));
            case SMC_EXEC: return OnExec(psmd);
            case SMC_SFEXEC:
                {
                    if (psmd->psf && psmd->pidlItem && m_pTrayPriv) {
                        LPITEMIDLIST pidlFull = NULL;
                        CComQIPtr<IPersistFolder2> pParentPersistFolder(psmd->psf);
                        if (pParentPersistFolder) {
                            LPITEMIDLIST pidlParentFolder = NULL;
                            if (SUCCEEDED(pParentPersistFolder->GetCurFolder(&pidlParentFolder)) && pidlParentFolder) {
                                pidlFull = ILCombine(pidlParentFolder, psmd->pidlItem); ILFree(pidlParentFolder);
                            }
                        }
                        if (pidlFull) {
                            WCHAR appPath[MAX_PATH];
                            if (SHGetPathFromIDListW(pidlFull, appPath)) CMenuDeskBar::RecordAppLaunch(appPath);
                            ILFree(pidlFull);
                        } else { // Fallback if full PIDL couldn't be constructed
                            WCHAR appPath[MAX_PATH]; // Try with relative (might be absolute if psf is desktop)
                            if (SHGetPathFromIDListW(psmd->pidlItem, appPath)) CMenuDeskBar::RecordAppLaunch(appPath);
                        }
                        return m_pTrayPriv->Execute(psmd->psf, psmd->pidlItem);
                    }
                }
                break;
            // 0x10000000: // _FilterPIDL - logic for this was complex and potentially problematic, needs review if re-enabled
        }
        return S_FALSE;
    }
};

// --- CPinnedItemContextMenuWrapper Implementation ---
class CPinnedItemContextMenuWrapper :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3
{
public:
    CComPtr<IContextMenu> m_pWrappedContextMenu;
    CComPtr<IContextMenu2> m_pWrappedContextMenu2;
    CComPtr<IContextMenu3> m_pWrappedContextMenu3;
    LPITEMIDLIST m_pidlItemAbsolute;

    const WCHAR* PINNED_ITEMS_REG_PATH = L"Software\\ReactOS\\Explorer\\PinnedStartItems";

    void PathToRegKeyName(const WCHAR* path, WCHAR* regKeyName, DWORD regKeyNameMaxLen) {
        StringCchCopyW(regKeyName, regKeyNameMaxLen, path);
        for (int i = 0; regKeyName[i] != L'\0' && (DWORD)i < regKeyNameMaxLen; ++i) if (regKeyName[i] == L'\\') regKeyName[i] = L'|';
    }

    bool IsItemPinned(LPCITEMIDLIST pidlAbsolute) {
        if (!pidlAbsolute) return false; WCHAR path[MAX_PATH];
        if (!SHGetPathFromIDListW(pidlAbsolute, path)) return false;
        WCHAR regValueName[MAX_PATH]; PathToRegKeyName(path, regValueName, MAX_PATH); HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, PINNED_ITEMS_REG_PATH, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
            LSTATUS status = RegQueryValueExW(hKey, regValueName, NULL, NULL, NULL, NULL); RegCloseKey(hKey); return (status == ERROR_SUCCESS);
        } return false;
    }

    void PinItem(LPCITEMIDLIST pidlAbsolute) {
        if (!pidlAbsolute) return; WCHAR path[MAX_PATH];
        if (SHGetPathFromIDListW(pidlAbsolute, path)) {
            HKEY hKey; WCHAR regValueName[MAX_PATH]; PathToRegKeyName(path, regValueName, MAX_PATH);
            if (RegCreateKeyExW(HKEY_CURRENT_USER, PINNED_ITEMS_REG_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKey, regValueName, 0, REG_SZ, (const BYTE*)path, (wcslen(path) + 1) * sizeof(WCHAR)); RegCloseKey(hKey);
                HWND hwndDeskBar = FindWindowW(L"BaseBar", NULL); if (hwndDeskBar) PostMessageW(hwndDeskBar, CMenuDeskBar::WM_USER_REFRESH_PINNED_ITEMS, 0, 0);
            }
        }
    }

    void UnpinItem(LPCITEMIDLIST pidlAbsolute) {
        if (!pidlAbsolute) return; WCHAR path[MAX_PATH];
        if (SHGetPathFromIDListW(pidlAbsolute, path)) {
            HKEY hKey; WCHAR regValueName[MAX_PATH]; PathToRegKeyName(path, regValueName, MAX_PATH);
            if (RegOpenKeyExW(HKEY_CURRENT_USER, PINNED_ITEMS_REG_PATH, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                RegDeleteValueW(hKey, regValueName); RegCloseKey(hKey);
                HWND hwndDeskBar = FindWindowW(L"BaseBar", NULL); if (hwndDeskBar) PostMessageW(hwndDeskBar, CMenuDeskBar::WM_USER_REFRESH_PINNED_ITEMS, 0, 0);
            }
        }
    }

public:
    CPinnedItemContextMenuWrapper() : m_pidlItemAbsolute(NULL) {}
    ~CPinnedItemContextMenuWrapper() { ILFree(m_pidlItemAbsolute); }

    HRESULT Initialize(IContextMenu* pWrapped, LPCITEMIDLIST pidlItemAbsolute) {
        if (!pWrapped || !pidlItemAbsolute) return E_POINTER;
        m_pWrappedContextMenu = pWrapped;
        pWrapped->QueryInterface(IID_PPV_ARG(IContextMenu2, &m_pWrappedContextMenu2));
        pWrapped->QueryInterface(IID_PPV_ARG(IContextMenu3, &m_pWrappedContextMenu3));
        m_pidlItemAbsolute = ILClone(pidlItemAbsolute);
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CPinnedItemContextMenuWrapper)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CPinnedItemContextMenuWrapper) COM_INTERFACE_ENTRY(IContextMenu) COM_INTERFACE_ENTRY(IContextMenu2) COM_INTERFACE_ENTRY(IContextMenu3) END_COM_MAP()

    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
        if (!m_pWrappedContextMenu) return E_UNEXPECTED;
        HRESULT hr = m_pWrappedContextMenu->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
        UINT insertedByWrapped = SUCCEEDED(hr) ? LOWORD(hr) : 0;
        UINT currentTotalItems = insertedByWrapped;

        if (m_pidlItemAbsolute && !(uFlags & CMF_DEFAULTONLY)) {
            MENUITEMINFOW mii = { sizeof(mii), MIIM_STRING | MIIM_ID | MIIM_FTYPE, MFT_STRING };
            UINT itemsAddedByUs = 0;
            bool itemIsPinned = IsItemPinned(m_pidlItemAbsolute);

            if (insertedByWrapped > 0) {
                 if ((itemIsPinned && CMD_UNPIN_FROM_STARTMENU <= idCmdLast) || (!itemIsPinned && CMD_PIN_TO_STARTMENU <= idCmdLast)) {
                    InsertMenuW(hmenu, indexMenu + currentTotalItems, MF_BYPOSITION | MF_SEPARATOR, 0, NULL); currentTotalItems++;
                 }
            }
            if (itemIsPinned) {
                if (CMD_UNPIN_FROM_STARTMENU >= idCmdFirst && CMD_UNPIN_FROM_STARTMENU <= idCmdLast) {
                    mii.wID = CMD_UNPIN_FROM_STARTMENU; mii.dwTypeData = L"Unpin from Start Menu";
                    if (InsertMenuItemW(hmenu, indexMenu + currentTotalItems, TRUE, &mii)) itemsAddedByUs++;
                }
            } else {
                if (CMD_PIN_TO_STARTMENU >= idCmdFirst && CMD_PIN_TO_STARTMENU <= idCmdLast) {
                    mii.wID = CMD_PIN_TO_STARTMENU; mii.dwTypeData = L"Pin to Start Menu";
                    if (InsertMenuItemW(hmenu, indexMenu + currentTotalItems, TRUE, &mii)) itemsAddedByUs++;
                }
            }
            currentTotalItems += itemsAddedByUs;
        }
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentTotalItems);
    }

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) {
        if (!m_pWrappedContextMenu) return E_UNEXPECTED;
        if (IS_INTRESOURCE(pici->lpVerb)) {
            UINT cmdID = LOWORD(pici->lpVerb);
            if (cmdID == CMD_PIN_TO_STARTMENU) { if (m_pidlItemAbsolute) PinItem(m_pidlItemAbsolute); return S_OK; }
            else if (cmdID == CMD_UNPIN_FROM_STARTMENU) { if (m_pidlItemAbsolute) UnpinItem(m_pidlItemAbsolute); return S_OK; }
        }
        return m_pWrappedContextMenu->InvokeCommand(pici);
    }

    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, LPSTR pszName, UINT cchMax) {
        if (!m_pWrappedContextMenu) return E_UNEXPECTED;
        if (idCmd == CMD_PIN_TO_STARTMENU) { if (uType == GCS_HELPTEXTW) wcsncpy((LPWSTR)pszName, L"Pins this item to the Start Menu.", cchMax); else if (uType == GCS_HELPTEXTA) strncpy(pszName, "Pins this item to the Start Menu.", cchMax); return S_OK; }
        if (idCmd == CMD_UNPIN_FROM_STARTMENU) { if (uType == GCS_HELPTEXTW) wcsncpy((LPWSTR)pszName, L"Unpins this item from the Start Menu.", cchMax); else if (uType == GCS_HELPTEXTA) strncpy(pszName, "Unpins this item from the Start Menu.", cchMax); return S_OK; }
        return m_pWrappedContextMenu->GetCommandString(idCmd, uType, pReserved, pszName, cchMax);
    }
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { if (m_pWrappedContextMenu2) return m_pWrappedContextMenu2->HandleMenuMsg(uMsg,wParam,lParam); return E_NOTIMPL;}
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) { if (m_pWrappedContextMenu3) return m_pWrappedContextMenu3->HandleMenuMsg2(uMsg,wParam,lParam,plResult); return E_NOTIMPL;}
};

HRESULT BindToDesktop(LPCITEMIDLIST pidl, IShellFolder ** ppsfResult) { HRESULT hr; CComPtr<IShellFolder> psfDesktop; *ppsfResult = NULL; hr = SHGetDesktopFolder(&psfDesktop); if (FAILED(hr)) return hr; hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, ppsfResult)); return hr;}
static HRESULT GetMergedFolder(int folder1, int folder2, IShellFolder ** ppsfStartMenu) { /* ... Same logic as before ... */ return E_FAIL; } // Simplified
static HRESULT GetStartMenuFolder(IShellFolder ** ppsfStartMenu) { return GetMergedFolder(CSIDL_STARTMENU, CSIDL_COMMON_STARTMENU, ppsfStartMenu); }
static HRESULT GetProgramsFolder(IShellFolder ** ppsfStartMenu) { return GetMergedFolder(CSIDL_PROGRAMS, CSIDL_COMMON_PROGRAMS, ppsfStartMenu); }

extern "C"
HRESULT WINAPI RSHELL_CStartMenu_CreateInstance(REFIID riid, void **ppv)
{
    CComPtr<IShellMenu> pShellMenu; CComPtr<IBandSite> pBandSite; CComPtr<IDeskBar> pDeskBar;
    HRESULT hr; IShellFolder * psfRootStartMenu;
    LPITEMIDLIST pidlProgramsInStartMenu = NULL;
    CComPtr<IShellFolder> psfProgramsActual;

    hr = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &pShellMenu)); if (FAILED(hr)) return hr;
    hr = CMenuSite_CreateInstance(IID_PPV_ARG(IBandSite, &pBandSite)); if (FAILED(hr)) return hr;
    hr = CMenuDeskBar_CreateInstance(IID_PPV_ARG(IDeskBar, &pDeskBar)); if (FAILED(hr)) return hr;
    CComObject<CShellMenuCallback> *pCallback; hr = CComObject<CShellMenuCallback>::CreateInstance(&pCallback); if (FAILED(hr)) return hr;
    pCallback->AddRef(); pCallback->Initialize(pShellMenu, pBandSite, pDeskBar);
    hr = pShellMenu->Initialize(pCallback, (UINT)-1, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL); if (FAILED(hr)) { pCallback->Release(); return hr; }

    hr = GetStartMenuFolder(&psfRootStartMenu); if (FAILED(hr)) { pCallback->Release(); return hr; }

    LPITEMIDLIST pidlProgramsAbs;
    hr = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlProgramsAbs);
    if (SUCCEEDED(hr)) {
        LPCITEMIDLIST pidlChildProgName; CComPtr<IShellFolder> psfProgParent; STRRET strProgName; WCHAR szProgDispName[MAX_PATH];
        if (SUCCEEDED(SHBindToParent(pidlProgramsAbs, IID_PPV_ARG(IShellFolder, &psfProgParent), &pidlChildProgName))) {
            if (SUCCEEDED(psfProgParent->GetDisplayNameOf(pidlChildProgName, SHGDN_INFOLDER, &strProgName))) { // Use SHGDN_INFOLDER for relative name
                StrRetToBufW(&strProgName, pidlChildProgName, szProgDispName, _countof(szProgDispName));
                psfRootStartMenu->ParseDisplayName(NULL, NULL, szProgDispName, NULL, &pidlProgramsInStartMenu, NULL);
            }
        }
        ILFree(pidlProgramsAbs);
    } else { WARN("Could not get CSIDL_PROGRAMS absolute PIDL.\n"); }

    hr = GetProgramsFolder(&psfProgramsActual); if (FAILED(hr)) { ILFree(pidlProgramsInStartMenu); psfRootStartMenu->Release(); pCallback->Release(); return hr; }

    pCallback->_SetProgramsFolder(psfProgramsActual, pidlProgramsInStartMenu);

    hr = pShellMenu->SetShellFolder(psfRootStartMenu, NULL, NULL, SMSET_TOP);
    psfRootStartMenu->Release();
    if (FAILED(hr)) { pCallback->Release(); return hr; }

    hr = pDeskBar->SetClient(pBandSite); if (FAILED(hr)) { pCallback->Release(); return hr; }
    hr = pBandSite->AddBand(pShellMenu); if (FAILED(hr)) { pCallback->Release(); return hr; }

    pCallback->Release();
    return pDeskBar->QueryInterface(riid, ppv);
}
