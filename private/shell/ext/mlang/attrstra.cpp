// AttrStrA.cpp : Implementation of CMLStrAttrAStr
#include "private.h"

#ifdef NEWMLSTR

#include "attrstra.h"
#include "mlswalk.h"
#include "mlsbwalk.h"

/////////////////////////////////////////////////////////////////////////////
// CMLStrAttrAStr

CMLStrAttrAStr::CMLStrAttrAStr(void) :
    m_pMLCPs(NULL),
    m_pMLStr(NULL)
{
}

CMLStrAttrAStr::~CMLStrAttrAStr(void)
{
    VERIFY(SetClient(NULL)); // Clean m_pMLStr
    if (m_pMLCPs)
        m_pMLCPs->Release();
}

STDMETHODIMP CMLStrAttrAStr::SetClient(IUnknown* pUnk)
{
    ASSERT_THIS;
    ASSERT_READ_PTR_OR_NULL(pUnk);

    HRESULT hr = S_OK;

    // Release old client
    IMLangString* const pMLStr = m_pMLStr;
    if (pMLStr && SUCCEEDED(hr = StartEndConnectionMLStr(pMLStr, FALSE))) // End connection to MLStr
    {
        pMLStr->Release();
        m_pMLStr = NULL;
    }

    // Set new client
    if (SUCCEEDED(hr) && pUnk) // pUnk is given
    {
        ASSERT(!m_pMLStr);
        if (SUCCEEDED(hr = pUnk->QueryInterface(IID_IMLangString, (void**)&m_pMLStr)))
        {
            ASSERT_READ_PTR(m_pMLStr);
            if (FAILED(hr = StartEndConnectionMLStr(pUnk, TRUE))) // Start connection to MLStr
            {
                m_pMLStr->Release();
                m_pMLStr = NULL;
            }
        }
    }

    return hr;
}

HRESULT CMLStrAttrAStr::StartEndConnectionMLStr(IUnknown* const pUnk, BOOL fStart)
{
    ASSERT_THIS;
    ASSERT_READ_PTR(pUnk);

    HRESULT hr;
    IConnectionPointContainer* pCPC;

    if (SUCCEEDED(hr = pUnk->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC)))
    {
        ASSERT_READ_PTR(pCPC);

        IConnectionPoint* pCP;

        if (SUCCEEDED(hr = pCPC->FindConnectionPoint(IID_IMLangStringNotifySink, &pCP)))
        {
            ASSERT_READ_PTR(pCP);

            if (fStart)
                hr = pCP->Advise((IMLStrAttr*)this, &m_dwMLStrCookie);
            else
                hr = pCP->Unadvise(m_dwMLStrCookie);

            pCP->Release();
        }

        pCPC->Release();
    }

    return hr;
}

STDMETHODIMP CMLStrAttrAStr::GetClient(IUnknown** ppUnk)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(ppUnk);

    if (ppUnk)
    {
        IUnknown* const pUnk = m_pMLStr;
        *ppUnk = pUnk;
        if (pUnk)
            pUnk->AddRef();
    }

    return S_OK;
}

STDMETHODIMP CMLStrAttrAStr::QueryAttr(REFIID riid, LPARAM lParam, IUnknown** ppUnk, long* lConf)
{
    return E_NOTIMPL; // CMLStrAttrAStr::QueryAttr()
}

STDMETHODIMP CMLStrAttrAStr::GetAttrInterface(IID* pIID, LPARAM* plParam)
{
    return E_NOTIMPL; // CMLStrAttrAStr::GetAttrInterface()
}

STDMETHODIMP CMLStrAttrAStr::SetMLStr(long lDestPos, long lDestLen, IUnknown* pSrcMLStr, long lSrcPos, long lSrcLen)
{
    return E_NOTIMPL; // CMLStrAttrAStr::SetMLStr()
}

STDMETHODIMP CMLStrAttrAStr::SetAStr(long lDestPos, long lDestLen, UINT uCodePage, const CHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr;

    // Fire OnRequestEdit
    IEnumConnections* pEnumConn;

    if (SUCCEEDED(hr = EnumConnections(&pEnumConn)))
    {
        ASSERT_READ_PTR(pEnumConn);

        CONNECTDATA cd;

        while ((hr = pEnumConn->Next(1, &cd, NULL)) == S_OK)
        {
            IMLStrAttrNotifySink* pSink;

            if (SUCCEEDED(hr = cd.pUnk->QueryInterface(IID_IMLStrAttrNotifySink, (void**)&pSink)))
            {
                // TODO: Regularize before fire OnRequestEdit
                // TODO: And, calculate lNewLen,
                hr = pSink->OnRequestEdit(lDestPos, lDestLen, /*lNewLen*/0, IID_IMLStrAttrAStr, 0, (IMLStrAttr*)this);
                pSink->Release();
            }
        }

        pEnumConn->Release();
    }

    hr = CheckThread();
    CLock Lock(TRUE, this, hr);
    long cchDestPos;
    long cchDestLen;
    long cchActual;
    long lActualLen;

    if (SUCCEEDED(hr) && (GetBufFlags() & MLSTR_WRITE))
        hr = E_INVALIDARG; // Not writable StrBuf; TODO: Replace StrBuf in this case if allowed

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = PrepareMLStrBuf()) &&
        SUCCEEDED(hr = RegularizePosLen(&lDestPos, &lDestLen)) &&
        SUCCEEDED(hr = GetCCh(0, lDestPos, &cchDestPos)) &&
        SUCCEEDED(hr = GetCCh(cchDestPos, lDestLen, &cchDestLen)))
    {
        IMLangStringBufA* const pMLStrBufA = GetMLStrBufA();

        if (uCodePage == CP_ACP)
            uCodePage = ::GetACP();

        if (pMLStrBufA && uCodePage == GetCodePage())
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
                    hr = CalcLenA(uCodePage, pszSrc, BufWalk.GetCCh(), &lLen);

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

            if (SUCCEEDED(hr = ((IMLStrAttr*)this)->QueryInterface(IID_IMLangStringWStr, (void**)&pMLStrW)))
            {
                CMLStrWalkW StrWalk(pMLStrW, lDestPos, lDestLen, MLSTR_WRITE, (pcchActual || plActualLen));

                cchActual = 0;
                lActualLen = 0;
                while (StrWalk.Lock(hr))
                {
                    long cchWrittenA;
                    long lWrittenLen;

                    if (SUCCEEDED(hr = ConvAStrToWStr(uCodePage, pszSrc, cchSrc, StrWalk.GetStr(), StrWalk.GetCCh(), &cchWrittenA, NULL, &lWrittenLen)))
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
}

STDMETHODIMP CMLStrAttrAStr::SetStrBufA(long lDestPos, long lDestLen, UINT uCodePage, IMLangStringBufA* pSrcBuf, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    return SetStrBufCommon(this, lDestPos, lDestLen, uCodePage, NULL, pSrcBuf, pcchActual, plActualLen);
}

STDMETHODIMP CMLStrAttrAStr::GetAStr(long lSrcPos, long lSrcLen, UINT uCodePageIn, UINT* puCodePageOut, CHAR* pszDest, long cchDest, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puCodePageOut);
    ASSERT_WRITE_BLOCK_OR_NULL(pszDest, cchDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    HRESULT hr = CheckThread();
    CLock Lock(FALSE, this, hr);
    long cchSrcPos;
    long cchSrcLen;
    long cchActual;
    long lActualLen;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
        IMLangStringBufA* const pMLStrBufA = GetMLStrBufA();

        if (pszDest)
            cchActual = min(cchSrcLen, cchDest);
        else
            cchActual = cchSrcLen;

        if (uCodePageIn == CP_ACP)
            uCodePageIn = ::GetACP();

        if (pMLStrBufA && (puCodePageOut || uCodePageIn == GetCodePage()))
        {
            uCodePageIn = GetCodePage();

            CMLStrBufWalkA BufWalk(pMLStrBufA, cchSrcPos, cchActual, (pcchActual || plActualLen));

            lActualLen = 0;
            while (BufWalk.Lock(hr))
            {
                long lLen;

                if (plActualLen)
                    hr = CalcLenA(uCodePageIn, BufWalk.GetStr(), BufWalk.GetCCh(), &lLen);

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

            if (SUCCEEDED(hr = m_pMLStr->QueryInterface(IID_IMLangStringWStr, (void**)&pMLStrW)))
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
                        SUCCEEDED(hr = pMLStrW->GetLocale(lSrcPos, lSrcLen, &locale, NULL, NULL)) &&
                        SUCCEEDED(hr = ::LocaleToCodePage(locale, &uLocaleCodePage)) &&
                        SUCCEEDED(hr = PrepareMLangCodePages()) &&
                        SUCCEEDED(hr = GetMLangCodePages()->CodePageToCodePages(uLocaleCodePage, &dwLocaleCodePages)) &&
                        SUCCEEDED(hr = GetMLangCodePages()->GetStrCodePages(StrWalk.GetStr(), StrWalk.GetCCh(), dwLocaleCodePages, &dwStrCodePages, NULL)))
                    {
                        fDontHaveCodePageIn = FALSE;
                        hr = GetMLangCodePages()->CodePagesToCodePage(dwStrCodePages, uLocaleCodePage, &uCodePageIn);
                    }

                    if (SUCCEEDED(hr) &&
                        SUCCEEDED(hr = ConvWStrToAStr(pcchActual || plActualLen, uCodePageIn, StrWalk.GetStr(), StrWalk.GetCCh(), pszDest, cchDest, &cchWritten, NULL, &lWrittenLen)))
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

    if (SUCCEEDED(hr))
    {
        if (puCodePageOut)
            *puCodePageOut = uCodePageIn;
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

STDMETHODIMP CMLStrAttrAStr::GetStrBufA(long lSrcPos, long lSrcMaxLen, UINT* puDestCodePage, IMLangStringBufA** ppDestBuf, long* plDestLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puDestCodePage);
    ASSERT_WRITE_PTR_OR_NULL(ppDestBuf);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    HRESULT hr = CheckThread();
    CLock Lock(FALSE, this, hr);
    IMLangStringBufA* pMLStrBufA;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = RegularizePosLen(&lSrcPos, &lSrcMaxLen)) &&
        lSrcMaxLen <= 0)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        pMLStrBufA = GetMLStrBufA();
        if (!pMLStrBufA)
            hr = MLSTR_E_STRBUFNOTAVAILABLE;
    }

    if (SUCCEEDED(hr))
    {
        if (puDestCodePage)
            *puDestCodePage = GetCodePage();
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
}

STDMETHODIMP CMLStrAttrAStr::LockAStr(long lSrcPos, long lSrcLen, long lFlags, UINT uCodePageIn, long cchRequest, UINT* puCodePageOut, CHAR** ppszDest, long* pcchDest, long* plDestLen)
{
    ASSERT_THIS;
    ASSERT_WRITE_PTR_OR_NULL(puCodePageOut);
    ASSERT_WRITE_PTR_OR_NULL(ppszDest);
    ASSERT_WRITE_PTR_OR_NULL(pcchDest);
    ASSERT_WRITE_PTR_OR_NULL(plDestLen);

    HRESULT hr = CheckThread();
    CLock Lock(lFlags & MLSTR_WRITE, this, hr);
    long cchSrcPos;
    long cchSrcLen;
    CHAR* pszBuf = NULL;
    long cchBuf;
    long lLockLen;
    BOOL fDirectLock;

    if (SUCCEEDED(hr) && (!lFlags || (lFlags & ~GetBufFlags() & MLSTR_WRITE)))
        hr = E_INVALIDARG; // No flags specified, or not writable StrBuf; TODO: Replace StrBuf in this case if allowed

    if (!(lFlags & MLSTR_WRITE))
        cchRequest = 0;

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = PrepareMLStrBuf()) &&
        SUCCEEDED(hr = RegularizePosLen(&lSrcPos, &lSrcLen)) &&
        SUCCEEDED(hr = GetCCh(0, lSrcPos, &cchSrcPos)) &&
        SUCCEEDED(hr = GetCCh(cchSrcPos, lSrcLen, &cchSrcLen)))
    {
        IMLangStringBufA* const pMLStrBufA = GetMLStrBufA();
        fDirectLock = (pMLStrBufA && (puCodePageOut || uCodePageIn == GetCodePage()));

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
                SetBufCCh(GetBufCCh() + cchInserted);
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
                hr = CalcLenA(uCodePageIn, pszBuf, cchBuf, &lLockLen);
        }
        else
        {
            long cchSize;

            if (SUCCEEDED(hr = CalcBufSizeA(lSrcLen, &cchSize)))
            {
                cchBuf = max(cchSize, cchRequest);
                hr = MemAlloc(sizeof(*pszBuf) * cchBuf, (void**)&pszBuf);
            }

            if (SUCCEEDED(hr) && ((lFlags & MLSTR_READ) || puCodePageOut))
                hr = GetAStr(lSrcPos, lSrcLen,  uCodePageIn, (puCodePageOut) ? &uCodePageIn : NULL, (lFlags & MLSTR_READ) ? pszBuf : NULL, cchBuf, (pcchDest) ? &cchBuf : NULL, (plDestLen) ? &lLockLen : NULL);
        }
    }

    if (SUCCEEDED(hr) &&
        SUCCEEDED(hr = Lock.FallThrough()))
    {
        hr = GetLockInfo()->Lock((fDirectLock) ? UnlockAStrDirect : UnlockAStrIndirect, lFlags, uCodePageIn, pszBuf, lSrcPos, lSrcLen, cchSrcPos, cchBuf);
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
                GetMLStrBufA()->UnlockBuf(pszBuf, 0, 0);
            else
                MemFree(pszBuf);
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
}

STDMETHODIMP CMLStrAttrAStr::UnlockAStr(const CHAR* pszSrc, long cchSrc, long* pcchActual, long* plActualLen)
{
    ASSERT_THIS;
    ASSERT_READ_BLOCK(pszSrc, cchSrc);
    ASSERT_WRITE_PTR_OR_NULL(pcchActual);
    ASSERT_WRITE_PTR_OR_NULL(plActualLen);

    return UnlockStrCommon(pszSrc, cchSrc, pcchActual, plActualLen);
}

STDMETHODIMP CMLStrAttrAStr::OnRegisterAttr(IUnknown* pUnk)
{
    return E_NOTIMPL; // CMLStrAttrAStr::OnRegisterAttr()
}

STDMETHODIMP CMLStrAttrAStr::OnUnregisterAttr(IUnknown* pUnk)
{
    return E_NOTIMPL; // CMLStrAttrAStr::OnUnregisterAttr()
}

STDMETHODIMP CMLStrAttrAStr::OnRequestEdit(long lDestPos, long lDestLen, long lNewLen, REFIID riid, LPARAM lParam, IUnknown* pUnk)
{
    return E_NOTIMPL; // CMLStrAttrAStr::OnRequestEdit()
}

STDMETHODIMP CMLStrAttrAStr::OnCanceledEdit(long lDestPos, long lDestLen, long lNewLen, REFIID riid, LPARAM lParam, IUnknown* pUnk)
{
    return E_NOTIMPL; // CMLStrAttrAStr::OnCanceledEdit()
}

STDMETHODIMP CMLStrAttrAStr::OnChanged(long lDestPos, long lDestLen, long lNewLen, REFIID riid, LPARAM lParam, IUnknown* pUnk)
{
    return E_NOTIMPL; // CMLStrAttrAStr::OnChanged()
}

#endif // NEWMLSTR
