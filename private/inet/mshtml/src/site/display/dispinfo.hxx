//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispinfo.hxx
//
//  Contents:   Display node information about coordinate system offsets,
//              clipping, and scrolling.
//
//----------------------------------------------------------------------------

#ifndef I_DISPINFO_HXX_
#define I_DISPINFO_HXX_
#pragma INCMSG("--- Beg 'dispinfo.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_DISPEXTRAS_HXX_
#define X_DISPEXTRAS_HXX_
#include "dispextras.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CDispInfo
//
//  Synopsis:   Structure for returning display node information regarding
//              coordinate system offsets, clipping, and scrolling.
//
//----------------------------------------------------------------------------

class CDispInfo
{
    friend class CDispExtras;
    
public:
    CDispInfo(const CDispExtras* pExtras)
            {if (pExtras == NULL)
                {_pInsetOffset = (const CSize*) &g_Zero.size;
                _prcBorderWidths = (const CRect*) &g_Zero.rc;}
            else pExtras->GetExtraInfo(this);}
    ~CDispInfo() {}
    
    // optional information filled in by CDispFlags::GetExtraInfo
    const CSize*    _pInsetOffset;      // never NULL
    const CRect*    _prcBorderWidths;   // never NULL
    
    // information filled in by CalcDispInfo virtual method
    CRect           _rcPositionedClip;  // clip positioned content (container coords.)
    CRect           _rcContainerClip;   // clip border and scroll bars (container coords.)
    CRect           _rcBackgroundClip;  // clip background content (content coords.)
    CRect           _rcFlowClip;        // clip flow content (flow coords.)
    CSize           _borderOffset;      // offset to border
    CSize           _contentOffset;     // offset to content inside border
    CSize           _scrollOffset;      // scroll offset
    CSize           _sizeContent;       // size of content
    CSize           _sizeBackground;    // size of background (exclude border and scrollbars)
    
private:
    // temporary rect used to store simple border widths
    CRect           _rcTemp;
};


#pragma INCMSG("--- End 'dispinfo.hxx'")
#else
#pragma INCMSG("*** Dup 'dispinfo.hxx'")
#endif

