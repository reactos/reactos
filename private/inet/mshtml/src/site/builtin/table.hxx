//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       table.hxx
//
//  Contents:   CTable and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_TABLE_HXX_
#define I_TABLE_HXX_
#pragma INCMSG("--- Beg 'table.hxx'")

#ifndef X_TCELL_HXX_
#define X_TCELL_HXX_
#include "tcell.hxx"
#endif

#ifndef X_TROW_HXX_
#define X_TROW_HXX_
#include "trow.hxx"
#endif

#ifndef X_TSECTION_HXX_
#define X_TSECTION_HXX_
#include "tsection.hxx"
#endif

#define _hxx_
#include "table.hdl"

#define _hxx_
#include "caption.hdl"

#define MAX_COL_SPAN  (1000)

class CTable;
class CTableRow;
class CTableCell;
class CImgCtx;
class CTableCol;
class CTableSection;
class CDataSourceBinder;
class CDataSourceProvider;
class CTableLayout;
class CRecordInstance;

#ifndef DB_NULL_HROW
typedef ULONG_PTR HROW;
#endif

MtExtern(CTable)
MtExtern(CTableCol)
MtExtern(Mem)

//+---------------------------------------------------------------------------
//
//  Class:      CTableCol (parent)
//
//  Purpose:    HTML table column object <COLGROUP> <COL>
//
//  Note:   This object will be used for both COLGROUP and COL
//
//----------------------------------------------------------------------------

class CTableCol : public CElement
{
    DECLARE_CLASS_TYPES(CTableCol, CElement)

    friend class CTable;
    friend class CTableRow;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableCol))

    CTableCol(ELEMENT_TAG etag, CDoc *pDoc) : CElement(etag, pDoc)
    {
        _fInheritFF = TRUE;
    }

    //+-----------------------------------------------------------------------
    //  ITableCol methods
    //------------------------------------------------------------------------

    #define _CTableCol_
    #include "table.hdl"

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static  HRESULT CreateElement(CHtmTag *pht,
                                  CDoc *pDoc, CElement **ppElementResult);

    virtual void    Notify(CNotification *pNF);
    
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    HRESULT EnterTree();

    //+-----------------------------------------------------------------------
    //  CTableCol methods
    //------------------------------------------------------------------------

    CTable *Table() const;
    CTableCol *ColGroup() const
    {
        return DYNCAST(CTableCol, GetParentAncestorSafe(ETAG_COLGROUP));
    }

    //
    // Make sure there are enough columns for the colgroup setting
    //
    HRESULT EnsureCols();

#if DBG == 1
    void Print(HANDLE pF, int iTabs);
#endif

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
    virtual HRESULT ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget);

    int Cols() const
    {
        int cSpan = GetAAspan();
        return cSpan < MAX_COL_SPAN ? cSpan : MAX_COL_SPAN;
    }


protected:

    int         _iCol;          // starting column
    int         _cCols;         // number of columns

    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CTableCol);
};


//+---------------------------------------------------------------------------
//
//  Class:      CColCalc
//
//  Purpose:    Used for computing column widths
//
//----------------------------------------------------------------------------
class CTableColCalc
{
public:
    void Clear()
    {
        _xMin = _xMax = _xWidth = 0; _uvWidth.SetNull(); _cVirtualSpan = 0;
        _fWidthFromCell = _fDontSetWidthFromCell = _fVirtualSpan = _fAdjustedForCell = _fDisplayNone = FALSE;
    }

    void AdjustMaxToUserWidth(CCalcInfo * pdci, CTableLayout * pTableLayout);
    void Print(HANDLE pF, int iTabs);
    void Set(int cx)                    { _xMin = _xMax = _xWidth = cx; }

    BOOL IsWidthSpecified()             { return _uvWidth.IsSpecified(); }
    BOOL IsWidthSpecifiedInPercent()    { return _uvWidth.IsSpecifiedInPercent(); }
    BOOL IsWidthSpecifiedInPixel()      { return _uvWidth.IsSpecifiedInPixel(); }
    int  GetPixelWidth(CCalcInfo * pci, CElement * pElem, int iExtra=0)
                                        { return _uvWidth.GetPixelWidth(pci, pElem, iExtra); }
    int  GetPercentWidth()              { return _uvWidth.GetPercent(); }
    void SetPercentWidth(int iP)        { _uvWidth.SetPercent(iP); return; }
    int  GetWidthUnitValue()            { return _uvWidth.GetUnitValue(); }
    int  SetPixelWidth(CCalcInfo * pci, int iW)
                                        { return _uvWidth.SetPixelWidth(pci, iW); }
    void  ResetWidth()                  { _uvWidth.SetNull(); }
    BOOL IsDisplayNone()                { return _fDisplayNone; }

    void AdjustForCell(CTableLayout *pTableLayout,
                       int iPixels, const CWidthUnitValue *puvWidth,
                       BOOL fUnsizedColumn, BOOL fFirstRow,
                       CCalcInfo * pci, int xMin, int xMax);

    void AdjustForCol(const CWidthUnitValue *puvWidth, int iPixelsWidth, CCalcInfo * pci, int cColumns);

    long        _xMin;
    long        _xMax;
    long        _xWidth;
    int         _cVirtualSpan;    // number of cells spanned accros this virtual column.
    CTableCell *_pCell;                 // the spanned cell. (set for virtual overlapped columns only)

    DWORD _fWidthInFirstRow     : 1;    // First row of cells has width for this column
    DWORD _fWidthFromCell       : 1;    // the uvWidth is coming from the cell
    DWORD _fDontSetWidthFromCell: 1;    // don't set the uvWidth from the cells
    DWORD _fVirtualSpan   : 1;          // virtual column (no real cells in it) and there are at least
                                        // 2 cells in a different rows that spanned over this column
                                        // Note: the width of the virtual column is including CellSpacing
    DWORD _fAdjustedForCell: 1;         // the column width was adjusted for cell
    DWORD _fDisplayNone: 1;             // the column has display-none attribute

    CWidthUnitValue  _uvWidth;          // Width specified in cells in this column
};


//+---------------------------------------------------------------------------
//
//  Class:      CTableCalcInfo
//
//  Purpose:    Table CalcInfo context to avoid recalculating of information
//
//----------------------------------------------------------------------------

class CTableCalcInfo : public CCalcInfo
{
    friend CTableLayout;
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:

    CTableCalcInfo(CCalcInfo * pci, CTable *pTable, CTableLayout *pTableLayout);    // called from CalcSize
    CTableCalcInfo(CTable *pTable, CTableLayout *pTableLayout);                     // called from DoLayout

    CTable * Table() { Assert(_pTable); return _pTable; }
    CTableLayout * TableLayout() { Assert(_pTableLayout); return _pTableLayout; }

    CTable            * _pTable;
    CTableLayout      * _pTableLayout;
    CLSMeasurer       * _pme;           // Measurer reused across table cell layouts.

    // Bitfields.
    union
    {
        DWORD           _dwFlags;

        struct
        {
            unsigned    _fTableContainsCols : 1; // An embedded table has cols set meaning we haven't evaluated minmax completely.
            unsigned    _fSetCellPosition : 1;   // set cell position
            unsigned    _fDontSaveHistory : 1;   // TRUE if the images downloading was not compleete and we do not need to save history
        };
    };
    int                 _cNestedCalcs;
    CTableRow          * _pRow;
    CTableRowLayout    * _pRowLayout;
    const CFancyFormat * _pFFRow;
};


//+---------------------------------------------------------------------------
//
//  Class:      CTable (parent)
//
//  Purpose:    HTML table object <TABLE>
//
//  Note:   The _arySite array contains CTableRow objects
//
//----------------------------------------------------------------------------

class CTable : public CSite
{
    DECLARE_CLASS_TYPES(CTable, CSite)
    typedef CSite super;

    friend class CTableRow;
    friend class CTableCell;
    friend class CTableCol;
    friend class CTableSection;
#ifndef NO_DATABINDING
    friend class CDetailGenerator;
    friend class CTableConsumer;
#endif
    friend class CDBindMethodsTable;
    friend class CTableColCalc;
    friend class CTableLayout;


    enum
    {
        TABLE_ROWS_COLLECTION = 0,
        TABLE_BODYS_COLLECTION = 1,
        TABLE_CELLS_COLLECTION = 2,
        NUMBER_OF_TABLE_COLLECTION = 3,
    };

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTable))

    CTable(CDoc *pDoc);
    virtual ~CTable();

    virtual void    Notify(CNotification *pNF);

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    //-------------------------------------------------------------------------
    //
    // Layout related functions
    //
    //-------------------------------------------------------------------------
    DECLARE_LAYOUT_FNS(CTableLayout);


    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static  HRESULT CreateElement(CHtmTag *pht,
                                  CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);
    virtual HRESULT Save(CStreamWriteBuff *pStmWrBuff, BOOL);

    // Clicks never bubble out of a table (bug 62295)
    virtual HRESULT ClickAction(CMessage * pMessage) {return S_OK;}

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual DWORD   GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE);

    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long lIndex, long * plCollectionVersion));
    HRESULT EnsureCollectionCache();

    // invalidate the table's and documents collection caches
    void InvalidateCollections();

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    //+-----------------------------------------------------------------------
    //  Population with repeated rows and Scrolling methods
    //------------------------------------------------------------------------

#ifndef NO_DATABINDING
    // return info about desired databinding
    virtual const CDBindMethods *GetDBindMethods();
#endif // ndef NO_DATABINDING

    // Support for hierarchy.
    HRESULT GetInstanceForRow(CTableRow *pRow, CRecordInstance **ppInstance);

    //+-----------------------------------------------------------------------
    //  ITable methods
    //------------------------------------------------------------------------

    // these  are declared baseimplementation in the pdl; we need prototypes here
    NV_DECLARE_TEAROFF_METHOD(put_dataPageSize, PUT_dataPageSize, (long v));
    NV_DECLARE_TEAROFF_METHOD(get_dataPageSize, GET_dataPageSize, (long*p));

    #define _CTable_
    #include "table.hdl"

    //+-----------------------------------------------------------------------
    //  TOM helper methods
    //------------------------------------------------------------------------

    HRESULT moveRowHelper(long iRow, long iRowTarget, IDispatch **ppRow, BOOL fConvertRowIndices = TRUE);

    // helper method for asking for ranges of the cells on the table.
    HRESULT cells(BSTR szRange, IHTMLElementCollection ** ppCells);
   

    //+-----------------------------------------------------------------------
    //  Databinding helper methods
    //------------------------------------------------------------------------

    virtual DWORD GetInfo(GETINFO gi);


    BOOL IsDataboundInBrowseMode()
    {
        return  !FormsIsEmptyString(GetAAdataSrc()) && !Doc()->_fDesignMode;
    }

    BOOL IsDatabound()
    {
        return  !FormsIsEmptyString(GetAAdataSrc());
    }

    CDataSourceProvider * GetProvider();

    BOOL    IsRepeating();
    LPCTSTR GetDataSrc();
    LPCTSTR GetDataFld();
    BOOL    IsFieldKnown(LPCTSTR strField);

    HRESULT AddDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining, LPCTSTR strField);

    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (BSTR bstrName, DWORD grfdex, DISPID *pid));
    NV_DECLARE_TEAROFF_METHOD (ContextThunk_InvokeEx, contextthunk_invokeex, (DISPID dispid,
            LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo, IServiceProvider *pSrvProvider));

    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (BSTR *pBSTR));
    NV_DECLARE_TEAROFF_METHOD(get_readyState, get_readyState, (VARIANT *pReadyState));

    HRESULT LoadHistoryValue();

protected:

    unsigned                    _fBuildingTemplate:1;   // save state of stream during template creation
    unsigned                    _fEnableDatabinding:1;  // set to disable tree manipuation done by databinding
    unsigned                    _readyStateTable : 3;   // is table expanding?
    unsigned                    _readyStateFired : 3;   // is table expanding?

    DECLARE_CLASSDESC_MEMBERS;
    static const CLSID *        s_apclsidPages[];

    CCollectionCache *          _pCollectionCache;

    // History support
    IStream                 *   _pStreamHistory;
    int                         _iDocDotWriteVersionEnterTree;  // doc.write version # when <table> enters the tree


private:

    NO_COPY(CTable);

    HRESULT SetReadyStateTable(long lReadyStateTable);
    void OnReadyStateChange();
};



class CTableCollectionCacheItem : public CCollectionCacheItem
{
    typedef CCollectionCacheItem super ;

protected:
    LONG _lCurrentIndex;
    CTableLayout *_pTableLayout;
public:
    CElement *GetNext ( void );
    CElement *MoveTo ( long lIndex );
};

MtExtern(BldRowCellsCol)
MtExtern(BldCellsCol)

class CTableRowCellsCollectionCacheItem : public CTableCollectionCacheItem
{
    typedef CTableCollectionCacheItem super;

    friend class CTableRow;
    friend class CTableRowLayout;

protected:
    CTableRowLayout *_pRowLayout;
    int              _iCol;             // column number for _lCurrentIndex for current cell index
    int              _iCurrentCol;      // column number for _lCurrentIndex
    int              _cDisplayNone;     // number of display none cells before the current index
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(BldRowCellsCol))
    CTableRowCellsCollectionCacheItem(CTableRow *pRow) 
    {
        if (pRow) 
        { 
            _pRowLayout = pRow->Layout(); 
            Assert (_pRowLayout);
            CTable *pTable = pRow->Table();
            if (pTable)
            {
                _pTableLayout = pTable->Layout();
                Assert (_pTableLayout);
            }
        }
    }
    CElement *MoveTo ( long lIndex ); 
    CElement *GetNext ( void );
    CElement *GetAt ( long lIndex );
    long Length ( void );
};


class CTableCellsCollectionCacheItem : public CTableRowCellsCollectionCacheItem
{
    typedef CTableRowCellsCollectionCacheItem super;

protected:
    LONG    _lCurrentCellIndex; // this is a global index, note then _lCurrentIndex is local to the _pRowLayout
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(BldCellsCol))
    CTableCellsCollectionCacheItem(CTable *pTable) 
    : CTableRowCellsCollectionCacheItem(NULL) {_pTableLayout = pTable->Layout();}
    CElement *MoveTo ( long lIndex );
    CElement *GetNext ( void );
    CElement *GetAt ( long lIndex );
    long Length ( void );
    BOOL  IsRangeSyntaxSupported ( void )
    {
        return TRUE;   // Only cell collection supports cell range syntax a1:a10
    }
    int SetCurrentRowAndGetRowIndex(int lIndex);
};




#pragma INCMSG("--- End 'table.hxx'")
#else
#pragma INCMSG("*** Dup 'table.hxx'")
#endif
