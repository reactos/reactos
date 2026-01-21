/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Font shell extension background menu
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <shlwapi_undoc.h>

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

CFontBkgndMenu::CFontBkgndMenu()
{
}

CFontBkgndMenu::~CFontBkgndMenu()
{
}

HRESULT WINAPI CFontBkgndMenu::Initialize(CFontExt* pFontExt, const DEFCONTEXTMENU *pdcm)
{
    m_pFontExt = pFontExt;
    m_psf = pdcm->psf;
    m_pmcb = pdcm->pcmcb;
    m_hwnd = pdcm->hwnd;
    return S_OK;
}

// IContextMenu
STDMETHODIMP CFontBkgndMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    TRACE("%d\n", idCmdFirst);
    CString strProp(MAKEINTRESOURCEW(IDS_PROPERTIES));
    INT idCmd = idCmdFirst;
    AppendMenuW(hMenu, MF_STRING, idCmd++, strProp);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmd - idCmdFirst);
}

STDMETHODIMP CFontBkgndMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    INT idCmd = IS_INTRESOURCE(lpcmi->lpVerb) ? LOWORD(lpcmi->lpVerb) : -1;
    TRACE("%d\n", idCmd);

    if (idCmd == 0 || (idCmd == -1 && !lstrcmpiA(lpcmi->lpVerb, "properties")))
    {
        // Open "Fonts" properties
        LPITEMIDLIST pidl;
        SHGetSpecialFolderLocation(m_hwnd, CSIDL_FONTS, &pidl);
        SHELLEXECUTEINFOW sei = {
            sizeof(sei), SEE_MASK_INVOKEIDLIST | SEE_MASK_ASYNCOK, NULL, L"properties",
            NULL, NULL, NULL, SW_SHOWNORMAL, NULL, const_cast<LPITEMIDLIST>(pidl)
        };
        BOOL bOK = ShellExecuteExW(&sei);
        CoTaskMemFree(pidl);
        return bOK ? S_OK : E_FAIL;
    }

    if (idCmd == FCIDM_SHVIEW_INSERT || (idCmd == -1 && !lstrcmpiA(lpcmi->lpVerb, "paste")))
    {
        CComPtr<IDataObject> pDataObj;
        HRESULT hr = OleGetClipboard(&pDataObj);
        if (FAILED_UNEXPECTEDLY(hr))
            return E_FAIL;

        return SHSimulateDrop(m_pFontExt, pDataObj, 0, NULL, NULL);
    }

    return S_OK;
}

STDMETHODIMP CFontBkgndMenu::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    TRACE("%d\n", idCommand);
    if (idCommand == 0)
    {
        lstrcpynA(lpszName, "properties", uMaxNameLen);
        return S_OK;
    }
    return E_FAIL;
}

// IContextMenu2
STDMETHODIMP CFontBkgndMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

// IContextMenu3
STDMETHODIMP CFontBkgndMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    if (uMsg == WM_INITMENUPOPUP)
    {
        HMENU hMenu = (HMENU)wParam;
        ::DeleteMenu(hMenu, FCIDM_SHVIEW_INSERTLINK, MF_BYCOMMAND);
        return S_OK;
    }
    return E_NOTIMPL;
}

static
HRESULT
CFontBkgndMenu_CreateInstance(CFontExt* pFontExt, const DEFCONTEXTMENU *pdcm, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CFontBkgndMenu>(pFontExt, pdcm, riid, ppv);
}

HRESULT
APIENTRY
CFontBkgndMenu_Create(
    CFontExt* pFontExt,
    HWND hwnd,
    IShellFolder* psf,
    IContextMenu** ppcm)
{
    DEFCONTEXTMENU dcm;
    ZeroMemory(&dcm, sizeof(dcm));
    dcm.hwnd = hwnd;
    dcm.psf = psf;
    dcm.cidl = 0;
    dcm.apidl = NULL;
    dcm.punkAssociationInfo = NULL;

    HRESULT hr = CFontBkgndMenu_CreateInstance(pFontExt, &dcm, IID_PPV_ARG(IContextMenu, ppcm));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}
