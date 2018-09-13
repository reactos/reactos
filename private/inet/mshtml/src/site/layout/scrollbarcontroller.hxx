//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       scrollbarcontroller.hxx
//
//  Contents:   Transient object to control scroll bar during user interaction.
//
//----------------------------------------------------------------------------

#ifndef I_SCROLLBARCONTROLLER_HXX_
#define I_SCROLLBARCONTROLLER_HXX_
#pragma INCMSG("--- Beg 'scrollbarcontroller.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SCROLLBAR_HXX_
#define X_SCROLLBAR_HXX_
#include "scrollbar.hxx"
#endif

class CLayout;
class CDispScroller;
class CServer;
class CMessage;

//+---------------------------------------------------------------------------
//
//  Class:      CScrollbarController
//
//  Synopsis:   Transient object to control scroll bar during user interaction.
//
//----------------------------------------------------------------------------

class CScrollbarController
        : public CScrollbar, public CVoid
{
public:
    static void             StartScrollbarController(
                                CLayout* pLayout,
                                CDispScroller* pDispScroller,
                                CServer* pServerHost,
                                long buttonWidth,
                                CMessage* pMessage);
    
    static void             StopScrollbarController();
    
private:
    friend HRESULT InitScrollbar(THREADSTATE *);
    friend void DeinitScrollbar(THREADSTATE *);
    
                            // constructor is private,
                            // use InitScrollbar or StartScrollbarController
                            CScrollbarController()
                                    {_pLayout = NULL;}
                            // destructor is private, because this object
                            // is deleted only by DeinitScrollbar
                            ~CScrollbarController()
                                    {AssertSz(_pLayout == NULL,
                                     "CScrollbarController destructed prematurely");}
                            
public:
    CLayout*                GetLayout() const {return _pLayout;}
    CDrawInfo*              GetDrawInfo() {return &_drawInfo;}

    SCROLLBARPART           GetPartPressed() const {return _partPressed;}
    
    static long             GetRepeatDelay()    {return 250;}
    static long             GetRepeatRate()     {return 50; }
    static long             GetFocusRate()      {return 500;}
    
#ifndef _MAC    
private:
#endif
    NV_DECLARE_ONTICK_METHOD(OnTick, ontick, ( UINT idTimer ));  // For global window
    NV_DECLARE_ONMESSAGE_METHOD(OnMessage, onmessage, (UINT msg, WPARAM wParam, LPARAM lParam));

#ifdef _MAC
private:
#endif
    void                    MouseMove(const CPoint& pt);
    void                    GetScrollInfo(
                                LONG* pContentSize,
                                LONG* pContainerSize,
                                LONG* pScrollAmount) const;
   
#ifdef UNIX
    void                    MoveThumb(const CPoint& pt);
#endif
 
    SCROLLBARPART           _partPressedStart;
    SCROLLBARPART           _partPressed;
    CLayout*                _pLayout;
    CDispScroller*          _pDispScroller;
    CServer*                _pServerHost;
    int                     _direction;
    long                    _buttonWidth;
    CRect                   _rcScrollbar;
    CPoint                  _ptMouse;
    long                    _mouseInThumb;
    CDrawInfo               _drawInfo;
};


#pragma INCMSG("--- End 'scrollbarcontroller.hxx'")
#else
#pragma INCMSG("*** Dup 'scrollbarcontroller.hxx'")
#endif

