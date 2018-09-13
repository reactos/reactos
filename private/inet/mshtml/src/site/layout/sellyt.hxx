//+----------------------------------------------------------------------------
//
// File:        SELLYT.HXX
//
// Contents:    CSelectLayout
//
// Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_SELLYT_HXX_
#define I_SELLYT_HXX_
#pragma INCMSG("--- Beg 'sellyt.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

MtExtern(CSelectLayout)

class CSelectLayout : public CLayout
{
public:

    typedef CLayout super;

    CSelectLayout(CElement * pElementLayout) : super(pElementLayout)
    {
    }
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSelectLayout))

    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);
            HRESULT EnsureDefaultSize(CCalcInfo * pci);
            HRESULT AcquireFont(CCalcInfo * pci);
            void    DrawControl(CFormDrawInfo *pDI, BOOL fPrinting);
    virtual void    Draw(CFormDrawInfo *pDI, CDispNode *);
    virtual void    HandleViewChange(
                        DWORD          flags, 
                        const RECT *   prcClient,
                        const RECT *   prcClip,
                        CDispNode *    pDispNode);
    virtual BOOL    ProcessDisplayTreeTraversal(void *pClientData);
    
    virtual CLayout       * IsFlowOrSelectLayout() { return this; }

            void    InternalNotify(void);


    BOOL IsDirty(void)  { return _fDirty; };
    void Dirty(void)    { _fDirty = TRUE; };

    virtual HRESULT OnFormatsChange(DWORD);

    virtual void    DoLayout(DWORD grfLayout);
            void    Notify(CNotification * pnf);

    virtual HRESULT GetChildElementTopLeft(POINT & pt, CElement * pChild);

    unsigned    _fFirstVisible : 1;
    unsigned    _fDirty        : 1;
    unsigned    _fRightToLeft  : 1;

private:

#if DBG == 1
    long        _cCalcSize;
    long        _cEnsureDefaultSize;
#endif

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

#pragma INCMSG("--- End 'sellyt.hxx'")
#else
#pragma INCMSG("*** Dup 'sellyt.hxx'")
#endif
