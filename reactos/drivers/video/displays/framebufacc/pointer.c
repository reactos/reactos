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

#include "framebuf_acc.h"


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
 *    @unimplemented
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
/*   return SPS_DECLINE;*/
   return EngSetPointerShape(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);
}




