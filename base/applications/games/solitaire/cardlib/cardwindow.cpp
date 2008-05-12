//
//    CardLib - CardWindow class
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

#include "globals.h"
#include "cardlib.h"
#include "cardbutton.h"
#include "cardregion.h"
#include "cardwindow.h"
#include "cardcolor.h"

extern HPALETTE __holdplacepal;

HPALETTE UseNicePalette(HDC hdc, HPALETTE hPalette)
{
    HPALETTE hOld;

    hOld = SelectPalette(hdc, hPalette, FALSE);
    RealizePalette(hdc);

    return hOld;
}

void RestorePalette(HDC hdc, HPALETTE hOldPal)
{
    SelectPalette(hdc, hOldPal, TRUE);
}

HPALETTE MakePaletteFromCols(COLORREF cols[], int nNumColours);
void     PaintRect(HDC hdc, RECT *rect, COLORREF colour);
HBITMAP  CreateSinkBmp(HDC hdcCompat, HDC hdc, COLORREF col, int width, int height);
void     GetSinkCols(COLORREF crBase, COLORREF *fg, COLORREF *bg, COLORREF *sh1, COLORREF *sh2);

void     LoadCardBitmaps();
void     FreeCardBitmaps();

static TCHAR szCardName[]   = _T("CardWnd32");
static bool  fRegistered    = false;
static LONG  uCardBitmapRef = 0;


void RegisterCardWindow()
{
    WNDCLASSEX wc;

    //Window class for the main application parent window
    wc.cbSize            = sizeof(wc);
    wc.style            = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc        = CardWindow::CardWndProc;
    wc.cbClsExtra        = 0;
    wc.cbWndExtra        = sizeof(CardWindow *);
    wc.hInstance        = GetModuleHandle(0);
    wc.hIcon            = 0;
    wc.hCursor            = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground    = 0;
    wc.lpszMenuName        = 0;
    wc.lpszClassName    = szCardName;
    wc.hIconSm            = 0;

    RegisterClassEx(&wc);
}

CardWindow::CardWindow() : m_hWnd(0)
{
    HDC hdc = GetDC(0);

    nNumButtons       = 0;
    nNumCardRegions   = 0;
    nNumDropZones     = 0;
    nBackCardIdx      = 53;

    ResizeWndCallback = 0;
    hbmBackImage      = 0;
    hdcBackImage      = 0;

    srand((unsigned)GetTickCount());

    //All colours (buttons, highlights, decks)
    //are calculated off this single base colour
    crBackgnd = PALETTERGB(0,80,0);//PALETTERGB(0,64,100);

    // If uCardBitmapRef was previously zero, then
    // load the card bitmaps
    if(1 == InterlockedIncrement(&uCardBitmapRef))
    {
        LoadCardBitmaps();

        __hPalette  = CreateCardPalette();

        __hdcPlaceHolder  = CreateCompatibleDC(hdc);

        __holdplacepal  = UseNicePalette(__hdcPlaceHolder, __hPalette);

        __hbmPlaceHolder  = CreateSinkBmp(hdc, __hdcPlaceHolder, crBackgnd, __cardwidth, __cardheight);

    }

    ReleaseDC(0, hdc);

    //register the window class if necessary
    if(!fRegistered)
    {
        fRegistered = true;
        RegisterCardWindow();
    }

}

BOOL CardWindow::Create(HWND hwndParent, DWORD dwExStyle, DWORD dwStyle, int x, int y, int width, int height)
{
    if(m_hWnd)
        return FALSE;

    //Create the window associated with this object
    m_hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, szCardName, 0,
        WS_CHILD | WS_VISIBLE,
        0,0,100,100,
        hwndParent, 0, GetModuleHandle(0), this);

    return TRUE;
}

BOOL CardWindow::Destroy()
{
    DestroyWindow(m_hWnd);
    m_hWnd = 0;

    return TRUE;
}

CardWindow::~CardWindow()
{
    if(m_hWnd)
        DestroyWindow(m_hWnd);

    DeleteAll();

    if(0 == InterlockedDecrement(&uCardBitmapRef))
    {
        FreeCardBitmaps();

        DeleteObject(__hbmPlaceHolder);
        DeleteDC    (__hdcPlaceHolder);

        RestorePalette(__hdcPlaceHolder, __holdplacepal);

        if(__hPalette)
            DeleteObject(__hPalette);
    }
}

bool CardWindow::DeleteAll()
{
    int i;

    for(i = 0; i < nNumCardRegions; i++)
    {
        delete Regions[i];
    }

    for(i = 0; i < nNumButtons; i++)
    {
        delete Buttons[i];
    }

    for(i = 0; i < nNumDropZones; i++)
    {
        delete dropzone[i];
    }

    nNumCardRegions = nNumButtons = nNumDropZones = 0;

    return true;
}

void CardWindow::SetBackColor(COLORREF cr)
{
    crBackgnd = cr;
    int i;

    //
    // Create the exact palette we need to render the buttons/stacks
    //
    RestorePalette(__hdcPlaceHolder, __holdplacepal);

    if(__hPalette)
        DeleteObject(__hPalette);

    __hPalette = CreateCardPalette();

    //
    // re-create the place-holder!
    HDC hdc = GetDC(m_hWnd);

    DeleteObject(__hbmPlaceHolder);

    __holdplacepal = UseNicePalette(__hdcPlaceHolder, __hPalette);

    __hbmPlaceHolder = CreateSinkBmp(hdc, __hdcPlaceHolder, crBackgnd, __cardwidth, __cardheight);
    //SelectObject(__hdcPlaceHolder, __hbmPlaceHolder);

    //reset all buttons to same colour
    for(i = 0; i < nNumButtons; i++)
    {
        if(Buttons[i]->GetStyle() & CB_PUSHBUTTON)
        {
            Buttons[i]->SetBackColor(ColorScaleRGB(crBackgnd, RGB(255,255,255), 0.1));
        }
        else
        {
            Buttons[i]->SetBackColor(crBackgnd);
        }
    }

    for(i = 0; i < nNumCardRegions; i++)
    {
        Regions[i]->SetBackColor(crBackgnd);
    }


    ReleaseDC(m_hWnd, hdc);
}

COLORREF CardWindow::GetBackColor()
{
    return crBackgnd;
}

CardButton* CardWindow::CardButtonFromPoint(int x, int y)
{
    CardButton *bptr = 0;

    POINT pt;
    pt.x = x;
    pt.y = y;

    //Search BACKWARDS...to reflect the implicit Z-order that
    //the button creation provided
    for(int i = nNumButtons - 1; i >= 0; i--)
    {
        bptr = Buttons[i];
        if(PtInRect(&bptr->rect, pt) && bptr->fVisible)
            return bptr;
    }

    return 0;
}

CardRegion* CardWindow::CardRegionFromPoint(int x, int y)
{
    POINT pt;
    pt.x = x;
    pt.y = y;

    //Search BACKWARDS...to reflect the implicit Z-order that
    //the stack creation provided
    for(int i = nNumCardRegions - 1; i >= 0; i--)
    {
        if(Regions[i]->IsPointInStack(x, y))
            return Regions[i];
    }

    return 0;
}

//
//    Forward all window messages onto the appropriate
//  class instance
//
LRESULT CALLBACK CardWindow::CardWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    CardWindow *cw = (CardWindow *)GetWindowLong(hwnd, 0);
    return cw->WndProc(hwnd, iMsg, wParam, lParam);
}

void CardWindow::Paint(HDC hdc)
{
    int i;
    RECT rect;
    HPALETTE hOldPal;

    hOldPal = UseNicePalette(hdc, __hPalette);

    //
    //    Clip the card stacks so that they won't
    //    get painted over
    //
    for(i = 0; i < nNumCardRegions; i++)
    {
        Regions[i]->Clip(hdc);
    }

    //
    //    Clip the buttons
    //
    for(i = 0; i < nNumButtons; i++)
    {
        Buttons[i]->Clip(hdc);
    }


    //    Now paint the whole screen with background colour,
    //
    GetClientRect(m_hWnd, &rect);

    //PaintRect(hdc, &rect, MAKE_PALETTERGB(crBackgnd));
    PaintCardRgn(hdc, 0, 0, rect.right, rect.bottom, 0, 0);
    SelectClipRgn(hdc, NULL);

    //    Don't let cards draw over buttons, so clip buttons again
    //
    for(i = 0; i < nNumButtons; i++)
    {
        Buttons[i]->Clip(hdc);
    }

    //    Paint each card stack in turn
    //
    for(i = 0; i < nNumCardRegions; i++)
    {
        Regions[i]->Render(hdc);
    }

    //    Paint each button now
    //
    SelectClipRgn(hdc, NULL);

    for(i = 0; i < nNumButtons; i++)
    {
        Buttons[i]->Redraw();
    }

    RestorePalette(hdc, hOldPal);
}




LRESULT CALLBACK CardWindow::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    CREATESTRUCT *cs;

    static CardButton *buttonptr   = 0;
    static CardRegion *stackptr    = 0;

    int x, y, i;

    switch(iMsg)
    {
    case WM_NCCREATE:

        // When we created this window, we passed in the
        // pointer to the class object (CardWindow *) in the
        // call to CreateWindow.
        cs = (CREATESTRUCT *)lParam;

        //
        // associate this class with the window
        //
        SetWindowLong(hwnd, 0, (LONG)cs->lpCreateParams);

        return 1;

    case WM_NCDESTROY:
        // Don't delete anything here..
        break;

    case WM_SIZE:
        nWidth = LOWORD(lParam);
        nHeight = HIWORD(lParam);

        //
        // reposition all the stacks and buttons
        // in case any of them are centered, right-justified etc
        //
        for(i = 0; i < nNumCardRegions; i++)
        {
            Regions[i]->AdjustPosition(nWidth, nHeight);
        }

        for(i = 0; i < nNumButtons; i++)
        {
            Buttons[i]->AdjustPosition(nWidth, nHeight);
        }

        //
        // Call the user-defined resize proc AFTER all the stacks
        // have been positioned
        //
        if(ResizeWndCallback)
            ResizeWndCallback(nWidth, nHeight);

        return 0;

    case WM_PAINT:

        hdc = BeginPaint(hwnd, &ps);

        Paint(hdc);

        EndPaint(hwnd, &ps);
        return 0;

    case WM_TIMER:

        //find the timer object in the registered funcs
        /*if(wParam >= 0x10000)
        {
            for(i = 0; i < nRegFuncs; i++)
            {
                if(RegFuncs[i].id == wParam)
                {
                    KillTimer(hwnd, wParam);

                    //call the registered function!!
                    RegFuncs[i].func(RegFuncs[i].dwParam);

                    RegFuncs[i] = RegFuncs[nRegFuncs-1];
                    nRegFuncs--;
                }
            }
        }
        else*/
        {
            //find the cardstack
            CardRegion *stackobj = (CardRegion *)wParam;//CardStackFromId(wParam);
            stackobj->DoFlash();
        }

        return 0;

    case WM_LBUTTONDBLCLK:

        x = (short)LOWORD(lParam);
        y = (short)HIWORD(lParam);

        if((buttonptr = CardButtonFromPoint(x, y)) != 0)
        {
            buttonptr->OnLButtonDown(hwnd, x, y);
            return 0;
        }

        if((stackptr = CardRegionFromPoint(x, y)) != 0)
        {
            stackptr->OnLButtonDblClk(x, y);
            stackptr = 0;
        }

        return 0;

    case WM_LBUTTONDOWN:

        x = (short)LOWORD(lParam);
        y = (short)HIWORD(lParam);

        //if clicked on a button
        if((buttonptr = CardButtonFromPoint(x, y)) != 0)
        {
            if(buttonptr->OnLButtonDown(hwnd, x, y) == 0)
                buttonptr = 0;

            return 0;
        }

        if((stackptr = CardRegionFromPoint(x, y)) != 0)
        {
            if(!stackptr->OnLButtonDown(x, y))
                stackptr = 0;
        }

        return 0;

    case WM_LBUTTONUP:

        x = (short)LOWORD(lParam);
        y = (short)HIWORD(lParam);

        //
        // if we were clicking a button
        //
        if(buttonptr != 0)
        {
            buttonptr->OnLButtonUp(hwnd, x, y);
            buttonptr = 0;
            return 0;
        }

        if(stackptr != 0)
        {
            stackptr->OnLButtonUp(x, y);
            stackptr = 0;
            return 0;
        }

        return 0;

    case WM_MOUSEMOVE:

        x = (short)LOWORD(lParam);
        y = (short)HIWORD(lParam);

        // if we were clicking a button
        if(buttonptr != 0)
        {
            buttonptr->OnMouseMove(hwnd, x, y);
            return 0;
        }

        if(stackptr != 0)
        {
            return stackptr->OnMouseMove(x, y);
        }

        return 0;

    }

      return DefWindowProc (hwnd, iMsg, wParam, lParam);
}


CardRegion* CardWindow::CardRegionFromId(int id)
{
    for(int i = 0; i < nNumCardRegions; i++)
    {
        if(Regions[i]->id == id)
            return Regions[i];
    }

    return 0;
}

CardButton* CardWindow::CardButtonFromId(int id)
{
    for(int i = 0; i < nNumButtons; i++)
    {
        if(Buttons[i]->id == id)
            return Buttons[i];
    }

    return 0;
}

void CardWindow::Redraw()
{
    InvalidateRect(m_hWnd, 0, 0);
    UpdateWindow(m_hWnd);
}

bool CardWindow::DeleteButton(CardButton *pButton)
{
    for(int i = 0; i < nNumButtons; i++)
    {
        if(Buttons[i] == pButton)
        {
            CardButton *cb = Buttons[i];

            //shift any after this one backwards
            for(int j = i; j < nNumButtons - 1; j++)
            {
                Buttons[j] = Buttons[j + 1];
            }

            delete cb;
            nNumButtons--;

            return true;
        }
    }

    return false;
}

bool CardWindow::DeleteRegion(CardRegion *pRegion)
{
    for(int i = 0; i < nNumCardRegions; i++)
    {
        if(Regions[i] == pRegion)
        {
            CardRegion *cr = Regions[i];

            //shift any after this one backwards
            for(int j = i; j < nNumCardRegions - 1; j++)
            {
                Regions[j] = Regions[j + 1];
            }

            delete cr;
            nNumCardRegions--;

            return true;
        }
    }

    return false;
}

void CardWindow::EmptyStacks(void)
{
    for(int i = 0; i < nNumCardRegions; i++)
    {
        Regions[i]->Clear();
        Regions[i]->Update();
    }

    Redraw();
}

bool CardWindow::DistributeStacks(int nIdFrom, int nNumStacks, UINT xJustify, int xSpacing, int nStartX)
{
    int numvisiblestacks = 0;
    int curx = nStartX;
    int startindex = -1;
    int i;

    //find the stack which starts with our ID
    for(i = 0; i < nNumCardRegions; i++)
    {
        if(Regions[i]->Id() == nIdFrom)
        {
            startindex = i;
            break;
        }
    }

    //if didn't find, return
    if(i == nNumCardRegions) return false;

    //count the stacks that are visible
    for(i = startindex; i < startindex + nNumStacks; i++)
    {
        if(Regions[i]->IsVisible())
            numvisiblestacks++;
    }

    if(xJustify == CS_XJUST_CENTER)
    {
        //startx -= ((numvisiblestacks + spacing) * cardwidth - spacing) / 2;
        int viswidth;
        viswidth = numvisiblestacks * __cardwidth;
        viswidth += xSpacing * (numvisiblestacks - 1);
        curx = -(viswidth  - __cardwidth) / 2;

        for(i = startindex; i < startindex + nNumStacks; i++)
        {
            if(Regions[i]->IsVisible())
            {
                Regions[i]->xadjust = curx;
                Regions[i]->xjustify = CS_XJUST_CENTER;
                curx += Regions[i]->width + xSpacing;
            }

        }
    }

    if(xJustify == CS_XJUST_RIGHT)
    {
        nStartX -= ((numvisiblestacks + xSpacing) * __cardwidth - xSpacing);
    }

    if(xJustify == CS_XJUST_NONE)
    {
        for(i = startindex; i < startindex + nNumStacks; i++)
        {
            if(Regions[i]->IsVisible())
            {
                Regions[i]->xpos = curx;
                curx += Regions[i]->width + xSpacing;
                Regions[i]->UpdateSize();
            }

        }
    }

    return 0;
}

void CardWindow::Update()
{
    for(int i = 0; i < nNumCardRegions; i++)
    {
        Regions[i]->AdjustPosition(nWidth, nHeight);
    }
}


void CardWindow::SetResizeProc(pResizeWndProc proc)
{
    ResizeWndCallback = proc;
}


HPALETTE CardWindow::CreateCardPalette()
{
    COLORREF cols[10];
    int nNumCols;


    //include button text colours
    cols[0] = RGB(0, 0, 0);
    cols[1] = RGB(255, 255, 255);

    //include the base background colour
    cols[1] = crBackgnd;

    //include the standard button colours...
    cols[3] = CardButton::GetHighlight(crBackgnd);
    cols[4] = CardButton::GetShadow(crBackgnd);
    cols[5] = CardButton::GetFace(crBackgnd);

    //include the sunken image bitmap colours...
    GetSinkCols(crBackgnd, &cols[6], &cols[7], &cols[8], &cols[9]);

    nNumCols = 10;

    return MakePaletteFromCols(cols, nNumCols);
}

void CardWindow::SetBackCardIdx(UINT uBackIdx)
{
    if(uBackIdx >= 52 && uBackIdx <= 68)
        nBackCardIdx = uBackIdx;

    for(int i = 0; i < nNumCardRegions; i++)
        Regions[i]->SetBackCardIdx(uBackIdx);

}

UINT CardWindow::GetBackCardIdx()
{
    return nBackCardIdx;
}

void CardWindow::PaintCardRgn(HDC hdc, int dx, int dy, int width, int height, int sx, int sy)
{
    RECT rect;

    //if just a solid background colour
    if(hbmBackImage == 0)
    {
        SetRect(&rect, dx, dy, dx+width, dy+height);

        /*if(GetVersion() < 0x80000000)
        {
            PaintRect(hdc, &rect, MAKE_PALETTERGB(crBackgnd));
        }
        else*/
        {
            HBRUSH hbr = CreateSolidBrush(MAKE_PALETTERGB(crBackgnd));
            FillRect(hdc, &rect, hbr);
            DeleteObject(hbr);
        }
    }
    //otherwise, paint using the bitmap
    else
    {
        // Draw whatever part of background we can
        BitBlt(hdc, dx, dy, width, height, hdcBackImage, sx, sy, SRCCOPY);

        // Now we need to paint any area outside the bitmap,
        // just in case the bitmap is too small to fill whole window
        if(0)//sx + width > bm.bmWidth || sy + height > bm.bmHeight)
        {
            // Find out size of bitmap
            BITMAP bm;
            GetObject(hbmBackImage, sizeof(bm), &bm);

            HRGN hr1 = CreateRectRgn(sx, sy, sx+width, sy+height);
            HRGN hr2 = CreateRectRgn(0, 0, bm.bmWidth, bm.bmHeight);
            HRGN hr3 = CreateRectRgn(0,0, 1, 1);
            HRGN hr4 = CreateRectRgn(0,0, 1, 1);

            CombineRgn(hr3, hr1, hr2, RGN_DIFF);

            GetClipRgn(hdc, hr4);

            CombineRgn(hr3, hr4, hr3, RGN_AND);
            SelectClipRgn(hdc, hr3);

            // Fill remaining space not filled with bitmap
            HBRUSH hbr = CreateSolidBrush(crBackgnd);
            FillRgn(hdc, hr3, hbr);
            DeleteObject(hbr);

            // Clean up
            SelectClipRgn(hdc, hr4);

            DeleteObject(hr1);
            DeleteObject(hr2);
            DeleteObject(hr3);
            DeleteObject(hr4);
        }
    }
}

void CardWindow::SetBackImage(HBITMAP hBitmap)
{
    //delete current image?? NO!
    if(hdcBackImage == 0)
    {
        hdcBackImage = CreateCompatibleDC(0);
    }

    hbmBackImage = hBitmap;

    if(hBitmap)
        SelectObject(hdcBackImage, hBitmap);
}
