/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/mouse.cpp
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void
placeSelWin()
{
    selectionWindow.MoveWindow(selectionModel.GetDestRectLeft() * toolsModel.GetZoom() / 1000, selectionModel.GetDestRectTop() * toolsModel.GetZoom() / 1000,
        selectionModel.GetDestRectWidth() * toolsModel.GetZoom() / 1000 + 6, selectionModel.GetDestRectHeight() * toolsModel.GetZoom() / 1000 + 6, TRUE);
    selectionWindow.BringWindowToTop();
    imageArea.InvalidateRect(NULL, FALSE);
}

void
regularize(LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    if (abs(x1 - x0) >= abs(y1 - y0))
        y1 = y0 + (y1 > y0 ? abs(x1 - x0) : -abs(x1 - x0));
    else
        x1 = x0 + (x1 > x0 ? abs(y1 - y0) : -abs(y1 - y0));
}

void
roundTo8Directions(LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    if (abs(x1 - x0) >= abs(y1 - y0))
    {
        if (abs(y1 - y0) * 5 < abs(x1 - x0) * 2)
            y1 = y0;
        else
            y1 = y0 + (y1 > y0 ? abs(x1 - x0) : -abs(x1 - x0));
    }
    else
    {
        if (abs(x1 - x0) * 5 < abs(y1 - y0) * 2)
            x1 = x0;
        else
            x1 = x0 + (x1 > x0 ? abs(y1 - y0) : -abs(y1 - y0));
    }
}

POINT pointStack[256];
short pointSP;

void
startPaintingL(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    start.x = x;
    start.y = y;
    last.x = x;
    last.y = y;
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
            selectionWindow.ShowWindow(SW_HIDE);
            selectionModel.ResetPtStack();
            selectionModel.PushToPtStack(x, y);
            break;
        case TOOL_LINE:
        case TOOL_RECT:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            imageModel.CopyPrevious();
            break;
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            imageModel.CopyPrevious();
            selectionWindow.ShowWindow(SW_HIDE);
            selectionModel.SetSrcRectSizeToZero();
            break;
        case TOOL_RUBBER:
            imageModel.CopyPrevious();
            Erase(hdc, x, y, x, y, bg, toolsModel.GetRubberRadius());
            break;
        case TOOL_FILL:
            imageModel.CopyPrevious();
            Fill(hdc, x, y, fg);
            break;
        case TOOL_PEN:
            imageModel.CopyPrevious();
            SetPixel(hdc, x, y, fg);
            break;
        case TOOL_BRUSH:
            imageModel.CopyPrevious();
            Brush(hdc, x, y, x, y, fg, toolsModel.GetBrushStyle());
            break;
        case TOOL_AIRBRUSH:
            imageModel.CopyPrevious();
            Airbrush(hdc, x, y, fg, toolsModel.GetAirBrushWidth());
            break;
        case TOOL_BEZIER:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP == 0)
            {
                imageModel.CopyPrevious();
                pointSP++;
            }
            break;
        case TOOL_SHAPE:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
            if (pointSP == 0)
            {
                imageModel.CopyPrevious();
                pointSP++;
            }
            break;
    }
}

void
whilePaintingL(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
            if (selectionModel.PtStackSize() == 1)
                imageModel.CopyPrevious();
            selectionModel.PushToPtStack(max(0, min(x, imageModel.GetWidth())), max(0, min(y, imageModel.GetHeight())));
            imageModel.ResetToPrevious();
            selectionModel.DrawFramePoly(hdc);
            break;
        case TOOL_RECTSEL:
        case TOOL_TEXT:
        {
            POINT temp;
            imageModel.ResetToPrevious();
            temp.x = max(0, min(x, imageModel.GetWidth()));
            temp.y = max(0, min(y, imageModel.GetHeight()));
            selectionModel.SetSrcAndDestRectFromPoints(start, temp);
            RectSel(hdc, start.x, start.y, temp.x, temp.y);
            break;
        }
        case TOOL_RUBBER:
            Erase(hdc, last.x, last.y, x, y, bg, toolsModel.GetRubberRadius());
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, fg, 1);
            break;
        case TOOL_BRUSH:
            Brush(hdc, last.x, last.y, x, y, fg, toolsModel.GetBrushStyle());
            break;
        case TOOL_AIRBRUSH:
            Airbrush(hdc, x, y, fg, toolsModel.GetAirBrushWidth());
            break;
        case TOOL_LINE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, x, y);
            Line(hdc, start.x, start.y, x, y, fg, toolsModel.GetLineWidth());
            break;
        case TOOL_BEZIER:
            imageModel.ResetToPrevious();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            switch (pointSP)
            {
                case 1:
                    Line(hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, fg,
                         toolsModel.GetLineWidth());
                    break;
                case 2:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], fg, toolsModel.GetLineWidth());
                    break;
                case 3:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], fg, toolsModel.GetLineWidth());
                    break;
            }
            break;
        case TOOL_RECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Rect(hdc, start.x, start.y, x, y, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_SHAPE:
            imageModel.ResetToPrevious();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   pointStack[pointSP].x, pointStack[pointSP].y);
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
            break;
        case TOOL_ELLIPSE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Ellp(hdc, start.x, start.y, x, y, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_RRECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            RRect(hdc, start.x, start.y, x, y, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
    }

    last.x = x;
    last.y = y;
}

void
endPaintingL(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        {
            selectionModel.CalculateBoundingBoxAndContents(hdc);
            if (selectionModel.PtStackSize() > 1)
            {
                selectionModel.DrawBackgroundPoly(hdc, bg);
                imageModel.CopyPrevious();

                selectionModel.DrawSelection(hdc);

                placeSelWin();
                selectionWindow.ShowWindow(SW_SHOW);
                ForceRefreshSelectionContents();
            }
            selectionModel.ResetPtStack();
            break;
        }
        case TOOL_RECTSEL:
            imageModel.ResetToPrevious();
            if (selectionModel.IsSrcRectSizeNonzero())
            {
                selectionModel.CalculateContents(hdc);
                selectionModel.DrawBackgroundRect(hdc, bg);
                imageModel.CopyPrevious();

                selectionModel.DrawSelection(hdc);

                placeSelWin();
                selectionWindow.ShowWindow(SW_SHOW);
                ForceRefreshSelectionContents();
            }
            break;
        case TOOL_TEXT:
            imageModel.ResetToPrevious();
            if (selectionModel.IsSrcRectSizeNonzero())
            {
                imageModel.CopyPrevious();

                placeSelWin();
                selectionWindow.ShowWindow(SW_SHOW);
                ForceRefreshSelectionContents();
            }
            break;
        case TOOL_RUBBER:
            Erase(hdc, last.x, last.y, x, y, bg, toolsModel.GetRubberRadius());
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, fg, 1);
            SetPixel(hdc, x, y, fg);
            break;
        case TOOL_LINE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, x, y);
            Line(hdc, start.x, start.y, x, y, fg, toolsModel.GetLineWidth());
            break;
        case TOOL_BEZIER:
            pointSP++;
            if (pointSP == 4)
                pointSP = 0;
            break;
        case TOOL_RECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Rect(hdc, start.x, start.y, x, y, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_SHAPE:
            imageModel.ResetToPrevious();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   pointStack[pointSP].x, pointStack[pointSP].y);
            pointSP++;
            if (pointSP >= 2)
            {
                if ((pointStack[0].x - x) * (pointStack[0].x - x) +
                    (pointStack[0].y - y) * (pointStack[0].y - y) <= toolsModel.GetLineWidth() * toolsModel.GetLineWidth() + 1)
                {
                    Poly(hdc, pointStack, pointSP, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), TRUE, FALSE);
                    pointSP = 0;
                }
                else
                {
                    Poly(hdc, pointStack, pointSP, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                }
            }
            if (pointSP == 255)
                pointSP--;
            break;
        case TOOL_ELLIPSE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Ellp(hdc, start.x, start.y, x, y, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_RRECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            RRect(hdc, start.x, start.y, x, y, fg, bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
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
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        case TOOL_TEXT:
        case TOOL_LINE:
        case TOOL_RECT:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            imageModel.CopyPrevious();
            break;
        case TOOL_RUBBER:
            imageModel.CopyPrevious();
            Replace(hdc, x, y, x, y, fg, bg, toolsModel.GetRubberRadius());
            break;
        case TOOL_FILL:
            imageModel.CopyPrevious();
            Fill(hdc, x, y, bg);
            break;
        case TOOL_PEN:
            imageModel.CopyPrevious();
            SetPixel(hdc, x, y, bg);
            break;
        case TOOL_BRUSH:
            imageModel.CopyPrevious();
            Brush(hdc, x, y, x, y, bg, toolsModel.GetBrushStyle());
            break;
        case TOOL_AIRBRUSH:
            imageModel.CopyPrevious();
            Airbrush(hdc, x, y, bg, toolsModel.GetAirBrushWidth());
            break;
        case TOOL_BEZIER:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP == 0)
            {
                imageModel.CopyPrevious();
                pointSP++;
            }
            break;
        case TOOL_SHAPE:
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
            if (pointSP == 0)
            {
                imageModel.CopyPrevious();
                pointSP++;
            }
            break;
    }
}

void
whilePaintingR(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_RUBBER:
            Replace(hdc, last.x, last.y, x, y, fg, bg, toolsModel.GetRubberRadius());
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, bg, 1);
            break;
        case TOOL_BRUSH:
            Brush(hdc, last.x, last.y, x, y, bg, toolsModel.GetBrushStyle());
            break;
        case TOOL_AIRBRUSH:
            Airbrush(hdc, x, y, bg, toolsModel.GetAirBrushWidth());
            break;
        case TOOL_LINE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, x, y);
            Line(hdc, start.x, start.y, x, y, bg, toolsModel.GetLineWidth());
            break;
        case TOOL_BEZIER:
            imageModel.ResetToPrevious();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            switch (pointSP)
            {
                case 1:
                    Line(hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, bg,
                         toolsModel.GetLineWidth());
                    break;
                case 2:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], bg, toolsModel.GetLineWidth());
                    break;
                case 3:
                    Bezier(hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], bg, toolsModel.GetLineWidth());
                    break;
            }
            break;
        case TOOL_RECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Rect(hdc, start.x, start.y, x, y, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_SHAPE:
            imageModel.ResetToPrevious();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   pointStack[pointSP].x, pointStack[pointSP].y);
            if (pointSP + 1 >= 2)
                Poly(hdc, pointStack, pointSP + 1, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
            break;
        case TOOL_ELLIPSE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Ellp(hdc, start.x, start.y, x, y, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_RRECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            RRect(hdc, start.x, start.y, x, y, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
    }

    last.x = x;
    last.y = y;
}

void
endPaintingR(HDC hdc, LONG x, LONG y, COLORREF fg, COLORREF bg)
{
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_RUBBER:
            Replace(hdc, last.x, last.y, x, y, fg, bg, toolsModel.GetRubberRadius());
            break;
        case TOOL_PEN:
            Line(hdc, last.x, last.y, x, y, bg, 1);
            SetPixel(hdc, x, y, bg);
            break;
        case TOOL_LINE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(start.x, start.y, x, y);
            Line(hdc, start.x, start.y, x, y, bg, toolsModel.GetLineWidth());
            break;
        case TOOL_BEZIER:
            pointSP++;
            if (pointSP == 4)
                pointSP = 0;
            break;
        case TOOL_RECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Rect(hdc, start.x, start.y, x, y, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_SHAPE:
            imageModel.ResetToPrevious();
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
                roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                                   pointStack[pointSP].x, pointStack[pointSP].y);
            pointSP++;
            if (pointSP >= 2)
            {
                if ((pointStack[0].x - x) * (pointStack[0].x - x) +
                    (pointStack[0].y - y) * (pointStack[0].y - y) <= toolsModel.GetLineWidth() * toolsModel.GetLineWidth() + 1)
                {
                    Poly(hdc, pointStack, pointSP, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), TRUE, FALSE);
                    pointSP = 0;
                }
                else
                {
                    Poly(hdc, pointStack, pointSP, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                }
            }
            if (pointSP == 255)
                pointSP--;
            break;
        case TOOL_ELLIPSE:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            Ellp(hdc, start.x, start.y, x, y, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
        case TOOL_RRECT:
            imageModel.ResetToPrevious();
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                regularize(start.x, start.y, x, y);
            RRect(hdc, start.x, start.y, x, y, bg, fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
            break;
    }
}
