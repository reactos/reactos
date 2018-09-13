// MLStrA.cpp : Implementation of CMLStrA
#include "private.h"

#ifndef NEWMLSTR

#include "mlstr.h"
#ifdef ASTRIMPL
#include "mlswalk.h"
#include "mlsbwalk.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CMLStrA

#ifdef ASTRIMPL
CMLStrA::CMLStrA(void) :
    m_pMLCPs(NULL)
{
}

CMLStrA::~CMLStrA(void)
{
    if (m_pMLCPs)
        m_pMLCPs->Release();
}
#endif

STDMETHODIMP CMLStrA::Sync(BOOL fNoAccess)
{
    ASSERT_THIS;
    return GetOwner()->Sync(fNoAccess);
}

STDMETHODIMP CMLStrA::GetLength(long* plLen)
{
    ASSERT_THIS;
    return GetOwner()->GetLength(plLen);
}

STDMETHODIMP CMLStrA::SetMLStr(long lDestPos, long lDestLen, IUnknown* pSrcMLStr, long lSrcPos, long lSrcLen)
{
    ASSERT_THIS;
    return GetOwner()->SetMLStr(lDestPos, lDestLen, pSrcMLStr, lSrcPos, lSrcLen);
}

STDMETHODIMP CMLStrA::GetMLStr(long lSrcPos, long lSrcLen, IUnknown* pUnkOuter, DWORD dwClsContext, const IID* piid, IUnknown** ppDestMLStr, long* plDestPos, long* plDestLen)
{
    ASSERT_THIS;
    return GetOwner()->GetMLStr(lSrcPos, lSrcLen, pUnkOuter, dwClsContext, piid, ppDestMLStr, plDestPos, plDestLen);
}

STDMETHODIMP CMLStrA::SetAStr(long lDestPos, long lDestLen, UINT uCodePage, const CHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
#ifdef ASTRIMPL
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
        IMLangStringBufA* const pMLStrBufA = pOwner->GetMLStrBufA();

        if (uCodePage == CP_ACP)
            uCodePage = ::GetACP();

        if (pMLStrBufA && uCodePage == pOwner->GetCodePage())
        {
            if (cchSrc > cchDestLen)
            {
                hr = pMLStrBufA->Insert(cchDestPos, cchSrc - cchDestLen, (pcchActual || plActualLen) ? &cchSrc : NULL);
                cchSrc += cchDestLen;
            }
            else if  (cchSrc < cchDestLen)
            {
                hr = pMLStrBufA->Delete(cchDestPos, cchDestLen - cchSrc);
            }

            CMLStrBufWalkA BufWalk(pMLStrBufA, cchDestPos, cchSrc, (pcchActual || plActualLen));

            lActualLen = 0;
            while (BufWalk.Lock(hr))
            {
                long lLen;

                if (plActualLen)
                    hr = pOwner->CalcLenA(uCodePage, pszSrc, BufWalk.GetCCh(), &lLen);

                if (SUCCEEDED(hr))
                {
                    lActualLen += lLen;
                    ::memcpy(BufWalk.GetStr(), pszSrc, sizeof(CHAR) * BufWalk.GetCCh());
                    pszSrc += BufWalk.GetCCh();
                }

                BufWalk.Unlock(hr);
            }

            cchActual = BufWalk.GetDoneCCh();
        }
        else
        {
            IMLangStringWStr* pMLStrW;

            if (SUCCEEDED(hr = pOwner->QueryInterface(IID_IMLangStringWStr, (void**)&pMLStrW)))
            {
                CMLStrWalkW StrWalk(pMLStrW, lDestPos, lDestLen, MLSTR_WRITE, (pcchActual || plActualLen));

                cchActual = 0;
                lActualLen = 0;
                while (StrWalk.Lock(hr))
                {
                    long cchWrittenA;
                    long lWrittenLen;

                    if (SUCCEEDED(hr = pOwner->ConvAStrToWStr(uCodePage, pszSrc, cchSrc, StrWalk.GetStr(), StrWalk.GetCCh(), &cchWrittenA, NULL, &lWrittenLen)))
                    {
                        pszSrc += cchWrittenA;
                        cchSrc -= cchWrittenA;
                        cchActual += cchWrittenA;
                        lActualLen += lWrittenLen;
                    }

                    StrWalk.Unlock(hr, lWrittenLen);
                }

                pMLStrW->Release();
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
#else
    return E_NOTIMPL; // !ASTRIMPL
#endif
}

STDMETHODIMP CMLStrA::SetStrBufA(long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufA* pSrcBuf, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    return GetOwner()->SetStrBufCommon(this, lDestPos, lDestLen, uCodePage, NULL, pSrcBuf, pcchActual, plActualLen);
}

STDMETHODIMP CMLStrA::GetAStr(long lSrcPos, long lSrcLen, UINT uCodePageIn, UINT* puCodePageOut, CHAR* pszDest, long cchDest, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puCodePageOut);
    ASSERT_WRITE_BLOCK_OR_NULL(pszDest, cchDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
#ifdef ASTRIMPL
    CMLStr::CLock Lock(FALSE, pOwner, hr);
#endif
    long cchSrcPos;
    long cchSrcLen;
    long cchActual;
    long lActualLen;

#ifndef ASTRIMPL
    if (SUCCEEDED(hr = m_pOwner->CheckThread()) &&
        (m_pOwner->IsLocked() || puCodePageOut || uCodePageIn != m_pOwner->GetCodePage())) // !ASTRIMPL
    {
        hr = E_INVALIDARG;
    }
#endif

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pOwner->RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = pOwner->GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = pOwner->GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
#ifdef ASTRIMPL
        IMLangStringBufA* const pMLStrBufA = pOwner->GetMLStrBufA();

        if (pszDest)
            cchActual = min(cchSrcLen, cchDest);
        else
            cchActual = cchSrcLen;

        if (uCodePageIn == CP_ACP)
            uCodePageIn = ::GetACP();

        if (pMLStrBufA && (puCodePageOut || uCodePageIn == pOwner->GetCodePage()))
        {
            uCodePageIn = pOwner->GetCodePage();

            CMLStrBufWalkA BufWalk(pMLStrBufA, cchSrcPos, cchActual, (pcchActual || plActualLen));

            lActualLen = 0;
            while (BufWalk.Lock(hr))
            {
                long lLen;

                if (plActualLen)
                    hr = pOwner->CalcLenA(uCodePageIn, BufWalk.GetStr(), BufWalk.GetCCh(), &lLen);

                if (SUCCEEDED(hr))
                {
                    lActualLen += lLen;

                    if (pszDest)
                    {
                        ::memcpy(pszDest, BufWalk.GetStr(), sizeof(CHAR) * BufWalk.GetCCh());
                        pszDest += BufWalk.GetCCh();
                    }
                }

                BufWalk.Unlock(hr);
            }

            cchActual = BufWalk.GetDoneCCh();
        }
        else
        {
            IMLangStringWStr* pMLStrW;

            if (SUCCEEDED(hr = pOwner->QueryInterface(IID_IMLangStringWStr, (void**)&pMLStrW)))
            {
                BOOL fDontHaveCodePageIn = (puCodePageOut != 0);
                CMLStrWalkW StrWalk(pMLStrW, lSrcPos, lSrcLen, (pcchActual || plActualLen));

                cchActual = 0;
                while (StrWalk.Lock(hr))
                {
                    LCID locale;
                    UINT uLocaleCodePage;
                    DWORD dwLocaleCodePages;
                    DWORD dwStrCodePages;
                    long cchWritten;
                    long lWrittenLen;

                    if (fDontHaveCodePageIn &&
                        SUCCEEDED(hr = pOwner->GetLocale(lSrcPos, lSrcLen, &locale, NULL, NULL)) &&
                        SUCCEEDED(hr = ::LocaleToCodePage(locale, &uLocaleCodePage)) &&
                        SUCCEEDED(hr = PrepareMLangCodePages()) &&
                        SUCCEEDED(hr = GetMLangCodePages()->CodePageToCodePages(uLocaleCodePage, &dwLocaleCodePages)) &&
                        SUCCEEDED(hr = GetMLangCodePages()->GetStrCodePages(StrWalk.GetStr(), StrWalk.GetCCh(), dwLocaleCodePages, &dwStrCodePages, NULL)))
                    {
                        fDontHaveCodePageIn = FALSE;
                        hr = GetMLangCodePages()->CodePagesToCodePage(dwStrCodePages, uLocaleCodePage, &uCodePageIn);
                    }

                    if (SUCCEEDED(hr) &&
                        SUCCEEDED(hr = pOwner->ConvWStrToAStr(pcchActual || plActualLen, uCodePageIn, StrWalk.GetStr(), StrWalk.GetCCh(), pszDest, cchDest, &cchWritten, NULL, &lWrittenLen)))
                    {
                        pszDest += cchWritten;
                        cchDest -= cchWritten;
                        cchActual += cchWritten;
                    }

                    StrWalk.Unlock(hr, lWrittenLen);
                }

                lActualLen = StrWalk.GetDoneLen();

                pMLStrW->Release();
            }
        }
    }
#else
        if (pszDest)
        {
            hr = E_FAIL; // TODO: Not implemented in this version
        }
        else
        {
            cchActual = cchSrcLen;
        }
    }

    if (SUCCEEDED(hr) && plActualLen)
        hr = m_pOwner->CalcLenA(m_pOwner->GetCodePage(), 0, cchActual, &lActualLen);
#endif

    if (SUCCEEDED(hr))
    {
#ifdef ASTRIMPL
        if (puCodePageOut)
            *puCodePageOut = uCodePageIn;
#endif
        if (pcchActual)
            *pcchActual = cchActual;
        if (plActualLen)
            *plActualLen = lActualLen;
    }
    else
    {
        if (puCodePageOut)
            *puCodePageOut = 0;
        if (pcchActual)
            *pcchActual = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    return hr;
}

STDMETHODIMP CMLStrA::GetStrBufA(long lSrcPos, long lSrcMaxLen, UINT* puDestCodePage, IMLangStringBufA** ppDestBuf, long* plDestLen)
{
#ifdef ASTRIMPL
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puDestCodePage);
    ASSERT_WRITE_PTR_OR_NULL(ppDestBuf);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
    CMLStr::CLock Lock(FALSE, pOwner, hr);
    IMLangStringBufA* pMLStrBufA;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pOwner->RegularizePosLen(&lSrcPos, &lSrcMaxLen)) &&
        lSrcMaxLen <= 0)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        pMLStrBufA = pOwner->GetMLStrBufA();
        if (!pMLStrBufA)
            hr = MLSTR_E_STRBUFNOTAVAILABLE;
    }

    if (SUCCEEDED(hr))
    {
        if (puDestCodePage)
            *puDestCodePage = pOwner->GetCodePage();
        if (ppDestBuf)
        {
            pMLStrBufA->AddRef();
            *ppDestBuf = pMLStrBufA;
        }
        if (plDestLen)
            *plDestLen = lSrcMaxLen;
    }
    else
    {
        if (puDestCodePage)
            *puDestCodePage = 0;
        if (ppDestBuf)
            *ppDestBuf = NULL;
        if (plDestLen)
            *plDestLen = 0;
    }

    return hr;
#else
    return E_NOTIMPL; // !ASTRIMPL
#endif
}

STDMETHODIMP CMLStrA::LockAStr(long lSrcPos, long lSrcLen, long lFlags, UINT uCodePageIn, long cchRequest, UINT* puCodePageOut, CHAR** ppszDest, long* pcchDest, long* plDestLen)
{
#ifdef ASTRIMPL
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puCodePageOut);
    ASSERT_WRITE_PTR_OR_NULL(ppszDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchDest);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    POWNER const pOwner = GetOwner();
    HRESULT hr = pOwner->CheckThread();
    CMLStr::CLock Lock(lFlags & MLSTR_WRITE, pOwner, hr);
    long cchSrcPos;
    long cchSrcLen;
    CHAR* pszBuf = NULL;
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
        IMLangStringBufA* const pMLStrBufA = pOwner->GetMLStrBufA();
        fDirectLock = (pMLStrBufA && (puCodePageOut || uCodePageIn == pOwner->GetCodePage()));

        if (fDirectLock)
        {
            long cchInserted;
            long cchLockLen = cchSrcLen;

            if (puCodePageOut)
                hr = GetAStr(lSrcPos, lSrcLen, 0, &uCodePageIn, NULL, 0, NULL, NULL);

            if (SUCCEEDED(hr) &&
                cchRequest > cchSrcLen &&
                SUCCEEDED(hr = pMLStrBufA->Insert(cchSrcPos + cchSrcLen, cchRequest - cchSrcLen, &cchInserted)))
            {
                pOwner->SetBufCCh(pOwner->GetBufCCh() + cchInserted);
                cchLockLen += cchInserted;

                if (!pcchDest && cchLockLen < cchRequest)
                    hr = E_OUTOFMEMORY; // Can't insert in StrBuf
            }

            if (SUCCEEDED(hr) &&
                SUCCEEDED(hr = pMLStrBufA->LockBuf(cchSrcPos, cchLockLen, &pszBuf, &cchBuf)) &&
                !pcchDest && cchBuf < max(cchSrcLen, cchRequest))
            {
                hr = E_OUTOFMEMORY; // Can't lock StrBuf
            }

            if (plDestLen && SUCCEEDED(hr))
                hr = pOwner->CalcLenA(uCodePageIn, pszBuf, cchBuf, &lLockLen);
        }
        else
        {
            long cchSize;

            if (SUCCEEDED(hr = pOwner->CalcBufSizeA(lSrcLen, &cchSize)))
            {
                cchBuf = max(cchSize, cchRequest);
                hr = pOwner->MemAlloc(sizeof(*pszBuf) * cchBuf, (void**)&pszBuf);
            }

            if (SUCCEEDED(hr) && ((lFlags & MLSTR_READ) || puCodePageOut))
                hr = GetAStr(lSrcPos, lSrcLen,  uCodePageIn, (puCodePageOut) ? &uCodePageIn : NULL, (lFlags & MLSTR_READ) ? pszBuf : NULL, cchBuf, (pcchDest) ? &cchBuf : NULL, (plDestLen) ? &lLockLen : NULL);
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = Lock.FallThrough()))
    {
        hr = pOwner->GetLockInfo()->Lock((fDirectLock) ? pOwner->UnlockAStrDirect : pOwner->UnlockAStrIndirect, lFlags, uCodePageIn, pszBuf, lSrcPos, lSrcLen, cchSrcPos, cchBuf);
    }

    if (SUCCEEDED(hr))
    {
        if (puCodePageOut)
            *puCodePageOut = uCodePageIn;
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
                pOwner->GetMLStrBufA()->UnlockBuf(pszBuf, 0, 0);
            else
                pOwner->MemFree(pszBuf);
        }

        if (puCodePageOut)
            *puCodePageOut = 0;
        if (ppszDest)
            *ppszDest = NULL;
        if (pcchDest)
            *pcchDest = 0;
        if (plDestLen)
            *plDestLen = 0;
    }

    return hr;
#else
    return E_NOTIMPL; // !ASTRIMPL
#endif
}

STDMETHODIMP CMLStrA::UnlockAStr(const CHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
#ifdef ASTRIMPL
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    return GetOwner()->UnlockStrCommon(pszSrc, cchSrc, pcchActual, plActualLen);
#else
    return E_NOTIMPL; // !ASTRIMPL
#endif
}

STDMETHODIMP CMLStrA::SetLocale(long lDestPos, long lDestLen, LCID locale)
{
    ASSERT_THIS;
    return GetOwner()->SetLocale(lDestPos, lDestLen, locale);
}

STDMETHODIMP CMLStrA::GetLocale(long lSrcPos, long lSrcMaxLen, LCID* plocale, long* plLocalePos, long* plLocaleLen)
{
    ASSERT_THIS;
    return GetOwner()->GetLocale(lSrcPos, lSrcMaxLen, plocale, plLocalePos, plLocaleLen);
}

#else // NEWMLSTR

#include "mlstr.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrA

CMLStrA::CMLStrA(void) :
    m_pAttrAStr(NULL),
    m_pAttrLocale(NULL),
    m_dwAttrAStrCookie(NULL),
    m_dwAttrLocaleCookie(NULL)
{
    ::InitializeCriticalSection(&m_cs);
}

CMLStrA::~CMLStrA(void)
{
    if (m_dwAttrLocaleCookie)
        GetOwner()->UnregisterAttr(m_dwAttrLocaleCookie);
    if (m_dwAttrAStrCookie)
        GetOwner()->UnregisterAttr(m_dwAttrAStrCookie);
    if (m_pAttrLocale)
        m_pAttrLocale->Release();
    if (m_pAttrAStr)
        m_pAttrAStr->Release();

    ::DeleteCriticalSection(&m_cs);
}

HRESULT CMLStrA::GetAttrAStrReal(IMLStrAttrAStr** ppAttr)
{
    HRESULT hr = S_OK;

    ::EnterCriticalSection(&m_cs);

    if (!m_pAttrAStr)
    {
        IMLStrAttrAStr* pAttr;

        hr = ::CoCreateInstance(CLSID_CMLStrAttrAStr, NULL, CLSCTX_ALL, IID_IUnknown, (void**)&pAttr);

        if (SUCCEEDED(hr))
            hr = GetOwner()->RegisterAttr(pAttr, &m_dwAttrAStrCookie);

        if (SUCCEEDED(hr))
        {
            pAttr->Release();

            hr = GetOwner()->FindAttr(IID_IMLStrAttrAStr, 0, (IUnknown**)&pAttr);
        }

        if (SUCCEEDED(hr))
            m_pAttrAStr = pAttr;
    }

    if (ppAttr)
        *ppAttr = m_pAttrAStr;

    ::LeaveCriticalSection(&m_cs);

    return hr;
}

HRESULT CMLStrA::GetAttrLocaleReal(IMLStrAttrLocale** ppAttr)
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

STDMETHODIMP CMLStrA::LockMLStr(long lPos, long lLen, DWORD dwFlags, DWORD* pdwCookie, long* plActualPos, long* plActualLen)
{
    ASSERT_THIS;
    return GetOwner()->LockMLStr(lPos, lLen, dwFlags, pdwCookie, plActualPos, plActualLen);
}

STDMETHODIMP CMLStrA::UnlockMLStr(DWORD dwCookie)
{
    ASSERT_THIS;
    return GetOwner()->UnlockMLStr(dwCookie);
}

STDMETHODIMP CMLStrA::GetLength(long* plLen)
{
    ASSERT_THIS;
    return GetOwner()->GetLength(plLen);
}

STDMETHODIMP CMLStrA::SetMLStr(long lDestPos, long lDestLen, IUnknown* pSrcMLStr, long lSrcPos, long lSrcLen)
{
    ASSERT_THIS;
    return GetOwner()->SetMLStr(lDestPos, lDestLen, pSrcMLStr, lSrcPos, lSrcLen);
}

STDMETHODIMP CMLStrA::RegisterAttr(IUnknown* pUnk, DWORD* pdwCookie)
{
    ASSERT_THIS;
    return GetOwner()->RegisterAttr(pUnk, pdwCookie);
}

STDMETHODIMP CMLStrA::UnregisterAttr(DWORD dwCookie)
{
    ASSERT_THIS;
    return GetOwner()->UnregisterAttr(dwCookie);
}

STDMETHODIMP CMLStrA::EnumAttr(IEnumUnknown** ppEnumUnk)
{
    ASSERT_THIS;
    return GetOwner()->EnumAttr(ppEnumUnk);
}

STDMETHODIMP CMLStrA::FindAttr(REFIID riid, LPARAM lParam, IUnknown** ppUnk)
{
    ASSERT_THIS;
    return GetOwner()->FindAttr(riid, lParam, ppUnk);
}

STDMETHODIMP CMLStrA::SetAStr(long lDestPos, long lDestLen, UINT uCodePage, const CHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrAStr* pAttr;
    HRESULT hr = GetAttrAStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->SetAStr(lDestPos, lDestLen, uCodePage, pszSrc, cchSrc, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrA::SetStrBufA(long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufA* pSrcBuf, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrAStr* pAttr;
    HRESULT hr = GetAttrAStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->SetStrBufA(lDestPos, lDestLen, uCodePage, pSrcBuf, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrA::GetAStr(long lSrcPos, long lSrcLen, UINT uCodePageIn, UINT* puCodePageOut, CHAR* pszDest, long cchDest, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrAStr* pAttr;
    HRESULT hr = GetAttrAStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->GetAStr(lSrcPos, lSrcLen, uCodePageIn, puCodePageOut, pszDest, cchDest, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrA::GetStrBufA(long lSrcPos, long lSrcMaxLen, UINT* puDestCodePage, IMLangStringBufA** ppDestBuf, long* plDestLen)
{
    ASSERT_THIS;

    IMLStrAttrAStr* pAttr;
    HRESULT hr = GetAttrAStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->GetStrBufA(lSrcPos, lSrcMaxLen, puDestCodePage, ppDestBuf, plDestLen);

    return hr;
}

STDMETHODIMP CMLStrA::LockAStr(long lSrcPos, long lSrcLen, long lFlags, UINT uCodePageIn, long cchRequest, UINT* puCodePageOut, CHAR** ppszDest, long* pcchDest, long* plDestLen)
{
    ASSERT_THIS;

    IMLStrAttrAStr* pAttr;
    HRESULT hr = GetAttrAStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->LockAStr(lSrcPos, lSrcLen, lFlags, uCodePageIn, cchRequest, puCodePageOut, ppszDest, pcchDest, plDestLen);

    return hr;
}

STDMETHODIMP CMLStrA::UnlockAStr(const CHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;

    IMLStrAttrAStr* pAttr;
    HRESULT hr = GetAttrAStr(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->UnlockAStr(pszSrc, cchSrc, pcchActual, plActualLen);

    return hr;
}

STDMETHODIMP CMLStrA::SetLocale(long lDestPos, long lDestLen, LCID locale)
{
    ASSERT_THIS;

    IMLStrAttrLocale* pAttr;
    HRESULT hr = GetAttrLocale(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->SetLong(lDestPos, lDestLen, (long)locale);

    return hr;
}

STDMETHODIMP CMLStrA::GetLocale(long lSrcPos, long lSrcMaxLen, LCID* plocale, long* plLocalePos, long* plLocaleLen)
{
    ASSERT_THIS;

    IMLStrAttrLocale* pAttr;
    HRESULT hr = GetAttrLocale(&pAttr);

    if (SUCCEEDED(hr))
        hr = pAttr->GetLong(lSrcPos, lSrcMaxLen, (long*)plocale, plLocalePos, plLocaleLen);

    return hr;
}

#endif NEWMLSTR
