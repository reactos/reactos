//
//    CardLib - CardRegion class
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>

#include "cardlib.h"
#include "cardregion.h"
#include "cardwindow.h"
#include "cardcolor.h"

HBITMAP CreateSinkBmp(HDC hdcCompat, HDC hdc, int width, int height);

void PaintRect(HDC hdc, RECT *rect, COLORREF colour);

CardRegion::CardRegion(CardWindow &parent, int Id, bool visible, int x, int y, int xOffset, int yOffset) 
: id(Id), parentWnd(parent), xpos(x), ypos(y), xoffset(xOffset), yoffset(yOffset), fVisible(visible)
{
    width  = __cardwidth;
    height = __cardheight;

    crBackgnd  = RGB(0, 64, 100);

    uFaceDirType   = CS_FACE_UP;
    nFaceDirOption = 0;
    uEmptyImage  = CS_EI_SUNK;

    fVisible     = visible;

    nThreedCount = 1;
    nBackCardIdx = 53;

    Update();                //Update this stack's size+card count

    hdcBackGnd = 0;
    hbmBackGnd = 0;
    hdcDragCard = 0;
    hbmDragCard = 0;

    nDragCardWidth = 0;
    nDragCardHeight = 0;
    
    CanDragCallback  = 0;
    CanDropCallback  = 0;
    AddCallback      = 0;
    RemoveCallback   = 0;
    ClickCallback    = 0;
    DblClickCallback = 0;

    uDragRule = CS_DRAG_ALL;
    uDropRule = CS_DROP_ALL;

    xjustify = yjustify = xadjust = yadjust = 0;

    nFlashCount        = 0;
    fFlashVisible    = false;
    uFlashTimer        = (UINT)-1;

    fMouseDragging = false;

    mxlock = CreateMutex(0, FALSE, 0);
}

CardRegion::~CardRegion()
{
    CloseHandle(mxlock);
}

void CardRegion::SetBackColor(COLORREF cr)
{
    crBackgnd = cr;
}

int CardRegion::CalcApparentCards(int realnum)
{
    return ((realnum + nThreedCount - 1) - (realnum + nThreedCount - 1) % nThreedCount) / nThreedCount;
}

void CardRegion::CalcApparentCards()
{
    nNumApparentCards = CalcApparentCards(cardstack.NumCards());
}


void CardRegion::UpdateSize(void)
{
    if(cardstack.NumCards() > 0)
    {
        if(xoffset > 0)
            width  = (nNumApparentCards - 1) * xoffset + __cardwidth;
        else
            width  = (nNumApparentCards - 1) * -xoffset + __cardwidth;

        if(yoffset > 0)
            height = (nNumApparentCards - 1) * yoffset + __cardheight;
        else
            height = (nNumApparentCards - 1) * -yoffset + __cardheight;
    }
    else
    {
        width = __cardwidth;
        height = __cardheight;
    }
}

CardRegion *CardWindow::CreateRegion(int id, bool fVisible, int x, int y, int xoffset, int yoffset)
{
    CardRegion *cr;

    if(nNumCardRegions == MAXCARDSTACKS)
        return FALSE;

    cr = new CardRegion(*this, id, fVisible, x, y, xoffset, yoffset);
    cr->SetBackColor(crBackgnd);
    cr->SetBackCardIdx(nBackCardIdx);

    Regions[nNumCardRegions++] = cr;
    
    return cr;
}

int CardRegion::GetOverlapRatio(int x, int y, int w, int h)
{
    RECT me, him;
    RECT inter;
    SetRect(&him, x, y, x+w, y+h);
    SetRect(&me,  xpos, ypos, xpos+width, ypos+height);

    //see if the specified rectangle overlaps us
    if(IntersectRect(&inter, &me, &him))
    {
        int wi = inter.right  - inter.left;
        int hi = inter.bottom - inter.top;

        int overlap = wi * hi;
        int total   = width * height;

        int percent = (overlap << 16) / total;
        return (percent * 100) >> 16;
    }
    //do not overlap
    else
    {
        return 0;
    }
}

bool CardRegion::SetDragRule(UINT uDragType, pCanDragProc proc)
{ 
    switch(uDragType)
    {
    case CS_DRAG_NONE: case CS_DRAG_ALL: case CS_DRAG_TOP:
        uDragRule = uDragType;
        return true;

    case CS_DRAG_CALLBACK:
        uDragRule = uDragType;
        CanDragCallback = proc;
        return true;

    default:
        return false;
    }
}

bool CardRegion::SetDropRule(UINT uDropType, pCanDropProc proc)
{ 
    switch(uDropType)
    {
    case CS_DROP_NONE: case CS_DROP_ALL: 
        uDropRule = uDropType;
        return true;

    case CS_DROP_CALLBACK:
        uDropRule = uDropType;
        CanDropCallback = proc;
        return true;

    default:
        return false;
    }
}

void CardRegion::SetClickProc(pClickProc proc)
{
    ClickCallback = proc;
}

void CardRegion::SetDblClickProc(pClickProc proc)
{
    DblClickCallback = proc;
}

void CardRegion::SetAddCardProc(pAddProc proc)
{
    AddCallback = proc;
}

void CardRegion::SetRemoveCardProc(pRemoveProc proc)
{
    RemoveCallback = proc;
}

void CardRegion::Update()
{
    CalcApparentCards();
    UpdateSize(); 
    UpdateFaceDir(cardstack);
}


bool CardRegion::SetThreedCount(int count)
{
    if(count < 1) 
    {
        return false;
    }
    else
    {
        nThreedCount = count;
        return true;
    }
}

void CardRegion::SetOffsets(int x, int y)
{
    xoffset = x;
    yoffset = y;
}

void CardRegion::SetPos(int x, int y)
{
    xpos = x;
    ypos = y;
}

void CardRegion::Show(bool fShow)
{
    fVisible = fShow;
}

bool CardRegion::IsVisible()
{ 
    return fVisible;
}

void CardRegion::SetPlacement(UINT xJustify, UINT yJustify, int xAdjust, int yAdjust)
{
    xjustify = xJustify;
    yjustify = yJustify;
    xadjust  = xAdjust;
    yadjust  = yAdjust;
}

void CardRegion::SetFaceDirection(UINT uDirType, int nOption)
{
    switch(uDirType)
    {
    case CS_FACE_UP:     case CS_FACE_DOWN: case CS_FACE_DOWNUP:
    case CS_FACE_UPDOWN: case CS_FACE_ANY:
        uFaceDirType    = uDirType;
        nFaceDirOption  = nOption;

        UpdateFaceDir(cardstack);

        break;
    }
}

UINT CardRegion::GetFaceDirection(int *pnOption)
{
    if(pnOption)
        *pnOption = nFaceDirOption;

    return uFaceDirType;
}

void CardRegion::AdjustPosition(int winwidth, int winheight)
{
    Update();            //Update this stack's card count + size

    switch(xjustify)
    {
    default: case CS_XJUST_NONE: break;
    
    case CS_XJUST_CENTER:        //centered
        xpos = (winwidth - (width & ~0x1)) / 2;
        xpos += xadjust;

        if(xoffset < 0)    xpos += (width - __cardwidth);
    
        break;

    case CS_XJUST_RIGHT:        //right-aligned
        xpos = winwidth - __cardwidth;//width - 20;
        xpos += xadjust;
        break;
    }

    switch(yjustify)
    {
    default: case CS_YJUST_NONE: break;
    
    case CS_YJUST_CENTER:        //centered
        ypos = (winheight - height) / 2;
        ypos += yadjust;
        if(yoffset < 0)    ypos += (height - __cardheight);
        break;

    case CS_YJUST_BOTTOM:        //bottom-aligned
        ypos = winheight - __cardheight;//height - 20;
        ypos += yadjust;
        break;
    }

}


void CardRegion::Flash(int count, int milliseconds)
{
    if(count <= 0) return;

    nFlashCount        = count;
    fFlashVisible   = false;
    uFlashTimer        = SetTimer((HWND)parentWnd, (WPARAM)this, milliseconds, 0);
    
    parentWnd.Redraw();
}

void CardRegion::StopFlash()
{
    if(uFlashTimer != (UINT)-1)
    {
        KillTimer((HWND)parentWnd, uFlashTimer);
        nFlashCount        = 0;
        uFlashTimer        = (UINT)-1;
        fFlashVisible    = true;
    }
}

void CardRegion::DoFlash()
{
    if(uFlashTimer != (UINT)-1)
    {
        fFlashVisible = !fFlashVisible;

        if(--nFlashCount == 0)
        {
            KillTimer((HWND)parentWnd, uFlashTimer);
            uFlashTimer = (UINT)-1;
            fFlashVisible = true;
        }
    
        parentWnd.Redraw();
    }
}

int CardRegion::Id()
{
    return id;
}

void CardRegion::SetEmptyImage(UINT uImage)
{
    switch(uImage)
    {
    case CS_EI_NONE: case CS_EI_SUNK:
        uEmptyImage = uImage;
        break;

    default:
        uEmptyImage = CS_EI_NONE;
        break;
    }
    
}

void CardRegion::SetBackCardIdx(UINT uBackIdx)
{
    if(uBackIdx >= 52 && uBackIdx <= 68)
        nBackCardIdx = uBackIdx;
}

void CardRegion::SetCardStack(const CardStack &cs)
{ 
    //make a complete copy of the specified stack..
    cardstack = cs; 

    // Update the face-direction and stack-size
    Update();
}

const CardStack & CardRegion::GetCardStack()
{ 
    //return reference to our internal stack
    return cardstack; 
}

//
//    Update specified card-stack using THIS stack's
//  face direction rules!
//
void CardRegion::UpdateFaceDir(CardStack &cards)
{
    int i, n, num;

    num = cards.NumCards();

    //Now apply the face direction rules..
    switch(uFaceDirType)
    {
    case CS_FACE_UP:

        for(i = 0; i < num; i++)
        {
            cards[i].SetFaceUp(true);
        }

        break;

    case CS_FACE_DOWN:

        for(i = 0; i < num; i++)
        {
            cards[i].SetFaceUp(false);
        }

        break;

    case CS_FACE_DOWNUP:

        num = cardstack.NumCards();
        n = min(nFaceDirOption, num);

        //bottom n cards..
        for(i = 0; i < n; i++)
        {
            cards[num - i - 1].SetFaceUp(false);
        }

        for(i = n; i < num; i++)
        {
            cards[num - i - 1].SetFaceUp(true);
        }

        break;

    case CS_FACE_UPDOWN:

        num = cardstack.NumCards();
        n = min(nFaceDirOption, num);

        for(i = 0; i < n; i++)
        {
            cards[num - i - 1].SetFaceUp(true);
        }

        for(i = n; i < num; i++)
        {
            cards[num - i - 1].SetFaceUp(false);
        }

        break;

    case CS_FACE_ANY:    //cards can be any orientation
    default:
        break;
    }
}

bool CardRegion::MoveCard(CardRegion *pDestStack, int nNumCards, bool fAnimate)
{
    HDC hdc;

    int x, y;

    if(pDestStack == 0) return false; //{ forcedfacedir = -1 ;return 0; }

    if(nNumCards < 0 || nNumCards > cardstack.NumCards())
        return false;

    x = xpos + xoffset * (nNumApparentCards - nNumCards);
    y = ypos + yoffset * (nNumApparentCards - nNumCards);

    oldx = x;
    oldy = y;
    
    dragstack = cardstack.Pop(nNumCards);

    //Alter the drag-stack so that it's cards are the same way up
    //as the destination. Use the destination's drag-rules
    //instead of this ones!!
    CardStack temp;
    temp.Push(pDestStack->GetCardStack());
    temp.Push(dragstack);

    pDestStack->UpdateFaceDir(temp);

    dragstack = temp.Pop(nNumCards);

    if(fAnimate)
    {
        iNumDragCards = nNumCards;
        PrepareDragBitmaps(nNumCards);
    }

    Update();        //Update this stack's size+card count

    if(fAnimate)
    {
        hdc = GetDC((HWND)parentWnd);

        ZoomCard(hdc, x, y, pDestStack);
        
        ReleaseDC((HWND)parentWnd, hdc);
        ReleaseDragBitmaps();
    }

    // Get a copy of the cardstack
    CardStack cs = pDestStack->GetCardStack();
    cs.Push(dragstack);
    
    pDestStack->SetCardStack(cs);
    
    //cs = pDestStack->GetCardStack();
    //pDestStack->Update();
    //pDestStack->UpdateFaceDir(cs);

    RedrawIfNotDim(pDestStack, false);

    //forcedfacedir = -1;
    return true;
}

//
//    Simple wrappers
//
int CardRegion::NumCards() const
{
    if(fMouseDragging)
        return cardstack.NumCards() + dragstack.NumCards();
    else
        return cardstack.NumCards(); 
}

bool CardRegion::Lock()
{
    DWORD dw = WaitForSingleObject(mxlock, 0);

    if(dw == WAIT_OBJECT_0)
    {
        //TRACE("LockStack succeeded\n");
        return true; 
    }
    else
    {
        //TRACE("LockStack failed\n");
        return false;
    }
    return false;
}

bool CardRegion::UnLock()
{
    if(ReleaseMutex(mxlock))
    {
        //TRACE("Unlocking stack\n");
        return true;
    }
    else
    {
        //TRACE("Unlocking stack failed\n");    
        return false;
    }
}

bool CardRegion::PlayCard(CardRegion *pDestStack, int value, int num)
{
    //search the stack for the specified card value...
    while(num--)
    {
        for(int i = 0; i < cardstack.NumCards(); i++)
        {
            if(cardstack[i].HiVal() == value)
            {
                //swap the card with one at top pos...
                Card card = cardstack.RemoveCard(i);
                cardstack.Push(card);

                Redraw();

                MoveCard(pDestStack, 1, true);
                break;
            }
        }
    }

    return true;
}

//
//    Redraw the current stack if it has a different
//    layout than the comparison stack.
//
void CardRegion::RedrawIfNotDim(CardRegion *pCompare, bool fFullRedraw)
{
    //
    //
    //
    if( pCompare->xoffset != xoffset || 
        pCompare->yoffset != yoffset || 
        pCompare->nThreedCount != nThreedCount ||
        pCompare->uFaceDirType != uFaceDirType ||
        pCompare->uFaceDirType != CS_FACE_ANY
        )
    {
        if(fFullRedraw)
            parentWnd.Redraw();
        else
            pCompare->Redraw();
    }
    
}

//
//    SimulateDrag mimicks the complete drag+drop process.
//  It basically just a MoveCard(..), but it calls the
//  event callbacks as well.
//
bool CardRegion::SimulateDrag(CardRegion *pDestStack, int iNumDragCards, bool fAnimate)
{
    if(pDestStack == 0)
        return false;

    if(CanDragCards(iNumDragCards) != false)
    {
        //make a list of the cards that would be in the drag list
        CardStack tempstack = cardstack.Top(iNumDragCards);

        if(pDestStack->CanDropCards(tempstack)) 
        {
            MoveCard(pDestStack, iNumDragCards, fAnimate);        
                
            if(RemoveCallback)
                RemoveCallback(*this, iNumDragCards);

            if(pDestStack->AddCallback)
                pDestStack->AddCallback(*pDestStack, pDestStack->cardstack);
        
            RedrawIfNotDim(pDestStack, true);
        }    

    }

    return true;
}
