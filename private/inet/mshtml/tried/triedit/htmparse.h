// HtmParse.h : Declaration of the CHtmParse
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __HTMPARSE_H_
#define __HTMPARSE_H_

#include "resource.h"       // main symbols
#include "guids.h"
#include "lexhtml.h"
#include "token.h"

#define tokClsIgnore tokclsError // if you don't want to use the token class info in the rule, use this.

#define cbBufPadding 0x800 // we allocate this much extra memory so that subsequent reallocs are saved
#define MIN_TOK 100 // init size for token stack to keep track of nested blocks. e.g. <table>...<table>...</table>...</table>

// init value for number of <TBODY> tags
#define cTBodyInit 20 // init size of nested TBODY's. we start with the assumption that we won't have more than these many nested TBODYs and reallocate if needed.

#define cchspBlockMax 20 // size of spacing block index. we can't have more than 20 digit number
// state flags for space preservation
#define initState   0x0000
#define inChar      0x0001
#define inSpace     0x0002
#define inEOL       0x0003
#define inTab       0x0004
#define inTagOpen   0x0005
#define inTagClose  0x0006
#define inTagEq     0x0007

// used by space preservation in comments
#define chCommentSp '2'
#define chCommentEOL '3'
#define chCommentTab '4'

// Specializations for hrTokenizeAndParse
#define PARSE_SPECIAL_NONE		0x00000000
#define PARSE_SPECIAL_HEAD_ONLY	0x00000001

/////////////////////////////////////////////////////////////////////////////
// CTriEditParse
class ATL_NO_VTABLE CTriEditParse : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTriEditParse, &CLSID_TriEditParse>,
    public ITokenGen
{
public:
    CTriEditParse();
    ~CTriEditParse();

DECLARE_REGISTRY_RESOURCEID(IDR_TRIEDITPARSE)

BEGIN_COM_MAP(CTriEditParse)
    COM_INTERFACE_ENTRY_IID(IID_ITokenGen, ITokenGen)
END_COM_MAP()



// ITokenGen // copied from CColorHtml
public:
    STDMETHOD(NextToken)(LPCWSTR pszText, UINT cbText, UINT* pcbCur, DWORD * pLXS, TXTB* pToken);
    STDMETHOD(hrTokenizeAndParse)(HGLOBAL hOld, HGLOBAL *phNew, IStream *pStmNew, DWORD dwFlags, FilterMode mode, int cbSizeIn, UINT* pcbSizeOut, IUnknown* pUnkTrident, HGLOBAL *phgTokArray, UINT *pcMaxToken, HGLOBAL *phgDocRestore, BSTR bstrBaseURL, DWORD dwReserved);

private:
    static long m_bInit;
    PSUBLANG    m_rgSublang;

    IUnknown *m_pUnkTrident; // we cache it in hrTokenizeAndParse()
    HGLOBAL m_hgDocRestore; // we cache it in hrTokenizeAndParse()
    LPWSTR m_bstrBaseURL;

    // flag used to remember if we have a unicode file that has 0xff,0xfe at the begining
    BOOL m_fUnicodeFile;

    // following m_c's keep track of number of respective tags found
    // during parsing. e.g. m_cHtml will keep track of count of 
    // <html> tags
    INT m_cHtml;
    INT m_cDTC;
    INT m_cObj;
    INT m_cSSSIn;
    INT m_cSSSOut;
    INT m_cNbsp;
    INT m_cHdr;
    INT m_cFtr;
    INT m_cObjIn;
    INT m_cComment;
    INT m_cAImgLink;

    UINT m_cMaxToken;       // Max of token array (pTokArray)
    BOOL m_fEndTagFound;    // end tag found
    INT  m_iControl;        // index in applet collection
    BOOL m_fSpecialSSS;     // found special SSS <%@....%>

    // used to save space preservation info
    HGLOBAL m_hgspInfo;
    WORD *m_pspInfo;
    WORD *m_pspInfoOut;
    WORD *m_pspInfoOutStart;
    WORD *m_pspInfoCur;
    UINT m_ichStartSP;          // save all prev spacing info at this ich
    INT m_ispInfoBase;
    INT m_ispInfoIn;
    INT m_ispInfoOut;
    INT m_iArrayspLast;
    INT m_ispInfoBlock;
    INT m_cchspInfoTotal;
    BOOL m_fDontDeccItem;       // we don't have counters for items that we don't process, so we use this to preserve the total count
    
    // used by <TBODY> code.
    // Trident puts in extra <tbody></tbody> tags inside table
    // and filtering tries to remove them.
    HGLOBAL m_hgTBodyStack;
    UINT *m_pTBodyStack;
    INT m_iMaxTBody;
    INT m_iTBodyMax;

    // used by Page Transition DTC code
    // page transition dtc is a special case in filtering because 
    // we have to maintain its location inside the head section.
    BOOL m_fInHdrIn;
    INT m_cchPTDTCObj;
    INT m_ichPTDTC;
    INT m_cchPTDTC;
    INT m_indexBeginBody;
    INT m_indexEndBody;
    WCHAR *m_pPTDTC;
    HGLOBAL m_hgPTDTC;

    // used by the code that recreates our own pre-Body part of the document
    BOOL m_fHasTitleIn;
    INT m_indexTitleIn;
    INT m_ichTitleIn;
    INT m_cchTitleIn;
    INT m_ichBeginBodyTagIn;
    INT m_indexHttpEquivIn;
    INT m_ichBeginHeadTagIn;

    // used by APPLET pretty-printing code
    int m_cAppletIn;
    int m_cAppletOut;

    // used to keep track of multiple occurances of BODY, HTML, TITLE & HEAD tags
    int m_cBodyTags;
    int m_cHtmlTags;
    int m_cTitleTags;
    int m_cHeadTags;

    void SetTable(DWORD lxs);
    void InitSublanguages();
    void PreProcessToken(TOKSTRUCT *pTokArray, INT *pitokCur, LPWSTR pszText, UINT cbCur, TXTB token, DWORD lxs, INT tagID, FilterMode mode);
    void PostProcessToken(OLECHAR *pwOld, OLECHAR *pwNew, UINT *pcbNew, UINT cbCur, UINT cbCurSav, TXTB token, FilterMode mode, DWORD lxs, DWORD dwFlags);
    int ValidateTag(LPWSTR pszText);
    int GetTagID(LPWSTR pszText, TXTB token);
    HRESULT hrMarkSpacing(WCHAR *pwOld, UINT cbCur, INT *pchStartSP);
    void SetSPInfoState(WORD inState, WORD *pdwState, WORD *pdwStatePrev, BOOL *pfSave);
    BOOL FRestoreSpacing(LPWSTR pwNew, LPWSTR pwOld, UINT *pichNewCur, INT *pcchwspInfo, INT cchRange, INT ichtoktagStart, BOOL fLookback, INT index);
    HRESULT hrMarkOrdering(WCHAR *pwOld, TOKSTRUCT *pTokArray, INT iArrayStart, INT iArrayEnd, UINT cbCur, INT *pichStartOR);
    BOOL FRestoreOrder(WCHAR *pwNew, WCHAR *pwOld, WORD *pspInfoOrder, UINT *pichNewCur, INT cwOrderInfo, TOKSTRUCT *pTokArray, INT iArrayStart, INT iArrayEnd, INT iArrayDSPStart, INT iArrayDSPEnd, INT cchNewCopy, HGLOBAL *phgNew);
    void SaveSpacingSpecial(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR *ppwNew, HGLOBAL *phgNew, TOKSTRUCT *pTokArray, INT iArray, UINT *pichNewCur);
    void RestoreSpacingSpecial(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR *ppwNew, HGLOBAL *phgNew, TOKSTRUCT *pTokArray, UINT iArray, UINT *pichNewCur);

    HRESULT ProcessToken(DWORD &lxs, TXTB &tok, LPWSTR pszText, UINT cbCur, TOKSTACK *pTokStack, INT *pitokTop, TOKSTRUCT *pTokArray, INT iArrayPos, INT tagID);
    void FilterHtml(LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, TOKSTRUCT *pTokArray, FilterMode mode, DWORD dwFlags);


    struct FilterTok
    {
        TOKEN tokBegin;
        TOKEN tokBegin2; // supporting token
        TOKEN tokClsBegin;
        TOKEN tokEnd;
        TOKEN tokEnd2; // supporting token
        TOKEN tokClsEnd;
    };

    typedef  void (_stdcall* PFNACTION)(CTriEditParse *, LPWSTR, LPWSTR *, UINT *, HGLOBAL *, TOKSTRUCT *, UINT*, FilterTok, INT*, UINT*, UINT*, DWORD);
    struct FilterRule
    {
        FilterTok ft;
        PFNACTION pfn;
    };

    // Following are static functions. We could make them members, but it wasn't felt necessary then.
    void static fnRestoreDTC(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveDTC(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);

    void static fnRestoreSSS(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveSSS(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);

    void static fnRestoreHtmlTag(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveHtmlTag(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveNBSP(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy,
              DWORD dwFlags);
    void static fnRestoreNBSP(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy,
              DWORD dwFlags);
    void static fnSaveHdr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy,
              DWORD dwFlags);
    void static fnRestoreHdr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy,
              DWORD dwFlags);
    void static fnSaveFtr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy,
              DWORD dwFlags);
    void static fnRestoreFtr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy,
              DWORD dwFlags);
    void static fnRestoreSpace(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveSpace(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreSpaceEnd(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreObject(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveObject(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreTbody(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveTbody(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveApplet(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreApplet(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveAImgLink(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreAImgLink(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveComment(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreComment(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnSaveTextArea(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);
    void static fnRestoreTextArea(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags);

    #define cRuleMax 26 /* max number of filtering rules. if you add a new rule above, change this too */
    FilterRule m_FilterRule[cRuleMax];

};


#endif //__HTMPARSE_H_
