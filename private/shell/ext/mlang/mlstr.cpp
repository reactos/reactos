// MLStr.cpp : Implementation of CMLStr
#include "private.h"

#ifndef NEWMLSTR

#include "mlstr.h"
#ifdef ASTRIMPL
#include "mlsbwalk.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CMLStr Helper functions

HRESULT RegularizePosLen(long lStrLen, long* plPos, long* plLen)
{
    ASSERT_WRITE_PTR(plPos);
    ASSERT_WRITE_PTR(plLen);

    long lPos = *plPos;
    long lLen = *plLen;

    if (lPos < 0)
        lPos = lStrLen;
    else
        lPos = min(lPos, lStrLen);

    if (lLen < 0)
        lLen = lStrLen - lPos;
    else
        lLen = min(lLen, lStrLen - lPos);

    *plPos = lPos;
    *plLen = lLen;

    return S_OK;
}

#ifdef ASTRIMPL
HRESULT LocaleToCodePage(LCID locale, UINT* puCodePage)
{
    HRESULT hr = S_OK;

    if (puCodePage)
    {
        TCHAR szCodePage[8];

        if (::GetLocaleInfo(locale, LOCALE_IDEFAULTANSICODEPAGE, szCodePage, ARRAYSIZE(szCodePage)) > 0)
            *puCodePage = _ttoi(szCodePage);
        else
            hr = E_FAIL; // NLS failed
    }

    return hr;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CMLStr

CMLStr::CMLStr(void) :
    m_pMLStrBufW(NULL),
    m_pMLStrBufA(NULL),
    m_lBufFlags(0),
    m_cchBuf(0),
    m_locale(0),
#ifdef ASTRIMPL
    m_LockInfo(this)
#else
    m_lLockFlags(0)
#endif
{
    m_dwThreadID = ::GetCurrentThreadId();
}

CMLStr::~CMLStr(void)
{
    if (m_pMLStrBufW)
        m_pMLStrBufW->Release();
    if (m_pMLStrBufA)
        m_pMLStrBufA->Release();
}

STDMETHODIMP CMLStr::Sync(BOOL)
{
    ASSERT_THIS;
    return S_OK; // No multithread supported; Always synchronized
}

STDMETHODIMP CMLStr::GetLength(long* plLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(plLen);

    HRESULT hr = CheckThread();
#ifdef ASTRIMPL
    CLock Lock(FALSE, this, hr);
#endif
    long lLen;

    if (SUCCEEDED(hr))
        hr = GetLen(0, GetBufCCh(), &lLen);

    if (plLen)
    {
        if (SUCCEEDED(hr))
            *plLen = lLen;
        else
            *plLen = 0;
    }

    return hr;
}

STDMETHODIMP CMLStr::SetMLStr(long, long, IUnknown*, long, long)
{
    return E_NOTIMPL; // IMLangString::SetMLStr()
}

STDMETHODIMP CMLStr::GetMLStr(long, long, IUnknown*, DWORD, const IID*, IUnknown**, long*, long*)
{
    return E_NOTIMPL; // IMLangString::GetMLStr()
}

#ifndef ASTRIMPL
STDMETHODIMP CMLStr::SetWStr(long lDestPos, long lDestLen, const WCHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    return E_NOTIMPL; // !ASTRIMPL
}

STDMETHODIMP CMLStr::SetStrBufW(long lDestPos, long lDestLen, IMLangStringBufW* pSrcBuf, long* pcchActual, long* plActualLen)
{
    return SetStrBufCommon(NULL, lDestPos, lDestLen, 0, pSrcBuf, NULL, pcchActual, plActualLen);
}
#endif

HRESULT CMLStr::SetStrBufCommon(void* pMLStrX, long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufW* pSrcBufW, IMLangStringBufA* pSrcBufA, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_PTR_OR_NULL(pSrcBufW);
    ASSERT_READ_PTR_OR_NULL(pSrcBufA);
    ASSERT(!pSrcBufW || !pSrcBufA); // Either one or both should be NULL
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr = CheckThread();
#ifdef ASTRIMPL
    CLock Lock(TRUE, this, hr);
#endif
    long lBufFlags = 0; // '= 0' for in case of both of pSrcBufW and pSrcBufA are NULL
    long cchBuf = 0;
    long cchDestPos;
    long cchDestLen;
    long lActualLen = 0;

#ifndef ASTRIMPL
    if (SUCCEEDED(hr) && IsLocked())
        hr = E_INVALIDARG; // This MLStr is locked
#endif

    if (SUCCEEDED(hr) &&
        (!pSrcBufW || SUCCEEDED(hr = pSrcBufW->GetStatus(&lBufFlags, &cchBuf))) &&
        (!pSrcBufA || SUCCEEDED(hr = pSrcBufA->GetStatus(&lBufFlags, &cchBuf))) &&
        SUCCEEDED(hr = RegularizePosLen(&lDestPos, &lDestLen)) &&
        SUCCEEDED(hr = GetCCh(0, lDestPos, &cchDestPos)) &&
        SUCCEEDED(hr = GetCCh(cchDestPos, lDestLen, &cchDestLen)))
    {
        if (!cchDestPos && cchDestLen == GetBufCCh()) // Replacing entire string
        {
            IMLangStringBufW* const pOldBufW = GetMLStrBufW();
            IMLangStringBufA* const pOldBufA = GetMLStrBufA();

            if (pOldBufW)
                pOldBufW->Release();
            else if (pOldBufA)
                pOldBufA->Release();

            if (pSrcBufW)
                pSrcBufW->AddRef();
            else if (pSrcBufA)
                pSrcBufA->AddRef();

            SetMLStrBufW(pSrcBufW);
            SetMLStrBufA(pSrcBufA);
            SetCodePage(uCodePage);
            SetBufFlags(lBufFlags);
            SetBufCCh(cchBuf);

            if (plActualLen)
                hr = GetLen(0, GetBufCCh(), &lActualLen);
        }
        else
        {
#ifdef ASTRIMPL
            if (pSrcBufW)
            {
                CMLStrBufWalkW BufWalk(pSrcBufW, 0, cchBuf, (pcchActual || plActualLen));

                while (BufWalk.Lock(hr))
                {
                    long cchSet;
                    long lSetLen;

                    hr = ((IMLangStringWStr*)pMLStrX)->SetWStr(lDestPos, lDestLen, BufWalk.GetStr(), BufWalk.GetCCh(), &cchSet, (plActualLen) ? &lSetLen : NULL);
                    lActualLen += lSetLen;
                    BufWalk.Unlock(hr, cchSet);
                }

                cchBuf = BufWalk.GetDoneCCh();

                pSrcBufW->Release();
            }
            else if (pSrcBufA && pMLStrX)
            {
                CMLStrBufWalkA BufWalk(pSrcBufA, 0, cchBuf, (pcchActual || plActualLen));

                while (BufWalk.Lock(hr))
                {
                    long cchSet;
                    long lSetLen;

                    hr = ((IMLangStringAStr*)pMLStrX)->SetAStr(lDestPos, lDestLen, uCodePage, BufWalk.GetStr(), BufWalk.GetCCh(), &cchSet, (plActualLen) ? &lSetLen : NULL);
                    lActualLen += lSetLen;
                    BufWalk.Unlock(hr, cchSet);
                }

                cchBuf = BufWalk.GetDoneCCh();

                pSrcBufA->Release();
            }
            else
            {
                hr = SetMLStr(lDestPos, lDestLen, NULL, 0, 0);
            }
#else
            hr = E_INVALIDARG; // !ASTRIMPL
#endif
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pcchActual)
            *pcchActual = cchBuf;
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

#ifndef ASTRIMPL
STDMETHODIMP CMLStr::GetWStr(long lSrcPos, long lSrcLen, WCHAR* pszDest, long cchDest, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_BLOCK_OR_NULL(pszDest, cchDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr = CheckThread();
    long cchSrcPos;
    long cchSrcLen;
    long cchActual;
    long lActualLen;

    if (SUCCEEDED(hr) && IsLocked())
        hr = E_INVALIDARG; // This MLStr is locked

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
        if (pszDest)
        {
            long cchActualTemp = min(cchSrcLen, cchDest);
            cchActual = cchActualTemp;

            while (SUCCEEDED(hr) && cchActualTemp > 0)
            {
                WCHAR* pszBuf;
                long cchBuf;

                if (m_pMLStrBufW)
                {
                    if (SUCCEEDED(hr = m_pMLStrBufW->LockBuf(cchSrcPos, cchActualTemp, &pszBuf, &cchBuf)))
                    {
                        ::memcpy(pszDest, pszBuf, sizeof(WCHAR) * cchBuf);
                        hr = m_pMLStrBufW->UnlockBuf(pszBuf, 0, 0);

                        cchSrcPos += cchBuf;
                        cchActualTemp -= cchBuf;
                        pszDest += cchBuf;
                    }
                }
                else // m_pMLStrBufW
                {
                    hr = E_FAIL;  // !ASTRIMPL
                }
            }

            if (FAILED(hr) && cchActualTemp < cchActual && (pcchActual || plActualLen))
            {
                cchActual -= cchActualTemp;
                hr = S_OK;
            }
        }
        else
        {
            cchActual = cchSrcLen;
        }
    }

    if (SUCCEEDED(hr) && plActualLen)
        hr = CalcLenW(0, cchActual, &lActualLen);

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

STDMETHODIMP CMLStr::GetStrBufW(long, long, IMLangStringBufW**, long*)
{
    return E_NOTIMPL; // !ASTRIMPL
}

STDMETHODIMP CMLStr::LockWStr(long lSrcPos, long lSrcLen, long lFlags, long cchRequest, WCHAR** ppszDest, long* pcchDest, long* plDestLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppszDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchDest);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    HRESULT hr = CheckThread();
    long cchSrcPos;
    long cchSrcLen;
    WCHAR* pszBuf = NULL;
    long cchBuf;
    long lLockLen;

    if (SUCCEEDED(hr) && (IsLocked() || !lFlags || (lFlags & ~GetBufFlags() & MLSTR_WRITE)))
        hr = E_INVALIDARG; // This MLStr is locked, no flags specified or not writable

    if (!(lFlags & MLSTR_WRITE))
        cchRequest = 0;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = PrepareMLStrBuf()) &&
        SUCCEEDED(hr = RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
        IMLangStringBufW* const pMLStrBufW = GetMLStrBufW();
        SetDirectLockFlag(pMLStrBufW != 0);

        if (IsDirectLock())
        {
            long cchInserted;
            long cchLockLen = cchSrcLen;

            if (cchRequest > cchSrcLen &&
                SUCCEEDED(hr = pMLStrBufW->Insert(cchSrcPos + cchSrcLen, cchRequest - cchSrcLen, &cchInserted)))
            {
                SetBufCCh(GetBufCCh() + cchInserted);
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

        }
        else if (m_pMLStrBufA)
        {
            long cchSize;

            if (SUCCEEDED(hr = CalcBufSizeW(lSrcLen, &cchSize)))
            {
                cchBuf = max(cchSize, cchRequest);
                hr = MemAlloc(sizeof(*pszBuf) * cchBuf, (void**)&pszBuf);
            }

            if (SUCCEEDED(hr) && (lFlags & MLSTR_READ))
                hr = ConvertMLStrBufAToWStr(m_uCodePage, m_pMLStrBufA, cchSrcPos, cchSrcLen, pszBuf, cchBuf, (pcchDest) ? &cchBuf : NULL);
        }
        else
        {
            hr = E_FAIL; // !ASTRIMPL
        }
    }

    if (plDestLen && SUCCEEDED(hr))
        hr = CalcLenW(pszBuf, cchBuf, &lLockLen);

    if (SUCCEEDED(hr))
    {
        SetLockFlags(lFlags);
        m_pszLockBuf = pszBuf;
        m_cchLockPos = cchSrcPos;
        m_cchLockLen = cchBuf;
        m_lLockPos = lSrcPos;
        m_lLockLen = lSrcLen;

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
            if (IsDirectLock())
                GetMLStrBufW()->UnlockBuf(pszBuf, 0, 0);
            else
                MemFree(pszBuf);
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
#endif

#ifdef ASTRIMPL
HRESULT CMLStr::UnlockWStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr;
    IMLangStringBufW* const pMLStrBufW = GetMLStrBufW();
    const long cchLockLen = GetLockInfo()->GetCChLen(pKey);

    if (SUCCEEDED(hr = pMLStrBufW->UnlockBuf((WCHAR*)pszSrc, 0, cchSrc)) &&
        (GetLockInfo()->GetFlags(pKey) & MLSTR_WRITE))
    {
        if (cchSrc < cchLockLen)
        {
            if (SUCCEEDED(hr = pMLStrBufW->Delete(GetLockInfo()->GetCChPos(pKey) + cchSrc, cchLockLen - cchSrc)))
                SetBufCCh(GetBufCCh() - (cchLockLen - cchSrc));
        }

        if (SUCCEEDED(hr) && plActualLen)
            hr = CalcLenW((WCHAR*)pszSrc, cchSrc, plActualLen);

        if (pcchActual)
            *pcchActual = cchSrc;
    }

    return hr;
}

HRESULT CMLStr::UnlockWStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr = S_OK;

    if (GetLockInfo()->GetFlags(pKey) & MLSTR_WRITE)
    {
        CComQIPtr<IMLangStringWStr, &IID_IMLangStringWStr> pMLStrW(this);
        ASSERT(pMLStrW);
        hr = pMLStrW->SetWStr(GetLockInfo()->GetPos(pKey), GetLockInfo()->GetLen(pKey), (WCHAR*)pszSrc, cchSrc, pcchActual, plActualLen);
    }

    ASSIGN_IF_FAILED(hr, MemFree((void*)pszSrc));

    return hr;
}

HRESULT CMLStr::UnlockAStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr;
    IMLangStringBufA* const pMLStrBufA = GetMLStrBufA();
    const long cchLockLen = GetLockInfo()->GetCChLen(pKey);

    if (SUCCEEDED(hr = pMLStrBufA->UnlockBuf((CHAR*)pszSrc, 0, cchSrc)) &&
        (GetLockInfo()->GetFlags(pKey) & MLSTR_WRITE))
    {
        if (cchSrc < cchLockLen)
        {
            if (SUCCEEDED(hr = pMLStrBufA->Delete(GetLockInfo()->GetCChPos(pKey) + cchSrc, cchLockLen - cchSrc)))
                SetBufCCh(GetBufCCh() - (cchLockLen - cchSrc));
        }

        if (SUCCEEDED(hr) && plActualLen)
            hr = CalcLenA(GetCodePage(), (CHAR*)pszSrc, cchSrc, plActualLen);

        if (pcchActual)
            *pcchActual = cchSrc;
    }

    return hr;
}

HRESULT CMLStr::UnlockAStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr = S_OK;

    if (GetLockInfo()->GetFlags(pKey) & MLSTR_WRITE)
    {
        CComQIPtr<IMLangStringAStr, &IID_IMLangStringAStr> pMLStrA(this);
        ASSERT(pMLStrA);
        hr = pMLStrA->SetAStr(GetLockInfo()->GetPos(pKey), GetLockInfo()->GetLen(pKey), GetLockInfo()->GetCodePage(pKey), (CHAR*)pszSrc, cchSrc, pcchActual, plActualLen);
    }

    ASSIGN_IF_FAILED(hr, MemFree((void*)pszSrc));

    return hr;
}
#endif

#ifndef ASTRIMPL
STDMETHODIMP CMLStr::UnlockWStr(const WCHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr = CheckThread();
    long lSrcLen;
    const long lLockFlags = GetLockFlags();

    if (SUCCEEDED(hr) && (!IsLocked() || pszSrc != m_pszLockBuf))
        hr = E_INVALIDARG; // This MLStr is not locked

    if (!(lLockFlags & MLSTR_WRITE))
    {
        cchSrc = 0;
        lSrcLen = 0;
    }

    if (SUCCEEDED(hr))
    {
        IMLangStringBufW* const pMLStrBufW = GetMLStrBufW();

        if (IsDirectLock())
        {
            if (SUCCEEDED(hr = pMLStrBufW->UnlockBuf(pszSrc, 0, cchSrc)) &&
                (lLockFlags & MLSTR_WRITE))
            {
                if (cchSrc < m_cchLockLen)
                {
                    if (SUCCEEDED(hr = pMLStrBufW->Delete(m_cchLockPos + cchSrc, m_cchLockLen - cchSrc)))
                        SetBufCCh(GetBufCCh() - (m_cchLockLen - cchSrc));
                }

                if (SUCCEEDED(hr) && plActualLen)
                    hr = CalcLenW(pszSrc, cchSrc, &lSrcLen);
            }
        }
        else
        {
            if (lLockFlags & MLSTR_WRITE)
                hr = SetWStr(m_lLockPos, m_lLockLen, pszSrc, cchSrc, (pcchActual) ? &cchSrc : NULL, (plActualLen) ? &lSrcLen : NULL);

            HRESULT hrTemp = MemFree((void*)pszSrc);
            if (FAILED(hrTemp) && SUCCEEDED(hr))
                hr = hrTemp;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (pcchActual)
            *pcchActual = cchSrc;
        if (plActualLen)
            *plActualLen = lSrcLen;
    }
    else
    {
        if (pcchActual)
            *pcchActual = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    SetLockFlags(0); // Unlock it anyway

    return hr;
}
#endif

#ifdef ASTRIMPL
HRESULT CMLStr::UnlockStrCommon(const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr = CheckThread();
    void* pLockKey;
    long lSrcLen;

    if (SUCCEEDED(hr))
        hr = GetLockInfo()->Find(pszSrc, cchSrc, &pLockKey);

    if (SUCCEEDED(hr))
        hr = GetLockInfo()->Unlock(pLockKey, pszSrc, cchSrc, (pcchActual) ? &cchSrc : NULL, (plActualLen) ? &lSrcLen : NULL);

    if (SUCCEEDED(hr))
    {
        if (pcchActual)
            *pcchActual = cchSrc;
        if (plActualLen)
            *plActualLen = lSrcLen;
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
#endif

STDMETHODIMP CMLStr::SetLocale(long lDestPos, long lDestLen, LCID locale)
{
    ASSERT_THIS;

    HRESULT hr = CheckThread();
#ifdef ASTRIMPL
    CLock Lock(TRUE, this, hr);
#endif
    long cchDestPos;
    long cchDestLen;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = RegularizePosLen(&lDestPos, &lDestLen)) &&
        SUCCEEDED(hr = GetCCh(0, lDestPos, &cchDestPos)) &&
        SUCCEEDED(hr = GetCCh(cchDestPos, lDestLen, &cchDestLen)))
    {
        //if (!cchDestPos && cchDestLen == GetBufCCh())
            SetLocale(locale);
        //else
        //    hr = E_NOTIMPL; // Cannot set the locale to a part of string in this version.
    }

    return hr;
}

STDMETHODIMP CMLStr::GetLocale(long lSrcPos, long lSrcMaxLen, LCID* plocale, long* plLocalePos, long* plLocaleLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(plocale);
    ASSERT_WRITE_PTR_OR_NULL(plLocalePos);
    ASSERT_WRITE_PTR_OR_NULL(plLocaleLen);

    HRESULT hr = CheckThread();
#ifdef ASTRIMPL
    CLock Lock(FALSE, this, hr);
#endif
    long lStrLen;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = GetLen(0, GetBufCCh(), &lStrLen)) &&
        SUCCEEDED(hr = ::RegularizePosLen(lStrLen, &lSrcPos, &lSrcMaxLen)))
    {
        if (plocale)
            *plocale = GetLocale();
        if (plLocalePos)
            *plLocalePos = 0;
        if (plLocaleLen)
        {
            if (plLocalePos)
                *plLocaleLen = lStrLen;
            else
                *plLocaleLen = lSrcMaxLen;
        }
    }
    else
    {
        if (plocale)
            *plocale = 0;
        if (plLocalePos)
            *plLocalePos = 0;
        if (plLocaleLen)
            *plLocaleLen = 0;
    }

    return hr;
}

HRESULT CMLStr::PrepareMLStrBuf(void)
{
    if (GetMLStrBufW() || GetMLStrBufA())
        return S_OK;
#ifdef ASTRIMPL

    IMLangStringBufW* pBuf = new CMLStr::CMLStrBufStandardW;
    if (pBuf)
    {
        SetMLStrBufW(pBuf);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
#else
    else
        return E_NOTIMPL; //!ASTRIMPL
#endif
}

HRESULT CMLStr::RegularizePosLen(long* plPos, long* plLen)
{
    HRESULT hr;
    long lStrLen;

    if (SUCCEEDED(hr = GetLen(0, GetBufCCh(), &lStrLen)))
        hr = ::RegularizePosLen(lStrLen, plPos, plLen);

    return hr;
}

HRESULT CMLStr::GetCCh(long cchOffset, long lLen, long* pcchLen)
{
    if (GetMLStrBufW())
    {
        if (pcchLen)
            *pcchLen = lLen; // The number of characters is equal to the length
        return S_OK;
    }
    else if (GetMLStrBufA())
    {
        HRESULT hr = S_OK;
#ifdef ASTRIMPL
        CMLStrBufWalkA BufWalk(GetMLStrBufA(), cchOffset, GetBufCCh() - cchOffset);

        while (lLen > 0 && BufWalk.Lock(hr))
        {
            for (LPCSTR pszTemp = BufWalk.GetStr(); lLen > 0 && *pszTemp; lLen--)
                pszTemp = ::CharNextExA((WORD)GetCodePage(), pszTemp, 0);

            if (!*pszTemp)
                lLen = 0; // String terminated

            BufWalk.Unlock(hr);
        }
#else
        long cchDone = 0;
        long cchRest = GetBufCCh() - cchOffset;

        while (SUCCEEDED(hr) && lLen > 0)
        {
            CHAR* pszBuf;
            long cchBuf;

            if (SUCCEEDED(hr = m_pMLStrBufA->LockBuf(cchOffset, cchRest, &pszBuf, &cchBuf)))
            {
                for (LPCSTR pszTemp = pszBuf; lLen > 0 && *pszTemp; lLen--)
                    pszTemp = ::CharNextExA((WORD)m_uCodePage, pszTemp, 0);

                if (!*pszBuf)
                    lLen = 0; // String terminated

                hr = m_pMLStrBufA->UnlockBuf(pszBuf, 0, 0);

                cchOffset += cchBuf;
                cchRest -= cchBuf;
                cchDone += (int)(pszTemp - pszBuf);
            }
        }
#endif

        if (pcchLen)
        {
            if (SUCCEEDED(hr))
#ifdef ASTRIMPL
                *pcchLen = BufWalk.GetDoneCCh();
#else
                *pcchLen = cchDone;
#endif
            else
                *pcchLen = 0;
        }

        return hr;
    }
    else
    {
        if (pcchLen)
            *pcchLen = 0; // No string
        return S_OK;
    }
}

HRESULT CMLStr::GetLen(long cchOffset, long cchLen, long* plLen)
{
    if (GetMLStrBufW())
    {
        if (plLen)
            *plLen = cchLen; // The length is equal to the number of characters
        return S_OK;
    }
    else if (GetMLStrBufA())
    {
        HRESULT hr = S_OK;
        long lDoneLen = 0;
#ifdef ASTRIMPL
        CMLStrBufWalkA BufWalk(GetMLStrBufA(), cchOffset, cchLen);

        while (BufWalk.Lock(hr))
        {
            long lTempLen;

            hr = CalcLenA(GetCodePage(), BufWalk.GetStr(), BufWalk.GetCCh(), &lTempLen);
            if (hr == S_FALSE)
                cchLen = 0; // String terminated
            lDoneLen += lTempLen;

            BufWalk.Unlock(hr);
        }
#else

        while (SUCCEEDED(hr) && cchLen > 0)
        {
            CHAR* pszBuf;
            long cchBuf;

            if (SUCCEEDED(hr = m_pMLStrBufA->LockBuf(cchOffset, cchLen, &pszBuf, &cchBuf)))
            {
                long lTempLen;

                hr = CalcLenA(GetCodePage(), pszBuf, cchBuf, &lTempLen);
                if (hr == S_FALSE)
                    cchLen = 0; // String terminated
                lDoneLen += lTempLen;

                hr = m_pMLStrBufA->UnlockBuf(pszBuf, 0, 0);

                cchOffset += cchBuf;
                cchLen -= cchBuf;
            }
        }
#endif

        if (plLen)
        {
            if (SUCCEEDED(hr))
                *plLen = lDoneLen;
            else
                *plLen = 0;
        }

        return hr;
    }
    else
    {
        if (plLen)
            *plLen = 0; // No string
        return S_OK;
    }
}

HRESULT CMLStr::CalcLenA(UINT uCodePage, const CHAR* psz, long cchLen, long* plLen)
{
    long lLen = 0;
    const CHAR* const pszEnd = psz + cchLen;

    for (; psz < pszEnd && *psz; lLen++)
    {
        const CHAR* const pszNew = ::CharNextExA((WORD)uCodePage, psz, 0);

        if (pszNew > pszEnd) // Overrun out of buffer
            break;

        psz = pszNew;
    }

    if (plLen)
        *plLen = lLen;

    if (*psz)
        return S_OK;
    else
        return S_FALSE;
}

#ifdef ASTRIMPL
HRESULT CMLStr::CalcCChA(UINT uCodePage, const CHAR* psz, long lLen, long* pcchLen)
{
    const CHAR* const pszStart = psz;

    for (; lLen > 0 && *psz; lLen--)
        psz = ::CharNextExA((WORD)uCodePage, psz, 0);

    if (pcchLen)
        *pcchLen = psz - pszStart;

    if (*psz)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT CMLStr::ConvAStrToWStr(UINT uCodePage, const CHAR* pszSrc, long cchSrc, WCHAR* pszDest, long cchDest, long* pcchActualA, long* pcchActualW, long* plActualLen)
{
    HRESULT hr = S_OK;
    long lWrittenLen;
    long cchWrittenA;

    long cchWrittenW = ::MultiByteToWideChar(uCodePage, 0, pszSrc, cchSrc, pszDest, (pszDest) ? cchDest : 0);
    if (!cchWrittenW)
        hr = E_FAIL; // NLS failed

    if ((pcchActualA || plActualLen) && SUCCEEDED(hr))
        hr = CalcLenW(pszDest, cchWrittenW, &lWrittenLen); // BOGUS: pszDest may be NULL

    if (pcchActualA && SUCCEEDED(hr))
        hr = CalcCChA(uCodePage, pszSrc, lWrittenLen, &cchWrittenA);

    if (SUCCEEDED(hr))
    {
        if (pcchActualA)
            *pcchActualA = cchWrittenA;
        if (pcchActualW)
            *pcchActualW = cchWrittenW;
        if (plActualLen)
            *plActualLen = lWrittenLen;
    }
    else
    {
        if (pcchActualA)
            *pcchActualA = 0;
        if (pcchActualW)
            *pcchActualW = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    return hr;
}

HRESULT CMLStr::ConvWStrToAStr(BOOL fCanStopAtMiddle, UINT uCodePage, const WCHAR* pszSrc, long cchSrc, CHAR* pszDest, long cchDest, long* pcchActualA, long* pcchActualW, long* plActualLen)
{
    HRESULT hr = S_OK;
    long lWrittenLen;
    long cchWrittenW;

    long cchWrittenA = ::WideCharToMultiByte(uCodePage, (fCanStopAtMiddle) ? 0 : WC_DEFAULTCHAR, pszSrc, cchSrc, pszDest, (pszDest) ? cchDest : 0, NULL, NULL);
    if (!cchWrittenA)
        hr = E_FAIL; // NLS failed

    if ((pcchActualW || plActualLen) && SUCCEEDED(hr))
    {
        if (pszDest)
            hr = CalcLenA(uCodePage, pszDest, cchWrittenA, &lWrittenLen);
        else
            hr = E_NOTIMPL; // Can't retrieve pcchActualW and plActualLen
    }

    if (pcchActualW && SUCCEEDED(hr))
        hr = CalcCChW(pszSrc, lWrittenLen, &cchWrittenW);

    if (SUCCEEDED(hr))
    {
        if (pcchActualA)
            *pcchActualA = cchWrittenA;
        if (pcchActualW)
            *pcchActualW = cchWrittenW;
        if (plActualLen)
            *plActualLen = lWrittenLen;
    }
    else
    {
        if (pcchActualA)
            *pcchActualA = 0;
        if (pcchActualW)
            *pcchActualW = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    return hr;
}
#endif

#ifndef ASTRIMPL
HRESULT CMLStr::ConvertMLStrBufAToWStr(UINT uCodePage, IMLangStringBufA* pMLStrBufA, long cchSrcPos, long cchSrcLen, WCHAR* pszBuf, long cchBuf, long* pcchActual)
{
    HRESULT hr = S_OK;
    long cchDone = 0;

    while (SUCCEEDED(hr) && cchSrcLen > 0)
    {
        CHAR* pszBufA;
        long cchBufA;

        if (SUCCEEDED(hr = pMLStrBufA->LockBuf(cchSrcPos, cchSrcLen, &pszBufA, &cchBufA)))
        {
            long cchWritten = ::MultiByteToWideChar(uCodePage, 0, pszBufA, cchBufA, pszBuf, cchBuf);
            if (!cchWritten)
                hr = E_FAIL; // NLS failed

            HRESULT hrTemp = pMLStrBufA->UnlockBuf(pszBufA, 0, 0);
            if (FAILED(hrTemp) && SUCCEEDED(hr))
                hr = hrTemp;

            cchSrcPos += cchBufA;
            cchSrcLen -= cchBufA;
            pszBuf += cchWritten;
            cchBuf -= cchWritten;
            cchDone += cchWritten;
            ASSERT(cchBuf >= 0);
        }
    }

    if (pcchActual)
    {
        *pcchActual = cchDone;

        if (FAILED(hr) && cchDone > 0)
            hr = S_OK;
    }

    return hr;
}

HRESULT CMLStr::ConvertWStrToMLStrBufA(const WCHAR*, long, UINT, IMLangStringBufA*, long, long)
{
    return E_NOTIMPL; // !ASTRIMPL
}
#endif

#ifdef ASTRIMPL
/////////////////////////////////////////////////////////////////////////////
// CMLStr::CLockInfo

HRESULT CMLStr::CLockInfo::UnlockAll(void)
{
    if (m_pLockArray)
    {
        for (int n = 0; n < MAX_LOCK_COUNT; n++)
        {
            if (m_pLockArray[n].m_psz)
                Unlock(&m_pLockArray[n], m_pLockArray[n].m_psz, m_pLockArray[n].m_cchLen, NULL, NULL);
        }
    }

    return S_OK;
}

HRESULT CMLStr::CLockInfo::Lock(PFNUNLOCKPROC pfnUnlockProc, long lFlags, UINT uCodePage, void* psz, long lPos, long lLen, long cchPos, long cchLen)
{
    HRESULT hr = S_OK;
    int nIndex;

    if (!m_pLockArray)
    {
        m_pLockArray = new CLockInfoEntry[MAX_LOCK_COUNT];

        if (m_pLockArray)
        {
            for (nIndex = 0; nIndex < MAX_LOCK_COUNT; nIndex++)
                m_pLockArray[nIndex].m_psz = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        for (nIndex = 0; nIndex < MAX_LOCK_COUNT; nIndex++)
        {
            if (!m_pLockArray[nIndex].m_psz)
                break;
        }
        if (nIndex >= MAX_LOCK_COUNT)
            hr = MLSTR_E_TOOMANYNESTOFLOCK;
    }

    if (SUCCEEDED(hr))
    {
        m_pLockArray[nIndex].m_psz = psz;
        m_pLockArray[nIndex].m_pfnUnlockProc = pfnUnlockProc;
        m_pLockArray[nIndex].m_lFlags = lFlags;
        m_pLockArray[nIndex].m_uCodePage = uCodePage;
        m_pLockArray[nIndex].m_lPos = lPos;
        m_pLockArray[nIndex].m_lLen = lLen;
        m_pLockArray[nIndex].m_cchPos = cchPos;
        m_pLockArray[nIndex].m_cchLen = cchLen;
    }

    return hr;
}

HRESULT CMLStr::CLockInfo::Find(const void* psz, long, void** ppKey)
{
    HRESULT hr = S_OK;
    int nIndex;

    if (m_pLockArray)
    {
        for (nIndex = 0; nIndex < MAX_LOCK_COUNT; nIndex++)
        {
            if (psz == m_pLockArray[nIndex].m_psz)
                break;
        }
    }
    if (!m_pLockArray || nIndex >= MAX_LOCK_COUNT)
        hr = E_INVALIDARG;

    if (ppKey)
    {
        if (SUCCEEDED(hr))
            *ppKey = &m_pLockArray[nIndex];
        else
            *ppKey = NULL;
    }

    return hr;
}

HRESULT CMLStr::CLockInfo::Unlock(void* pKey, const void* psz, long cch, long* pcchActual, long* plActualLen)
{
    CLockInfoEntry* const pEntry = (CLockInfoEntry*)pKey;
    HRESULT hr;

    if (!(pEntry->m_lFlags & MLSTR_WRITE))
    {
        cch = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    hr = (m_pMLStr->*(pEntry->m_pfnUnlockProc))(pKey, psz, cch, pcchActual, plActualLen);

    if (SUCCEEDED(hr))
        hr = EndLock(pEntry->m_lFlags & MLSTR_WRITE);

    pEntry->m_psz = NULL; // Remove from lock array anyway

    if (FAILED(hr))
    {
        if (pcchActual)
            *pcchActual = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CMLStr::CMLStrBufStandardW

long CMLStr::CMLStrBufStandardW::RoundBufSize(long cchStr)
{
    for (int n = 8; n < 12; n++)
    {
        if (cchStr < (1L << n))
            break;
    }
    const long cchTick = (1L << (n - 4));
    return (cchStr + cchTick - 1) / cchTick * cchTick;
}

#endif

#else // NEWMLSTR

#include "mlstr.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStr

CMLStr::CMLStr(void) :
    m_lLen(0),
    m_hUnlockEvent(NULL),
    m_hZeroEvent(NULL)
{
}

CMLStr::~CMLStr(void)
{
    void* pv;

    if (m_hZeroEvent)
        ::CloseHandle(m_hZeroEvent);
    if (m_hUnlockEvent)
        ::CloseHandle(m_hUnlockEvent);

    // m_lock should be empty
    ASSERT(SUCCEEDED(m_lock.Top(&pv)));
    ASSERT(!pv);

    // Release all attributes in m_attr
    VERIFY(SUCCEEDED(m_attr.Top(&pv)));
    while (pv)
    {
        IMLStrAttr* const pAttr = m_attr.GetAttr(pv);
        ASSERT(pAttr);
        VERIFY(SUCCEEDED(pAttr->SetClient(NULL))); // Reset
        VERIFY(SUCCEEDED(StartEndConnectionAttr(pAttr, NULL, m_attr.GetCookie(pv)))); // Disconnect
        pAttr->Release();
        VERIFY(SUCCEEDED(m_attr.Next(pv, &pv)));
    }
}

STDMETHODIMP CMLStr::LockMLStr(long lPos, long lLen, DWORD dwFlags, DWORD* pdwCookie, long* plActualPos, long* plActualLen)
{
    ASSERT_WRITE_PTR_OR_NULL(pdwCookie);
    ASSERT_WRITE_PTR_OR_NULL(plActualPos);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr;
    void* pv;

    Lock();

    if (SUCCEEDED(hr = ::RegularizePosLen(m_lLen, &lPos, &lLen)))
    {
        const DWORD dwThrd = ::GetCurrentThreadId();

        if (SUCCEEDED(hr = CheckAccessValidation(lPos, lLen, dwFlags, dwThrd, plActualPos, plActualLen)) &&
            SUCCEEDED(hr = m_lock.Add(&pv)))
        {
            if (plActualPos && !plActualLen)
                lLen -= *plActualPos - lPos;
            else if (plActualLen)
                lLen = *plActualLen;
            if (plActualPos)
                lPos = *plActualPos;

            hr = m_lock.SetLock(pv, lPos, lLen, dwFlags, dwThrd);

            if (FAILED(hr))
                VERIFY(SUCCEEDED(m_lock.Remove(pv)));
        }
    }
    else
    {
        if (plActualPos)
            *plActualPos = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    Unlock();

    if (pdwCookie)
    {
        if (SUCCEEDED(hr))
            *pdwCookie = (DWORD)pv;
        else
            *pdwCookie = 0;
    }

    return hr;
}

HRESULT CMLStr::CheckAccessValidation(long lPos, long lLen, DWORD dwFlags, DWORD dwThrd, long* plActualPos, long* plActualLen)
{
    HRESULT hr;
    DWORD dwStartTime = 0;
    long lActualPos;
    long lActualLen;

    for (;;) // Waiting unlock loop
    {
        void* pv;
        HRESULT hrValidation = S_OK;

        lActualPos = lPos;
        lActualLen = lLen;

        hr = m_lock.Top(&pv);
        while (SUCCEEDED(hr) && pv) // Enumerate all locks
        {
            LOCKINFO* plinfo;

            if (SUCCEEDED(hr = m_lock.GetLockInfo(pv, &plinfo))) // Retrieve info of a lock
            {
                if ((dwFlags & MLSTR_MOVE) && // Moving this lock
                    lPos < plinfo->lPos + plinfo->lLen && // Overwrap or left of this lock
                    (dwThrd != plinfo->dwThrd || // Another thread
                     (plinfo->dwFlags & (MLSTR_READ | MLSTR_WRITE)))) // Same thread and has read or write access
                {
                    if (dwThrd == plinfo->dwThrd)
                        hr = MLSTR_E_ACCESSDENIED;
                    else
                        hr = MLSTR_E_BUSY;
                }

                if (SUCCEEDED(hr) &&
                    lActualPos < plinfo->lPos + plinfo->lLen &&
                    lActualPos + lActualLen >= plinfo->lPos) // Overwraping with this lock
                {
                    DWORD dwShareMask = 0;
                    if (dwThrd == plinfo->dwThrd) // Same thread
                        dwShareMask = ~(MLSTR_SHARE_DENYREAD | MLSTR_SHARE_DENYWRITE); // Ignore share flags

                    if (((dwFlags & MLSTR_WRITE) && (plinfo->dwFlags & (MLSTR_READ | MLSTR_WRITE | MLSTR_SHARE_DENYWRITE) & dwShareMask)) || // Write on read/write
                        ((dwFlags & MLSTR_READ)  && (plinfo->dwFlags & (             MLSTR_WRITE | MLSTR_SHARE_DENYREAD ) & dwShareMask)) || // Read on write
                        ((dwFlags & MLSTR_SHARE_DENYWRITE & dwShareMask) && (plinfo->dwFlags & MLSTR_WRITE)) || // Share deny on write
                        ((dwFlags & MLSTR_SHARE_DENYREAD  & dwShareMask) && (plinfo->dwFlags & MLSTR_READ)))    // Share deny on read
                    {
                        // Conflicting access
                        if ((plinfo->lPos <= lActualPos && plinfo->lPos + plinfo->lLen >= lActualPos + lActualLen) || // No valid range left
                            (!plActualPos && !plActualLen)) // Needs to lock entire range
                        {
                            lActualPos = 0;
                            lActualLen = 0;
                            if (dwThrd == plinfo->dwThrd)
                                hr = MLSTR_E_ACCESSDENIED;
                            else
                                hr = MLSTR_E_BUSY;
                        }
                        else if ((!plActualPos && plinfo->lPos <= lActualPos) || // Forward processing, Starting from invalid range
                                 (!plActualLen && plinfo->lPos + plinfo->lLen < lActualPos + lActualLen) || // Backward processing, Trancate valid range
                                 (plActualPos && plActualLen && plinfo->lPos - lActualPos >= (lActualPos + lActualLen) - (plinfo->lPos + plinfo->lLen))) // Maximum valid range, Right valid range is bigger
                        {
                            lActualLen += lActualPos;
                            lActualPos = plinfo->lPos + plinfo->lLen;
                            lActualLen -= lActualPos;
                            if (!plActualPos) // Forward processing
                            {
                                if (dwThrd == plinfo->dwThrd)
                                    hrValidation = MLSTR_E_ACCESSDENIED;
                                else
                                    hrValidation = MLSTR_E_BUSY;
                            }
                        }
                        else
                        {
                            lActualLen = plinfo->lPos - lActualPos;
                            if (!plActualLen) // Backward processing
                            {
                                if (dwThrd == plinfo->dwThrd)
                                    hrValidation = MLSTR_E_ACCESSDENIED;
                                else
                                    hrValidation = MLSTR_E_BUSY;
                            }
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
                hr = m_lock.Next(pv, &pv);
        }

        if (SUCCEEDED(hr) && FAILED(hrValidation))
        {
            hr = hrValidation;
            if (plActualLen && lPos < lActualPos) // Forward processing
            {
                lActualLen = lActualPos - lPos;
                lActualPos = lPos;
            }
            else if (plActualPos && lPos + lLen != lActualPos + lActualLen) // Backward processing
            {
                lActualPos += lActualLen;
                lActualLen = lPos + lLen - lActualPos;
            }
        }

        if (hr != MLSTR_E_BUSY || (dwFlags | MLSTR_NOWAIT)) // No busy state, or don't want to wait even if busy
            break;

        // Now, let's wait another thread run UnlockMLStr. Then, try validation again.

        if (!dwStartTime) // Not initialized yet
            dwStartTime = ::GetTickCount(); // Remember starting time

        const DWORD dwElapsedTime = ::GetTickCount() - dwStartTime;
        if (dwElapsedTime >= MLSTR_LOCK_TIMELIMIT) // Already elapsed long time
            break;

        if (!m_hUnlockEvent) // We don't have event object yet
        {
            m_hUnlockEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL); // Manual reset, initial reset
            if (!m_hUnlockEvent)
                break;

            m_cWaitUnlock = -1; // Initialize
        }
        else // After second time
        {
            ASSERT(m_cWaitUnlock == 0 || m_cWaitUnlock == -1 || m_cWaitUnlock >= 1);
            if (m_cWaitUnlock == 0) // Don't reset if m_cWaitUnlock is not zero
            {
                ::ResetEvent(m_hUnlockEvent);
                m_cWaitUnlock = -1;
            }
            else
            {
                if (!m_hZeroEvent)
                {
                    m_hZeroEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL); // Auto-reset, initial reset
                    if (!m_hZeroEvent)
                        break;
                }
                if (m_cWaitUnlock == -1)
                    m_cWaitUnlock = 2;
                else
                    m_cWaitUnlock++;
            }
        }
        ASSERT(m_cWaitUnlock == -1 || m_cWaitUnlock >= 2);

        // CAUTION: Don't leave here until we make sure m_cWaitUnlock gets zero.

        Unlock();

        // === The story of m_cWaitUnlock ===
        // If we don't have m_cWaitUnlock, the following scenario can be considered.
        // (1) Thread A: ResetEvent(m_hUnlockEvent)
        // (2) Thread A: Unlock()
        // (3) Thread B: SetEvent(m_hUnlockEvent) // UnlockMLStr!!!
        // (4) Thread C: Lock()
        // (5) Thread C: ResetEvent(m_hUnlockEvent) // Problem!!!
        // (6) Thread C: Unlock()
        // (7) Thread A: WaitForSingleObject(m_hUnlockEvent)
        // In this scenario, thread A is missing a event of (3). This situation should not happen.
        // m_cWaitUnlock solves the problem.

        const DWORD dwWaitResult = ::WaitForSingleObject(m_hUnlockEvent, MLSTR_LOCK_TIMELIMIT - dwElapsedTime); // Now wait unlock

        Lock();

        ASSERT(m_cWaitUnlock == -1 || m_cWaitUnlock >= 1);
        if (m_cWaitUnlock == -1)
        {
            m_cWaitUnlock = 0;
        }
        else // m_cWaitUnlock >= 1
        {
            m_cWaitUnlock--;

            // Here, let's wait until m_cWaitUnlock gets zero.
            // Unless this, it may not good for performance.
            // In worst case, it makes thousands of loops in this function because it never reset m_hUnlockEvent.
            // m_hUnlockEvent will be signaled even though UnlockMLStr is called yet.
            if (m_cWaitUnlock > 0)
            {
                Unlock();
                ::WaitForSingleObject(m_hZeroEvent, INFINITE); // Wait until m_cWaitUnlock gets zero, auto-reset
                Lock();
            }
            else // Now it's zero! Yeah!
            {
                ::SetEvent(m_hZeroEvent); // Release other threads
            }
        }
        // ASSERT(m_cWaitUnlock == 0); This is not true. Maybe non-zero for next time.
        // Now we may leave here.

        if (dwWaitResult != WAIT_OBJECT_0) // Time expired or an error occurred
            break;
    }

    if (plActualPos)
        *plActualPos = lActualPos;
    if (plActualLen)
        *plActualLen = lActualLen;

    return hr;
}

STDMETHODIMP CMLStr::UnlockMLStr(DWORD dwCookie)
{
    Lock();

    void* const pv = (void*)dwCookie;

    const HRESULT hr = m_lock.Remove(pv);

    if (m_hUnlockEvent)
        ::SetEvent(m_hUnlockEvent);

    Unlock();

    return hr;
}

STDMETHODIMP CMLStr::GetLength(long* plLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(plLen);

    if (plLen)
        *plLen = m_lLen;

    return S_OK;
}

STDMETHODIMP CMLStr::SetMLStr(long, long, IUnknown*, long, long)
{
    return E_NOTIMPL; // IMLangString::SetMLStr()
}

STDMETHODIMP CMLStr::RegisterAttr(IUnknown* pUnk, DWORD* pdwCookie)
{
    ASSERT_THIS;
    ASSERT_READ_PTR(pUnk);
    ASSERT_WRITE_PTR_OR_NULL(pdwCookie);

    HRESULT hr;
    void* pv;
    IMLStrAttr* pAttr = NULL;
    BOOL fConnStarted = FALSE;
    DWORD dwConnCookie;

    Lock();

    if (SUCCEEDED(hr = m_attr.Add(&pv)) &&
        SUCCEEDED(hr = pUnk->QueryInterface(IID_IMLStrAttr, (void**)&pAttr)))
    {
        ASSERT_READ_PTR(pAttr);
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = StartEndConnectionAttr(pAttr, &dwConnCookie, 0))) // Connect
    {
        fConnStarted = TRUE;
        if (SUCCEEDED(hr = pAttr->SetClient((IMLangString*)this)))
        {
            CFire fire(hr, this);
            while (fire.Next())
                hr = fire.Sink()->OnRegisterAttr(pAttr);
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pAttr->SetMLStr(0, -1, (IMLangString*)this, 0, m_lLen)))
    {
        m_attr.SetAttr(pv, pAttr);
        m_attr.SetCookie(pv, dwConnCookie);

        if (pdwCookie)
            *pdwCookie = (DWORD)pv;
    }
    else
    {
        if (pAttr)
        {
            pAttr->SetClient(NULL);
            if (fConnStarted)
                VERIFY(SUCCEEDED(StartEndConnectionAttr(pAttr, NULL, dwConnCookie))); // Disconnect
            pAttr->Release();
        }

        if (pv)
            m_attr.Remove(pv);

        if (pdwCookie)
            *pdwCookie = NULL;
    }

    Unlock();

    return hr;
}

STDMETHODIMP CMLStr::UnregisterAttr(DWORD dwCookie)
{
    ASSERT_THIS;

    void* const pv = (void*)dwCookie;

    Lock();

    IMLStrAttr* const pAttr = m_attr.GetAttr(pv);
    ASSERT(pAttr);

    // Fire OnUnregisterAttr
    HRESULT hr;
    CFire fire(hr, this);
    while (fire.Next())
        hr = fire.Sink()->OnUnregisterAttr(pAttr);

    // Release attribute
    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = pAttr->SetClient(NULL))) // Reset
    {
        VERIFY(SUCCEEDED(hr = StartEndConnectionAttr(pAttr, NULL, m_attr.GetCookie(pv)))); // Disconnect
        pAttr->Release();

        // Remove entry from attr table
        m_attr.Remove(pv);
    }

    Unlock();

    return hr;
}

STDMETHODIMP CMLStr::EnumAttr(IEnumUnknown** ppEnumUnk)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppEnumUnk);

    if (!ppEnumUnk)
        return S_OK;

    CEnumAttr* const pEnum = new CComObject<CEnumAttr>;

    *ppEnumUnk = pEnum;

    if (pEnum)
    {
        pEnum->Init(this);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CMLStr::FindAttr(REFIID riid, LPARAM lParam, IUnknown** ppUnk)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppUnk);

    HRESULT hr;
    void* pv;
    IUnknown* pMaxUnk = NULL;
    long lMaxConf = 0;

    Lock();

    for (hr = m_attr.Top(&pv); SUCCEEDED(hr) && pv; hr = m_attr.Next(pv, &pv))
    {
        IMLStrAttr* const pIMLStrAttr = m_attr.GetAttr(pv);
        IUnknown* pUnk;
        long lConf;

        hr = pIMLStrAttr->QueryAttr(riid, lParam, &pUnk, &lConf);
        if (SUCCEEDED(hr))
        {
            if (lConf > lMaxConf)
            {
                lMaxConf = lConf;
                if (pMaxUnk)
                    pMaxUnk->Release();
                pMaxUnk = pUnk;
            }
            else
            {
                if (pUnk)
                    pUnk->Release();
            }

            if (lMaxConf == MLSTR_CONF_MAX)
                break;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (ppUnk)
            *ppUnk = pMaxUnk;
        else if (pMaxUnk)
            pMaxUnk->Release();
    }
    else
    {
        if (pMaxUnk)
            pMaxUnk->Release();
        if (ppUnk)
            *ppUnk = NULL;
    }

    Unlock();

    return hr;
}

STDMETHODIMP CMLStr::OnRequestEdit(long lDestPos, long lDestLen, long lNewLen, REFIID riid, LPARAM lParam, IUnknown* pUnk)
{
    HRESULT hr;
    CFire fire(hr, this);
    while (fire.Next())
        hr = fire.Sink()->OnRequestEdit(lDestPos, lDestLen, lNewLen, riid, lParam, pUnk);
    return hr;
}

STDMETHODIMP CMLStr::OnCanceledEdit(long lDestPos, long lDestLen, long lNewLen, REFIID riid, LPARAM lParam, IUnknown* pUnk)
{
    HRESULT hr;
    CFire fire(hr, this);
    while (fire.Next())
        hr = fire.Sink()->OnCanceledEdit(lDestPos, lDestLen, lNewLen, riid, lParam, pUnk);
    return hr;
}

STDMETHODIMP CMLStr::OnChanged(long lDestPos, long lDestLen, long lNewLen, REFIID riid, LPARAM lParam, IUnknown* pUnk)
{
    HRESULT hr;
    CFire fire(hr, this);
    while (fire.Next())
        hr = fire.Sink()->OnChanged(lDestPos, lDestLen, lNewLen, riid, lParam, pUnk);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CMLStr::CEnumAttr

CMLStr::CEnumAttr::CEnumAttr(void) :
    m_pMLStr(NULL),
    m_pv(NULL)
{
}

CMLStr::CEnumAttr::~CEnumAttr(void)
{
    if (m_pMLStr)
        m_pMLStr->Unlock();
}

void CMLStr::CEnumAttr::Init(CMLStr* pMLStr)
{
    ASSERT_THIS;
    ASSERT_READ_PTR(pMLStr);

    if (m_pMLStr)
        m_pMLStr->Unlock();

    m_pMLStr = pMLStr;
    m_pMLStr->Lock();

    VERIFY(SUCCEEDED(Reset()));
}

HRESULT CMLStr::CEnumAttr::Next(ULONG celt, IUnknown** rgelt, ULONG* pceltFetched)
{
    ASSERT_THIS;
    ASSERT_WRITE_BLOCK_OR_NULL(rgelt, celt);
    ASSERT_WRITE_PTR_OR_NULL(pceltFetched);

    ULONG c = 0;

    if (rgelt && m_pMLStr)
    {
        for (; m_pv && c < celt; c++)
        {
            *rgelt = m_pMLStr->m_attr.GetAttr(m_pv);
            ASSERT(*rgelt);
            (*rgelt)->AddRef();

            VERIFY(SUCCEEDED(m_pMLStr->m_attr.Next(m_pv, &m_pv)));
            rgelt++;
        }
    }

    if (pceltFetched)
        *pceltFetched = c;

    return S_OK;
}

HRESULT CMLStr::CEnumAttr::Skip(ULONG celt)
{
    ASSERT_THIS;

    for (ULONG c = 0; m_pv && c < celt; c++)
        VERIFY(SUCCEEDED(m_pMLStr->m_attr.Next(m_pv, &m_pv)));

    return S_OK;
}

HRESULT CMLStr::CEnumAttr::Reset(void)
{
    ASSERT_THIS;
    ASSERT_READ_PTR(m_pMLStr);

    VERIFY(SUCCEEDED(m_pMLStr->m_attr.Top(&m_pv)));
    return S_OK;
}

HRESULT CMLStr::CEnumAttr::Clone(IEnumUnknown** ppEnum)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppEnum);
    ASSERT_READ_PTR(m_pMLStr);

    return m_pMLStr->EnumAttr(ppEnum);
}

#endif // NEWMLSTR
