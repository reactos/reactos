//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       peer.hxx
//
//  Contents:   peer holder / peer site
//
//----------------------------------------------------------------------------

#ifndef I_PEER_HXX_
#define I_PEER_HXX_
#pragma INCMSG("--- Beg 'peer.hxx'")

#ifndef _BEHAVIOR_H_
#define _BEHAVIOR_H_
#include "behavior.h"
#endif

// #define PEER_NOTIFICATIONS_AFTER_INIT

/////////////////////////////////////////////////////////////////////////////////

// size of range of dispids of a single peer and events (efficiently mapped)
#if DBG == 1
// in debug builds we allow range slightly larger to simplify debugging: (1 << 16) = 65K ~= 65,000 < 100,000
#define DISPID_PEER_HOLDER_RANGE_SIZE       100000
#else
#define DISPID_PEER_HOLDER_RANGE_SIZE       (1 << 16)
#endif

// min/max dispids reserved for HTC DD
#define DISPID_HTCDD_BASE                   (DISPID_PEER_HOLDER_BASE)
#define DISPID_HTCDD_MAX                    (DISPID_PEER_HOLDER_BASE +  999999)

// min/max dispids of peer holders exposed as scripting identity via short name
#define DISPID_PEER_NAME_BASE               (DISPID_PEER_HOLDER_BASE + 1000000)
#define DISPID_PEER_NAME_MAX                (DISPID_PEER_HOLDER_BASE + 1999999)

// min/max dispids of peer events
#define DISPID_PEER_EVENTS_BASE             (DISPID_PEER_HOLDER_BASE + 2000000)
#define DISPID_PEER_EVENTS_MAX              (DISPID_PEER_HOLDER_BASE + 2999999)

// base of first peer holder range
#define DISPID_PEER_HOLDER_FIRST_RANGE_BASE (DISPID_PEER_HOLDER_BASE + 3000000)

// min/max dispid of a single peer
#define DISPID_PEER_BASE                    0                                   
#define DISPID_PEER_MAX                     DISPID_PEER_HOLDER_RANGE_SIZE - 1

/////////////////////////////////////////////////////////////////////////////////

MtExtern(CPeerMgr)
MtExtern(CPeerHolder)
MtExtern(CPeerHolder_CEventsBag)
MtExtern(CPeerHolder_CEventsBag_aryEvents)
MtExtern(CPeerHolder_CRenderBag)
MtExtern(CPeerUrnCollection)
MtExtern(CDefaultBehaviorCache_aryCache)

class CPeerFactory;

#define HTMLEVENTOBJECT_USESAME ((IHTMLEventObj*)&g_Zero)

/////////////////////////////////////////////////////////////////////////////////
//
// Class:   CPeerMgr
//
/////////////////////////////////////////////////////////////////////////////////

class CPeerMgr : public CVoid
{
public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerMgr))

    //
    // methods
    //

    CPeerMgr(CElement * pElement);
    ~CPeerMgr();

    static HRESULT EnsurePeerMgr       (CElement * pElement, CPeerMgr ** ppPeerMgr);
    static void    EnsureDeletePeerMgr (CElement * pElement, BOOL fForce = FALSE);

    inline BOOL CanDelete()
    {
        return  0 == _cPeerDownloads &&
                READYSTATE_COMPLETE == _readyState &&
                !IsEnterExitTreeStablePending() &&
                !IsApplyStyleStablePending();
    };

    inline void IncPeerDownloads()
    {
        _cPeerDownloads++;
    }

    inline void DecPeerDownloads()
    {
        Assert (_cPeerDownloads);
        _cPeerDownloads--;
    }

    static void UpdateReadyState(CElement * pElement, READYSTATE readyStateNew = READYSTATE_UNINITIALIZED);
           void UpdateReadyState(READYSTATE readyState = READYSTATE_UNINITIALIZED);

    void AddDownloadProgress();
    void DelDownloadProgress();

    void OnExitTree();

    inline BOOL IsEnterExitTreeStablePending()        { return _fEnterExitTreeStablePending; };
    inline void SetEnterExitTreeStablePending(BOOL f) { _fEnterExitTreeStablePending = f; }

    inline BOOL IsApplyStyleStablePending()           { return _fApplyStyleStablePending; };
    inline void SetApplyStyleStablePending(BOOL f)    { _fApplyStyleStablePending = f; }

    //
    // data
    //

    CElement *          _pElement;
    DWORD               _dwDownloadProgressCookie;
    READYSTATE          _readyState;
    WORD                _cPeerDownloads;
    BOOL                _fEnterExitTreeStablePending : 1;
    BOOL                _fApplyStyleStablePending : 1;
};

/////////////////////////////////////////////////////////////////////////////////
//
// Class:   CPeerHolder
//
/////////////////////////////////////////////////////////////////////////////////

class CPeerHolder :
    public CVoid,
    public IUnknown // see comments about absence of object identity of CPeerHolder below
{
    DECLARE_CLASS_TYPES(CPeerHolder, CVoid)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder))

    //
    // consruction / destruction
    //

    CPeerHolder(CElement * pElement);
    ~CPeerHolder();

    void Passivate();

    //
    // helpers for thunks and refcounting
    //
    // NOTE: CPeerHolder is not a COM object: it does not have COM object identity,
    // and should never be QI-ed directly. The only callers of QI method 
    // can be CPeerSite and thunks created in CElement::CreateTearoffThunk. IUnknown
    // of the peer holder should never be handed out to anobody!
    // See comments in CPeerHolder::QueryInterface for details.
    //

    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    ULONG SubAddRef();
    ULONG SubRelease();

    ULONG GetObjectRefs(); // helper for CPeerSite (for DECLARE_FORMS_SUBOBJECT_IUNKNOWN)

    //
    // creation
    //

    HRESULT Create(
        LPTSTR          pchPeerName,
        CPeerFactory *  pPeerFactory);

    HRESULT AttachPeer(IElementBehavior * pPeer);
    HRESULT InitRender();
    HRESULT InitUI();
    HRESULT InitAttributes();
    HRESULT InitCategory();
    HRESULT InitCmdTarget();
    HRESULT InitReadyState();
    
    //
    // rendering
    //

    HRESULT OnLayoutAvailable(CLayout * pLayout);
    HRESULT UpdateSurfaceFlags();
    HRESULT Draw(CFormDrawInfo * pDI, LONG lRenderInfo);
    HRESULT HitTestPoint(POINT * pPoint, BOOL * pfHit);

    inline LONG RenderFlags() { return _pRenderBag ? _pRenderBag->_lRenderInfo : 0; };
    inline BOOL TestRenderFlags(LONG l) { return l == (RenderFlags() & l); };

    inline BOOL IsRenderPeer() { return 0 != RenderFlags(); }

    //
    // IDispatch[Ex] helpers
    //

    HRESULT GetDispIDMulti(BSTR bstrName, DWORD grfdex, DISPID *pid);
    HRESULT GetDispIDSingle(BSTR bstrName, DWORD grfdex, DISPID *pid);

    HRESULT InvokeExMulti(
        DISPID              dispidMember,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider);
    HRESULT InvokeExSingle(
        DISPID              dispidMember,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider);

    HRESULT GetEventDispidMulti(LPOLESTR pchEvent, DISPID * pdispid);

    HRESULT GetNextDispIDMulti(
        DWORD       grfdex,
        DISPID      dispid,
        DISPID *    pdispid);

    HRESULT GetMemberNameMulti(
        DISPID      dispid,
        BSTR *      pbstrName);

    //
    // misc helpers
    //

    HRESULT QueryPeerInterface      (REFIID riid, void ** ppv);
    HRESULT QueryPeerInterfaceMulti (REFIID riid, void ** ppv, BOOL fIdentityOnly);

    void    EnsureDispatch();

    BOOL    IllegalSiteCall();

    inline BOOL IsIdentityPeer() { return TestFlag(IDENTITYPEER); }
    inline BOOL IsCssPeer()      { return TestFlag(CSSPEER); }

#if DBG == 1
    void AssertCorrectIdentityPeerHolder();
#endif

    CPeerHolder * GetRenderPeerHolder();

    inline LONG CookieID()       { return _dispidBase; }

    void    OnElementNotification(CNotification *pNF);
    void    NotifyMulti(LONG lEvent);

    HRESULT ApplyStyleMulti();

    HRESULT Save     (CStreamWriteBuff * pStreamWriteBuff = NULL, BOOL * pfAny = NULL);
    HRESULT SaveMulti(CStreamWriteBuff * pStreamWriteBuff = NULL, BOOL * pfAny = NULL);

    HRESULT GetCustomEventsTypeInfoCount(ULONG * pCount);
    HRESULT CreateCustomEventsTypeInfo(ULONG iTI, ITypeInfo ** ppTICoClass);
    HRESULT CreateCustomEventsTypeInfo(ITypeInfo ** ppTICoClass);

    inline LONG CustomEventsCount()
    {
        return _pEventsBag  ? _pEventsBag->_aryEvents.Size() : 0;
    }
    inline DISPID CustomEventDispid(int cookie)
    {
        return _pEventsBag->_aryEvents[cookie].dispid;
    }
    inline DISPID CustomEventFlags(int cookie)
    {
        return _pEventsBag->_aryEvents[cookie].lFlags;
    }
    LPTSTR  CustomEventName(int cookie);
    BOOL    HasCustomEventMulti(DISPID dispid);

    BOOL IsRelated     (LPTSTR pchCategory);
    BOOL IsRelatedMulti(LPTSTR pchCategory, CPeerHolder ** ppPeerHolder = NULL);

    HRESULT ExecMulti(
        const GUID *    pguidCmdGroup, 
        DWORD           nCmdID, 
        DWORD           nCmdExecOpt, 
        VARIANTARG *    pvaIn, 
        VARIANTARG *    pvaOut); 

    HRESULT     UpdateReadyState();
    READYSTATE  GetReadyStateMulti();

    HRESULT     EnsureNotificationsSentMulti();
    HRESULT     EnsureNotificationsSent();

    //
    // dispid mapping helpers
    //

    inline static BOOL IsCustomEventDispid(DISPID dispid)
    {
        return (DISPID_PEER_EVENTS_BASE <= dispid && dispid <= DISPID_PEER_EVENTS_MAX);
    }

    inline static DISPID AtomToEventDispid(DWORD atom)
    {
        return (atom + DISPID_PEER_EVENTS_BASE);
    }

    inline static DISPID AtomFromEventDispid(DISPID dispid)
    {
        return (dispid - DISPID_PEER_EVENTS_BASE);
    }

    inline DISPID DispidToExternalRange(DISPID dispid)
    {
        return (dispid + _dispidBase);
    }

    inline DISPID DispidFromExternalRange(DISPID dispid)
    {
        return (dispid - _dispidBase);
    }

    inline static DISPID UniquePeerNumberToBaseDispid(DWORD dwNumber)
    {
        return DISPID_PEER_HOLDER_FIRST_RANGE_BASE + DISPID_PEER_HOLDER_RANGE_SIZE * dwNumber;
    }

    //+---------------------------------------------------------------------------
    //
    //  CPeerSite
    //
    //----------------------------------------------------------------------------

    class CPeerSite :
        public CVoid,
        public IElementBehaviorSite
    {
        DECLARE_CLASS_TYPES(CPeerSite, CVoid)

    public:

        //
        // IUnknown
        //

        DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CPeerHolder)

        //
        // IElementBehaviorSite
        //

        STDMETHOD(GetElement)(IHTMLElement ** ppElement);
        STDMETHOD(RegisterNotification)(LONG lEvent);
        
        //
        // IElementBehaviorSiteOM
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteOM)

        NV_DECLARE_TEAROFF_METHOD(RegisterEvent,     registerevent,     (LPOLESTR pchEvent, LONG lFlags, LONG * plCookie));
        NV_DECLARE_TEAROFF_METHOD(GetEventCookie,    geteventcookie,    (LPOLESTR pchEvent, LONG * plCookie));
        NV_DECLARE_TEAROFF_METHOD(FireEvent,         fireevent,         (LONG   lCookie, IHTMLEventObj * pEventObject));
        NV_DECLARE_TEAROFF_METHOD(CreateEventObject, createeventobject, (IHTMLEventObj ** ppEventObject));
        NV_DECLARE_TEAROFF_METHOD(RegisterName,      registername,      (LPOLESTR pchName));
        NV_DECLARE_TEAROFF_METHOD(RegisterUrn,       registerurn,       (LPOLESTR pchUrn));

        //
        // IElementBehaviorSiteRender
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteRender)

        NV_DECLARE_TEAROFF_METHOD(Invalidate,           invalidate,           (RECT* pRect));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRenderInfo, invalidaterenderinfo, ());
        NV_DECLARE_TEAROFF_METHOD(InvalidateStyle,      invalidatestyle,      ());

        //
        // IElementBehaviorSiteCategory
        //

        DECLARE_TEAROFF_TABLE(IElementBehaviorSiteCategory)

        NV_DECLARE_TEAROFF_METHOD(GetRelatedBehaviors, getrelatedbehaviors, (LONG lDirection, LPOLESTR pchCategory, IEnumUnknown ** ppEnumerator));

        //
        // IServiceProvider
        //

        DECLARE_TEAROFF_TABLE(IServiceProvider)

        NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject));

        //
        // IBindHost
        //

        DECLARE_TEAROFF_TABLE(IBindHost)

        NV_DECLARE_TEAROFF_METHOD(CreateMoniker,        createmoniker,       (LPOLESTR pchName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved));
        NV_DECLARE_TEAROFF_METHOD(MonikerBindToStorage, monikerbindtostorage,(IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));
        NV_DECLARE_TEAROFF_METHOD(MonikerBindToObject,  monikerbindtoobject, (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));

        //
        // IPropertyNotifySink
        //

        DECLARE_TEAROFF_TABLE(IPropertyNotifySink)

        NV_DECLARE_TEAROFF_METHOD(OnChanged,     onchanged,     (DISPID dispid));
        NV_DECLARE_TEAROFF_METHOD(OnRequestEdit, onrequestedit, (DISPID dispid)) { return E_NOTIMPL; };
            
        //
        // IOleCommandTarget
        //

        DECLARE_TEAROFF_TABLE(IOleCommandTarget)

        NV_DECLARE_TEAROFF_METHOD(QueryStatus,  querystatus,    (const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText)) { RRETURN (E_NOTIMPL); }
        NV_DECLARE_TEAROFF_METHOD(Exec,         exec,           (const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANT * pvarArgIn, VARIANT * pvarArgOut));
            
        //
        // misc
        //

        inline CDoc * Doc() { return MyCPeerHolder()->_pElement->Doc(); }

        HRESULT GetEventCookieHelper (LPOLESTR pchEvent, LONG lFlags, LONG * plCookie, BOOL fEnsureCookie);
        HRESULT GetEventDispid       (LPOLESTR pchEvent, DISPID * pdispid);

        HRESULT FireEvent (LONG lCookie, BOOL fSameEventObject = FALSE);

        //
        // data (DEBUG ONLY)
        //

#if DBG == 1
        CPeerHolder *   _pPeerHolderDbg;
#endif
    };

    //+---------------------------------------------------------------------------
    //
    //  CListMgr
    //
    //----------------------------------------------------------------------------

    class CListMgr
    {
    public:

        //
        // methods
        //

        CListMgr();

        HRESULT                 Init(CPeerHolder * pPeerHolder);

        HRESULT                 StartBuild(CElement * pElement);
        HRESULT                 DoneBuild();

        inline CPeerHolder *    Head()    { return _pHead;    };
        inline CPeerHolder *    Current() { return _pCurrent; };

        inline BOOL             IsEmpty() { return NULL == _pHead;    };
        inline BOOL             IsEnd()   { return NULL == _pCurrent; };

        void                    Reset();
        void                    Step();

        BOOL                    Find(LPTSTR pchUrl);

        void                    AddTo    (CPeerHolder * pItem, BOOL fAddToHead = TRUE);
        inline void             AddToHead(CPeerHolder * pItem) { AddTo(pItem, TRUE);  }
        inline void             AddToTail(CPeerHolder * pItem) { AddTo(pItem, FALSE); }
    
        void                    MoveCurrentTo      (CListMgr * pTargetList, BOOL fMoveToHead = TRUE, BOOL fSave = FALSE);
        inline void             MoveCurrentToHeadOf(CListMgr * pTargetList) { MoveCurrentTo (pTargetList, TRUE);  }
        inline void             MoveCurrentToTailOf(CListMgr * pTargetList) { MoveCurrentTo (pTargetList, FALSE); }

        inline void             DetachCurrent(BOOL fSave = FALSE) { MoveCurrentTo(NULL, TRUE, fSave); }

        void                    InsertInHeadOf     (CListMgr * pTargetList);

        static HRESULT          AttachPeerHolderToElement(CPeerHolder * pPeerHolder, CElement * pElement);

        long                    GetPeerHolderCount(BOOL fNonEmptyOnly);
        CPeerHolder *           GetPeerHolderByIndex(long lIndex, BOOL fNonEmptyOnly);
        BOOL                    HasPeerWithUrn(LPCTSTR Urn);

        //
        // data
        //

        CElement *      _pElement;      // the element we are building list for
        CPeerHolder *   _pHead;
        CPeerHolder *   _pCurrent;
        CPeerHolder *   _pPrevious;
    };

    //+---------------------------------------------------------------------------
    //
    //  CPeerHolderIterator
    //
    //----------------------------------------------------------------------------

    class CPeerHolderIterator
    {
    public:

        inline void             Start (CPeerHolder * pPH)   { _pPH = pPH; }
        inline void             Step()                      { _pPH = _pPH->_pPeerHolderNext; }
        inline BOOL             IsEnd()                     { return NULL == _pPH; }
        inline CPeerHolder *    PH()                        { return _pPH; }

        CPeerHolder *           _pPH;
    };

    //
    // main data
    //

    IElementBehavior *  _pPeer;                     // peer
    CPeerSite           _PeerSite;                  // site for the peer
    CElement *          _pElement;                  // element the peer is bound to
    CPeerHolder *       _pPeerHolderNext;           // next peer holder in the list

    ULONG               _ulRefs;                    // refcount
    ULONG               _ulAllRefs;                 // sub-refcount

    IDispatch *         _pDisp;                     // contains IDispatchEx if _fDispEx is true, and IDispatch otherwise
    DISPID              _dispidBase;                // base dispid of dispid range of this peer holder

    IOleCommandTarget * _pPeerCmdTarget;
    IElementBehaviorUI *_pPeerUI;
    
    //
    // render bag
    //

    class CRenderBag
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CRenderBag))

        CRenderBag() {};
        ~CRenderBag() { ReleaseInterface(_pPeerRender); };

        IElementBehaviorRender *    _pPeerRender;   // IElementBehaviorRender of peer object if it has one
        LONG                        _lRenderInfo;   // flags controlling our default drawing and when to call peer's draw
    };

    CRenderBag *        _pRenderBag;

    //
    // misc
    //

    CStr                _cstrName;                  // name of this peer - registered with RegisterName
    CStr                _cstrUrn;                   // urn  of this peer - registered with RegisterUrn

    CPeerFactoryUrl *   _pPeerFactoryUrl;           // pointer to CPeerFactoryUrl created this peer holder
    CStr                _cstrCategory;              // category supplied by IElementBehavior::GetCategory
    
    //
    // custom events firing
    //

    class CEventsItem
    {
    public:
        DISPID      dispid;
        DWORD       lFlags;
    };

    class CEventsBag
    {
    public:
        DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerHolder_CEventsBag))

        CEventsBag() { _ulTypeInfoIdx = ULONG_MAX; };

        DECLARE_CDataAry(CEventsArray, CEventsItem, Mt(Mem), Mt(CPeerHolder_CEventsBag_aryEvents));

        CEventsArray    _aryEvents;             // custom events names
        ULONG           _ulTypeInfoIdx;         // index of typeinfo through which we expose custom events
    };

    CEventsBag *        _pEventsBag;

    //+---------------------------------------------------------------------------
    //
    //  locks and flags management
    //
    //----------------------------------------------------------------------------

    enum FLAGS
    {
        NOFLAGS                     = 0,
        DISPCACHED                  = 1 <<  0,      // we made an attempt to QI for IDispatch/IDispatchEx of peer
        DISPEX                      = 1 <<  1,      // _pDisp contains IDispatchEx (otherwise it is IDispatch)
        AFTERINIT                   = 1 <<  2,      // set after peer initialized and attached
        CSSPEER                     = 1 <<  3,      // set when the behavior is created by css
        IDENTITYPEER                = 1 <<  4,      // set when the behavior is an identity behavior

        LOCKINQI                    = 1 <<  5,      // peer initiated QI and we are in the middle of it
        LOCKGETDISPID               = 1 <<  6,      // we are in the middle of asking peer for a name
        LOCKAPPLYSTYLE              = 1 <<  7,      // we are in the middle of applying styles

        NEEDNOTIFICATIONOFFSET      =       8,
        NEEDCONTENTREADY            = 1 <<  8,      // peer wants CONTENTREADY            notification
        NEEDDOCUMENTREADY           = 1 <<  9,      // peer wants DOCUMENTREADY           notification
        NEEDAPPLYSTYLE              = 1 << 10,      // peer wants APPLYSTYLE              notification
        NEEDDOCUMENTCONTEXTCHANGE   = 1 << 11,      // peer wants ONDOCUMENTCONTEXTCHANGE notification

        FLAGS_COUNT                 =      12,

        LOCKFLAGS                   = LOCKINQI | LOCKGETDISPID | LOCKAPPLYSTYLE

    };

    class CLock
    {
    public:
        CLock(CPeerHolder * pPeerHolder, FLAGS enumFlags = NOFLAGS);
        ~CLock();

        CPeerHolder *   _pPeerHolder;
        WORD            _wPrevFlags;
    };
    
    inline BOOL TestFlag  (FLAGS flags) { return 0 != (_wFlags & (WORD)flags); };
    inline void SetFlag   (FLAGS flags) { _wFlags |=  (WORD)flags; };
    inline void ClearFlag (FLAGS flags) { _wFlags &= ~(WORD)flags; };

    BOOL TestFlagMulti(FLAGS flag);

    inline FLAGS FlagFromNotification (LONG  lEvent) { return (FLAGS) (1 << (NEEDNOTIFICATIONOFFSET + lEvent)); }
    inline WORD  NotificationFromFlag (FLAGS flag  )
    {
        WORD    i, w;
        for (i = 0, w = ((WORD)flag) >> (NEEDNOTIFICATIONOFFSET + 1); w; i++, w = w >> 1)
            NULL;
        return i;
    }

    WORD                _wFlags         : FLAGS_COUNT;
    READYSTATE          _readyState     : 4;

};

/////////////////////////////////////////////////////////////////////////////////
//
// class CUrnCollection
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#define _hxx_
#include "urncoll.hdl"

class CPeerUrnCollection : public CCollectionBase
{
public:

    DECLARE_CLASS_TYPES(CPeerUrnCollection, CCollectionBase)
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CPeerUrnCollection))

    //
    // methods
    //

    CPeerUrnCollection(CElement *pElement);
    ~CPeerUrnCollection();

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    //
    // CCollectionBase overrides
    //

    long FindByName(LPCTSTR pszName, BOOL fCaseSensitive = TRUE) { return -1; }
    LPCTSTR GetName(long lIdx) { return NULL; }
    HRESULT GetItem(long lIndex, VARIANT *pvar);

    //
    // wiring
    // 

    DECLARE_PLAIN_IUNKNOWN(CPeerUrnCollection);

    #define _CPeerUrnCollection_
    #include "urncoll.hdl"

    DECLARE_CLASSDESC_MEMBERS;

    //
    // data
    //

    CElement *      _pElement;
};

/////////////////////////////////////////////////////////////////////////////////
//
// misc
//
/////////////////////////////////////////////////////////////////////////////////

// IceCAP

#if 1
#ifndef StartCAP
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
#endif
#endif

// service SElementBehaviorMisc

const static GUID SID_SElementBehaviorMisc = {0x3050f632,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
const static GUID CGID_ElementBehaviorMisc = {0x3050f633,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};

#define CMDID_ELEMENTBEHAVIORMISC_GETCONTENTS     1
#define CMDID_ELEMENTBEHAVIORMISC_PUTCONTENTS     2

// eof

#pragma INCMSG("--- End 'peer.hxx'")
#else
#pragma INCMSG("*** Dup 'peer.hxx'")
#endif
