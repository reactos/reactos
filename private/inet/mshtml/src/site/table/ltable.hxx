//+----------------------------------------------------------------------------
//
// File:        ltable.hxx
//
// Contents:    CTableLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LTABLE_HXX_
#define I_LTABLE_HXX_
#pragma INCMSG("--- Beg 'ltable.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

class CTableCaption;
class CDataSourceProvider;
class CDispNode;
class CTableBorderRenderer;
interface IHTMLTableCaption;

MtExtern(CTableLayout)
MtExtern(CTableLayout_pTableBorderRenderer)
// secure number of nested tables

#define SECURE_NESTED_LEVEL  (26)



//+---------------------------------------------------------------------------
//
//  Class:      CTableLayout (parent)
//
//  Purpose:    table layout object corresponding to HTML table object <TABLE>
//
//----------------------------------------------------------------------------

class CTableLayout : public CLayout
{
    typedef CLayout super;

    friend class CTable;
    friend class CTableRow;
    friend class CTableRowLayout;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableCol;
    friend class CTableSection;
#ifndef NO_DATABINDING
    friend class CDetailGenerator;
    friend class CTableConsumer;
#endif
    friend class CDBindMethodsTable;
    friend class CTableColCalc;
    friend class CTableCalcInfo;
    friend class CTableRowsCollectionCacheItem;
    friend class CTableBodysCollectionCacheItem;
    friend class CTableCellsCollectionCacheItem;
    friend class CTableSectionRowsCollectionCacheItem;

    // Sensible max value for borders
    enum
    {
//        MAX_BORDER_SPACE = 1000,
        MAX_CELL_SPACING = 1000,
        MAX_CELL_PADDING = 1000
//        ,MAX_TABLE_WIDTH = 32765 - 2*MAX_BORDER_SPACE,
    };

    enum
    {
        TABLE_ROWS_COLLECTION = 0,
        TABLE_BODYS_COLLECTION = 1,
        TABLE_CELLS_COLLECTION = 2,
        NUMBER_OF_TABLE_COLLECTION = 3,
    };

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableLayout))

    CTableLayout(CElement * pElementFlowLayout);
    ~CTableLayout();

#if DBG == 1
    void CheckTable();
#endif

    // Accessor to table object.
    CTable * Table() const { return DYNCAST(CTable, ElementOwner()); }

    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------

    virtual CTableLayout * IsTableLayout()    { return this; }

    // Enumeration method to loop thru children (start + continue)
    virtual CLayout *   GetFirstLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);
    virtual CLayout *   GetNextLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);
    virtual BOOL        ContainsChildLayout(BOOL fRaw = FALSE);

    // Drawing methods overriding CLayout
    DWORD   GetTableBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE);

    virtual void Draw(CFormDrawInfo *pDI, CDispNode * pDispNode);
            void DrawCellBorders(CFormDrawInfo * pDI,
                                 const RECT *    prcBounds,
                                 const RECT *    prcRedraw,
                                 CDispSurface *  pDispSurface,
                                 CDispNode *     pDispNode,
                                 void *          cookie,
                                 void *          pClientData,
                                 DWORD           dwFlags);

            void ClearCellMark(int i, int n, BOOL fAssert);

    virtual DWORD GetClientLayersInfo(CDispNode *pDispNodeFor);

    virtual HRESULT GetElementsInZOrder(CPtrAry<CElement *> *paryElements,
                                        CElement            *pElementThis,
                                        RECT                *prc,
                                        HRGN                 hrgn,
                                        BOOL                 fIncludeNotVisible);
    virtual BOOL    CanHaveChildren() { return TRUE; }

    // Layout methods overriding CLayout
    virtual BOOL  PercentSize();
    virtual BOOL  PercentHeight();
    virtual DWORD CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);

    virtual void  GetPositionInFlow(CElement * pElement, CPoint * ppt);

    VOID ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected , BOOL fLayoutCompletelyEnclosed, BOOL fFireOM );

    void    DoLayout(DWORD grfLayout);
    void    Notify(CNotification * pnf);
    void    Resize();

    virtual HRESULT OnFormatsChange(DWORD);
    virtual void    RegionFromElement(CElement       * pElement,
                                      CDataAry<RECT> * paryRects,
                                      RECT           * prcBound,
                                      DWORD            dwFlags);

    //
    //  Channel control methods
    //

    BOOL    IsDirty();
    BOOL    IsListening();
    void    Listen(BOOL fListen = TRUE);
    void    Reset(BOOL fForce);

    // clear cached format information.
    void VoidCachedFormats();

    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);

    //+-----------------------------------------------------------------------
    //  CTableLayout cache helper functions
    //------------------------------------------------------------------------

    //
    // Support for lazy table layout cache (TLC) maintenance.
    //
    
    HRESULT EnsureTableLayoutCache();
    HRESULT CreateTableLayoutCache();
    void    ClearTableLayoutCache();
    void    ClearTopTableLayoutCache();
    BOOL    IsTableLayoutCacheCurrent()      { return !_fCompleted || !_fTLCDirty || _fEnsuringTableLayoutCache; }
    void    MarkTableLayoutCacheDirty()      { if (!_fTLCDirty) Table()->InvalidateCollections(); _fTLCDirty = TRUE; }
    void    MarkTableLayoutCacheCurrent()    { _fTLCDirty = FALSE; }
    void    ReleaseRowsAndSections(BOOL fReleaseHeaderFooter, BOOL fReleaseTCs);
    void    ReleaseBodysAndTheirRows(int iBodyStart, int iBodyFinish);
    void    ReleaseTCs();
#if DBG == 1
    BOOL AssertTableLayoutCacheCurrent() { return _fDisableTLCAssert || IsTableLayoutCacheCurrent(); }
#endif

    HRESULT AddRow(CTableRow * pRow, BOOL fNewRow = TRUE);
    HRESULT AddCaption(CTableCaption * pCaption);
    HRESULT AddSection(CTableSection * pSection);
    HRESULT AddColGroup(CTableCol * pColGroup);
    HRESULT AddCol(CTableCol * pCol);

    HRESULT GetCellFromRowCol(int iRow, int iCol, CTableCell **ppTableCell);
    HRESULT GetCellsFromRowColRange(int iRowTop, int iColLeft, int iRowBottom, int iColRight, CPtrAry<CTableCell *> *paryCells);

    CTableRow *  GetRow(int i)              { Assert(AssertTableLayoutCacheCurrent()); return _aryRows[i]; }
    CTableCol *  GetCol(int i)              { Assert(AssertTableLayoutCacheCurrent()); return i < _aryCols.Size() ? _aryCols[i] : NULL; }
    CTableCol *  GetColGroup(int i)         { Assert(AssertTableLayoutCacheCurrent()); return i < _aryColGroups.Size() ? _aryColGroups[i] : NULL; }
    CTableSection * GetHeader()             { Assert(AssertTableLayoutCacheCurrent()); return _pHead; }
    CTableSection * GetFooter()             { Assert(AssertTableLayoutCacheCurrent()); return _pFoot; }

    int     GetRows()                       { Assert(AssertTableLayoutCacheCurrent()); return _aryRows.Size(); }
    int     GetCols()                       { Assert(AssertTableLayoutCacheCurrent()); return _cCols; }
    int     GetBodys()                      { Assert(AssertTableLayoutCacheCurrent()); return _aryBodys.Size(); }
    BOOL    HasBody()                       { Assert(AssertTableLayoutCacheCurrent()); return (_aryBodys.Size() > 0); }

    HRESULT EnsureCols(int cCols);  // Make sure column array is at least cCols
    HRESULT EnsureCells();          // Make sure there are column groups for at least cCols columns
    HRESULT EnsureRowSpanVector(int cCols); // Make sure column array is ar least cCols
    void    ClearRowSpanVector();
    void    AddRowSpanCell(int iAt, int cRowSpan)
    {
        Assert(cRowSpan > 1);
        (*_paryCurrentRowSpans)[iAt] = cRowSpan - 1;
        _cCurrentRowSpans++;
    }

#if NEED_A_SOURCE_ORDER_ITERATOR
    CTableRow *  GetRowInSourceOrder(int iSourceIndex);
    BOOL    IsAryRowsInSourceOrder()
    {
        return !_iHeadRowSourceIndex && _iFootRowSourceIndex == (_pHead ? _pHead->_cRows : 0);
    }
#endif

    HRESULT AddAbsolutePositionCell(CTableCell *pCell);

    //+-----------------------------------------------------------------------
    //  Layout methods (table specific)
    //------------------------------------------------------------------------

    void CalculateLayout(CTableCalcInfo * ptci, CSize * psize, BOOL fWidthChanged = FALSE, BOOL fHeightChange = FALSE);
    void CalculateBorderAndSpacing(CDocInfo * pdci);
    void CalculateMinMax(CTableCalcInfo * ptci);
    void CalculateColumns(
                CTableCalcInfo * ptci,
                CSize *     psize);
    BOOL CalculateRow(
                CTableCalcInfo *    ptci,
                CSize *             psize,
                CLayout **          ppLayoutSibling,
                CDispContainer *    pDispNode);
    void CalculateRows(
                CTableCalcInfo * ptci,
                CSize *     psize,
                long        yTableTop);

    // helper function
    void CalcAbsolutePosCell(CTableCalcInfo *ptci, CTableCell *pCell);

    void AdjustForColSpan(CTableCalcInfo * ptci, CTableColCalc *pColLastNonVirtual);
    void AdjustRowHeights(CTableCalcInfo * ptci, CSize * psize, CTableCell * pCell);
    void SetCellPositions(CTableCalcInfo * ptci, long yTableTop);
    void SizeAndPositionCaption(
                        CTableCalcInfo *     ptci,
                        CSize *         psize,
                        CLayout **      ppLayoutSibling,
                        CTableCaption * pCaption,
                        POINT *         ppt)
         {
             Assert(ppLayoutSibling);
             SizeAndPositionCaption(ptci, psize, ppLayoutSibling, (*ppLayoutSibling)->GetElementDispNode(), pCaption, ppt);
         }
    void SizeAndPositionCaption(
                        CTableCalcInfo *     ptci,
                        CSize *         psize,
                        CDispNode *     pDispNodeSibling,
                        CTableCaption * pCaption,
                        POINT *         ppt)
         {
             SizeAndPositionCaption(ptci, psize, NULL, pDispNodeSibling, pCaption, ppt);
         }
    void SizeAndPositionCaption(
                        CTableCalcInfo *     ptci,
                        CSize *         psize,
                        CLayout **      ppLayoutSibling,
                        CDispNode *     pDispNodeSibling,
                        CTableCaption * pCaption,
                        POINT *         ppt);

    CLayout *   AddLayoutDispNode(
                        CLayout *           pLayout,
                        CDispContainer *    pDispContainer,
                        CLayout *           pLayoutSibling,
                        const POINT *       ppt = NULL,
                        BOOL                fBefore = FALSE)
                {
                    HRESULT hr = AddLayoutDispNode(pLayout,
                                                pDispContainer,
                                                (pLayoutSibling
                                                        ? pLayoutSibling->GetElementDispNode()
                                                        : NULL),
                                                ppt,
                                                fBefore);

                    return (hr != S_OK
                                ? pLayoutSibling
                                : pLayout);
                }
    HRESULT     AddLayoutDispNode(
                        CLayout *           pLayout,
                        CDispContainer *    pDispContainer,
                        CDispNode *         pDispNodeSibling,
                        const POINT *       ppt = NULL,
                        BOOL                fBefore = FALSE);

    HRESULT             EnsureTableDispNode(CDocInfo * pdci, BOOL fForce);
    HRESULT             EnsureTableBorderDispNode();
    virtual void        EnsureContentVisibility(CDispNode * pDispNode, BOOL fVisible);

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //
    HRESULT             EnsureTableFatHitTest(CDispNode* pDispNode);
    BOOL                GetFatHitTest();
    
    CDispContainer *    GetCaptionDispNode();
    CDispContainer *    GetTableInnerDispNode();
    CDispContainer *    GetTableOuterDispNode();
    void                GetTableSize(CSize * psize);
    void                GetTableSize(SIZE * psize)
                        {
                            GetTableSize((CSize *)psize);
                        }
    void                SizeTableDispNode(CCalcInfo * pci, const SIZE & size, const SIZE & sizeTable);
    void                DestroyFlowDispNodes();
    BOOL                IsGridAndMainDisplayNodeTheSame() { return !_fHasCaptionDispNode; }

    virtual CDispNode * GetElementDispNode(CElement * pElement = NULL) const;

    BOOL IsZeroWidth()    { return _fZeroWidth; }

    // Debug support.
#if DBG == 1
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    void Print(HANDLE pF, int iTabs);
    void DumpTable(const TCHAR * pch);
#endif

    //+-----------------------------------------------------------------------
    //  CTable-delegated methods - invoked by CTable versions of these fctns
    //------------------------------------------------------------------------

    void Detach();

    //+-----------------------------------------------------------------------
    //  Databind support methods
    //------------------------------------------------------------------------

#ifndef NO_DATABINDING
    // virtual const CDBindMethods *GetDBindMethods(); // Return info about desired databinding.
    HRESULT GetTemplate(BSTR * pbstr); // Create the repeating template.
    HRESULT Populate();                // Populate the table with repeated rows if the RepeatSrc property is specified.
    HRESULT refresh();                 // Refresh (regenerate repeated rows, DataSrc is changed).
    BOOL    IsRepeating()         { return _pDetailGenerator != NULL; }    // Returns TRUE if the table is/(will be) repated over the row set
    BOOL    IsGenerated(int iRow) { return IsRepeating() && iRow >= GetHeadFootRows(); }
    long    RowIndex2RecordNumber(int iRow);
#endif // ndef NO_DATABINDING

    BOOL IsFixed()          { return _fFixed; }
    BOOL CollapseBorders()  { return _fCollapse; }
    BOOL IsFixedBehaviour() { return _fFixed || _fUsingHistory; }

    // Remove non-header/footer rows from the table
    HRESULT RemoveRowsAndTheirSections(int iRowStart, int iRowFinish);

    // Remove that part of the table which is considered a template
    HRESULT RemoveTemplate( ) { return RemoveBodys(0, GetBodys() - 1); }
    HRESULT RemoveBodys(int iBodyStart, int iBodyFinish);

    void    PastingRows(BOOL fPasting, CTableSection *pSectionInsertBefore=NULL)
        { _fPastingRows = fPasting;  _pSectionInsertBefore = pSectionInsertBefore; }

    // Save/restore state of databinding walk (see dbwalker.cxx)
    void    SaveDatabindWalkState(BOOL fState) { _fSaveDataBindWalkState = fState; }
    BOOL    GetDatabindWalkState() { return _fSaveDataBindWalkState; }
    
    //+-----------------------------------------------------------------------
    //  Helper methods
    //------------------------------------------------------------------------

    // get header and footer rows counter
    int GetHeadFootRows();

#if 0
    int GetCellInsetX() { return ((_xBorder ? 1 : 0) + _xCellPadding); }
    int GetCellInsetY() { return ((_yBorder ? 1 : 0) + _yCellPadding); }
#endif
    int BorderX() { return _fBorder ? _xBorder : 0; }
    int BorderY() { return _fBorder ? _yBorder : 0; }
    int CellSpacingX() { return _xCellSpacing; }
    int CellSpacingY() { return _yCellSpacing; }
    int CellPaddingX() { return _xCellPadding; }
    int CellPaddingY() { return _yCellPadding; }


    // These two routines should be used to generate indexes for walking the table
    // row array rather than using a simple counter. They ensure that rows will be
    // accessed in the correct order (i.e., header, body, then footer).
    // NOTE: These routines assume the caller will quit calling after all rows
    //       have been exhausted (i.e., once iRow >= GetRows())
    int  GetFirstRow();
    int  GetNextRow(int iRow);
    int  GetNextRowSafe(int iRow);
    int  GetPreviousRow(int iRow);
    int  GetPreviousRowSafe(int iRow);
    int  GetLastRow();
    BOOL IsFooterRow(int iRow);

    // convert visual row index into an index of _aryRows
    int VisualRow2Index(int iRow);

    // get user's specified width (0 if not specified or specified in %%)
    long GetSpecifiedPixelWidth(CTableCalcInfo * ptci, BOOL *pfSpecifiedInPercent = NULL);

    CTableCell * GetCellFromPos(POINT * ptPosition);
    CTableRowLayout * GetRowLayoutFromPos(int y, int * pyRowTop, int * pyRowBottom, BOOL *pfBelowRows = NULL);
    int GetColExtentFromPos(int x, int * pxColLead, int * pxColTrail, BOOL fRightToLeft=FALSE);
    BOOL GetRowTopBottom(int iRowLocate, int * pyRowTop, int * pyRowBottom);
    BOOL GetColLeftRight(int iColLocate, int * pxColLeft, int * pxColRight);
    void GetHeaderFooterRects(RECT * prcTableHeader, RECT * prcTableFooter);

    // for keyborad navigation
    CFlowLayout *GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);
    CFlowLayout *GetNextFlowLayoutFromCell(NAVIGATE_DIRECTION iDir, POINT ptPosition, int iRow, int iCol);
    CFlowLayout *GetNextFlowLayoutFromCaption(NAVIGATE_DIRECTION iDir, POINT ptPosition, CTableCaption *pCaption);
    // helper function to get the cell from spec row and spec col under X pos
    CTableCell *GetCellBy(int iRow, int iCol, int x);
    CTableCell *GetCellBy(int iRow, int iCol);

    virtual CFlowLayout *GetFlowLayoutAtPoint(POINT pt);

    virtual HRESULT PageBreak(LONG yStart, LONG yIdeal, CStackPageBreaks * paryValues);
    virtual HRESULT ContainsPageBreak(long yTop, long yBottom, long * pyBreak, BOOL * pfPageBreakAfterAtTopLevel = NULL);
            HRESULT ContainsCaptionPageBreak(BOOL fTopCaptions, long yBreakWalk, long yTop, long yBottom, long * pyBreak);

    virtual HRESULT DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    virtual HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    //+-----------------------------------------------------------------------
    //  Helper methods for Table OM.
    //------------------------------------------------------------------------
    HRESULT insertElement(CElement *pAdjacentElement, CElement *pInsertElement, CElement::Where where, BOOL fIncrementalUpdatePossible = FALSE);
    HRESULT deleteElement(CElement *pDeleteElement, BOOL fIncrementalUpdatePossible = FALSE);
    HRESULT moveElement(IMarkupServices * pMarkupServices, IMarkupPointer  * pmpBegin, IMarkupPointer  * pmpEnd, IMarkupPointer  * pmpTarget);

    HRESULT createTHead(IDispatch** ppHead);
    HRESULT deleteTHead();
    HRESULT createTFoot(IDispatch** ppFoot);
    HRESULT deleteTFoot();
    HRESULT createCaption(IHTMLTableCaption** ppCaption);
    HRESULT deleteCaption();
    HRESULT ensureTBody();

    // Helper function, to get the first real caption (not ETAG_TC).
    CTableCaption *GetFirstCaption();

    // After insert/delete operation we need to fixup table.
    HRESULT Fixup(BOOL fIncrementalUpdatePossible = FALSE);

    // Flush the grid information, we are about to rebuild it.
    void FlushGrid();

    // Let the rest of the world know if we're completely loaded.
    BOOL IsCompleted()    { return _fCompleted; }
    BOOL CanRecalc()      { return _fCompleted || _fIncrementalRecalc; }
    BOOL IsCalced()       { return CanRecalc() || (_cCalcedRows || _pFoot); }   // _cCalcedRows doesn't include footer rows
    void ConsiderResizingFixedTable(CTableRow * pRow);
    BOOL IsFullyCalced()  { return _fCompleted && !IsRepeating() && (_cRowsParsed - (_pFoot? _pFoot->_cRows : 0) == _cCalcedRows); }
    void InitCalcInfoForLazyCell(CCalcInfo *pci, CFormDrawInfo *pDI);
    void ResetMinMax()    { _sizeMin.cx = _sizeMin.cy = 
                            _sizeMax.cx = _sizeMax.cy = -1;
                            _cCalcedRows = 0;                   // number of calculated rows (used in incremental recalc)
                          }

    // History support
    HRESULT LoadHistory(IStream *   pStreamHistory, CCalcInfo * pci);
    HRESULT SaveHistoryValue(CHistorySaveCtx *phsc);
    BOOL IsHistoryAvailable()  { return !!Table()->_pStreamHistory; }

    // Helper function called on exit tree for row
    void RowExitTree(int iRow, CTableSection *pCurSection);
    void BodyExitTree(CTableSection *pSection);

protected:

    void SetLastNonVirtualCol(int iAt);

    unsigned                    _fHavePercentageInset: 1;  // (1) 
    unsigned                    _fDetailCloning: 1;     // (2) if == 1 then we are doing cloning
    unsigned                    _fCompleted: 1;         // (3) table is not parsed yet if FALSE
    unsigned                    _fTLCDirty:1;           // (4) Table layout cache (TLC) is dirty.  !_fCompleted implies !_fTLCDirty.
    unsigned                    _fRefresh : 1;          // (5) the refresh was called
    unsigned                    _fBorder : 1;           // (6) there is border to draw
    unsigned                    _fZeroWidth: 1;         // (7) set to 1 if table is empty (0 width).
    unsigned                    _fHavePercentageRow:1;  // (8) one or more rows have heights which are a percent
    unsigned                    _fHavePercentageCol:1;  // (9) one or more cols have widths which are a percent

    unsigned                    _fCols:1;               // (10) COLS attr is specified/column widths are kinda fixed
    unsigned                    _fAlwaysMinMaxCells:1;  // (11) calculate min/max for all cells
    unsigned                    _fSaveDataBindWalkState:1;  // (12) for nested bound tables (see dbwalker.cxx)
    unsigned                    _fCalcedOnce:1;         // (13) CalculateLayout was called at least once
    unsigned                    _fDontSaveHistory:1;    // (14) Don't save the history of the table
    unsigned                    _fTableOM:1;            // (15) Table OM operation
    unsigned                    _fFixed:1;              // (17) Table has a fixed attr specified via CSS2 style
    unsigned                    _fCollapse:1;           // (18) Table is collapsing borders (CSS2 style)
    unsigned                    _fBorderInfoCellDefaultCached:1; // (19) TRUE if we have cached the tablewide default border settings
    unsigned                    _fEnsuringTableLayoutCache:1;    // (20) TRUE only while CTableLayout::EnsureTableLayoutCache is running.
    unsigned                    _fTemplateRows:1;       // (21) BUGBUG: hack for databinding to know when rows for a body section have been inserted into the tree.
    unsigned                    _fListen:1;             // (22) Listen to notifications
    unsigned                    _fDirty:1;              // (23)  
    unsigned                    _fIncrementalRecalc: 1; // (24) 1 when table itself fired incremental recal, 0 if handled or all other cases
    unsigned                    _fPastingRows:1;        // (25) pasting rows (data-binding optimization)
    unsigned                    _fRemovingRows: 1;      // (26) pasting rows (data-binding optimization)
    unsigned                    _fBodyParentOfTC:1;     // (27) 1 when there is at least 1 body who is a parent of TC element.
    unsigned                    _fRuleOrFrameAffectBorders: 1;  // (28) (oliverse) BUGBUG: comment?
    unsigned                    _fUsingHistory:1;       // (29) the history exists for this table and we are using it.
    unsigned                    _fHasCaptionDispNode:1; // (30) Has separate display node to hold CAPTIONs
    unsigned                    _fAllRowsSameShape:1;   // (31) All the rows have the same total number of col spans
    unsigned                    _fEnsureTBody: 1;       // (32) ensuring TBODY for the table
    unsigned                    _fForceMinMaxOnResize: 1; // (33) force min max on resize
    WHEN_DBG(unsigned           _fDisableTLCAssert:1;)  // (34) disable the assert for situations where we WANT to use an outdated TLC

    int                         _cSizedCols;    // Number of sized columns

    SIZE                        _sizeMin;
    SIZE                        _sizeMax;
    SIZE                        _sizeParent;    // last parent width used for measuring

    CPtrAry<CTableRow *>        _aryRows;       // row array
    CPtrAry<CTableSection *>    _aryBodys;      // TBODY-s
    int                         _cCols;         // max # of cells in a row
    CPtrAry<CTableCol *>        _aryCols;       // col array
    CPtrAry<CTableCol *>        _aryColGroups;  // COLGROUP-s
    CPtrAry<CTableCaption *>    _aryCaptions;   // CAPTIONs
    CTableSection *             _pHead;         // THEAD
    CTableSection *             _pFoot;         // TFOOT

    CDataAry<CTableColCalc>     _aryColCalcs;
    int                         _xWidestCollapsedBorder; // collapsed border: widest collapsed cell border width
    CBorderInfo *               _pBorderInfoCellDefault; // default cell border info
    CTableBorderRenderer *      _pTableBorderRenderer; // for collapsed border (or rules/frame) - object that drives rendering of cell borders
    int                         _xBorder;       // vertical border width
    int                         _yBorder;       // horizontal border width
    int                         _aiBorderWidths[4]; // top,right,bottom,left
    int                         _xCellSpacing;  // vertical distance between cells
    int                         _yCellSpacing;  // horizontal distance between cells
    int                         _xCellPadding;  // vertical padding in cells
    int                         _yCellPadding;  // horizontal padding in cells

#ifndef NO_DATABINDING
    CDetailGenerator *          _pDetailGenerator;  // pointer to the details host object (row creation, etc.)
#endif

    ULONG                       _cDirtyRows;            // how many resize req's I've ignored
    ULONG                       _nDirtyRows;            // how many resize req's to ignore

    int                         _iLastNonVirtualCol;    // last non virtual column in the table
    int                         _cNonVirtualCols;       // number of non virtual columns
    
    // Incremental recalc
    int                         _cCalcedRows;           // number of processed and calculated rows
    int                         _cRowsParsed;           // number of rows parsed (cached for fixed table incremental recalc)
    DWORD                       _dwTimeEndLastRecalc;   // used for incremental recalc only
    DWORD                       _dwTimeBetweenRecalc;
    CTableSection             * _pSectionInsertBefore;  // for pasting rows and sections
    CDataAry<int>             * _paryCurrentRowSpans;   // pointer to an array of coming down rowspans
    int                         _cCurrentRowSpans;      // number of current row spans
    int                         _cNestedLevel;          // nested tables level
    int                         _cTotalColSpan;         // for the current row (used by AddCell)
    int                         _iInsertRowAt;          // temp hack for Table OM
    
#if NEED_A_SOURCE_ORDER_ITERATOR
    int                         _iHeadRowSourceIndex;   // source index for the header rows
    int                         _iFootRowSourceIndex;   // source index for the footer rows
#endif

    CPtrAry<CTableCell *>      *_pAbsolutePositionCells;// pointer to an array of absolute position cells
    int                         _iCollectionVersion;    // collection verion number

private:

    NO_COPY(CTableLayout);

    HRESULT DeleteGeneratedRows();
    void SetDirtyRowThreshold(ULONG nDirtyRows) { _nDirtyRows = nDirtyRows; }

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};


//+----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------
inline int
CTableLayout::GetHeadFootRows()
{
    Assert(AssertTableLayoutCacheCurrent());
    return (_pHead? _pHead->_cRows: 0) + (_pFoot? _pFoot->_cRows: 0);
}

inline int
CTableLayout::GetFirstRow()
{
    Assert(AssertTableLayoutCacheCurrent());
    return ((_pHead && _pHead->_cRows) || !_pFoot || _pFoot->_cRows == GetRows()
                ? 0
                : _pFoot->_cRows);
}

inline int
CTableLayout::GetNextRow(
    int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert((void *)_aryRows != (void *)NULL);
    Assert(GetRows());
    Assert(iRow >= 0 && iRow < GetRows());
    Assert(!_pHead ||
           (_pHead->_iRow == 0 && "Header section should be the first in the table"));
    Assert(!_pFoot || ((_pFoot->_iRow == (_pHead? _pHead->_cRows: 0) || _pFoot->_cRows == 0)
                       && "Footer section should follow the header section or be the first section if header is not specified"));
    iRow++;
    if (_pFoot)
    {
        // If at the last row of the header, move the to first body row
        if (_pHead && iRow == _pHead->_cRows)
        {
            iRow += _pFoot->_cRows;
        }

        // If at the last row of the body, move to the first footer row
        // (This check must always be made since a table may not contain a body)
        if (iRow >= GetRows())
        {
            iRow = _pFoot->_iRow;
        }
    }
    return iRow;
}

inline int
CTableLayout::GetPreviousRow(
    int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    Assert((void *)_aryRows != (void *)NULL);
    Assert(GetRows());
    Assert(iRow >= 0 && iRow < GetRows());
    Assert(!_pHead ||
           (_pHead->_iRow == 0 && "Header section should be the first in the table"));
    Assert(!_pFoot || ((_pFoot->_iRow == (_pHead? _pHead->_cRows: 0) || _pFoot->_cRows == 0)
                       && "Footer section should follow the header section or be the first section if header is not specified"));
    if (_pFoot)
    {
        // If at the first row of the footer, move to the last body row.
        if (iRow == (_pHead?_pHead->_cRows:0))
        {
            iRow = GetRows();
        }

        // If at the first body, move to last row of the header.
        // (This check must always be made since a table may not contain a body)
        if (iRow == (_pHead?_pHead->_cRows:0) + _pFoot->_cRows)
        {
            iRow -= _pFoot->_cRows;
        }
    }
    iRow--;
    return iRow;
}

inline int
CTableLayout::GetLastRow()
{
    Assert(AssertTableLayoutCacheCurrent());
    return ((_pFoot && _pFoot->_cRows)
                ? _pFoot->_iRow + _pFoot->_cRows - 1
                : GetRows() - 1);
}

inline BOOL
CTableLayout::IsFooterRow(int iRow)
{
    Assert(AssertTableLayoutCacheCurrent());
    return _pFoot
                ? iRow >= _pFoot->_iRow && iRow < _pFoot->_iRow + _pFoot->_cRows
                : FALSE;
}


inline CFlowLayout *
CTableLayout::GetFlowLayoutAtPoint(
    POINT   pt)
{
    CTableCell *pCell = GetCellFromPos(&pt);
    if (pCell)
        return pCell->GetFlowLayout();
    return NULL;
}

inline HRESULT
CTableLayout::DragEnter(
    IDataObject *   pDataObj,
    DWORD           grfKeyState,
    POINTL          ptl,
    DWORD *         pdwEffect)
{
    return DragOver(grfKeyState, ptl, pdwEffect);
}

inline HRESULT
CTableLayout::DragOver(
    DWORD   grfKeyState,
    POINTL  pt,
    DWORD * pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;
    DragHide();
    RRETURN(S_OK);
}

inline void
CTableLayout::SetLastNonVirtualCol(
    int iAt)
{
    if (iAt > _iLastNonVirtualCol)
    {
        _iLastNonVirtualCol = iAt;
    }
}

inline void
CTableLayout::Listen(
    BOOL    fListen)
{
    if ((unsigned)fListen != _fListen)
    {
        if (IsListening())
        {
            Reset(FALSE);
        }

        _fListen = (unsigned)fListen;
    }
}

inline BOOL
CTableLayout::IsListening()
{
    return !!_fListen;
}

inline BOOL
CTableLayout::IsDirty()
{
    return !!(_fDirty || _fSizeThis);
}

inline void
CTableLayout::Reset(BOOL fForce)
{
    _fDirty = FALSE;

    super::Reset(fForce);
}


class CTableBorderRenderer : public CDispClient
{
    friend class CTableLayout;

private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CTableLayout_pTableBorderRenderer))

public:

    CTableBorderRenderer(CTableLayout * pTableLayout)
    {
        _pTableLayout = pTableLayout;
        _pDispNode = NULL;
        _ulRefs = 1;
    }

    DECLARE_FORMS_STANDARD_IUNKNOWN(CTableBorderRenderer)

    CTableLayout * _pTableLayout;
    CDispNode    * _pDispNode;

    //
    // CDispClient overrides
    // (only DrawClient() is interesting)
    //

    virtual void GetOwner(
                CDispNode *    pDispNode,
                void **        ppv)
    {
        Assert(pDispNode);
        Assert(pDispNode == _pDispNode);
        Assert(ppv);
        Assert(_pTableLayout);
        *ppv = _pTableLayout->ElementOwner();
    }

    virtual void DrawClient(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         cookie,
                void *         pClientData,
                DWORD          dwFlags);

    virtual void DrawClientBackground(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual void DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual void DrawClientScrollbar(
                int            iDirection,
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                long           contentSize,
                long           containerSize,
                long           scrollAmount,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual void DrawClientScrollbarFiller(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
    { return; }

    virtual BOOL HitTestScrollbar(
                int            iDirection,
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL HitTestScrollbarFiller(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL HitTestContent(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL HitTestFuzzy(
                const POINT *  pptHitInParentCoords,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

        

    virtual BOOL HitTestBorder(
                const POINT *  pptHit,
                CDispNode *    pDispNode,
                void *         pClientData)
    { return FALSE; }

    virtual BOOL ProcessDisplayTreeTraversal(
                void *         pClientData)
    { return TRUE; }

    virtual LONG GetZOrderForSelf()
    { return 0; }

    virtual LONG GetZOrderForChild(
                void *         cookie)
    { return 0; }

    virtual LONG CompareZOrder(
                CDispNode *    pDispNode1,
                CDispNode *    pDispNode2)
    { return -1; }

    // we're not expecting to filter table borders directly
    virtual CDispFilter* GetFilter()
    { Assert(FALSE); return NULL;}
    
    virtual void HandleViewChange( 
                DWORD          flags,
                const RECT *   prcClient,
                const RECT *   prcClip,
                CDispNode *    pDispNode)
    { return; }

    virtual void NotifyScrollEvent(
                RECT *  prcScroll,
                SIZE *  psizeScrollDelta)
    { return; }

    DWORD   GetClientLayersInfo(CDispNode *pDispNodeFor)
    { return 0; }

    void    DrawClientLayers(
                const RECT* prcBounds,
                const RECT* prcRedraw,
                CDispSurface *pSurface,
                CDispNode *pDispNode,
                void *cookie,
                void *pClientData,
                DWORD dwFlags)
    { return; }

#if DBG==1
    virtual void DumpDebugInfo(
                HANDLE         hFile,
                long           level, 
                long           childNumber,
                CDispNode *    pDispNode,
                void *         cookie);
#endif
};

#pragma INCMSG("--- End 'ltable.hxx'")
#else
#pragma INCMSG("*** Dup 'ltable.hxx'")
#endif
