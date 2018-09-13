//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       framelyt.hxx
//
//  Contents:   CFrameSetLayout class definition
//
//----------------------------------------------------------------------------

#ifndef I_FRAMELYT_HXX_
#define I_FRAMELYT_HXX_
#pragma INCMSG("--- Beg 'framelyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

MtExtern(CFrameSetLayout)

//+---------------------------------------------------------------------------
//
//  Class:      CFrameSetLayout (cfs)
//
//  Purpose:    Site class that implements the <FRAMESET> tag
//
//----------------------------------------------------------------------------

class CFrameSetLayout : public CLayout
{
    friend class CFrameSetSite;
    DECLARE_CLASS_TYPES(CFrameSetLayout, CLayout)

public:

    typedef CLayout super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFrameSetLayout))

    static int iPixelFrameHighlightWidth;
    static int iPixelFrameHighlightBuffer;

    CFrameSetLayout(CElement *pElementLayout);
    virtual ~CFrameSetLayout() { }

    CFrameSetSite * FrameSetElement()
        { return (CFrameSetSite *) ElementOwner(); }

    virtual DWORD CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual HRESULT GetElementsInZOrder(CPtrAry<CElement *> *parySites,
                                        CElement            *pElementThis,
                                        RECT                *prc,
                                        HRGN                 hrgn,
                                        BOOL                 fIncludeNotVisible);
    virtual HRESULT SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate);
    virtual HRESULT SelectSite(CLayout * pLayout, DWORD dwFlags);
    virtual BOOL    CanHaveChildren() { return TRUE; }

    void SetRowsCols();
    BOOL CheckFrame3DBorder(CDoc * pDoc, BYTE b3DBorder);

    void CalcPositions(CCalcInfo * pci, SIZE size);

    //  the following structs and functions are helpers for manual frame resizing
    struct FrameResizeInfo1 {
        int iTravel;
        int iEdge;        // -1 means no edge
        int iMinTravel;
        int iMaxTravel;
    RECT rcEdge;
    };

    struct FrameResizeInfo {
        FrameResizeInfo1 vertical, horizontal;
    };

    struct ManualResizeRecord {
        short iTravel;         // positive is down or right, negative is up or left
        short iEdge;           // which edge is moving; 0 is leftmost or topmost
                               //    -1 means the rightmost
        BOOL  fVerticalTravel; // TRUE is we're moving a horizontal edge
    };

    BOOL MaxTravelForEdge(int iEdge, BOOL fVerticalTravel,
                          int *pMaxTravel, int *pMinTravel);

    BOOL CancelManualResize(BOOL fVerticalTravel);
    int GetManualResizeTravel(BOOL fVertical, int iEdge);
    void Resize(const ManualResizeRecord *pMRR);
    BOOL GetResizeInfo(POINT pt, FrameResizeInfo *pFRI);
    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage);
    virtual BOOL GetBackgroundInfo(CFormDrawInfo * pDI, BACKGROUNDINFO * pbginfo, BOOL fAll = TRUE, BOOL fRightToLeft = FALSE);

    // Enumeration method to loop thru children (start)
    virtual CLayout * GetFirstLayout (DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE );

    // Enumeration method to loop thru children (continue)
    virtual CLayout * GetNextLayout (DWORD_PTR * pdw, BOOL fBack = FALSE, BOOL fRaw = FALSE );

    virtual void ClearLayoutIterator (DWORD_PTR dw, BOOL fRaw);

    virtual BOOL ContainsChildLayout(BOOL fRaw = FALSE);
    virtual void    Notify(CNotification *pNF);
    
    void    DoLayout(DWORD grfLayout);

    virtual void    Reset(BOOL fForce);

public:
    CPtrAry <CLayout *> &SiteArray()
    {
        return __aryLayouts;
    }

    CPtrAry <CLayout *> __aryLayouts;           // currently until tree services
                                                // provides us with the necessary
                                                // infomation (covers parent site)
protected:

    short _iNumActualRows, _iNumActualCols;
    CDataAry<CUnitValue>   _aryRows;
    CDataAry<CUnitValue>   _aryCols;
    CDataAry<ManualResizeRecord> _aryResizeRecords;

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'framelyt.hxx'")
#else
#pragma INCMSG("*** Dup 'framelyt.hxx'")
#endif
