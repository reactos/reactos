//
//    CardLib - not much of interest in here
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include "cardlib.h"
#include "globals.h"

void LoadCardBitmaps(void);

//static bool __CARDLIB_ACES_HIGH = false;
extern double __CARDZOOMSPEED;

//
//    Global variables!
//
HDC     __hdcCardBitmaps;
HBITMAP __hbmCardBitmaps;

HDC        __hdcPlaceHolder;
HBITMAP    __hbmPlaceHolder;
HPALETTE __holdplacepal;

int        __cardwidth;
int        __cardheight;

HPALETTE __hPalette;


//
//    Cardlib global functions!
//
void CardLib_SetZoomSpeed(int speed)
{
    __CARDZOOMSPEED = (double)speed;
}

/*

  It's dangerous to use these operators, because of all
  the implicit conversions that could take place, which
  would have unpredicted side-effects.

  e.g. Card card(Hearts, 4);
       if(card == 4)    - how does 4 get converted??
                          It uses the Card(int uval) constructor,
                          which results in a 2 of clubs...
                          not what was expected
*/ 
/*
void CardLib_SetAcesHigh(bool fHigh);
bool operator != (const Card &lhs, const Card &rhs);
bool operator == (const Card &lhs, const Card &rhs);
bool operator <  (const Card &lhs, const Card &rhs);
bool operator <= (const Card &lhs, const Card &rhs);
bool operator >  (const Card &lhs, const Card &rhs);
bool operator >= (const Card &lhs, const Card &rhs);
*/

/*
void CardLib_SetAcesHigh(bool fHigh)
{
    __CARDLIB_ACES_HIGH = fHigh;
}

bool operator == (const Card &lhs, const Card &rhs)
{
    if(__CARDLIB_ACES_HIGH)
        return lhs.HiVal() == rhs.HiVal();
    else
        return lhs.LoVal() == rhs.LoVal();
}

bool operator != (const Card &lhs, const Card &rhs)
{
    if(__CARDLIB_ACES_HIGH)
        return lhs.HiVal() != rhs.HiVal();
    else
        return lhs.LoVal() != rhs.LoVal();
}

bool operator > (const Card &lhs, const Card &rhs)
{
    if(__CARDLIB_ACES_HIGH)
        return lhs.HiVal() > rhs.HiVal();
    else
        return lhs.LoVal() > rhs.LoVal();
}

bool operator >= (const Card &lhs, const Card &rhs)
{
    if(__CARDLIB_ACES_HIGH)
        return lhs.HiVal() >= rhs.HiVal();
    else
        return lhs.LoVal() >= rhs.LoVal();
}

bool operator < (const Card &lhs, const Card &rhs)
{
    if(__CARDLIB_ACES_HIGH)
        return lhs.HiVal() < rhs.HiVal();
    else
        return lhs.LoVal() < rhs.LoVal();
}

bool operator <= (const Card &lhs, const Card &rhs)
{
    if(__CARDLIB_ACES_HIGH)
        return lhs.HiVal() <= rhs.HiVal();
    else
        return lhs.LoVal() <= rhs.LoVal();
}
*/

void PaintRect(HDC hdc, RECT *rect, COLORREF colour)
{
    COLORREF oldcr = SetBkColor(hdc, colour);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, rect, "", 0, 0);
    SetBkColor(hdc, oldcr);
}
