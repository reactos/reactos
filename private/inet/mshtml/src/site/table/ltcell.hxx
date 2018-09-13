//+----------------------------------------------------------------------------
//
// File:        ltcell.hxx
//
// Contents:    CTableCellLayout and related classes
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_LTCELL_HXX_
#define I_LTCELL_HXX_
#pragma INCMSG("--- Beg 'ltcell.hxx'")

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

MtExtern(CTableCellLayout)

class CTableCellLayout : public CFlowLayout
{
    typedef CFlowLayout super;

    friend class CTableCell;
    friend class CTable;
    friend class CTableLayout;
    friend class CTableRow;
    friend class CTableRowLayout;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTableCellLayout))

    CTableCellLayout(CElement * pElementFlowLayout) : CFlowLayout(pElementFlowLayout)
    {
        _fGrabInside        = TRUE;     // CLayout
        _yBaseLine          = -1;
        _sizeCell.cx        = -1;
    }

    // Accessor to table cell object.
    CTableCell *    TableCell()   const { return DYNCAST(CTableCell, ElementOwner()); }

    // Accessors involving element tree climbing.
    CTableRow *     Row()         const { return TableCell()->Row(); }
    CTableSection * Section()     const { return TableCell()->Section(); }
    CTable *        Table()       const { return TableCell()->Table(); }
    CTableLayout *  TableLayout() const { CTable *pTable = Table(); return pTable? pTable->Layout() : NULL; }
    CTableCol *     Col()         const { CTableLayout *pTableLayout = TableLayout(); return pTableLayout? pTableLayout->GetCol(_iCol) : NULL; }
    CTableCol *     ColGroup()    const { CTableLayout *pTableLayout = TableLayout(); return pTableLayout? pTableLayout->GetColGroup(_iCol) : NULL; }

    //+-----------------------------------------------------------------------
    //  CLayout methods (formerly CSite methods)
    //------------------------------------------------------------------------

            void  Resize();

    virtual DWORD CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
            DWORD CalcSizeAtUserWidth(CCalcInfo * pci, SIZE * psize);

            void  DoLayout(DWORD grfLayout);
    virtual void  Notify(CNotification * pnf);

            DWORD GetCellBorderInfoDefault(CDocInfo * pdci, CBorderInfo * pborderinfo, BOOL fRender, CTable * pTable = NULL, CTableLayout * pTableLayout = NULL);
            DWORD GetCellBorderInfo(CDocInfo * pdci, CBorderInfo * pborderinfo, BOOL fRender = FALSE, HDC hdc = 0, CTable * pTable = NULL, CTableLayout * pTableLayout = NULL, BOOL * pfShrunkDCClipRegion = NULL);

    virtual void Draw(CFormDrawInfo *pDI, CDispNode * pDispNode);
    virtual void DrawBorder(CFormDrawInfo *pDI) { return; }
    virtual void DrawBorderHelper(CFormDrawInfo *pDI, BOOL * pfShrunkDCClipRegion = NULL);
    virtual void DrawClientBorder(const RECT * prcBounds, const RECT * prcRedraw, CDispSurface * pDispSurface, CDispNode * pDispNode, void * pClientData, DWORD dwFlags);

    virtual void PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo);

    //+-----------------------------------------------------------------------
    //  CFlowLayout methods (formerly CTxtSite methods)
    //------------------------------------------------------------------------

    virtual BOOL GetAutoSize () const;
    virtual void MarkHasAlignedLayouts(BOOL fHasAlignedLayouts) { _fAlignedLayouts = fHasAlignedLayouts; }

    //+-----------------------------------------------------------------------
    //  CTableCell methods
    //------------------------------------------------------------------------

#if DBG == 1
    void Print(HANDLE pF, int iTabs);
#endif

    // Helper functions.
    BOOL    NoContent()
    {
#ifndef QUILL
        return (_dp.NoContent() && _dp.LineCount() <= 1);
#else
        return IsZeroWidthWrapper();
#endif
    }

#ifndef NO_DATABINDING
    inline BOOL IsGenerated();
#endif

    // Get user's specified width in pixel (0 if not specified or if specified in percent).
    int GetSpecifiedPixelWidth(CDocInfo * pdci);
    int GetBorderAndPaddingWidth( CDocInfo *pdci, BOOL fOnlyBorderWidths = FALSE );

    virtual LONG GetMaxLineWidth();
    inline int ColIndex() const {return _iCol;}

    // helper function to handle position/display property changes
    void    HandlePositionDisplayChange();

protected:

    int             _iCol;
    int             _yBaseLine;
    SIZE            _sizeCell;      // TRUE size of the cell, based on the size calculated
                                    // by the text engine. We use this to calculate the inset
                                    // for a proper Vertical Alignment.
    unsigned int    _fDelayCalc:1;  // TRUE if the calculation of the cell is delayed
    unsigned int    _fAlignedLayouts:1;  // Table cell contains aligned layouts
    unsigned int    _fForceMinMaxOnResize : 1;  // force min max on resize
    unsigned int    _fNotInAryCells: 1;  // display none or absolute positioned cell
    
private:
    NO_COPY(CTableCellLayout);

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'ltcell.hxx'")
#else
#pragma INCMSG("*** Dup 'ltcell.hxx'")
#endif
