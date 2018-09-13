//+----------------------------------------------------------------------------
//
// File:        ltrow.hxx
//
// Contents:    CTableCellLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LTROW_HXX_
#define I_LTROW_HXX_
#pragma INCMSG("--- Beg 'ltrow.hxx'")

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

MtExtern(CTableRowLayout)
MtExtern(CTableRowLayout_aryCells_pv)

//+---------------------------------------------------------------------------
//
//  Class:      CTableRowLayout (parent)
//
//  Purpose:    Layout object for HTML <TR> table row object
//
//  Note:   The _arySite array contains CTableCell objects
//
//----------------------------------------------------------------------------

class CTableRowLayout : public CLayout
{
    typedef CLayout super;

    friend class CTableRow;
    friend class CTable;
    friend class CTableLayout;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableSection;
    friend class CTableRowCellsCollectionCacheItem;
    friend class CTableCellsCollectionCacheItem;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableRowLayout))

    CTableRowLayout (CElement *pElementLayout)
      : CLayout(pElementLayout), _aryCells(Mt(CTableRowLayout_aryCells_pv))

    {
        // _fOwnsRuns  = TRUE; // CElement
        // _fInheritFF = TRUE; // CElement
        _fOwnTabOrder = TRUE; // CLayout
        _yBaseLine = -1;
        _yHeight   = -1;  // not calculated...
        _yWidestCollapsedBorder = 0;
   }
   ~CTableRowLayout();

    // Accessor to table row object.
    CTableRow *     TableRow()    const { return DYNCAST(CTableRow, ElementOwner()); }

    // Accessors involving element tree climbing.
    CTableSection * Section()     const { return TableRow()->Section(); }
    CTable *        Table()       const { return TableRow()->Table(); }
    CTableLayout *  TableLayout() const { return Table() ? Table()->Layout() : NULL; }

    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------
    virtual void GetPositionInFlow(CElement * pElement, CPoint * ppt);

    virtual CLayout * GetFirstLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);
    virtual CLayout * GetNextLayout(DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE);

    virtual BOOL    ContainsChildLayout(BOOL fRaw = FALSE);
    virtual BOOL    CanHaveChildren()
                    {
                        return TRUE;
                    }

    virtual void Draw(CFormDrawInfo *pDI, CDispNode *);

    virtual void Notify(CNotification * pnf);

    virtual void RegionFromElement( CElement       * pElement,
                                    CDataAry<RECT> * paryRects,
                                    RECT           * prcBound,
                                    DWORD            dwFlags);

    void    ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected, BOOL fLayoutCompletelyEnclosed, BOOL fFireOM );
    
    //+-----------------------------------------------------------------------
    //  CTableRow cache helper methods
    //------------------------------------------------------------------------

    // Add cell at position, return position added at
    // Note: in can be different because of row-spanned cells
    HRESULT AddCell(CTableCell * pCell);
    HRESULT AddDisplayNoneCell(CTableCell * pCell);

    inline CTableCell * GetCell(int i)
                        {
                            return (CTableCell *)((DWORD_PTR)_aryCells[i] & ~3);
                        }
            int         GetCells()
                        {
                            return _aryCells.Size();
                        }

    // Make sure cell array is at least cCells
    HRESULT EnsureCells(int cCells);
    void    ClearRowLayoutCache();

#if DBG == 1
    void Print(HANDLE pF, int iTabs);
#endif

    int RowPosition() { return _iRow; }
    
    //+-----------------------------------------------------------------------
    //  Helper methods
    //------------------------------------------------------------------------

    //+-----------------------------------------------------------------------
    //  Scrolling methods
    //------------------------------------------------------------------------

    BOOL IsHeightSpecified()              { return _uvHeight.IsSpecified(); }
    BOOL IsHeightSpecifiedInPercent()     { return _uvHeight.IsSpecifiedInPercent(); }
    BOOL IsHeightSpecifiedInPixel()       { return _uvHeight.IsSpecifiedInPixel(); }
    int  GetPixelHeight(CDocInfo * pdci, int iExtra=0)   { return _uvHeight.GetPixelHeight(pdci, TableRow(), iExtra); }
    int  GetPercentHeight()                { return _uvHeight.GetPercent(); }
    int  GetHeightUnitValue()              { return _uvHeight.GetUnitValue(); }
    void SetPercentHeight(int iP)         { _uvHeight.SetPercent(iP); return; }

    CFlowLayout *   GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL);

    virtual HRESULT PageBreak(LONG yStart, LONG yIdeal, CStackPageBreaks * paryValues);


    // adjust height of the row for specified height of the cell
    void    AdjustHeight(CTreeNode *pNodeCell, CCalcInfo *pci, CTable *pTable); 

    // cache row height from format info
    void CacheRowHeight(const CFancyFormat *pff);

    BOOL IsEmptyFrom(int iCellIndex)
    {
        for (int i= iCellIndex; i < _aryCells.Size(); i++)
        {
            if (!IsEmpty(_aryCells[i]))
                return FALSE;
        }
        return TRUE;
    }

    // helper function to calculate absolutely positioned child layout
    virtual void        CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pChildLayout);

protected:
// BUGBUG: This is a hack - CalculateRows (and only CalculateRows) uses _sizeProposed as a temporary cache
//         This field is used instead for now but we need a better long-term solution (brendand)
    long                    _yHeightOld;

    // 1 means empty cell, EVEN means real cell, ODD means spanned over
    CPtrAry<CTableCell *>   _aryCells;

    int                     _iRow;
    long                    _yBaseLine;

    int                     _yWidestCollapsedBorder; // collapsed border: widest collapsed cell border width to our bottom

    // BUGBUG: Is _yHeight redundant with _sizeProposed.cy? If so, remove _yHeight. (brendand)
    long                    _yHeight;

    CHeightUnitValue        _uvHeight;               // height specified in cells in this row

    // CCollectionCache *      _pCollectionCache;
    int                     _cRealCells;            // number of real cells

    CPtrAry<CTableCell *>   *_pDisplayNoneCells;    // pointer to an array of none display cells

    // DECLARE_CLASSDESC_MEMBERS;


private:
    NO_COPY(CTableRowLayout);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'ltrow.hxx'")
#else
#pragma INCMSG("*** Dup 'ltrow.hxx'")
#endif
