/*
 * PROJECT:         ReactOS Framebuffer Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            boot/drivers/video/displays/framebuf/pointer.c
 * PURPOSE:         Hardware Pointer Support
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 */

#include "driver.h"

BOOL NTAPI bCopyColorPointer(
PPDEV ppdev,
SURFOBJ *psoMask,
SURFOBJ *psoColor,
XLATEOBJ *pxlo);

BOOL NTAPI bCopyMonoPointer(
PPDEV ppdev,
SURFOBJ *psoMask);

BOOL NTAPI bSetHardwarePointerShape(
SURFOBJ  *pso,
SURFOBJ  *psoMask,
SURFOBJ  *psoColor,
XLATEOBJ *pxlo,
LONG      x,
LONG      y,
FLONG     fl);

/******************************Public*Routine******************************\
* DrvMovePointer
*
* Moves the hardware pointer to a new position.
*
\**************************************************************************/

VOID NTAPI DrvMovePointer
(
    SURFOBJ *pso,
    LONG     x,
    LONG     y,
    RECTL   *prcl
)
{
    PPDEV ppdev = (PPDEV) pso->dhpdev;
    DWORD returnedDataLength;
    VIDEO_POINTER_POSITION NewPointerPosition;

    // We don't use the exclusion rectangle because we only support
    // hardware Pointers. If we were doing our own Pointer simulations
    // we would want to update prcl so that the engine would call us
    // to exclude out pointer before drawing to the pixels in prcl.

    UNREFERENCED_PARAMETER(prcl);

    if (x == -1)
    {
        //
        // A new position of (-1,-1) means hide the pointer.
        //

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_DISABLE_POINTER,
                               NULL,
                               0,
                               NULL,
                               0,
                               &returnedDataLength))
        {
            //
            // Not the end of the world, print warning in checked build.
            //

            DISPDBG((1, "DISP vMoveHardwarePointer failed IOCTL_VIDEO_DISABLE_POINTER\n"));
        }
    }
    else
    {
        NewPointerPosition.Column = (SHORT) x - (SHORT) (ppdev->ptlHotSpot.x);
        NewPointerPosition.Row    = (SHORT) y - (SHORT) (ppdev->ptlHotSpot.y);

        //
        // Call miniport driver to move Pointer.
        //

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SET_POINTER_POSITION,
                               &NewPointerPosition,
                               sizeof(VIDEO_POINTER_POSITION),
                               NULL,
                               0,
                               &returnedDataLength))
        {
            //
            // Not the end of the world, print warning in checked build.
            //

            DISPDBG((1, "DISP vMoveHardwarePointer failed IOCTL_VIDEO_SET_POINTER_POSITION\n"));
        }
    }
}

/******************************Public*Routine******************************\
* DrvSetPointerShape
*
* Sets the new pointer shape.
*
\**************************************************************************/

ULONG NTAPI DrvSetPointerShape
(
    SURFOBJ  *pso,
    SURFOBJ  *psoMask,
    SURFOBJ  *psoColor,
    XLATEOBJ *pxlo,
    LONG      xHot,
    LONG      yHot,
    LONG      x,
    LONG      y,
    RECTL    *prcl,
    FLONG     fl
)
{
    PPDEV   ppdev = (PPDEV) pso->dhpdev;
    DWORD   returnedDataLength;

    // We don't use the exclusion rectangle because we only support
    // hardware Pointers. If we were doing our own Pointer simulations
    // we would want to update prcl so that the engine would call us
    // to exclude out pointer before drawing to the pixels in prcl.
    UNREFERENCED_PARAMETER(prcl);

    if (ppdev->pPointerAttributes == (PVIDEO_POINTER_ATTRIBUTES) NULL)
    {
        // Mini-port has no hardware Pointer support.
        return(SPS_ERROR);
    }

    // See if we are being asked to hide the pointer

    if (psoMask == (SURFOBJ *) NULL)
    {
        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_DISABLE_POINTER,
                               NULL,
                               0,
                               NULL,
                               0,
                               &returnedDataLength))
        {
            //
            // It should never be possible to fail.
            // Message supplied for debugging.
            //

            DISPDBG((1, "DISP bSetHardwarePointerShape failed IOCTL_VIDEO_DISABLE_POINTER\n"));
        }

        return(TRUE);
    }

    ppdev->ptlHotSpot.x = xHot;
    ppdev->ptlHotSpot.y = yHot;

    if (!bSetHardwarePointerShape(pso,psoMask,psoColor,pxlo,x,y,fl))
    {
            if (ppdev->fHwCursorActive) {
                ppdev->fHwCursorActive = FALSE;

                if (EngDeviceIoControl(ppdev->hDriver,
                                       IOCTL_VIDEO_DISABLE_POINTER,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       &returnedDataLength)) {

                    DISPDBG((1, "DISP bSetHardwarePointerShape failed IOCTL_VIDEO_DISABLE_POINTER\n"));
                }
            }

            //
            // Mini-port declines to realize this Pointer
            //

            return(SPS_DECLINE);
    }
    else
    {
        ppdev->fHwCursorActive = TRUE;
    }

    return(SPS_ACCEPT_NOEXCLUDE);
}

/******************************Public*Routine******************************\
* bSetHardwarePointerShape
*
* Changes the shape of the Hardware Pointer.
*
* Returns: True if successful, False if Pointer shape can't be hardware.
*
\**************************************************************************/

BOOL NTAPI bSetHardwarePointerShape(
SURFOBJ  *pso,
SURFOBJ  *psoMask,
SURFOBJ  *psoColor,
XLATEOBJ *pxlo,
LONG      x,
LONG      y,
FLONG     fl)
{
    PPDEV     ppdev = (PPDEV) pso->dhpdev;
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes = ppdev->pPointerAttributes;
    DWORD     returnedDataLength;

    if (psoColor != (SURFOBJ *) NULL)
    {
        if ((ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER) &&
                bCopyColorPointer(ppdev, psoMask, psoColor, pxlo))
        {
            pPointerAttributes->Flags |= VIDEO_MODE_COLOR_POINTER;
        } else {
            return(FALSE);
        }

    } else {

        if ((ppdev->PointerCapabilities.Flags & VIDEO_MODE_MONO_POINTER) &&
                bCopyMonoPointer(ppdev, psoMask))
        {
            pPointerAttributes->Flags |= VIDEO_MODE_MONO_POINTER;
        } else {
            return(FALSE);
        }
    }

    //
    // Initialize Pointer attributes and position
    //

    pPointerAttributes->Enable = 1;

    //
    // if x,y = -1,-1 then pass them directly to the miniport so that
    // the cursor will be disabled

    pPointerAttributes->Column = (SHORT)(x);
    pPointerAttributes->Row    = (SHORT)(y);

    if ((x != -1) || (y != -1)) {
        pPointerAttributes->Column -= (SHORT)(ppdev->ptlHotSpot.x);
        pPointerAttributes->Row    -= (SHORT)(ppdev->ptlHotSpot.y);
    }

    //
    // set animate flags
    //

    if (fl & SPS_ANIMATESTART) {
        pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_START;
    } else if (fl & SPS_ANIMATEUPDATE) {
        pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_UPDATE;
    }

    //
    // Set the new Pointer shape.
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_POINTER_ATTR,
                           pPointerAttributes,
                           ppdev->cjPointerAttributes,
                           NULL,
                           0,
                           &returnedDataLength)) {

        DISPDBG((1, "DISP:Failed IOCTL_VIDEO_SET_POINTER_ATTR call\n"));
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bCopyMonoPointer
*
* Copies two monochrome masks into a buffer of the maximum size handled by the
* miniport, with any extra bits set to 0.  The masks are converted to topdown
* form if they aren't already.  Returns TRUE if we can handle this pointer in
* hardware, FALSE if not.
*
\**************************************************************************/

BOOL NTAPI bCopyMonoPointer(
    PPDEV    ppdev,
    SURFOBJ *pso)
{
    ULONG cy;
    PBYTE pjSrcAnd, pjSrcXor;
    LONG  lDeltaSrc, lDeltaDst;
    LONG  lSrcWidthInBytes;
    ULONG cxSrc = pso->sizlBitmap.cx;
    ULONG cySrc = pso->sizlBitmap.cy;
    ULONG cxSrcBytes;
    PVIDEO_POINTER_ATTRIBUTES pPointerAttributes = ppdev->pPointerAttributes;
    PBYTE pjDstAnd = pPointerAttributes->Pixels;
    PBYTE pjDstXor = pPointerAttributes->Pixels;

    // Make sure the new pointer isn't too big to handle
    // (*2 because both masks are in there)
    if ((cxSrc > ppdev->PointerCapabilities.MaxWidth) ||
        (cySrc > (ppdev->PointerCapabilities.MaxHeight * 2)))
    {
        return(FALSE);
    }

    pjDstXor += ((ppdev->PointerCapabilities.MaxWidth + 7) / 8) *
            ppdev->pPointerAttributes->Height;

    // set the desk and mask to 0xff
    RtlFillMemory(pjDstAnd, ppdev->pPointerAttributes->WidthInBytes *
            ppdev->pPointerAttributes->Height, 0xFF);

    // Zero the dest XOR mask
    RtlZeroMemory(pjDstXor, ppdev->pPointerAttributes->WidthInBytes *
            ppdev->pPointerAttributes->Height);

    cxSrcBytes = (cxSrc + 7) / 8;

    if ((lDeltaSrc = pso->lDelta) < 0)
    {
        lSrcWidthInBytes = -lDeltaSrc;
    } else {
        lSrcWidthInBytes = lDeltaSrc;
    }

    pjSrcAnd = (PBYTE) pso->pvBits;

    // If the incoming pointer bitmap is bottomup, we'll flip it to topdown to
    // save the miniport some work
    if (!(pso->fjBitmap & BMF_TOPDOWN))
    {
        // Copy from the bottom
        pjSrcAnd += lSrcWidthInBytes * (cySrc - 1);
    }

    // Height of just AND mask
    cySrc = cySrc / 2;

    // Point to XOR mask
    pjSrcXor = pjSrcAnd + (cySrc * lDeltaSrc);

    // Offset from end of one dest scan to start of next
    lDeltaDst = ppdev->pPointerAttributes->WidthInBytes;

    for (cy = 0; cy < cySrc; ++cy)
    {
        RtlCopyMemory(pjDstAnd, pjSrcAnd, cxSrcBytes);
        RtlCopyMemory(pjDstXor, pjSrcXor, cxSrcBytes);

        // Point to next source and dest scans
        pjSrcAnd += lDeltaSrc;
        pjSrcXor += lDeltaSrc;
        pjDstAnd += lDeltaDst;
        pjDstXor += lDeltaDst;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bCopyColorPointer
*
* Copies the mono and color masks into the buffer of maximum size
* handled by the miniport with any extra bits set to 0. Color translation
* is handled at this time. The masks are converted to topdown form if they
* aren't already.  Returns TRUE if we can handle this pointer in  hardware,
* FALSE if not.
*
\**************************************************************************/
BOOL NTAPI bCopyColorPointer(
PPDEV ppdev,
SURFOBJ *psoMask,
SURFOBJ *psoColor,
XLATEOBJ *pxlo)
{
    return(FALSE);
}


/******************************Public*Routine******************************\
* bInitPointer
*
* Initialize the Pointer attributes.
*
\**************************************************************************/

BOOL NTAPI bInitPointer(PPDEV ppdev, DEVINFO *pdevinfo)
{
    DWORD    returnedDataLength;

    ppdev->pPointerAttributes = (PVIDEO_POINTER_ATTRIBUTES) NULL;
    ppdev->cjPointerAttributes = 0; // initialized in screen.c

    //
    // Ask the miniport whether it provides pointer support.
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES,
                           NULL,
                           0,
                           &ppdev->PointerCapabilities,
                           sizeof(ppdev->PointerCapabilities),
                           &returnedDataLength))
    {
         return(FALSE);
    }

    //
    // If neither mono nor color hardware pointer is supported, there's no
    // hardware pointer support and we're done.
    //

    if ((!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_MONO_POINTER)) &&
        (!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER)))
    {
        return(TRUE);
    }

    //
    // Note: The buffer itself is allocated after we set the
    // mode. At that time we know the pixel depth and we can
    // allocate the correct size for the color pointer if supported.
    //

    //
    // Set the asynchronous support status (async means miniport is capable of
    // drawing the Pointer at any time, with no interference with any ongoing
    // drawing operation)
    //

    if (ppdev->PointerCapabilities.Flags & VIDEO_MODE_ASYNC_POINTER)
    {
       pdevinfo->flGraphicsCaps |= GCAPS_ASYNCMOVE;
    }
    else
    {
       pdevinfo->flGraphicsCaps &= ~GCAPS_ASYNCMOVE;
    }

    return(TRUE);
}
