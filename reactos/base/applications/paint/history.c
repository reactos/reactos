/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        history.c
 * PURPOSE:     Undo and redo functionality
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include "globalvar.h"
#include "dib.h"
#include "definitions.h"

/* FUNCTIONS ********************************************************/

extern void updateCanvasAndScrollbars(void);

void setImgXYRes(int x, int y)
{
    if ((imgXRes!=x)||(imgYRes!=y))
    {
        imgXRes = x;
        imgYRes = y;
        updateCanvasAndScrollbars();
    }
}

void newReversible()
{
    DeleteObject(hBms[(currInd+1)%4]);
    hBms[(currInd+1)%4] = CopyImage( hBms[currInd], IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
    currInd = (currInd+1)%4;
    if (undoSteps<3) undoSteps++;
    redoSteps = 0;
    SelectObject(hDrawingDC, hBms[currInd]);
    imgXRes = GetDIBWidth(hBms[currInd]);
    imgYRes = GetDIBHeight(hBms[currInd]);
}

void undo()
{
    if (undoSteps>0)
    {
        ShowWindow(hSelection, SW_HIDE);
        currInd = (currInd+3)%4;
        SelectObject(hDrawingDC, hBms[currInd]);
        undoSteps--;
        if (redoSteps<3) redoSteps++;
        setImgXYRes(GetDIBWidth(hBms[currInd]), GetDIBHeight(hBms[currInd]));
    }
}

void redo()
{
    if (redoSteps>0)
    {
        ShowWindow(hSelection, SW_HIDE);
        currInd = (currInd+1)%4;
        SelectObject(hDrawingDC, hBms[currInd]);
        redoSteps--;
        if (undoSteps<3) undoSteps++;
        setImgXYRes(GetDIBWidth(hBms[currInd]), GetDIBHeight(hBms[currInd]));
    }
}

void resetToU1()
{
    DeleteObject(hBms[currInd]);
    hBms[currInd] = CopyImage( hBms[(currInd+3)%4], IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG);
    SelectObject(hDrawingDC, hBms[currInd]);
    imgXRes = GetDIBWidth(hBms[currInd]);
    imgYRes = GetDIBHeight(hBms[currInd]);
}

void clearHistory()
{
    undoSteps = 0;
    redoSteps = 0;
}

void insertReversible(HBITMAP hbm)
{
    DeleteObject(hBms[(currInd+1)%4]);
    hBms[(currInd+1)%4] = hbm;
    currInd = (currInd+1)%4;
    if (undoSteps<3) undoSteps++;
    redoSteps = 0;
    SelectObject(hDrawingDC, hBms[currInd]);
    setImgXYRes(GetDIBWidth(hBms[currInd]), GetDIBHeight(hBms[currInd]));
}

void cropReversible(int x, int y)//FIXME: This function is broken
{
    SelectObject(hDrawingDC, hBms[currInd]);
    DeleteObject(hBms[(currInd+1)%4]);
    hBms[(currInd+1)%4] = CreateDIBWithProperties(x, y);
    currInd = (currInd+1)%4;
    if (undoSteps<3) undoSteps++;
    redoSteps = 0;
    
    HBITMAP oldBitmap = SelectObject(hSelDC, hBms[currInd]);
    HPEN oldPen = SelectObject(hSelDC, CreatePen(PS_SOLID, 1, bgColor));
    HBRUSH oldBrush = SelectObject(hSelDC, CreateSolidBrush(bgColor));
    Rectangle(hSelDC, 0, 0, x, y);
    DeleteObject(SelectObject(hSelDC, oldBrush));
    DeleteObject(SelectObject(hSelDC, oldPen));
    BitBlt(hDrawingDC, 0, 0, imgXRes, imgYRes, hSelDC, 0, 0, SRCCOPY);
    SelectObject(hDrawingDC, SelectObject(hSelDC, oldBitmap));
    
    setImgXYRes(x, y);
}
