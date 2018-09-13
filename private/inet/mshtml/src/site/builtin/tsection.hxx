//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       tsection.hxx
//
//  Contents:   CTableSection and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_TSECTION_HXX_
#define I_TSECTION_HXX_
#pragma INCMSG("--- Beg 'tsection.hxx'")

MtExtern(CTableSection)

#define MAX_COL_SPAN  (1000)

class CTable;
class CTableRow;
class CTableCell;
class CImgCtx;
class CTableCol;
class CTableSection;
class CDataSourceBinder;
class CTableLayout;

extern ELEMENT_TAG s_atagTSection[];

//+---------------------------------------------------------------------------
//
//  Class:      CTableSection (parent)
//
//  Purpose:    HTML row section object <THEAD> <TBODY> <TFOOT>
//
//  Note:   This object will be used for both COLGROUP and COL
//
//----------------------------------------------------------------------------

class CTableSection : public CElement
{
    DECLARE_CLASS_TYPES(CTableSection, CElement)
    
    friend class CTable;
    friend class CTableLayout;
    friend class CTableRow;
    friend class CTableCell;
    friend class CTableCellLayout;
    friend class CTableDetailGenerator;
    friend class CTableSectionRowsCollectionCacheItem;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableSection))

    CTableSection(ELEMENT_TAG etag, CDoc *pDoc) : CElement(etag, pDoc)
    {
        _fInheritFF = TRUE;
    }

    virtual ~CTableSection();

    //+-----------------------------------------------------------------------
    //  ITableSection methods
    //------------------------------------------------------------------------

    #define _CTableSection_
    #include "table.hdl"

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    virtual void    Notify(CNotification *pNF);

    HRESULT EnterTree();   // notification: section is added to the table

    //+-----------------------------------------------------------------------
    //  CTableSection methods
    //------------------------------------------------------------------------

    CTable *Table() const;
    enum { ROWS_COLLECTION = 0 };
    void            InvalidateCollections(CTable *pTable);
    HRESULT BUGCALL EnsureCollections(long, long * plCollectionVersion);
    HRESULT         EnsureCollectionCache(long lIndex);

    HRESULT GetCellFromRowCol(int iRow, int iCol, CTableCell **ppTableCell);
    HRESULT GetCellsFromRowColRange(int iRowTop, int iColLeft, int iRowBottom, int iColRight, CPtrAry<CTableCell *> *paryCells);

protected:

    int         _iRow;                  // starting row
    int         _cRows;                 // number of rows it contains
    int         _cGeneratedRows;        // number of generated rows (repeated tables)
    unsigned    _fBodySection: 1;       // TRUE - treat as a body section, FALSE - use etag
    unsigned    _fParentOfTC:1;         // TRUE if section is a parent of the ETAG_TC element (fake caption)
    
    CCollectionCache * _pCollectionCache;

    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CTableSection);
};

#pragma INCMSG("--- End 'tsection.hxx'")
#else
#pragma INCMSG("*** Dup 'tsection.hxx'")
#endif
