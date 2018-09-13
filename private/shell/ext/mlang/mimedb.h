#ifndef _MIMEDB_H_
#define _MIMEDB_H_

#define ALLOC_BLOCK             10

typedef struct tagMIMECODEPAGE {
   UINT    uiCodePage;
   LPWSTR  wszHeaderCharset;
   LPWSTR  wszBodyCharset;
   LPWSTR  wszWebCharset;
   UINT    uidFixedWidthFont;  
   UINT    uidProportionalFont;
   UINT    uidDescription;     
   UINT    uiFamilyCodePage;
   DWORD   dwFlags;
} MIMECODEPAGE;

typedef struct tagMIMEREGCHARSET
{
    LPCWSTR szCharset;
    UINT uiCodePage;
    UINT uiInternetEncoding;
    DWORD   dwFlags;
}   MIMECHARSET;

typedef struct tagMIMERFC1766
{
    LCID    LcId;
    LPCWSTR szRfc1766;
    UINT    uidLCID;
    DWORD   dwFlags;
}   MIMERFC1766;


extern MIMECODEPAGE MimeCodePage[];
extern const MIMERFC1766  MimeRfc1766[];
extern const MIMECHARSET  MimeCharSet[];


#ifdef  __cplusplus
//
//  CMimeDatabase declaration without IMimeDatabase Interface
//
class CMimeDatabase     // This would support IMimeDatabase when available
{
    MIMECONTF dwMimeSource;
public:
    // Possible IMimeDatabase methods
    STDMETHODIMP GetNumberOfCodePageInfo(UINT *pcCodePage);
    STDMETHODIMP EnumCodePageInfo(void);
    STDMETHODIMP GetCodePageInfo(UINT uiCodePage, LANGID LangId, PMIMECPINFO pcpInfo);
    STDMETHODIMP GetCodePageInfoWithIndex(UINT uidx, LANGID LangId, PMIMECPINFO pcpInfo);
    STDMETHODIMP GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo);
    STDMETHODIMP ValidateCP(UINT uiCodePage);
    STDMETHODIMP SetMimeDBSource(MIMECONTF dwSource)
    {        
        if ((dwSource != MIMECONTF_MIME_IE4) &&
            (dwSource != MIMECONTF_MIME_LATEST) &&
            (dwSource != MIMECONTF_MIME_REGISTRY))
        {
            return E_INVALIDARG;
        }
        dwMimeSource = dwSource;
        return S_OK;
    }
    CMimeDatabase(void)
    {
        dwMimeSource = MIMECONTF_MIME_LATEST;
    }


protected:
    void FreeMimeDatabase(void);
    BOOL CheckFont(BYTE bGDICharset);


};

//
//  Globals
//
extern CMimeDatabase    *g_pMimeDatabase;
//
//  CMimeDatabase declaration without IMimeDatabase Interface
//

class CMimeDatabaseReg     // This would support IMimeDatabase when available
{
public:
    // Possible IMimeDatabase methods
    STDMETHODIMP GetNumberOfCodePageInfo(UINT *pcCodePage);
    STDMETHODIMP EnumCodePageInfo(void);
    STDMETHODIMP GetCodePageInfo(UINT uiCodePage, PMIMECPINFO pcpInfo);
    STDMETHODIMP GetCodePageInfoWithIndex(UINT uidx, PMIMECPINFO pcpInfo);
    STDMETHODIMP GetCharsetInfo(BSTR Charset, PMIMECSETINFO pcsetInfo);
    STDMETHODIMP ValidateCP(UINT uiCodePage);
    STDMETHODIMP LcidToRfc1766A(LCID Locale, LPSTR pszRfc1766, int iMaxLength);
    STDMETHODIMP LcidToRfc1766W(LCID Locale, LPWSTR pwszRfc1766, int nChar);
    STDMETHODIMP Rfc1766ToLcidA(PLCID pLocale, LPCSTR pszRfc1766);
    STDMETHODIMP Rfc1766ToLcidW(PLCID pLocale, LPCWSTR pwszRfc1766);
    void EnsureRfc1766Table(void);

    // Constructor & Destructor
    CMimeDatabaseReg();
    ~CMimeDatabaseReg();

protected:
    void BuildCodePageMimeDatabase(void);
    void BuildCharsetMimeDatabase(void);
    void FreeMimeDatabase(void);
    int FindCodePageFromCache(UINT uiCodePage);
    BOOL FindCodePageFromRegistry(UINT uiCodePage, MIMECPINFO *pCPInfo);
    int FindCharsetFromCache(BSTR Charset);
    int FindCharsetFromRegistry(BSTR Charset, BOOL fFromAlias);
    BOOL CheckFont(BYTE bGDICharset);
    void QSortCodePageInfo(LONG left, LONG right);
    void QSortCharsetInfo(LONG left, LONG right);
    void BuildRfc1766Table(void);
    void FreeRfc1766Table(void);

    CRITICAL_SECTION _cs;

    BOOL            _fAllCPCached;    
    PMIMECPINFO     _pCodePage;
    UINT            _cCodePage;
    UINT            _cMaxCodePage;
    PMIMECSETINFO   _pCharset;
    UINT            _cCharset;
    UINT            _cMaxCharset;
};

extern CMimeDatabaseReg    *g_pMimeDatabaseReg;

#endif  // __cplusplus


#endif  // _MIMEDB_H_
