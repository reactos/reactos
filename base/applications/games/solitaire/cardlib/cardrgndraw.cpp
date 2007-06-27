//
//    CardLib - CardRegion drawing support
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include <stdlib.h>
#include "cardlib.h"
#include "cardregion.h"
#include "cardcolor.h"

HPALETTE UseNicePalette(HDC hdc, HPALETTE hPalette);
void PaintRect(HDC hdc, RECT *rect, COLORREF colour);
void CardBlt(HDC hdc, int x, int y, int nCardNum);
void DrawCard(HDC hdc, int x, int y, HDC hdcSource, int width, int height);

//
//    Draw specified card at position x, y
//    xoff   - source offset from left of card
//    yoff   - source offset from top of card
//    width  - width to draw
//    height - height to draw
//
void CardBlt(HDC hdc, int x, int y, int nCardNum)//, int xoff, int yoff, int width, int height)
{
    int sx = nCardNum * __cardwidth;
    int sy = 0;
    int width  = __cardwidth;
    int height = __cardheight;

    //draw main center band
    BitBlt(hdc, x+2, y, width - 4, height, __hdcCardBitmaps, sx+2, sy+0, SRCCOPY);

    //draw the two bits to the left
    BitBlt(hdc, x,   y+2, 1, height - 4, __hdcCardBitmaps, sx+0, sy+2, SRCCOPY);
    BitBlt(hdc, x+1, y+1, 1, height - 2, __hdcCardBitmaps, sx+1, sy+1, SRCCOPY);

    //draw the two bits to the right
    BitBlt(hdc, x+width-2, y+1, 1, height - 2, __hdcCardBitmaps, sx+width-2, sy+1, SRCCOPY);
    BitBlt(hdc, x+width-1, y+2, 1, height - 4, __hdcCardBitmaps, sx+width-1, sy+2, SRCCOPY);
}

//
//    Draw a shape this this:
//
//   ++++++++++++
//  ++++++++++++++
// ++            ++
//
void DrawHorzCardStrip(HDC hdc, int x, int y, int nCardNum, int height, BOOL fDrawTips)
{
    int sx  = nCardNum * __cardwidth;
    int sy  = 0;
    int one = 1;
    int two = 2;
    BOOL tips = fDrawTips ? FALSE : TRUE;

    if(height == 0) return;

    if(height < 0)
    {
        sy = sy + __cardheight;
        y  -= height;
        one = -one;
        two = -two;
    }

    // draw the main vertical band
    //
    BitBlt(hdc, x + 2, y, __cardwidth - 4, height, __hdcCardBitmaps, sx+2, sy, SRCCOPY);

    //if(height <= 1) return;

    // draw the "lips" at the left and right
    BitBlt(hdc, x+1,             y+one, 1, height-one*tips, __hdcCardBitmaps, sx+1,             sy+one, SRCCOPY);
    BitBlt(hdc, x+__cardwidth-2, y+one, 1, height-one*tips, __hdcCardBitmaps, sx+__cardwidth-2, sy+one, SRCCOPY);

    //if(height <= 2) return;

    // draw the outer-most lips
    BitBlt(hdc, x,               y+two, 1, height-two*tips, __hdcCardBitmaps, sx,               sy+two, SRCCOPY);
    BitBlt(hdc, x+__cardwidth-1, y+two, 1, height-two*tips, __hdcCardBitmaps, sx+__cardwidth-1, sy+two, SRCCOPY);    
}

//
//    Draw a shape like this:
//
//     +++
//      +++
//   +++
//   +++
//   +++
//   +++
//   +++
//   +++
//    +++
//     +++
//
//
void DrawVertCardStrip(HDC hdc, int x, int y, int nCardNum, int width, BOOL fDrawTips)
{
    int sx  = nCardNum * __cardwidth;
    int sy  = 0;
    int one = 1;
    int two = 2;
    BOOL tips = fDrawTips ? FALSE : TRUE;

    if(width == 0) return;
    

    if(width < 0)
    {
        sx = sx + __cardwidth;
        x  -= width;
        one = -1;
        two = -2;
    }

    // draw the main vertical band
    //
    BitBlt(hdc, x, y + 2, width, __cardheight - 4, __hdcCardBitmaps, sx, sy+2, SRCCOPY);

    //if(width <= 1) return;

    // draw the "lips" at the top and bottom
    BitBlt(hdc, x+one, y+1,              width-one*tips, 1, __hdcCardBitmaps, sx+one, sy + 1,              SRCCOPY);
    BitBlt(hdc, x+one, y+__cardheight-2, width-one*tips, 1, __hdcCardBitmaps, sx+one, sy + __cardheight-2, SRCCOPY);

    //if(width <= 2) return;

    // draw the outer-most lips
    BitBlt(hdc, x+two, y,                width-two*tips, 1, __hdcCardBitmaps, sx+two, sy,                  SRCCOPY);
    BitBlt(hdc, x+two, y+__cardheight-1, width-two*tips, 1, __hdcCardBitmaps, sx+two, sy + __cardheight-1, SRCCOPY);
}

//
//    xdir -   <0 or >0
//    ydir -   <0 or >0
//
void DrawCardCorner(HDC hdc, int x, int y, int cardval, int xdir, int ydir)
{
    int sx = cardval * __cardwidth;
    int sy = 0;

    HDC hdcSource = __hdcCardBitmaps;

    if(xdir < 0) 
    {
        x  += __cardwidth + xdir - 1;
        sx += __cardwidth + xdir - 1;
    }
    else
    {
        x  += xdir;
        sx += xdir;
    }

    if(ydir < 0) 
    {
        y  += __cardheight + ydir - 1;
        sy += __cardheight + ydir - 1;
    }
    else
    {
        y  += ydir;
        sy += ydir;
    }

    //convert x,y directions to -1, +1
    xdir = xdir < 0 ? -1 : 1;
    ydir = ydir < 0 ? -1 : 1;

    SetPixel(hdc, x+xdir, y ,     GetPixel(hdcSource, sx+xdir, sy));
    SetPixel(hdc, x,      y,      GetPixel(hdcSource, sx,      sy));
    SetPixel(hdc, x,      y+ydir, GetPixel(hdcSource, sx,      sy+ydir));

}

//
//    Draw a card (i.e. miss out the corners)
//
void DrawCard(HDC hdc, int x, int y, HDC hdcDragCard, int width, int height)
{
    //draw main center band
    BitBlt(hdc, x+2, y, width - 4, height, hdcDragCard, 2, 0, SRCCOPY);

    //draw the two bits to the left
    BitBlt(hdc, x,   y+2, 1, height - 4, hdcDragCard, 0, 2, SRCCOPY);
    BitBlt(hdc, x+1, y+1, 1, height - 2, hdcDragCard, 1, 1, SRCCOPY);

    //draw the two bits to the right
    BitBlt(hdc, x+width-2, y+1, 1, height - 2, hdcDragCard, width-2, 1, SRCCOPY);
    BitBlt(hdc, x+width-1, y+2, 1, height - 4, hdcDragCard, width-1, 2, SRCCOPY);
}

//
//    Clip a card SHAPE - basically any rectangle
//  with rounded corners
//
int ClipCard(HDC hdc, int x, int y, int width, int height)
{
    ExcludeClipRect(hdc, x+2,        y,     x+2+width-4, y+  height);
    ExcludeClipRect(hdc, x,            y+2, x+1,          y+2+height-4);
    ExcludeClipRect(hdc, x+1,        y+1, x+2,          y+1+height-2);
    ExcludeClipRect(hdc, x+width-2, y+1, x+width-2+1, y+1+height-2);
    ExcludeClipRect(hdc, x+width-1, y+2, x+width-1+1, y+2+height-4);
    return 0;
}

void CardRegion::Clip(HDC hdc)
{
    int numtoclip;

    if(fVisible == false) 
        return;

    Update();                //Update this stack's size+card count
    numtoclip = nNumApparentCards;

    //if we are making this stack flash on/off, then only 
    //clip the stack for drawing if the flash is in its ON state
    if(nFlashCount != 0)
    {
        if(fFlashVisible == FALSE)
            numtoclip = 0;
    }

    //if offset along a diagonal
    if(xoffset != 0 && yoffset != 0 && cardstack.NumCards() != 0)
    {
        for(int j = 0; j < numtoclip; j ++)
        {    
            ClipCard(hdc, xpos + xoffset * j, ypos + yoffset * j, __cardwidth, __cardheight);
        }    
    }
    //otherwise if just offset along a horizontal/vertical axis
    else
    {
        if(yoffset < 0 && numtoclip > 0)
        {
            ClipCard(hdc, xpos, ypos-((numtoclip-1)*-yoffset), width, height);
        }
        else if(xoffset < 0 && numtoclip > 0)
        {
            ClipCard(hdc, xpos-((numtoclip-1)*-xoffset), ypos, width, height);
        }
        else
        {
            ClipCard(hdc, xpos, ypos, width, height);
        }
    }

}

void CardRegion::Render(HDC hdc)
{
    int cardnum = 0;
    int numtodraw;
    BOOL fDrawTips;
    
    Update();            //Update this stack's card count + size

    numtodraw = nNumApparentCards;

    if(nFlashCount != 0)
    {
        if(fFlashVisible == false)
            numtodraw = 0;
    }

    if(fVisible == 0) return;
    
    cardnum = cardstack.NumCards() - numtodraw;
    int counter;

    for(counter = 0; counter < numtodraw; counter++)
    {
        int cardval;
        
        int x = xoffset * counter + xpos;
        int y = yoffset * counter + ypos;

        //if about to draw last card, then actually draw the top card
        if(counter == numtodraw - 1) cardnum = cardstack.NumCards() - 1;
        
        Card card = cardstack.cardlist[cardnum];
        cardval = card.Idx();
        
        if(card.FaceDown())
            cardval = nBackCardIdx;    //card-back
            
        //only draw the visible part of the card
        if(counter < numtodraw - 1)
        {
            if(yoffset != 0 && xoffset != 0)
                fDrawTips = FALSE;
            else
                fDrawTips = TRUE;

            if(yoffset != 0 && abs(xoffset) == 1 ||    xoffset != 0 && abs(yoffset) == 1)
                fDrawTips = TRUE;

            //draw horizontal strips
            if(yoffset > 0) 
            {
                DrawHorzCardStrip(hdc, x, y, cardval, yoffset, fDrawTips);
            }
            else if(yoffset < 0)
            {
                DrawHorzCardStrip(hdc, x, y+__cardheight+yoffset, cardval, yoffset, fDrawTips);
            }

            //draw some vertical bars
            if(xoffset > 0)
            {
                DrawVertCardStrip(hdc, x, y, cardval, xoffset, fDrawTips);
            }
            else if(xoffset < 0)
            {
                DrawVertCardStrip(hdc, x+__cardwidth+xoffset, y, cardval, xoffset, fDrawTips);
            }

            if(yoffset != 0 && xoffset != 0)//fDrawTips == FALSE)
            {
                //if we didn't draw any tips, then this is a 2-dim stack
                //(i.e, it goes at a diagonal).
                //in this case, we need to fill in the small triangle in
                //each corner!
                DrawCardCorner(hdc, x, y, cardval, xoffset, yoffset);
            }
        }
        //if the top card, draw the whole thing
        else
        {
            CardBlt(hdc, x, y, cardval);
        }

        cardnum ++;

    } //end of index
    
    if(counter == 0)    //if the cardstack is empty, then draw it that way
    {
        int x = xpos;
        int y = ypos;
        
        switch(uEmptyImage)
        {
        default: case CS_EI_NONE:
            //this wipes the RECT variable, so watch out!
            //SetRect(&rect, x, y, x+__cardwidth, y+__cardheight);
            //PaintRect(hdc, &rect, MAKE_PALETTERGB(crBackgnd));
            parentWnd.PaintCardRgn(hdc, x, y, __cardwidth, __cardheight, x, y);
            break;
            
        case CS_EI_SUNK:    //case CS_EI_CIRC: case CS_EI_X:
            DrawCard(hdc, x, y, __hdcPlaceHolder, __cardwidth, __cardheight);
            break;
        }
        
    }

    return;
}

int calc_offset(int offset, int numcards, int numtodrag, int realvisible)
{
    if(offset >= 0)
        return -offset * numcards;
    else
        return -offset * (numtodrag)       + 
               -offset * (realvisible - 1);
}

void CardRegion::PrepareDragBitmaps(int numtodrag)
{
    RECT rect;
    HDC hdc;
    int icard;
    int numcards = cardstack.NumCards();
    int xoff, yoff;

    if(nThreedCount > 1)
    {
        PrepareDragBitmapsThreed(numtodrag);
        return;
    }

    //work out how big the bitmaps need to be
    nDragCardWidth  = (numtodrag - 1) * abs(xoffset) + __cardwidth;
    nDragCardHeight = (numtodrag - 1) * abs(yoffset) + __cardheight;

    //Create bitmap for the back-buffer
    hdc = GetDC(NULL);
    hdcBackGnd = CreateCompatibleDC(hdc);
    hbmBackGnd = CreateCompatibleBitmap(hdc, nDragCardWidth, nDragCardHeight);
    SelectObject(hdcBackGnd, hbmBackGnd);

    //Create bitmap for the drag-image
    hdcDragCard = CreateCompatibleDC(hdc);
    hbmDragCard = CreateCompatibleBitmap(hdc, nDragCardWidth, nDragCardHeight);
    SelectObject(hdcDragCard, hbmDragCard);
    ReleaseDC(NULL, hdc);

    UseNicePalette(hdcBackGnd,  __hPalette);
    UseNicePalette(hdcDragCard, __hPalette);

    int realvisible = numcards / nThreedCount;

    //if(numcards > 0 && realvisible == 0) realvisible = 1;
    int iwhichcard = numcards - 1;
    if(nThreedCount == 1) iwhichcard = 0;

    //grab the first bit of background so we can prep the back buffer; do this by
    //rendering the card stack (minus the card we are dragging) to the temporary
    //background buffer, so it appears if we have lifted the card from the stack
    //PaintRect(hdcBackGnd, &rect, crBackgnd);
    SetRect(&rect, 0, 0, nDragCardWidth, nDragCardHeight);
 
    xoff = calc_offset(xoffset, numcards, numtodrag, realvisible);
    yoff = calc_offset(yoffset, numcards, numtodrag, realvisible);
    
    parentWnd.PaintCardRgn(hdcBackGnd, 0, 0, nDragCardWidth, nDragCardHeight, xpos - xoff,    ypos - yoff);

    //
    //    Render the cardstack into the back-buffer. The stack
    //    has already had the dragcards removed, so just draw
    //    what is left
    //
    for(icard = 0; icard < realvisible; icard++)
    {
        Card card = cardstack.cardlist[iwhichcard];
        int nCardVal;
        
        nCardVal = card.FaceUp() ? card.Idx() : nBackCardIdx;

        xoff = xoffset * icard + calc_offset(xoffset, numcards, numtodrag, realvisible);//- xoffset * ((numcards+numtodrag) / nThreedCount - numtodrag);
        yoff = yoffset * icard + calc_offset(yoffset, numcards, numtodrag, realvisible);//- yoffset * ((numcards+numtodrag) / nThreedCount - numtodrag);

        CardBlt(hdcBackGnd, xoff, yoff, nCardVal);
        iwhichcard++;
    }
    
    //
    // If there are no cards under this one, just draw the place holder
    //
    if(numcards == 0)   
    {
        int xoff = 0, yoff = 0;

        if(xoffset < 0)    xoff = nDragCardWidth  -  __cardwidth;
        if(yoffset < 0)    yoff = nDragCardHeight -  __cardheight;

        switch(uEmptyImage)
        {
        case CS_EI_NONE:
            //No need to draw anything: We already cleared the
            //back-buffer before the main loop..

            //SetRect(&rc, xoff, yoff, xoff+ __cardwidth, yoff + __cardheight);
            //PaintRect(hdcBackGnd, &rc, MAKE_PALETTERGB(crBackgnd));
            //parentWnd.PaintCardRgn(hdcBackGnd, xoff, yoff, __cardwidth, __cardheight, xpos, ypos);// + xoff, ypos + yoff);
            break;

        case CS_EI_SUNK:
            DrawCard(hdcBackGnd, xoff, yoff, __hdcPlaceHolder, __cardwidth, __cardheight);
            break;
        }
    }

    //
    //    now render the drag-cards into the dragcard image
    //
    PaintRect(hdcDragCard, &rect, crBackgnd);

    for(icard = 0; icard < numtodrag; icard++)
    {
        int nCardVal;

        if(xoffset >= 0) xoff =  xoffset * icard;
        else              xoff = -xoffset * (numtodrag - icard - 1);
            
        if(yoffset >= 0) yoff =  yoffset * icard;
        else             yoff = -yoffset * (numtodrag - icard - 1);

        Card card = dragstack.cardlist[icard];
        
        nCardVal = card.FaceUp() ? card.Idx() : nBackCardIdx;

        CardBlt(hdcDragCard, xoff, yoff, nCardVal);
    }
}

void CardRegion::PrepareDragBitmapsThreed(int numtodrag)
{
    RECT rect;
    HDC hdc;
    int icard;
    int numunder = 0;
    int iwhichcard;

    int numcards = cardstack.NumCards();

    //work out how big the bitmaps need to be
    nDragCardWidth  = (numtodrag - 1) * abs(xoffset) + __cardwidth;
    nDragCardHeight = (numtodrag - 1) * abs(yoffset) + __cardheight;

    //Create bitmap for the back-buffer
    hdc = GetDC(NULL);
    hdcBackGnd = CreateCompatibleDC(hdc);
    hbmBackGnd = CreateCompatibleBitmap(hdc, nDragCardWidth, nDragCardHeight);
    SelectObject(hdcBackGnd, hbmBackGnd);

    //create bitmap for the drag-image
    hdcDragCard = CreateCompatibleDC(hdc);
    hbmDragCard = CreateCompatibleBitmap(hdc, nDragCardWidth, nDragCardHeight);
    SelectObject(hdcDragCard, hbmDragCard);
    ReleaseDC(NULL, hdc);

    UseNicePalette(hdcBackGnd,  __hPalette);
    UseNicePalette(hdcDragCard, __hPalette);

    //grab the first bit of background so we can prep the back buffer; do this by
    //rendering the card stack (minus the card we are dragging) to the temporary
    //background buffer, so it appears if we have lifted the card from the stack
    //--SetRect(&rect, 0, 0, nDragCardWidth, nDragCardHeight);
    //--PaintRect(hdcBackGnd, &rect, crBackgnd);

    int threedadjust = numcards  % nThreedCount == 0;
    
    numunder = CalcApparentCards(numcards);
    iwhichcard = (numcards+numtodrag) - numunder - 1;
    if(nThreedCount == 1) iwhichcard = 0;
    
    int xoff = calc_offset(xoffset, numunder, numtodrag, numunder);
    int yoff = calc_offset(yoffset, numunder, numtodrag, numunder);

    parentWnd.PaintCardRgn(hdcBackGnd, 0,0,    nDragCardWidth,nDragCardHeight,    xpos - xoff,ypos - yoff);

    //
    //    Render the cardstack into the back-buffer. The stack
    //    has already had the dragcards removed, so just draw
    //    what is left
    //
    for(icard = 0; icard < numunder; icard++)
    {
        Card card = cardstack.cardlist[iwhichcard];
        int nCardVal = card.FaceUp() ? card.Idx() : nBackCardIdx;

        CardBlt(hdcBackGnd, 
                xoffset * icard - xoffset*(numunder-numtodrag+threedadjust),
                yoffset * icard - yoffset*(numunder-numtodrag+threedadjust),
                nCardVal);

        iwhichcard++;
    }
    
    //
    // If there are no cards under this one, just draw the place holder
    //
    if(numcards == 0)   
    {
        switch(uEmptyImage)
        {
        case CS_EI_NONE:
            //no need! we've already cleared the whole
            //back-buffer before the main loop!
            //SetRect(&rect, 0, 0, __cardwidth, __cardheight);
            //PaintRect(hdcBackGnd, &rect, MAKE_PALETTERGB(crBackgnd));
            break;

        case CS_EI_SUNK:
            DrawCard(hdcBackGnd, 0, 0, __hdcPlaceHolder, __cardwidth, __cardheight);
            break;
    
        }
    }

    //
    //    now render the drag-cards into the dragcard image
    //
    PaintRect(hdcDragCard, &rect, crBackgnd);
    
    for(icard = 0; icard < numtodrag; icard++)
    {
        Card card = dragstack.cardlist[icard];
        int nCardVal = card.FaceUp() ? card.Idx() : nBackCardIdx;

        CardBlt(hdcDragCard, xoffset * icard, yoffset * icard, nCardVal);
    }
}

void CardRegion::ReleaseDragBitmaps(void)
{
    //SelectObject(hdcBackGnd, hOld1);
    DeleteObject(hbmBackGnd);
    DeleteDC(hdcBackGnd);

    //SelectObject(hdcDragCard, hOld2);
    DeleteObject(hbmDragCard);
    DeleteDC(hdcDragCard);
}


void CardRegion::Redraw()
{
    HDC hdc = GetDC((HWND)parentWnd);

    Update();
    Render(hdc);

    ReleaseDC((HWND)parentWnd, hdc);
}
