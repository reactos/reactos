//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       tcell.hxx
//
//  Contents:   CTableCell and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_TCELL_HXX_
#define I_TCELL_HXX_
#pragma INCMSG("--- Beg 'tcell.hxx'")

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#define _hxx_
#include "table.hdl"

#define MAX_COL_SPAN  (1000)

class CTable;
class CTableRow;
class CTableRowLayout;
class CTableCell;
class CTableCellLayout;
class CImgCtx;
class CTableCol;
class CTableSection;
class CDataSourceBinder;
class CTableLayout;

extern ELEMENT_TAG s_atagTSection[];

inline BOOL IsCaption(ELEMENT_TAG eTag)
{
    return eTag == ETAG_CAPTION || eTag == ETAG_TC;
}

class CNiceUnitValue: public CUnitValue
{
public:
    typedef CUnitValue super;

    CNiceUnitValue () {}                      // Default Constructor
    CNiceUnitValue (const CUnitValue &uv);    // Copy Constructor

    BOOL IsSpecified() const
        { return !IsNullOrEnum(); }
    BOOL IsSpecifiedInPercent() const
        {
            return GetUnitType() == CUnitValue::UNIT_PERCENT;
        }
    BOOL IsSpecifiedInPixel() const
        {
            return IsScalerUnit(GetUnitType());
        }
    int GetPercent() const
        {
            Assert (IsSpecifiedInPercent());
            return super::GetPercent();
        }
};

class CHeightUnitValue: public CNiceUnitValue
{
public:

    CHeightUnitValue () {}                      // Default Constructor
    CHeightUnitValue (const CUnitValue &uv);    // Copy Constructor
    CHeightUnitValue (const CHeightUnitValue &uv);    // Copy Constructor

    int GetPixelHeight(CDocInfo * pdci, CElement *p, int iExtra=0) const
    {
        int i = YGetPixelValue(pdci, 0, p->GetFirstBranch()->GetFontHeightInTwips((CUnitValue*)this) + iExtra);

#ifdef  IE5_ZOOM

        // Assert that object model value is non-zero
        Assert (GetUnitValue());

#else   // !IE5_ZOOM

        Assert (i >= 0); // NS compatibility to ignore values less then equal 0 (we should handle this case properly in table.pdl

#endif  // IE5_ZOOM

        return i;
    }
    int GetPercentSpecifiedHeightInPixel(CDocInfo * pdci, CElement *p, int iParentHeight) const
    {
        return YGetPixelValue(pdci, iParentHeight, p->GetFirstBranch()->GetFontHeightInTwips((CUnitValue*)this));
    }
};

class CWidthUnitValue:  public CNiceUnitValue
{
public:

    CWidthUnitValue () {}                       // Default Constructor
    CWidthUnitValue (const CUnitValue &uv);     // Copy Constructor
    CWidthUnitValue (const CWidthUnitValue &uv);     // Copy Constructor

    int GetPixelWidth(CDocInfo * pdci, CElement *p, int iExtra = 0) const
    {
        int i = XGetPixelValue(pdci, 0, p->GetFirstBranch()->GetFontHeightInTwips((CUnitValue*)this) + iExtra);
        // BUGBUG: It would be nice to have this assert, but it fires unnecessarily in
        //         AdjustForColSpan when the colspan'd cell encompasses cells in other rows
        //         that are "virtual" (that is, introduced by the colspan) - In those cases
        //         the xMin/xMax of the columns is zero causing this assert to fire. (brendand)
        // Assert (i); // NS compatibility to ignore values less then equal 0 (we should handle this case properly in table.pdl
        return i;
    }
    int GetPercentSpecifiedWidthInPixel(CDocInfo * pdci, CElement *p, int iParentWidth) const
    {
        return XGetPixelValue(pdci, iParentWidth, p->GetFirstBranch()->GetFontHeightInTwips((CUnitValue*)this));
    }
    int SetPixelWidth(CDocInfo * pdci, int iW)
    {
        iW = pdci->WindowXFromDocPixels (iW);
        return SetValue(iW, CUnitValue::UNIT_PIXELS);
    }

};

//+---------------------------------------------------------------------------
//
//  Class:      CTableCell
//
//  Purpose:    HTML table cell object <TH> <TD>
//
//  Note:   This object will be used for both <TH> and <TD>
//
//----------------------------------------------------------------------------

#if defined(DYNAMICARRAY_NOTSUPPORTED)
#define CTABLECELL_ACCELLIST_SIZE       2
#endif

MtExtern(CTableCell)

class CTableCell : public CTxtSite
{
    DECLARE_CLASS_TYPES(CTableCell, CTxtSite)

    friend class CTable;
    friend class CTableLayout;
    friend class CTableRow;
    friend class CTableRowLayout;
    friend class CTableCellLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableCell))

    CTableCell(ELEMENT_TAG etag, CDoc *pDoc) : CTxtSite(etag, pDoc)
    {
    }

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save(CStreamWriteBuff *pStmWrBuff, BOOL);

    BOOL CheckSameFormat
    (
        CTreeNode  * pNodeTarget,
        CTableCell * pCell1,
        CTableRow  * pRow,
        CTableRow  * pRow1,
        CTableCol  * pCol,
        CTableCol  * pCol1
    );

    virtual HRESULT ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget);

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    //-------------------------------------------------------------------------
    //
    // Layout related functions
    //
    //-------------------------------------------------------------------------

    DECLARE_LAYOUT_FNS(CTableCellLayout);

    //
    // Handle notification.
    //-------------------------------------------------------------------------
    virtual void    Notify(CNotification *pNF);

    virtual DWORD   GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll = FALSE);

    HRESULT EnterTree();   // handle the SN_ENTERTREE notification

    //+-----------------------------------------------------------------------
    //  CTableCell methods
    //------------------------------------------------------------------------

    HRESULT ApplyCellFormat(CFormatInfo *pCFI);


    //+-----------------------------------------------------------------------
    //  ITableCell methods
    //------------------------------------------------------------------------
    // these  are declared baseimplementation in the pdl; we need prototypes here
    NV_DECLARE_TEAROFF_METHOD(put_colSpan, PUT_colSpan, (long v));
    NV_DECLARE_TEAROFF_METHOD(get_colSpan, GET_colSpan, (long*p));

    // these  are declared baseimplementation in the pdl; we need prototypes here
    NV_DECLARE_TEAROFF_METHOD(put_rowSpan, PUT_rowSpan, (long v));
    NV_DECLARE_TEAROFF_METHOD(get_rowSpan, GET_rowSpan, (long*p));


    #define _CTableCell_
    #include "table.hdl"

    // Override/ignore changes to scrollxxxx properties
        HRESULT STDMETHODCALLTYPE put_scrollTop(long v)    { return S_OK; }
        HRESULT STDMETHODCALLTYPE put_scrollLeft(long v)   { return S_OK; }

    //+-----------------------------------------------------------------------
    //  CTableCell methods
    //------------------------------------------------------------------------

    CTableRow *Row() const;
    CTableSection *Section() const;
    CTable *Table() const;

    int ColSpan()
    {
        int cColSpan = GetAAcolSpan();
        return cColSpan < MAX_COL_SPAN? cColSpan : MAX_COL_SPAN;
    }
    int RowSpan()
    {
        int cRowSpan = GetAArowSpan();
        if (cRowSpan == 1)
            return 1;
        return RowSpanHelper(cRowSpan);
    }
    int RowSpanHelper(int cRowSpan);
    int RowIndex () const;
    int ColIndex();

    BOOL IsCaptionOnBottom()
    {
        Assert(Tag() == ETAG_CAPTION || Tag() == ETAG_TC);

        return GetFirstBranch()->GetParaFormat()->_bTableVAlignment == htmlCellVAlignBottom;
    }

    // Returns true if currect cell is in given range of cells
    BOOL IsInRange(RECT *pRect);

    // Returns TRUE, if there is no positioned elemnts under the cell
    BOOL IsNoPositionedElementsUnder();

    static HRESULT ApplyFormatUptoTable(CFormatInfo * pCFI);

protected:


    static ACCELS s_AccelsTCellDesign;
    static ACCELS s_AccelsTCellRun;

    DECLARE_CLASSDESC_MEMBERS;
    static const CLSID *        s_apclsidPages[];

private:
    NO_COPY(CTableCell);
};


//+---------------------------------------------------------------------------
//
//  Class:      CTableCaption
//
//  Purpose:    HTML table cell object <CAPTION>
//
//----------------------------------------------------------------------------

MtExtern(CTableCaption)

class CTableCaption : public CTableCell
{
    DECLARE_CLASS_TYPES(CTableCaption, CTableCell)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableCaption))

    CTableCaption(ELEMENT_TAG etag, CDoc *pDoc) : CTableCell(etag, pDoc)
    {}

    //+-----------------------------------------------------------------------
    //  CElement methods
    //------------------------------------------------------------------------

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    //+-----------------------------------------------------------------------
    //  ITableCaption methods
    //------------------------------------------------------------------------

    #define _CTableCaption_
    #include "caption.hdl"

    DECLARE_CLASSDESC_MEMBERS;

    enum
    {
        CAPTION_TOP     = 0,
        CAPTION_BOTTOM  = 1
    };

    unsigned    _uLocation;          // Caption placing (defaults to TOP)
};

#pragma INCMSG("--- End 'tcell.hxx'")
#else
#pragma INCMSG("*** Dup 'tcell.hxx'")
#endif
