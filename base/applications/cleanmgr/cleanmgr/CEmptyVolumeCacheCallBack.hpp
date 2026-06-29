/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CEmptyVolumeCacheCallBack definition / implementation
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */


// We don't really use this, but some windows handlers crash without it
struct CEmptyVolumeCacheCallBack
    : public IEmptyVolumeCacheCallBack
{

    STDMETHOD_(ULONG, AddRef)() throw()
    {
        return 2;
    }
    STDMETHOD_(ULONG, Release)() throw()
    {
        return 1;
    }
    STDMETHOD(QueryInterface)(
        REFIID riid,
        _COM_Outptr_ void** ppvObject) throw()
    {
        if (riid == IID_IUnknown || riid == IID_IEmptyVolumeCacheCallBack)
        {
            *ppvObject = (IUnknown*)this;
            return S_OK;
        }
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }


    STDMETHODIMP ScanProgress(
        _In_ DWORDLONG dwlSpaceUsed,
        _In_ DWORD dwFlags,
        _In_ LPCWSTR pcwszStatus) override
    {
        DPRINT("dwlSpaceUsed: %lld, dwFlags: %x\n", dwlSpaceUsed, dwFlags);
        return S_OK;
    }

    STDMETHODIMP PurgeProgress(
        _In_ DWORDLONG dwlSpaceFreed,
        _In_ DWORDLONG dwlSpaceToFree,
        _In_ DWORD dwFlags,
        _In_ LPCWSTR pcwszStatus) override
    {
        DPRINT("dwlSpaceFreed: %lld, dwlSpaceToFree: %lld, dwFlags: %x\n", dwlSpaceFreed, dwlSpaceToFree, dwFlags);
        return S_OK;
    }
};
