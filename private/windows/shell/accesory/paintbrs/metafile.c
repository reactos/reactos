/****************************Module*Header******************************\
* Module Name: metafile.c                                               *
* Routines to paste a metafile as a bitmap.                             *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

/* Computes dimensions of a metafile picture in pixels */
BOOL GetMFDimensions(
    HANDLE hMF,     /* handle to the CF_METAFILEPICT object from clipbrd */
    HDC hDC,        /* display context */
    int *pWidth,    /* width of picture in pixels, OUT param */
    int *pHeight)   /* height of picture in pixels, OUT param */
{
    METAFILEPICT FAR *lpMfp, Picture;
    int MapModeOld=0;
    RECT Rect;
    long xScale, yScale, Scale;
    int hRes, vRes;     /* horz and vert resolution, in pixels */
    int hSize, vSize;   /* horz and vert size, in mm */
    int fResult = FALSE;

    if (!hMF || !(lpMfp = (METAFILEPICT FAR *)GlobalLock(hMF)))
        return FALSE;
    /* copy metafile picture hdr */
    Picture = *lpMfp;
    GlobalUnlock(hMF);

    /* Do not modify given DC's attributes */
    SaveDC(hDC);

    /* set the mapping mode */
    MapModeOld = SetMapMode(hDC, Picture.mm);
    if (Picture.mm != MM_ISOTROPIC && Picture.mm != MM_ANISOTROPIC)
    {
        /* For modes other than ISOTROPIC and ANISOTROPIC the picture 
         * dimensions are given in logical units.
        /* Convert logical units to pixels. */
        Rect.left = 0; Rect.right = Picture.xExt; 
        Rect.top = 0;  Rect.bottom = Picture.yExt;
        if (!LPtoDP(hDC, (LPPOINT)&Rect, 2)) 
            goto Error;
        *pWidth = Rect.right - Rect.left + 1;
        *pHeight = Rect.bottom - Rect.top + 1;
        fResult = TRUE;
    }
    else    /* ISOTROPIC or ANISOTROPIC mode, 
             * using the xExt and yExt, determine pixel width and height of 
             * the image */
    {
        hRes = GetDeviceCaps(hDC, HORZRES);
        vRes = GetDeviceCaps(hDC, VERTRES);
        hSize = GetDeviceCaps(hDC, HORZSIZE);
        vSize = GetDeviceCaps(hDC, VERTSIZE);
        if (Picture.xExt == 0)  /* assume default size, aspect ratio */
        {
            *pWidth = imageWid;
            *pHeight = imageHgt;
        }
        else if (Picture.xExt > 0)  /* use suggested size in HIMETRIC units */
        {
            // convert suggested extents(in .01 mm units) for picture to pixel units. 

            // xPixelsPermm = hRes/hSize;, yPixelsPermm = vRes/vSize;
            // Use Pixels Per logical unit.
            // *pWidth = Picture.xExt*xPixelsPermm/100;
            // *pHeight = Picture.yExt*yPixelsPermm/100;
            *pWidth = ((long)Picture.xExt * hRes/hSize/100);
            *pHeight = ((long)Picture.yExt * vRes/vSize/100);
        }
        else if (Picture.xExt < 0)  /* use suggested aspect ratio, default size */
        {
            // 1 log unit = .01 mm.
            // (# of log units in imageWid pixels)/xExt;
            xScale = 100L * (long) imageWid *
                            hSize/hRes/-Picture.xExt;
            // (# of log units in imageHgt pixels)/yExt;
            yScale = 100L * (long) imageHgt *
                            vSize/vRes/-Picture.yExt;
            // choose the minimum to accomodate the entire image
            Scale = min(xScale, yScale);
            // use scaled Pixels Per log unit.
            *pWidth = ((long)-Picture.xExt * Scale * 
                            hRes/hSize / 100);
            *pHeight = ((long)-Picture.yExt * Scale * 
                            vRes/vSize / 100);
        }
        fResult = TRUE;
    }

Error:
    if (MapModeOld)
        SetMapMode(hDC, MapModeOld);    /* select the old mapping mode */
    RestoreDC(hDC, -1);
    return fResult;
}

BOOL PlayMetafileIntoDC(
    HANDLE hMF,
    RECT *pRect,
    HDC hDC)
{
    HBRUSH	hbrBackground;
    METAFILEPICT FAR *lpMfp;

    if (!(lpMfp = (METAFILEPICT FAR *)GlobalLock(hMF)))
        return FALSE;

    SaveDC(hDC);

    /* Setup background color for the bitmap */
	hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(hDC, pRect, hbrBackground);
    DeleteObject(hbrBackground);

    SetMapMode(hDC, lpMfp->mm);
    if (lpMfp->mm == MM_ISOTROPIC || lpMfp->mm == MM_ANISOTROPIC)
	MSetViewportExt(hDC, pRect->right-pRect->left, pRect->bottom-pRect->top);
    PlayMetaFile(hDC, lpMfp->hMF);
    GlobalUnlock(hMF);
    RestoreDC(hDC, -1);
    return TRUE;
}
