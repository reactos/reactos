#include "shelltest.h"

// + Adapted from https://devblogs.microsoft.com/oldnewthing/20130503-00/?p=4463
// In short: We want to create an IDLIST from an item that does not exist,
// so we have to provide WIN32_FIND_DATAW in a bind context.
// If we don't, the FS will be queried, and we do not get a valid IDLIST for a non-existing path.

CComModule gModule;

class CFileSysBindData :
    public CComCoClass<CFileSysBindData>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IFileSystemBindData
{
public:
    virtual HRESULT STDMETHODCALLTYPE SetFindData(const WIN32_FIND_DATAW *pfd)
    {
        m_Data = *pfd;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFindData(WIN32_FIND_DATAW *pfd)
    {
        *pfd = m_Data;
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CFileSysBindData)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CFileSysBindData)
        COM_INTERFACE_ENTRY_IID(IID_IFileSystemBindData, IFileSystemBindData)
    END_COM_MAP()
private:
    WIN32_FIND_DATAW m_Data;
};

static
HRESULT
AddFileSysBindCtx(_In_ IBindCtx *pbc)
{
    CComPtr<IFileSystemBindData> spfsbc(new CComObject<CFileSysBindData>());
    WIN32_FIND_DATAW wfd = { 0 };
    wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    spfsbc->SetFindData(&wfd);
    HRESULT hr = pbc->RegisterObjectParam((LPOLESTR)STR_FILE_SYS_BIND_DATA, spfsbc);
    ok(hr == S_OK, "hr = %lx\n", hr);
    return hr;
}

static
HRESULT
CreateBindCtxWithOpts(_In_ BIND_OPTS *pbo, _Outptr_ IBindCtx **ppbc)
{
    CComPtr<IBindCtx> spbc;
    HRESULT hr = CreateBindCtx(0, &spbc);
    ok(hr == S_OK, "hr = %lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = spbc->SetBindOptions(pbo);
        ok(hr == S_OK, "hr = %lx\n", hr);
    }
    *ppbc = SUCCEEDED(hr) ? spbc.Detach() : NULL;
    return hr;
}

static HRESULT
CreateFileSysBindCtx(_Outptr_ IBindCtx **ppbc)
{
    CComPtr<IBindCtx> spbc;
    BIND_OPTS bo = { sizeof(bo), 0, STGM_CREATE, 0 };
    HRESULT hr = CreateBindCtxWithOpts(&bo, &spbc);
    ok(hr == S_OK, "hr = %lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = AddFileSysBindCtx(spbc);
        ok(hr == S_OK, "hr = %lx\n", hr);
    }
    *ppbc = SUCCEEDED(hr) ? spbc.Detach() : NULL;
    return hr;
}

VOID
PathToIDList(LPCWSTR pszPath, ITEMIDLIST** ppidl)
{
    CComPtr<IBindCtx> spbc;
    HRESULT hr = CreateFileSysBindCtx(&spbc);
    ok(hr == S_OK, "hr = %lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = SHParseDisplayName(pszPath, spbc, ppidl, 0, NULL);
        ok(hr == S_OK, "hr = %lx\n", hr);
    }
}

// - Adapted from https://devblogs.microsoft.com/oldnewthing/20130503-00/?p=4463
