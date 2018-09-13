//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       bodylyt.hxx
//
//  Contents:   CBodyLayout class
//
//----------------------------------------------------------------------------

#ifndef I_BODYLYT_HXX_
#define I_BODYLYT_HXX_

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

class CDispNode;
class CShape;

//+---------------------------------------------------------------------------
//
// CBodyLayout
//
//----------------------------------------------------------------------------

class CBodyLayout : public CFlowLayout
{
public:
    typedef CFlowLayout super;

    CBodyLayout(CElement *pElement)
      : CFlowLayout(pElement)
    {
        _fFocusRect     = FALSE;
        _fContentsAffectSize = FALSE;
    }

    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage);
    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
    virtual void    NotifyScrollEvent(RECT * prcScroll, SIZE * psizeScrollDelta);

    void            RequestFocusRect(BOOL fOn);
    void            RedrawFocusRect();
    HRESULT         GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape);

    BOOL            GetBackgroundInfo(CFormDrawInfo * pDI, BACKGROUNDINFO * pbginfo, BOOL fAll = TRUE, BOOL fRightToLeft = FALSE);

    virtual HRESULT OnSelectionChange(void);

    void            UpdateScrollInfo(CDispNodeInfo * pdni) const;

    CBodyElement * Body() { return (CBodyElement *)ElementOwner(); }


private:
    unsigned    _fFocusRect:1;          // Draw a focus rect

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#endif
