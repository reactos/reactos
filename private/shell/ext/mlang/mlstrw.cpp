// MLStrW.cpp : Implementation of CMLStrW
#include "private.h"

#ifndef NEWMLSTR

#ifdef ASTRIMPL
#include "mlstr.h"
#include "mlsbwalk.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrW

STDMETHODIMP CMLStrW::Sync(BOOL fNoAccess)
{
    ASSERT_THIS;
    return GetOwner()->Sync(fNoAccess);
}

STDMETHODIMP CMLStrW::GetLength(long* plLen)
{
    ASSERT_THIS;
    return GetOwner()->GetLength(plLen);
}

STDMETHODIMP CMLStrW::SetMLStr(long lDestPos, long lDestLen, IUnknown* pSrcMLStr, long lSrcPos, long lSrcLen)
{
    ASSERT_THIS;
    return GetOwner()->SetMLStr(lDestPos, lDestLen, pSrcMLStr, lSrcPos, lSrcLen);
}

STDMETHODIMP CMLStrW::GetMLStr(long lSrcPos, long lSrcLen, IUnknown* pUnkOuter, DWORD dwClsContext, const IID* piid, IUnknown** ppDestMLStr, long* plDestPos, long* plDestLen)
{
    ASSERT_THIS;
    return GetOwner()->GetMLStr(lSrcPos, lSrcLen, pUnkOuter, dwClsContext, piid, ppDestMLStr, plDestPos, plDestLen);
}

STDMETHODIMP CMLStrW::SetWStr(long lDestPos, long lDestLen, const WCHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
    CMLStr::CLock Lock(TRUE, pOwner, hr);
    long cchDestPos;
    long cchDestLen;
    long cchActual;
    long lActualLen;

    if (SUCCEEDED(hr) && (pOwner->GetBufFlags() & MLSTR_WRITE))
        hr = E_INVALIDARG; // Not writable StrBuf; TODO: Replace StrBuf in this case if allowed

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pOwner->PrepareMLStrBuf()) &&
        SUCCEEDED(hr = pOwner->RegularizePosLen(&lDestPos, &lDestLen)) &&
        SUCCEEDED(hr = pOwner->GetCCh(0, lDestPos, &cchDestPos)) &&
        SUCCEEDED(hr = pOwner->GetCCh(cchDestPos, lDestLen, &cchDestLen)))
    {
        IMLangStringBufW* const pMLStrBufW = pOwner->GetMLStrBufW();

        if (pMLStrBufW)
        {
            if (cchSrc > cchDestLen)
            {
                hr = pMLStrBufW->Insert(cchDestPos, cchSrc - cchDestLen, (pcchActual || plActualLen) ? &cchSrc : NULL);
                cchSrc += cchDestLen;
            }
            else if  (cchSrc < cchDestLen)
            {
                hr = pMLStrBufW->Delete(cchDestPos, cchDestLen - cchSrc);
            }

            CMLStrBufWalkW BufWalk(pMLStrBufW, cchDestPos, cchSrc, (pcchActual || plActualLen));

            lActualLen = 0;
            while (BufWalk.Lock(hr))
            {
                long lLen;

                if (plActualLen)
                    hr = pOwner->CalcLenW(pszSrc, BufWalk.GetCCh(), &lLen);

                if (SUCCEEDED(hr))
                {
                    lActualLen += lLen;
                    ::memcpy(BufWalk.GetStr(), pszSrc, sizeof(WCHAR) * BufWalk.GetCCh());
                    pszSrc += BufWalk.GetCCh();
                }

                BufWalk.Unlock(hr);
            }

            cchActual = BufWalk.GetDoneCCh();
        }
        else
        {
            IMLangStringBufA* const pMLStrBufA = pOwner->GetMLStrBufA(); // Should succeed because PrepareMLStrBuf() above was succeeded
            const UINT uCodePage = pOwner->GetCodePage();
            long cchSrcA;

            if (SUCCEEDED(hr = pOwner->ConvWStrToAStr(pcchActual || plActualLen, uCodePage, pszSrc, cchSrc, NULL, 0, &cchSrcA, NULL, NULL)))
            {
                if (cchSrcA > cchDestLen)
                {
                    hr = pMLStrBufA->Insert(cchDestPos, cchSrcA - cchDestLen, (pcchActual || plActualLen) ? &cchSrcA : NULL);
                    cchSrcA += cchDestLen;
                }
                else if  (cchSrcA < cchDestLen)
                {
                    hr = pMLStrBufA->Delete(cchDestPos, cchDestLen - cchSrcA);
                }
            }

            CMLStrBufWalkA BufWalk(pMLStrBufA, cchDestPos, cchSrcA, (pcchActual || plActualLen));

            cchActual = 0;
            lActualLen = 0;
            while (BufWalk.Lock(hr))
            {
                long cchWrittenA;
                long cchWrittenW;
                long lWrittenLen;

                if (SUCCEEDED(hr = pOwner->ConvWStrToAStr(pcchActual || plActualLen, uCodePage, pszSrc, cchSrc, BufWalk.GetStr(), BufWalk.GetCCh(), &cchWrittenA, &cchWrittenW, &lWrittenLen)))
                {
                    pszSrc += cchWrittenW;
                    cchSrc -= cchWrittenW;
                    cchActual += cchWrittenW;
                    lActualLen += lWrittenLen;
                }

                BufWalk.Unlock(hr, cchWrittenA);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pcchActual)
            *pcchActual = cchActual;
        if (plActualLen)
            *plActualLen = lActualLen;
    }
    else
    {
        if (pcchActual)
            *pcchActual = 0;
        if (plActualLen)
            *plActualLen = 0;
    }
    return hr;
}

STDMETHODIMP CMLStrW::SetStrBufW(long lDestPos, long lDestLen, IMLangStringBufW* pSrcBuf, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    return GetOwner()->SetStrBufCommon(this, lDestPos, lDestLen, 0, pSrcBuf, NULL, pcchActual, plActualLen);
}

STDMETHODIMP CMLStrW::GetWStr(long lSrcPos, long lSrcLen, WCHAR* pszDest, long cchDest, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_BLOCK_OR_NULL(pszDest, cchDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
    CMLStr::CLock Lock(FALSE, pOwner, hr);
    long cchSrcPos;
    long cchSrcLen;
    long cchActual;
    long lActualLen;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pOwner->RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = pOwner->GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = pOwner->GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
        IMLangStringBufW* const pMLStrBufW = pOwner->GetMLStrBufW();
        IMLangStringBufA* const pMLStrBufA = pOwner->GetMLStrBufA();

        if (pszDest)
            cchActual = min(cchSrcLen, cchDest);
        else
            cchActual = cchSrcLen;

        if (pMLStrBufW)
        {
            CMLStrBufWalkW BufWalk(pMLStrBufW, cchSrcPos, cchActual, (pcchActual || plActualLen));

            lActualLen = 0;
            while (BufWalk.Lock(hr))
            {
                long lLen;

                if (plActualLen)
                    hr = pOwner->CalcLenW(BufWalk.GetStr(), BufWalk.GetCCh(), &lLen);

                if (SUCCEEDED(hr))
                {
                    lActualLen += lLen;

                    if (pszDest)
                    {
                        ::memcpy(pszDest, BufWalk.GetStr(), sizeof(WCHAR) * BufWalk.GetCCh());
                        pszDest += BufWalk.GetCCh();
                    }
                }

                BufWalk.Unlock(hr);
            }

            cchActual = BufWalk.GetDoneCCh();
        }
        else if (pMLStrBufA)
        {
            CMLStrBufWalkA BufWalk(pMLStrBufA, cchSrcPos, cchActual, (pcchActual || plActualLen));

            cchActual = 0;
            lActualLen = 0;
            while ((!pszDest || cchDest > 0) && BufWalk.Lock(hr))
            {
                CHAR* const pszBuf = BufWalk.GetStr();
                long cchWrittenA;
                long cchWrittenW;
                long lWrittenLen;

                if (SUCCEEDED(hr = pOwner->ConvAStrToWStr(pOwner->GetCodePage(), pszBuf, BufWalk.GetCCh(), pszDest, cchDest, &cchWrittenA, &cchWrittenW, &lWrittenLen)))
                {
                    lActualLen += lWrittenLen;
                    cchActual += cchWrittenW;

                    if (pszDest)
                    {
                        pszDest += cchWrittenW;
                        cchDest -= cchWrittenW;
                    }
                }

                BufWalk.Unlock(hr, cchWrittenA);
            }
        }
        else
        {
            ASSERT(cchActual == 0); // MLStrBuf is not available
            lActualLen = 0;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pcchActual)
            *pcchActual = cchActual;
        if (plActualLen)
            *plActualLen = lActualLen;
    }
    else
    {
        if (pcchActual)
            *pcchActual = 0;
        if (plActualLen)
            *plActualLen = 0;
    }
    return hr;
}

STDMETHODIMP CMLStrW::GetStrBufW(long lSrcPos, long lSrcMaxLen, IMLangStringBufW** ppDestBuf, long* plDestLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppDestBuf);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
    CMLStr::CLock Lock(FALSE, pOwner, hr);
    IMLangStringBufW* pMLStrBufW;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pOwner->RegularizePosLen(&lSrcPos, &lSrcMaxLen)) &&
        lSrcMaxLen <= 0)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        pMLStrBufW = pOwner->GetMLStrBufW();
        if (!pMLStrBufW)
            hr = MLSTR_E_STRBUFNOTAVAILABLE;
    }

    if (SUCCEEDED(hr))
    {
        if (ppDestBuf)
        {
            pMLStrBufW->AddRef();
            *ppDestBuf = pMLStrBufW;
        }
        if (plDestLen)
            *plDestLen = lSrcMaxLen;
    }
    else
    {
        if (ppDestBuf)
            *ppDestBuf = NULL;
        if (plDestLen)
            *plDestLen = 0;
    }

    return hr;
}

STDMETHODIMP CMLStrW::LockWStr(long lSrcPos, long lSrcLen, long lFlags, long cchRequest, WCHAR** ppszDest, long* pcchDest, long* plDestLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppszDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchDest);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
    CMLStr::CLock Lock(lFlags & MLSTR_WRITE, pOwner, hr);
    long cchSrcPos;
    long cchSrcLen;
    WCHAR* pszBuf = NULL;
    long cchBuf;
    long lLockLen;
    BOOL fDirectLock;

    if (SUCCEEDED(hr) && (!lFlags || (lFlags & ~pOwner->GetBufFlags() & MLSTR_WRITE)))
        hr = E_INVALIDARG; // No flags specified, or not writable StrBuf; TODO: Replace StrBuf in this case if allowed

    if (!(lFlags & MLSTR_WRITE))
        cchRequest = 0;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pOwner->PrepareMLStrBuf()) &&
        SUCCEEDED(hr = pOwner->RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = pOwner->GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = pOwner->GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
        IMLangStringBufW* const pMLStrBufW = pOwner->GetMLStrBufW();
        fDirectLock = (pMLStrBufW != 0);

        if (fDirectLock)
        {
            long cchInserted;
            long cchLockLen = cchSrcLen;

            if (cchRequest > cchSrcLen &&
                SUCCEEDED(hr = pMLStrBufW->Insert(cchSrcPos + cchSrcLen, cchRequest - cchSrcLen, &cchInserted)))
            {
                pOwner->SetBufCCh(pOwner->GetBufCCh() + cchInserted);
                cchLockLen += cchInserted;

                if (!pcchDest && cchLockLen < cchRequest)
                    hr = E_OUTOFMEMORY; // Can't insert in StrBuf
            }

            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = pMLStrBufW->LockBuf(cchSrcPos, cchLockLen, &pszBuf, &cchBuf)) &&
                !pcchDest && cchBuf < max(cchSrcLen, cchRequest))
            {
                hr = E_OUTOFMEMORY; // Can't lock StrBuf
            }

            if (plDestLen && SUCCEEDED(hr))
                hr = pOwner->CalcLenW(pszBuf, cchBuf, &lLockLen);
        }
        else
        {
            long cchSize;

            if (SUCCEEDED(hr = pOwner->CalcBufSizeW(lSrcLen, &cchSize)))
            {
                cchBuf = max(cchSize, cchRequest);
                hr = pOwner->MemAlloc(sizeof(*pszBuf) * cchBuf, (void**)&pszBuf);
            }

            if (SUCCEEDED(hr) && (lFlags & MLSTR_READ))
                hr = GetWStr(lSrcPos, lSrcLen,  pszBuf, cchBuf, (pcchDest) ? &cchBuf : NULL, (plDestLen) ? &lLockLen : NULL);
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = Lock.FallThrough()))
    {
        hr = pOwner->GetLockInfo()->Lock((fDirectLock) ? pOwner->UnlockWStrDirect : pOwner->UnlockWStrIndirect, lFlags, 0, pszBuf, lSrcPos, lSrcLen, cchSrcPos, cchBuf);
    }

    if (SUCCEEDED(hr))
    {
        if (ppszDest)
            *ppszDest = pszBuf;
        if (pcchDest)
            *pcchDest = cchBuf;
        if (plDestLen)
            *plDestLen = lLockLen;
    }
    else
    {
        if (pszBuf)
        {
            if (fDirectLock)
                pOwner->GetMLStrBufW()->UnlockBuf(pszBuf, 0, 0);
            else
                pOwner->MemFree(pszBuf);
        }

        if (ppszDest)
            *ppszDest = NULL;
        if (pcchDest)
            *pcchDest = 0;
        if (plDestLen)
            *plDestLen = 0;
    }

    return hr;
}

STDMETHODIMP CMLStrW::UnlockWStr(const WCHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    return GetOwner()->UnlockStrCommon(pszSrc, cchSrc, pcchActual, plActualLen);
}

STDMETHODIMP CMLStrW::SetLocale(long lDestPos, long lDestLen, LCID locale)
{
    ASSERT_THIS;
    return GetOwner()->SetLocale(lDestPos, lDestLen, locale);
}

STDMETHODIMP CMLStrW::GetLocale(long lSrcPos, long lSrcMaxLen, LCID* plocale, long* plLocalePos, long* plLocaleLen)
{
    ASSERT_THIS;
    return GetOwner()->GetLocale(lSrcPos, lSrcMaxLen, plocale, plLocalePos, plLocaleLen);
}
#endif

#else // NEWMLSTR

#include "mlstr.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrW

CMLStrW::CMLStrW(void) :
    m_pAttrWStr(NULL),
    m_pAttrLocale(NULL),
    m_dwAttrWStrCookie(NULL),
    m_dwAttrLocaleCookie(NULL)
{
    ::InitializeCriticalSection(&m_cs);
}

CMLStrW::~CMLStrW(void)
{
    if (m_dwAttrLocaleCookie)
        GetOwner()->UnregisterAttr(m_dwAttrLocaleCookie);
    if (m_dwAttrWStrCookie)
        GetOwner()->UnregisterAttr(m_dwAttrWStrCookie);
    if (m_pAttrLocale)
        m_pAttrLocale->Release();
    if (m_pAttrWStr)
        m_pAttrWStr->Release();

    ::DeleteCriticalSection(&m_cs);
}

HRESULT CMLStrW::GetAttrWStrReal(IMLStrAttrWStr** ppAttr)
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection(&m_cs);

    if (!m_pAttrWStr)
    {
        IMLStrAttrWStr* pAttr;

        hr = ::CoCreateInstance(CLSID_CMLStrAttrWStr, NULL, CLSCTX_ALL, IID_IUnknown, (void**)&pAttr);

        if (SUCCEEDED(hr))
            hr = GetOwner()->RegisterAttr(pAttr, &m_dwAttrWStrCookie);

        if (SUCCEEDED(hr))
        {
            pAttr->Release();

            hr = GetOwner()->FindAttr(IID_IMLStrAttrWStr, 0, (IUnknown**)&pAttr);
        }

        if (SUCCEEDED(hr))
            m_pAttrWStr = pAttr;
    }

    if (ppAttr)
        *ppAttr = m_pAttrWStr;

    ::LeaveCriticalSection(&m_cs);

    return hr;
}

HRESULT CMLStrW::GetAttrLocaleReal(IMLStrAttrLocale** ppAttr)
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection(&m_cs);

    if (!m_pAttrLocale)
    {
        IMLStrAttrLocale* pAttr;

        hr = ::CoCreateInstance(CLSID_CMLStrAttrLocale, NULL, CLSCTX_ALL, IID_IUnknown, (void**)&pAttr);

        if (SUCCEEDED(hr))
            hr = GetOwner()->RegisterAttr(pAttr, &m_dwAttrLocaleCookie);

        if (SUCCEEDED(hr))
        {
            pAttr->Release();

            hr = GetOwner()->FindAttr(IID_IMLStrAttrLocale, 0, (IUnknown**)&pAttr);
        }

        if (SUCCEEDED(hr))
            m_pAttrLocale = pAttr;
    }

    if (ppAttr)
        *ppAttr = m_pAttrLocale;

    ::LeaveCriticalSection(&m_cs);

    return hr;
}

STDMETHODIMP CMLStrW::LockMLStr(long lPos, long lLen, DWORD dwFlags, DWORD* pdwCookie, long* plActualPos, long* plActualLen)
{
    ASSERT_THIS;
    return GetOwner()->LockMLStr(lPos, lLen, dwFlags, pdwCookie, plActualPos, plActualLen);
}

STDMETHODIMP CMLStrW::UnlockMLStr(DWORD dwCookie)
{
    ASSERT_THIS;
    return GetOwner()->UnlockMLStr(dwCookie);
}

STDMETHODIMP CMLStrW::GetLength(long* plLen)
{
    ASSERT_THIS;
    return GetOwner()->GetLength(plLen);
}

STDMETHODIMP CMLStrW::SetMLStr(long lDestPos, long lDestLen, IUnknown* pSrcMLStr, long lSrcPos, long lSrcLen)
{
    ASSERT_THIS;
    return GetOwner()->SetMLStr(lDestPos, lDestLen, pSrcMLStr, lSrcPos, lSrcLen);
}

STDMETHODIMP CMLStrW::RegisterAttr(IUnknown* pUnk, DWORD* pdwCookie)
{
    ASSERT_THIS;
    return GetOwner()->RegisterAttr(pUnk, pdwCookie);
}

STDMETHODIMP CMLStrW::UnregisterAttr(DWORD dwCookie)
{
    ASSERT_THIS;
    return GetOwner()->UnregisterAttr(dwCookie);
}

STDMETHODIMP CMLStrW::EnumAttr(IEnumUnknown** ppEnumUnk)
{
    ASSERT_THIS;
    return GetOwner()->EnumAttr(ppEnumUnk);
}

STDMETHODIMP CMLStrW::FindAttr(REFIID riid, LPARAM lParam, IUnknown** ppUnk)
{
    ASSERT_THIS;
    return GetOwner()->FindAttr(riid, lParam, ppUnk);
}

STDMETHODIMP CMLStrW::SetWStr(long lDestPos, long lDestLen, const WCHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrWStr* pAttr;
    HRESULT hr = GetAttrWStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->SetWStr(lDestPos, lDestLen, pszSrc, cchSrc, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrW::SetStrBufW(long lDestPos, long lDestLen, IMLangStringBufW* pSrcBuf, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrWStr* pAttr;
    HRESULT hr = GetAttrWStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->SetStrBufW(lDestPos, lDestLen, pSrcBuf, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrW::GetWStr(long lSrcPos, long lSrcLen, WCHAR* pszDest, long cchDest, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrWStr* pAttr;
    HRESULT hr = GetAttrWStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->GetWStr(lSrcPos, lSrcLen, pszDest, cchDest, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrW::GetStrBufW(long lSrcPos, long lSrcMaxLen, IMLangStringBufW** ppDestBuf, long* plDestLen)
{
    ASSERT_THIS;

    IMLStrAttrWStr* pAttr;
    HRESULT hr = GetAttrWStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->GetStrBufW(lSrcPos, lSrcMaxLen, ppDestBuf, plDestLen);

    return hr;
}

STDMETHODIMP CMLStrW::LockWStr(long lSrcPos, long lSrcLen, long lFlags, long cchRequest, WCHAR** ppszDest, long* pcchDest, long* plDestLen)
{
    ASSERT_THIS;

    IMLStrAttrWStr* pAttr;
    HRESULT hr = GetAttrWStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->LockWStr(lSrcPos, lSrcLen, lFlags, cchRequest, ppszDest, pcchDest, plDestLen);

    return hr;
}

STDMETHODIMP CMLStrW::UnlockWStr(const WCHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrWStr* pAttr;
    HRESULT hr = GetAttrWStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->UnlockWStr(pszSrc, cchSrc, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrW::SetLocale(long lDestPos, long lDestLen, LCID locale)
{
    ASSERT_THIS;

    IMLStrAttrLocale* pAttr;
    HRESULT hr = GetAttrLocale(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->SetLong(lDestPos, lDestLen, (long)locale);

    return hr;
}

STDMETHODIMP CMLStrW::GetLocale(long lSrcPos, long lSrcMaxLen, LCID* plocale, long* plLocalePos, long* plLocaleLen)
{
    ASSERT_THIS;

    IMLStrAttrLocale* pAttr;
    HRESULT hr = GetAttrLocale(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->GetLong(lSrcPos, lSrcMaxLen, (long*)plocale, plLocalePos, plLocaleLen);

    return hr;
}

#endif // NEWMLSTR
