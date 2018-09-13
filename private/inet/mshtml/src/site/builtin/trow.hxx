//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       trow.hxx
//
//  Contents:   CTableRow and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_TROW_HXX_
#define I_TROW_HXX_
#pragma INCMSG("--- Beg 'trow.hxx'")

MtExtern(CTableRow)
MtExtern(CTableRow_aryCells_pv)

#define MAX_COL_SPAN  (1000)

class CTable;
class CTableLayout;
class CTableRow;
class CTableRowLayout;
class CTableCell;
class CImgCtx;
class CTableCol;
class CTableSection;
class CDataSourceBinder;


//+---------------------------------------------------------------------------
//
//  Class:      CTableRow (parent)
//
//  Purpose:    HTML table row object  <TR>
//
//  Note:   The _arySite array contains CTableCell objects
//
//----------------------------------------------------------------------------

class CTableRow : public CSite
{
    DECLARE_CLASS_TYPES(CTableRow, CSite)

    friend class CTable;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableLayout;
    friend class CTableSection;
    friend class CTableRowLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableRow))

    CTableRow (CDoc *pDoc)
      : super(ETAG_TR, pDoc)
    {
        _fOwnsRuns  = TRUE;
        _fInheritFF = TRUE;
    }

    virtual ~CTableRow();

    void Passivate();
    HRESULT EnterTree();
    void    ExitTree(CTableLayout *pTableLayout);

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    //+-----------------------------------------------------------------------
    //  ITableRow methods
    //------------------------------------------------------------------------

    #define _CTableRow_
    #include "table.hdl"

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save(CStreamWriteBuff *pStmWrBuff, BOOL);

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);
    HRESULT         PropagatePropertyChangeToCells(DISPID dispid, DWORD dwFlags, BOOL fSpecialProp = FALSE);

    //-------------------------------------------------------------------------
    // Layout related functions
    //-------------------------------------------------------------------------

    // Create the layout object to be associated with the current element
    DECLARE_LAYOUT_FNS(CTableRowLayout)

    //+-----------------------------------------------------------------------
    //  CTableRow methods
    //------------------------------------------------------------------------

    CTable *Table() const;
    CTableSection *Section() const;

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
    virtual HRESULT ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget);

    inline void     InvalidateCollections();
    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long, long * plCollectionVersion));

    enum { CELLS_COLLECTION = 0 };
    HRESULT         EnsureCollectionCache(long lIndex);

    virtual void    Notify(CNotification *pNF);
    
protected:

    unsigned                _fCompleted:1;          // TRUE if row has completed parsing
    unsigned                _fParentOfTC:1;         // TRUE if row is a parent of the ETAG_TC element (fake caption)
    unsigned                _fCrossingRowSpan:1;    // TRUE if row crosses a rowspaned cell
    unsigned                _fHaveRowSpanCells:1;   // TRUE if rowspaned cell starts in this row
    unsigned                _fAdjustHeight: 1;      // TRUE if there are cells with the specified height

    CCollectionCache *      _pCollectionCache;

    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CTableRow);
};

inline CTableCell * MarkSpanned(CTableCell * pCell) { return (CTableCell *)((DWORD_PTR)pCell | 1); }
inline CTableCell * MarkEmpty() { return (CTableCell *)1; }
inline CTableCell * Cell(CTableCell * pCell) { return (CTableCell *)((DWORD_PTR)pCell & ~3); }
inline BOOL IsReal(CTableCell * pCell) { return !((DWORD_PTR)pCell & 1); }
inline BOOL IsEmpty(CTableCell * pCell) { return (DWORD_PTR)pCell == 1; }
inline BOOL IsSpanned(CTableCell * pCell) { return ((DWORD_PTR)pCell & 1); }
inline CTableCell * MarkSpannedAndOverlapped(CTableCell * pCell) { return (CTableCell *)((DWORD_PTR)pCell | 3); }
inline BOOL IsOverlapped(CTableCell * pCell) { return ((DWORD_PTR)pCell & 2); }

void CTableRow::InvalidateCollections()
{
    if (_pCollectionCache)
        _pCollectionCache->Invalidate();    // this will reset collection version number
    return;
}

#pragma INCMSG("--- End 'trow.hxx'")
#else
#pragma INCMSG("*** Dup 'trow.hxx'")
#endif
