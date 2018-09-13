//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       htmtags.hxx
//
//  Contents:   TAGDESC
//              ELEMENT_TAG
//              CHtmlTag
//              CHtmParseCtx
//
//----------------------------------------------------------------------------

MtExtern(CHtmRootParseCtx);
MtExtern(CHtmRootParseCtx_CNotifyList_pv);

#if DBG == 1
#define VALIDATE(pNodeUnder) Validate(pNodeUnder)
#else
#define VALIDATE(pNodeUnder)
#endif

class CHtmRootParseCtx : public CHtmParseCtx
{
public:
    typedef CHtmParseCtx super;
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmRootParseCtx));
    
    CHtmRootParseCtx(CMarkup *pMarkup);
    virtual ~CHtmRootParseCtx();

    virtual HRESULT Init();
    virtual HRESULT Prepare();
    virtual HRESULT Commit();
    virtual HRESULT Finish();
    virtual HRESULT BeginElement(CTreeNode **ppNodeNew, CElement *pel, CTreeNode *pNodeCur, BOOL fEmpty);
    virtual HRESULT EndElement(CTreeNode **ppNodeNew, CTreeNode *pNodeCur, CTreeNode *pNodeEnd);
    virtual HRESULT AddText(CTreeNode *pNode, TCHAR *pch, ULONG cch, BOOL fAscii);
    virtual HRESULT InsertLPointer ( CTreePos * * pptp, CTreeNode * pNodeCur );
    virtual HRESULT InsertRPointer ( CTreePos * * pptp, CTreeNode * pNodeCur );
    virtual CHtmParseCtx *GetHpxEmbed();
    
    HRESULT InsertPointer ( CTreePos * * pptp, CTreeNode * pNodeCur, BOOL fRightGravity );
    HRESULT InsertTextFrag ( TCHAR * pch, ULONG cch, CTreeNode * pNodeCur );

    BOOL    SetGapToFrontier( CTreePosGap * ptpg );

private:

    friend class CHtmTopParseCtx;
    friend class CHtmTextParseCtx;
    
    HRESULT HookBeginElement(CTreeNode * pNode);
    HRESULT HookEndElement(CTreeNode * pNode, CTreeNode * pNodeCur);

    HRESULT OverlappedEndElement( CTreeNode **ppNodeNew, CTreeNode* pNodeCur, CTreeNode *pNodeEnd, BOOL fFlushNotification );

    void    AddCharsToNotification( long cpStart, long cch  );

    void    FlushTextNotification();
    HRESULT FlushNotifications();
    HRESULT NailDownChain();

    CTreePos * InsertNewTextPosInChain( 
                LONG cch, 
                SCRIPT_ID sid, 
                CTreePos *ptpBeforeOnChain );

#if DBG==1
    CTreePos *  AdvanceFrontier();
#else
    void        AdvanceFrontier();
#endif
                

    void    LazyPrepare( CTreeNode * pNodeUnder );
    WHEN_DBG( void Validate( CTreeNode * pNodeUnder) );
    
    CMarkup *       _pMarkup;
    CDoc *          _pDoc;

    SCRIPT_ID       _sidLast;

    unsigned        _fTextPendingValid      : 1;
    unsigned        _fLazyPrepareNeeded     : 1;

    // Data about the frontier
    CTreePos *      _ptpAfterFrontier;  

    // The chain that is waiting to
    // be put into the tree
    CTreePos *      _ptpChainTail;
    CTreeDataPos    _tdpTextDummy;

    // The current insertion point
    // in the chain.
    CTreeNode *     _pNodeChainCurr;
    CTreePos *      _ptpChainCurr;
    long            _cpChainCurr;

    // WCH_NODE characters that need
    // to be inserted at _cpChainCur
    long            _cchNodeBefore;
    long            _cchNodeAfter;

    // Notification Data
    CNotification   _nfTextPending;
    CTreeNode *     _pNodeForNotify;
    long            _nElementsAdded;
    CTreePos *      _ptpElementAdded;
};
