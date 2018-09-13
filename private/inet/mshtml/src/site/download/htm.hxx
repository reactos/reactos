//+ ---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1995-1997
//
//  File:       htm.hxx
//
//  Contents:   CHtmInfo
//              CHtmLoad
//              CHtmPre
//              CHtmPost
//
// ----------------------------------------------------------------------------

#ifndef I_HTM_HXX_
#define I_HTM_HXX_
#pragma INCMSG("--- Beg 'htm.hxx'")

#ifndef X_HTMVER_HXX_
#define X_HTMVER_HXX_
#include "htmver.hxx"
#endif

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif


// Forward --------------------------------------------------------------------

#ifdef VSTUDIO7
class  CTagsource;
#endif //VSTUDIO7
class  CHtmInfo;
class  CHtmLoad;
class  CHtmPre;
class  CHtmPost;
class  CHtmlParseClass;
class  CFrameSetSite;
class  CTitleElement;
class  CTextArea;
class  CTable;
class  CVersions;
class  CBuffer2;

MtExtern(CHtmParse)
MtExtern(CHtmParse_aryContextStack_pv)
MtExtern(CHtmParse_aryPelEndParse_pv)
MtExtern(CHtmParse_aryPelFrontier_pv)
MtExtern(CHtmParse_aryFccl_pv)
MtExtern(CHtmPost)
MtExtern(CHtmPost_aryphtSaved_pv)
MtExtern(CHtmPre)
MtExtern(CHtmPre_aryCchSaved_pv)
MtExtern(CHtmPre_aryCchAsciiSaved_pv)
MtExtern(CHtmPre_aryInsert_pv)
#ifdef VSTUDIO7
MtExtern(CTagsource)
#endif //VSTUDIO7
MtExtern(CHtmInfo)
#ifdef VSTUDIO7
MtExtern(CHtmInfo_aryTagsource_pv)
#endif //VSTUDIO7
MtExtern(CHtmLoad)
MtExtern(CHtmLoad_aryDwnCtx_pv)

//+------------------------------------------------------------------------
//
//  Character classes
//
//  Synopsis:   Sets of characters distinguished (compat. Netscape)
//
//-------------------------------------------------------------------------

// no class includes \0 (to allow fast end-of-buffer detection)
// no class includes \r (to allow fast line counting)

#define ISTXTCH(ch) (((ch) > _T('<')) || g_charclass[ch] & CCF_TXTCH)
#define ISNONSP(ch) (((ch) > _T(' ')) || g_charclass[ch] & CCF_NONSP)
#define ISNAMCH(ch) (((ch) > _T('>')) || g_charclass[ch] & CCF_NAMCH)
#define ISATTCH(ch) (((ch) > _T('>')) || g_charclass[ch] & CCF_ATTCH)
#define ISVALCH(ch) (((ch) > _T('>')) || g_charclass[ch] & CCF_VALCH)
#define ISUPPER(ch) (((unsigned)(ch) - _T('A')) <= _T('Z')-_T('A'))
#define ISALPHA(ch) (((unsigned)((ch) & ~(_T('a') ^ _T('A'))) - _T('A')) <= _T('Z')-_T('A'))
#define ISDIGIT(ch) (((unsigned)((ch) - _T('0'))) <= _T('9') - _T('0'))
#define ISSPACR(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 12 - 9)
#define ISNDASH(ch) (((ch) > _T('-')) || g_charclass[ch] & CCF_NDASH)
#define ISDASHC(ch) (((ch) == _T('-')))
#define ISQUOTE(ch) (((ch) == _T('"')) || ((ch) == _T('\'')) || ((ch) == _T('`')))
#define ISENTYC(ch) (ISALPHA(ch) || ISDIGIT(ch))
#define ISTAGCH(ch) (((ch) > _T('>')) ? ((ch) != _T('`')) : (g_charclass[ch] & CCF_TAGCH))
#define ISMRKCH(ch) (((ch) > _T('>')) || g_charclass[ch] & CCF_MRKCH)
#define ISHEXAL(ch) (((unsigned)((ch) & ~(_T('a') ^ _T('A'))) - _T('A')) <= _T('F')-_T('A'))
#define ISHEX(ch)   (ISDIGIT(ch) || ISHEXAL(ch))

// this class contains \r (for whitespace/nonspace breaking in OutputText)
#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)

#define MAXENTYLEN  10

// HTML 1.0 defined entities all fall below 255; newer ones all appear above.
#define IS_HTML1_ENTITY_CHAR(chEnt) (chEnt <= 255)

// five char classes which are faster to look up than to compute
#define CCF_TXTCH 0x01
#define CCF_NONSP 0x02
#define CCF_NAMCH 0x04
#define CCF_VALCH 0x08
#define CCF_ATTCH 0x10
#define CCF_NDASH 0x20
#define CCF_TAGCH 0x40
#define CCF_MRKCH 0x80

#define XML_PI _T("xml:namespace")

extern const BYTE g_charclass[64];

#ifndef NO_UTF16
typedef DWORD XCHAR;
#else
typedef TCHAR XCHAR;
#endif

#if DBG==1
inline
BOOL IsAllSpaces(const TCHAR * pch, ULONG cch)
{
    for (; cch > 0; --cch, ++pch)
        if (ISNONSP(*pch))
            return(FALSE);
    return(TRUE);
}
#endif

// PostMan --------------------------------------------------------------------

void    PostManEnqueue(CHtmPost * pHtmPost);
void    PostManSuspend(CHtmPost * pHtmPost);
void    PostManResume(CHtmPost * pHtmPost, BOOL fExecute);
void    PostManDequeue(CHtmPost * pHtmPost);

// CHtmLoad ----------------------------------------------------------------

class CHtmLoad : public CDwnLoad
{
    typedef CDwnLoad super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmLoad))

    // CDwnLoad methods

    virtual void        Passivate();
    virtual HRESULT     Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo);
    virtual HRESULT     OnBindRedirect(LPCTSTR pchRedirect, LPCTSTR pchMethod);
    virtual HRESULT     OnBindHeaders();
    virtual HRESULT     OnBindMime(MIMEINFO * pmi);
    virtual HRESULT     OnBindData();
    virtual void        OnBindDone(HRESULT hrErr);

    // CHtmLoad methods

    HRESULT             AddDwnCtx(UINT dt, CDwnCtx * pDwnCtx);
    CDwnCtx *           GetDwnCtx(UINT dt, LPCTSTR pch);

    void                FinishSyncLoad();
    
    HRESULT             Write(LPCTSTR pchText, BOOL fParseNow);
    HRESULT             Close();

    HRESULT             OnRedirect(LPCTSTR pchUrl);

    HRESULT             OnPostStart();
    void                OnPostDone(HRESULT hrErr);
    void                OnPostFinishScript();
    HRESULT             OnPostRestart(CODEPAGE codepage);
    void                DoStop();

    LPCTSTR             GetUrl() { return(_cstrUrlDef ? _cstrUrlDef : g_Zero.ach); }
    BOOL                IsRunningScript();
    void                Sleep(BOOL fSleep, BOOL fExecute);

    CHtmInfo *          GetHtmInfo() { return((CHtmInfo *)_pDwnInfo); }
    CHtmPre *           GetHtmPreAsync();

    DWORD               GetSecFlags() { return _pDwnInfo->GetSecFlags(); }
    void                GiveRawEcho(BYTE **ppb, ULONG *pcb);
    void                GiveSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci);

#ifdef XMV_PARSE
    void                SetGenericParse(BOOL fDoGeneric);
#endif

    // Data members

    DECLARE_CPtrAry(CAryDwnCtx, CDwnCtx *, Mt(Mem), Mt(CHtmLoad_aryDwnCtx_pv))

    BOOL                _fPasting;
    MIMEINFO *          _pmi;
    FILETIME            _ftHistory;
    CDoc *              _pDoc;
    CMarkup *           _pMarkup;
    CHtmPre *           _pHtmPre;
    CHtmPost *          _pHtmPost;
    CStr                _cstrUrlDef;
    CStr                _cstrRedirect;
    CStr                _cstrMethod;
    CStr                _cstrRefresh;
    BYTE *              _pbRawEcho;
    ULONG               _cbRawEcho;
    INTERNET_SECURITY_CONNECTION_INFO *_pSecConInfo;
    CAryDwnCtx          _aryDwnCtx[DWNCTX_MAX];
    int                 _iDwnCtxFirst[DWNCTX_MAX];
    BOOL                _fLoadHistory:1;
    BOOL                _fImageFile:1;

};

#ifdef VSTUDIO7
// CTagsource -------------------------------------------------------------------

class CTagsource
{
public:
    CStr           _cstrTagName;
    ELEMENT_TAG    _etagBase;
    CStr           _cstrCodebase;
};
#endif //VSTUDIO7

// CHtmInfo -------------------------------------------------------------------

#ifdef VSTUDIO7
DECLARE_CDataAry(CAryTagsource, CTagsource, Mt(Mem), Mt(CHtmInfo_aryTagsource_pv))
#endif //VSTUDIO7

class CHtmInfo : public CDwnInfo
{
    typedef CDwnInfo super;
    friend class CHtmCtx;
    friend class CHtmLoad;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmInfo))

    virtual ~CHtmInfo();
    
    // CBaseFT methods

    virtual ULONG       Release();
    virtual void        Passivate();

    // CDwnInfo methods

    virtual UINT        GetType()           { return(DWNCTX_HTM); }
    virtual HRESULT     Init(DWNLOADINFO * pdli);
    virtual HRESULT     NewDwnCtx(CDwnCtx ** ppDwnCtx);
    virtual HRESULT     NewDwnLoad(CDwnLoad ** ppDwnLoad);
    virtual HRESULT     GetFile(LPTSTR * ppch);
    virtual DWORD       GetProgSinkClass()  { return _dwClass; }

    // CHtmInfo methods

    CHtmCtx *           GetHtmCtx() { return((CHtmCtx *)_pDwnCtxHead); }
    CHtmLoad *          GetHtmLoad() { return((CHtmLoad *)_pDwnLoad); }

    BOOL                IsLoading()         { return(!!GetHtmLoad()); }
    BOOL                IsOpened()          { return(_fOpened); }
    BOOL                WasOpened()         { return(_fWasOpened); }
    BOOL                IsHttp449()         { return(!!_pbRawEcho); }
    void                SetMimeFilter(BOOL f) { _fMimeFilter = !!f; }
    BOOL                FromMimeFilter()    { return(_fMimeFilter); }
    HRESULT             GetBindResult()     { return(_hrBind); }
    BOOL                IsRunningScript()   { return(GetHtmLoad() && GetHtmLoad()->IsRunningScript()); }
    HRESULT             Write(LPCTSTR pch, BOOL f)  { return(GetHtmLoad() ? GetHtmLoad()->Write(pch, f) : S_OK); }
    void                Close()             { Assert(_fOpened); GetHtmLoad()->Close(); _fOpened = FALSE; }
    void                Sleep(BOOL fSleep, BOOL fExecute)  { if (GetHtmLoad()) {GetHtmLoad()->Sleep(fSleep, fExecute);} }
    void                SetDocCodePage(CODEPAGE cp);
    CDwnCtx *           GetDwnCtx(UINT dt, LPCTSTR pch);
    void                DoStop();
    void                TakeErrorString(TCHAR **ppchError);
    void                TakeRawEcho(BYTE **ppb, ULONG *pcb);
    void                GetRawEcho(BYTE **ppb, ULONG *pcb);
    void                TakeSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci);
    void                GetSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci);

    void                OnBindResult(HRESULT hr);
    void                OnLoadDone(HRESULT hr);
    HRESULT             OnLoadFile(LPCTSTR pchFile, HANDLE * phLock, BOOL fPretransform);
    HRESULT             OnSource(BYTE * pb, ULONG cb);

    HRESULT             OpenSource(DWORD dwFlags = 0);
    HRESULT             ReadSource(BYTE * pb, ULONG ib, ULONG cb, ULONG * pcb, DWORD dwFlags = 0);
    void                CloseSource();
    HRESULT             DecodeSource();

    HRESULT             CopyOriginalSource(IStream * pstm, DWORD dwFlags);
    HRESULT             ReadUnicodeSource(TCHAR * pch, ULONG ich, ULONG cch,
                            ULONG * pcch);

    TCHAR *             GetFailureUrl()     { return(_cstrFailureUrl); }
    BOOL                IsKeepRefresh()     { return(_fKeepRefresh); }
    IStream *           GetRefreshStream()  { return(_pstmRefresh); }
    TCHAR *             GetErrorString()    { return(_pchError); }
    
#ifdef XMV_PARSE
    void                SetGenericParse(BOOL fDoGeneric);
#endif

    HRESULT             GetPretransformedFile(LPTSTR * ppch);

#ifdef VSTUDIO7
    HRESULT             AddTagsources(LPTSTR *rgstrTag, ELEMENT_TAG *rgetagBase, LPTSTR *rgstrCodebase, INT cTags);
    HRESULT             AddTagsource(LPTSTR pchTag, ELEMENT_TAG etagBase, LPTSTR pchCodebase);
    CTagsource *        GetTagsource(LPTSTR pchTagName);
#endif //VSTUDIO7

private:

    // Data members

    unsigned            _fOpened    :1; // is currently inside document.open/document.close
    unsigned            _fWasOpened :1; // was generated by document.open
    unsigned            _fKeepRefresh:1;// keep _pstmRefresh because it's an error doc
    unsigned            _fMimeFilter:1; // contents was from an urlmon mime filter
    DWORD               _dwClass;       // ProgSink class
    CStr                _cstrFile;      // Path to source (file or cache)
    HANDLE              _hLock;         // HTTP cache file locking handle
    IUnknown *          _pUnkLock;      // IUnknown which locks cache file
    IStream *           _pstmFile;      // IStream to opened source file
    BYTE *              _pbSrc;         // The source if not in file and small
    ULONG               _cbBuf;         // Allocated size of source buffer
    CDwnStm *           _pstmSrc;       // The source if not in file and big
    BOOL                _fUniSrc;       // The source is in UNICODE
    CODEPAGE            _cpDoc;         // CodePage of the source
    ULONG               _cbSrc;         // Total bytes of source read so far
    TCHAR *             _pchDecoded;    // Unicode normalized source
    ULONG               _cchDecoded;    // Unicode normalized source size
    ULONG               _cbDecoded;     // Size of source when last decoded
    CStr                _cstrFailureUrl;// Url to display in case of failure
    IStream *           _pstmRefresh;   // History stream for refresh after failure
    HRESULT             _hrBind;        // HRESULT from bind
    TCHAR *             _pchError;      // Parsing error message
    ULONG               _cbRawEcho;     // Length of the following field
    BYTE *              _pbRawEcho;     // Raw headers for 449 echo (ascii)
    INTERNET_SECURITY_CONNECTION_INFO *_pSecConInfo;  // Wininet security information structure
#ifdef VSTUDIO7
    CAryTagsource       _aryTagsource; // Tagsource table.
#endif //VSTUDIO7
    CStr                _cstrPretransformedFile; // Path to original pre-transformed source
                                                 // (for XML Mimeviewer: the xml file path)
};

// CHtmTagStm -----------------------------------------------------------------

MtExtern(CHtmTagStm)

class CHtmTagStm : public CDwnChan
{

public:

    typedef CDwnChan super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmTagStm))
    
               ~CHtmTagStm();

    HRESULT     AllocTextBuffer(UINT cch, TCHAR ** ppch);
    HRESULT     GrowTextBuffer(UINT cch, TCHAR ** ppch);
    void        DequeueTextBuffer();

    HRESULT     AllocTagBuffer(UINT cbNeed, void * pvCopy, UINT cbCopy);
    CHtmTag *   DequeueTagBuffer();

    HRESULT     WriteTag(ELEMENT_TAG etag, TCHAR * pch, ULONG cch, BOOL fAscii);
    HRESULT     WriteTag(ELEMENT_TAG etag, ULONG ul1 = 0, ULONG ul2 = 0);
    HRESULT     WriteTagBeg(ELEMENT_TAG etag, CHtmTag ** ppht);
    HRESULT     WriteTagGrow(CHtmTag ** ppht, CHtmTag::CAttr ** ppAttr);
    void        WriteTagEnd();
    void        WriteEof(HRESULT hrEof);
    HRESULT     WriteSource(TCHAR *pch, ULONG cch);

    CHtmTag *   ReadTag(CHtmTag * pht);
    HRESULT     ReadSource(CBuffer2 *pBuf, ULONG cch);
    HRESULT     SkipSource(ULONG cch);

    void        Signal();

    BOOL        IsPending()         { return(_chtRead == _chtWrite && !_fEof); }
    BOOL        IsEof()             { return(_fEof && (_hrEof || (_chtRead == _chtWrite))); }
    BOOL        IsEofWritten()      { return(_fEof); }

#ifdef XMV_PARSE
    ULONG       TagsWritten()       { return (_chtWrite); }
#endif

#if DBG==1 || defined(PERFTAGS)
    virtual char *  GetOnMethodCallName() { return("CHtmTagStm::OnMethodCall"); }
#endif

private:

    struct TEXTBUF
    {
        TEXTBUF *   ptextbufNext;       // Next text buffer
        TCHAR       ach[1];             // The text bytes
    };

    struct TAGBUF
    {
        TAGBUF *    ptagbufNext;        // Next tag buffer
        CHtmTag *   phtWrite;           // Place to write next tag
        BYTE        ab[1];              // The tag bytes
    };

    TEXTBUF *       _ptextbufHead;      // First text buffer in queue
    TEXTBUF *       _ptextbufTail;      // Last text buffer in queue
    TEXTBUF *       _ptextbufWrite;     // Text buffer being written (not in queue yet)
    TAGBUF *        _ptagbufHead;       // First tag buffer in queue
    TAGBUF *        _ptagbufTail;       // Last tag buffer in queue
    CHtmTag *       _phtRead;           // Current tag being read in _ptagbufHead
    ULONG           _chtRead;           // Count of tags read
    ULONG           _chtWrite;          // Count of tags written
    ULONG           _cbLeft;            // Space left in tag buffer for writing
    BOOL            _fEof;              // Async flag (don't combine!)
    HRESULT         _hrEof;             // Error to return at EOF
    BOOL            _fSignal;           // Signal is needed
    BOOL            _fNextBuffer;       // Set TAGF_NEXTBUF bit on next tag
    UINT            _cbTagBuffer;       // Size of next tag buffer
    CDwnStm *       _pdsSource;         // Extra dwnstm for echoing source when needed
};

MtExtern(CHtmTagQueue)

class CHtmTagQueue : public CHtmTagStm
{
public:
    typedef CHtmTagStm super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmTagQueue));
    
    HRESULT EnqueueTag(CHtmTag *pht);
    CHtmTag *DequeueTag();
    HRESULT ParseAndEnqueueTag(TCHAR *pch, ULONG cch);
    BOOL IsEmpty() { return !_cEnqueued; }

    ULONG _cEnqueued;

#if DBG==1 || defined(PERFTAGS)
    virtual char *  GetOnMethodCallName() { return("CHtmTagQueue::OnMethodCall"); }
#endif

};


// FindContainer cache -------------------------------------------------------

#define FCC_WIDTH  4

class CFccItem
{
public:
    ELEMENT_TAG *_pset1;
    ELEMENT_TAG *_pset2;
    CTreeNode *_pNodeOut;
};


class CFccLine
{
public:
    CTreeNode *_pNodeCheck;
    ULONG _cCached;
    ULONG _iFirst;
    CFccItem _afcci[FCC_WIDTH];
};

// CHtmParse ------------------------------------------------------------------

class CHtmParse : public CVoid
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmParse))
    
    CHtmParse() : _aryContextStack(Mt(CHtmParse_aryContextStack_pv)), _aryPelEndParse(Mt(CHtmParse_aryPelEndParse_pv)),
                  _aryPelFrontier(Mt(CHtmParse_aryPelFrontier_pv)), _aryFccl(Mt(CHtmParse_aryFccl_pv)) {};
    ~CHtmParse();

    HRESULT Init(CDoc *pDoc, CMarkup *pMarkup, CTreeNode *pNode);
    HRESULT BUGCALL Prepare();
    HRESULT BUGCALL Commit();
    HRESULT BUGCALL Execute();
    HRESULT BUGCALL Finish();
    BOOL    NeedExecute() { return _phpxExecute || _pMergeTagQueue || _aryPelEndParse.Size(); }
    HRESULT ParseToken(CHtmTag *pht);

    void GetPointers(CTreePos **pptpL, CTreePos **pptpR);

    CElement *GetCurrentElement();

    HRESULT BUGCALL Die();

private:

    HRESULT ParseText(TCHAR * pch, ULONG cch, BOOL fAscii);
    HRESULT ParseEof();
    HRESULT ParseBeginTag(CHtmTag *pht);
    HRESULT ParseEndTag(CHtmTag *pht);
    HRESULT ParseUnknownTag(CHtmTag *pht);
    HRESULT ParseIgnoredTag(CHtmTag *pht);
    HRESULT ParseSource(CHtmTag *pht);
    HRESULT ParseParam(CHtmTag *pht);
    HRESULT ParseComment(CHtmTag *pht);
    HRESULT ParseMarker(CHtmTag *pht);
    HRESULT ParseTextFrag(CHtmTag *pht);
    HRESULT RequestMergeTag(CHtmTag *pht);
    HRESULT MergeTags();
    HRESULT QueueTag(CHtmTag *pht);
    HRESULT CloseContainer(CTreeNode *pNodeContainer, BOOL fExplicit);
    HRESULT CloseAllContainers(ELEMENT_TAG *atagClose, ELEMENT_TAG *atagStop);
    HRESULT OpenContainer(ELEMENT_TAG etag);
    HRESULT PushHpx(HRESULT (*phpc)(CHtmParseCtx **pphpx, CElement *pelTop, CHtmParseCtx *phpxParent), CElement *pel, CTreeNode *pNode);
    HRESULT PopHpx();
    HRESULT AddImplicitChildren(ELEMENT_TAG etagNext);
    HRESULT InsertLPointer();
    HRESULT InsertRPointer();
    HRESULT InsertLPointerNow();

    enum PREPARE_CODE
    {
        PREPARE_NORMAL = 0,
        PREPARE_UNKNOWN,
        PREPARE_MERGE,
        PREPARE_QUEUE,
    };
    
    HRESULT PrepareContainer(ELEMENT_TAG etag, CHtmTag *pht, PREPARE_CODE *pcode);
    HRESULT BeginElement(CElement *pel, BOOL fExplicit, BOOL fEndTag = FALSE);
    HRESULT EndElement(CTreeNode *pNodeEnd);
    ELEMENT_TAG RequiredTextContainer();
    CTreeNode *FindContainer(ELEMENT_TAG *pSetMatch, ELEMENT_TAG *pSetStop);
    CTreeNode *FindContainer(ELEMENT_TAG etagMatch, ELEMENT_TAG *pSetStop);
    CTreeNode *FindGenericContainer(CHtmTag *pht, ELEMENT_TAG *pSetStop);
    BOOL FindContainerCache(ULONG iCache, CTreeNode *pNodeCheck, ELEMENT_TAG *pset1, ELEMENT_TAG *pset2, CTreeNode **ppNodeOut);
    void SetContainerCache(ULONG iCache, CTreeNode *pNodeCheck, ELEMENT_TAG *pset1, ELEMENT_TAG *pset2, CTreeNode *pNodeOut);
    
#if DBG == 1
    void AssertNoneProhibitedImpl(ELEMENT_TAG etag);
    void AssertAllRequiredImpl(ELEMENT_TAG etag);
    void AssertNoEndContaineesImpl(CTreeNode *pNode);
    void AssertOnStackImpl(CTreeNode *pNode);
    void AssertInsideContextImpl(CTreeNode *pNode);
#endif

private:

    // current tree node
    CTreeNode * _pNode;

    CMarkup *   _pMarkup;
    BOOL        _fDone;
    BOOL        _fIgnoreInput;

    CDoc      * _pDoc;

    // stack of contexts
    struct CContext
    {
        CHtmParseCtx *_phpx;
        CElement * _pelTop;
    };
    CStackDataAry <CContext,10> _aryContextStack;
    CContext _ctx;

    // literal context tag (TITLE)
    ELEMENT_TAG _etagLitCtx;

    // Tags that need to be merged into the specified element at Execute
    CHtmTagQueue *_pMergeTagQueue;
    CElement *_pelMerge;

    // queued tags and the containers needed before they're dequeued
    ELEMENT_TAG _etagReplay;
    CHtmTagQueue *_pTagQueue;
    
    // context needing Execute
    CHtmParseCtx *_phpxExecute;

    // elements needing notification of parse-done
    CPtrAry <CElement *> _aryPelEndParse;

    // required text container
    BOOL _fValidRTC;
    ELEMENT_TAG _etagRTC;

    // implicit child needed
    BOOL _fImplicitChild;

    // pointer poses
    CTreePos *_ptpL;
    CTreePos *_ptpR;
    BOOL _fDelayPointer;
    
    // stop everything
    BOOL _fDie;

    // frontier (held between commit+prepare): ptr and nodes
    CPtrAry<CElement *> _aryPelFrontier;
    LONG _cDepth;
    LONG _lVersionSafe;

    // FindContainer cache
    CDataAry<CFccLine> _aryFccl;
};

HRESULT ValidateElementChain(
    CElement*  pelSourceBottom, 
    CElement*  pelSourceTop, 
    CElement*  pelTargetBottom,
    CElement** ppelFailBottom,
    CElement** ppelFailTop);

// CHtmPost -------------------------------------------------------------------

class CHtmPost : public CBaseFT
{
    typedef CBaseFT super;
    friend class CHtmLoad;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmPost))

    // CBaseFT methods

    virtual void        Passivate();

    // CHtmPost methods

    HRESULT             Init(CHtmLoad *pHtmLoad, CHtmTagStm * pHtmTagStm, CDoc *pDoc, CMarkup * pMarkup, HTMPASTEINFO * phpi, BOOL fSync);
    void                Run(DWORD dwTimeout);
    HRESULT             Exec(DWORD dwTimeout);
    HRESULT             RunNested();

    BOOL                IsPending();
    BOOL                IsAtEof();
    BOOL                IsDone();

    LPCTSTR             GetUrl() { return(_pHtmLoad ? _pHtmLoad->GetUrl() : g_Zero.ach); }

    void                Die();      // "hard" stop
    void                DoStop();   // "soft" stop

    ELEMENT_TAG         IsGenericElement          (CHtmTag * pht);
    BOOL                IsXmlPI                   (CHtmTag * pht);

    void                RegisterXmlPI             (CHtmTag * pht);
    void                RegisterHtmlTagNamespaces (CHtmTag * pht);

protected:


    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg);

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    void                OnIncludeDwnChan(CBitsCtx * pBitxCtx);
    static void CALLBACK OnIncludeDwnChanCallback(void * pvObj, void * pvArg)
        { ((CHtmPost *) pvArg)->OnIncludeDwnChan((CBitsCtx *)pvObj); }
#endif

private:

    HRESULT ProcessTokens(DWORD dwTimeout);
    HRESULT ParseToken(CHtmTag *pht);
    HRESULT Broadcast(HRESULT (BUGCALL CHtmParse::*pfnAction)());
    HRESULT IsIndexHack(CHtmTag *pht);
#ifdef VSTUDIO7
    HRESULT ProcessFactoryPI(CHtmTag *pht);
#endif //VSTUDIO7

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    HRESULT ProcessIncludeToken(CHtmTag *pht);
#endif

public:

    // top data
    DWORD               _dwFlags;
    CHtmLoad *          _pHtmLoad;
    CHtmTagStm *        _pHtmTagStm;
    CHtmParse *         _pHtmParse;
    CDoc *              _pDoc;
    CMarkup *           _pMarkup;
    CTreeNode *         _pNodeRoot;
    DWORD               _dwCompat;
    TCHAR *             _pchError;
    CHtmPost *          _pHtmPostNext;
    CODEPAGE            _cpRestart;
    HTMPASTEINFO *      _phpi;

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    CBitsCtx *          _pBitsCtxInclude;
    DWORD               _dwIncludeDownloadCookie;
    ULONG               _ulIncludes;
#endif
};

enum
{
    POSTF_DIE                   = 0x00000001,
    POSTF_STOP                  = 0x00000002,
    POSTF_WAIT                  = 0x00000004,
    POSTF_SLEEP                 = 0x00000008,
    POSTF_NEED_EXECUTE          = 0x00000010,
    POSTF_RESUME_PREPARSER      = 0x00000020,
    POSTF_IN_FORM               = 0x00000040,
    POSTF_RUNNING               = 0x00000080,
    POSTF_BLOCKED               = 0x00000100,
    POSTF_ENQUEUED              = 0x00000200,
    POSTF_PASTING               = 0x00000400,
    POSTF_ONETIME               = 0x00000800,
    POSTF_DONTWAITFORINPLACE    = 0x00001000,
    POSTF_RESTART               = 0x00002000,
    POSTF_NESTED                = 0x00004000,
    POSTF_SEENFRAMESET          = 0x00008000,
#ifdef VSTUDIO7
    POSTF_NEED_BASETAGS         = 0x00010000,
#endif //VSTUDIO7
};

// CHtmPre --------------------------------------------------------------------

// These codes are used to define inserts which inject tokens into the
// output stream after the tokenizer has processed up to a given # of bytes
// in the input stream (CHtmPre::AddInsert())
enum TOKENINSERTCODE {
    TIC_BEGINSEL     = 1,
    TIC_ENDSEL       = 2,
    TIC_BEGINFRAG    = 3,
    TIC_ENDFRAG      = 4,
    TOKENINSERTCODE_Last_Enum
};

#ifdef UNIX
typedef DWORD CODE;
#else
typedef WORD CODE;
#endif

class CHtmPre : public CDwnTask, public CEncodeReader
{
    typedef CDwnTask super;
    friend class CHtmLoad;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmPre))

    // CDwnTask methods

    virtual void    Run();

    // CHtmPre methods

                    CHtmPre(CODEPAGE codepage) :
                        CEncodeReader(codepage, 4096),
                        _aryCchSaved(Mt(CHtmPre_aryCchSaved_pv)),
                        _aryInsert(Mt(CHtmPre_aryInsert_pv)),
                        _aryCchAsciiSaved(Mt(CHtmPre_aryCchAsciiSaved_pv))
                        {}
    virtual        ~CHtmPre();
#ifdef XMV_PARSE
    HRESULT         Init(CHtmLoad * pHtmLoad, CDwnDoc * pDwnDoc,
                        IInternetSession * pInetSess, IStream *pstmLeader,
                        CDwnBindData * pDwnBindData, CHtmTagStm * pHtmTagStm,
                        HTMPASTEINFO * phpi, LPCTSTR pchUrl, CVersions *pVersions, BOOL fXML);
#else
    HRESULT         Init(CHtmLoad * pHtmLoad, CDwnDoc * pDwnDoc,
                        IInternetSession * pInetSess, IStream *pstmLeader,
                        CDwnBindData * pDwnBindData, CHtmTagStm * pHtmTagStm,
                        HTMPASTEINFO * phpi, LPCTSTR pchUrl, CVersions *pVersions);
#endif
    HRESULT         Exec();

    HRESULT         OnRedirect(LPCTSTR pchUrl);

    LPCTSTR         GetUrl() { return(_pHtmLoad ? _pHtmLoad->GetUrl() : g_Zero.ach); }

    HRESULT         Suspend();
    ULONG           Resume();
    HRESULT         InsertText(LPCTSTR pchInsert, ULONG cchInsert);
    HRESULT         DuplicateTextAtStart(ULONG cchInsert);
    HRESULT         TokenizeText(BOOL * fExhausted);
    
    HRESULT         InsertImage(LPCTSTR pchUrl, CDwnBindData * pDwnBindData);
    HRESULT         GoIntoPlaintextMode();
    HRESULT         SetContentTypeFromHeader(LPCTSTR pch);

    BOOL            IsSuspended() { return(_cSuspended > 0); }

    ELEMENT_TAG     GetGenericTag(LPTSTR pch, int cch);

#ifdef VSTUDIO7
    ELEMENT_TAG     GetIdentityBaseTag(LPTSTR pch, int cch);
#endif //VSTUDIO7

#ifdef XMV_PARSE
    void            SetGenericParse(BOOL fDoGeneric);
#endif

    static HRESULT  DoTokenizeOneTag(TCHAR *pchStart, ULONG cch, CHtmTagStm *pHts, CHtmPre *pHtmPre, ULONG ulLine, ULONG ulOff, ULONG fCount, DWORD *potCode);

protected:

    virtual void    Passivate();
private:

    HRESULT         Tokenize();
    int             PreprocessBuffer(int cch);
    HRESULT         FlushLast();
    HRESULT         MakeRoomForChars(int cch);
    HRESULT         Read();
    HRESULT         ReadStream(IStream *pstm, BOOL *pfEof);
    HRESULT         SaveBuffer();
    void            RestoreBuffer();
    BOOL            DoSwitchCodePage(CODEPAGE cp, BOOL *pfNeedRestart, BOOL fRestart);

    // CEncode override
    virtual BOOL    SwitchCodePage(CODEPAGE cp, BOOL *pfDifferentEncoding = NULL, BOOL fAutoDetected = FALSE);

    // Insert handling
    HRESULT         AddInsert(int cb, TOKENINSERTCODE tokcode);
    BOOL            AtInsert() { return(_cbNextInsert == _cbReadTotal); }
    HRESULT         OutputInserts();
    void            QueueInserts();

    // Source echo handling
    HRESULT         SaveSource(TCHAR *pch, ULONG cch);

    // Output token
    HRESULT         OutputTag(ULONG ulLine, ULONG ulOff, ULONG fCount, DWORD *potCode)
                    { RRETURN(DoTokenizeOneTag(_pchStart, _pch - _pchStart, _pHtmTagStm, this, ulLine, ulOff, fCount, potCode)); }
    HRESULT         OutputComment(TCHAR *pch, ULONG cch);
    HRESULT         OutputConditional(TCHAR *pch, ULONG cch, CONDVAL val);
    HRESULT         OutputDocSize();
    HRESULT         OutputEntity(TCHAR *pch, ULONG cch, XCHAR chEnt);
    void            OutputEof(HRESULT hr);

    // tag/attr lookup helpers
    HRESULT         SpecialToken(CHtmTag *pht);
    int             IndexOfAttr(TCHAR *pch);
    HRESULT         AddDwnCtx(UINT dt, LPCTSTR pchUrl, int cchUrl, CDwnBindData * pDwnBindData = NULL, DWORD dwProgClass = 0);
    HRESULT         HandleMETA(CHtmTag *pht);
#ifdef VSTUDIO7
    VOID            HandleTagsourcePI(CHtmTag *pht);
#endif //VSTUDIO7

    // Bits

    // Tokenizer state
    BOOL            _fLiteralEnt : 1;   // literal mode: no entities
    BOOL            _fEOF : 1;          // EOF and end of buffer
    BOOL            _fNoSuspend : 1;    // suppress suspending for </script>
    BOOL            _fEndCR : 1;        // identifis CR and the end of previous normalization buffer
    BOOL            _fCheckedForLeadingNull : 1;// for ClarisWorks header handling
    BOOL            _fSuppressLeadingText : 1;  // for ClarisWorks header handling
    BOOL            _fCountStart : 1;   // 1 if was counting lines at pchStart, 0 otherwise
    BOOL            _fCondComment : 1;  // inside <!--# comment-style conditional
    BOOL            _fScriptDefer : 1;  // current <SCRIPT> had a DEFER attribute (with no value)

    // Loader state
    BOOL            _fDone : 1;
    BOOL            _fPasting : 1;
    BOOL            _fRestarted : 1;
    BOOL            _fMetaCharsetOverride : 1;

#ifdef XMV_PARSE
    BOOL            _fXML : 1;
#endif
    
    // The tokenizer state
    ULONG           _state;             // state
    ELEMENT_TAG     _etagLiteral;       // literal mode: closing tag to match
    CStr            _cstrLiteral;       // literal mode: beginning of opening tag
    TCHAR           _chQuote;           // quote character
    TCHAR          *_pchStart;          // beginning of unprocessed chars
    TCHAR          *_pchWord;           // start of token, etc.
    TCHAR          *_pch;               // current scan point
    TCHAR          *_pchTop;            // first saved char
    TCHAR          *_pchAscii;          // first guaranteed Ascii char
    ULONG           _nLine;             // the line on which _pch lies
    ULONG           _nLineStart;        // the line on which _pchStart lies
    ULONG           _nLineWord;         // the line on which _pchWord lies
    ULONG           _fCount;            // 1 if counting lines, 0 otherwise
    DWORD           _hash;              // Entity hash
    XCHAR           _chEnt;             // Entity character
    
    // CHROME
    ULONG           _cDownloadSupression; // Non-zero if the download of images embedded in object tags is to be supressed

    // saved buffer stack for recursive doc.write
    int             _cchSaved;          // total number of saved characters
    BOOL            _fSuspend;          // in suspended state
    int             _cSuspended;        // count of nested suspendings
    CDataAry<int>   _aryCchSaved;       // saved char buf sizes (size==_cSuspended-_fSuspend)
    CDataAry<int>   _aryCchAsciiSaved;  // chars known to be ascii at end of each saved char buf

    // Token-insert state (for paste)
    class CInsertMap
    {
    public:
        int         _cb;
        TOKENINSERTCODE _icode;
    };
    int             _cbReadTotal;
    int             _cbNextInsert;
    int             _cOutputInsert;
    CStackDataAry <CInsertMap, 4> _aryInsert;
    
    // state used by SpecialToken
    CODEPAGE        _cpNew;             // new codepage due to META tag.
    
    // echo-source context tags
    ELEMENT_TAG *_atagEchoSourceBegin;
    ELEMENT_TAG _etagEchoSourceEnd;

    // Loader state
    CStr            _cstrDocUrl;
    CStr            _cstrBase;
    CHtmLoad *      _pHtmLoad;
    CHtmInfo *      _pHtmInfo;
    CDwnBindData *  _pDwnBindData;
    CHtmTagStm *    _pHtmTagStm;
    CDwnDoc *       _pDwnDoc;
    IInternetSession *  _pInetSess;

    // Conditional directive state
    ULONG           _cCondNestTrue;
    ULONG           _cCondNest;
    CVersions *     _pVersions;

    // script position tracking
    ULONG           _ulCharsEnd;
    ULONG           _ulCharsUncounted;
    ULONG           _ulLastCharsSeen;
    TCHAR *         _pchHead;

    CAtomTable      _aryGenericTags;
#if 0
    CAtomTable      _aryGenericTagsUnconfirmed;
#endif

    // source-copy stream chan
//    CStmChan *      _pscCopy;

    NO_COPY(CHtmPre);

};

XCHAR EntityChFromName(TCHAR *pchText, ULONG cchText, DWORD hash);
XCHAR EntityChFromNumber(TCHAR *pch, ULONG cch);
XCHAR EntityChFromHex(TCHAR *pch, ULONG cch);

void  ProcessValueEntities(TCHAR *pch, ULONG *pcch);

HRESULT CreateHtmGenericParseCtx(CHtmParseCtx **pphpx, CElement *pel, CHtmParseCtx *phpxParent);

// CreateElement function prototype.
HRESULT CreateElement(CHtmTag *     pht, 
                      CElement **   ppElementResult, 
                      CDoc *        pDoc, 
                      CMarkup *     pMarkup, 
                      BOOL *        pfDie,
                      DWORD         dwFlags = 0);

#pragma INCMSG("--- End 'htm.hxx'")
#else
#pragma INCMSG("*** Dup 'htm.hxx'")
#endif
