// MLLBCons.h : Declaration of the CMLLBCons

#ifndef __MLLBCONS_H_
#define __MLLBCONS_H_

#include "mlatl.h"

class CMultiLanguage;

/////////////////////////////////////////////////////////////////////////////
// CMLLBCons
class ATL_NO_VTABLE CMLLBCons :
    public CComTearOffObjectBase<CMultiLanguage>,
    public IMLangLineBreakConsole
{
public:
    CMLLBCons(void) {m_pMLStrClass = NULL;}
    ~CMLLBCons(void) {if (m_pMLStrClass) m_pMLStrClass->Release();}

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLLBCons)
        COM_INTERFACE_ENTRY(IMLangLineBreakConsole)
    END_COM_MAP()

public:
// IMLangLineBreakConsole
    STDMETHOD(BreakLineML)(/*[in]*/ IMLangString* pSrcMLStr, /*[in]*/ long lSrcPos, /*[in]*/ long lSrcLen, /*[in]*/ long cMinColumns, /*[in]*/ long cMaxColumns, /*[out]*/ long* plLineLen, /*[out]*/ long* plSkipLen);
    STDMETHOD(BreakLineW)(/*[in]*/ LCID locale, /*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[in]*/ long cMaxColumns, /*[out]*/ long* pcchLine, /*[out]*/ long* pcchSkip);
    STDMETHOD(BreakLineA)(/*[in]*/ LCID locale, /*[in]*/ UINT uCodePage, /*[in, size_is(cchSrc)]*/ const CHAR* pszSrc, /*[in]*/ long cchSrc, /*[in]*/ long cMaxColumns, /*[out]*/ long* pcchLine, /*[out]*/ long* pcchSkip);

protected:
    template <DWORD INFOTYPE, int CACHESIZE>
    class CCharType
    {
    public:
#ifdef ASTRIMPL
        inline CCharType(void);
#else
        inline CCharType(LCID locale);
#endif
        inline ~CCharType(void);
        inline void Flush(void);
#ifdef ASTRIMPL
        WORD GetCharType(IMLangString* pMLStr, long lPos, long lLen, HRESULT* phr = NULL);
#else
        WORD GetCharType(LPCWSTR psz, int cch);
#endif

    protected:
        LPWORD m_pwBuf;
#ifdef ASTRIMPL
        IMLangStringAStr* m_pMLStrAStr;
        long m_lPos;
        long m_lLen;
#else
        LPSTR m_pszConv;
        LPCWSTR m_psz;
        int m_cch;
        LCID m_locale;
        UINT m_uCodePage;
#endif
        WORD m_wHalfWidth;
    };

    HRESULT PrepareMLStrClass(void)
    {
        if (m_pMLStrClass)
            return S_OK;
        else
            return ::_Module.GetClassObject(CLSID_CMLangString, IID_IClassFactory, (void**)&m_pMLStrClass);
    }

    IClassFactory* m_pMLStrClass;
};

template <DWORD INFOTYPE, int CACHESIZE>
#ifdef ASTRIMPL
CMLLBCons::CCharType<INFOTYPE, CACHESIZE>::CCharType(void)
#else
CMLLBCons::CCharType<INFOTYPE, CACHESIZE>::CCharType(LCID locale)
#endif
{
#ifndef ASTRIMPL
    TCHAR szCodePage[8];
#endif

    m_pwBuf = NULL;
#ifdef ASTRIMPL
    m_pMLStrAStr = NULL;

    // TODO: Set m_wHaldWidth here.
    m_wHalfWidth = 0;
#else
    m_pszConv = NULL;
    m_locale = locale;
    ::GetLocaleInfo(m_locale, LOCALE_IDEFAULTANSICODEPAGE, szCodePage, ARRAYSIZE(szCodePage));
    m_uCodePage = _ttoi(szCodePage);

    CPINFO cpi;
    ::GetCPInfo(m_uCodePage, &cpi);
    m_wHalfWidth = (cpi.LeadByte[0]) ? 0 : C3_HALFWIDTH;
#endif
}

template <DWORD INFOTYPE, int CACHESIZE>
CMLLBCons::CCharType<INFOTYPE, CACHESIZE>::~CCharType(void)
{
    if (m_pwBuf)
        delete[] m_pwBuf;
#ifdef ASTRIMPL
    if (m_pMLStrAStr)
        m_pMLStrAStr->Release();
#else
    if (m_pszConv)
        delete[] m_pszConv;
#endif
}

template <DWORD INFOTYPE, int CACHESIZE>
void CMLLBCons::CCharType<INFOTYPE, CACHESIZE>::Flush(void)
{
#ifdef ASTRIMPL
    m_lPos = 0;
    m_lLen = 0;
#else
    m_psz = NULL;
    m_cch = 0;
#endif
}

template <DWORD INFOTYPE, int CACHESIZE>
#ifdef ASTRIMPL
WORD CMLLBCons::CCharType<INFOTYPE, CACHESIZE>::GetCharType(IMLangString* pMLStr, long lPos, long lLen, HRESULT* phr)
#else
WORD CMLLBCons::CCharType<INFOTYPE, CACHESIZE>::GetCharType(LPCWSTR psz, int cch)
#endif
{
#ifdef ASTRIMPL
    if (lPos >= m_lPos + m_lLen || lPos + lLen <= m_lPos)
#else
    if (psz < m_psz || psz >= m_psz + m_cch)
#endif
    {
#ifdef ASTRIMPL
        HRESULT hr;
#endif
        LPWORD pwCharType;
#ifdef ASTRIMPL
        WORD wCharType;
        CHAR* psz;
        long cch;
        LCID locale;
        UINT uCodePage;

        if (m_pMLStrAStr)
        {
            m_pMLStrAStr->Release();
            m_pMLStrAStr = NULL;
        }

        if (SUCCEEDED(hr = pMLStr->QueryInterface(IID_IMLangStringAStr, (void**)&m_pMLStrAStr)))
        {
            if (!m_pwBuf)
                m_pwBuf = new WORD[CACHESIZE];

            if (m_pwBuf)
            {
                pwCharType = m_pwBuf;
                cch = CACHESIZE;
            }
            else
            {
                pwCharType = &wCharType;
                cch = 1;
            }

            lLen = min(lLen, cch);

            if (SUCCEEDED(hr = m_pMLStrAStr->GetLocale(lPos, lLen, &locale, NULL, &lLen)) &&
                SUCCEEDED(hr = ::LocaleToCodePage(locale, &uCodePage)) &&
                SUCCEEDED(hr = m_pMLStrAStr->LockAStr(lPos, lLen, MLSTR_READ, uCodePage, 0, NULL, &psz, &cch, NULL)))
            {
                if (!::GetStringTypeExA(locale, INFOTYPE, psz, cch, pwCharType))
                    hr = E_FAIL; // NLS failed

                ASSIGN_IF_FAILED(hr, m_pMLStrAStr->UnlockAStr(psz, 0, NULL, NULL));
            }
        }

        if (phr)
            *phr = hr;

        if (SUCCEEDED(hr))
        {
            m_lPos = lPos;
            m_lLen = lLen;
            return pwCharType[0] | m_wHalfWidth;
        }
        else
        {
            m_lPos = 0;
            m_lLen = 0;
            return 0 | m_wHalfWidth;
        }
#else
        LPSTR pszConv;
        WORD wCharType = 0;
        CHAR szTemp[2];

        if (!m_pwBuf)
            m_pwBuf = new WORD[CACHESIZE];

        if (m_pwBuf)
        {
            pwCharType = m_pwBuf;
            cch = min(cch, CACHESIZE);
        }
        else
        {
            pwCharType = &wCharType;
            cch = 1;
        }

        if (!m_pszConv)
            m_pszConv = new CHAR[CACHESIZE * 2];

        if (m_pszConv)
        {
            pszConv = m_pszConv;
        }
        else
        {
            pszConv = szTemp;
            cch = 1;
        }

        if (m_pwBuf && m_pszConv)
        {
            m_psz = psz;
            m_cch = cch;
        }

        int cchTemp = ::WideCharToMultiByte(m_uCodePage, 0, psz, cch, pszConv, CACHESIZE * 2, NULL, NULL);
        ::GetStringTypeExA(m_locale, INFOTYPE, pszConv, cchTemp, pwCharType);

        return pwCharType[0] | m_wHalfWidth;
#endif
    }
    else
    {
#ifdef ASTRIMPL
        return m_pwBuf[lPos - m_lPos] | m_wHalfWidth;
#else
        return m_pwBuf[psz - m_psz] | m_wHalfWidth;
#endif
    }
}

#endif //__MLLBCONS_H_
