/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            win32ss/drivers/displays/vmx_svga/pointer.c
 * PURPOSE:         Software pointer delegation for VMware SVGA-II
 */

#include "vmx_svga_disp.h"

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
    return EngSetPointerShape(pso,
                              psoMask,
                              psoColor,
                              pxlo,
                              xHot,
                              yHot,
                              x,
                              y,
                              prcl,
                              fl);
}

VOID APIENTRY
DrvMovePointer(IN SURFOBJ *pso, IN LONG x, IN LONG y, IN RECTL *prcl)
{
    EngMovePointer(pso, x, y, prcl);
}
