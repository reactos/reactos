// MLMain.h : Declaration of the CMultiLanguage

#ifndef __MLMAIN_H_
#define __MLMAIN_H_

#include "mlflink.h"
#include "mllbcons.h"

/////////////////////////////////////////////////////////////////////////////
// CMultiLanguage


class ATL_NO_VTABLE CMultiLanguage : 
    public CComObjectRoot,
    public CComCoClass<CMultiLanguage, &CLSID_CMultiLanguage>,
    public IMultiLanguage
{
    MIMECONTF       dwMimeSource;
    CMimeDatabase   *m_pMimeDatabase;

public:
    CMultiLanguage(void) 
    {
        m_pMimeDatabase = new CMimeDatabase;
        dwMimeSource = MIMECONTF_MIME_IE4;
        if (m_pMimeDatabase)
            m_pMimeDatabase->SetMimeDBSource(MIMECONTF_MIME_IE4);
    }
    ~CMultiLanguage(void)
    {
        if (m_pMimeDatabase)
            delete m_pMimeDatabase;
    }

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMultiLanguage)
        COM_INTERFACE_ENTRY(IMultiLanguage)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangCodePages, CMLFLink)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangFontLink, CMLFLink)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangFontLink2, CMLFLink2)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMultiLanguage2, CMultiLanguage2)
        COM_INTERFACE_ENTRY_TEAR_OFF(IID_IMLangLineBreakConsole, CMLLBCons)
    END_COM_MAP()

public:
// IMultiLanguage
    virtual STDMETHODIMP GetNumberOfCodePageInfo(UINT *pcCodePage);
    virtual STDMETHODIMP GetCodePageInfo(UINT uiCodePage, PMIMECPINFO pcpInfo);
    virtual STDMETHODIMP GetFamilyCodePage(UINT uiCodePage, UINT *puiFamilyCodePage);
    virtual STDMETHODIMP EnumCodePages(DWORD grfFlags, IEnumCodePage **ppEnumCodePage);
    virtual STDMETHODIMP GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo);
    virtual STDMETHODIMP IsConvertible(DWORD dwSrcEncoding, DWORD dwDstEncoding);
    virtual STDMETHODIMP ConvertString(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, BYTE *pSrcStr, UINT *pcSrcSize, BYTE *pDstStr, UINT *pcDstSize);
    virtual STDMETHODIMP ConvertStringToUnicode(LPDWORD lpdwMode, DWORD dwEncoding, CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize);
    virtual STDMETHODIMP ConvertStringFromUnicode(LPDWORD lpdwMode, DWORD dwEncoding, WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize);
    virtual STDMETHODIMP ConvertStringReset(void);
    virtual STDMETHODIMP GetRfc1766FromLcid(LCID Locale, BSTR *pbstrRfc1766);
    virtual STDMETHODIMP GetLcidFromRfc1766(PLCID pLocale, BSTR bstrRfc1766);
    virtual STDMETHODIMP EnumRfc1766(IEnumRfc1766 **ppEnumRfc1766);
    virtual STDMETHODIMP GetRfc1766Info(LCID Locale, PRFC1766INFO pRfc1766Info);
    virtual STDMETHODIMP CreateConvertCharset(UINT uiSrcCodePage, UINT uiDstCodePage, DWORD dwProperty, IMLangConvertCharset **ppMLangConvertCharset);

};

class ATL_NO_VTABLE CMultiLanguage2 : 
    public CComTearOffObjectBase<CMultiLanguage>,
    public IMultiLanguage2
{        
    IMultiLanguage  * m_pIML;
    MIMECONTF       dwMimeSource;
    CMimeDatabase   * m_pMimeDatabase;


public:

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMultiLanguage2)
        COM_INTERFACE_ENTRY(IMultiLanguage2)
    END_COM_MAP()

    CMultiLanguage2(void);
    ~CMultiLanguage2(void);

    virtual STDMETHODIMP GetNumberOfCodePageInfo(UINT *pcCodePage);
    virtual STDMETHODIMP GetCodePageInfo(UINT uiCodePage, LANGID LangId, PMIMECPINFO pcpInfo);
    virtual STDMETHODIMP GetFamilyCodePage(UINT uiCodePage, UINT *puiFamilyCodePage);
    virtual STDMETHODIMP EnumCodePages(DWORD grfFlags, LANGID LangId, IEnumCodePage **ppEnumCodePage);
    virtual STDMETHODIMP GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo);
    virtual STDMETHODIMP IsConvertible(DWORD dwSrcEncoding, DWORD dwDstEncoding)
    {
        if (m_pIML)
            return m_pIML->IsConvertible(dwSrcEncoding, dwDstEncoding);
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP ConvertString(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, BYTE *pSrcStr, UINT *pcSrcSize, BYTE *pDstStr, UINT *pcDstSize)
    {
        if (m_pIML)
            return m_pIML->ConvertString(lpdwMode, dwSrcEncoding, dwDstEncoding, pSrcStr, pcSrcSize, pDstStr, pcDstSize);
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP ConvertStringToUnicode(LPDWORD lpdwMode, DWORD dwEncoding, CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize)
    {
        if (m_pIML)
            return m_pIML->ConvertStringToUnicode(lpdwMode, dwEncoding, pSrcStr, pcSrcSize, pDstStr, pcDstSize);
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP ConvertStringFromUnicode(LPDWORD lpdwMode, DWORD dwEncoding, WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize)
    {
        if (m_pIML)
            return m_pIML->ConvertStringFromUnicode(lpdwMode, dwEncoding, pSrcStr, pcSrcSize, pDstStr, pcDstSize);
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP ConvertStringReset(void)
    {
        if (m_pIML)
            return m_pIML->ConvertStringReset();
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP GetRfc1766FromLcid(LCID Locale, BSTR *pbstrRfc1766)
    {
        if (m_pIML)
            return m_pIML->GetRfc1766FromLcid(Locale, pbstrRfc1766);
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP GetLcidFromRfc1766(PLCID pLocale, BSTR bstrRfc1766)
    {
        if (m_pIML)
            return m_pIML->GetLcidFromRfc1766(pLocale, bstrRfc1766);
        else
            return E_FAIL;
    }
    virtual STDMETHODIMP EnumRfc1766(LANGID LangId, IEnumRfc1766 **ppEnumRfc1766);
    virtual STDMETHODIMP GetRfc1766Info(LCID Locale, LANGID LangId, PRFC1766INFO pRfc1766Info);
    virtual STDMETHODIMP CreateConvertCharset(UINT uiSrcCodePage, UINT uiDstCodePage, DWORD dwProperty, IMLangConvertCharset **ppMLangConvertCharset)
    {
        if (m_pIML)
            return m_pIML->CreateConvertCharset(uiSrcCodePage, uiDstCodePage, dwProperty, ppMLangConvertCharset);
        else
            return E_FAIL;
    }

    virtual STDMETHODIMP ConvertStringInIStream(LPDWORD lpdwMode, DWORD dwFlag, WCHAR *lpFallBack, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut);
    virtual STDMETHODIMP ConvertStringToUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, CHAR *pSrcStr, UINT *pcSrcSize, WCHAR *pDstStr, UINT *pcDstSize, DWORD dwFlag=0, WCHAR *lpFallBack = NULL);
    virtual STDMETHODIMP ConvertStringFromUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, WCHAR *pSrcStr, UINT *pcSrcSize, CHAR *pDstStr, UINT *pcDstSize, DWORD dwFlag=0, WCHAR *lpFallBack = NULL);
    virtual STDMETHODIMP DetectCodepageInIStream(DWORD dwFlag, DWORD uiPrefWinCodepage, IStream *pstmIn, DetectEncodingInfo *lpEncoding, INT *pnScores);
    virtual STDMETHODIMP DetectInputCodepage(DWORD dwFlag, DWORD uiPrefWinCodepage, CHAR *pSrcStr, INT *pcSrcSize, DetectEncodingInfo *lpEncoding, INT *pnScores);

    virtual STDMETHODIMP ValidateCodePage(UINT uiCodePage, HWND hwnd);
    virtual STDMETHODIMP GetCodePageDescription(UINT uiCodePage, LCID lcid, LPWSTR lpWideCharStr,  int cchWideChar);
    virtual STDMETHODIMP IsCodePageInstallable(UINT uiCodePage);
    virtual STDMETHODIMP SetMimeDBSource(MIMECONTF dwSource);
    virtual STDMETHODIMP GetNumberOfScripts(UINT *pnScripts);
    virtual STDMETHODIMP EnumScripts(DWORD dwFlags, LANGID LangId, IEnumScript **ppEnumScript);
    
    virtual STDMETHODIMP ValidateCodePageEx(UINT uiCodePage, HWND hwnd, DWORD dwfIODControl);
protected:
    HRESULT EnsureIEStatus(void);
    class CIEStatus
    {
    public:
        CIEStatus(void) { _IEFlags.fJITDisabled = FALSE;}
        HRESULT Init(void);
        BOOL IsJITEnabled(void) 
        { 
            return !_IEFlags.fJITDisabled;
        }
    protected:
        struct {
            BOOL fJITDisabled:1;
        } _IEFlags;
    };
    CIEStatus *m_pIEStat;
};




#endif //__MLMAIN_H_
