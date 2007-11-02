/*
 * ReactOS Generic Framebuffer display driver
 *
 * Copyright (C) 2007 Magnus Olsen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "framebufacc.h"



/*
 * DrvMovePointer
 *
 * Moves the pointer to a new position and ensures that GDI does not interfere
 * with the display of the pointer.
 *
 * Status
 *    @implemented
 */

VOID APIENTRY
DrvMovePointer(IN SURFOBJ *pso,
               IN LONG x,
               IN LONG y,
               IN RECTL *prcl)
{
    PPDEV ppdev = (PPDEV) pso->dhpdev;
    DWORD returnedDataLength;
    VIDEO_POINTER_POSITION NewPointerPosition;

    x -= ppdev->ScreenOffsetXY.x;
    y -= ppdev->ScreenOffsetXY.y;

    /* position of (-1,-1) hide the pointer */
    if ((x == -1) || (y == -1))
    {
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_DISABLE_POINTER, NULL, 0, NULL, 0, &returnedDataLength))
        {
            /* hw did not disable the mouse, we try then with software */
            EngMovePointer(pso, x, y, prcl);
        }
    }
    else
    {
        /* Calc the mouse positions and set it to the new positions */
        NewPointerPosition.Column = (SHORT) x - (SHORT) (ppdev->PointerHotSpot.x);
        NewPointerPosition.Row    = (SHORT) y - (SHORT) (ppdev->PointerHotSpot.y);

        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_POINTER_POSITION, &NewPointerPosition,
                               sizeof(VIDEO_POINTER_POSITION), NULL, 0, &returnedDataLength))
        {
            /* hw did not disable the mouse, we try then with software */
            EngMovePointer(pso, x, y, prcl);
        }
    }
}


/*
 * DrvSetPointerShape
 *
 * Sets the new pointer shape.
 *
 * Status
 *    @implemented
 */

ULONG APIENTRY
DrvSetPointerShape(
   IN SURFOBJ *pso,
   IN SURFOBJ *psoMask,
   IN SURFOBJ *psoColor,
   IN XLATEOBJ *pxlo,
   IN LONG xHot,
   IN LONG yHot,
   IN LONG x,
   IN LONG y,
   IN RECTL *prcl,
   IN FLONG fl)
{
    PPDEV   ppdev = (PPDEV) pso->dhpdev;
    ULONG returnedDataLength = 0;

    if (ppdev->pPointerAttributes == NULL)
    {
        /* hw did not support hw mouse pointer, we try then with software */
        return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
    }

    /* check see if the apps ask to hide the mouse or not */
    if (psoMask == (SURFOBJ *) NULL)
    {
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_DISABLE_POINTER, NULL, 0, NULL, 0, &returnedDataLength))
        {
            /* no hw support found for the mouse we try then the software version */
            return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
        }

        return TRUE;
    }

    /* set our hotspot */
    ppdev->PointerHotSpot.x = xHot;
    ppdev->PointerHotSpot.y = yHot;

    /* Set the hw mouse shape */

    if (psoColor != (SURFOBJ *) NULL)
    {
        /* We got a color mouse pointer */
        if ((ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER) &&
            (CopyColorPointer(ppdev, psoMask, psoColor, pxlo)) )
        {
            ppdev->pPointerAttributes->Flags |= VIDEO_MODE_COLOR_POINTER;
        }
        else
        {
            /* No color mouse pointer being set, so we need try the software version then */
            if (ppdev->HwMouseActive)
            {
                ppdev->HwMouseActive = FALSE;
                if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_DISABLE_POINTER, NULL, 0, NULL, 0, &returnedDataLength))
                {
                    /* hw did not support hw mouse pointer, we try then with software */
                    return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
                }
            }
            return SPS_DECLINE ;
        }
    }
    else
    {
        /* We got a mono mouse pointer */
        if ( (ppdev->PointerCapabilities.Flags & VIDEO_MODE_MONO_POINTER) &&
              (CopyMonoPointer(ppdev, psoMask)))
        {
            ppdev->pPointerAttributes->Flags |= VIDEO_MODE_MONO_POINTER;
        }
        else
        {
            /* No mono mouse pointer being set, so we need try the software version then */
            if (ppdev->HwMouseActive)
            {
                ppdev->HwMouseActive = FALSE;
                if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_DISABLE_POINTER, NULL, 0, NULL, 0, &returnedDataLength))
                {
                    /* hw did not support hw mouse pointer, we try then with software */
                    return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
                }
            }
            return SPS_DECLINE ;
        }
    }

    /* we goto hw mouse pointer then we contnue filling in more info */

    /* calc the mouse point positions */
    if ((x != -1) || (y != -1))
    {
        ppdev->pPointerAttributes->Column -= (SHORT)(ppdev->PointerHotSpot.x);
        ppdev->pPointerAttributes->Row -= (SHORT)(ppdev->PointerHotSpot.y);
    }

    /* set correct flags if it animated or need be updated anime or no flags at all */
    if (fl & SPS_ANIMATESTART)
    {
        ppdev->pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_START;
    }
    else if (fl & SPS_ANIMATEUPDATE)
    {
        ppdev->pPointerAttributes->Flags |= VIDEO_MODE_ANIMATE_UPDATE;
    }

    ppdev->pPointerAttributes->Enable = 1;
    ppdev->pPointerAttributes->Column = (SHORT)(x);
    ppdev->pPointerAttributes->Row    = (SHORT)(y);

    /* Set the new mouse pointer shape */
    if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_POINTER_ATTR, ppdev->pPointerAttributes,
                           ppdev->PointerAttributesSize, NULL, 0, &returnedDataLength))
    {
            /* no hw support found for the mouse we try then the software version */
            return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
    }

    /* we got real hw support */
    ppdev->HwMouseActive = TRUE;
    return SPS_ACCEPT_NOEXCLUDE;
}


/* Internal api that are only use in DrvSetPointerShape */

BOOL
CopyColorPointer(PPDEV ppdev,
                SURFOBJ *psoMask,
                SURFOBJ *psoColor,
                XLATEOBJ *pxlo)
{
    /* FIXME unimplement */
    return FALSE;
}

BOOL
CopyMonoPointer(PPDEV ppdev,
                SURFOBJ *pso)
{
    /* FIXME unimplement */
    return FALSE;
}


