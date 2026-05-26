

#include "xboxdisp.h"

static DRVFN DrvFunctionTable[] =
{
    {INDEX_DrvEnablePDEV,        (PFN)DrvEnablePDEV},
    {INDEX_DrvCompletePDEV,      (PFN)DrvCompletePDEV},
    {INDEX_DrvDisablePDEV,       (PFN)DrvDisablePDEV},
    {INDEX_DrvEnableSurface,     (PFN)DrvEnableSurface},
    {INDEX_DrvDisableSurface,    (PFN)DrvDisableSurface},
    {INDEX_DrvAssertMode,        (PFN)DrvAssertMode},
    {INDEX_DrvGetModes,          (PFN)DrvGetModes},
    {INDEX_DrvSetPalette,        (PFN)DrvSetPalette},
    {INDEX_DrvSetPointerShape,   (PFN)DrvSetPointerShape},
    {INDEX_DrvMovePointer,       (PFN)DrvMovePointer},
    {INDEX_DrvBitBlt,            (PFN)DrvBitBlt},
    {INDEX_DrvCopyBits,          (PFN)DrvCopyBits},
    {INDEX_DrvCreateDeviceBitmap,(PFN)DrvCreateDeviceBitmap},
    {INDEX_DrvDeleteDeviceBitmap,(PFN)DrvDeleteDeviceBitmap},
    {INDEX_DrvEscape,            (PFN)DrvEscape},
    {INDEX_DrvEnableDirectDraw,  (PFN)DrvEnableDirectDraw},
    {INDEX_DrvDisableDirectDraw, (PFN)DrvDisableDirectDraw},
};

BOOL APIENTRY
DrvEnableDriver(
    ULONG iEngineVersion,
    ULONG cj,
    PDRVENABLEDATA pded)
{
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
    NV2A_CAPS caps;
    DWORD outBytes;

    ppdev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
    if (ppdev == NULL)
        return NULL;

    ppdev->hDriver = hDriver;

    if (!IntInitScreenInfo(ppdev, pdm, &GdiInfo, &DevInfo))
    {
        EngFreeMem(ppdev);
        return NULL;
    }

    if (!IntInitDefaultPalette(ppdev, &DevInfo))
    {
        EngFreeMem(ppdev);
        return NULL;
    }

    /* Best-effort: ask the miniport whether the GPU 2D engine is live.  The
     * value only affects logging — IOCTLs fall through to a CPU path if the
     * GPU pipeline is off. */
    RtlZeroMemory(&caps, sizeof(caps));
    caps.StructSize = sizeof(caps);
    if (!EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_NV2A_QUERY_CAPS,
                            NULL, 0, &caps, sizeof(caps), &outBytes) &&
        outBytes >= sizeof(caps))
    {
        ppdev->AccelAvailable = TRUE;
        ppdev->AccelHardware = (caps.HardwareAccelEnabled != 0);

        /* Seed the offscreen device-bitmap heap with the free span the miniport
         * reported (absolute GPU offsets).  VramBase is filled in once the
         * framebuffer is mapped in DrvEnableSurface. */
        ppdev->FbGpuOffset = caps.FrameBufferGpuOffset;
        if (caps.OffscreenHeapSize != 0)
        {
            ppdev->HeapFree[0].Off  = caps.OffscreenHeapStart;
            ppdev->HeapFree[0].Size = caps.OffscreenHeapSize;
            ppdev->HeapSpanCount    = 1;
        }
    }

    memcpy(pdi, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));
    memcpy(pdevcaps, &GdiInfo, min(sizeof(GDIINFO), cjCaps));

    return (DHPDEV)ppdev;
}

VOID APIENTRY
DrvCompletePDEV(
    IN DHPDEV dhpdev,
    IN HDEV hdev)
{
    ((PPDEV)dhpdev)->hDevEng = hdev;
}

VOID APIENTRY
DrvDisablePDEV(
    IN DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV)dhpdev;

    if (ppdev->DefaultPalette)
        EngDeletePalette(ppdev->DefaultPalette);
    if (ppdev->PaletteEntries != NULL)
        EngFreeMem(ppdev->PaletteEntries);

    EngFreeMem(ppdev);
}
