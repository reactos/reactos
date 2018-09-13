/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    miniicon.c
**
** Purpose: Creates and Manages the mini icons
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "resource.h"
#include "miniicon.h"



/*
**------------------------------------------------------------------------------
**	Local variables
**------------------------------------------------------------------------------
*/
HINSTANCE   MiniIcon::hInstance             = NULL;


MiniIcon::MiniIcon(
    void
    )
{
    hdcMiniMem = NULL;
    hbmMiniImage = NULL;
    hbmMiniMask = NULL;
    NumImages = 0;
 
    CreateMiniIcons();
}

MiniIcon::~MiniIcon(
    void
    )
{
    DestroyMiniIcons();
}

void
MiniIcon::Register(
    HINSTANCE hInstance
    )
{
    MiniIcon::hInstance = hInstance;
}

void
MiniIcon::Unregister(
    void
    )
{
    MiniIcon::hInstance= NULL;
}

BOOL
MiniIcon::CreateMiniIcons(
    void
    )
{
    HDC     hdc, hdcMem;
    HBITMAP hbmOld;
    BITMAP  bm;
    BOOL    bRet = FALSE;          // assume failure

    MiDebugMsg((0, "MiniIcon:CreateMiniIcons()"));

    if(hdcMiniMem) 
    {
        //
        // Then the mini-icon list has already been built, so
        // return success.
        //
        return TRUE;
    }

    hdc = GetDC(NULL);
    hdcMiniMem = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);
    if(!hdcMiniMem) 
    {
        goto clean0;
    }


    if(!(hdcMem = CreateCompatibleDC(hdcMiniMem))) 
    {
        goto clean0;
    }

    if(!(hbmMiniImage = LoadBitmap(MiniIcon::hInstance, MAKEINTRESOURCE(BMP_MINIICONS)))) 
    {
        goto clean1;
    }


    GetObject(hbmMiniImage, sizeof(bm), &bm);

    if(!(hbmMiniMask = CreateBitmap(bm.bmWidth,
                                    bm.bmHeight,
                                    1,
                                    1,
                                    NULL))) 
    {
        goto clean1;
    }


    hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMiniImage);
    SelectObject(hdcMiniMem, hbmMiniMask);

    //
    // make the mask.  white where transparent, black where opaque
    //
    SetBkColor(hdcMem, RGB_TRANSPARENT);
    BitBlt(hdcMiniMem,
           0,
           0,
           bm.bmWidth,
           bm.bmHeight,
           hdcMem,
           0,
           0,
           SRCCOPY
          );

    //
    // black-out all of the transparent parts of the image, in preparation
    // for drawing.
    //
    SetBkColor(hdcMem, RGB_BLACK);
    SetTextColor(hdcMem, RGB_WHITE);
    BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcMiniMem, 0, 0, SRCAND);

    SelectObject(hdcMiniMem, hbmOld);

    NumImages = bm.bmWidth/MINIX;
    bRet = TRUE;

    SelectObject(hdcMem, hbmOld);

clean1:
    DeleteObject(hdcMem);

clean0:
    //
    // If failure, clean up anything we might have built
    //
    if(!bRet) 
    {
        DestroyMiniIcons();
    }

    return bRet;
}

void
MiniIcon::DestroyMiniIcons(
    void
    )
{
    MiDebugMsg((0, "MiniIcon::DestroyMiniIcons()"));

    if(hdcMiniMem) 
    {
        DeleteDC(hdcMiniMem);
        hdcMiniMem = NULL;
    }

    if(hbmMiniImage) 
    {
        DeleteObject(hbmMiniImage);
        hbmMiniImage = NULL;
    }

    if(hbmMiniMask) 
    {
        DeleteObject(hbmMiniMask);
        hbmMiniMask = NULL;
    }

    NumImages = 0;
}

/**************************************************************************
Routine Description:

    This routine draws the specified mini-icon at the requested location.

Arguments:

    hdc - Supplies the handle of the device context in which the mini-icon
        will be drawn.

    rc - Supplies the rectangle in the specified HDC to draw the icon in.

    MiniIconIndex - The index of the mini-icon

            0   Check
            1   Uncheck

    Flags - Controls the drawing operation.  The LOWORD contains the actual flags
        defined as follows:

        DMI_MASK - Draw the mini-icon's mask into HDC.

        DMI_BKCOLOR - Use the system color index specified in the HIWORD of Flags
            as the background color.  If not specified, COLOR_WINDOW is used.

        DMI_USERECT - If set, DrawMiniIcon will use the supplied rect,
            stretching the icon to fit as appropriate.

Return Value:

    This function returns the offset from the left of rc where the string should
    start.

Remarks:

    By default, the icon will be centered vertically and butted against the left
    corner of the specified rectangle.
**********************************************************************************/
int
MiniIcon::DrawMiniIcon(
    HDC   hdc,
    RECT  rc,
    INT   MiniIconIndex,
    DWORD Flags
    )
{
    HBITMAP hbmOld;
    DWORD rgbBk, rgbText;
    INT ret = 0;

    if(hbmMiniImage) 
    {
        //
        // Set the Foreground and  background color for the
        // conversion of the Mono Mask image
        //
        if(Flags & DMI_MASK) 
        {
            rgbBk = SetBkColor(hdc, RGB_WHITE);
        } 
        
        else 
        {
            rgbBk = SetBkColor(hdc,
                               GetSysColor(((int)(Flags & DMI_BKCOLOR
                                                      ? HIWORD(Flags)
                                                      : COLOR_WINDOW)))
                              );
        }
        
        rgbText = SetTextColor(hdc, RGB_BLACK);

        if(Flags & DMI_USERECT) 
        {
            //
            // Copy the converted mask into the dest.  The transparent
            // areas will be drawn with the current window color.
            //
            hbmOld = (HBITMAP)SelectObject(hdcMiniMem,
                                  hbmMiniMask
                                 );
            StretchBlt(hdc,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       rc.bottom - rc.top,
                       hdcMiniMem,
                       MINIX * MiniIconIndex,
                       0,
                       MINIX,
                       MINIY,
                       SRCCOPY);

            if(!(Flags & DMI_MASK)) 
            {
                //
                // OR the image into the dest
                //
                SelectObject(hdcMiniMem,
                             hbmMiniImage
                            );
                            
                StretchBlt(hdc,
                           rc.left,
                           rc.top,
                           rc.right - rc.left,
                           rc.bottom - rc.top,
                           hdcMiniMem,
                           MINIX * MiniIconIndex,
                           0,
                           MINIX,
                           MINIY,
                           SRCPAINT);
            }

        } 
        
        else 
        {
            //
            // Copy the converted mask into the dest.  The transparent
            // areas will be drawn with the current window color.
            //
            hbmOld = (HBITMAP)SelectObject(hdcMiniMem,
                                  hbmMiniMask
                                 );
            
            BitBlt(hdc,
                   rc.left,
                   rc.top + (rc.bottom - rc.top - MINIY)/2,
                   MINIX,
                   MINIY,
                   hdcMiniMem,
                   MINIX * MiniIconIndex,
                   0,
                   SRCCOPY
                  );


            if(!(Flags & DMI_MASK)) 
            {
                //
                // OR the image into the dest
                //
                SelectObject(hdcMiniMem,
                             hbmMiniImage
                            );
                BitBlt(hdc,
                       rc.left,
                       rc.top + (rc.bottom - rc.top - MINIY)/2,
                       MINIX,
                       MINIY,
                       hdcMiniMem,
                       MINIX * MiniIconIndex,
                       0,
                       SRCPAINT
                      );
            }
        }

        SetBkColor(hdc, rgbBk);
        SetTextColor(hdc, rgbText);

        SelectObject(hdcMiniMem, hbmOld);
        if(Flags & DMI_USERECT) 
        {
            ret = rc.right - rc.left + 2;   // offset to string from left edge
        } 
        
        else 
        {
            ret = MINIX + 2;                // offset to string from left edge
        }
    }

    return ret;
}
