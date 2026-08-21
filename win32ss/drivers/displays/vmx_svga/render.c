/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            win32ss/drivers/displays/vmx_svga/render.c
 * PURPOSE:         Software-rendering bridge with VMware damage notification
 */

#include "vmx_svga_disp.h"

static PPDEV
VmxPdevFromSurface(SURFOBJ *pso)
{
    PPDEV ppdev;

    if (pso == NULL || pso->iType != STYPE_DEVICE || pso->dhpdev == NULL)
        return NULL;

    ppdev = (PPDEV)pso->dhpdev;
    if (ppdev->Signature != VMX_PDEV_SIGNATURE)
        return NULL;

    return ppdev;
}

static SURFOBJ *
VmxTranslateSurface(SURFOBJ *pso)
{
    PPDEV ppdev = VmxPdevFromSurface(pso);
    return (ppdev != NULL) ? ppdev->psoBacking : pso;
}

static BOOL
VmxNotifyDamage(PPDEV ppdev, const RECTL *Rect)
{
    VMX_SVGA_UPDATE_RECT Damage;
    ULONG ReturnedLength;
    LONG Left;
    LONG Top;
    LONG Right;
    LONG Bottom;
    LONG Temp;

    if (Rect == NULL)
    {
        Left = 0;
        Top = 0;
        Right = (LONG)ppdev->ScreenWidth;
        Bottom = (LONG)ppdev->ScreenHeight;
    }
    else
    {
        Left = Rect->left;
        Top = Rect->top;
        Right = Rect->right;
        Bottom = Rect->bottom;

        if (Left > Right)
        {
            Temp = Left;
            Left = Right;
            Right = Temp;
        }
        if (Top > Bottom)
        {
            Temp = Top;
            Top = Bottom;
            Bottom = Temp;
        }
    }

    if (Right <= 0 || Bottom <= 0 ||
        Left >= (LONG)ppdev->ScreenWidth ||
        Top >= (LONG)ppdev->ScreenHeight)
    {
        return TRUE;
    }

    if (Left < 0)
        Left = 0;
    if (Top < 0)
        Top = 0;
    if (Right > (LONG)ppdev->ScreenWidth)
        Right = (LONG)ppdev->ScreenWidth;
    if (Bottom > (LONG)ppdev->ScreenHeight)
        Bottom = (LONG)ppdev->ScreenHeight;

    if (Right <= Left || Bottom <= Top)
        return TRUE;

    Damage.Left = Left;
    Damage.Top = Top;
    Damage.Right = Right;
    Damage.Bottom = Bottom;

    return EngDeviceIoControl(ppdev->hDriver,
                              IOCTL_VIDEO_VMX_SVGA_UPDATE,
                              &Damage,
                              sizeof(Damage),
                              NULL,
                              0,
                              &ReturnedLength) == 0;
}

BOOL APIENTRY
DrvBitBlt(
    IN OUT SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMask,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclTrg,
    IN POINTL *pptlSrc,
    IN POINTL *pptlMask,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrush,
    IN ROP4 rop4)
{
    PPDEV TargetPdev = VmxPdevFromSurface(psoTrg);
    BOOL Result;

    Result = EngBitBlt(VmxTranslateSurface(psoTrg),
                       VmxTranslateSurface(psoSrc),
                       VmxTranslateSurface(psoMask),
                       pco,
                       pxlo,
                       prclTrg,
                       pptlSrc,
                       pptlMask,
                       pbo,
                       pptlBrush,
                       rop4);

    if (!Result || TargetPdev == NULL)
        return Result;

    return VmxNotifyDamage(TargetPdev, prclTrg);
}

BOOL APIENTRY
DrvCopyBits(
    IN SURFOBJ *psoDest,
    IN SURFOBJ *psoSrc,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDest,
    IN POINTL *pptlSrc)
{
    PPDEV TargetPdev = VmxPdevFromSurface(psoDest);
    BOOL Result;

    Result = EngCopyBits(VmxTranslateSurface(psoDest),
                         VmxTranslateSurface(psoSrc),
                         pco,
                         pxlo,
                         prclDest,
                         pptlSrc);

    if (!Result || TargetPdev == NULL)
        return Result;

    return VmxNotifyDamage(TargetPdev, prclDest);
}

BOOL APIENTRY
DrvPaint(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX mix)
{
    PPDEV ppdev = VmxPdevFromSurface(pso);
    BOOL Result;

    if (ppdev == NULL)
        return FALSE;

    Result = EngPaint(ppdev->psoBacking, pco, pbo, pptlBrushOrg, mix);
    if (!Result)
        return FALSE;

    return VmxNotifyDamage(ppdev, (pco != NULL) ? &pco->rclBounds : NULL);
}

BOOL APIENTRY
DrvLineTo(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN LONG x1,
    IN LONG y1,
    IN LONG x2,
    IN LONG y2,
    IN RECTL *prclBounds,
    IN MIX mix)
{
    PPDEV ppdev = VmxPdevFromSurface(pso);
    BOOL Result;

    if (ppdev == NULL)
        return FALSE;

    Result = EngLineTo(ppdev->psoBacking,
                       pco,
                       pbo,
                       x1,
                       y1,
                       x2,
                       y2,
                       prclBounds,
                       mix);
    if (!Result)
        return FALSE;

    return VmxNotifyDamage(ppdev, prclBounds);
}
