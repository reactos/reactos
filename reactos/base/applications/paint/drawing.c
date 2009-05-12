/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        drawing.c
 * PURPOSE:     The drawing functions used by the tools
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>

/* FUNCTIONS ********************************************************/

void Line(HDC hdc, short x1, short y1, short x2, short y2, int color, int thickness)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, thickness, color));
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
    DeleteObject(SelectObject(hdc, oldPen));
}

void Rect(HDC hdc, short x1, short y1, short x2, short y2, int fg, int bg, int thickness, BOOL filled)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    LOGBRUSH logbrush;
    if (filled) logbrush.lbStyle = BS_SOLID; else logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = bg;
    logbrush.lbHatch = 0;
    HBRUSH oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Rectangle(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void Ellp(HDC hdc, short x1, short y1, short x2, short y2, int fg, int bg, int thickness, BOOL filled)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    LOGBRUSH logbrush;
    if (filled) logbrush.lbStyle = BS_SOLID; else logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = bg;
    logbrush.lbHatch = 0;
    HBRUSH oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Ellipse(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void RRect(HDC hdc, short x1, short y1, short x2, short y2, int fg, int bg, int thickness, BOOL filled)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, thickness, fg));
    LOGBRUSH logbrush;
    if (filled) logbrush.lbStyle = BS_SOLID; else logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = bg;
    logbrush.lbHatch = 0;
    HBRUSH oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    RoundRect(hdc, x1, y1, x2, y2, 16, 16);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void Fill(HDC hdc, int x, int y, int color)
{
    HBRUSH oldBrush = SelectObject(hdc, CreateSolidBrush(color));
    ExtFloodFill(hdc, x, y, GetPixel(hdc, x, y), FLOODFILLSURFACE);
    DeleteObject(SelectObject(hdc, oldBrush));
}

void Erase(HDC hdc, short x1, short y1, short x2, short y2, int color, int radius)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, color));
    HBRUSH oldBrush = SelectObject(hdc, CreateSolidBrush(color));
    short a;
    for (a=0; a<=100; a++)
        Rectangle(hdc, (x1*(100-a)+x2*a)/100-radius+1, (y1*(100-a)+y2*a)/100-radius+1, (x1*(100-a)+x2*a)/100+radius+1, (y1*(100-a)+y2*a)/100+radius+1);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void Airbrush(HDC hdc, short x, short y, int color, int r)
{
    short a;
    short b;
    for (b=-r; b<=r; b++) for (a=-r; a<=r; a++) if ((a*a+b*b<=r*r)&&(rand()%4==0)) SetPixel(hdc, x+a, y+b, color);
}

void Brush(HDC hdc, short x1, short y1, short x2, short y2, int color, int style)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, color));
    HBRUSH oldBrush = SelectObject(hdc, CreateSolidBrush(color));
    short a;
    switch (style)
    {
        case 0:
            for (a=0; a<=100; a++)
                Ellipse(hdc, (x1*(100-a)+x2*a)/100-3, (y1*(100-a)+y2*a)/100-3, (x1*(100-a)+x2*a)/100+4, (y1*(100-a)+y2*a)/100+4);
            break;
        case 1:
            for (a=0; a<=100; a++)
                Ellipse(hdc, (x1*(100-a)+x2*a)/100-1, (y1*(100-a)+y2*a)/100-1, (x1*(100-a)+x2*a)/100+3, (y1*(100-a)+y2*a)/100+3);
            break;
        case 2:
            MoveToEx(hdc, x1, y1, NULL);
            LineTo(hdc, x2, y2);
            SetPixel(hdc, x2, y2, color);
            break;
        case 3:
            for (a=0; a<=100; a++)
                Rectangle(hdc, (x1*(100-a)+x2*a)/100-3, (y1*(100-a)+y2*a)/100-3, (x1*(100-a)+x2*a)/100+5, (y1*(100-a)+y2*a)/100+5);
            break;
        case 4:
            for (a=0; a<=100; a++)
                Rectangle(hdc, (x1*(100-a)+x2*a)/100-2, (y1*(100-a)+y2*a)/100-2, (x1*(100-a)+x2*a)/100+3, (y1*(100-a)+y2*a)/100+3);
            break;
        case 5:
            for (a=0; a<=100; a++)
                Rectangle(hdc, (x1*(100-a)+x2*a)/100-1, (y1*(100-a)+y2*a)/100-1, (x1*(100-a)+x2*a)/100+1, (y1*(100-a)+y2*a)/100+1);
            break;
        case 6:
            for (a=0; a<=100; a++)
            {
                MoveToEx(hdc, (x1*(100-a)+x2*a)/100-3, (y1*(100-a)+y2*a)/100+5, NULL);
                LineTo(hdc, (x1*(100-a)+x2*a)/100+5, (y1*(100-a)+y2*a)/100-3);
            }
            break;
        case 7:
            for (a=0; a<=100; a++)
            {
                MoveToEx(hdc, (x1*(100-a)+x2*a)/100-2, (y1*(100-a)+y2*a)/100+3, NULL);
                LineTo(hdc, (x1*(100-a)+x2*a)/100+3, (y1*(100-a)+y2*a)/100-2);
            }
            break;
        case 8:
            for (a=0; a<=100; a++)
            {
                MoveToEx(hdc, (x1*(100-a)+x2*a)/100-1, (y1*(100-a)+y2*a)/100+1, NULL);
                LineTo(hdc, (x1*(100-a)+x2*a)/100+1, (y1*(100-a)+y2*a)/100-1);
            }
            break;
        case 9:
            for (a=0; a<=100; a++)
            {
                MoveToEx(hdc, (x1*(100-a)+x2*a)/100-3, (y1*(100-a)+y2*a)/100-3, NULL);
                LineTo(hdc, (x1*(100-a)+x2*a)/100+5, (y1*(100-a)+y2*a)/100+5);
            }
            break;
        case 10:
            for (a=0; a<=100; a++)
            {
                MoveToEx(hdc, (x1*(100-a)+x2*a)/100-2, (y1*(100-a)+y2*a)/100-2, NULL);
                LineTo(hdc, (x1*(100-a)+x2*a)/100+3, (y1*(100-a)+y2*a)/100+3);
            }
            break;
        case 11:
            for (a=0; a<=100; a++)
            {
                MoveToEx(hdc, (x1*(100-a)+x2*a)/100-1, (y1*(100-a)+y2*a)/100-1, NULL);
                LineTo(hdc, (x1*(100-a)+x2*a)/100+1, (y1*(100-a)+y2*a)/100+1);
            }
            break;
    }
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void RectSel(HDC hdc, short x1, short y1, short x2, short y2)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_DOT, 1, 0x00000000));
    LOGBRUSH logbrush;
    logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = 0;
    logbrush.lbHatch = 0;
    HBRUSH oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Rectangle(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}

void SelectionFrame(HDC hdc, int x1, int y1, int x2, int y2)
{
    HPEN oldPen = SelectObject(hdc, CreatePen(PS_DOT, 1, 0x00000000));
    LOGBRUSH logbrush;
    logbrush.lbStyle = BS_HOLLOW;
    logbrush.lbColor = 0;
    logbrush.lbHatch = 0;
    HBRUSH oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    Rectangle(hdc, x1, y1, x2, y2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, 0x00000000));
    oldBrush = SelectObject(hdc, CreateSolidBrush(0x00000000));
    Rectangle(hdc, x1-1, y1-1, x1+2, y1+2);
    Rectangle(hdc, x2-2, y1-1, x2+2, y1+2);
    Rectangle(hdc, x1-1, y2-2, x1+2, y2+1);
    Rectangle(hdc, x2-2, y2-2, x2+2, y2+1);
    Rectangle(hdc, (x1+x2)/2-1, y1-1, (x1+x2)/2+2, y1+2);
    Rectangle(hdc, (x1+x2)/2-1, y2-2, (x1+x2)/2+2, y2+1);
    Rectangle(hdc, x1-1, (y1+y2)/2-1, x1+2, (y1+y2)/2+2);
    Rectangle(hdc, x2-2, (y1+y2)/2-1, x2+1, (y1+y2)/2+2);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
}
