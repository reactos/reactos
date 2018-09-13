//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollbar.hxx
//
//  Contents:   Class to render default horizontal and vertical scrollbars.
//
//----------------------------------------------------------------------------

#ifndef I_SCROLLBAR_HXX_
#define I_SCROLLBAR_HXX_
#pragma INCMSG("--- Beg 'scrollbar.hxx'")

class CDrawInfo;
class CScrollbarParams;
class CDispNode;
class ThreeDColors;


//+---------------------------------------------------------------------------
//
//  Class:      CScrollbar
//
//  Synopsis:   Class to render default horizontal and vertical scrollbars.
//
//----------------------------------------------------------------------------

class CScrollbar
{
public:
                            // no construction necessary -- all static methods
                            CScrollbar() {}
                            ~CScrollbar() {}
    
    enum SCROLLBARPART
    {
        SB_NONE,
        SB_PREVBUTTON,
        SB_NEXTBUTTON,
        SB_TRACK,
        SB_PREVTRACK,
        SB_NEXTTRACK,
        SB_THUMB
    };
                            
    static void             Draw(
                                int direction,
                                const CRect& rcScrollbar,
                                const CRect& rcRedraw,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                SCROLLBARPART partPressed,
                                HDC hdc,
                                const CScrollbarParams& params,
                                CDrawInfo* pDI,
                                DWORD dwFlags);
    
    static SCROLLBARPART    GetPart(
                                int direction,
                                const CRect& rcScrollbar,
                                const CPoint& ptHit,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long buttonWidth,
                                CDrawInfo* pDI,
                                BOOL fRightToLeft);
    
    
    static void             GetPartRect(
                                CRect* prcPart,
                                SCROLLBARPART part,
                                int direction,
                                const CRect& rcScrollbar,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long buttonWidth,
                                CDrawInfo* pDI,
                                BOOL fRightToLeft);
    
    static long             GetScaledButtonWidth(
                                int direction,
                                const CRect& rcScrollbar,
                                long buttonWidth)
                                    {return min(buttonWidth,
                                                rcScrollbar.Size(direction)/2L);}
    
    static long             GetTrackSize(
                                int direction,
                                const CRect& rcScrollbar,
                                long buttonWidth)
                                    {return rcScrollbar.Size(direction) - 2 *
                                        GetScaledButtonWidth(
                                            direction,rcScrollbar,buttonWidth);}
    
    static long             GetThumbSize(
                                int direction,
                                const CRect& rcScrollbar,
                                long contentSize,
                                long containerSize,
                                long buttonWidth,
                                CDrawInfo* pDI);
    
    static long             GetThumbOffset(
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long trackSize,
                                long thumbSize)
                                { return MulDivQuick(trackSize-thumbSize, scrollAmount, contentSize-containerSize); }


    static void             InvalidatePart(
                                CScrollbar::SCROLLBARPART part,
                                int direction,
                                const CRect& rcScrollbar,
                                long contentSize,
                                long containerSize,
                                long scrollAmount,
                                long buttonWidth,
                                CDispNode* pDispNodeToInvalidate,
                                CDrawInfo* pDI);
    
private:
    static void             DrawTrack(
                                const CRect& rcTrack,
                                BOOL fPressed,
                                BOOL fDisabled,
                                HDC hdc,
                                const CScrollbarParams& params);
    static void             DrawThumb(
                                const CRect& rcThumb,
                                BOOL fPressed,
                                HDC hdc,
                                const CScrollbarParams& params,
                                CDrawInfo* pDI);
};


//+---------------------------------------------------------------------------
//
//  Class:      CScrollbarParams
//
//  Synopsis:   Customizable scroll bar parameters.
//
//----------------------------------------------------------------------------

class CScrollbarParams
{
public:
    ThreeDColors*   _pColors;
    
    LONG            _buttonWidth;
    BOOL            _fFlat;
    BOOL            _fForceDisabled;
#ifdef UNIX // Used for motif scrollbar
    BOOL            _bDirection;
#endif
};


#pragma INCMSG("--- End 'scrollbar.hxx'")
#else
#pragma INCMSG("*** Dup 'scrollbar.hxx'")
#endif

