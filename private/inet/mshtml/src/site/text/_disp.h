/*
 *  _DISP.H
 *
 *  Purpose:
 *      DISP class
 *
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#ifndef I_DISP_H_
#define I_DISP_H_
#pragma INCMSG("--- Beg '_disp.h'")

#ifdef QUILL
#include "..\lequill\qdisp.h"
#else

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

class CFlowLayout;
class CBgRecalcInfo;

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

class CLed;
class CLine;
class CLinePtr;
class CMeasurer;
class CLSMeasurer;
class CTxtSite;
class CRecalcLinePtr;
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
                      UINT *     puJustified,
                      long *     pdxRemainder = NULL);

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

    CRelDispNode() 
    { 
        _pElement = NULL;
        _pDispNode = NULL;
    };

    ~CRelDispNode() 
        {  ClearContents();  };

    // Helper Functions
    //----------------------------

    void ClearContents ()
    {
        if (_pElement)
        {
            _pElement->Release();
            _pElement = NULL;
        }

        if (_pDispNode)
        {
            _pDispNode->Destroy();
            _pDispNode = NULL;
        }
    }

    CElement * GetElement () { return _pElement; }

    void SetElement( CElement * pNewElement )
    {
        if (_pElement)
            _pElement->Release();

        _pElement = pNewElement;

        if (_pElement)
            _pElement->AddRef();
    }

    // Data Members 
    //----------------------------
    long        _ili;
    long        _yli;
    long        _cLines;
    CPoint      _ptOffset;
    CDispNode * _pDispNode;

private:
    CElement  * _pElement;
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
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *cookie,
                                void *pClientData,
                                DWORD dwFlags);

    virtual void            DrawClientBackground(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags);

    virtual void            DrawClientBorder(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
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
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual void            DrawClientScrollbarFiller(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
                                CDispNode *pDispNode,
                                void *pClientData,
                                DWORD dwFlags)
    {
    }

    virtual BOOL            HitTestContent(
                                const POINT *pptHit,
                                CDispNode *pDispNode,
                                void *pClientData);

    virtual BOOL            HitTestFuzzy(
                                const POINT *pptHitInParentCoords,
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

    virtual LONG            CompareZOrder(
                                CDispNode * pDispNode1,
                                CDispNode * pDispNode2);

    // we're not expecting to filter relative items directly
    virtual CDispFilter*    GetFilter()
    {
        Assert(FALSE);
        return NULL;
    }
    
    virtual void            HandleViewChange(
                                DWORD          flags,
                                const RECT *   prcClient,
                                const RECT *   prcClip,
                                CDispNode *    pDispNode)
    {
    }


    // provide opportunity for client to fire_onscroll event
    virtual void            NotifyScrollEvent(
                                RECT *  prcScroll,
                                SIZE *  psizeScrollDelta)
    {
    }


    virtual DWORD           GetClientLayersInfo(CDispNode *pDispNodeFor)
    {
        return 0;
    }

    virtual void            DrawClientLayers(
                                const RECT* prcBounds,
                                const RECT* prcRedraw,
                                CDispSurface *pSurface,
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
    void            SetElementDispNode(CElement * pElement, CDispNode * pDispNode);
    void            EnsureDispNodeVisibility(CElement * pElement = NULL);
    void            HandleDisplayChange();
    void            Delete(long iPosFrom, long iPosTo);

    // To handle invalidate notifications on relative elements:
    // See CFlowLayout::Notify() handling of invalidation.
    void            Invalidate( CElement *pElement, const RECT * prc = NULL, int nRects = 1 );

    void InsertAt(long iPos, CRelDispNode & rdn)
    {
        _aryRelDispNodes.InsertIndirect(iPos, &rdn);
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
;

class CDisplay : public CLineArray
{
    friend class CFlowLayout;
    friend class CLinePtr;
    friend class CLed;
    friend class CBodyElement;
    friend class CTxtSite;
    friend class CMarquee;
    friend class CRecalcLinePtr;
    friend class CLSMeasurer;
    friend class CCaret;

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
    DWORD _fHasMultipleTextNodes   :1; // TRUE if we have more than one disp node for text flow

    DWORD _fNavHackPossible        :1; // TRUE if we can have the NAV BR hack
    DWORD _fContainsHorzPercentAttr :1; // TRUE if we've handled an element that has horizontal percentage attributes (e.g. indents, padding)
    DWORD _fContainsVertPercentAttr :1; // TRUE if we've handled an element that has vertical percentage attributes (e.g. indents, padding)

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
    BOOL    CreateEmptyLine(CLSMeasurer * pMe,
                            CRecalcLinePtr * pRecalcLinePtr,
                            LONG * pyHeight, BOOL fHasEOP );

    void    DrawBackgroundAndBorder(CFormDrawInfo * pDI,
                                    long            cpIn,
                                    LONG            ili,
                                    LONG            lCount,
                                    LONG          * piliDrawn,
                                    LONG            yLi,
                                    const RECT    * rcView,
                                    const RECT    * rcClip,
                                    const CPoint  * ptOffset);

    void    DrawBackgroundAndBorder(long cpIn, CFormDrawInfo *pDI,
                                    LONG ili, LONG lCount,
                                    LONG * piliDrawn, LONG yLi, RECT rcView, RECT rcClip);

    void    DrawElementBackground(CTreeNode *,
                                    CDataAry <RECT> * paryRects,  RECT * prcBound,
                                    const RECT * prcView, const RECT * prcClip,
                                    CFormDrawInfo * pDI);

    void    DrawElementBorder(CTreeNode *,
                                    CDataAry <RECT> * paryRects, RECT * prcBound,
                                    const RECT * prcView, const RECT * prcClip,
                                    CFormDrawInfo * pDI);
    
    // Computes the indent for a given Node and a left and/or
    // right aligned site that a current line is aligned to.
    void    ComputeIndentsFromParentNode(CCalcInfo * pci, CTreeNode * pNode, DWORD dwFlags,
                                         LONG * pxLeftIndent, LONG * pxRightIndent);                                    

public:
    void    SetNavHackPossible()  { _fNavHackPossible = TRUE; }
    BOOL    GetNavHackPossible()  { return _fNavHackPossible; }
    
    void    RecalcLineShift(CCalcInfo * pci, DWORD grfLayout);
    void    RecalcLineShiftForNestedLayouts();

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
                     const CPoint    *   pptOffset,
                     BOOL                fDrawBackground,
                     BOOL                fDrawBorder,
                     LONG                cpStart = -1,       // default -1
                     LONG                cpFinish = -1,      // default -1
                     BOOL                fNonRelative = FALSE);


protected:

    void    InitLinePtr ( CLinePtr & );

    void ComputeVertPaddAndBordFromParentNode(
            CCalcInfo * pci,
            CTreePos * ptpStart, CTreePos * ptpFinish,
            LONG * pyPaddBordTop, LONG * pyPaddBordBottom);

public:
    LONG    _xMinWidth;             // min possible width with word break
    LONG    _xMaxWidth;             // max possible width without word break
    LONG    _yBottomMargin;         // bottom margin is not taken into account
                                    // in lines. Left, Right margins of the
                                    // TxtSite are accumulated in _xLeft & _xRight
                                    // of each line respectively

    CTreeNode *FormattingNodeForLine(LONG         cpForLine,    // IN
                                     CTreePos    *ptp,          // IN
                                     LONG         cchLine,      // IN
                                     LONG        *pcch,         // OUT
                                     CTreePos   **pptp,         // OUT
                                     BOOL        *pfMeasureFromStart) const;  // OUT

    CTreeNode* EndNodeForLine(LONG         cpEndForLine,               // IN
                              CTreePos    *ptp,                        // IN
                              LONG        *pcch,                       // OUT
                              CTreePos   **pptp,                       // OUT
                              CTreeNode  **ppNodeForAfterSpace) const; // OUT

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDisplay))

    CDisplay ();
    ~CDisplay();

    BOOL    Init();

    CFlowLayout *   GetFlowLayout() const;
    CElement *      GetFlowLayoutElement() const;
    CMarkup *       GetMarkup() const;

    void    Detach();

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
    void    GetSize(CSize * psize) const
            {
                psize->cx = GetWidth();
                psize->cy = GetHeight();
            }
    void    GetSize(SIZE * psize) const
            {
                GetSize((CSize *)psize);
            }
    LONG    LineCount() const               { return CLineArray::Count(); }

    // View rectangle
    LONG    GetViewWidth() const            { return _xWidthView; }
    LONG    GetViewHeight() const           { return _yHeightView; }
    BOOL    SetViewSize(const RECT &rcView, BOOL fViewUpdate=TRUE);

    void    GetViewWidthAndHeightForChild(
                CParentInfo * ppri,
                long        * pxWidth,
                long        * pyHeight,
                BOOL fMinMax = FALSE);

    void    GetPadding(CParentInfo * ppri, long lPadding[], BOOL fMinMax = FALSE);

    LONG    GetFirstCp() const;
    LONG    GetLastCp() const;
    inline LONG GetFirstVisibleCp() const;
    inline LONG GetFirstVisibleLine() const;
    LONG    GetMaxCpCalced() const;

    // Line info
    LONG    CpFromLine(LONG ili, LONG *pyLine = NULL) const;
    void    Notify(CNotification * pnf);

    LONG    YposFromLine(LONG ili, CCalcInfo * pci = NULL);

    void    RcFromLine(RECT *prc, LONG top, LONG xLead, CLine *pli);

    enum LFP_FLAGS
    {
        LFP_ZORDERSEARCH    = 0x00000001,   // Hit lines on a z-order basis (default is source order)
        LFP_IGNOREALIGNED   = 0x00000002,   // Ignore frame lines (those for aligned content)
        LFP_IGNORERELATIVE  = 0x00000004,   // Ignore relative lines
        LFP_INTERSECTBOTTOM = 0x00000008,   // Intersect at the bottom (instead of the top)
        LFP_EXACTLINEHIT    = 0x00000010,   // find the exact line hit, do not return the
                                            // closest line hit.
    };

    LONG    LineFromPos(
                    const CRect &   rc,
                    DWORD           grfFlags = 0) const
            {
                return LineFromPos(rc, NULL, NULL, grfFlags);
            }
    LONG    LineFromPos (
                    const CRect &   rc,
                    LONG *          pyLine,
                    LONG *          pcpLine,
                    DWORD           grfFlags = 0,
                    LONG            iliStart = -1,
                    LONG            iliFinish = -1) const;
    LONG    LineFromCp(LONG cp, BOOL fAtEnd);

    enum CFP_FLAGS
    {
        CFP_ALLOWEOL                = 0x0001,
        CFP_EXACTFIT                = 0x0002,
        CFP_IGNOREBEFOREAFTERSPACE  = 0x0004,
        CFP_NOPSEUDOHIT                 = 0x0008
    };

    // Point <-> cp conversion
    LONG    CpFromPointReally(
         POINT            pt,                   // Point to compute cp at (client coords)
         CLinePtr * const prp,                  // Returns line pointer at cp (may be NULL)
         CTreePos **      pptp,                 // tree pos corresponding to the cp returned
         DWORD            dwFlags,              
         BOOL *           pfRightOfCp = NULL,
         LONG *           pcchPreChars = NULL);

    LONG    CpFromPoint(POINT       pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout **  ppLayout,
                        DWORD       dwFlags,
                        BOOL *      pfRightOfCp = NULL,
                        BOOL *      pfPseudoHit = NULL,
                        LONG *      pcchPreChars = NULL,
                        CCalcInfo * pci = NULL);

    LONG    CpFromPointEx(LONG      ili,
                        LONG        yLine,
                        LONG        cp,
                        POINT       pt,
                        CLinePtr * const prp,
                        CTreePos ** pptp,             // tree pos corresponding to the cp returned
                        CLayout **  ppLayout,
                        DWORD       dwFlags,
                        BOOL *      pfRightOfCp,
                        BOOL *      pfPseudoHit,
                        LONG *      pcchPreChars,
                        CCalcInfo * pci);

    LONG    PointFromTp (
                    LONG cp,
                    CTreePos * ptp,
                    BOOL fAtEnd,
                    BOOL fAfterPrevCp,
                    POINT &pt,
                    CLinePtr * const prp,
                    UINT taMode,
                    CCalcInfo * pci = NULL,
                    BOOL *pfComplexLine = NULL,
                    BOOL *pfRTLFlow = NULL);

    LONG    RenderedPointFromTp (
                    LONG cp,
                    CTreePos * ptp,
                    BOOL fAtEnd,
                    POINT &pt,
                    CLinePtr * const prp,
                    UINT taMode,
                    CCalcInfo * pci = NULL);

    void    RegionFromElement(
                        CElement       * pElement,
                        CDataAry<RECT> * paryRects,
                        CPoint         * pptOffset = NULL,
                        CFormDrawInfo  * pDI = NULL,
                        DWORD            dwFlags  = 0,
                        long             cpStart  = -1,
                        long             cpFinish = -1,
                        RECT *           prcBoundingRect = NULL);
    void    RegionFromRange(
                    CDataAry<RECT> *    paryRects,
                    long                cp,
                    long                cch);

    CFlowLayout *MoveLineUpOrDown(NAVIGATE_DIRECTION iDir, CLinePtr& rp, LONG xCaret, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    CFlowLayout *NavigateToLine  (NAVIGATE_DIRECTION iDir, CLinePtr& rp, POINT pt,    LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    BOOL      IsTopLine(CLinePtr& rp);
    BOOL      IsBottomLine(CLinePtr& rp);

    // Line break recalc.
    void    FlushRecalc();
    BOOL    RecalcView(CCalcInfo * pci, BOOL fFullRecalc);
    BOOL    UpdateView(CCalcInfo * pci, long cpFirst, LONG cchOld, LONG cchNew);
    BOOL    UpdateViewForLists(RECT *prcView, long cpFirst,
                               long  iliFirst, long  yPos,  RECT *prcInval);

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

    // Selection
    void ShowSelected(CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected);

    HRESULT GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape);

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
    void DumpLineText(HANDLE hFile);
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
    void    Render( CFormDrawInfo * pDI,
                    const RECT    & rcView,
                    const RECT    & rcRender,
                    CDispNode     * pDispNode);

public:
    BOOL IsLastTextLine(LONG ili);
    void SetCaretWidth(int dx) { Assert (dx >=0); _dxCaret = dx; }

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
    CDispNode * GetPreviousDispNode(LONG cp, LONG iliStart);
    void        AdjustDispNodes(CDispNode * pDNAdjustFrom, CLed * pled);

    HRESULT HandleNegativelyPositionedContent(CLine       * pliNew,
                                              CLSMeasurer * pme,
                                              CDispNode   * pDNBefore,
                                              long          iLinePrev,
                                              long          yHeight);

    HRESULT InsertNewContentDispNode(CDispNode *  pDNBefore,
                                     CDispNode ** ppDispContent,
                                     long         iLine,
                                     long         yHeight);

    inline BOOL          HasRelDispNodeCache() const;
    HRESULT              SetRelDispNodeCache(void * pv);
    CRelDispNodeCache *  GetRelDispNodeCache() const;
    CRelDispNodeCache *  DelRelDispNodeCache();

    void SetVertPercentAttrInfo() { _fContainsVertPercentAttr = TRUE; }
    void SetHorzPercentAttrInfo() { _fContainsHorzPercentAttr = TRUE; }
    void ElementResize(CFlowLayout * pFlowLayout);


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
    CDispNode *     FindElementDispNode(CElement * pElement) const
    {
        return HasRelDispNodeCache()
                ? GetRelDispNodeCache()->FindElementDispNode(pElement)
                : NULL;
    }
    void SetElementDispNode(CElement * pElement, CDispNode * pDispNode)
    {
        if (HasRelDispNodeCache())
            GetRelDispNodeCache()->SetElementDispNode(pElement, pDispNode);
    }
    void EnsureDispNodeVisibility(CElement * pElement = NULL)
    {
        if (HasRelDispNodeCache())
            GetRelDispNodeCache()->EnsureDispNodeVisibility(pElement);
    }
    void HandleDisplayChange()
    {
        if (HasRelDispNodeCache())
            GetRelDispNodeCache()->HandleDisplayChange();
    }
// CRelDispNodeCache wants access to GetRelNodeFlowOffset()
public:
    void GetRelNodeFlowOffset(CDispNode * pDispNode, CPoint * ppt);
protected:
    void GetRelElementFlowOffset(CElement * pElement, CPoint * ppt);
    void TranslateRelDispNodes(const CSize & size, long lStart);
    void ZChangeRelDispNodes();
};

#define ALIGNEDFEEDBACKWIDTH    4

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
    Assert(pTreeNode->Element()->GetCurLayout());

    return AddLayoutDispNode(ppi, pTreeNode->Element()->GetCurLayout(), dx, dy, pDispSibling);
}

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

#endif

#pragma INCMSG("--- End '_disp.h'")
#else
#pragma INCMSG("*** Dup '_disp.h'")
#endif
