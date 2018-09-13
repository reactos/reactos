// MLFLink.h : Declaration of the CMLFLink

#ifndef __MLFLINK_H_
#define __MLFLINK_H_

#include "mlatl.h"
#include "font.h"

#define NUMFONTMAPENTRIES 15

// Error Code
#define FACILITY_MLSTR                  0x0A15
#define MLSTR_E_FACEMAPPINGFAILURE      MAKE_HRESULT(1, FACILITY_MLSTR, 1001)


extern FONTINFO *g_pfont_table;


class CMultiLanguage;
/////////////////////////////////////////////////////////////////////////////
// CMLFLink

class ATL_NO_VTABLE CMLFLink : 
    public CComTearOffObjectBase<CMultiLanguage>,
    public IMLangFontLink
{
    friend void CMLangFontLink_FreeGlobalObjects(void);

public:
    CMLFLink(void);
    ~CMLFLink(void)
    {
        if (m_pFlinkTable)
            FreeFlinkTable();
    }

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(CMLFLink)
        COM_INTERFACE_ENTRY(IMLangCodePages)
        COM_INTERFACE_ENTRY(IMLangFontLink)
    END_COM_MAP()

public:
// IMLangCodePages
    STDMETHOD(GetCharCodePages)(/*[in]*/ WCHAR chSrc, /*[out]*/ DWORD* pdwCodePages);
    STDMETHOD(GetStrCodePages)(/*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[in]*/ DWORD dwPriorityCodePages, /*[out]*/ DWORD* pdwCodePages, /*[out]*/ long* pcchCodePages);
    STDMETHOD(CodePageToCodePages)(/*[in]*/ UINT uCodePage, /*[out]*/ DWORD* pdwCodePages);
    STDMETHOD(CodePagesToCodePage)(/*[in]*/ DWORD dwCodePages, /*[in]*/ UINT uDefaultCodePage, /*[out]*/ UINT* puCodePage);
// IMLangFontLink
    STDMETHOD(GetFontCodePages)(/*[in]*/ HDC hDC, /*[in]*/ HFONT hFont, /*[out]*/ DWORD* pdwCodePages);
    STDMETHOD(MapFont)(/*[in]*/ HDC hDC, /*[in]*/ DWORD dwCodePages, /*[in]*/ HFONT hSrcFont, /*[out]*/ HFONT* phDestFont);
    STDMETHOD(ReleaseFont)(/*[in]*/ HFONT hFont);
    STDMETHOD(ResetFontMapping)(void);

protected:
    static int CALLBACK GetFontCodePagesEnumFontProc(const LOGFONT *lplf, const TEXTMETRIC *lptm, DWORD dwFontType, LPARAM lParam);

// MapFont() support functions
    class CFontMappingInfo
    {
    public:
        CFontMappingInfo(void) : hDestFont(NULL) {}
        ~CFontMappingInfo(void) {if (hDestFont) ::DeleteObject(hDestFont);}

        HDC hDC;
        int iCP;
        HFONT hDestFont;
        TCHAR szFaceName[LF_FACESIZE];
        LOGFONT lfSrcFont;
        LOGFONT lfDestFont;
        UINT auCodePage[32 + 1]; // +1 for end mark
        DWORD adwCodePages[32 + 1];
    };

    typedef HRESULT (CMLFLink::*PFNGETFACENAME)(CFontMappingInfo& fmi);

    HRESULT MapFontCodePages(CFontMappingInfo& fmi, PFNGETFACENAME pfnGetFaceName);
    static int CALLBACK MapFontEnumFontProc(const LOGFONT* lplf, const TEXTMETRIC*, DWORD, LPARAM lParam);
    HRESULT GetFaceNameRegistry(CFontMappingInfo& fmi);
    HRESULT GetFaceNameGDI(CFontMappingInfo& fmi);
    HRESULT GetFaceNameMIME(CFontMappingInfo& fmi);
    HRESULT GetFaceNameRealizeFont(CFontMappingInfo& fmi);
    HRESULT VerifyFaceMap(CFontMappingInfo& fmi);

// Font Mapping Cache
    class CFontMappingCache
    {
        class CFontMappingCacheEntry
        {
            friend class CFontMappingCache;

        protected:
            CFontMappingCacheEntry* m_pPrev;
            CFontMappingCacheEntry* m_pNext;

            int m_nLockCount;

            UINT m_uSrcCodePage;
            LONG m_lSrcHeight; 
            LONG m_lSrcWidth; 
            LONG m_lSrcEscapement; 
            LONG m_lSrcOrientation; 
            LONG m_lSrcWeight; 
            BYTE m_bSrcItalic; 
            BYTE m_bSrcUnderline; 
            BYTE m_bSrcStrikeOut; 
            BYTE m_bSrcPitchAndFamily; 
            TCHAR m_szSrcFaceName[LF_FACESIZE]; 

            HFONT m_hDestFont;
        };

    public:
        CFontMappingCache(void);
        ~CFontMappingCache(void);
        HRESULT FindEntry(UINT uCodePage, const LOGFONT& lfSrcFont, HFONT* phDestFont);
        HRESULT UnlockEntry(HFONT hDestFont);
        HRESULT AddEntry(UINT uCodePage, const LOGFONT& lfSrcFont, HFONT hDestFont);
        HRESULT FlushEntries(void);

    protected:
        CRITICAL_SECTION m_cs;
        CFontMappingCacheEntry* m_pEntries;
        CFontMappingCacheEntry* m_pFree;
        int m_cEntries;
    };

// Code Page Table Cache
    class CCodePagesCache
    {
    public:
        CCodePagesCache(void);
        ~CCodePagesCache(void);
        inline HRESULT Load(void);
        inline operator PBYTE(void) const;

    protected:
        HRESULT RealLoad(void);

    protected:
        CRITICAL_SECTION m_cs;
        BYTE* m_pbBuf;
    };


// Code Page Table Header
    struct CCodePagesHeader
    {
        DWORD m_dwID;
        DWORD m_dwVersion;
        DWORD m_dwFileSize;
        DWORD m_dwBlockSize;
        DWORD m_dwTableOffset;
        DWORD m_dwReserved;
        BYTE m_abCmdCode[8];
    };

    static CFontMappingCache* m_pFontMappingCache;
    static CCodePagesCache* m_pCodePagesCache;

    // For NT5 system font link
    typedef struct tagFLinkFont {    
        WCHAR   szFaceName[LF_FACESIZE];
        LPWSTR  pmszFaceName;
    } FLINKFONT, *PFLINKFONT;
    
    UINT m_uiFLinkFontNum;
    PFLINKFONT m_pFlinkTable;

    void FreeFlinkTable(void);
    HRESULT CreateNT5FontLinkTable(void);
    HRESULT GetNT5FLinkFontCodePages(HDC hDC, LOGFONTW* plfEnum, DWORD * lpdwCodePages);
    static int CALLBACK GetFontCodePagesEnumFontProcW(const LOGFONTW *lplf, const TEXTMETRICW *lptm, DWORD dwFontType, LPARAM lParam);
    static int CALLBACK VerifyFontSizeEnumFontProc(const LOGFONT *lplf, const TEXTMETRIC *lptm, DWORD dwFontType, LPARAM lParam);
};

class CMultiLanguage2;

class ATL_NO_VTABLE CMLFLink2 :
#ifdef UNIX // Unix VTable isn't portable, we need to use CMultiLanguage 
    public CComTearOffObjectBase<CMultiLanguage>,
#else 
    public CComTearOffObjectBase<CMultiLanguage2>,
#endif
    public IMLangFontLink2
{
    IMLangFontLink * m_pIMLFLnk;

public:
    BEGIN_COM_MAP(CMLFLink2)
        COM_INTERFACE_ENTRY(IMLangFontLink2)
    END_COM_MAP()

    CMLFLink2(void)
    {
        CComCreator< CComPolyObject< CMLFLink > >::CreateInstance( NULL, IID_IMLangFontLink, (void **)&m_pIMLFLnk );
    }

    ~CMLFLink2(void)
    {
        if (m_pIMLFLnk)
        {
            m_pIMLFLnk->Release();
            m_pIMLFLnk = NULL;
        }
    }

// IMLangCodePages
    STDMETHOD(GetCharCodePages)(/*[in]*/ WCHAR chSrc, /*[out]*/ DWORD* pdwCodePages)
    {
        if (m_pIMLFLnk)
            return m_pIMLFLnk->GetCharCodePages(chSrc, pdwCodePages);
        else
            return E_FAIL;
    }
    STDMETHOD(CodePageToCodePages)(/*[in]*/ UINT uCodePage, /*[out]*/ DWORD* pdwCodePages)
    {
        if (m_pIMLFLnk)
            return m_pIMLFLnk->CodePageToCodePages(uCodePage, pdwCodePages);
        else
            return E_FAIL;
    }
    STDMETHOD(CodePagesToCodePage)(/*[in]*/ DWORD dwCodePages, /*[in]*/ UINT uDefaultCodePage, /*[out]*/ UINT* puCodePage)
    {
        if (m_pIMLFLnk)
            return m_pIMLFLnk->CodePagesToCodePage(dwCodePages, uDefaultCodePage, puCodePage);
        else
            return E_FAIL;
    }
// IMLangFontLink
    STDMETHOD(GetFontCodePages)(/*[in]*/ HDC hDC, /*[in]*/ HFONT hFont, /*[out]*/ DWORD* pdwCodePages)
    {
        if (m_pIMLFLnk)
            return m_pIMLFLnk->GetFontCodePages(hDC, hFont, pdwCodePages);
        else
            return E_FAIL;
    }

    STDMETHOD(ReleaseFont)(/*[in]*/ HFONT hFont)
    {
        if (m_pIMLFLnk)
            return m_pIMLFLnk->ReleaseFont(hFont);
        else
            return E_FAIL;
    }

// IMLangFontLink2
    STDMETHOD(ResetFontMapping)(void);
    STDMETHOD(GetStrCodePages)(/*[in, size_is(cchSrc)]*/ const WCHAR* pszSrc, /*[in]*/ long cchSrc, /*[in]*/ DWORD dwPriorityCodePages, /*[out]*/ DWORD* pdwCodePages, /*[out]*/ long* pcchCodePages);
    STDMETHOD(MapFont)(/*[in]*/ HDC hDC, /*[in]*/ DWORD dwCodePages, /*[in]*/ WCHAR chSrc, /*[out]*/ HFONT* pFont);
    STDMETHOD(GetFontUnicodeRanges)(/*[in]*/ HDC hDC, /*[in,out]*/ UINT *puiRanges, /*[out]*/ UNICODERANGE* pUranges);
    STDMETHOD(GetScriptFontInfo)(SCRIPT_ID sid, DWORD dwFlags, UINT *puiFonts, SCRIPTFONTINFO* pScriptFont);
    STDMETHOD(CodePageToScriptID)(UINT uiCodePage, SCRIPT_ID *pSid);    

// Font Mapping Cache2 for MapFont
    class CFontMappingCache2
    {
    protected:
        TCHAR   szFontDataFilePath[MAX_PATH];
    public:
        CFontMappingCache2(void);
        ~CFontMappingCache2(void);
        int fetchCharSet(BYTE *pCharset, int iURange);
        BOOL GetNonCpFontUnicodeRanges(TCHAR *szFontName, int iFontIndex);
        BOOL SetFontScripts(void);
        BOOL SetFontTable(void);
        BOOL GetFontURangeBits(TCHAR *szFontName, DWORD *pdwURange);
        BOOL IsFontUpdated(void);
        HRESULT UnicodeRanges(LPTSTR pszFont, UINT *puiRanges, UNICODERANGE* pURanges);
        HRESULT SetFontUnicodeRanges(void);
        HRESULT MapFontFromCMAP(HDC hDC, WCHAR wchar, HFONT hSrcFont, HFONT *phDestFont);
        HRESULT LoadFontDataFile(void);
        HRESULT SaveFontDataFile(void);
        HRESULT EnsureFontTable(BOOL bUpdateURangeTable);
        static int CALLBACK MapFontExEnumFontProc(const LOGFONT* plfFont, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam);
        static int CALLBACK SetFontScriptsEnumFontProc(const LOGFONT* plfFont, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam);
    };

    static CFontMappingCache2* m_pFontMappingCache2;
};

/////////////////////////////////////////////////////////////////////////////
// CMLFLink inline functions

HRESULT CMLFLink::CCodePagesCache::Load(void)
{
    if (m_pbBuf)
        return S_OK;
    else
        return RealLoad();
}

CMLFLink::CCodePagesCache::operator PBYTE(void) const
{
    return m_pbBuf;
}


#endif //__MLFLINK_H_
