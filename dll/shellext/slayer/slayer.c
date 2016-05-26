/*
 * ReactOS Compatibility Layer Shell Extension
 * Copyright (C) 2004 - 2005 ReactOS Team
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
/*
 * PROJECT:         ReactOS Compatibility Layer Shell Extension
 * FILE:            dll/shellext/slayer/slayer.c
 * PURPOSE:         ReactOS Compatibility Layer Shell Extension
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      09/25/2004  Created
 */

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tchar.h>

#include "resource.h"
#include "slayer.h"

HINSTANCE hInstance = NULL;
static LONG dllrefs = 0;

static ifaceICompatibilityPageVbtl efvt =
{
    /* IUnknown methods */
    ICompatibilityPage_fnQueryInterface,
    ICompatibilityPage_fnAddRef,
    ICompatibilityPage_fnRelease,
};

static ifaceIShellPropSheetExtVbtl efvtIShellPropSheetExt =
{
    /* IShellPropSheetExt */
    ICompatibilityPage_fnAddPages,
    ICompatibilityPage_fnReplacePage,
};

static ifaceIShellExtInitVbtl efvtIShellExtInit =
{
    /* IShellExtInit */
    ICompatibilityPage_fnInitialize,
};

static ifaceIClassFactoryVbtl efvtIClassFactory =
{
    /* IClassFactory */
    ICompatibilityPage_fnCreateInstance,
    ICompatibilityPage_fnLockServer,
};

/******************************************************************************
   ICompatibilityPage
 ******************************************************************************/

static VOID
ClearCItemList(LPCOMPATIBILITYPAGE info)
{
    PCITEM item, next;

    for (item = info->CItems;
         item != NULL;
         item = next)
    {
        next = item->next;
        HeapFree(GetProcessHeap(),
                 0,
                 item);
    }

    info->CSelectedItem = NULL;
    info->CItems = NULL;
    info->nItems = 0;
}

static BOOL
ReadDWORDFlag(HKEY hk,
              LPTSTR szValueName,
              LPDWORD lpOutValue,
              DWORD dwDefault)
{
    DWORD dwType, dwSize = sizeof(DWORD);
    LONG e = RegQueryValueEx(hk,
                             szValueName,
                             0,
                             &dwType,
                             (LPBYTE)lpOutValue,
                             &dwSize);

    if (e != ERROR_SUCCESS || dwSize != sizeof(DWORD))
    {
        *lpOutValue = dwDefault;

        return TRUE;
    }

    return FALSE;
}

static BOOL
LoadAndParseAppCompatibilityFlags(LPCOMPATIBILITYPAGE info,
                                  LPTSTR szValueName)
{
    LONG e;
    HKEY hk;
    DWORD dwType, dwSize;
    TCHAR szStr[256];

    e = RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
                   &hk);
    if (e == ERROR_SUCCESS)
    {
        dwSize = sizeof(szStr);

        e = RegQueryValueEx(hk,
                            szValueName,
                            0,
                            &dwType,
                            (LPBYTE)szStr,
                            &dwSize);

        if (e == ERROR_SUCCESS)
        {
            /* FIXME - make sure the string is NULL-terminated! */
            TCHAR *c;
            for (c = szStr;
                 *c != TEXT('\0');
                 c++)
            {
                /* only the first word represents the compatibility mode */
                /* FIXME - parse all words! */
                if (*c == TEXT(' '))
                {
                    *c = TEXT('\0');
                    break;
                }
            }

            info->CSelectedItem = NULL;
            if (_tcslen(szStr) > 0)
            {
                PCITEM item;

                for (item = info->CItems;
                     item != NULL;
                     item = item->next)
                {
                    if (!_tcsicmp(szStr, item->szKeyName))
                    {
                        info->CSelectedItem = item;
                        break;
                    }
                }
            }
        }
        RegCloseKey(hk);
    }

    return FALSE;
}

static BOOL
LoadCompatibilityModes(LPCOMPATIBILITYPAGE info)
{
    BOOL Ret;
    LONG e;
    HKEY hk, hk2;
    TCHAR szKey[256];

    ClearCItemList(info);

    e = RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
                   &hk);

    if (e == ERROR_SUCCESS)
    {
        DWORD i;
        PCITEM lastitem = NULL;

        for(i = 0;
            (RegEnumKey(hk, i,szKey, sizeof(szKey) / sizeof(szKey[0])) == ERROR_SUCCESS);
            i++)
        {
            e = RegOpenKey(hk,
                           szKey,
                           &hk2);

            if (e == ERROR_SUCCESS)
            {
                DWORD dwType;

                e = RegQueryValueEx(hk2,
                                    NULL,
                                    0,
                                    &dwType,
                                    NULL,
                                    NULL);

                if (e != ERROR_SUCCESS || (e == ERROR_SUCCESS && dwType == REG_SZ))
                {
                    PCITEM item;

                    item = HeapAlloc(GetProcessHeap(),
                                     0,
                                     sizeof(CITEM));
                    if (item != NULL)
                    {
                        DWORD cdb = sizeof(item->szName);

                        /* description */
                        e = RegQueryValueEx(hk2,
                                            NULL,
                                            0,
                                            NULL,
                                            (LPBYTE)item->szName,
                                            &cdb);

                        /* make sure it is null-terminated */
                        if (cdb > sizeof(item->szName) - sizeof(item->szName[0]))
                        {
                            item->szName[(sizeof(item->szName) / sizeof(item->szName[0])) - 1] = TEXT('\0');
                        }

                        if (e != ERROR_SUCCESS ||
                            cdb < sizeof(item->szName[0]))
                        {
                            _tcscpy(item->szName, szKey);
                            e = ERROR_SUCCESS;
                        }

                        _tcscpy(item->szKeyName, szKey);
                        info->nItems++;

                        ReadDWORDFlag(hk2,
                                      TEXT("MajorVersion"),
                                      &item->MajorVersion,
                                      0);
                        ReadDWORDFlag(hk2,
                                      TEXT("MinorVersion"),
                                      &item->MinorVersion,
                                      0);
                        ReadDWORDFlag(hk2,
                                      TEXT("BuildNumber"),
                                      &item->BuildNumber,
                                      0);
                        ReadDWORDFlag(hk2,
                                      TEXT("PlatformId"),
                                      &item->PlatformId,
                                      0);
                        ReadDWORDFlag(hk2,
                                      TEXT("SPMajorVersion"),
                                      &item->SPMajorVersion,
                                      0);
                        ReadDWORDFlag(hk2,
                                      TEXT("SPMinorVersion"),
                                      &item->SPMinorVersion,
                                      0);

                        if (e == ERROR_SUCCESS)
                        {
                            item->next = NULL;
                            if (lastitem != NULL)
                            {
                                lastitem->next = item;
                            }
                            else
                            {
                                info->CItems = item;
                            }
                            lastitem = item;
                        }
                        else
                        {
                            HeapFree(GetProcessHeap(),
                                     0,
                                     item);
                        }
                    }
                }

                RegCloseKey(hk2);
            }

            if (e != ERROR_SUCCESS)
            {
                e = ERROR_SUCCESS;
            }
        }
        RegCloseKey(hk);
    }

    Ret = ((e == ERROR_SUCCESS || e == ERROR_NO_MORE_ITEMS) ? TRUE : FALSE);

    return Ret;
}

static VOID
FillComboBoxWithCompatibilityModes(LPCOMPATIBILITYPAGE info,
                                   HWND hwndDlg,
                                   HWND hCombo,
                                   BOOL bSelectItem,
                                   BOOL bDisableControlsIfEmpty)
{
    PCITEM item;
    int i = 0;
    BOOL sel = FALSE;

    SendMessage(hCombo,
                CB_RESETCONTENT,
                0,
                0);

    for (item = info->CItems;
         item != NULL;
         item = item->next)
    {
        int iIndex = (int)SendMessage(hCombo,
                                      CB_ADDSTRING,
                                      0,
                                      (LPARAM)item->szName);

        if (item == info->CSelectedItem && bSelectItem)
        {
            SendMessage(hCombo,
                        CB_SETCURSEL,
                        (WPARAM)iIndex,
                        0);
            sel = TRUE;
        }
        i++;
    }

    if (!sel && bSelectItem && i > 0)
    {
        /* select the first item */
        SendMessage(hCombo,
                    CB_SETCURSEL,
                    0,
                    0);
    }

    if (bDisableControlsIfEmpty)
    {
        BOOL enable = (i > 0);

        EnableWindow(GetDlgItem(hwndDlg,
                                IDC_COMPATGROUP),
                     enable);

        EnableWindow(hCombo,
                     (enable && sel));

        EnableWindow(GetDlgItem(hwndDlg,
                                IDC_CHKRUNCOMPATIBILITY),
                     enable);

        CheckDlgButton(hwndDlg,
                       IDC_CHKRUNCOMPATIBILITY,
                       ((enable && sel) ? BST_CHECKED : BST_UNCHECKED));
    }
}

static VOID
FillEditListBoxWithCompatibilityModes(LPCOMPATIBILITYPAGE info,
                                      HWND hwndDlg,
                                      HWND hListBox,
                                      BOOL bDisableControlsIfEmpty)
{
    PCITEM item;
    int i;

    SendMessage(hListBox,
                LB_RESETCONTENT,
                0,
                0);

    for (item = info->CItems, i = 0;
         item != NULL;
         item = item->next, i++)
    {
        SendMessage(hListBox,
                    LB_ADDSTRING,
                    0,
                    (LPARAM)item->szName);
    }

    if (bDisableControlsIfEmpty)
    {
    }
}

static INT_PTR CALLBACK
EditCompatibilityModesProc(HWND hwndDlg,
                           UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam)
{
    LPCOMPATIBILITYPAGE this;

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg,
                              IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg,
                              IDCANCEL);
                    break;
            }
            break;
        }

        case WM_CLOSE:
        {
            EndDialog(hwndDlg,
                      IDCANCEL);
            break;
        }

        case WM_INITDIALOG:
        {
            HWND hList = GetDlgItem(hwndDlg,
                                    IDC_COMPATIBILITYMODE);

            this = (LPCOMPATIBILITYPAGE)lParam;
            SetWindowLongPtr(hwndDlg,
                             GWLP_USERDATA,
                             (LONG_PTR)this);

            FillEditListBoxWithCompatibilityModes(this,
                                                  hwndDlg,
                                                  hList,
                                                  FALSE);
            break;
        }
    }

    return FALSE;
}

static VOID
InitializePage(LPCOMPATIBILITYPAGE this,
               HWND hwndDlg)
{
    HWND hList;

    LoadCompatibilityModes(this);

    /* initialize the controls */
    hList = GetDlgItem(hwndDlg,
                       IDC_COMPATIBILITYMODE);

    LoadAndParseAppCompatibilityFlags(this,
                                      this->szFile);
    FillComboBoxWithCompatibilityModes(this,
                                       hwndDlg,
                                       hList,
                                       TRUE,
                                       TRUE);
}

static VOID
ReportPropertyChange(LPCOMPATIBILITYPAGE this,
                     HWND hwndDlg)
{
    this->Changed = TRUE;

    SendMessage(GetParent(hwndDlg),
                PSM_CHANGED,
                (WPARAM)hwndDlg,
                0);
}

static BOOL
ComposeFlags(LPCOMPATIBILITYPAGE this,
             LPTSTR szFlags)
{
    if (this->CSelectedItem != NULL)
    {
        _tcscpy(szFlags,
                this->CSelectedItem->szKeyName);
        return TRUE;
    }

    return FALSE;
}

static BOOL
ApplySettings(LPCOMPATIBILITYPAGE this,
              HWND hwndDlg)
{
    HKEY hk;
    LONG e;
    TCHAR szFlags[256];
    BOOL enabled = IsDlgButtonChecked(hwndDlg,
                                      IDC_CHKRUNCOMPATIBILITY) == BST_CHECKED;

    if (enabled)
    {
        HWND hCombo = GetDlgItem(hwndDlg,
                                 IDC_COMPATIBILITYMODE);
        int index = (int)SendMessage(hCombo,
                                     CB_GETCURSEL,
                                     0,
                                     0);
        if (index >= 0)
        {
            int i;
            PCITEM sel = this->CItems;

            /* map the index to a CITEM structure */
            for(i = index;
                i > 0 && sel != NULL;
                i--)
            {
                sel = sel->next;
            }

            /* update the CSelectedItem member */
            this->CSelectedItem = sel;
        }
        else
            enabled = FALSE;
    }

    e = RegOpenKey(HKEY_CURRENT_USER,
                   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"),
                   &hk);
    if (e == ERROR_SUCCESS)
    {
        if (!enabled)
        {
            /* FIXME - only delete if nothing else is selected! */
            e = RegDeleteValue(hk,
                               this->szFile);
        }
        else
        {
            if (ComposeFlags(this,
                             szFlags))
            {
                e = RegSetValueEx(hk,
                                  this->szFile,
                                  0,
                                  REG_SZ,
                                  (LPBYTE)szFlags,
                                  (_tcslen(szFlags) + 1) * sizeof(TCHAR));
            }
            else
            {
                e = RegDeleteValue(hk,
                                   this->szFile);
            }
        }

        RegCloseKey(hk);
    }

    this->Changed = FALSE;
    return (e == ERROR_SUCCESS);
}

static INT_PTR CALLBACK
CompatibilityPageProc(HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    LPCOMPATIBILITYPAGE this = (LPCOMPATIBILITYPAGE)GetWindowLongPtr(hwndDlg,
                                                                     GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMPATIBILITYMODE)
            {
                ReportPropertyChange(this,
                                     hwndDlg);
            }
            else
            {
                switch (LOWORD(wParam))
                {
                    case IDC_CHKRUNCOMPATIBILITY:
                    {
                        HWND hList = GetDlgItem(hwndDlg,
                                                IDC_COMPATIBILITYMODE);

                        if (hList != NULL)
                        {
                            EnableWindow(hList,
                                         IsDlgButtonChecked(hwndDlg,
                                                            IDC_CHKRUNCOMPATIBILITY) == BST_CHECKED);
                        }
                        /* fall through */
                    }

                    case IDC_CHKRUNIN256COLORS:
                    case IDC_CHKRUNIN640480RES:
                    case IDC_CHKDISABLEVISUALTHEMES:
                        ReportPropertyChange(this,
                                             hwndDlg);
                        break;

                    case IDC_EDITCOMPATIBILITYMODES:
                    {
                        if (DialogBoxParam(hInstance,
                                           MAKEINTRESOURCE(IDD_EDITCOMPATIBILITYMODES),
                                           hwndDlg,
                                           EditCompatibilityModesProc,
                                           (LPARAM)this) == IDOK)
                        {
                            InitializePage(this,
                                           hwndDlg);
                        }
                        break;
                    }
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            NMHDR *hdr = (NMHDR*)lParam;
            switch (hdr->code)
            {
                case PSN_APPLY:
                    if (this->Changed)
                    {
                        return ApplySettings(this,
                                             hwndDlg);
                    }
                    break;
            }
            break;
        }

        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
            this = (LPCOMPATIBILITYPAGE)psp->lParam;
            SetWindowLongPtr(hwndDlg,
                             GWLP_USERDATA,
                             (LONG_PTR)this);

            InitializePage(this,
                           hwndDlg);
            break;
        }
    }

    return FALSE;
}

static UINT CALLBACK
CompatibilityPageCallback(HWND hwnd,
                          UINT uMsg,
                          LPPROPSHEETPAGE ppsp)
{
    LPCOMPATIBILITYPAGE this = (LPCOMPATIBILITYPAGE)ppsp->lParam;

    switch (uMsg)
    {
        case PSPCB_CREATE:
            return TRUE;

        case PSPCB_RELEASE:
            ICompatibilityPage_fnRelease(this);
            return FALSE;

        default:
            return FALSE;
    }
}

static LPCOMPATIBILITYPAGE
ICompatibilityPage_fnConstructor(VOID)
{
    LPCOMPATIBILITYPAGE cp;

    cp = HeapAlloc(GetProcessHeap(),
                   HEAP_ZERO_MEMORY,
                   sizeof(COMPATIBILITYPAGE));
    if (cp != NULL)
    {
        cp->lpVtbl = &efvt;
        cp->lpVtbl->fn.IShellPropSheetExt = efvtIShellPropSheetExt;
        cp->ref = 1;
        InterlockedIncrement(&dllrefs);
    }

    return cp;
}

HRESULT STDMETHODCALLTYPE
ICompatibilityPage_fnQueryInterface(LPCOMPATIBILITYPAGE this,
                                    REFIID iid,
                                    PVOID *pvObject)
{
    if (IsEqualIID(iid,
                   &IID_IShellPropSheetExt))
    {
        this->lpVtbl->fn.IShellPropSheetExt = efvtIShellPropSheetExt;
        ICompatibilityPage_fnAddRef(this);
        *pvObject = this;
        return S_OK;
    }
    else if (IsEqualIID(iid,
                        &IID_IShellExtInit))
    {
        this->lpVtbl->fn.IShellExtInit = efvtIShellExtInit;
        ICompatibilityPage_fnAddRef(this);
        *pvObject = this;
        return S_OK;
    }
    else if (IsEqualIID(iid,
                        &IID_IClassFactory))
    {
        this->lpVtbl->fn.IClassFactory = efvtIClassFactory;
        ICompatibilityPage_fnAddRef(this);
        *pvObject = this;
        return S_OK;
    }
    else if (IsEqualIID(iid,
                        &IID_IUnknown))
    {
        ICompatibilityPage_fnAddRef(this);
        *pvObject = this;
        return S_OK;
    }

    *pvObject = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
ICompatibilityPage_fnAddRef(LPCOMPATIBILITYPAGE this)
{
    return (ULONG)InterlockedIncrement(&this->ref);
}

ULONG STDMETHODCALLTYPE
ICompatibilityPage_fnRelease(LPCOMPATIBILITYPAGE this)
{
    ULONG rfc;

    rfc = (ULONG)InterlockedDecrement(&this->ref);
    if (rfc == 0)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 this);
        InterlockedDecrement(&dllrefs);
    }
    return rfc;
}

HRESULT STDMETHODCALLTYPE
ICompatibilityPage_fnAddPages(LPCOMPATIBILITYPAGE this,
                              LPFNADDPROPSHEETPAGE lpfnAddPage,
                              LPARAM lParam)
{
    PROPSHEETPAGE psp = {0};
    HPROPSHEETPAGE hPage;

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    psp.hInstance = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SLAYERSHEET);
    psp.pfnDlgProc = CompatibilityPageProc;
    psp.lParam = (LPARAM)this;
    psp.pfnCallback = CompatibilityPageCallback;

    hPage = CreatePropertySheetPage(&psp);

    if (hPage != NULL)
    {
        if (!lpfnAddPage(hPage,
                         lParam))
        {
            DestroyPropertySheetPage(hPage);
            return E_OUTOFMEMORY;
        }

        ICompatibilityPage_fnAddRef(this);
        return S_OK;
    }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
ICompatibilityPage_fnReplacePage(LPCOMPATIBILITYPAGE this,
                                 UINT uPageID,
                                 LPFNADDPROPSHEETPAGE lpfnReplacePage,
                                 LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
ICompatibilityPage_fnInitialize(LPCOMPATIBILITYPAGE this,
                                LPCITEMIDLIST pidlFolder,
                                IDataObject *pdtobj,
                                HKEY hkeyProgID)
{
    FORMATETC fetc;
    STGMEDIUM smdm;

    if (pdtobj == NULL)
    {
        return E_INVALIDARG;
    }

    fetc.cfFormat = CF_HDROP;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj,
                                          &fetc,
                                          &smdm)))
    {
        UINT nFiles = DragQueryFile(smdm.hGlobal,
                                    0xFFFFFFFF,
                                    this->szFile,
                                    sizeof(this->szFile) / sizeof(this->szFile[0]));
        if (nFiles == 1)
        {
            /* FIXME - support editing of multiple files later */
            DragQueryFile(smdm.hGlobal,
                          0, this->szFile,
                          sizeof(this->szFile) / sizeof(this->szFile[0]));
            ReleaseStgMedium(&smdm);

            return S_OK;
        }
    }

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
ICompatibilityPage_fnCreateInstance(LPCOMPATIBILITYPAGE this,
                                    LPUNKNOWN pUnkOuter,
                                    REFIID riid,
                                    PVOID *ppvObject)
{
    LPCOMPATIBILITYPAGE cp;

    if (pUnkOuter != NULL &&
        !IsEqualIID(riid,
                    &IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }

    cp = ICompatibilityPage_fnConstructor();
    if (cp != NULL)
    {
        HRESULT ret = ICompatibilityPage_fnQueryInterface(cp,
                                                          riid,
                                                          ppvObject);
        ICompatibilityPage_fnRelease(cp);
        return ret;
    }

    return E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE
ICompatibilityPage_fnLockServer(LPCOMPATIBILITYPAGE this,
                                BOOL fLock)
{
    if (fLock)
    {
        InterlockedIncrement(&dllrefs);
    }
    else
    {
        InterlockedDecrement(&dllrefs);
    }

    return S_OK;
}

/******************************************************************************
   Exported
 ******************************************************************************/

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID iid,
                  LPVOID *ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    if (IsEqualCLSID(&CLSID_ICompatibilityPage,
                     rclsid))
    {
        LPCOMPATIBILITYPAGE iface = ICompatibilityPage_fnConstructor();
        if (iface != NULL)
        {
            HRESULT ret = ICompatibilityPage_fnQueryInterface(iface,
                                                              iid,
                                                              ppv);
            ICompatibilityPage_fnRelease(iface);
            return ret;
        }
        return E_OUTOFMEMORY;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    return ((dllrefs == 0) ? S_OK : S_FALSE);
}

static int
UnregisterPropSheetHandler(LPTSTR szType)
{
    TCHAR szKey[255];

    _stprintf(szKey,
              TEXT("%s\\shellex\\PropertySheetHandlers\\Compatibility Property Page"),
              szType);

    return RegDeleteKey(HKEY_CLASSES_ROOT,
                        szKey);
}

HRESULT WINAPI
DllUnregisterServer(VOID)
{
    LONG e;
    HKEY hk;
    WCHAR szGuid[40];

    StringFromGUID2(&CLSID_ICompatibilityPage,
                    szGuid,
                    sizeof(szGuid) / sizeof(szGuid[0]));

    e = RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
                   &hk);
    if (e == ERROR_SUCCESS)
    {
        e = RegDeleteValueW(hk,
                            szGuid);
        RegCloseKey(hk);
    }

    if (e == ERROR_SUCCESS)
    {
        e = UnregisterPropSheetHandler(TEXT("exefile"));
    }

    if (e == ERROR_SUCCESS)
    {
        e = RegOpenKey(HKEY_CLASSES_ROOT,
                       TEXT("CLSID"),
                       &hk);
        if (e == ERROR_SUCCESS)
        {
            TCHAR szInprocKey[255];

            _stprintf(szInprocKey,
                      TEXT("%ws\\InprocServer32"),
                      szGuid);

            e = RegDeleteKey(hk,
                             szInprocKey);
            if (e == ERROR_SUCCESS)
            {
                e = RegDeleteKeyW(hk,
                                  szGuid);
            }
            RegCloseKey(hk);
        }
    }

    return ((e == ERROR_SUCCESS) ? S_OK : E_ACCESSDENIED);
}

static int
RegisterPropSheetHandler(LPTSTR szType,
                         LPWSTR szGuid)
{
    TCHAR szKey[255];
    HKEY hk;
    int e;

    _stprintf(szKey,
              TEXT("%s\\shellex\\PropertySheetHandlers\\Compatibility Property Page"),
              szType);

    e = RegCreateKey(HKEY_CLASSES_ROOT,
                     szKey,
                     &hk);
    if (e == ERROR_SUCCESS)
    {
        e = RegSetValueExW(hk,
                           NULL,
                           0,
                           REG_SZ,
                           (BYTE*)szGuid,
                           (wcslen(szGuid) + 1) * sizeof(WCHAR));
        RegCloseKey(hk);
    }

    return e;
}

HRESULT WINAPI
DllRegisterServer(VOID)
{
    LONG e = E_ACCESSDENIED;
    HKEY hk;
    WCHAR szGuid[40];
    WCHAR szDescription[255];
    TCHAR szModule[MAX_PATH + 1];
    int lnszDescription;

    if (!GetModuleFileName(hInstance,
                           szModule,
                           sizeof(szModule) / sizeof(szModule[0])))
    {
        return E_ACCESSDENIED;
    }

    /* unregister first */
    DllUnregisterServer();

    lnszDescription = LoadStringW(hInstance,
                                  IDS_DESCRIPTION,
                                  szDescription, sizeof(szDescription) / sizeof(szDescription[0]));
    if (lnszDescription > 0)
    {
        StringFromGUID2(&CLSID_ICompatibilityPage,
                        szGuid,
                        sizeof(szGuid) / sizeof(szGuid[0]));

        e = RegOpenKey(HKEY_LOCAL_MACHINE,
                       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
                       &hk);
        if (e == ERROR_SUCCESS)
        {
            e = RegSetValueExW(hk,
                               szGuid,
                               0,
                               REG_SZ,
                               (BYTE*)szDescription,
                               (lnszDescription + 1) * sizeof(WCHAR));
            RegCloseKey(hk);
        }

        if (e == ERROR_SUCCESS)
        {
            TCHAR szInprocKey[255];

            _stprintf(szInprocKey,
                      TEXT("CLSID\\%ws\\InprocServer32"),
                      szGuid);

            e = RegCreateKey(HKEY_CLASSES_ROOT,
                             szInprocKey,
                             &hk);
            if (e == ERROR_SUCCESS)
            {
                e = RegSetValueEx(hk,
                                  NULL,
                                  0,
                                  REG_SZ,
                                  (BYTE*)szModule,
                                  (_tcslen(szModule) + 1) * sizeof(TCHAR));
                if (e == ERROR_SUCCESS)
                {
                    const TCHAR szApartment[] = TEXT("Apartment");

                    e = RegSetValueEx(hk,
                                      TEXT("ThreadingModel"),
                                      0,
                                      REG_SZ,
                                      (BYTE*)szApartment,
                                      (_tcslen(szApartment) + 1) * sizeof(TCHAR));
                }

                RegCloseKey(hk);
            }
        }

        if (e == ERROR_SUCCESS)
        {
            e = RegisterPropSheetHandler(TEXT("exefile"),
                                         szGuid);
        }
    }

    return ((e == ERROR_SUCCESS) ? S_OK : E_ACCESSDENIED);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInstance = hinstDLL;
            DisableThreadLibraryCalls(hInstance);
            break;
    }

    return TRUE;
}

