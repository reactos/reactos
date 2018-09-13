/*
 *  QDISP.H
 *
 *  Purpose:
 *      CDisplay class
 *
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 *
 *  Quill fork 02/23/98.
 */

#ifndef I_QDISP_H_
#define I_QDISP_H_
#pragma INCMSG("--- Beg 'qdisp.h'")

#ifdef QUILL

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_LSTCACHE_HXX_
#define X_LSTCACHE_HXX_
#include "lstcache.hxx"
#endif

class CFlowLayout;
class CBgRecalcInfo;

class CDisplay;
class CLed;
class CLine;
class CLinePtr;
class CMeasurer;
class CLSMeasurer;
class CTxtSite;
class CRecalcLinePtr;
class CTxtRange;
class CRecalcTask;
class CLSRenderer;
class CNotification;

class CShape;
class CWigglyShape;
enum NAVIGATE_DIRECTION;

enum
{
    PADDING_TOP = 0,
    PADDING_LEFT,
    PADDING_RIGHT,
    PADDING_BOTTOM,
    PADDING_MAX
};

class CDrawInfoRE;
class CSelection;

// Helper
long ComputeLineShift(htmlAlign  atAlign,
                      BOOL       fRTLDisplay,
                      BOOL       fRTLLine,
                      BOOL       fMinMax,
                      long       xWidthMax,
                      long       xWidth,
                      UINT *     puJustified);

// ============================  CLed  ====================================
// Line Edit Descriptor - describes impact of an edit on line breaks

MtExtern(CLed)
MtExtern(CRelDispNodeCache)

class CLed
{
public:
    LONG _cpFirst;          // cp of first affected line
    LONG _iliFirst;         // index of first affected line
    LONG _yFirst;           // y offset of first affected line

    LONG _cpMatchOld;       // pre-edit cp of first matched line
    LONG _iliMatchOld;      // pre-edit index of first matched line
    LONG _yMatchOld;        // pre-edit y offset of first matched line

    LONG _cpMatchNew;       // post-edit cp of first matched line
    LONG _iliMatchNew;      // post-edit index of first matched line
    LONG _yMatchNew;        // post-edit y offset of bottom of first matched line

public:
    CLed();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CLed))

    VOID    SetNoMatch();
};

class CAlignedLine
{
public:
    CLine * _pLine;
    LONG    _yLine;
};

class CRelDispNode
{
public:
    long        _ili;
    long        _yli;
    long        _cLines;
    CPoint      _ptOffset;
    CElement  * _pElement;
    CDispNode * _pDispNode;
};

class CRelDispNodeCache : public CDispClient
{
public:

    CRelDispNodeCache(CDisplay * pdp) : _aryRelDispNodes(Mt(CRelDispNodeCache))
    {
        _pdp = pdp;
    }

    virtual void            GetOwner(
                                CDispNode * pDispNode,
                                void ** ppv);

    virtual void            DrawClient(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                IDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags);

    virtual void            DrawClientBackground(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                IDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual void            DrawClientBorder(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                IDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual void            DrawClientScrollbar(
                                int whichScrollbar,
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                LONG contentSize,
                                LONG containerSize,
                                LONG scrollAmount,
                                IDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual void            DrawClientScrollbarFiller(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                IDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual BOOL            HitTestContent(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData);

    virtual BOOL            HitTestScrollbar(
                                int whichScrollbar,
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData)
    {
        return FALSE;
    }

    virtual BOOL            HitTestScrollbarFiller(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData)
    {
        return FALSE;
    }

    virtual BOOL            HitTestBorder(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData)
    {
        return FALSE;
    }

    virtual BOOL            ProcessDisplayTreeTraversal(
                                void *pClientData)
    {
        return TRUE;
    }
                                          
    
    // called only for z ordered items
    virtual LONG            GetZOrderForSelf()
    {
        return 0;
    }

    virtual LONG            GetZOrderForChild(
                                void *cookie)
    {
        return 0;
    }

    virtual void            HandleViewChange(
                                DWORD          flags,
                                const RECT *   prcClient,
                                const RECT *   prcClip,
                                CDispNode *    pDispNode)
    {
    }


    // provide opportunity for client to fire_onscroll event
    virtual void            NotifyScrollEvent ()
    {
    }


    virtual DWORD           GetClientLayersInfo()
    {
        return 0;
    }

    virtual void            DrawClientLayers(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                IDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

#if DBG==1
    virtual void            DumpDebugInfo(
                                HANDLE hFile,
                                long level,
                                long childNumber,
                                CDispNode *pDispNode,
                                void* cookie) {}
#endif
    CRelDispNode *  operator [] (long i) { return &_aryRelDispNodes[i]; }
    long            Size()               { return _aryRelDispNodes.Size(); }
    void            DestroyDispNodes();
    CDispNode *     FindElementDispNode(CElement * pElement);

    void InsertAt(long iPos, CRelDispNode & rdn)
    {
        _aryRelDispNodes.InsertIndirect(iPos, &rdn);
    }
    void Delete(long iPosFrom, long iPosTo)
    {
        _aryRelDispNodes.DeleteMultiple(iPosFrom, iPosTo);
    }

    CDisplay    * GetDisplay()      { return _pdp; }

private:
    CDataAry <CRelDispNode> _aryRelDispNodes;
    CDisplay *              _pdp;
};

inline CLed::CLed()
{
}

// ==========================  CDisplay  ====================================
// display - keeps track of line breaks for a device
// all measurements are in pixels on rendering device,
// EXCEPT xWidthMax and yHeightMax which are in twips

MtExtern(CDisplay)

class CDisplay : public CLineArray, public CListCache
{
    friend class CFlowLayout;
    friend class CLinePtr;
    friend class CLed;
    friend class CBodyElement;
    friend class CTxtSite;
    friend class CMarquee;
    friend class CRecalcLinePtr;
    friend class CLSMeasurer;
	friend class CQuillGlue;		// for _yHeightMax

protected:

    DWORD _fInBkgndRecalc          :1; // avoid reentrant background recalc
    DWORD _fLineRecalcErr          :1; // error occured during background recalc
    DWORD _fNoUpdateView           :1; // don't update visible view
    DWORD _fWordWrap               :1; // word wrap text

    DWORD _fWrapLongLines          :1; // true if we want to wrap long lines
    DWORD _fRecalcDone             :1; // is line recalc done ?
    DWORD _fUpdateCaret            :1; // Whether Draw needs to update the cursor
    DWORD _fNoContent              :1; // if there is no real content, table cell's compute
                                       // width differently.

    DWORD _dxCaret                 :2; // caret width, 1 for edit 0 for browse
    DWORD _fMinMaxCalced           :1; // Min/max size is valid and cached

    DWORD _fPrinting               :1; // Display is used for printing
    DWORD _fRecalcMost             :1; // Do we recalc the most neg/pos lines?
    DWORD _fRTL                    :1; // TRUE if outer flow is right-to-left

    DWORD _fHasEmbeddedLayouts     :1; // TRUE if we have layouts inside
    DWORD _fHasRelDispNodeCache    :1; // TRUE if we have a relative disp node cache

    LONG  _dcpCalcMax;                 // last cp for which line breaks have been calc'd + 1
    LONG  _yCalcMax;                   // height of calculated lines

    LONG  _yHeightMax;                 // max height of this display (-1 for infinite)
    LONG  _xWidth;                     // width of longest calculated line
    LONG  _yHeight;                    // sum of heights of calculated lines

    LONG  _yMostNeg;                   // Largest negative offset that a line or its contents
                                       // extend from the actual y offset of any given line

    LONG  _yMostPos;

    LONG  _xWidthView;                 // view rect width
    LONG  _yHeightView;                // view rect height

#if DBG == 1
    CFlowLayout * _pFL;
#endif


private:

    // Layout
    BOOL    RecalcPlainTextSingleLine(CCalcInfo * pci);
    BOOL    RecalcLines(CCalcInfo * pci);
    BOOL    RecalcLinesWithMeasurer(CCalcInfo * pci, CLSMeasurer * pme);
    BOOL    RecalcLines(CCalcInfo * pci,
                        long cpFirst,
                        LONG cchOld,
                        LONG cchNew,
                        BOOL fBackground,
                        CLed *pled,
                        BOOL fHack = FALSE);
    BOOL    AllowBackgroundRecalc(CCalcInfo * pci, BOOL fBackground = FALSE);

    LONG    CalcDisplayWidth();

    void    NoteMost(CLine *pli);
    void    RecalcMost();

    // Helpers
#ifndef DISPLAY_TREE
    inline void InitVars();
#endif
    BOOL    CreateEmptyLine(CLSMeasurer * pMe,
                            CRecalcLinePtr * pRecalcLinePtr,
                            LONG * pyHeight, BOOL fHasEOP );

    void    DrawBackgroundAndBorder(long cpIn, CFormDrawInfo *pDI,
                                    LONG ili, LONG lCount,
                                    LONG * piliDrawn, LONG yLi, RECT rcView, RECT rcClip,
                                    long cpLim);

    void    DrawElementBackground(CTreeNode *,
                                    CDataAry <RECT> * paryRects,  RECT * prcBound,
                                    const RECT * prcView, const RECT * prcClip,
                                    CFormDrawInfo * pDI);

    void    DrawElementBorder(CTreeNode *,
                                    CDataAry <RECT> * paryRects, RECT * prcBound,
                                    const RECT * prcView, const RECT * prcClip,
                                    CFormDrawInfo * pDI);

public:
    void    RecalcLineShift(CCalcInfo * pci, DWORD grfLayout);

    void    DrawRelElemBgAndBorder(
                     long            cp,
                     CTreePos      * ptp,
                     CRelDispNode  * prdn,
                     const RECT    * prcView,
                     const RECT    * prcClip,
                     CFormDrawInfo * pDI);

    void    DrawElemBgAndBorder(
                     CElement        *   pElementRelative,
                     CDataAry <RECT> *   paryRects,
                     const RECT      *   prcView,
                     const RECT      *   prcClip,
                     CFormDrawInfo   *   pDI,
                     CPoint          *   pptOffset,
                     BOOL                fDrawBackground,
                     BOOL                fDrawBorder,
                     LONG                cpStart = -1,       // default -1
                     LONG                cpFinish = -1,      // default -1
                     BOOL                fNonRelative = FALSE);


protected:

    void    InitLinePtr ( CLinePtr & );

#ifndef DISPLAY_TREE
    // Scrollbars
    void    CacheScrollBarWidths(CDocInfo * pdci, LONG xWidth);
#endif

    void ComputeVertPaddAndBordFromParentNode(
            CCalcInfo * pci,
            CTreePos * ptpStart, CTreePos * ptpFinish,
            LONG * pyPaddBordTop, LONG * pyPaddBordBottom);

public:
    LONG    _xMinWidth;         // min possible width with word break
    LONG    _xMaxWidth;         // max possible width without word break
    LONG    _yBottomMargin;     // bottom margin is not taken into account
                                // in lines. Left, Right margins of the
                                // TxtSite are accumulated in _xLeft & _xRight
                                // of each line respectively

#ifndef DISPLAY_TREE
    //
    // NOTE(SujalP): Please do not call these 2 functions directly. Only
    // CTxtSite::GetMax[XY]Scroll() should be calling these 2 functions.
    // To obtain max scroll values, always use the functions GetMax[XY]Scroll()
    // on the txtsite. This is because CBodyElement overrides these functions
    // and returns different values when we have absolutely positioned sites.
    //
    LONG GetMaxXScrollFromDisplay() const
    {
        return max(0L, (GetWidth() - _xWidthView));
    }
    LONG GetMaxYScrollFromDisplay() const
    {
        return max(0L, (_yHeightMax + _yBottomMargin - _yHeightView));
    }
    //
    // NOTE (SujalP): End
    //
#endif

    HRESULT SetListIndex( long cpIn, LONG ili );

    HRESULT SetListIndexCache( WORD wLevel,
                            struct CListIndex * pLI, enum INDEXCACHE e);
#ifndef DISPLAY_TREE
    void    ResyncListIndex( LONG iliOldFirstVisible );
#else
    void    ResyncListIndex(long yScroll);
#endif
    CTreeNode *FormattingNodeForLine(LONG         cpForLine,    // IN
                                     CTreePos    *ptp,          // IN
                                     LONG         cchLine,      // IN
                                     LONG        *pcch,         // OUT
                                     CTreePos   **pptp) const;  // OUT

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDisplay))

    CDisplay ();
    ~CDisplay();

    BOOL    Init();

    CFlowLayout *   GetFlowLayout() const;
    CElement *      GetFlowLayoutElement() const;
    CMarkup *       GetMarkup() const;

    void    Detach();

    // Support for vertical writing
    VOID    ConvGetRect(LPRECT prc);
    VOID    ConvSetRect(LPRECT prc);


    // Getting properties

    BOOL    GetWordWrap() const             { return _fWordWrap; }
    void    SetWordWrap(BOOL fWrap)         { _fWordWrap = fWrap; }

    BOOL    GetWrapLongLines() const        { return _fWrapLongLines; }
    void    SetWrapLongLines(BOOL fWrapLongLines)
                                            { _fWrapLongLines = fWrapLongLines; }
    BOOL    Printing() const                { return _fPrinting; }
    BOOL    NoContent() const               { return _fNoContent; }

    // maximum height and width
    LONG    GetMaxWidth() const             { return max(long(_xWidthView), GetWidth()); }
    LONG    GetMaxHeight() const            { return max(long(_yHeightView), GetHeight()); }

#ifndef DISPLAY_TREE
    // maximum viewable height and width
    LONG    GetMaxViewableWidth() const;
    LONG    GetMaxViewableHeight() const;
#endif

    LONG    GetMaxPixelWidth() const
    {
        return GetViewWidth();
    }

    LONG    GetCaret() const                { return _dxCaret; }

    // flow direction
    BOOL    IsRTL() const                   { return _fRTL; }
    void    SetRTL(BOOL fRTL)               { _fRTL = fRTL; }

    // Width, height and line count (all text)
    LONG    GetWidth() const                { return (_xWidth + _dxCaret); }
    LONG    GetHeight() const               { return (_yHeightMax + _yBottomMargin); }
#ifdef DISPLAY_TREE
    void    GetSize(CSize * psize) const
            {
                psize->cx = GetWidth();
                psize->cy = GetHeight();
            }
    void    GetSize(SIZE * psize) const
            {
                GetSize((CSize *)psize);
            }
#endif
    LONG    LineCount() const               { return CLineArray::Count(); }

    // View rectangle
    LONG    GetViewWidth() const            { return _xWidthView; }
    LONG    GetViewHeight() const           { return _yHeightView; }
    BOOL    SetViewSize(const RECT &rcView, BOOL fViewUpdate=TRUE);

    void    GetViewWidthAndHeightForChild(
                CParentInfo * ppri,
                long        * pxWidth,
                long        * pyHeight,
                BOOL fMinMax = FALSE) const;

    void    GetPadding(CParentInfo * ppri, long lPadding[], BOOL fMinMax = FALSE) const;

#ifndef DISPLAY_TREE
    // Visible view properties
    LONG    GetCliVisible(LONG *pcpMostVisible = NULL) const;
    LONG    GetFirstCp() const;
    LONG    GetLastCp() const;
    LONG    GetFirstVisibleCp() const;
    LONG    GetMaxCpCalced() const;
    inline LONG GetFirstVisibleLine() const;
    inline LONG GetFirstVisibleDCp() const;
    inline LONG GetFirstVisibleDY() const;
#else
    LONG    GetFirstCp() const;
    LONG    GetLastCp() const;
    inline LONG GetFirstVisibleCp() const;
    inline LONG GetFirstVisibleLine() const;
    LONG    GetMaxCpCalced() const;
#endif

    // Line info
    LONG    GetLineText(LONG ili, TCHAR *pchBuff, LONG cchMost);
    LONG    CpFromLine(LONG ili, LONG *pyLine = NULL);
    void    Notify(CNotification * pnf);

    const CCharFormat * CFFromLine(CLine * pli, LONG cp);

    LONG    YposFromLine(LONG ili, CCalcInfo * pci = NULL);
    LONG    YposFromLineDebug(LONG ili);

#ifndef DISPLAY_TREE
    typedef enum {
        ISF_DONTIGNORE_LOOKFURTHER,
        ISF_DONTIGNORE_DONTLOOKFURTHER
    } IGNORESTATUSFLAG;
#endif

    void    RcFromLine(RECT *prc, LONG top, LONG xLead, CLine *pli);

#ifndef DISPLAY_TREE
    void    FindVisualStartLine(LONG &ili, CLine *&pli, LONG &cpLi, LONG &yLi);
    LONG    LineFromPos(
                    LONG xPos,
                    LONG yPos,
                    BOOL fHitTesting,
                    BOOL fSkipFrame = TRUE,
                    LONG *pyLine = NULL,
                    LONG *pcpFirst = NULL,
                    LONG xExposedWidth = 0,
                    IGNORESTATUSFLAG isfIgnoreFlag = ISF_DONTIGNORE_DONTLOOKFURTHER,
                    CCalcInfo * pci = NULL,
                    BOOL fIgnoreRelLines = TRUE);
#else
    enum LFP_FLAGS
    {
        LFP_ZORDERSEARCH    = 0x00000001,   // Hit lines on a z-order basis (default is source order)
        LFP_IGNOREALIGNED   = 0x00000002,   // Ignore frame lines (those for aligned content)
        LFP_IGNORERELATIVE  = 0x00000004,   // Ignore relative lines
        LFP_INTERSECTBOTTOM = 0x00000008    // Intersect at the bottom (instead of the top)
    };

    LONG    LineFromPos(
                    const CRect &   rc,
                    DWORD           grfFlags = (LFP_IGNOREALIGNED | LFP_IGNORERELATIVE)) const
            {
                return LineFromPos(rc, NULL, NULL, grfFlags);
            }
    LONG    LineFromPos (
                    const CRect &   rc,
                    LONG *          pyLine,
                    LONG *          pcpLine,
                    DWORD           grfFlags = (LFP_IGNOREALIGNED | LFP_IGNORERELATIVE)) const;
#endif
    LONG    LineFromCp(LONG cp, BOOL fAtEnd) ;

    // Point <-> cp conversion
    LONG    CpFromPointReally(
         POINT pt,                     // Point to compute cp at (client coords)
         CLinePtr * const prp,         // Returns line pointer at cp (may be NULL)
         CTreePos ** pptp,             // tree pos corresponding to the cp returned
         BOOL fAllowEOL,               // Click at EOL returns cp after CRLF
         BOOL fIgnoreBeforeAfterSpace = FALSE, // TRUE if ignoring pbefore/after space
         BOOL fExactFit = FALSE,               // TRUE if cp should always round down (for element-hit-testing)
         BOOL * pfRightOfCp = NULL);

    LONG    CpFromPoint(POINT pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout ** ppLayout,
                        BOOL fAllowEOL,
                        BOOL fIgnoreBeforeAfterSpace = FALSE,
                        BOOL fExactFit = FALSE,
                        BOOL *pfRightOfCp = NULL,
                        BOOL *pfPsuedoHit = NULL,
                        CCalcInfo * pci = NULL);

    LONG    CpFromPointRelative(POINT pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout ** ppLayout,
                        BOOL fAllowEOL,
                        BOOL fIgnoreBeforeAfterSpace = FALSE,
                        BOOL fExactFit = FALSE,
                        BOOL *pfRightOfCp = NULL,
                        BOOL *pfPsuedoHit = NULL,
                        CCalcInfo *pci = NULL);

    LONG    CpFromPointEx(LONG ili,
                        LONG  yLine,
                        LONG  cp,
                        POINT pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout ** ppLayout,
                        BOOL fAllowEOL,
                        BOOL fIgnoreBeforeAfterSpace,
                        BOOL fExactFit,
                        BOOL * pfRightOfCp,
                        BOOL *pfPsuedoHit,
                        CCalcInfo * pci);

    LONG    PointFromTp (
                    LONG cp,
                    CTreePos * ptp,
                    BOOL fAtEnd,
                    POINT &pt,
                    CLinePtr * const prp,
                    UINT taMode,
                    CCalcInfo * pci = NULL,
                    BOOL *pfComplexLine = NULL);

    LONG    RenderedPointFromTp (
                    LONG cp,
                    CTreePos * ptp,
                    BOOL fAtEnd,
                    POINT &pt,
                    CLinePtr * const prp,
                    UINT taMode,
                    CCalcInfo * pci = NULL);

    enum RFE_FLAGS
    {
        RFE_SCREENCOORD  = 1,
        RFE_CLIPPED      = 2,
        RFE_RANGE_METRIC = 4,
        RFE_NONRELATIVE  = 8,
    };

    void    RegionFromElement(
                        CElement       * pElement,
                        CDataAry<RECT> * paryRects,
                        CPoint         * pptOffset = NULL,
                        CFormDrawInfo  * pDI = NULL,
                        DWORD            dwFlags  = 0,
                        long             cpStart  = -1,
                        long             cpFinish = -1,
                        RECT *           prcBoundingRect = NULL);
#ifdef DISPLAY_TREE
    void    RegionFromRange(
                    CDataAry<RECT> *    paryRects,
                    long                cp,
                    long                cch);
#endif

    CFlowLayout *MoveLineUpOrDown(NAVIGATE_DIRECTION iDir, CLinePtr& rp, LONG xCaret, LONG *pcp, BOOL *pfCaretNotAtBOL);
    CFlowLayout *NavigateToLine  (NAVIGATE_DIRECTION iDir, CLinePtr& rp, POINT pt,    LONG *pcp, BOOL *pfCaretNotAtBOL);
    BOOL      IsTopLine(CLinePtr& rp);
    BOOL      IsBottomLine(CLinePtr& rp);

    // Line break recalc.
    void    FlushRecalc();
    BOOL    RecalcView(CCalcInfo * pci, BOOL fFullRecalc);
    BOOL    UpdateView(CCalcInfo * pci, long cpFirst, LONG cchOld, LONG cchNew);
    BOOL    UpdateViewForLists(RECT *prcView, long cpFirst,
                               long  iliFirst, long  yPos,  RECT *prcInval);

    // Rendering
    VOID    EraseBkgnd(HDC hdc);
    HRESULT Draw(CFormDrawInfo *pDI, RECT * prcView);

    // Background recalc
    VOID    StartBackgroundRecalc(DWORD grfLayout);
    VOID    StepBackgroundRecalc(DWORD dwTimeOut, DWORD grfLayout);
    VOID    StopBackgroundRecalc();
    BOOL    WaitForRecalc(LONG cpMax, LONG yMax, CCalcInfo * pci = NULL);
    BOOL    WaitForRecalcIli(LONG ili, CCalcInfo * pci = NULL);
    BOOL    WaitForRecalcView(CCalcInfo * pci = NULL);
    inline CBgRecalcInfo * BgRecalcInfo();
    inline HRESULT EnsureBgRecalcInfo();
    inline void DeleteBgRecalcInfo();
    inline BOOL HasBgRecalcInfo() const;
    inline BOOL CanHaveBgRecalcInfo() const;
    //inline LONG DCpCalcMax() const;
    //inline LONG YCalcMax() const;
    inline LONG YWait() const;
    inline LONG CPWait() const;
    inline CRecalcTask * RecalcTask() const;
    inline DWORD BgndTickMax() const;

#ifndef DISPLAY_TREE
    // Scrolling
    inline BOOL HasScrollInfo() const;
    inline CScrollInfo * ScrollInfo();
    inline LONG GetXScroll() const;
    inline LONG GetYScroll() const;
    inline LONG GetXScrollEx() const;
    inline LONG XVScrollBarWidth() const;
    VOID    HScrollN(WORD wCode, LONG iliCount);
    VOID    HScroll(WORD wCode, LONG xPos, BOOL fRepeating);
    VOID    VScrollPercent(WORD wCode, LONG yPercent, BOOL fRepeating);
    VOID    VScrollN(WORD wCode, LONG iliCount);
    LRESULT VScroll(WORD wCode, LONG xPos, BOOL fFixed, BOOL fRepeating = FALSE);
    BOOL    ScrollMore(POINT *ppt, POINT *pptDelta);
    void    ScrollMoreHorizontal(POINT *ppt,
                                 RECT  *prc,
                                 LONG  *pxScroll,
                                 BOOL  *pfScrollClientRect,
                                 BOOL  *pfScrollContents);
    void    ScrollMoreVertical(POINT *ppt,
                               RECT  *prc,
                               LONG  *pyScroll,
                               BOOL  *pfScrollClientRect,
                               BOOL  *pfScrollContents);

    VOID    ScrollToLineStart();
    LONG    CalcYLineScrollDelta (LONG cli, BOOL fFractionalFirst );
    BOOL    DragScroll(const POINT * ppt);   // outside of client rect.
    BOOL    AutoScroll( POINT pt, const WORD xScrollInset, const WORD yScrollInset );
    BOOL    ScrollView(CPositionInfo * ppi,
                       LONG xScroll, LONG yScroll,
                       BOOL fTracking, BOOL fFractionalScroll,
                       BOOL fRepeating = FALSE, BOOL fDisableSmoothScrolling = FALSE);

    // Scrollbars
    LONG    GetScrollRange(INT nBar) const;
    void    UpdateScrollBars(CDocInfo * pdci);
#endif

    // Selection
    void ShowSelected(CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected);

    HRESULT GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape);
    HRESULT GetWigglyFromLineRange(CCalcInfo * pDI, long cp, long cch, CWigglyShape * pShape);


    void TextSelectSites ( CTreePos * ptpStart, CTreePos* ptpEnd, BOOL fSelected );

    BOOL HasEmbeddedLayouts()    { return _fHasEmbeddedLayouts; }

    //
    // Text change notification stuff
    //

#if DBG==1
    void    CheckLineArray();
    void    CheckView();
    BOOL    VerifyFirstVisible();
#endif
#if DBG==1 || defined(DUMPTREE)
    void DumpLines ( );
    void DumpLineText(HANDLE hFile, CTxtPtr* ptp, long iLine);
#endif

    // Misc helpers

    void    GetRectForChar(CCalcInfo   *pci,
                           LONG        *pTop,
                           LONG        *pBottom,
                           LONG        *pWidth,
                           WCHAR        ch,
                           LONG         yTop,
                           CLine       *pli,
                     const CCharFormat *pcf);

    void    GetTopBottomForCharEx(CCalcInfo *pci,  LONG  *pTop, LONG  *pBottom,
                                  LONG       yTop, CLine *pli,  LONG  xPos,
                            const CCharFormat *pcf);

    void    GetClipRectForLine(RECT *prcClip, LONG top, LONG xOrigin, CLine *pli) const;

    // Rendering
    LONG Render(CFormDrawInfo * pDI,
                const RECT &rcView,
                const RECT &rcRender,
                LONG iliStop = -1);

#ifndef DISPLAY_TREE
private:
    // Smooth scrolling info structure
    struct SMOOTHSCROLLINFO
    {
        CSelection *  _psel;          // IN: just caching it
        CFlowLayout          *  _pFlowLayoutSelOwner; // IN: just caching it
        RECT                    _rcView;        // IN: cache
        LONG                    _dx;            // OUT: the result delta (how moush was scrolled)
        LONG                    _dy;            // OUT: the result delta
        unsigned int            _fTracking :1;  // IN: cache
        unsigned int            _fRepeat :1;    // IN: cache
    };

    // Smooth scrolling helper functions
    BOOL    SmoothScrolling(CPositionInfo * ppi, LONG xScroll, LONG yScroll, SMOOTHSCROLLINFO *psinfo);
    BOOL    ScrollAction   (CPositionInfo * ppi, LONG xScroll, LONG yScroll, SMOOTHSCROLLINFO *psinfo,
                            RECT* prcInvalid);
    HRESULT PositionAndLayoutObjects(CPositionInfo * ppi, LONG dx, LONG dy);
#endif

public:
    BOOL IsLastTextLine(LONG ili);
    void SetCaretWidth(int dx) { Assert (dx >=0); _dxCaret = dx; }

#ifdef DISPLAY_TREE
    void    DestroyFlowDispNodes();

    CDispNode * AddLayoutDispNode(
                    CParentInfo *   ppi,
                    CTreeNode *     pTreeNode,
                    long            dx,
                    long            dy,
                    CDispNode *     pDispSibling);
    CDispNode * AddLayoutDispNode(
                    CParentInfo *   ppi,
                    CLayout *       pLayout,
                    long            dx,
                    long            dy,
                    CDispNode *     pDispSibling);
    CDispNode * GetPreviousDispNode(LONG cp);
    void        AdjustDispNodes(CDispNode * pDNAdjustFrom, CLed * pled);
#endif

    inline BOOL          HasRelDispNodeCache() const;
    HRESULT              SetRelDispNodeCache(void * pv);
    CRelDispNodeCache *  GetRelDispNodeCache() const;
    CRelDispNodeCache *  DelRelDispNodeCache();

protected:
    // Rel line support
    CRelDispNodeCache * EnsureRelDispNodeCache();

    void    UpdateRelDispNodeCache(CLed * pled);

    void    AddRelNodesToCache( long cpStart, LONG yPos,
                                LONG iliStart, LONG iliFinish,
                                CDataAry<CRelDispNode> * prdnc);

    void    VoidRelDispNodeCache();
#if DBG==1
    CRelDispNodeCache * _pRelDispNodeCache;
#endif
    CDispNode *     FindElementDispNode(CElement * pElement)
    {
        return HasRelDispNodeCache()
                ? GetRelDispNodeCache()->FindElementDispNode(pElement)
                : NULL;
    }
    void GetRelNodeFlowOffset(CDispNode * pDispNode, CPoint * ppt);
    void GetRelElementFlowOffset(CElement * pElement, CPoint * ppt);
    void TranslateRelDispNodes(const CSize & size, long lStart);
    void ZChangeRelDispNodes();
};

#define ALIGNEDFEEDBACKWIDTH    4

#ifndef DISPLAY_TREE
/*
 *  CDisplay::WaitForRecalcView()
 *
 *  @mfunc
 *      Ensure visible lines are completly recalced
 *
 *  @rdesc TRUE iff successful
 */
inline
BOOL CDisplay::WaitForRecalcView(CCalcInfo * pci)
{
    return WaitForRecalc(-1, GetYScroll() + _yHeightView, pci);
}
#else
inline CDispNode *
CDisplay::AddLayoutDispNode(
    CParentInfo *   ppi,
    CTreeNode *     pTreeNode,
    long            dx,
    long            dy,
    CDispNode *     pDispSibling)
{
    Assert(pTreeNode);
    Assert(pTreeNode->Element());
    Assert(pTreeNode->Element()->GetLayout());

    return AddLayoutDispNode(ppi, pTreeNode->Element()->GetLayout(), dx, dy, pDispSibling);
}

#endif

/*
 *  CDisplayL::InitLinePtr ( CLinePtr & plp )
 *
 *  @mfunc
 *      Initialize a CLinePtr properly
 */
inline
void CDisplay::InitLinePtr (
    CLinePtr & plp )        //@parm Ptr to line to initialize
{
    plp.Init( * this );
}

inline BOOL
CDisplay::HasRelDispNodeCache() const
{
    return _fHasRelDispNodeCache;
}

#if DBG!=1
#define CheckView()
#define CheckLineArray()
#endif

#endif  // QUILL

#pragma INCMSG("--- End 'qdisp.h'")
#else
#pragma INCMSG("*** Dup 'qdisp.h'")
#endif
