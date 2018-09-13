//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995, 1996, 1997, 1998
//
//  File:       filter.hxx
//
//  Contents:   CFilter and related classes, constants, etc.
//
//-----------------------------------------------------------------------------

#ifndef I_FILTER_HXX_
#define I_FILTER_HXX_
#pragma INCMSG("--- Beg 'filter.hxx'")

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_HTMLFILTER_H_
#define X_HTMLFILTER_H
#include "htmlfilter.h"
#endif

#ifndef X_NOTIFY_HXX_
#define X_NOTIFY_HXX_
#include "notify.hxx"
#endif

#if DBG == 1
void ShowDeviceCoordinates(HDC hdc, LPRECT prc);
void ShowDeviceCoordinates(HDC hdc, LPPOINT ppt);
void ShowDeviceCoordinates(HDC hdc, int x, int y);
#endif

MtExtern(CFilter)


//+----------------------------------------------------------------------------
//
//  Class:  CFilter
//
//-----------------------------------------------------------------------------

class CFilter : public IHTMLViewFilter,
                public IHTMLViewFilterSite,
                public CDispFilter
{
    friend class CElement;

public:

    //
    //  Constructors, destructors, etc.
    //

    CFilter(CElement *pElement);
    ~CFilter();
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFilter))

    //
    //  IUnknown methods
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    //
    //  IHTMLViewFilter methods
    //

    STDMETHOD(SetSource)(IHTMLViewFilter *pFilter);
    STDMETHOD(GetSource)(IHTMLViewFilter **ppFilter);
    STDMETHOD(SetSite)(IHTMLViewFilterSite *pSink);
    STDMETHOD(GetSite)(IHTMLViewFilterSite **ppSink);
    STDMETHOD(SetPosition)(LPCRECT prc);
    STDMETHOD(Draw)(HDC hdc, LPCRECT prc);
    STDMETHOD(GetStatusBits)(DWORD *pdwFlags);

    //
    //  IHTMLViewFilterSite/IViewTransitionSite methods
    //

    STDMETHOD(GetDC)(LPCRECT prc, DWORD dwFlags, HDC *phdc);
    STDMETHOD(ReleaseDC)(HDC hdc);
    STDMETHOD(InvalidateRect)(LPCRECT prc, BOOL fErase);
    STDMETHOD(InvalidateRgn)(HRGN hrgn, BOOL fErase);
    STDMETHOD(OnStatusBitsChange)(DWORD dwFlags);
    
    // CDispFilter methods

    virtual void            SetSize(const SIZE& size);
    virtual void            DrawFiltered(CDispDrawContext* pContext);
    virtual void            Invalidate(const RECT& rc, BOOL fSynchronousRedraw);
    virtual void            Invalidate(HRGN hrgn, BOOL fSynchronousRedraw);
    virtual void            SetOpaque(BOOL fOpaque);

    //
    //  Internal methods
    //

    CDispFilter *GetDispFilter() { return (CDispFilter *) this; }

    HRESULT     AddFilter(IHTMLViewFilter *pViewFilter);
    HRESULT     RemoveFilter(IHTMLViewFilter *pViewFilter);

    void        EnsureFilterState();

    long        GetSourceIndex() const
                {
                    Assert(_pElement);
                    return _pElement->GetSourceIndex();
                }

    HRESULT     FilteredGetDC(LPCRECT prc, DWORD dwFlags, HDC *phdc);
    HRESULT     FilteredReleaseDC(HDC hdc);
    void        FilteredSetStatusBits(DWORD dwMask, DWORD dwFlags);
    BOOL        FilteredIsHidden(BOOL fHiddenOriginal);

    void        Detach(BOOL fCleanup = TRUE);

private:

    ULONG               _ulRefs;

    CElement *          _pElement;
    IHTMLViewFilter *       _pSource;
    IHTMLViewFilterSite *   _pFilterSite;

    SIZE                _size;
    DWORD               _dwStatus;

    union
    {
        DWORD           _grfFlags;

        struct
        {
            DWORD       _fInHookup          :1;
            DWORD       _fBlockHiddenChange :1;
            DWORD       _fDisplayHidden     :1;
            DWORD       _fNeedInvalidate    :1;
        };
    };

    CDispDrawContext * _pContext;   // Valid only during the context of a DrawFiltered call
    void        FinishUpdate();

    CDispNode *GetDispNode()
    {
        Assert(Element()->GetCurLayout());

        return Element()->GetCurLayout()->GetElementDispNode();
    }
    CElement * Element() { return _pElement; }
};

#pragma INCMSG("--- End 'filter.hxx'")
#else
#pragma INCMSG("*** Dup 'filter.hxx'")
#endif
