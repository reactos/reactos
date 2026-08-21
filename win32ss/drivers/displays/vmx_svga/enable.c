/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            win32ss/drivers/displays/vmx_svga/enable.c
 * PURPOSE:         VMware SVGA-II display driver initialization
 */

#include "vmx_svga_disp.h"

static DRVFN DrvFunctionTable[] =
{
    {INDEX_DrvEnablePDEV, (PFN)DrvEnablePDEV},
    {INDEX_DrvCompletePDEV, (PFN)DrvCompletePDEV},
    {INDEX_DrvDisablePDEV, (PFN)DrvDisablePDEV},
    {INDEX_DrvEnableSurface, (PFN)DrvEnableSurface},
    {INDEX_DrvDisableSurface, (PFN)DrvDisableSurface},
    {INDEX_DrvAssertMode, (PFN)DrvAssertMode},
    {INDEX_DrvGetModes, (PFN)DrvGetModes},
    {INDEX_DrvSetPalette, (PFN)DrvSetPalette},
    {INDEX_DrvSetPointerShape, (PFN)DrvSetPointerShape},
    {INDEX_DrvMovePointer, (PFN)DrvMovePointer},
    {INDEX_DrvBitBlt, (PFN)DrvBitBlt},
    {INDEX_DrvCopyBits, (PFN)DrvCopyBits},
    {INDEX_DrvPaint, (PFN)DrvPaint},
    {INDEX_DrvLineTo, (PFN)DrvLineTo},
};

BOOL APIENTRY
DrvEnableDriver(
    ULONG iEngineVersion,
    ULONG cj,
    PDRVENABLEDATA pded)
{
    UNREFERENCED_PARAMETER(iEngineVersion);

    if (cj < sizeof(DRVENABLEDATA))
        return FALSE;

    pded->c = sizeof(DrvFunctionTable) / sizeof(DRVFN);
    pded->pdrvfn = DrvFunctionTable;
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
    return TRUE;
}

DHPDEV APIENTRY
DrvEnablePDEV(
    IN DEVMODEW *pdm,
    IN LPWSTR pwszLogAddress,
    IN ULONG cPat,
    OUT HSURF *phsurfPatterns,
    IN ULONG cjCaps,
    OUT ULONG *pdevcaps,
    IN ULONG cjDevInfo,
    OUT DEVINFO *pdi,
    IN HDEV hdev,
    IN LPWSTR pwszDeviceName,
    IN HANDLE hDriver)
{
    PPDEV ppdev;
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    ULONG CopySize;

    UNREFERENCED_PARAMETER(pwszLogAddress);
    UNREFERENCED_PARAMETER(cPat);
    UNREFERENCED_PARAMETER(phsurfPatterns);
    UNREFERENCED_PARAMETER(hdev);
    UNREFERENCED_PARAMETER(pwszDeviceName);

    ppdev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
    if (ppdev == NULL)
        return NULL;

    ppdev->Signature = VMX_PDEV_SIGNATURE;
    ppdev->hDriver = hDriver;

    if (!IntInitScreenInfo(ppdev, pdm, &GdiInfo, &DevInfo) ||
        !IntInitDefaultPalette(ppdev, &DevInfo))
    {
        if (ppdev->DefaultPalette != NULL)
            EngDeletePalette(ppdev->DefaultPalette);
        if (ppdev->PaletteEntries != NULL)
            EngFreeMem(ppdev->PaletteEntries);
        EngFreeMem(ppdev);
        return NULL;
    }

    CopySize = (cjDevInfo < sizeof(DEVINFO)) ? cjDevInfo : sizeof(DEVINFO);
    memcpy(pdi, &DevInfo, CopySize);

    CopySize = (cjCaps < sizeof(GDIINFO)) ? cjCaps : sizeof(GDIINFO);
    memcpy(pdevcaps, &GdiInfo, CopySize);

    return (DHPDEV)ppdev;
}

VOID APIENTRY
DrvCompletePDEV(IN DHPDEV dhpdev, IN HDEV hdev)
{
    ((PPDEV)dhpdev)->hDevEng = hdev;
}

VOID APIENTRY
DrvDisablePDEV(IN DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV)dhpdev;

    if (ppdev->DefaultPalette != NULL)
        EngDeletePalette(ppdev->DefaultPalette);
    if (ppdev->PaletteEntries != NULL)
        EngFreeMem(ppdev->PaletteEntries);

    ppdev->Signature = 0;
    EngFreeMem(ppdev);
}
