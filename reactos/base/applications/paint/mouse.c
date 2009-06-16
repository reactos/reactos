/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        mouse.c
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "globalvar.h"
#include "dib.h"
#include "drawing.h"
#include "history.h"

/* FUNCTIONS ********************************************************/

void placeSelWin()
{
    MoveWindow(hSelection, rectSel_dest[0]*zoom/1000, rectSel_dest[1]*zoom/1000, rectSel_dest[2]*zoom/1000+6, rectSel_dest[3]*zoom/1000+6, TRUE);
    BringWindowToTop(hSelection);
    SendMessage(hImageArea, WM_PAINT, 0, 0);
    //SendMessage(hSelection, WM_PAINT, 0, 0);
}

void startPainting(HDC hdc, short x, short y, int fg, int bg)
{
    startX = x;
    startY = y;
    lastX = x;
    lastY = y;
    if ((activeTool!=5)&&(activeTool!=6)) newReversible();
    switch (activeTool)
    {
        case 2:
            ShowWindow(hSelection, SW_HIDE);
            break;
        case 3:
            Erase(hdc, x, y, x, y, bg, rubberRadius);
            break;
        case 4:
            Fill(hdc, x, y, fg);
            break;
        case 7:
            SetPixel(hdc, x, y, fg);
            break;
        case 8:
            Brush(hdc, x, y, x, y, fg, brushStyle);
            break;
        case 9:
            Airbrush(hdc, x, y, fg, airBrushWidth);
            break;
    }
}

void whilePainting(HDC hdc, short x, short y, int fg, int bg)
{
    switch (activeTool)
    {
        case 2:
            {
                short tempX;
                short tempY;
                resetToU1();
                tempX = max(0, min(x, imgXRes));
                tempY = max(0, min(y, imgYRes));
                rectSel_dest[0] = rectSel_src[0] = min(startX, tempX);
                rectSel_dest[1] = rectSel_src[1] = min(startY, tempY);
                rectSel_dest[2] = rectSel_src[2] = max(startX, tempX)-min(startX, tempX);
                rectSel_dest[3] = rectSel_src[3] = max(startY, tempY)-min(startY, tempY);
                RectSel(hdc, startX, startY, tempX, tempY);
            }
            break;
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
        case 13:
            resetToU1();
            switch (shapeStyle)
            {
                case 0:
                    Rect(hdc, startX, startY, x, y, fg, bg, lineWidth, FALSE);
                    break;
                case 1:
                    Rect(hdc, startX, startY, x, y, fg, bg, lineWidth, TRUE);
                    break;
                case 2:
                    Rect(hdc, startX, startY, x, y, fg, fg, lineWidth, TRUE);
                    break;
            }
            break;
        case 15:
            resetToU1();
            switch (shapeStyle)
            {
                case 0:
                    Ellp(hdc, startX, startY, x, y, fg, bg, lineWidth, FALSE);
                    break;
                case 1:
                    Ellp(hdc, startX, startY, x, y, fg, bg, lineWidth, TRUE);
                    break;
                case 2:
                    Ellp(hdc, startX, startY, x, y, fg, fg, lineWidth, TRUE);
                    break;
            }
            break;
        case 16:
            resetToU1();
            switch (shapeStyle)
            {
                case 0:
                    RRect(hdc, startX, startY, x, y, fg, bg, lineWidth, FALSE);
                    break;
                case 1:
                    RRect(hdc, startX, startY, x, y, fg, bg, lineWidth, TRUE);
                    break;
                case 2:
                    RRect(hdc, startX, startY, x, y, fg, fg, lineWidth, TRUE);
                    break;
            }
            break;
    }
    
    lastX = x;
    lastY = y;
}

void endPainting(HDC hdc, short x, short y, int fg, int bg)
{
    switch (activeTool)
    {
        case 2:
            resetToU1();
            if ((rectSel_src[2]!=0)&&(rectSel_src[3]!=0))
            {
                DeleteObject(SelectObject(hSelDC, hSelBm = (HBITMAP)CreateDIBWithProperties(rectSel_src[2], rectSel_src[3])));
                BitBlt(hSelDC, 0, 0, rectSel_src[2], rectSel_src[3], hDrawingDC, rectSel_src[0], rectSel_src[1], SRCCOPY);
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
        case 13:
            resetToU1();
            switch (shapeStyle)
            {
                case 0:
                    Rect(hdc, startX, startY, x, y, fg, bg, lineWidth, FALSE);
                    break;
                case 1:
                    Rect(hdc, startX, startY, x, y, fg, bg, lineWidth, TRUE);
                    break;
                case 2:
                    Rect(hdc, startX, startY, x, y, fg, fg, lineWidth, TRUE);
                    break;
            }
            break;
        case 15:
            resetToU1();
            switch (shapeStyle)
            {
                case 0:
                    Ellp(hdc, startX, startY, x, y, fg, bg, lineWidth, FALSE);
                    break;
                case 1:
                    Ellp(hdc, startX, startY, x, y, fg, bg, lineWidth, TRUE);
                    break;
                case 2:
                    Ellp(hdc, startX, startY, x, y, fg, fg, lineWidth, TRUE);
                    break;
            }
            break;
        case 16:
            resetToU1();
            switch (shapeStyle)
            {
                case 0:
                    RRect(hdc, startX, startY, x, y, fg, bg, lineWidth, FALSE);
                    break;
                case 1:
                    RRect(hdc, startX, startY, x, y, fg, bg, lineWidth, TRUE);
                    break;
                case 2:
                    RRect(hdc, startX, startY, x, y, fg, fg, lineWidth, TRUE);
                    break;
            }
            break;
    }
}
