//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispextras.cxx
//
//  Contents:   Extra information that can be added to CDispXXXPlus nodes.
//
//  Classes:    CDispExtras
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPEXTRAS_HXX_
#define X_DISPEXTRAS_HXX_
#include "dispextras.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#define DEFINE_FLAG(FLAGNAME,FLAGVALUE) \
    const CDispExtras::CFlags CDispExtras::FLAGNAME (FLAGVALUE);
#define DEFINE_SHIFT(SHIFTNAME,SHIFTVALUE) \
    const int CDispExtras::SHIFTNAME (SHIFTVALUE);
#define DEFINE_VALUE(VALUENAME,FLAGVALUE) \
    static const LONG VALUENAME (FLAGVALUE);

#define EXTRAFLAGS_BORDERTYPE               (0x03 << 0)
#define EXTRAFLAGS_BORDERTYPE_SHIFT         0

#define EXTRAFLAGS_HASUSERCLIP              (1 << 2)

#define EXTRAFLAGS_HASINSET                 (1 << 3)

#define EXTRAFLAGS_HASEXTRACOOKIE           (1 << 4)

#define EXTRAFLAGS_EXTRASMASK               (0x0F)
#define EXTRAFLAGS_EXTRAS_SHIFT             0

#define EXTRAFLAGS_EXTRASWITHCOOKIEMASK     (0x1F)
#define EXTRAFLAGS_EXTRASWITHCOOKIE_SHIFT   0

DEFINE_FLAG(    s_borderType,               EXTRAFLAGS_BORDERTYPE)
DEFINE_SHIFT(   s_borderTypeShift,          EXTRAFLAGS_BORDERTYPE_SHIFT)
DEFINE_FLAG(    s_fHasUserClip,             EXTRAFLAGS_HASUSERCLIP)
DEFINE_FLAG(    s_fHasInset,                EXTRAFLAGS_HASINSET)
DEFINE_FLAG(    s_fHasExtraCookie,          EXTRAFLAGS_HASEXTRACOOKIE)

DEFINE_FLAG(    s_extrasMask,               EXTRAFLAGS_EXTRASMASK)
DEFINE_SHIFT(   s_extrasShift,              EXTRAFLAGS_EXTRAS_SHIFT)
DEFINE_FLAG(    s_extrasWithCookieMask,     EXTRAFLAGS_EXTRASWITHCOOKIEMASK)
DEFINE_SHIFT(   s_extrasWithCookieShift,    EXTRAFLAGS_EXTRASWITHCOOKIE_SHIFT)

DEFINE_VALUE(   s_ck0uc0in0b0,  (0 << 4) + (0 << 2) + (0 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck0uc0in0bs,  (0 << 4) + (0 << 2) + (0 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck0uc0in0bc,  (0 << 4) + (0 << 2) + (0 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck0uc0in1b0,  (0 << 4) + (0 << 2) + (1 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck0uc0in1bs,  (0 << 4) + (0 << 2) + (1 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck0uc0in1bc,  (0 << 4) + (0 << 2) + (1 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck0uc1in0b0,  (0 << 4) + (1 << 2) + (0 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck0uc1in0bs,  (0 << 4) + (1 << 2) + (0 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck0uc1in0bc,  (0 << 4) + (1 << 2) + (0 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck0uc1in1b0,  (0 << 4) + (1 << 2) + (1 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck0uc1in1bs,  (0 << 4) + (1 << 2) + (1 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck0uc1in1bc,  (0 << 4) + (1 << 2) + (1 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck1uc0in0b0,  (1 << 4) + (0 << 2) + (0 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck1uc0in0bs,  (1 << 4) + (0 << 2) + (0 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck1uc0in0bc,  (1 << 4) + (0 << 2) + (0 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck1uc0in1b0,  (1 << 4) + (0 << 2) + (1 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck1uc0in1bs,  (1 << 4) + (0 << 2) + (1 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck1uc0in1bc,  (1 << 4) + (0 << 2) + (1 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck1uc1in0b0,  (1 << 4) + (1 << 2) + (0 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck1uc1in0bs,  (1 << 4) + (1 << 2) + (0 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck1uc1in0bc,  (1 << 4) + (1 << 2) + (0 << 3) + DISPNODEBORDER_COMPLEX)
DEFINE_VALUE(   s_ck1uc1in1b0,  (1 << 4) + (1 << 2) + (1 << 3) + DISPNODEBORDER_NONE)
DEFINE_VALUE(   s_ck1uc1in1bs,  (1 << 4) + (1 << 2) + (1 << 3) + DISPNODEBORDER_SIMPLE)
DEFINE_VALUE(   s_ck1uc1in1bc,  (1 << 4) + (1 << 2) + (1 << 3) + DISPNODEBORDER_COMPLEX)


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::GetExtraSizes
//              
//  Synopsis:   Return size of extra information for nodes.
//              
//  Arguments:  fHasExtraCookie     TRUE if this node has optional cookie
//              fHasUserClip        TRUE if this node has user clip rect
//              fHasInset           TRUE if this node has inset
//              borderType          type of borders for this node
//              
//  Returns:    size of extra information
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

size_t
CDispExtras::GetExtrasSize(
        BOOL fHasExtraCookie,
        BOOL fHasUserClip,
        BOOL fHasInset,
        DISPNODEBORDER borderType)
{
    #ifdef _WIN64
    #define AlignToPtr(x)       (((x)+7)&(~7))
    #else
    #define AlignToPtr(x)       (x)
    #endif

    return
        AlignToPtr(sizeof(CDispExtras::CFlags)) +
        AlignToPtr((fHasExtraCookie ? sizeof(void*) : 0) +
                    (fHasUserClip ? sizeof(RECT) : 0) +
                    (fHasInset ? sizeof(SIZE) : 0) +
                    (borderType == DISPNODEBORDER_NONE ? 0
                        : (borderType == DISPNODEBORDER_SIMPLE 
                           ? sizeof(LONG) : sizeof(RECT))));
}

size_t
CDispExtras::GetExtrasSize(const CDispExtras* pExtras)
{
    return (pExtras == NULL ? 0 :
        GetExtrasSize(
            pExtras->HasExtraCookie(),
            pExtras->HasUserClip(),
            pExtras->HasInset(),
            pExtras->GetBorderType()));
}



//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::GetExtraCookie
//              
//  Synopsis:   Return value of extra cookie.
//              
//  Arguments:  none
//              
//  Returns:    value of extra cookie
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void*
CDispExtras::GetExtraCookie() const
{
    switch (GetExtrasCodeWithCookie())
    {
    case s_ck1uc0in0b0: return _ck1uc0in0b0._cookie;
    case s_ck1uc0in0bs: return _ck1uc0in0bs._cookie;
    case s_ck1uc0in0bc: return _ck1uc0in0bc._cookie;
    case s_ck1uc0in1b0: return _ck1uc0in1b0._cookie;
    case s_ck1uc0in1bs: return _ck1uc0in1bs._cookie;
    case s_ck1uc0in1bc: return _ck1uc0in1bc._cookie;
    case s_ck1uc1in0b0: return _ck1uc1in0b0._cookie;
    case s_ck1uc1in0bs: return _ck1uc1in0bs._cookie;
    case s_ck1uc1in0bc: return _ck1uc1in0bc._cookie;
    case s_ck1uc1in1b0: return _ck1uc1in1b0._cookie;
    case s_ck1uc1in1bs: return _ck1uc1in1bs._cookie;
    case s_ck1uc1in1bc: return _ck1uc1in1bc._cookie;
    default:
        AssertSz(FALSE, "Tried to read non-existent optional cookie");
        break;
    }
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::SetExtraCookie
//              
//  Synopsis:   Set extra cookie value.
//              
//  Arguments:  cookie      the cookie
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispExtras::SetExtraCookie(void* cookie)
{
    switch (GetExtrasCodeWithCookie())
    {
    case s_ck1uc0in0b0: _ck1uc0in0b0._cookie = cookie; break;
    case s_ck1uc0in0bs: _ck1uc0in0bs._cookie = cookie; break;
    case s_ck1uc0in0bc: _ck1uc0in0bc._cookie = cookie; break;
    case s_ck1uc0in1b0: _ck1uc0in1b0._cookie = cookie; break;
    case s_ck1uc0in1bs: _ck1uc0in1bs._cookie = cookie; break;
    case s_ck1uc0in1bc: _ck1uc0in1bc._cookie = cookie; break;
    case s_ck1uc1in0b0: _ck1uc1in0b0._cookie = cookie; break;
    case s_ck1uc1in0bs: _ck1uc1in0bs._cookie = cookie; break;
    case s_ck1uc1in0bc: _ck1uc1in0bc._cookie = cookie; break;
    case s_ck1uc1in1b0: _ck1uc1in1b0._cookie = cookie; break;
    case s_ck1uc1in1bs: _ck1uc1in1bs._cookie = cookie; break;
    case s_ck1uc1in1bc: _ck1uc1in1bc._cookie = cookie; break;
    default:
        AssertSz(FALSE, "Tried to set non-existent optional cookie");
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::GetUserClip
//              
//  Synopsis:   Get optional user clip rect.
//              
//  Arguments:  none
//              
//  Returns:    reference to optional user clip rect
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

const RECT&
CDispExtras::GetUserClip() const
{
    switch (GetExtrasCode())
    {
    case s_ck0uc1in0b0: return _ck0uc1in0b0._rcUserClip;
    case s_ck0uc1in0bs: return _ck0uc1in0bs._rcUserClip;
    case s_ck0uc1in0bc: return _ck0uc1in0bc._rcUserClip;
    case s_ck0uc1in1b0: return _ck0uc1in1b0._rcUserClip;
    case s_ck0uc1in1bs: return _ck0uc1in1bs._rcUserClip;
    case s_ck0uc1in1bc: return _ck0uc1in1bc._rcUserClip;
    default:
        AssertSz(FALSE, "Tried to read non-existent user clip");
        break;
    }
    return g_Zero.rc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::SetUserClip
//              
//  Synopsis:   Set optional user clip rect.
//              
//  Arguments:  rcUserClip      user clip rect
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispExtras::SetUserClip(const RECT& rcUserClip)
{
    // restrict user clip values to a reasonable range so translation
    // will work without overflow or underflow.
    CRect rc(rcUserClip);
    if (rc.left < LONG_MIN/2) rc.left = LONG_MIN/2;
    if (rc.left > LONG_MAX/2) rc.left = LONG_MAX/2;
    if (rc.top < LONG_MIN/2) rc.top = LONG_MIN/2;
    if (rc.top > LONG_MAX/2) rc.top = LONG_MAX/2;
    if (rc.right < LONG_MIN/2) rc.right = LONG_MIN/2;
    if (rc.right > LONG_MAX/2) rc.right = LONG_MAX/2;
    if (rc.bottom < LONG_MIN/2) rc.bottom = LONG_MIN/2;
    if (rc.bottom > LONG_MAX/2) rc.bottom = LONG_MAX/2;
    
    switch (GetExtrasCode())
    {
    case s_ck0uc1in0b0: _ck0uc1in0b0._rcUserClip = rc; break;
    case s_ck0uc1in0bs: _ck0uc1in0bs._rcUserClip = rc; break;
    case s_ck0uc1in0bc: _ck0uc1in0bc._rcUserClip = rc; break;
    case s_ck0uc1in1b0: _ck0uc1in1b0._rcUserClip = rc; break;
    case s_ck0uc1in1bs: _ck0uc1in1bs._rcUserClip = rc; break;
    case s_ck0uc1in1bc: _ck0uc1in1bc._rcUserClip = rc; break;
    default:
        AssertSz(FALSE, "Tried to set non-existent user clip");
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::GetInset
//              
//  Synopsis:   Return optional inset value.
//              
//  Arguments:  none
//              
//  Returns:    reference to inset amount
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

const SIZE&
CDispExtras::GetInset() const
{
    switch (GetExtrasCode())
    {
    case s_ck0uc0in1b0:    return _ck0uc0in1b0._sizeInset;
    case s_ck0uc0in1bs:    return _ck0uc0in1bs._sizeInset;
    case s_ck0uc0in1bc:    return _ck0uc0in1bc._sizeInset;
    case s_ck0uc1in1b0:    return _ck0uc1in1b0._sizeInset;
    case s_ck0uc1in1bs:    return _ck0uc1in1bs._sizeInset;
    case s_ck0uc1in1bc:    return _ck0uc1in1bc._sizeInset;
    }
    
    AssertSz(FALSE, "Tried to read non-existent inset");
    return g_Zero.size;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::SetInset
//              
//  Synopsis:   Set optional inset amount for this node.
//              
//  Arguments:  sizeInset       inset amount
//              
//  Notes:      Right to left nodes should have negative values for _sizeInset.cx
//              Origin for RTL is top/right
//              
//----------------------------------------------------------------------------

void
CDispExtras::SetInset(const SIZE& sizeInset)
{
    switch (GetExtrasCode())
    {
    case s_ck0uc0in1b0: _ck0uc0in1b0._sizeInset = sizeInset; break;
    case s_ck0uc0in1bs: _ck0uc0in1bs._sizeInset = sizeInset; break;
    case s_ck0uc0in1bc: _ck0uc0in1bc._sizeInset = sizeInset; break;
    case s_ck0uc1in1b0: _ck0uc1in1b0._sizeInset = sizeInset; break;
    case s_ck0uc1in1bs: _ck0uc1in1bs._sizeInset = sizeInset; break;
    case s_ck0uc1in1bc: _ck0uc1in1bc._sizeInset = sizeInset; break;
    default:
        AssertSz(FALSE, "Tried to set non-existent inset");
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::GetBorderWidths
//              
//  Synopsis:   Return border widths.
//              
//  Arguments:  prcBorderWidths     RECT-like struct returns width of each border
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispExtras::GetBorderWidths(CRect* prcBorderWidths) const
{
    switch (GetExtrasCode())
    {
    case s_ck0uc0in0b0:
        prcBorderWidths->SetRect(0);
        break;
    case s_ck0uc0in0bs:
        prcBorderWidths->SetRect(_ck0uc0in0bs._borderWidth);
        break;
    case s_ck0uc0in0bc:
        *prcBorderWidths = _ck0uc0in0bc._rcBorderWidths;
        break;
    case s_ck0uc0in1b0:
        prcBorderWidths->SetRect(0);
        break;
    case s_ck0uc0in1bs:
        prcBorderWidths->SetRect(_ck0uc0in1bs._borderWidth);
        break;
    case s_ck0uc0in1bc:
        *prcBorderWidths = _ck0uc0in1bc._rcBorderWidths;
        break;
    case s_ck0uc1in0b0:
        prcBorderWidths->SetRect(0);
        break;
    case s_ck0uc1in0bs:
        prcBorderWidths->SetRect(_ck0uc1in0bs._borderWidth);
        break;
    case s_ck0uc1in0bc:
        *prcBorderWidths = _ck0uc1in0bc._rcBorderWidths;
        break;
    case s_ck0uc1in1b0:
        prcBorderWidths->SetRect(0);
        break;
    case s_ck0uc1in1bs:
        prcBorderWidths->SetRect(_ck0uc1in1bs._borderWidth);
        break;
    case s_ck0uc1in1bc:
        *prcBorderWidths = _ck0uc1in1bc._rcBorderWidths;
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::SetBorderWidths
//              
//  Synopsis:   Set border widths.
//              
//  Arguments:  borderWidth     width of all border sides
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispExtras::SetBorderWidths(LONG borderWidth)
{
    switch (GetExtrasCode())
    {
    case s_ck0uc0in0b0:
        Assert(FALSE);
        break;
    case s_ck0uc0in0bs:
        _ck0uc0in0bs._borderWidth = borderWidth;
        break;
    case s_ck0uc0in0bc:
        ((CRect&)_ck0uc0in0bc._rcBorderWidths).SetRect(borderWidth);
        break;
    case s_ck0uc0in1b0:
        Assert(FALSE);
        break;
    case s_ck0uc0in1bs:
        _ck0uc0in1bs._borderWidth = borderWidth;
        break;
    case s_ck0uc0in1bc:
        ((CRect&)_ck0uc0in1bc._rcBorderWidths).SetRect(borderWidth);
        break;
    case s_ck0uc1in0b0:
        Assert(FALSE);
        break;
    case s_ck0uc1in0bs:
        _ck0uc1in0bs._borderWidth = borderWidth;
        break;
    case s_ck0uc1in0bc:
        ((CRect&)_ck0uc1in0bc._rcBorderWidths).SetRect(borderWidth);
        break;
    case s_ck0uc1in1b0:
        Assert(FALSE);
        break;
    case s_ck0uc1in1bs:
        _ck0uc1in1bs._borderWidth = borderWidth;
        break;
    case s_ck0uc1in1bc:
        ((CRect&)_ck0uc1in1bc._rcBorderWidths).SetRect(borderWidth);
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::SetBorderWidths
//              
//  Synopsis:   Set border width.
//              
//  Arguments:  rcBorderWidths  RECT-like struct containing all border widths
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispExtras::SetBorderWidths(const RECT& rcBorderWidths)
{
    switch (GetExtrasCode())
    {
    case s_ck0uc0in0b0:
        Assert(FALSE);
        break;
    case s_ck0uc0in0bs:
        _ck0uc0in0bs._borderWidth = rcBorderWidths.left;
        break;
    case s_ck0uc0in0bc:
        _ck0uc0in0bc._rcBorderWidths = rcBorderWidths;
        break;
    case s_ck0uc0in1b0:
        Assert(FALSE);
        break;
    case s_ck0uc0in1bs:
        _ck0uc0in1bs._borderWidth = rcBorderWidths.left;
        break;
    case s_ck0uc0in1bc:
        _ck0uc0in1bc._rcBorderWidths = rcBorderWidths;
        break;
    case s_ck0uc1in0b0:
        Assert(FALSE);
        break;
    case s_ck0uc1in0bs:
        _ck0uc1in0bs._borderWidth = rcBorderWidths.left;
        break;
    case s_ck0uc1in0bc:
        _ck0uc1in0bc._rcBorderWidths = rcBorderWidths;
        break;
    case s_ck0uc1in1b0:
        Assert(FALSE);
        break;
    case s_ck0uc1in1bs:
        _ck0uc1in1bs._borderWidth = rcBorderWidths.left;
        break;
    case s_ck0uc1in1bc:
        _ck0uc1in1bc._rcBorderWidths = rcBorderWidths;
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispExtras::GetExtraInfo
//              
//  Synopsis:   Return extra information including optional user clip rect,
//              optional inset amount, and optional border widths.
//              
//  Arguments:  pInfo           returns extra information
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispExtras::GetExtraInfo(CDispInfo* pInfo) const
{
    switch (GetExtrasCode())
    {
    case s_ck0uc0in0b0:
        pInfo->_pInsetOffset = &((CSize&)g_Zero.size);
        pInfo->_prcBorderWidths = &((CRect&)g_Zero.rc);
        break;
    case s_ck0uc0in0bs:
        pInfo->_pInsetOffset = &((CSize&)g_Zero.size);
        pInfo->_prcBorderWidths = &(pInfo->_rcTemp);
        pInfo->_rcTemp.SetRect(_ck0uc0in0bs._borderWidth);
        break;
    case s_ck0uc0in0bc:
        pInfo->_pInsetOffset = &((CSize&)g_Zero.size);
        pInfo->_prcBorderWidths = &((CRect&)_ck0uc0in0bc._rcBorderWidths);
        break;
    case s_ck0uc0in1b0:
        pInfo->_pInsetOffset = &((CSize&)_ck0uc0in1b0._sizeInset);
        pInfo->_prcBorderWidths = &((CRect&)g_Zero.rc);
        break;
    case s_ck0uc0in1bs:
        pInfo->_pInsetOffset = &((CSize&)_ck0uc0in1bs._sizeInset);
        pInfo->_prcBorderWidths = &(pInfo->_rcTemp);
        pInfo->_rcTemp.SetRect(_ck0uc0in1bs._borderWidth);
        break;
    case s_ck0uc0in1bc:
        pInfo->_pInsetOffset = &((CSize&)_ck0uc0in1bc._sizeInset);
        pInfo->_prcBorderWidths = &((CRect&)_ck0uc0in1bc._rcBorderWidths);
        break;
    case s_ck0uc1in0b0:
        pInfo->_pInsetOffset = &((CSize&)g_Zero.size);
        pInfo->_prcBorderWidths = &((CRect&)g_Zero.rc);
        break;
    case s_ck0uc1in0bs:
        pInfo->_pInsetOffset = &((CSize&)g_Zero.size);
        pInfo->_prcBorderWidths = &(pInfo->_rcTemp);
        pInfo->_rcTemp.SetRect(_ck0uc1in0bs._borderWidth);
        break;
    case s_ck0uc1in0bc:
        pInfo->_pInsetOffset = &((CSize&)g_Zero.size);
        pInfo->_prcBorderWidths = &((CRect&)_ck0uc1in0bc._rcBorderWidths);
        break;
    case s_ck0uc1in1b0:
        pInfo->_pInsetOffset = &((CSize&)_ck0uc1in1b0._sizeInset);
        pInfo->_prcBorderWidths = &((CRect&)g_Zero.rc);
        break;
    case s_ck0uc1in1bs:
        pInfo->_pInsetOffset = &((CSize&)_ck0uc1in1bs._sizeInset);
        pInfo->_prcBorderWidths = &(pInfo->_rcTemp);
        pInfo->_rcTemp.SetRect(_ck0uc1in1bs._borderWidth);
        break;
    case s_ck0uc1in1bc:
        pInfo->_pInsetOffset = &((CSize&)_ck0uc1in1bc._sizeInset);
        pInfo->_prcBorderWidths = &((CRect&)_ck0uc1in1bc._rcBorderWidths);
        break;
    }
}


