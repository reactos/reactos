//
//    CardLib - Card bitmap support
//
//    Freeware
//    Copyright J Brown 2001
//
#include <windows.h>
#include "globals.h"
#include "cardcolor.h"

#ifndef __REACTOS__
#pragma comment( lib, "..\\CardLib\\cards16.lib" )

extern "C" HINSTANCE WINAPI LoadLibrary16( PSTR );
extern "C" void         WINAPI FreeLibrary16( HINSTANCE );
#endif

#define NUMCARDBITMAPS (52+16)

void PaintRect(HDC hdc, RECT *rect, COLORREF col);

void LoadCardBitmapsFromLibrary(HINSTANCE hCardDll, int *pwidth, int *pheight)
{
    HBITMAP   hBitmap;
    HDC          hdcCard = NULL;
    HANDLE      hOld;
    int        i, xpos;
    int        width, height;
    BITMAP bmp;

    for(i = 0; i < NUMCARDBITMAPS; i++)
    {
        //convert into the range used by the cdt_xxx functions
        int val;
        
        if(i < 52) val = (i % 4) * 13 + (i/4);
        else       val = i;
        
        hBitmap = LoadBitmap(hCardDll, MAKEINTRESOURCE(val + 1));
        GetObject(hBitmap, sizeof(bmp), &bmp);
        
        width  = bmp.bmWidth;
        height = bmp.bmHeight;

        if(i == 0)    //if first time through, create BIG bitmap..
        {
            HDC hdc = GetDC(0);
            __hdcCardBitmaps = CreateCompatibleDC(hdc);
            __hbmCardBitmaps = CreateCompatibleBitmap(hdc, width * NUMCARDBITMAPS, height);
            SelectObject(__hdcCardBitmaps, __hbmCardBitmaps);

            hdcCard = CreateCompatibleDC(0);

            ReleaseDC(0, hdc);
        }
        
        hOld = SelectObject(hdcCard, hBitmap);
        BitBlt(__hdcCardBitmaps, i*width, 0, width, height, hdcCard, 0, 0, SRCCOPY);
        SelectObject(hdcCard, hOld);
        
        //Now draw a black border around each card...
        xpos = i*width;
        MoveToEx(__hdcCardBitmaps, xpos+2, 0, 0);
        LineTo(__hdcCardBitmaps, xpos+width - 3, 0);
        LineTo(__hdcCardBitmaps, xpos+width - 1, 2);
        LineTo(__hdcCardBitmaps, xpos+width - 1, height - 3);    //vertical
        LineTo(__hdcCardBitmaps, xpos+width - 3, height - 1);
        LineTo(__hdcCardBitmaps, xpos+2, height - 1);
        LineTo(__hdcCardBitmaps, xpos+0, height - 3);
        LineTo(__hdcCardBitmaps, xpos+0, 2);
        LineTo(__hdcCardBitmaps, xpos+2, 0);
        
        DeleteObject(hBitmap);
    }
    
    DeleteDC(hdcCard);

    *pwidth = width;
    *pheight = height;
                
}

void LoadCardBitmaps(void)
{
    HINSTANCE hCardDll;
    

    //If Windows NT/2000/XP
    if(GetVersion() < 0x80000000)
    {
        hCardDll = LoadLibrary(TEXT("cards.dll"));

        if(hCardDll == 0)
        {
            MessageBox(0, TEXT("Error loading cards.dll (32bit)"), TEXT("Shed"), MB_OK | MB_ICONEXCLAMATION);
            PostQuitMessage(0);
            return;
        }
        
        LoadCardBitmapsFromLibrary(hCardDll, &__cardwidth, &__cardheight);
        
        FreeLibrary(hCardDll);
    }
#ifndef __REACTOS__
    //Else, Win9X
    else
    {
        hCardDll = LoadLibrary16("cards.dll");

        if(hCardDll == 0)
        {
            MessageBox(0, "Error loading cards.dll (16bit)", "Shed", MB_OK | MB_ICONEXCLAMATION);
            PostQuitMessage(0);
            return;
        }

        LoadCardBitmapsFromLibrary(hCardDll, &__cardwidth, &__cardheight);

        FreeLibrary16(hCardDll);
    }
#endif
}

void FreeCardBitmaps()
{
    DeleteObject (__hbmCardBitmaps);
    DeleteDC     (__hdcCardBitmaps);
}
//
//    Paint a checkered rectangle, with each alternate
//    pixel being assigned a different colour
//
static void DrawCheckedRect(HDC hdc, RECT *rect, COLORREF fg, COLORREF bg)
{
    static WORD wCheckPat[8] = 
    { 
        0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555 
    };

    HBITMAP hbmp;
    HBRUSH  hbr, hbrold;
    COLORREF fgold, bgold;

    hbmp = CreateBitmap(8, 8, 1, 1, wCheckPat);
    hbr  = CreatePatternBrush(hbmp);

    //UnrealizeObject(hbr);

    SetBrushOrgEx(hdc, rect->left, rect->top, 0);

    hbrold = (HBRUSH)SelectObject(hdc, hbr);

    fgold = SetTextColor(hdc, fg);
    bgold = SetBkColor(hdc, bg);
    
    PatBlt(hdc, rect->left, rect->top, 
                rect->right - rect->left, 
                rect->bottom - rect->top, 
                PATCOPY);
    
    SetBkColor(hdc, bgold);
    SetTextColor(hdc, fgold);
    
    SelectObject(hdc, hbrold);
    DeleteObject(hbr);
    DeleteObject(hbmp);
}

void GetSinkCols(COLORREF crBase, COLORREF *fg, COLORREF *bg, COLORREF *sh1, COLORREF *sh2)
{
    if(bg) *bg     = crBase;
    if(fg) *fg   = ColorScaleRGB(crBase, RGB(255,255,255), 0.2);//RGB(49, 99, 140);
    if(sh1) *sh1 = ColorScaleRGB(crBase, RGB(0,0,0), 0.4);
    if(sh2) *sh2 = ColorScaleRGB(crBase, RGB(0,0,0), 0.2);
}

HBITMAP CreateSinkBmp(HDC hdcCompat, HDC hdc, COLORREF col, int width, int height)
{
    HANDLE hold, hpold;
    HBITMAP hbm = CreateCompatibleBitmap(hdcCompat, width, height);

    HPEN hpfg, hpbg, hpsh, hpsh2;

    RECT rect;
    COLORREF fg, bg, shadow, shadow2;

    GetSinkCols(col, &fg, &bg, &shadow, &shadow2);

    hold = SelectObject(hdc, hbm);

    //fill with a solid base colour
    SetRect(&rect, 0,0,width,height);
    PaintRect(hdc, &rect, MAKE_PALETTERGB(bg));

    //draw the outline
    hpfg = CreatePen(PS_SOLID, 0, MAKE_PALETTERGB(fg));
    hpbg = CreatePen(PS_SOLID, 0, MAKE_PALETTERGB(bg));
    hpsh = CreatePen(PS_SOLID, 0, MAKE_PALETTERGB(shadow));
    hpsh2= CreatePen(PS_SOLID, 0, MAKE_PALETTERGB(shadow2));    

    hpold = SelectObject(hdc, hpsh);
    MoveToEx(hdc, 2, 0, NULL);
    LineTo  (hdc, width-3,0);
    LineTo  (hdc, width-1, 2);
    
    SelectObject(hdc, hpold);
    hpold = SelectObject(hdc, hpsh2);
    LineTo  (hdc, width-1, height-3);    //vertical
    LineTo  (hdc, width-3, height-1);
    LineTo  (hdc, 2, height-1);
    LineTo  (hdc, 0, height-3);
    SelectObject(hdc, hpold);
    hpold = SelectObject(hdc, hpsh);

    //MoveToEx( hdc, 0, height-3,0);
    LineTo  (hdc, 0, 2);
    LineTo  (hdc, 2, 0);

    SelectObject(hdc, hpold);

    //draw the highlight (vertical)
    hpold = SelectObject(hdc, hpfg);
    MoveToEx(hdc, width - 2, 3, NULL);
    LineTo  (hdc, width - 2, height - 2);
    
    //(horz)
    MoveToEx(hdc, width - 3, height-2, NULL);
    LineTo  (hdc, 3, height-2);
    SelectObject(hdc, hpold);
    
    //draw the background
    InflateRect(&rect, -2, -2);
    DrawCheckedRect(hdc, &rect, MAKE_PALETTERGB(bg), MAKE_PALETTERGB(fg));

    //overwrite the top-left background pixel
    SetPixel(hdc, 2, 2, MAKE_PALETTERGB(bg));

    DeleteObject(hpsh);
    DeleteObject(hpsh2);
    DeleteObject(hpfg);
    DeleteObject(hpbg);

    
    return hbm;
}



void CopyColor(PALETTEENTRY *pe, COLORREF col)
{
    pe->peBlue  = GetBValue(col);
    pe->peGreen = GetGValue(col);
    pe->peRed   = GetRValue(col);
    pe->peFlags = 0;
}

HPALETTE MakePaletteFromCols(COLORREF cols[], int nNumColours)
{
    LOGPALETTE    *lp;
    HPALETTE    hPalette;

    //    Allocate memory for the logical palette
    lp = (LOGPALETTE *)HeapAlloc(
        GetProcessHeap(), 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * nNumColours);

    lp->palNumEntries = (WORD)nNumColours;
    lp->palVersion    = 0x300;

    //copy the colours into the logical palette format
    for(int i = 0; i < nNumColours; i++)
    {
        CopyColor(&lp->palPalEntry[i], cols[i]);
    }

    // create palette!
    hPalette = CreatePalette(lp);

    HeapFree(GetProcessHeap(), 0, lp);

    return hPalette;
}
