//+----------------------------------------------------------------------------
//
// File:        QLAYOUT.HXX
//
// Contents:    CFlowLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
// This is the version customized for hosting Quill.
//
//-----------------------------------------------------------------------------

#ifndef I_QLAYOUT_HXX_
#define I_QLAYOUT_HXX_
#pragma INCMSG("--- Beg 'qlayout.hxx'")

#ifdef QUILL

#ifndef X_QLAYOUT_HXX_
#define X_QLAYOUT_HXX_
#include "..\lequill\qlayout.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_QDISP_H_
#define X_QDISP_H_
#include "qdisp.h"
#endif

struct  DROPTARGETSELINFO;

class   CIme;
class   CRecalcTask;
class   CDropTargetInfo;

class   CQuillGlue;

#define BEGINSUPPRESSFORQUILL   if (!_pFlowLayout->FExternalLayout()) {
#define ENDSUPPRESSFORQUILL     }

enum TXTBACKSTYLE {
    TXTBACK_TRANSPARENT = 0,        //@emem background should show through
    TXTBACK_OPAQUE                  //@emem erase background
};

MtExtern(CFlowLayout)

//+----------------------------------------------------------------------------
//
//  Class:      CFlowLayout
//
//  Synopsis:   A CLayout-derivative that lays content out using a text flow
//              algorithm.
//
//-----------------------------------------------------------------------------

class CFlowLayout : public CLayout
{
public:
    typedef CLayout super;
    friend class CDisplay;
    friend class CMeasurer;
    friend class CRecalcLinePtr;

    // Construct / Destruct
    CFlowLayout (CElement * pElementFlowLayout);  // Normal constructor.

    virtual ~ CFlowLayout ( );

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFlowLayout))

    void    FindAndDirtyFlowLayouts (long iRunStart, long iRunStop);
    void    DoLayout(DWORD grfLayout);
    void    Notify(CNotification * pnf);

    inline BOOL FExternalLayout() { return Doc()->FExternalLayout(); }

    //
    //  Channel control methods
    //

    virtual void    Reset();

    virtual BOOL    IsDirty()   { return _dtr.IsDirty(); }

            BOOL    IsRangeBeforeDirty(long cp, long cch)
                    {
                        Assert(cp >= 0);
                        Assert((cp + cch) <= GetContentTextLength());
                        return _dtr.IsBeforeDirty(cp, cch);
                    }
            BOOL    RangeIntersectsDirty(long cp, long cch)
                    {
                        Assert(cp >= 0);
                        Assert((cp + cch) <= GetContentTextLength());
                        return _dtr.IntersectsDirty(cp, cch);
                    }
            BOOL    IsRangeAfterDirty(long cp, long cch)
                    {
                        Assert(cp >= 0);
                        Assert((cp + cch) <= GetContentTextLength());
                        return _dtr.IsAfterDirty(cp, cch);
                    }

    void    Listen(BOOL fListen = TRUE);
    BOOL    IsListening();

    BOOL    IsTopDown() { return !!_fTopDown; }

    long    Cp()        { return _dtr._cp; }
    long    CchOld()    { return _dtr._cchOld; }
    long    CchNew()    { return _dtr._cchNew; }

#if DBG==1
    void    Lock()      { _fLocked = TRUE; }
    void    Unlock()    { _fLocked = FALSE; }
    BOOL    IsLocked()  { return !!_fLocked; }
#endif

    // IDispNode not yet implemented
    // STDMETHOD(GetDisplayNode(IDispNode **pNode));

    virtual CFlowLayout * IsFlowLayout() { return this; }
    virtual CLayout     * IsFlowOrSelectLayout() { return this; }

    // void    FlushLayout();


    virtual LONG   GetMaxLength()   { return MAXLONG;                  }

    TCHAR GetPasswordCh()
    {
        return GetFirstBranch()->GetCharFormat()->_fPassword ? TEXT('*') : 0;
    }

    // Helper functions
    // BOOL    AllowBackgroundRecalc() { return _pBgRecalcInfo != NULL; }

    LONG    GetMaxLineWidth();
    virtual BOOL            GetAutoSize()  const    { return FALSE; }
            TXTBACKSTYLE    GetBackStyle() const    { return _fTxtStyle ? TXTBACK_OPAQUE : TXTBACK_TRANSPARENT; }
    virtual BOOL            GetMultiLine() const    { return TRUE; }
    virtual BOOL            GetWordWrap() const     { return TRUE; }
            BOOL            GetVertical() const     { return _fVertical; }
            void            Sound()                 { if (_fAllowBeep) MessageBeep(0); }

    BOOL FRecalcDone() const
    {
        return _dp._fRecalcDone;
    }

    IF_DBG( BOOL IsInPlace(); )

    //
    // Notify site that it is being detached from the form
    // Children should be detached and released

    virtual void Detach();

    void Fire_onfocus()     {}
    void Fire_onblur()      {}
    void Fire_onresize()    {}

    // Helper to push change notification over the entire text
    HRESULT AllCharsAffected();
    void    CommitChanges(CCalcInfo * pci);
    void    CancelChanges();
    BOOL    IsCommitted();

    // Property Change Helpers
    virtual HRESULT OnTextChange(void) { return S_OK; }
    virtual HRESULT OnSelectionChange(void) { return S_OK; }

    // View change notification
    void    ViewChange ( BOOL fUpdate );

    // Table cell layout support
    virtual void    MarkHasAlignedLayouts(BOOL fHasAlignedLayouts) { }


    // Recalc support
            HRESULT WaitForParentToRecalc(CElement *pElement, CCalcInfo * pci = NULL);
            HRESULT WaitForParentToRecalc(LONG cpMax, LONG yMax, CCalcInfo * pci);

    //--------------------------------------------------------------
    // CSite override methods
    //--------------------------------------------------------------

    virtual void RegionFromElement( CElement * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT * prcBound);

    // Enumeration method to loop thru children (start)
    virtual CLayout * GetFirstLayout (DWORD * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE );

    // Enumeration method to loop thru children (continue)
    virtual CLayout * GetNextLayout (DWORD * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE );

    virtual BOOL ContainsChildLayout(BOOL fRaw = FALSE);
    virtual void ClearLayoutIterator(DWORD dw, BOOL dRaw = FALSE);

    virtual HRESULT GetElementsInZOrder(CPtrAry<CElement *> *paryElements,
                                        CElement            *pElementThis,
                                        RECT                *prc,
                                        HRGN                 hrgn,
                                        BOOL                 fIncludeNotVisible);
    virtual HRESULT SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate);
    virtual BOOL    CanHaveChildren() { return TRUE; }

    virtual HRESULT ScrollElementIntoView(CElement *    pElement,
                                          SCROLLPIN     spVert = SP_MINIMAL,
                                          SCROLLPIN     spHorz = SP_MINIMAL,
                                          BOOL          fFirstVisible = FALSE);

    virtual HRESULT ScrollRangeIntoView  (long          cpMin,
                                          long          cpMost,
                                          SCROLLPIN     spVert = SP_MINIMAL,
                                          SCROLLPIN     spHorz = SP_MINIMAL);

    virtual HRESULT ScrollIntoView(SCROLLPIN spVert = SP_MINIMAL,
                                   SCROLLPIN spHorz = SP_MINIMAL,
                                   BOOL      fFirstVisible = FALSE);

    // Scrolling overrides
            BOOL    ScrollMore(POINT *ppt, POINT *pptDelta)
            {
#ifndef DISPLAY_TREE
                return _dp.ScrollMore(ppt, pptDelta);
#else
// BUGBUG: Rewrite this to use the display tree (brendand)
                return FALSE;
#endif
            }

            HTC     BranchFromPoint(
                                    DWORD               dwFlags,
                                    POINT               pt,
                                    CTreeNode **        ppNodeBranch,
                                    HITTESTRESULTS *    presultsHitTest);
            HTC     BranchFromPointEx(
                                    POINT           pt,
                                    CLinePtr &      rp,
                                    CTreePos  *     ptp,
                                    CTreeNode *     pNodeRelative,
                                    CTreeNode **    ppNodeBranch,
                                    BOOL            fPsuedoHit,
                                    BOOL *          pfWantArrow,
                                    BOOL            fIngoreBeforeAfter,
                                    BOOL            fVirtual);


    virtual void Draw(CFormDrawInfo *pDI);
    virtual void ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected);

    // Selection access
            HRESULT NotifyKillSelection();

    // Sizing and Positioning
            void    ResizePercentHeightSites();
            void    ResetMinMax();

            BOOL    GetSiteWidth(CLayout   *pLayout,
                                 CCalcInfo *pci,
                                 BOOL       fBreakAtWord,
                                 LONG       xWidthMax,
                                 INT       *pxMinSiteWidth,
                                 LONG      *pxWidth,
                                 LONG      *pyHeight = NULL);
            int     MeasureSite (CLayout   *pLayout,
                                 CCalcInfo *pci,
                                 LONG       xWidthMax,
                                 BOOL       fBreakAtWord,
                                 SIZE      *psizeObj,
                                 int       *pxMinWidth);
    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
            void    CalcTextSize(CCalcInfo * pci, SIZE * psize, SIZE *psizeDefault = NULL);

    virtual void    GetPositionInFlow(CElement * pElement, CPoint * ppt);
    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);

    virtual void        GetFlowPosition(CDispNode * pDispNode, CPoint * ppt)
                        {
                            _dp.GetRelNodeFlowOffset(pDispNode, ppt);
                        }
    virtual void        GetFlowPosition(CElement * pElement, CPoint * ppt)
                        {
                            _dp.GetRelElementFlowOffset(pElement, ppt);
                        }

    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL, BOOL fForParent = TRUE) const;

    virtual void    TranslateRelDispNodes(const CSize & size, long lStart = 0)
                    {
                        Assert(_fContainsRelative);
                        _dp.TranslateRelDispNodes(size, lStart);
                    }

    virtual void    ZChangeRelDispNodes()
                    {
                        Assert(_fContainsRelative);
                        _dp.ZChangeRelDispNodes();
                    }

    // Message processing
    BOOL    IsZeroWidthWrapper();

    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage,
                                          CElement * pElem,
                                          CTreeNode *pNodeContext);
            HRESULT         HandleButtonDblClk(CMessage *pMessage);
            HRESULT         HandleMouseWheel(CMessage * pMessage);
            void            ExecReaderMode(CMessage * pMessage, BOOL fByMouse);
            void            ReaderModeScroll(int dx, int dy);
            HRESULT         HandleSysKeyDown(CMessage *pMessage);

    // Mouse cursor management
    virtual BOOL    OnSetCursor(CMessage *pMessage);
            HRESULT HandleSetCursor(CMessage *pMessage, BOOL fIsOverEmptyRegion);

    // Check whether an element is in the scope of this text site
    BOOL    IsElementBlockInContext (CElement *pElement);

    // Drag & drop
    virtual HRESULT PreDrag(DWORD dwKeyState,
                            IDataObject **ppDO, IDropSource **ppDS);
    virtual HRESULT PostDrag(HRESULT hr, DWORD dwEffect);
    virtual HRESULT Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT DragLeave();
    virtual HRESULT ParseDragData(IDataObject *pDO);
    virtual void    DrawDragFeedback();
    virtual HRESULT InitDragInfo(IDataObject *pDataObj, POINTL ptlScreen);
    virtual HRESULT UpdateDragFeedback(POINTL ptlScreen);

    // Get next text site in the specified direction from the specified position
    virtual CFlowLayout *GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL);
    virtual CFlowLayout *GetFlowLayoutAtPoint(POINT pt) { return this; }
            CFlowLayout *GetPositionInFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, LONG *pcp, BOOL *pfCaretNotAtBOL);

            LONG     XCaretToRelative(LONG xCaret);
            LONG     XCaretToAbsolute(LONG xCaretReally);

    // Command processing
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext,
            BOOL fStopBobble);
    virtual HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut,
            BOOL fStopBobble);

    // Currency and UI Activation helper
    //
    HRESULT YieldCurrencyHelper(CElement * pElemNew);
    HRESULT BecomeCurrentHelper(long lSubDivision,
                                BOOL *pfYieldFailed = NULL,
                                CMessage * pMessage = NULL);

    // Pagination support
    HRESULT Paginate(LONG * plViewWidth, LONG yInitialPageHeight = 0, BOOL fFillLastPage = TRUE);
    virtual HRESULT PageBreak(LONG yStart, LONG yIdeal, CStackPageBreaks * paryValues);
    virtual HRESULT ContainsPageBreak(long yTop, long yBottom, long * pyBreak, BOOL * pfPageBreakAfterAtTopLevel = NULL);


#if DBG==1 || defined(DUMPTREE)
    void DumpLines ( ) { GetDisplay()->DumpLines(); }
#endif

    // Helper functions
    // long LineCount() const { return (_pLineArray ? _pLineArray->Count() : 0); }

    CDisplay * GetDisplay() { return &_dp; }
    CQuillGlue * GetQuillGlue() { return _pQuillGlue; }

    HRESULT     CreateControlRange(int cnt, CLayout ** ppLayout, IDispatch ** ppDisp);

    // Helper methods

    virtual HRESULT Init();
    virtual HRESULT OnExitTree();

    // member data
    union
    {
        DWORD   dwFlags;                // describe the type of element
                                        // layout
        struct
        {
            unsigned  _fTxtStyle                : 1;  // TRUE/FALSE - opaque/transparent

            unsigned  _fAllowBeep               : 1;    // Allow beep at doc boundaries
            unsigned  _fPassword                : 1;
            unsigned  _fRecalcDone              : 1;
            unsigned  _fSizeCalculated          : 1;
#if DBG==1
            unsigned  _fLocked                  : 1;
#endif
            unsigned  _fListen                  : 1;    // Listen to notifications
            unsigned  _fTopDown                 : 1;    // BUGBUG: Remove once notifications are precise (brendand)
            unsigned  _fContainsAbsolute        : 1;
            unsigned  _fChildWidthPercent       : 1;
            unsigned  _fChildHeightPercent      : 1;
            unsigned  _fPreserveMinMax          : 1;

            unsigned  _fAutoScroll              : 1;
            unsigned  _fCapture                 : 1;    // Control has mouse capture

            // CTableCell flags
            unsigned  _fInheritedWidth          : 1;    // cuvWidth in CFancyFormat is inherited
            unsigned  _fEatLeftDown             : 1;    // eat the next left down?

            unsigned  _fSizeToContent           : 1;    // TRUE if a 1DLayout is sizing to content
        };
    };

    CDirtyTextRegion    _dtr;

    // CDispClient overrides
    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData);

    virtual void NotifyScrollEvent();

#if DBG==1
    virtual void DumpDebugInfo(
                HANDLE         hFile,
                long           level,
                long           childNumber,
                CDispNode *    pDispNode,
                void *         cookie);
#endif

protected:
    CDisplay            _dp;
    CQuillGlue *        _pQuillGlue;

    LONG                _xMin;
    LONG                _xMax;

private:

    CDropTargetInfo* _pDropTargetSelInfo;
};


//+----------------------------------------------------------------------------
//
//  Class:      C1DLayout
//
//  Synopsis:   A CFlowLayout derivative used for arbitrary text containers
//              (as opposed to specific ones which use their own
//               CFlowLayout-derivative - e.g., TD, BODY)
//
//-----------------------------------------------------------------------------
class C1DLayout: public CFlowLayout
{
public:
    typedef CFlowLayout super;

    // tear off interfaces
    //
    // constructor & destructor
    //
    C1DLayout(CElement * pElement1DLayout) : CFlowLayout(pElement1DLayout)
    {
    }
    ~C1DLayout() {};

    virtual HRESULT Init();

    virtual BOOL GetAutoSize() const
    {
        return _fContentsAffectSize;
    }
    virtual DWORD CalcSize(CCalcInfo * pci,
                           SIZE      * psize,
                           SIZE      * psizeDefault = NULL);

    virtual void    Notify(CNotification *pNF);

protected:
    DWORD RecalcText(CCalcInfo * pci,
                     SIZE      * psize,
                     SIZE * psizeDefault = NULL);

    void CalcAbsoluteSize(CCalcInfo* pci,
                          SIZE* psize,
                          long* pcxWidth,
                          long* pcyHeight,
                          long* pxLeft,
                          long* pxRight,
                          long* pxTop,
                          long* pxBottom,
                          BOOL  fClipContent);

    DECLARE_LAYOUTDESC_MEMBERS;

    // History support
    //
    ULONG       _uHistory;

};


//+--------------------------------------------------------
// CDisplay::BgRecalcInfo accessors
//+--------------------------------------------------------

inline CBgRecalcInfo * CDisplay::BgRecalcInfo()   { return GetFlowLayout()->BgRecalcInfo();        }
inline HRESULT CDisplay::EnsureBgRecalcInfo()     { return GetFlowLayout()->EnsureBgRecalcInfo();  }
inline void CDisplay::DeleteBgRecalcInfo()        {        GetFlowLayout()->DeleteBgRecalcInfo();  }
inline BOOL CDisplay::HasBgRecalcInfo()     const { return GetFlowLayout()->HasBgRecalcInfo();     }
inline BOOL CDisplay::CanHaveBgRecalcInfo() const { return GetFlowLayout()->CanHaveBgRecalcInfo(); }
inline LONG CDisplay::YWait()               const { return GetFlowLayout()->YWait();               }
inline LONG CDisplay::CPWait()              const { return GetFlowLayout()->CPWait();              }
inline CRecalcTask * CDisplay::RecalcTask() const { return GetFlowLayout()->RecalcTask();          }
inline DWORD CDisplay::BgndTickMax()        const { return GetFlowLayout()->BgndTickMax();         }

inline CFlowLayout *
CDisplay::GetFlowLayout() const
{
    return CONTAINING_RECORD(this, CFlowLayout, _dp);
}

//+--------------------------------------------------------
// CDisplay::GetFirstVisiblexxxx routines
//+--------------------------------------------------------

inline LONG
CDisplay::GetFirstVisibleCp() const
{
    CRect   rc;
    long    cp;

    GetFlowLayout()->GetClientRect(&rc);
    LineFromPos(rc, NULL, &cp, LFP_IGNORERELATIVE);
    return cp;
}

inline LONG
CDisplay::GetFirstVisibleLine() const
{
    CRect   rc;

    GetFlowLayout()->GetClientRect(&rc);
    return LineFromPos(rc, NULL, NULL, LFP_IGNORERELATIVE);
}


#endif  // ndef QUILL

#pragma INCMSG("--- End 'qlayout.hxx'")
#else
#pragma INCMSG("*** Dup 'qlayout.hxx'")
#endif
