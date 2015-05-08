/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/mouse.c
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void
placeSelWin()
{
    MoveWindow(hSelection, rectSel_dest.left * zoom / 1000, rectSel_dest.top * zoom / 1000,
               RECT_WIDTH(rectSel_dest) * zoom / 1000 + 6, RECT_HEIGHT(rectSel_dest) * zoom / 1000 + 6, TRUE);
    BringWindowToTop(hSelection);
    InvalidateRect(hImageArea, NULL, FALSE);
}

void
regularize(LONG x0, LONG y0, LONG *x1, LONG *y1)
{
    if (abs(*x1 - x0) >= abs(*y1 - y0))
        *y1 = y0 + (*y1 > y0 ? abs(*x1 - x0) : -abs(*x1 - x0));
    else
        *x1 = x0 + (*x1 > x0 ? abs(*y1 - y0) : -abs(*y1 - y0));
}

void
roundTo8Directions(LONG x0, LONG y0, LONG *x1, LONG *y1)
{
    if (abs(*x1 - x0) >= abs(*y1 - y0))
    {
        if (abs(*y1 - y0) * 5 < abs(*x1 - x0) * 2)
            *y1 = y0;
        else
            *y1 = y0 + (*y1 > y0 ? abs(*x1 - x0) : -abs(*x1 - x0));
    }
    else
    {
        if (abs(*x1 - x0) * 5 < abs(*y1 - y0) * 2)
            *x1 = x0;
        else
            *x1 = x0 + (*x1 > x0 ? abs(*y1 - y0) : -abs(*y1 - y0));
    }
}

POINT pointStack[256];
short pointSP;
POINT *ptStack = NULL;
int ptSP = 0;

void
startPaintingL(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    start.x = x;
    start.y = y;
    last.x = x;
    last.y = y;
    switch (activeTool)
    {
        case TOOL_FREESEL:
            ShowWindow(hSelection, SW_HIDE);
            if (ptStack != NULL)
                HeapFree(GetProcessHeap(), 0, ptStack);
            ptStack = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(POINT) * 1024);
            ptSP = 0;
            ptStack[0].x = x;
            ptStack[0].y = y;
            break;
        case TOOL_LINE:
        case TOOL_RECT:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            newReversible();
            break;
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            newReversible();
            ShowWindow(hSelection, SW_HIDE);
            rectSel_src.right = rectSel_src.left;
            rectSel_src.bottom = rectSel_src.top;
            break;
        case TOOL_RUBBER:
            newReversible();
            Erase(hdc, x, y, x, y, bg, rubberRadius);
            break;
        case TOOL_FILL:
            newReversible();
            Fill(hdc, x, y, fg);
            break;
        case TOOL_PEN:
            newReversible();
            SetPixel(hdc, x, y, fg);
            break;
        case TOOL_BRUSH:
            newReversible();
            Brush(hdc, x, y, x, y, fg, brushStyle);
            break;
        case TOOL_AIRBRUSH:
            newReversible();
            Airbrush(hdc, x, y, fg, airBrushWidth);
            break;
        case TOOL_BEZIER:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
        case TOOL_SHAPE:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, fg, bg, lineWidth, shapeStyle, FALSE, FALSE);
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
    }
}

void
whilePaintingL(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (activeTool)
    {
        case TOOL_FREESEL:
            if (ptSP == 0)
                newReversible();
            ptSP++;
            if (ptSP % 1024 == 0)
                ptStack = HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, ptStack, sizeof(POINT) * (ptSP + 1024));
            ptStack[ptSP].x = max(0, min(x, imgXRes));
            ptStack[ptSP].y = max(0, min(y, imgYRes));
            resetToU1();
            Poly(hdc, ptStack, ptSP + 1, 0, 0, 2, 0, FALSE, TRUE); /* draw the freehand selection inverted/xored */
            break;
        case TOOL_RECTSEL:
        case TOOL_TEXT:
        {
            POINT temp;
            resetToU1();
            temp.x = max(0, min(x, imgXRes));
            temp.y = max(0, min(y, imgYRes));
            rectSel_dest.left = rectSel_src.left = min(start.x, temp.x);
            rectSel_dest.top = rectSel_src.top = min(start.y, temp.y);
            rectSel_dest.right = rectSel_src.right = max(start.x, temp.x);
            rectSel_dest.bottom = rectSel_src.bottom = max(start.y, temp.y);
            RectSel(hdc, start.x, start.y, temp.x, temp.y);
            break;
        }
        case TOOL_RUBBER:
            Erase(hdc, last.x, last.y, x, y, bg, rubberRadius);
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, fg, 1);
            break;
        case TOOL_BRUSH:
            Brush(hdc, last.x, last.y, x, y, fg, brushStyle);
            break;
        case TOOL_AIRBRUSH:
            Airbrush(hdc, x, y, fg, airBrushWidth);
            break;
        case TOOL_LINE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, &x, &y);
            Line(hdc, start.x, start.y, x, y, fg, lineWidth);
            break;
        case TOOL_BEZIER:
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
        case TOOL_RECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Rect(hdc, start.x, start.y, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case TOOL_SHAPE:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   &pointStack[pointSP].x, &pointStack[pointSP].y);
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, fg, bg, lineWidth, shapeStyle, FALSE, FALSE);
            break;
        case TOOL_ELLIPSE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Ellp(hdc, start.x, start.y, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case TOOL_RRECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            RRect(hdc, start.x, start.y, x, y, fg, bg, lineWidth, shapeStyle);
            break;
    }

    last.x = x;
    last.y = y;
}

void
endPaintingL(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (activeTool)
    {
        case TOOL_FREESEL:
        {
            POINT *ptStackCopy;
            int i;
            rectSel_src.left = rectSel_src.top = MAXLONG;
            rectSel_src.right = rectSel_src.bottom = 0;
            for (i = 0; i <= ptSP; i++)
            {
                if (ptStack[i].x < rectSel_src.left)
                    rectSel_src.left = ptStack[i].x;
                if (ptStack[i].y < rectSel_src.top)
                    rectSel_src.top = ptStack[i].y;
                if (ptStack[i].x > rectSel_src.right)
                    rectSel_src.right = ptStack[i].x;
                if (ptStack[i].y > rectSel_src.bottom)
                    rectSel_src.bottom = ptStack[i].y;
            }
            rectSel_src.right  += 1;
            rectSel_src.bottom += 1;
            rectSel_dest.left   = rectSel_src.left;
            rectSel_dest.top    = rectSel_src.top;
            rectSel_dest.right  = rectSel_src.right;
            rectSel_dest.bottom = rectSel_src.bottom;
            if (ptSP != 0)
            {
                DeleteObject(hSelMask);
                hSelMask = CreateBitmap(RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), 1, 1, NULL);
                DeleteObject(SelectObject(hSelDC, hSelMask));
                ptStackCopy = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(POINT) * (ptSP + 1));
                for (i = 0; i <= ptSP; i++)
                {
                    ptStackCopy[i].x = ptStack[i].x - rectSel_src.left;
                    ptStackCopy[i].y = ptStack[i].y - rectSel_src.top;
                }
                Poly(hSelDC, ptStackCopy, ptSP + 1, 0x00ffffff, 0x00ffffff, 1, 2, TRUE, FALSE);
                HeapFree(GetProcessHeap(), 0, ptStackCopy);
                SelectObject(hSelDC, hSelBm = CreateDIBWithProperties(RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src)));
                resetToU1();
                MaskBlt(hSelDC, 0, 0, RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), hDrawingDC, rectSel_src.left,
                        rectSel_src.top, hSelMask, 0, 0, MAKEROP4(SRCCOPY, WHITENESS));
                Poly(hdc, ptStack, ptSP + 1, bg, bg, 1, 2, TRUE, FALSE);
                newReversible();

                MaskBlt(hDrawingDC, rectSel_src.left, rectSel_src.top, RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), hSelDC, 0,
                        0, hSelMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND));

                placeSelWin();
                ShowWindow(hSelection, SW_SHOW);
                /* force refresh of selection contents */
                SendMessage(hSelection, WM_LBUTTONDOWN, 0, 0);
                SendMessage(hSelection, WM_MOUSEMOVE, 0, 0);
                SendMessage(hSelection, WM_LBUTTONUP, 0, 0);
            }
            HeapFree(GetProcessHeap(), 0, ptStack);
            ptStack = NULL;
            break;
        }
        case TOOL_RECTSEL:
            resetToU1();
            if ((RECT_WIDTH(rectSel_src) != 0) && (RECT_HEIGHT(rectSel_src) != 0))
            {
                DeleteObject(hSelMask);
                hSelMask = CreateBitmap(RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), 1, 1, NULL);
                DeleteObject(SelectObject(hSelDC, hSelMask));
                Rect(hSelDC, 0, 0, RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), 0x00ffffff, 0x00ffffff, 1, 2);
                SelectObject(hSelDC, hSelBm = CreateDIBWithProperties(RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src)));
                resetToU1();
                BitBlt(hSelDC, 0, 0, RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), hDrawingDC, rectSel_src.left,
                       rectSel_src.top, SRCCOPY);
                Rect(hdc, rectSel_src.left, rectSel_src.top, rectSel_src.right,
                     rectSel_src.bottom, bgColor, bgColor, 0, TRUE);
                newReversible();

                BitBlt(hDrawingDC, rectSel_src.left, rectSel_src.top, RECT_WIDTH(rectSel_src), RECT_HEIGHT(rectSel_src), hSelDC, 0,
                       0, SRCCOPY);

                placeSelWin();
                ShowWindow(hSelection, SW_SHOW);
                ForceRefreshSelectionContents();
            }
            break;
        case TOOL_TEXT:
            resetToU1();
            if ((RECT_WIDTH(rectSel_src) != 0) && (RECT_HEIGHT(rectSel_src) != 0))
            {
                newReversible();

                placeSelWin();
                ShowWindow(hSelection, SW_SHOW);
                ForceRefreshSelectionContents();
            }
            break;
        case TOOL_RUBBER:
            Erase(hdc, last.x, last.y, x, y, bg, rubberRadius);
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, fg, 1);
            SetPixel(hdc, x, y, fg);
            break;
        case TOOL_LINE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, &x, &y);
            Line(hdc, start.x, start.y, x, y, fg, lineWidth);
            break;
        case TOOL_BEZIER:
            pointSP++;
            if (pointSP == 4)
                pointSP = 0;
            break;
        case TOOL_RECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Rect(hdc, start.x, start.y, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case TOOL_SHAPE:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   &pointStack[pointSP].x, &pointStack[pointSP].y);
            pointSP++;
            if (pointSP >= 2)
            {
                if ((pointStack[0].x - x) * (pointStack[0].x - x) +
                    (pointStack[0].y - y) * (pointStack[0].y - y) <= lineWidth * lineWidth + 1)
                {
                    Poly(hdc, pointStack, pointSP, fg, bg, lineWidth, shapeStyle, TRUE, FALSE);
                    pointSP = 0;
                }
                else
                {
                    Poly(hdc, pointStack, pointSP, fg, bg, lineWidth, shapeStyle, FALSE, FALSE);
                }
            }
            if (pointSP == 255)
                pointSP--;
            break;
        case TOOL_ELLIPSE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Ellp(hdc, start.x, start.y, x, y, fg, bg, lineWidth, shapeStyle);
            break;
        case TOOL_RRECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            RRect(hdc, start.x, start.y, x, y, fg, bg, lineWidth, shapeStyle);
            break;
    }
}

void
startPaintingR(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    start.x = x;
    start.y = y;
    last.x = x;
    last.y = y;
    switch (activeTool)
    {
        case TOOL_FREESEL:
        case TOOL_TEXT:
        case TOOL_LINE:
        case TOOL_RECT:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            newReversible();
            break;
        case TOOL_RUBBER:
            newReversible();
            Replace(hdc, x, y, x, y, fg, bg, rubberRadius);
            break;
        case TOOL_FILL:
            newReversible();
            Fill(hdc, x, y, bg);
            break;
        case TOOL_PEN:
            newReversible();
            SetPixel(hdc, x, y, bg);
            break;
        case TOOL_BRUSH:
            newReversible();
            Brush(hdc, x, y, x, y, bg, brushStyle);
            break;
        case TOOL_AIRBRUSH:
            newReversible();
            Airbrush(hdc, x, y, bg, airBrushWidth);
            break;
        case TOOL_BEZIER:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
        case TOOL_SHAPE:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, bg, fg, lineWidth, shapeStyle, FALSE, FALSE);
            if (pointSP == 0)
            {
                newReversible();
                pointSP++;
            }
            break;
    }
}

void
whilePaintingR(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (activeTool)
    {
        case TOOL_RUBBER:
            Replace(hdc, last.x, last.y, x, y, fg, bg, rubberRadius);
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, bg, 1);
            break;
        case TOOL_BRUSH:
            Brush(hdc, last.x, last.y, x, y, bg, brushStyle);
            break;
        case TOOL_AIRBRUSH:
            Airbrush(hdc, x, y, bg, airBrushWidth);
            break;
        case TOOL_LINE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, &x, &y);
            Line(hdc, start.x, start.y, x, y, bg, lineWidth);
            break;
        case TOOL_BEZIER:
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
        case TOOL_RECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Rect(hdc, start.x, start.y, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case TOOL_SHAPE:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   &pointStack[pointSP].x, &pointStack[pointSP].y);
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, bg, fg, lineWidth, shapeStyle, FALSE, FALSE);
            break;
        case TOOL_ELLIPSE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Ellp(hdc, start.x, start.y, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case TOOL_RRECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            RRect(hdc, start.x, start.y, x, y, bg, fg, lineWidth, shapeStyle);
            break;
    }

    last.x = x;
    last.y = y;
}

void
endPaintingR(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (activeTool)
    {
        case TOOL_RUBBER:
            Replace(hdc, last.x, last.y, x, y, fg, bg, rubberRadius);
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, bg, 1);
            SetPixel(hdc, x, y, bg);
            break;
        case TOOL_LINE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, &x, &y);
            Line(hdc, start.x, start.y, x, y, bg, lineWidth);
            break;
        case TOOL_BEZIER:
            pointSP++;
            if (pointSP == 4)
                pointSP = 0;
            break;
        case TOOL_RECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Rect(hdc, start.x, start.y, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case TOOL_SHAPE:
            resetToU1();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   &pointStack[pointSP].x, &pointStack[pointSP].y);
            pointSP++;
            if (pointSP >= 2)
            {
                if ((pointStack[0].x - x) * (pointStack[0].x - x) +
                    (pointStack[0].y - y) * (pointStack[0].y - y) <= lineWidth * lineWidth + 1)
                {
                    Poly(hdc, pointStack, pointSP, bg, fg, lineWidth, shapeStyle, TRUE, FALSE);
                    pointSP = 0;
                }
                else
                {
                    Poly(hdc, pointStack, pointSP, bg, fg, lineWidth, shapeStyle, FALSE, FALSE);
                }
            }
            if (pointSP == 255)
                pointSP--;
            break;
        case TOOL_ELLIPSE:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            Ellp(hdc, start.x, start.y, x, y, bg, fg, lineWidth, shapeStyle);
            break;
        case TOOL_RRECT:
            resetToU1();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, &x, &y);
            RRect(hdc, start.x, start.y, x, y, bg, fg, lineWidth, shapeStyle);
            break;
    }
}
