

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

EXTERN_C HRESULT WINAPI SHCreateShellItem(
    _In_opt_ PCIDLIST_ABSOLUTE pidlParent,
    _In_opt_ IShellFolder *psfParent,
    _In_ PCUITEMID_CHILD pidl,
    _Out_ IShellItem **ppsi);

/***********************************************************************
 *   CShellItemArray
 */

CShellItemArray::CShellItemArray() : m_pHIDA(NULL)
{
}

CShellItemArray::~CShellItemArray()
{
    delete m_pHIDA;
}

HRESULT
CShellItemArray::Initialize(_In_ IDataObject *pdo)
{
    m_pHIDA = new CDataObjectHIDA(pdo);
    if (!m_pHIDA)
        return E_OUTOFMEMORY;

    HRESULT hr = m_pHIDA->hr();
    if (FAILED(hr))
    {
        delete m_pHIDA;
        m_pHIDA = NULL;
    }
    return hr;
}

UINT
CShellItemArray::ItemCount() const
{
    return m_pHIDA ? (*m_pHIDA)->cidl : 0;
}

STDMETHODIMP
CShellItemArray::BindToHandler(_In_opt_ IBindCtx *pbc, _In_ REFGUID rbhid, _In_ REFIID riid, _Out_ void **ppv)
{
    UNIMPLEMENTED;
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CShellItemArray::GetPropertyStore(_In_ GETPROPERTYSTOREFLAGS flags, _In_ REFIID riid, _Out_ void **ppv)
{
    UNIMPLEMENTED;
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CShellItemArray::GetPropertyDescriptionList(_In_ REFPROPERTYKEY keyType, _In_ REFIID riid, _Out_ void **ppv)
{
    UNIMPLEMENTED;
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CShellItemArray::GetAttributes(_In_ SIATTRIBFLAGS dwAttribFlags, _In_ SFGAOF sfgaoMask, _Out_ SFGAOF *psfgaoAttribs)
{
    UNIMPLEMENTED;
    *psfgaoAttribs = 0;
    return E_NOTIMPL;
}

STDMETHODIMP
CShellItemArray::GetCount(_Out_ DWORD *pCount)
{
    if (!pCount)
        return E_INVALIDARG;
    *pCount = ItemCount();
    return S_OK;
}

STDMETHODIMP
CShellItemArray::GetItemAt(_In_ DWORD nIndex, _Out_ IShellItem **ppItem)
{
    if (!ppItem)
        return E_INVALIDARG;
    *ppItem = NULL;
    if (!m_pHIDA)
        return E_UNEXPECTED;
    if (nIndex >= ItemCount())
        return E_FAIL;
    return SHCreateShellItem(HIDA_GetPIDLFolder(*m_pHIDA), NULL,
                             HIDA_GetPIDLItem(*m_pHIDA, nIndex), ppItem);
}

STDMETHODIMP
CShellItemArray::EnumItems(_Out_ IEnumShellItems **ppESI)
{
    UNIMPLEMENTED;
    *ppESI = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *   CSimpleShellItemArray
 */

HRESULT
CSimpleShellItemArray::Initialize(_In_reads_(count) IShellItem **pitems, _In_ UINT count)
{
    if (!m_items.SetCount(0))
        return E_OUTOFMEMORY;
    for (UINT i = 0; i < count; ++i)
    {
        CComPtr<IShellItem> ptr = pitems[i];
        if (!ptr)
            return E_INVALIDARG;
        m_items.Add(ptr);
    }
    return S_OK;
}

STDMETHODIMP
CSimpleShellItemArray::BindToHandler(_In_opt_ IBindCtx *pbc, _In_ REFGUID rbhid, _In_ REFIID riid, _Out_ void **ppv)
{
    UNIMPLEMENTED;
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CSimpleShellItemArray::GetPropertyStore(_In_ GETPROPERTYSTOREFLAGS flags, _In_ REFIID riid, _Out_ void **ppv)
{
    UNIMPLEMENTED;
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CSimpleShellItemArray::GetPropertyDescriptionList(_In_ REFPROPERTYKEY keyType, _In_ REFIID riid, _Out_ void **ppv)
{
    UNIMPLEMENTED;
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP
CSimpleShellItemArray::GetAttributes(_In_ SIATTRIBFLAGS dwAttribFlags, _In_ SFGAOF sfgaoMask, _Out_ SFGAOF *psfgaoAttribs)
{
    HRESULT hr = S_OK;
    SFGAOF attr;

    if (!psfgaoAttribs)
        return E_POINTER;

    if (dwAttribFlags & ~(SIATTRIBFLAGS_AND | SIATTRIBFLAGS_OR))
        FIXME("%08x contains unsupported attribution flags\n", dwAttribFlags);

    for (size_t i = 0; i < m_items.GetCount(); ++i)
    {
        hr = m_items[i]->GetAttributes(sfgaoMask, &attr);
        if (FAILED_UNEXPECTEDLY(hr))
            break;

        if (i == 0)
        {
            *psfgaoAttribs = attr;
            continue;
        }

        if ((dwAttribFlags & SIATTRIBFLAGS_MASK) == SIATTRIBFLAGS_AND)
            *psfgaoAttribs &= attr;
        else
            *psfgaoAttribs |= attr;
    }

    if (SUCCEEDED(hr))
    {
        if (*psfgaoAttribs == sfgaoMask)
            return S_OK;
        return S_FALSE;
    }
    return hr;
}

STDMETHODIMP
CSimpleShellItemArray::GetCount(_Out_ DWORD *pdwNumItems)
{
    if (!pdwNumItems)
        return E_INVALIDARG;
    *pdwNumItems = static_cast<DWORD>(m_items.GetCount());
    return S_OK;
}

STDMETHODIMP
CSimpleShellItemArray::GetItemAt(_In_ DWORD dwIndex, _Out_ IShellItem **ppsi)
{
    if (!ppsi)
        return E_INVALIDARG;
    *ppsi = NULL;
    if (dwIndex >= m_items.GetCount())
        return E_FAIL;
    *ppsi = m_items[dwIndex];
    (*ppsi)->AddRef();
    return S_OK;
}

STDMETHODIMP
CSimpleShellItemArray::EnumItems(_Out_ IEnumShellItems **ppenumShellItems)
{
    UNIMPLEMENTED;
    *ppenumShellItems = NULL;
    return E_NOTIMPL;
}

HRESULT
CreateShellItemArrayFromItems(_In_reads_(cidl) IShellItem **items, _In_ UINT cidl, _In_ REFIID riid, _Out_ void **ppv)
{
    return ShellObjectCreatorInit<CSimpleShellItemArray>(items, cidl, riid, ppv);
}
