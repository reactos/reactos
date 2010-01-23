/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/mouse.c
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "globalvar.h"
#include "dib.h"
#include "drawing.h"
#include "history.h"

/* FUNCTIONS ********************************************************/

void
placeSelWin()
{
    MoveWindow(hSelection, rectSel_dest[0] * zoom / 1000, rectSel_dest[1] * zoom / 1000,
               rectSel_dest[2] * zoom / 1000 + 6, rectSel_dest[3] * zoom / 1000 + 6, TRUE);
    BringWindowToTop(hSelection);
    SendMessage(hImageArea, WM_PAINT, 0, 0);
    //SendMessage(hSelection, WM_PAINT, 0, 0);
}

POINT pointStack[256];
short pointSP;
POINT *ptStack = NULL;
int ptSP = 0;

void
startPaintingL(HDC hdc, short x, short y, int fg, int bg)
{
    startX = x;
    startY = y;
    lastX = x;
    lastY = y;
    switch (activeTool)
    {
        case 1:
            ShowWindow(hSelection, SW_HIDE);
            if (ptStack != NULL)
                HeapFree(GetProcessHeap(), 0, ptStack);
            ptStack = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(POINT) * 1024);
            ptSP = 0;
            ptStack[0].x = x;
            ptStack[0].y = y;
            break;
        case 10:
        case 11:
        case 13:
        case 15:
        case 16:
            newReversible();
            break;
        case 2:
            newReversible();
            ShowWindow(hSelection, SW_HIDE);
            rectSel_src[2] = rectSel_src[3] = 0;
            break;
        case 3:
            newReversible();
            Erase(hdc, x, y, x, y, bg, rubberRadius);
            break;
        case 4:
            newReversible();
            Fill(hdc, x, y, fg);
            break;
        case 7:
            newReversible();
            SetPixel(hdc, x, y, fg);
            break;
        case 8:
            newReversible();
            Brush(hdc, x, y, x, y, fg, brushStyle);
            break;
        case 9:
            newReversible();
            Airbrush(hdc, x, y, fg, airBrushWidth);
            break;
        case 12:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
        case 14:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, fg, bg, lineWidth, shapeStyle, FALSE);
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
    }
}

void
whilePaintingL(HDC hdc, short x, short y, int fg, int bg)
{
    switch (activeTool)
    {
        case 1:
            if (ptSP == 0)
                newReversible();
            ptSP++;
            if (ptSP % 1024 == 0)
                ptStack = HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, ptStack, sizeof(POINT) * (ptSP + 1024));
            ptStack[ptSP].x = max(0, min(x, imgXRes));
            ptStack[ptSP].y = max(0, min(y, imgYRes));
            resetToU1();
            Poly(hdc, ptStack, ptSP + 1, 0, 0, 2, 0, FALSE);
            break;
        case 2:
        {
            short tempX;
            short tempY;
            resetToU1();
            tempX = max(0, min(x, imgXRes));
            tempY = max(0, min(y, imgYRes));
            rectSel_dest[0] = rectSel_src[0] = min(startX, tempX);
            rectSel_dest[1] = rectSel_src[1] = min(startY, tempY);
            rectSel_dest[2] = rectSel_src[2] = max(startX, tempX) - min(startX, tempX);
            rectSel_dest[3] = rectSel_src[3] = max(startY, tempY) - min(startY, tempY);
            RectSel(hdc, startX, startY, tempX, tempY);
            break;
        }
        case 3:
            Erase(hdc, lastX, lastY, x, y, bg, rubberRadius);
            break;
        case 7:
            Line(hdc, lastX, lastY, x, y, fg, 1);
            break;
        case 8:
            Brush(hdc, lastX, lastY, x, y, fg, brushStyle);
            break;
        case 9:
            Airbrush(hdc, x, y, fg, airBrushWidth);
            break;
        case 11:
            resetToU1();
            Line(hdc, startX, startY, x, y, fg, lineWidth);
            break;
        case 12:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            switch (pointSP)
            {
                case 1:
                    Line(hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, fg,
                         lineWidth);
                    break;
                case 2:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], fg, lineWidth);
                    break;
                case 3:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], fg, lineWidth);
                    break;
            }
            break;
        case 13:
            resetToU1();
            Rect(hdc, startX, startY, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case 14:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, fg, bg, lineWidth, shapeStyle, FALSE);
            break;
        case 15:
            resetToU1();
            Ellp(hdc, startX, startY, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case 16:
            resetToU1();
            RRect(hdc, startX, startY, x, y, fg, bg, lineWidth, shapeStyle);
            break;
    }

    lastX = x;
    lastY = y;
}

void
endPaintingL(HDC hdc, short x, short y, int fg, int bg)
{
    switch (activeTool)
    {
        case 1:
        {
            POINT *ptStackCopy;
            int i;
            rectSel_src[0] = rectSel_src[1] = 0x7fffffff;
            rectSel_src[2] = rectSel_src[3] = 0;
            for (i = 0; i <= ptSP; i++)
            {
                if (ptStack[i].x < rectSel_src[0])
                    rectSel_src[0] = ptStack[i].x;
                if (ptStack[i].y < rectSel_src[1])
                    rectSel_src[1] = ptStack[i].y;
                if (ptStack[i].x > rectSel_src[2])
                    rectSel_src[2] = ptStack[i].x;
                if (ptStack[i].y > rectSel_src[3])
                    rectSel_src[3] = ptStack[i].y;
            }
            rectSel_src[2] += 1 - rectSel_src[0];
            rectSel_src[3] += 1 - rectSel_src[1];
            rectSel_dest[0] = rectSel_src[0];
            rectSel_dest[1] = rectSel_src[1];
            rectSel_dest[2] = rectSel_src[2];
            rectSel_dest[3] = rectSel_src[3];
            if (ptSP != 0)
            {
                DeleteObject(hSelMask);
                hSelMask = CreateBitmap(rectSel_src[2], rectSel_src[3], 1, 1, NULL);
                DeleteObject(SelectObject(hSelDC, hSelMask));
                ptStackCopy = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(POINT) * (ptSP + 1));
                for (i = 0; i <= ptSP; i++)
                {
                    ptStackCopy[i].x = ptStack[i].x - rectSel_src[0];
                    ptStackCopy[i].y = ptStack[i].y - rectSel_src[1];
                }
                Poly(hSelDC, ptStackCopy, ptSP + 1, 0x00ffffff, 0x00ffffff, 1, 2, TRUE);
                HeapFree(GetProcessHeap(), 0, ptStackCopy);
                SelectObject(hSelDC, hSelBm = CreateDIBWithProperties(rectSel_src[2], rectSel_src[3]));
                resetToU1();
                MaskBlt(hSelDC, 0, 0, rectSel_src[2], rectSel_src[3], hDrawingDC, rectSel_src[0],
                        rectSel_src[1], hSelMask, 0, 0, MAKEROP4(SRCCOPY, WHITENESS));
                Poly(hdc, ptStack, ptSP + 1, bg, bg, 1, 2, TRUE);
                newReversible();

                placeSelWin();
                ShowWindow(hSelection, SW_SHOW);
            }
            HeapFree(GetProcessHeap(), 0, ptStack);
            ptStack = NULL;
            break;
        }
        case 2:
            resetToU1();
            if ((rectSel_src[2] != 0) && (rectSel_src[3] != 0))
            {
                DeleteObject(SelectObject
                             (hSelDC, hSelBm =
                              (HBITMAP) CreateDIBWithProperties(rectSel_src[2], rectSel_src[3])));
                DeleteObject(hSelMask);
                BitBlt(hSelDC, 0, 0, rectSel_src[2], rectSel_src[3], hDrawingDC, rectSel_src[0],
                       rectSel_src[1], SRCCOPY);
                Rect(hdc, rectSel_src[0], rectSel_src[1], rectSel_src[0] + rectSel_src[2],
                     rectSel_src[1] + rectSel_src[3], bgColor, bgColor, 0, TRUE);
                newReversible();
                placeSelWin();
                ShowWindow(hSelection, SW_SHOW);
            }
            break;
        case 3:
            Erase(hdc, lastX, lastY, x, y, bg, rubberRadius);
            break;
        case 7:
            Line(hdc, lastX, lastY, x, y, fg, 1);
            SetPixel(hdc, x, y, fg);
            break;
        case 11:
            resetToU1();
            Line(hdc, startX, startY, x, y, fg, lineWidth);
            break;
        case 12:
            pointSP++;
            if (pointSP == 4)
                pointSP = 0;
            break;
        case 13:
            resetToU1();
            Rect(hdc, startX, startY, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case 14:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            pointSP++;
            if (pointSP >= 2)
            {
                if ((pointStack[0].x - x) * (pointStack[0].x - x) +
                    (pointStack[0].y - y) * (pointStack[0].y - y) <= lineWidth * lineWidth + 1)
                {
                    Poly(hdc, pointStack, pointSP, fg, bg, lineWidth, shapeStyle, TRUE);
                    pointSP = 0;
                }
                else
                {
                    Poly(hdc, pointStack, pointSP, fg, bg, lineWidth, shapeStyle, FALSE);
                }
            }
            if (pointSP == 255)
                pointSP--;
            break;
        case 15:
            resetToU1();
            Ellp(hdc, startX, startY, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case 16:
            resetToU1();
            RRect(hdc, startX, startY, x, y, fg, bg, lineWidth, shapeStyle);
            break;
    }
}

void
startPaintingR(HDC hdc, short x, short y, int fg, int bg)
{
    startX = x;
    startY = y;
    lastX = x;
    lastY = y;
    switch (activeTool)
    {
        case 1:
        case 10:
        case 11:
        case 13:
        case 15:
        case 16:
            newReversible();
            break;
        case 3:
            newReversible();
            Replace(hdc, x, y, x, y, fg, bg, rubberRadius);
            break;
        case 4:
            newReversible();
            Fill(hdc, x, y, bg);
            break;
        case 7:
            newReversible();
            SetPixel(hdc, x, y, bg);
            break;
        case 8:
            newReversible();
            Brush(hdc, x, y, x, y, bg, brushStyle);
            break;
        case 9:
            newReversible();
            Airbrush(hdc, x, y, bg, airBrushWidth);
            break;
        case 12:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
        case 14:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, bg, fg, lineWidth, shapeStyle, FALSE);
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
    }
}

void
whilePaintingR(HDC hdc, short x, short y, int fg, int bg)
{
    switch (activeTool)
    {
        case 3:
            Replace(hdc, lastX, lastY, x, y, fg, bg, rubberRadius);
            break;
        case 7:
            Line(hdc, lastX, lastY, x, y, bg, 1);
            break;
        case 8:
            Brush(hdc, lastX, lastY, x, y, bg, brushStyle);
            break;
        case 9:
            Airbrush(hdc, x, y, bg, airBrushWidth);
            break;
        case 11:
            resetToU1();
            Line(hdc, startX, startY, x, y, bg, lineWidth);
            break;
        case 12:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            switch (pointSP)
            {
                case 1:
                    Line(hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, bg,
                         lineWidth);
                    break;
                case 2:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], bg, lineWidth);
                    break;
                case 3:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], bg, lineWidth);
                    break;
            }
            break;
        case 13:
            resetToU1();
            Rect(hdc, startX, startY, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case 14:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, bg, fg, lineWidth, shapeStyle, FALSE);
            break;
        case 15:
            resetToU1();
            Ellp(hdc, startX, startY, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case 16:
            resetToU1();
            RRect(hdc, startX, startY, x, y, bg, fg, lineWidth, shapeStyle);
            break;
    }

    lastX = x;
    lastY = y;
}

void
endPaintingR(HDC hdc, short x, short y, int fg, int bg)
{
    switch (activeTool)
    {
        case 3:
            Replace(hdc, lastX, lastY, x, y, fg, bg, rubberRadius);
            break;
        case 7:
            Line(hdc, lastX, lastY, x, y, bg, 1);
            SetPixel(hdc, x, y, bg);
            break;
        case 11:
            resetToU1();
            Line(hdc, startX, startY, x, y, bg, lineWidth);
            break;
        case 12:
            pointSP++;
            if (pointSP == 4)
                pointSP = 0;
            break;
        case 13:
            resetToU1();
            Rect(hdc, startX, startY, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case 14:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            pointSP++;
            if (pointSP >= 2)
            {
                if ((pointStack[0].x - x) * (pointStack[0].x - x) +
                    (pointStack[0].y - y) * (pointStack[0].y - y) <= lineWidth * lineWidth + 1)
                {
                    Poly(hdc, pointStack, pointSP, bg, fg, lineWidth, shapeStyle, TRUE);
                    pointSP = 0;
                }
                else
                {
                    Poly(hdc, pointStack, pointSP, bg, fg, lineWidth, shapeStyle, FALSE);
                }
            }
            if (pointSP == 255)
                pointSP--;
            break;
        case 15:
            resetToU1();
            Ellp(hdc, startX, startY, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case 16:
            resetToU1();
            RRect(hdc, startX, startY, x, y, bg, fg, lineWidth, shapeStyle);
            break;
    }
}
