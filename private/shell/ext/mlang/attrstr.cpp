// AttrStr.cpp : Implementation of CMLStrAttrStrCommonAttrStrCommon
#include "private.h"

#ifdef NEWMLSTR

#include "attrstr.h"
#include "mlsbwalk.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrStrCommon

CMLStrAttrStrCommon::CMLStrAttrStrCommon(void) :
    m_pMLStrBufW(NULL),
    m_pMLStrBufA(NULL),
    m_lBufFlags(0),
    m_cchBuf(0),
    m_locale(0),
    m_LockInfo(this)
{
    m_dwThreadID = ::GetCurrentThreadId();
}

CMLStrAttrStrCommon::~CMLStrAttrStrCommon(void)
{
    if (m_pMLStrBufW)
        m_pMLStrBufW->Release();
    if (m_pMLStrBufA)
        m_pMLStrBufA->Release();
}

HRESULT CMLStrAttrStrCommon::SetStrBufCommon(void* pMLStrX, long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufW* pSrcBufW, IMLangStringBufA* pSrcBufA, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_PTR_OR_NULL(pSrcBufW);
    ASSERT_READ_PTR_OR_NULL(pSrcBufA);
    ASSERT(!pSrcBufW || !pSrcBufA); // Either one or both should be NULL
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr = CheckThread();
    CLock Lock(TRUE, this, hr);
    long lBufFlags = 0; // '= 0' for in case of both of pSrcBufW and pSrcBufA are NULL
    long cchBuf = 0;
    long cchDestPos;
    long cchDestLen;
    long lActualLen = 0;

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
                hr = GetMLStrAttr()->SetMLStr(lDestPos, lDestLen, NULL, 0, 0);
            }
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

HRESULT CMLStrAttrStrCommon::UnlockWStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
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

HRESULT CMLStrAttrStrCommon::UnlockWStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr = S_OK;

    if (GetLockInfo()->GetFlags(pKey) & MLSTR_WRITE)
    {
        CComQIPtr<IMLStrAttrWStr, &IID_IMLStrAttrWStr> pAttrWStr(GetMLStrAttr());
        ASSERT(pAttrWStr);
        hr = pAttrWStr->SetWStr(GetLockInfo()->GetPos(pKey), GetLockInfo()->GetLen(pKey), (WCHAR*)pszSrc, cchSrc, pcchActual, plActualLen);
    }

    ASSIGN_IF_FAILED(hr, MemFree((void*)pszSrc));

    return hr;
}

HRESULT CMLStrAttrStrCommon::UnlockAStrDirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
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

HRESULT CMLStrAttrStrCommon::UnlockAStrIndirect(void* pKey, const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    HRESULT hr = S_OK;

    if (GetLockInfo()->GetFlags(pKey) & MLSTR_WRITE)
    {
        CComQIPtr<IMLStrAttrAStr, &IID_IMLStrAttrAStr> pAttrAStr(GetMLStrAttr());
        ASSERT(pAttrAStr);
        hr = pAttrAStr->SetAStr(GetLockInfo()->GetPos(pKey), GetLockInfo()->GetLen(pKey), GetLockInfo()->GetCodePage(pKey), (CHAR*)pszSrc, cchSrc, pcchActual, plActualLen);
    }

    ASSIGN_IF_FAILED(hr, MemFree((void*)pszSrc));

    return hr;
}

HRESULT CMLStrAttrStrCommon::UnlockStrCommon(const void* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
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

HRESULT CMLStrAttrStrCommon::PrepareMLStrBuf(void)
{
    if (GetMLStrBufW() || GetMLStrBufA())
        return S_OK;

    IMLangStringBufW* pBuf = new CMLStrAttrStrCommon::CMLStrBufStandardW;
    if (pBuf)
    {
        SetMLStrBufW(pBuf);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

HRESULT CMLStrAttrStrCommon::RegularizePosLen(long* plPos, long* plLen)
{
    HRESULT hr;
    long lStrLen;

    if (SUCCEEDED(hr = GetLen(0, GetBufCCh(), &lStrLen)))
        hr = ::RegularizePosLen(lStrLen, plPos, plLen);

    return hr;
}

HRESULT CMLStrAttrStrCommon::GetCCh(long cchOffset, long lLen, long* pcchLen)
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
        CMLStrBufWalkA BufWalk(GetMLStrBufA(), cchOffset, GetBufCCh() - cchOffset);

        while (lLen > 0 && BufWalk.Lock(hr))
        {
            for (LPCSTR pszTemp = BufWalk.GetStr(); lLen > 0 && *pszTemp; lLen--)
                pszTemp = ::CharNextExA((WORD)GetCodePage(), pszTemp, 0);

            if (!*pszTemp)
                lLen = 0; // String terminated

            BufWalk.Unlock(hr);
        }

        if (pcchLen)
        {
            if (SUCCEEDED(hr))
                *pcchLen = BufWalk.GetDoneCCh();
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

HRESULT CMLStrAttrStrCommon::GetLen(long cchOffset, long cchLen, long* plLen)
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

HRESULT CMLStrAttrStrCommon::CalcLenA(UINT uCodePage, const CHAR* psz, long cchLen, long* plLen)
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

HRESULT CMLStrAttrStrCommon::CalcCChA(UINT uCodePage, const CHAR* psz, long lLen, long* pcchLen)
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

HRESULT CMLStrAttrStrCommon::ConvAStrToWStr(UINT uCodePage, const CHAR* pszSrc, long cchSrc, WCHAR* pszDest, long cchDest, long* pcchActualA, long* pcchActualW, long* plActualLen)
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

HRESULT CMLStrAttrStrCommon::ConvWStrToAStr(BOOL fCanStopAtMiddle, UINT uCodePage, const WCHAR* pszSrc, long cchSrc, CHAR* pszDest, long cchDest, long* pcchActualA, long* pcchActualW, long* plActualLen)
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

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrStrCommon::CLockInfo

HRESULT CMLStrAttrStrCommon::CLockInfo::UnlockAll(void)
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

HRESULT CMLStrAttrStrCommon::CLockInfo::Lock(PFNUNLOCKPROC pfnUnlockProc, long lFlags, UINT uCodePage, void* psz, long lPos, long lLen, long cchPos, long cchLen)
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

HRESULT CMLStrAttrStrCommon::CLockInfo::Find(const void* psz, long, void** ppKey)
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

HRESULT CMLStrAttrStrCommon::CLockInfo::Unlock(void* pKey, const void* psz, long cch, long* pcchActual, long* plActualLen)
{
    CLockInfoEntry* const pEntry = (CLockInfoEntry*)pKey;
    HRESULT hr;

    if (!(pEntry->m_lFlags & MLSTR_WRITE))
    {
        cch = 0;
        if (plActualLen)
            *plActualLen = 0;
    }

    hr = (m_pCommon->*(pEntry->m_pfnUnlockProc))(pKey, psz, cch, pcchActual, plActualLen);

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
// CMLStrAttrStrCommon::CMLStrBufStandardW

long CMLStrAttrStrCommon::CMLStrBufStandardW::RoundBufSize(long cchStr)
{
    for (int n = 8; n < 12; n++)
    {
        if (cchStr < (1L << n))
            break;
    }
    const long cchTick = (1L << (n - 4));
    return (cchStr + cchTick - 1) / cchTick * cchTick;
}

#endif // NEWMLSTR
