/*
 * Folder options.
 *
 * Copyright (C) 2016 Mark Jansen
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
#include <shdeprecated.h>


WINE_DEFAULT_DEBUG_CHANNEL(fprop);

CFolderOptions::CFolderOptions()
    :m_pSite(NULL)
{
}

CFolderOptions::~CFolderOptions()
{
}

/*************************************************************************
 * FolderOptions IShellPropSheetExt interface
 */

INT_PTR CALLBACK FolderOptionsGeneralDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FolderOptionsViewDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FolderOptionsFileTypesDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT STDMETHODCALLTYPE CFolderOptions::AddPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hPage;
    LPARAM sheetparam = (LPARAM)static_cast<CFolderOptions*>(this);

    hPage = SH_CreatePropertySheetPageEx(IDD_FOLDER_OPTIONS_GENERAL, FolderOptionsGeneralDlg,
                                         sheetparam, NULL, &PropSheetPageLifetimeCallback<CFolderOptions>);
    HRESULT hr = AddPropSheetPage(hPage, pfnAddPage, lParam);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        ERR("Failed to create property sheet page FolderOptionsGeneral\n");
        return hr;
    }
    else
    {
        AddRef(); // For PropSheetPageLifetimeCallback
    }

    hPage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_VIEW, FolderOptionsViewDlg, sheetparam, NULL);
    if (hPage == NULL)
    {
        ERR("Failed to create property sheet page FolderOptionsView\n");
        return E_FAIL;
    }
    if (!pfnAddPage(hPage, lParam))
        return E_FAIL;

    hPage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_FILETYPES, FolderOptionsFileTypesDlg, sheetparam, NULL);
    if (hPage == NULL)
    {
        ERR("Failed to create property sheet page FolderOptionsFileTypes\n");
        return E_FAIL;
    }
    if (!pfnAddPage(hPage, lParam))
        return E_FAIL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolderOptions::ReplacePage(EXPPS uPageID, LPFNSVADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    TRACE("(%p) (uPageID %u, pfnReplaceWith %p lParam %p\n", this, uPageID, pfnReplaceWith, lParam);
    return E_NOTIMPL;
}

/*************************************************************************
 * FolderOptions IShellExtInit interface
 */

HRESULT STDMETHODCALLTYPE CFolderOptions::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    return S_OK;
}


/*************************************************************************
 * FolderOptions IObjectWithSite interface
 */
HRESULT STDMETHODCALLTYPE CFolderOptions::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFolderOptions::GetSite(REFIID riid, void **ppvSite)
{
    return m_pSite ? m_pSite->QueryInterface(riid, ppvSite) : E_FAIL;
}

/*************************************************************************
 * FolderOptions helper methods
 */
HRESULT CFolderOptions::HandleDefFolderSettings(int Action)
{
    IBrowserService2 *bs2;
    HRESULT hr = IUnknown_QueryService(m_pSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService2, &bs2));
    if (SUCCEEDED(hr))
    {
        if (Action == DFSA_APPLY)
        {
            hr = bs2->SetAsDefFolderSettings();
        }
        else if (Action == DFSA_RESET)
        {
            // There does not seem to be a method in IBrowserService2 for this
            IUnknown_Exec(bs2, CGID_DefView, DVCMDID_RESET_DEFAULTFOLDER_SETTINGS, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }
        else
        {
            // FFSA_QUERY: hr is already correct
        }
        bs2->Release();
    }

    if (Action == DFSA_RESET)
    {
        IGlobalFolderSettings *pgfs;
        HRESULT hr = CoCreateInstance(CLSID_GlobalFolderSettings, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IGlobalFolderSettings, (void **)&pgfs);
        if (SUCCEEDED(hr))
        {
            hr = pgfs->Set(NULL, 0, 0);
            pgfs->Release();
        }
    }

    return hr;
}
