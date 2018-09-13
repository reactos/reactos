/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mousectl.c

Abstract:

    This module contains the routines for the Mouse control.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "mousectl.h"
#include "rc.h"



//
//  Constant Declarations.
//

#define MOUSECTL_BITMAP_NAME          MAKEINTRESOURCE(IDB_MOUSE)

#define MOUSECTL_DATA                 0

//
//  These are indices into the VGA palette!
//
#define MOUSECTL_BKGND_INDEX          14    // light cyan
#define MOUSECTL_LBUTTON_INDEX        13    // light magenta
#define MOUSECTL_RBUTTON_INDEX        11    // light yellow
#define MOUSECTL_LBUTTON_ALTERNATE    5     // magenta
#define MOUSECTL_RBUTTON_ALTERNATE    3     // yellow

#define MOUSECTL_NORMAL_COLOR         RGB(255, 255, 255)

#define HALFRGB(c)  RGB(GetRValue(c) / 2, GetGValue(c) / 2, GetBValue(c) / 2)




//
//  Typedef Declarations.
//

typedef struct tagMOUSECTLDATA
{
    HWND     window;         // window handle for this control
    BOOL     swapped;        // are the buttons swapped?

    HBITMAP  image;          // DIB section image of mouse
    HDC      imagedc;        // DC where image lives
    HBITMAP  olddcimage;     // previous inhabitant of imagedc

    POINT    offset;         // offset of bitmap in control
    SIZE     size;           // size of bitmap

} MOUSECTLDATA, *PMOUSECTLDATA;




//
//  Forward Declarations.
//

LRESULT CALLBACK
MouseControlWndProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

void
MouseControlShowSwap(
    PMOUSECTLDATA ctldata);





////////////////////////////////////////////////////////////////////////////
//
//  SetMouseControlData
//
////////////////////////////////////////////////////////////////////////////

__inline void SetMouseControlData(
    HWND window,
    PMOUSECTLDATA ctldata)
{
    SetWindowLongPtr(window, MOUSECTL_DATA, (LONG_PTR)ctldata);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetMouseControlData
//
////////////////////////////////////////////////////////////////////////////

__inline PMOUSECTLDATA GetMouseControlData(
    HWND window)
{
    return ((PMOUSECTLDATA)GetWindowLongPtr(window, MOUSECTL_DATA));
}


////////////////////////////////////////////////////////////////////////////
//
//  RegisterMouseControlStuff
//
////////////////////////////////////////////////////////////////////////////

BOOL RegisterMouseControlStuff(
    HINSTANCE instance)
{
    WNDCLASS wc;

    wc.style         = 0;
    wc.lpfnWndProc   = MouseControlWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PMOUSECTLDATA);
    wc.hInstance     = instance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = MOUSECTL_CLASSNAME;

    return (RegisterClass(&wc));
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateMouseControlData
//
////////////////////////////////////////////////////////////////////////////

PMOUSECTLDATA CreateMouseControlData(
    HWND window)
{
    PMOUSECTLDATA ctldata = (PMOUSECTLDATA)LocalAlloc(LPTR, sizeof(MOUSECTLDATA));

    if (ctldata)
    {
        ctldata->window = window;
        SetMouseControlData(window, ctldata);
    }

    return (ctldata);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyMouseControlData
//
////////////////////////////////////////////////////////////////////////////

void DestroyMouseControlData(
    PMOUSECTLDATA ctldata)
{
    if (ctldata)
    {
        SetMouseControlData(ctldata->window, 0);

        LocalFree(ctldata);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlNCCreate
//
////////////////////////////////////////////////////////////////////////////

BOOL MouseControlNCCreate(
    PMOUSECTLDATA ctldata,
    LPCREATESTRUCT cs)
{
    if (!ctldata)
    {
        return (FALSE);
    }

    ctldata->image = LoadImage( cs->hInstance,
                                MOUSECTL_BITMAP_NAME,
                                IMAGE_BITMAP,
                                0,
                                0,
                                LR_CREATEDIBSECTION );

    if (ctldata->image)
    {
        ctldata->imagedc = CreateCompatibleDC(NULL);

        if (ctldata->imagedc)
        {
            BITMAP bm;

            ctldata->olddcimage =
                SelectBitmap(ctldata->imagedc, ctldata->image);

            ctldata->offset.x = ctldata->offset.y = 0;

            if (GetObject(ctldata->image, sizeof(BITMAP), &bm))
            {
                ctldata->size.cx  = bm.bmWidth;
                ctldata->size.cy  = bm.bmHeight;
                ctldata->offset.x = (cs->cx - ctldata->size.cx) / 2;
                ctldata->offset.y = (cs->cy - ctldata->size.cy) / 2;

                ctldata->swapped = FALSE;
                MouseControlShowSwap(ctldata);
                return (TRUE);
            }

            SelectBitmap(ctldata->imagedc, ctldata->olddcimage);
            ctldata->olddcimage = NULL;
        }

        DeleteBitmap(ctldata->image);
        ctldata->image = NULL;
    }

    ctldata->olddcimage = NULL;
    ctldata->imagedc    = NULL;

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlNCDestroy
//
////////////////////////////////////////////////////////////////////////////

void MouseControlNCDestroy(
    PMOUSECTLDATA ctldata)
{
    if (ctldata)
    {
        if (ctldata->olddcimage)
        {
            SelectBitmap(ctldata->imagedc, ctldata->olddcimage);
            ctldata->olddcimage = NULL;
        }

        if (ctldata->imagedc)
        {
            DeleteDC(ctldata->imagedc);
            ctldata->imagedc = NULL;
        }

        if (ctldata->image)
        {
            DeleteBitmap(ctldata->image);
            ctldata->image = NULL;
        }

        DestroyMouseControlData(ctldata);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlPaint
//
////////////////////////////////////////////////////////////////////////////

void MouseControlPaint(
    PMOUSECTLDATA ctldata)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(ctldata->window, &ps);

    BitBlt( dc,
            ctldata->offset.x,
            ctldata->offset.y,
            ctldata->size.cx,
            ctldata->size.cy,
            ctldata->imagedc,
            0,
            0,
            SRCCOPY );

    EndPaint(ctldata->window, &ps);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlSetColor
//
////////////////////////////////////////////////////////////////////////////

__inline void MouseControlSetColor(
    PMOUSECTLDATA ctldata,
    UINT index,
    COLORREF color)
{
    RGBQUAD rgbq = { GetBValue(color), GetGValue(color), GetRValue(color), 0 };

    SetDIBColorTable(ctldata->imagedc, index, 1, &rgbq);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlShowSwap
//
////////////////////////////////////////////////////////////////////////////

void MouseControlShowSwap(
    PMOUSECTLDATA ctldata)
{
    MouseControlSetColor( ctldata,
                          MOUSECTL_BKGND_INDEX,
                          GetSysColor(COLOR_BTNFACE) );

    MouseControlSetColor( ctldata,
                          ctldata->swapped
                            ? MOUSECTL_RBUTTON_INDEX
                            : MOUSECTL_LBUTTON_INDEX,
                          GetSysColor(COLOR_HIGHLIGHT) );

    MouseControlSetColor( ctldata,
                          ctldata->swapped
                            ? MOUSECTL_RBUTTON_ALTERNATE
                            : MOUSECTL_LBUTTON_ALTERNATE,
                          HALFRGB(GetSysColor(COLOR_HIGHLIGHT)) );

    MouseControlSetColor( ctldata,
                          ctldata->swapped
                            ? MOUSECTL_LBUTTON_INDEX
                            : MOUSECTL_RBUTTON_INDEX,
                          MOUSECTL_NORMAL_COLOR );

    MouseControlSetColor( ctldata,
                          ctldata->swapped
                            ? MOUSECTL_LBUTTON_ALTERNATE
                            : MOUSECTL_RBUTTON_ALTERNATE,
                          HALFRGB(MOUSECTL_NORMAL_COLOR) );

    InvalidateRect(ctldata->window, NULL, FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlSetSwap
//
////////////////////////////////////////////////////////////////////////////

void MouseControlSetSwap(
    HWND window,
    BOOL swap)
{
    PMOUSECTLDATA ctldata = GetMouseControlData(window);

    if (ctldata->swapped != swap)
    {
        ctldata->swapped = swap;
        MouseControlShowSwap(ctldata);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseControlWndProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MouseControlWndProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PMOUSECTLDATA ctldata = (message == WM_NCCREATE)
                              ? CreateMouseControlData(window)
                              : GetMouseControlData(window);

    switch (message)
    {
        HANDLE_MSG(ctldata, WM_NCCREATE,  MouseControlNCCreate );
        HANDLE_MSG(ctldata, WM_NCDESTROY, MouseControlNCDestroy);
        HANDLE_MSG(ctldata, WM_PAINT,     MouseControlPaint    );
    }

    return (DefWindowProc(window, message, wParam, lParam));
}
