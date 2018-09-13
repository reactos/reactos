/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   rlmeter.c: Audio recording level window
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * This window class acts as a 'VU Meter' showing the current and peak
 * volume. Set the volume via the WMRL_SETLEVEL message (lParam is new level).
 * The peak level will be tracked by the control by means of a 2-second timer.
 */

#include <windows.h>
#include <windowsx.h>

#include "rlmeter.h"

#ifdef _WIN32
#ifndef EXPORT
#define EXPORT
#endif
#endif

LRESULT FAR PASCAL EXPORT
RLMeterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * generic window class to support a volume level display.
 *
 * The window has a white background, and draws a black filled
 * rectangle to show the current volume level, and a red line at the
 * peak. Every two seconds on a timer we lower the peak (we set the
 * saved peak value to 0 so that at the next update we move the line to
 * whatever is the current level.
 *
 * We store the pen and brush handles and the current and maximum levels
 * as window words using SetWindowWord on win16 and SetWindowLong on win32.
 */

// window data layout
#define WD_MAX      0                           // current max
#define WD_PREVMAX  (WD_MAX + sizeof(UINT))     // currently drawn max
#define WD_PREVLVL  (WD_PREVMAX + sizeof(UINT)) // currently drawn level

#define WD_PEN      (WD_PREVLVL + sizeof(UINT)) // pen for max line

#define WDBYTES     (WD_PEN + sizeof(UINT_PTR))     // window bytes to alloc

#ifdef _WIN32
#define SetWindowUINT     SetWindowLong
#define GetWindowUINT     GetWindowLong
#define SetWindowUINTPtr     SetWindowLongPtr
#define GetWindowUINTPtr     GetWindowLongPtr
#else
#define SetWindowUINT     SetWindowWord
#define GetWindowUINT     GetWindowWord
#define SetWindowUINTPtr     SetWindowWord
#define GetWindowUINTPtr     GetWindowWord
#endif


// call (if first instance) to register class
BOOL
RLMeter_Register(HINSTANCE hInstance)
{
    WNDCLASS cls;

    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = RLMETERCLASS;
    cls.hbrBackground  = GetStockObject(WHITE_BRUSH);
    cls.hInstance      = hInstance;
    cls.style          = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc    = RLMeterProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = WDBYTES;

    return RegisterClass(&cls);


}


LRESULT FAR PASCAL EXPORT
RLMeterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
    case WM_CREATE:
        // init current level and max to 0
        SetWindowUINT(hwnd, WD_MAX, 0);
        SetWindowUINT(hwnd, WD_PREVMAX, 0);
        SetWindowUINT(hwnd, WD_PREVLVL, 0);

        // create a red pen for the max line and store this
        SetWindowUINTPtr(hwnd, WD_PEN,
                (UINT_PTR) CreatePen(PS_SOLID, 2, RGB(255, 0, 0)));

        break;

    case WM_DESTROY:
        // destroy the pen we created
        {
            HPEN hpen = (HPEN) GetWindowUINTPtr(hwnd, WD_PEN);
            if (hpen) {
                DeleteObject(hpen);
                SetWindowUINTPtr(hwnd, WD_PEN, 0);
            }

            // also kill the timer we created
            KillTimer(hwnd, 0);
        }
        break;

    case WM_PAINT:
        /*
         * paint the entire control
         *
         * nb we must paint exactly as it is currently drawn because we
         * may be clipped to only part of the control. Thus we must draw
         * the max at WD_PREVMAX as it is currently drawn, since WD_MAX
         * may have been set to 0 and not yet drawn - in this case, with
         * some unfortunate timing and clipping, we would have two max lines.
         */
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rc, rcFill;
            HPEN hpenOld, hpen;

            hdc = BeginPaint(hwnd, &ps);

            GetClientRect(hwnd, &rc);

            // treat the level as a percentage and fill that much of the
            // control with black (from left)
            rcFill = rc;
            rcFill.right = (rc.right * GetWindowUINT(hwnd, WD_PREVLVL)) / 100;
            SetBkColor(hdc, RGB(0,0,0));
            // easy way to fill without creating a brush
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);

            // draw the max line
            rcFill.right = (rc.right * GetWindowUINT(hwnd, WD_PREVLVL)) / 100;
            hpen = (HPEN) GetWindowUINTPtr(hwnd, WD_PEN);
            hpenOld = SelectObject(hdc, hpen);
            MoveToEx(hdc, rcFill.right, rcFill.top, NULL);
            LineTo(hdc, rcFill.right, rcFill.bottom);
            SelectObject(hdc, hpenOld);

            EndPaint(hwnd, &ps);

        }
        break;

    case WMRL_SETLEVEL:
        // set new level, and update the displayed level block and max line
        {
            RECT rc, rcFill;
            UINT uMax, uPrevMax, uPrevLevel, uLevel;
            HDC hdc;

            // new level is lParam
            uLevel = (UINT) lParam;

            // fetch other parameters
            uMax = GetWindowUINT(hwnd, WD_MAX);
            uPrevMax = GetWindowUINT(hwnd, WD_PREVMAX);
            uPrevLevel = GetWindowUINT(hwnd, WD_PREVLVL);


            // decay the max level. This rate works best if we are called
            // to update every 1/20th sec - in this case the decay will be
            // 64% in a second.
            if (uMax > 0) {
                uMax = (uMax * 2007) / 2048;     // = 0.98 * uMax
            }

            hdc = GetDC(hwnd);

            GetClientRect(hwnd, &rc);
            rcFill = rc;

            // is the current level a new peak ?
            if (uLevel > uMax) {
                uMax = uLevel;
            }

            SetWindowUINT(hwnd, WD_MAX, uMax);

            // if the max has moved, erase the old line
            if (uMax != uPrevMax) {
                // white out the line by filling a 2-pixel wide rect
                rcFill.right = ((rc.right * uPrevMax) / 100) + 1;
                rcFill.left = rcFill.right - 2;
                SetBkColor(hdc, RGB(255, 255, 255));
                ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);
            }

            // calculate the area to update
            rcFill.right = (rc.right * uPrevLevel) / 100;
            rcFill.left = (rc.right * uLevel) / 100;

            // are we erasing (lowering level) or drawing more black?
            if (rcFill.right > rcFill.left) {

                // level has dropped - so fill with white down to new level
                SetBkColor(hdc, RGB(255, 255, 255));
            } else {
                // level has gone up so fill with black up to new level
                int t;

                t = rcFill.right;
                rcFill.right = rcFill.left;
                rcFill.left = t;

                SetBkColor(hdc, RGB(0, 0, 0));

                // fill a little extra to ensure no rounding gaps
                if (rcFill.left > 0) {
                    rcFill.left -= 1;
                }
            }
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);
            SetWindowUINT(hwnd, WD_PREVLVL, uLevel);

            // draw the new max line if needed
            if (uMax != uPrevMax) {
                HPEN hpen, hpenOld;

                rcFill.right = (rc.right * uMax) /100;

                hpen = (HPEN) GetWindowUINTPtr(hwnd, WD_PEN);
                hpenOld = SelectObject(hdc, hpen);
                MoveToEx(hdc, rcFill.right, rcFill.top, NULL);
                LineTo(hdc, rcFill.right, rcFill.bottom);
                SelectObject(hdc, hpenOld);

                SetWindowUINT(hwnd, WD_PREVMAX, uMax);
            }
            ReleaseDC(hwnd, hdc);
            return(0);
        }

    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}



