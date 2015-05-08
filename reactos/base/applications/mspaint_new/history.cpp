/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/history.cpp
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

extern void updateCanvasAndScrollbars(void);

void
setImgXYRes(int x, int y)
{
    if ((imgXRes != x) || (imgYRes != y))
    {
        imgXRes = x;
        imgYRes = y;
        updateCanvasAndScrollbars();
    }
}

void
newReversible()
{
    DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = (HBITMAP) CopyImage(hBms[currInd], IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
    currInd = (currInd + 1) % HISTORYSIZE;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;
    SelectObject(hDrawingDC, hBms[currInd]);
    imgXRes = GetDIBWidth(hBms[currInd]);
    imgYRes = GetDIBHeight(hBms[currInd]);
    imageSaved = FALSE;
}

void
undo()
{
    if (undoSteps > 0)
    {
        ShowWindow(hSelection, SW_HIDE);
        currInd = (currInd + HISTORYSIZE - 1) % HISTORYSIZE;
        SelectObject(hDrawingDC, hBms[currInd]);
        undoSteps--;
        if (redoSteps < HISTORYSIZE - 1)
            redoSteps++;
        setImgXYRes(GetDIBWidth(hBms[currInd]), GetDIBHeight(hBms[currInd]));
    }
}

void
redo()
{
    if (redoSteps > 0)
    {
        ShowWindow(hSelection, SW_HIDE);
        currInd = (currInd + 1) % HISTORYSIZE;
        SelectObject(hDrawingDC, hBms[currInd]);
        redoSteps--;
        if (undoSteps < HISTORYSIZE - 1)
            undoSteps++;
        setImgXYRes(GetDIBWidth(hBms[currInd]), GetDIBHeight(hBms[currInd]));
    }
}

void
resetToU1()
{
    DeleteObject(hBms[currInd]);
    hBms[currInd] =
        (HBITMAP) CopyImage(hBms[(currInd + HISTORYSIZE - 1) % HISTORYSIZE], IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
    SelectObject(hDrawingDC, hBms[currInd]);
    imgXRes = GetDIBWidth(hBms[currInd]);
    imgYRes = GetDIBHeight(hBms[currInd]);
}

void
clearHistory()
{
    undoSteps = 0;
    redoSteps = 0;
}

void
insertReversible(HBITMAP hbm)
{
    DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = hbm;
    currInd = (currInd + 1) % HISTORYSIZE;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;
    SelectObject(hDrawingDC, hBms[currInd]);
    setImgXYRes(GetDIBWidth(hBms[currInd]), GetDIBHeight(hBms[currInd]));
}

void
cropReversible(int width, int height, int xOffset, int yOffset)
{
    HDC hdc;
    HPEN oldPen;
    HBRUSH oldBrush;

    SelectObject(hDrawingDC, hBms[currInd]);
    DeleteObject(hBms[(currInd + 1) % HISTORYSIZE]);
    hBms[(currInd + 1) % HISTORYSIZE] = CreateDIBWithProperties(width, height);
    currInd = (currInd + 1) % HISTORYSIZE;
    if (undoSteps < HISTORYSIZE - 1)
        undoSteps++;
    redoSteps = 0;

    hdc = CreateCompatibleDC(hDrawingDC);
    SelectObject(hdc, hBms[currInd]);

    oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, bgColor));
    oldBrush = (HBRUSH) SelectObject(hdc, CreateSolidBrush(bgColor));
    Rectangle(hdc, 0, 0, width, height);
    BitBlt(hdc, -xOffset, -yOffset, imgXRes, imgYRes, hDrawingDC, 0, 0, SRCCOPY);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    DeleteDC(hdc);
    SelectObject(hDrawingDC, hBms[currInd]);

    setImgXYRes(width, height);
}
