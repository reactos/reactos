//
//    CardLib - CardRegion drawing support
//
//    Freeware
//    Copyright J Brown 2001
//

#include "cardlib.h"

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
void CardBlt(HDC hdc, int x, int y, int nCardNum)
{
    BltCardShape(hdc, x, y, __cardwidth, __cardheight, __hdcCardBitmaps,
                 nCardNum * __cardwidth, 0, CardCornerRadius(__cardwidth, __cardheight));
}

void DrawCard(HDC hdc, int x, int y, HDC source, int width, int height)
{
    BltCardShape(hdc, x, y, width, height, source, 0, 0,
                 CardCornerRadius(__cardwidth, __cardheight));
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

    // Exclude the union of the actual cards, not a rounded bounding box.
    // The gaps between exposed corners must remain part of the background.
    if (!numtoclip)
        numtoclip = 1;
    for (int j = 0; j < numtoclip; ++j)
        ExcludeCardShape(hdc, xpos + xoffset * j, ypos + yoffset * j,
                         __cardwidth, __cardheight, CardCornerRadius(__cardwidth, __cardheight));

}

void CardRegion::Render(HDC hdc)
{
    int cardnum = 0;
    int numtodraw;

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

        // Paint from bottom to top. The old strip optimisation assumed fixed
        // two-pixel corners and left holes when cards overlap rounded corners.
        CardBlt(hdc, x, y, cardval);

        cardnum ++;

    } //end of index

    if(counter == 0)    //if the cardstack is empty, then draw it that way
    {
        int x = xpos;
        int y = ypos;

        switch(uEmptyImage)
        {
        default:
        case CS_EI_NONE:
            //this wipes the RECT variable, so watch out!
            //SetRect(&rect, x, y, x+__cardwidth, y+__cardheight);
            //PaintRect(hdc, &rect, MAKE_PALETTERGB(crBackgnd));
            parentWnd.PaintCardRgn(hdc, x, y, __cardwidth, __cardheight, x, y);
            break;

        case CS_EI_SUNK:
            DrawCard(hdc, x, y, __hdcPlaceHolder, __cardwidth, __cardheight);
            break;

        case CS_EI_CIRC:
        case CS_EI_X:
            CardBlt(hdc, x, y, uEmptyImage);
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

        case CS_EI_CIRC:
        case CS_EI_X:
            CardBlt(hdc, xoff, yoff, uEmptyImage);
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

        case CS_EI_CIRC:
        case CS_EI_X:
            CardBlt(hdc, 0, 0, uEmptyImage);
            break;
        }
    }

    //
    //    now render the drag-cards into the dragcard image
    //
    SetRect(&rect, 0, 0, nDragCardWidth, nDragCardHeight);
    PaintRect(hdcDragCard, &rect, crBackgnd);

    for(icard = 0; icard < numtodrag; icard++)
    {
        Card card = dragstack.cardlist[icard];
        int nCardVal = card.FaceUp() ? card.Idx() : nBackCardIdx;

        int dx = xoffset >= 0 ? xoffset * icard : -xoffset * (numtodrag - icard - 1);
        int dy = yoffset >= 0 ? yoffset * icard : -yoffset * (numtodrag - icard - 1);
        CardBlt(hdcDragCard, dx, dy, nCardVal);
    }
}

void CardRegion::DrawDragShape(HDC dc, int x, int y, bool exclude)
{
    int count = dragstack.NumCards();
    for (int i = 0; i < count; ++i)
    {
        int dx = xoffset >= 0 ? xoffset * i : -xoffset * (count - i - 1);
        int dy = yoffset >= 0 ? yoffset * i : -yoffset * (count - i - 1);
        int radius = CardCornerRadius(__cardwidth, __cardheight);
        if (exclude)
            ExcludeCardShape(dc, x + dx, y + dy, __cardwidth, __cardheight, radius);
        else
            BltCardShape(dc, x + dx, y + dy, __cardwidth, __cardheight,
                         hdcDragCard, dx, dy, radius);
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
