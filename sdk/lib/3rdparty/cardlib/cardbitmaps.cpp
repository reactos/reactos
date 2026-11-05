//
//    CardLib - Card bitmap support
//
//    Freeware
//    Copyright J Brown 2001
//

#include "cardlib.h"

#ifndef __REACTOS__
#pragma comment( lib, "..\\CardLib\\cards16.lib" )

extern "C" HINSTANCE WINAPI LoadLibrary16( PSTR );
extern "C" void         WINAPI FreeLibrary16( HINSTANCE );
#endif

#define NUMCARDBITMAPS (52+16)

void PaintRect(HDC hdc, RECT *rect, COLORREF col);

static void LoadCardBitmapResources(HINSTANCE hCardDll, int *pwidth, int *pheight)
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

        xpos = i * width;
        FrameCardShape(__hdcCardBitmaps, xpos, 0, width, height,
                       CardCornerRadius(width, height), RGB(0, 0, 0));

        DeleteObject(hBitmap);
    }

    DeleteDC(hdcCard);

    *pwidth = width;
    *pheight = height;

}

/* Use the DLL API so that its user-selected deck and dimensions are honored.
 * Resource loading remains available for legacy libraries without that API. */
static void LoadCardBitmapsFromLibrary(HINSTANCE library, int *width, int *height)
{
    typedef BOOL (WINAPI *InitProc)(int *, int *);
    typedef BOOL (WINAPI *DrawProc)(HDC, int, int, int, int, COLORREF);
    typedef VOID (WINAPI *TermProc)(void);
    InitProc init = (InitProc)GetProcAddress(library, "cdtInit");
    DrawProc draw = (DrawProc)GetProcAddress(library, "cdtDraw");
    TermProc term = (TermProc)GetProcAddress(library, "cdtTerm");

    if (!init || !draw || !term)
    {
        LoadCardBitmapResources(library, width, height);
        return;
    }

    if (!init(width, height))
    {
        PostQuitMessage(1);
        return;
    }

    HDC screen = GetDC(NULL);
    __hdcCardBitmaps = CreateCompatibleDC(screen);
    __hbmCardBitmaps = CreateCompatibleBitmap(screen, *width * NUMCARDBITMAPS, *height);
    ReleaseDC(NULL, screen);
    if (__hdcCardBitmaps && __hbmCardBitmaps)
    {
        HGDIOBJ old = SelectObject(__hdcCardBitmaps, __hbmCardBitmaps);
        for (int i = 0; i < NUMCARDBITMAPS; ++i)
        {
            // Faces use rank/suit order; other images use their resource number.
            if (!draw(__hdcCardBitmaps, i * *width, 0, i < 52 ? i : i + 1,
                      (i < 52 ? 0 : 1) | 0x80000000, RGB(255, 255, 255)))
            {
                SelectObject(__hdcCardBitmaps, old);
                DeleteObject(__hbmCardBitmaps);
                DeleteDC(__hdcCardBitmaps);
                __hbmCardBitmaps = NULL;
                __hdcCardBitmaps = NULL;
                PostQuitMessage(1);
                break;
            }
            if (i != 52 && i < 66)
                FrameCardShape(__hdcCardBitmaps, i * *width, 0, *width, *height,
                               CardCornerRadius(*width, *height), RGB(0, 0, 0));
        }
    }
    else
    {
        DeleteObject(__hbmCardBitmaps);
        DeleteDC(__hdcCardBitmaps);
        __hbmCardBitmaps = NULL;
        __hdcCardBitmaps = NULL;
        PostQuitMessage(1);
    }
    term();
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

        LoadCardBitmapResources(hCardDll, &__cardwidth, &__cardheight);

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
    HBITMAP hbm = CreateCompatibleBitmap(hdcCompat, width, height);
    // The placeholder DC retains this bitmap for subsequent DrawCard calls.
    SelectObject(hdc, hbm);
    RECT rect = {0, 0, width, height};
    COLORREF fg, bg, shadow, shadow2;
    int radius = CardCornerRadius(width, height);

    GetSinkCols(col, &fg, &bg, &shadow, &shadow2);
    PaintRect(hdc, &rect, MAKE_PALETTERGB(bg));
    int saved = ClipCardShape(hdc, 1, 1, width - 2, height - 2, max(0, radius - 1));
    if (saved)
    {
        InflateRect(&rect, -1, -1);
        DrawCheckedRect(hdc, &rect, MAKE_PALETTERGB(bg), MAKE_PALETTERGB(fg));
        RestoreDC(hdc, saved);
    }
    FrameCardShape(hdc, 0, 0, width, height, radius, MAKE_PALETTERGB(shadow));
    FrameCardShape(hdc, 1, 1, width - 2, height - 2, max(0, radius - 1), MAKE_PALETTERGB(shadow2));
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
