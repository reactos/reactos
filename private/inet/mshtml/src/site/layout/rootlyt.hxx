//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       rootlyt.hxx
//
//  Contents:   CRootLayout
//
//----------------------------------------------------------------------------

#ifndef I_ROOTLYT_HXX_
#define I_ROOTLYT_HXX_
#pragma INCMSG("--- Beg 'rootlyt.hxx'")

#ifdef  NEVER

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CRootLayout
//
//  Synopsis:   The root of all evil.
//
//----------------------------------------------------------------------------

MtExtern(CRootLayout)

class CRootLayout : public CFlowLayout
{
    typedef CFlowLayout super;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRootLayout))


    CRootLayout (CElement *pElementLayout);  // Normal constructor.
    virtual BOOL    CanHaveChildren() { return TRUE; }

    virtual DWORD   CalcSize(CCalcInfo * pci, SIZE * psize, SIZE * psizeDefault = NULL);

    virtual BOOL    GetBackgroundInfo(CFormDrawInfo * pDI, BACKGROUNDINFO * pbginfo, BOOL fAll = TRUE);


    long        GetXScroll();
    long        GetXScrollEx();
    long        GetYScroll();

    unsigned    _fDirty:1;

    void        DoLayout(DWORD grfLayout);
    void        Notify(CNotification * pnf);

    BOOL        IsDirty()
                {
                    return !!_fDirty;
                }


    void        Reset();

    CElement *      ClientSite() { return GetContentMarkup()->GetElementClient(); }
    CRootElement *  MarkupRoot()  { return GetContentMarkup()->Root(); }

protected:
    DECLARE_LAYOUTDESC_MEMBERS;
};

inline CLayout *
CDoc::PrimaryTopLayout()
{
    Assert(GetElementClient());

    return GetElementClient()->GetLayout();
}

#endif

#pragma INCMSG("--- End 'rootlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'rootlyt.hxx'")
#endif
