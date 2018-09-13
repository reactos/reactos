// AttrLoc.cpp : Implementation of CMLStrAttrLocale
#include "private.h"

#ifdef NEWMLSTR

#include "attrloc.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrLocale


CMLStrAttrLocale::CMLStrAttrLocale() :
    m_lLen(0),
    m_lcid(0)
{
}

STDMETHODIMP CMLStrAttrLocale::SetClient(IUnknown* pUnk)
{
    return E_NOTIMPL; // CMLStrAttrLocale::SetClient()
}

STDMETHODIMP CMLStrAttrLocale::GetClient(IUnknown** ppUnk)
{
    return E_NOTIMPL; // CMLStrAttrLocale::GetClient()
}

STDMETHODIMP CMLStrAttrLocale::QueryAttr(REFIID riid, LPARAM lParam, IUnknown** ppUnk, long* lConf)
{
    return E_NOTIMPL; // CMLStrAttrLocale::QueryAttr()
}

STDMETHODIMP CMLStrAttrLocale::GetAttrInterface(IID* pIID, LPARAM* plParam)
{
    return E_NOTIMPL; // CMLStrAttrLocale::GetAttrInterface()
}

STDMETHODIMP CMLStrAttrLocale::SetMLStr(long lDestPos, long lDestLen, IUnknown* pSrcMLStr, long lSrcPos, long lSrcLen)
{
    if (pSrcMLStr)
    {
        return E_NOTIMPL; // CMLStrAttrLocale::SetMLStr()
    }
    else
    {
        HRESULT hr = ::RegularizePosLen(m_lLen, &lDestPos, &lDestLen);

        m_lLen -= lDestLen;
        m_lLen += lSrcLen; // Insert default

        return S_OK;
    }
}

STDMETHODIMP CMLStrAttrLocale::SetLong(long lDestPos, long lDestLen, long lValue)
{
    ASSERT_THIS;

    HRESULT hr = ::RegularizePosLen(m_lLen, &lDestPos, &lDestLen);

    if (SUCCEEDED(hr) && lDestPos == 0)
        m_lcid = (LCID)lValue; // In this version, saves only first locale.

    return hr;
}

STDMETHODIMP CMLStrAttrLocale::GetLong(long lSrcPos, long lSrcLen, long* plValue, long* plActualPos, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(plValue);
    ASSERT_WRITE_PTR_OR_NULL(plActualPos);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr = ::RegularizePosLen(m_lLen, &lSrcPos, &lSrcLen);

    if (SUCCEEDED(hr))
    {
        if (plValue)
            *plValue = (long)m_lcid;
        if (plActualPos)
            *plActualPos = lSrcPos;
        if (plActualLen)
            *plActualLen = lSrcLen;
    }
    else
    {
        if (plValue)
            *plValue = 0;
        if (plActualPos)
            *plActualPos = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    return hr;
}

#endif // NEWMLSTR
