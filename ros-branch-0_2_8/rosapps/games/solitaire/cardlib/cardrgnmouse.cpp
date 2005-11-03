//
//    CardLib - CardRegion mouse-related stuff
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include <math.h>
#include <stdio.h>

#include "cardlib.h"
#include "cardwindow.h"
#include "cardregion.h"

#if 1
#define TRACE(s)
#else
#define TRACE(s) printf("%s(%i): %s",__FILE__,__LINE__,s)
#endif

double __CARDZOOMSPEED = 32;

int ClipCard(HDC hdc, int x, int y, int width, int height);
void DrawCard(HDC hdc, int x, int y, HDC hdcSource, int width, int height);

#ifdef _DEBUG

static pDebugClickProc DebugStackClickProc = 0;

void CardLib_SetStackClickProc(pDebugClickProc proc)
{
    DebugStackClickProc = proc;
}

#endif

CardRegion *CardWindow::GetBestStack(int x, int y, int w, int h)
{
    int maxoverlap    =  0;
    int maxoverlapidx = -1;

    //find the stack which is most covered by the dropped
    //cards. Only include those which allow drops.
    //
    for(int i = 0; i < nNumCardRegions; i++)
    {
        int percent = Regions[i]->GetOverlapRatio(x, y, w, h);

        //if this stack has the biggest coverage yet
        if(percent > maxoverlap && Regions[i]->IsVisible())
        {
            maxoverlap = percent;
            maxoverlapidx = i;
        }
    }
    
    //if we found a stack to drop onto
    if(maxoverlapidx != -1)
    {
        return Regions[maxoverlapidx];
    }
    else
    {
        return 0;
    }
}

bool CardRegion::IsPointInStack(int x, int y)
{
    int axpos = xoffset < 0 ? xpos + (nNumApparentCards-1)*xoffset : xpos;
    int aypos = yoffset < 0 ? ypos + (nNumApparentCards-1)*yoffset : ypos;
    
    if(x >= axpos && x < axpos + width && y >= aypos && y < aypos + height && fVisible)
        return true;
    else
        return false;
}

int CardRegion::GetNumDragCards(int x, int y)
{
    int cardindex = 0;        //index from stack start
    int maxidx;

    //make x,y relative to the stack's upper left corner
    x -= xpos + (xoffset < 0 ? (nNumApparentCards/*cardstack.NumCards()*/ - 1) * xoffset : 0);
    y -= ypos + (yoffset < 0 ? (nNumApparentCards/*cardstack.NumCards()*/ - 1) * yoffset : 0);
    
    //if stack is empty, cannot drag any cards from it
    if(cardstack.NumCards() <= 0)
        return 0;

    //see which card in the stack has been clicked on
    //top-bottom ordering
    if(yoffset > 0)
    {
        if(y < height - __cardheight)
            cardindex = y / yoffset;
        else
            cardindex = cardstack.NumCards() - 1;
    }
    else if(yoffset < 0)
    {
        if(y < __cardheight)
            cardindex = cardstack.NumCards() - 1;
        else
            cardindex = cardstack.NumCards() - ((y - __cardheight) / -yoffset) - 2;
    }
    else    //yoffset == 0
    {
        cardindex = cardstack.NumCards() - 1;
    }

    maxidx = cardindex;

    //if left-right
    if(xoffset > 0)
    {
        if(x < width - __cardwidth)
            cardindex = x / xoffset;
        else
            cardindex = cardstack.NumCards() - 1;
    }
    else if(xoffset < 0)
    {
        if(x < __cardwidth)
            cardindex = cardstack.NumCards() - 1;
        else
            cardindex = cardstack.NumCards() - ((x - __cardwidth) / -xoffset) - 2;
    }
    else
    {
        cardindex = cardstack.NumCards() - 1;
    }

    if(cardindex > maxidx) cardindex = maxidx;

    if(cardindex > cardstack.NumCards())
        cardindex = 1;

    //if are trying to drag too many cards at once
    return cardstack.NumCards() - cardindex;
}

bool CardRegion::CanDragCards(int iNumCards)
{
    if(iNumCards <= 0) return false;
    if(nThreedCount > 1 && iNumCards > 1) return false;

    if(WaitForSingleObject(mxlock, 0) != WAIT_OBJECT_0)
    {
//        TRACE("Failed to gain access to card stack\n");
        return false;
    }

    ReleaseMutex(mxlock);

    switch(uDragRule)
    {
    case CS_DRAG_ALL:
        return true;
        
    case CS_DRAG_TOP:

        if(iNumCards == 1)
            return true;
        else
            return false;
        
    case CS_DRAG_NONE:
        return false;
        
    case CS_DRAG_CALLBACK:
        
        if(CanDragCallback)
        {
            return CanDragCallback(*this, iNumCards);
        }
        else
        {
            return false;
        }
        
    default:
        return false;
    }
}

bool CardRegion::CanDropCards(CardStack &cards)
{
    if(WaitForSingleObject(mxlock, 0) != WAIT_OBJECT_0)
    {
        return false;
    }

    ReleaseMutex(mxlock);

    switch(uDropRule)
    {
    case CS_DROP_ALL:
        return true;

    case CS_DROP_NONE:
        return false;

    case CS_DROP_CALLBACK:
        
        if(CanDropCallback)
        {
            return CanDropCallback(*this, cards);
        }
        else
        {
            return false;
        }

    default:
        return false;
    }
}

bool CardRegion::OnLButtonDblClk(int x, int y)
{
    iNumDragCards = GetNumDragCards(x, y); 

    if(DblClickCallback)
        DblClickCallback(*this, iNumDragCards);

    return true;
}

bool CardRegion::OnLButtonDown(int x, int y)
{
    iNumDragCards = GetNumDragCards(x, y); 

#ifdef _DEBUG
    if(DebugStackClickProc)
    {
        if(!DebugStackClickProc(*this))
            return false;
    }
#endif

    if(ClickCallback)
        ClickCallback(*this, iNumDragCards);

    if(CanDragCards(iNumDragCards) != false)
    {

        //offset of the mouse cursor relative to the top-left corner
        //of the cards that are being dragged
        mousexoffset = x - xpos - xoffset * (nNumApparentCards - iNumDragCards);
        mouseyoffset = y - ypos - yoffset * (nNumApparentCards - iNumDragCards);
        
        if(xoffset < 0)
            mousexoffset += -xoffset * (iNumDragCards - 1);

        if(yoffset < 0)
            mouseyoffset += -yoffset * (iNumDragCards - 1);
        
        //remove the cards from the source stack
        dragstack = cardstack.Pop(iNumDragCards);

        //prepare the back buffer, and the drag image
        PrepareDragBitmaps(iNumDragCards);

        oldx = x - mousexoffset;
        oldy = y - mouseyoffset;
        
        Update();            //Update this stack's card count + size

        SetCapture((HWND)parentWnd);

        //set AFTER settings the dragstack...
        fMouseDragging = true;

        return true;
    }

    return false;
}

bool CardRegion::OnLButtonUp(int x, int y)
{
    CardRegion *pDestStack = 0;
    HDC hdc;
    int dropstackid = CS_DROPZONE_NODROP;
    
    RECT dragrect;
    DropZone *dropzone;

    fMouseDragging = false;

    //first of all, see if any drop zones have been registered
    SetRect(&dragrect, x-mousexoffset, y-mouseyoffset, x-mousexoffset+nDragCardWidth, y-mouseyoffset+nDragCardHeight);

    dropzone = parentWnd.GetDropZoneFromRect(&dragrect);

    if(dropzone)
    {
        dropstackid = dropzone->DropCards(dragstack);
        
        if(dropstackid != CS_DROPZONE_NODROP)
            pDestStack = parentWnd.CardRegionFromId(dropstackid);
        else
            pDestStack = 0;
    }
    else
    {
        pDestStack = parentWnd.GetBestStack(x - mousexoffset, y - mouseyoffset, nDragCardWidth, nDragCardHeight);
    }
    
    // If have found a stack to drop onto
    //
    TRACE ( "can I drop card?\n" );
    if(pDestStack && pDestStack->CanDropCards(dragstack)) 
    {
        TRACE ( "yes, dropping card\n" );
        hdc = GetDC((HWND)parentWnd);
        //            UseNicePalette(hdc);
        ZoomCard(hdc, x - mousexoffset, y  - mouseyoffset, pDestStack);
        ReleaseDC((HWND)parentWnd, hdc);
        
        //
        //add the cards to the destination stack
        //
        CardStack temp = pDestStack->GetCardStack();
        temp.Push(dragstack);
        
        pDestStack->SetCardStack(temp);
//        pDestStack->Update();        //Update this stack's card count + size
//        pDestStack->UpdateFaceDir(temp);
        
        //    Call the remove callback on THIS stack, if one is specified
        //
        if(RemoveCallback)
            RemoveCallback(*this, iNumDragCards);

        //    Call the add callback, if one is specified
        //
        if(pDestStack->AddCallback)
            pDestStack->AddCallback(*pDestStack, pDestStack->cardstack);//index, deststack->numcards);
        
        RedrawIfNotDim(pDestStack, true);
        TRACE ( "done dropping card\n" );
    }

    //
    //    Otherwise, let the cards snap back onto this stack
    //
    else
    {
        TRACE ( "no, putting card back\n" );
        hdc = GetDC((HWND)parentWnd);
        TRACE ( "calling ZoomCard()\n" );
        ZoomCard(hdc, x - mousexoffset, y - mouseyoffset, this);
        TRACE ( "cardstack += dragstack\n" );
        cardstack += dragstack;
        TRACE ( "calling ReleaseDC()\n" );
        ReleaseDC((HWND)parentWnd, hdc);

        TRACE ( "calling Update()\n" );
        Update();        //Update this stack's card count + size
        TRACE ( "done putting card back\n" );
    }
    
    ReleaseDragBitmaps();
    ReleaseCapture();
    
    TRACE ( "OnLButtonUp() done\n" );
    return true;
}

bool CardRegion::OnMouseMove(int x, int y)
{
    HDC hdc;

    hdc = GetDC((HWND)parentWnd);
        
    x -= mousexoffset;
    y -= mouseyoffset;
        
    MoveDragCardTo(hdc, x, y);

    //BitBlt(hdc, nDragCardWidth+10, 0, nDragCardWidth, nDragCardHeight, hdcBackGnd, 0, 0, SRCCOPY);
    //BitBlt(hdc, 0, 0, nDragCardWidth, nDragCardHeight, hdcDragCard, 0, 0, SRCCOPY);
    
    ReleaseDC((HWND)parentWnd, hdc);
        
    oldx = x;
    oldy = y;
    
    return true;
}

//
//    There is a bug in BitBlt when the source x,y
//    become < 0. So this wrapper function simply adjusts
//    the coords so that we never try to blt in from this range
//
BOOL ClippedBitBlt(HDC hdcDest, int x, int y, int width, int height, HDC hdcSrc, int srcx, int srcy, DWORD dwROP)
{
    if(srcx < 0)
    {
        x = 0 - srcx;
        width = width + srcx;
        srcx = 0;
    }

    if(srcy < 0)
    {
        y = 0 - srcy;
        height = height + srcy;
        srcy = 0;
    }

    return BitBlt(hdcDest, x, y, width, height, hdcSrc, srcx, srcy, dwROP);
}

void CardRegion::MoveDragCardTo(HDC hdc, int x, int y)
{
    RECT inter, rect1, rect2;

    //mask off the new position of the drag-card, so
    //that it will not be painted over
    ClipCard(hdc, x, y, nDragCardWidth, nDragCardHeight);
    
    //restore the area covered by the card at its previous position
    BitBlt(hdc, oldx, oldy, nDragCardWidth, nDragCardHeight, hdcBackGnd, 0, 0, SRCCOPY);

    //remove clipping so we can draw the card at its new place
    SelectClipRgn(hdc, NULL);
    
    //if the card's old and new positions overlap, then we
    //need some funky code to update the "saved background" image,
    SetRect(&rect1, oldx, oldy, oldx+nDragCardWidth, oldy+nDragCardHeight);
    SetRect(&rect2,    x,    y,    x+nDragCardWidth,    y+nDragCardHeight);
    
    if(IntersectRect(&inter, &rect1, &rect2))
    {
        int interwidth = inter.right-inter.left;
        int interheight = inter.bottom-inter.top;
        int destx, desty, srcx, srcy;
        
        if(rect2.left > rect1.left) 
        {    
            destx = 0; srcx = nDragCardWidth - interwidth; 
        }
        else
        {
            destx = nDragCardWidth  - interwidth; srcx = 0;
        }
        
        if(rect2.top  > rect1.top) 
        {
            desty = 0; srcy = nDragCardHeight - interheight;
        }
        else 
        {
            desty = nDragCardHeight - interheight; srcy = 0;
        }
        
        //shift the bit we didn't use for the restore (due to the clipping)
        //into the opposite corner
        BitBlt(hdcBackGnd, destx,desty, interwidth, interheight, hdcBackGnd, srcx, srcy, SRCCOPY);
        
        ExcludeClipRect(hdcBackGnd, destx, desty, destx+interwidth, desty+interheight);
        
        //this bit requires us to clip the BitBlt (from screen to background)
        //as BitBlt is a bit buggy it seems
        ClippedBitBlt(hdcBackGnd, 0,0, nDragCardWidth, nDragCardHeight, hdc, x, y, SRCCOPY);
        SelectClipRgn(hdcBackGnd, NULL);
    }
    else
    {
        BitBlt(hdcBackGnd, 0,0, nDragCardWidth, nDragCardHeight, hdc, x, y, SRCCOPY);
    }
    
    //finally draw the card to the screen
    DrawCard(hdc, x, y, hdcDragCard, nDragCardWidth, nDragCardHeight);
}


//extern "C" int _fltused(void) { return 0; }
//extern "C" int _ftol(void) { return 0; }

//
//    Better do this in fixed-point, to stop
//    VC from linking in floatingpoint-long conversions
//
//#define FIXED_PREC_MOVE
#ifdef  FIXED_PREC_MOVE
#define PRECISION 12
void ZoomCard(HDC hdc, int xpos, int ypos, CARDSTACK *dest)
{
    long dx, dy, x , y;

    
    int apparentcards;
    x = xpos << PRECISION; y = ypos << PRECISION;

    oldx = (int)xpos;
    oldy = (int)ypos;

    apparentcards=dest->numcards/dest->threedcount;

    int idestx = dest->xpos + dest->xoffset * (apparentcards);// - iNumDragCards); 
    int idesty = dest->ypos + dest->yoffset * (apparentcards);// - iNumDragCards);

    //normalise the motion vector
    dx = (idestx<<PRECISION) - x;
    dy = (idesty<<PRECISION) - y;
    long recip = (1 << PRECISION) / 1;//sqrt(dx*dx + dy*dy);

    dx *= recip * 16;//CARDZOOMSPEED; 
    dy *= recip * 16;//CARDZOOMSPEED;

    //if(dx < 0) dxinc = 1.001; else

    for(;;)
    {
        int ix, iy;
        x += dx;
        y += dy;

        ix = (int)x>>PRECISION;
        iy = (int)y>>PRECISION;
        if(dx < 0 && ix < idestx) ix = idestx;
        else if(dx > 0 && ix > idestx) ix = idestx;

        if(dy < 0 && iy < idesty) iy = idesty;
        else if(dy > 0 && iy > idesty) iy = idesty;

        MoveDragCardTo(hdc, ix, iy);

        if(ix == idestx && iy == idesty)
            break;

        oldx = (int)x >> PRECISION;
        oldy = (int)y >> PRECISION;

        //dx *= 1.2;
        //dy *= 1.2;

        Sleep(10);
    }
}
#else
void CardRegion::ZoomCard(HDC hdc, int xpos, int ypos, CardRegion *pDestStack)
{
    TRACE ( "ENTER ZoomCard()\n" );
    double dx, dy, x ,y;
    int apparentcards;
    x = (double)xpos; y = (double)ypos;

    oldx = (int)x;
    oldy = (int)y;

    apparentcards = pDestStack->cardstack.NumCards() / pDestStack->nThreedCount;

    int idestx = pDestStack->xpos + pDestStack->xoffset * (apparentcards);
    int idesty = pDestStack->ypos + pDestStack->yoffset * (apparentcards);

    if(pDestStack->yoffset < 0)
        idesty += pDestStack->yoffset * (iNumDragCards-1);

    if(pDestStack->xoffset < 0)
        idestx += pDestStack->xoffset * (iNumDragCards-1);

    //normalise the motion vector
    dx = idestx - x;
    dy = idesty - y;
    if ( fabs(dx) + fabs(dy) < 0.001f )
    {
        MoveDragCardTo(hdc, idestx, idesty);
        return;
    }
    double recip = 1.0 / sqrt(dx*dx + dy*dy);
    dx *= recip * __CARDZOOMSPEED; dy *= recip * __CARDZOOMSPEED;

    //if(dx < 0) dxinc = 1.001; else

    for(;;)
    {
        bool attarget = true;
        int ix, iy;
        x += dx;
        y += dy;

        ix = (int)x;
        iy = (int)y;

        if(dx < 0.0 && ix < idestx) ix = idestx;
        else if(dx > 0.0 && ix > idestx) ix = idestx;
        else attarget = false;

        if(dy < 0.0 && iy < idesty) iy = idesty;
        else if(dy > 0.0 && iy > idesty) iy = idesty;
        else attarget = false;

        //if the target stack wants the drag cards drawn differently
        //to how they are, then redraw the drag card image just before
        //the cards land
        /*if(attarget == true)
        {
            for(int i = 0; i < iNumDragCards; i++)
            {
                int xdraw = pDestStack->xoffset*i;
                int ydraw = pDestStack->yoffset*i;

                if(pDestStack->yoffset < 0)
                    ydraw = -pDestStack->yoffset * (iNumDragCards-i-1);
                if(pDestStack->xoffset < 0)
                    xdraw = -pDestStack->xoffset * (iNumDragCards-i-1);

                if(pDestStack->facedirection == CS_FACEUP && 
                    pDestStack->numcards+i >= dest->numfacedown)
                {
                    //cdtDraw(hdcDragCard, xdraw, ydraw, iDragCards[i], ectFACES, 0);
                }
                else
                {
                    //cdtDraw(hdcDragCard, xdraw, ydraw, CARDSTACK::backcard, ectBACKS, 0);
                }
            }
        }*/

        MoveDragCardTo(hdc, ix, iy);

        if(attarget || ix == idestx && iy == idesty)
            break;

        oldx = (int)x;
        oldy = (int)y;

        //dx *= 1.2;
        //dy *= 1.2;

        Sleep(10);
    }
    TRACE ( "EXIT ZoomCard()\n" );
}
#endif
