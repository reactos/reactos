/****************************** Module*Header *****************************\
* Module Name: mirror.c                                                    *
*                                                                          *
* This module contains all the Right-To-Left (RTL) Mirroring support       *
* routines used to Right-To-Left mirror an icon on the fly so that         *
* it would be displayed normal on a RTL mirrored localized OS. This is     *
* mainly a concern for 3rd party Apps.                                     *
*                                                                          *
*                                                                          *
* Created: 01-Feb-1998 8:41:18 pm                                          *
* Author: Samer Arafeh [samera]                                            *
*                                                                          *
* Copyright (c) 1998 Microsoft Corporation                                 *
\**************************************************************************/

#include "priv.h"




/***************************************************************************\
* MirrorIcon
*
* Mirror an Icon , given an Icon handle so that when these icons are displayed
* on a Mirrored DC, they end would be displayed normal.
*
* History:
* 04-Feb-1998 samera    Created
\***************************************************************************/
STDAPI_(BOOL) SHMirrorIcon(HICON* phiconSmall, HICON* phiconLarge)
{
    HDC      hdcScreen;
    HBITMAP  hbm, hbmMask, hbmOld,hbmOldMask;
    BITMAP   bm;
    HICON    hicon[2] = {NULL,NULL};
    HICON    hiconNew[2] = {NULL,NULL};
    ICONINFO ii ;
    int      i;

    if (!g_bMirroredOS)
    {
        return FALSE;
    }

    //
    // Synchronize access to global DCs now!
    // Allocate DCs if we didn't so far.
    //  
    ENTERCRITICAL;
    if (!g_hdc && !g_hdcMask)
    {
        g_hdc = CreateCompatibleDC(NULL);
        if (g_hdc)
        {
            g_hdcMask = CreateCompatibleDC(NULL);

            if( g_hdcMask )
            {
                SET_DC_RTL_MIRRORED(g_hdc);
                SET_DC_RTL_MIRRORED(g_hdcMask);
            }
            else
            {
                DeleteDC( g_hdc );
                g_hdc = NULL;
            }
        }
    }

    if (phiconSmall)
        hicon[0] = *phiconSmall;

    if (phiconLarge)
        hicon[1] = *phiconLarge;

    //
    // Acquire screen DC
    //
    hdcScreen = GetDC(NULL);

    if (g_hdc && g_hdcMask && hdcScreen) 
    {
        for( i=0 ; i<(sizeof(hicon)/sizeof(hicon[0])) ; i++ )
        {
            if( hicon[i] )
            {
                if( GetIconInfo(hicon[i], &ii) &&
                    GetObject(ii.hbmColor, sizeof(bm), &bm))
                {

                    //
                    // I don't want these.
                    //
                    DeleteObject( ii.hbmMask );
                    DeleteObject( ii.hbmColor );
                    ii.hbmMask = ii.hbmColor = NULL;

                    hbm = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
                    hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
                    hbmOld = (HBITMAP)SelectObject(g_hdc, hbm);
                    hbmOldMask = (HBITMAP)SelectObject(g_hdcMask, hbmMask);
        
                    DrawIconEx(g_hdc, 0, 0, hicon[i], bm.bmWidth, bm.bmHeight, 0,
                               NULL, DI_IMAGE);

                    DrawIconEx(g_hdcMask, 0, 0, hicon[i], bm.bmWidth, bm.bmHeight, 0,
                               NULL, DI_MASK);

                    SelectObject(g_hdc, hbmOld);
                    SelectObject(g_hdcMask, hbmOldMask);

                    //
                    // create the new mirrored icon, and delete bmps
                    //
                    ii.hbmMask  = hbmMask;
                    ii.hbmColor = hbm;
                    hiconNew[i] = CreateIconIndirect(&ii);

                    DeleteObject(hbm);
                    DeleteObject(hbmMask);
                }
            }
        }
    }

    ReleaseDC(NULL, hdcScreen);

    //
    // Now we can reuse the global DCs
    //
    LEAVECRITICAL;

    //
    // Update icons if needed, and destroy old ones!
    //
    if (hicon[0] && hiconNew[0])
    {
        *phiconSmall = hiconNew[0];
        DestroyIcon(hicon[0]);
    }

    if (hicon[1] && hiconNew[1])
    {
        *phiconLarge = hiconNew[1];

        //
        // Don't delete twice
        //
        if (hicon[1] != hicon[0]) 
            DestroyIcon(hicon[1]);
    }

    return TRUE;
}


