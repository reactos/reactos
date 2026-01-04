/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     CRecycleBinCleaner implementation
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


CLSID CLSID_RecycleBinCleaner = { 0x5ef4af3a, 0xf726, 0x11d0, { 0xb8, 0xa2, 0x00, 0xc0, 0x4f, 0xc3, 0x09, 0xa4 } };

struct CRecycleBinCleaner :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRecycleBinCleaner, &CLSID_RecycleBinCleaner>,
    public IEmptyVolumeCache2
{
    WCHAR m_wszVolume[4];

    void
    OutputResourceString(DWORD dwResId, _Out_ LPWSTR *ppwszOutput)
    {
        CStringW Tmp(MAKEINTRESOURCEW(dwResId));
        SHStrDupW(Tmp, ppwszOutput);
    }

public:

    // +IEmptyVolumeCache
    STDMETHODIMP Initialize(
        _In_ HKEY hkRegKey,
        _In_ LPCWSTR pcwszVolume,
        _Out_ LPWSTR* ppwszDisplayName,
        _Out_ LPWSTR* ppwszDescription,
        _Out_ DWORD* pdwFlags)
    {
        if (!pdwFlags)
            return E_POINTER;

        *pdwFlags = EVCF_HASSETTINGS;

        OutputResourceString(IDS_RECYCLE_CLEANER_DISPLAYNAME, ppwszDisplayName);
        OutputResourceString(IDS_RECYCLE_CLEANER_DESCRIPTION, ppwszDescription);

        return StringCchCopyW(m_wszVolume, _countof(m_wszVolume), pcwszVolume);
    }

    STDMETHODIMP GetSpaceUsed(
        _Out_ DWORDLONG* pdwlSpaceUsed,
        _In_opt_ IEmptyVolumeCacheCallBack* picb)
    {
        if (!pdwlSpaceUsed)
            return E_POINTER;

        SHQUERYRBINFO bin = { sizeof(bin) };
        HRESULT hr = SHQueryRecycleBinW(m_wszVolume, &bin);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            bin.i64Size = 0;
        }
        *pdwlSpaceUsed = bin.i64Size;
        if (picb)
        {
            picb->ScanProgress(bin.i64Size, EVCCBF_LASTNOTIFICATION, NULL);
        }

        return S_OK;
    }

    STDMETHODIMP Purge(
        _In_ DWORDLONG dwlSpaceToFree,
        _In_opt_ IEmptyVolumeCacheCallBack *picb)
    {
        DWORDLONG dwlPrevious = 0;
        GetSpaceUsed(&dwlPrevious, NULL);

        SHEmptyRecycleBinW(NULL, m_wszVolume, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
        if (picb)
        {
            picb->PurgeProgress(dwlPrevious, 0, EVCCBF_LASTNOTIFICATION, NULL);
        }

        return S_OK;
    }

    STDMETHODIMP ShowProperties(
        _In_ HWND hwnd)
    {
        CComHeapPtr<ITEMIDLIST> pidl;
        HRESULT hr;
        if (FAILED_UNEXPECTEDLY(hr = SHGetSpecialFolderLocation(hwnd, CSIDL_BITBUCKET, &pidl)))
            return hr;

        SHELLEXECUTEINFOW seei = {sizeof(seei)};
        seei.hwnd = hwnd;
        seei.lpVerb = L"open";
        seei.nShow = SW_SHOWNORMAL;
        seei.fMask = SEE_MASK_IDLIST;
        seei.lpIDList = pidl;
        ShellExecuteExW(&seei);

        return S_OK;
    }

    STDMETHODIMP Deactivate(
        _Out_ DWORD* pdwFlags)
    {
        if (!pdwFlags)
            return E_POINTER;

        *pdwFlags = 0;

        return S_OK;
    }
    // -IEmptyVolumeCache


    // +IEmptyVolumeCache2
    STDMETHODIMP InitializeEx(
        _In_ HKEY hkRegKey,
        _In_ LPCWSTR pcwszVolume,
        _In_ LPCWSTR pcwszKeyName,
        _Out_ LPWSTR* ppwszDisplayName,
        _Out_ LPWSTR* ppwszDescription,
        _Out_ LPWSTR* ppwszBtnText,
        _Out_ DWORD* pdwFlags)
    {
        OutputResourceString(IDS_RECYCLE_CLEANER_BUTTON_TEXT, ppwszBtnText);

        return Initialize(hkRegKey, pcwszVolume, ppwszDisplayName, ppwszDescription, pdwFlags);
    }
    // -IEmptyVolumeCache2


    DECLARE_PROTECT_FINAL_CONSTRUCT();
    DECLARE_REGISTRY_RESOURCEID(IDR_RECYCLEBINCLEANER)
    DECLARE_NOT_AGGREGATABLE(CRecycleBinCleaner)

    BEGIN_COM_MAP(CRecycleBinCleaner)
        COM_INTERFACE_ENTRY_IID(IID_IEmptyVolumeCache2, IEmptyVolumeCache2)
        COM_INTERFACE_ENTRY_IID(IID_IEmptyVolumeCache, IEmptyVolumeCache)
        COM_INTERFACE_ENTRY_IID(IID_IUnknown, IUnknown)
    END_COM_MAP()
};


OBJECT_ENTRY_AUTO(CLSID_RecycleBinCleaner, CRecycleBinCleaner)
