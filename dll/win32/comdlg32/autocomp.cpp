/*
 * PROJECT:     ReactOS Common Dialogs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Implement auto-completion for comdlg32
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

EXTERN_C HRESULT
DoInitAutoCompleteWithCWD(FileOpenDlgInfos *pInfo, HWND hwndEdit)
{
    pInfo->pvCWD = pInfo->pvDropDown = pInfo->pvACList = NULL;

    WCHAR szClass[32];
    GetClassNameW(hwndEdit, szClass, _countof(szClass));
    if (lstrcmpiW(szClass, WC_COMBOBOXW) == 0)
    {
        COMBOBOXINFO info = { sizeof(info) };
        GetComboBoxInfo(hwndEdit, &info);
        hwndEdit = info.hwndItem;
    }
    else if (lstrcmpiW(szClass, WC_COMBOBOXEXW) == 0)
    {
        hwndEdit = (HWND)SendMessageW(hwndEdit, CBEM_GETEDITCONTROL, 0, 0);
    }

    IACList2 *pACList = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IACList2, reinterpret_cast<LPVOID *>(&pACList));
    if (FAILED(hr))
    {
        TRACE("CoCreateInstance(CLSID_ACListISF): 0x%08lX\n", hr);
        return hr;
    }
    pInfo->pvACList = static_cast<LPVOID>(pACList);

    IAutoComplete2 *pAC = NULL;
    hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                          IID_IAutoComplete2, reinterpret_cast<LPVOID *>(&pAC));
    if (SUCCEEDED(hr))
    {
        pAC->Init(hwndEdit, pACList, NULL, NULL);
        pAC->SetOptions(ACO_AUTOSUGGEST);
        pAC->QueryInterface(IID_IAutoCompleteDropDown, &pInfo->pvDropDown);
        pAC->Release();
    }
    else
    {
        TRACE("CoCreateInstance(CLSID_AutoComplete): 0x%08lX\n", hr);
        pACList->Release();
        pInfo->pvACList = NULL;
        return hr;
    }

    pACList->QueryInterface(IID_ICurrentWorkingDirectory, &pInfo->pvCWD);

    return hr;
}

EXTERN_C HRESULT
DoUpdateAutoCompleteWithCWD(const FileOpenDlgInfos *info, LPCITEMIDLIST pidl)
{
    FileOpenDlgInfos *pInfo = const_cast<FileOpenDlgInfos*>(info);
    if (!pInfo)
        return E_POINTER;

    ICurrentWorkingDirectory* pCWD =
        reinterpret_cast<ICurrentWorkingDirectory*>(pInfo->pvCWD);

    IAutoCompleteDropDown* pDropDown =
        reinterpret_cast<IAutoCompleteDropDown*>(pInfo->pvDropDown);

    IACList2* pACList = static_cast<IACList2*>(pInfo->pvACList);

    WCHAR szPath[MAX_PATH];
    if (!pidl || !SHGetPathFromIDListW(pidl, szPath))
    {
        GetCurrentDirectoryW(_countof(szPath), szPath);
    }

    if (pCWD)
        pCWD->SetDirectory(szPath);
    if (pDropDown)
        pDropDown->ResetEnumerator();
    if (pACList)
        pACList->Expand(L"");

    return S_OK;
}

EXTERN_C HRESULT
DoReleaseAutoCompleteWithCWD(FileOpenDlgInfos *pInfo)
{
    if (!pInfo)
        return E_POINTER;

    ICurrentWorkingDirectory* pCWD =
        reinterpret_cast<ICurrentWorkingDirectory*>(pInfo->pvCWD);
    if (pCWD)
        pCWD->Release();

    IAutoCompleteDropDown* pDropDown =
        reinterpret_cast<IAutoCompleteDropDown*>(pInfo->pvDropDown);
    if (pDropDown)
        pDropDown->Release();

    IACList2 *pACList = static_cast<IACList2*>(pInfo->pvACList);
    if (pACList)
        pACList->Release();

    pInfo->pvCWD = pInfo->pvDropDown = pInfo->pvACList = NULL;
    return S_OK;
}
