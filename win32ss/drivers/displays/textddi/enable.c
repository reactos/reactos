/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI display driver for text mode
 * PROGRAMMER:      Hervé Poussineau
 */

#include "textddi.h"
#include <stdarg.h>

#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
//#define NDEBUG
#include <debug.h>
#include "cursor.h"

#define ROP3_TO_ROP4(Rop3) ((((Rop3) >> 8) & 0xff00) | (((Rop3) >> 16) & 0x00ff))
#define GET_OPINDEX_FROM_ROP4(Rop4) ((Rop4) & 0xff)
#define GET_OPINDEX_FROM_ROP3(Rop3) (((Rop3) >> 16) & 0xff)

#define ENUM_RECT_LIMIT   50
typedef struct _RECT_ENUM
{
    ENUMRECTS;
    RECTL More[ENUM_RECT_LIMIT];
} RECT_ENUM;

#define CHAR_WIDTH 9
#define CHAR_HEIGHT 16
#define REAL_SCREEN_WIDTH 80
#define REAL_SCREEN_HEIGHT 25
#define SCREEN_COLOR_BITS 4
#define FAKE_SCREEN_WIDTH (REAL_SCREEN_WIDTH * CHAR_WIDTH)
#define FAKE_SCREEN_HEIGHT (REAL_SCREEN_HEIGHT * CHAR_HEIGHT)
#define SCREEN_COLORS (1 << SCREEN_COLOR_BITS)
#define FLOATL_1 0x3f800000


ULONG CDECL
DbgPrint(IN PCSTR Format, ...)
{
    va_list args;

    va_start(args, Format);
    EngDebugPrint("textddi: ", (PCHAR)Format, args);
    va_end(args);
    return 0;
}


VOID NTAPI DECLSPEC_IMPORT
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL)
{
    if (Message && *Message)
        DbgPrint("%s\n", Message);
    DbgPrint("Failed assertion %s at %s:%u\n", FailedAssertion, FileName, LineNumber);
    EngDebugBreak();
}


typedef struct tagCELL
{
    UCHAR Char;
    UCHAR Attribute;
} CELL;

typedef struct tagPDEV
{
    HANDLE hDriver;
    HDEV hdev;

    ULONG iBitmapFormat;
    SIZEL szlDisplay; /* Size of the surface (in pixels) */
    HSURF hsurf; /* Global surface */
    HSURF hsurfShadow; /* Our shadow surface, used for drawing */
    SURFOBJ *psoShadow;
    UCHAR dataShadow[FAKE_SCREEN_HEIGHT * FAKE_SCREEN_WIDTH]; /* Data of our shadow surface */
    CELL charShadow[REAL_SCREEN_HEIGHT][REAL_SCREEN_WIDTH];

    ULONG PaletteEntries[SCREEN_COLORS];

    CURSOR cur;
    PUCHAR FrameBuffer;
} PDEV, *PPDEV;


static
BOOL
RECTL_IntersectRect(
    _Out_ PRECTL prclDest,
    _In_ PRECTL prclSource1,
    _In_ PRECTL prclSource2)
{
    prclDest->left = max(prclSource1->left, prclSource2->left);
    prclDest->right = min(prclSource1->right, prclSource2->right);

    prclDest->top = max(prclSource1->top, prclSource2->top);
    prclDest->bottom = min(prclSource1->bottom, prclSource2->bottom);

    return prclDest->left <= prclDest->right && prclDest->top <= prclDest->bottom;
}


static
VOID
TextRefreshScreen(
    _In_ PPDEV pdev)
{
    RtlCopyMemory(pdev->FrameBuffer, pdev->charShadow, sizeof(pdev->charShadow));
    if (pdev->cur.Visible)
    {
        LONG x = (pdev->cur.x * REAL_SCREEN_WIDTH) / FAKE_SCREEN_WIDTH;
        LONG y = (pdev->cur.y * REAL_SCREEN_HEIGHT) / FAKE_SCREEN_HEIGHT;
        pdev->FrameBuffer[(y * REAL_SCREEN_WIDTH + x) * sizeof(CELL)] = '@';
    }
}


static DRVFN gadrvfn[] =
{
    /* Required fonctions */
    { INDEX_DrvGetModes, (PFN)DrvGetModes },
    { INDEX_DrvEnablePDEV, (PFN)DrvEnablePDEV },
    { INDEX_DrvCompletePDEV, (PFN)DrvCompletePDEV },
    { INDEX_DrvEnableSurface, (PFN)DrvEnableSurface },
    { INDEX_DrvDisableSurface, (PFN)DrvDisableSurface },
    { INDEX_DrvDisablePDEV, (PFN)DrvDisablePDEV },
    { INDEX_DrvDisableDriver, (PFN)DrvDisableDriver },
    { INDEX_DrvAssertMode, (PFN)DrvAssertMode },
    { INDEX_DrvResetDevice, (PFN)DrvResetDevice },

    /* required for device-managed surfaces */
    { INDEX_DrvCopyBits, (PFN)DrvCopyBits },
    { INDEX_DrvStrokePath, (PFN)DrvStrokePath },
    { INDEX_DrvTextOut, (PFN)DrvTextOut },

    /* Mouse support */
    { INDEX_DrvSetPointerShape, (PFN)DrvSetPointerShape },
    { INDEX_DrvMovePointer, (PFN)DrvMovePointer },

    /* Font support */
    { INDEX_DrvGetGlyphMode, (PFN)DrvGetGlyphMode },
    { INDEX_DrvQueryFontCaps, (PFN)DrvQueryFontCaps },
    { INDEX_DrvQueryFont, (PFN)DrvQueryFont },
    { INDEX_DrvQueryFontData, (PFN)DrvQueryFontData },
    { INDEX_DrvQueryFontTree, (PFN)DrvQueryFontTree },
    { INDEX_DrvLoadFontFile, (PFN)DrvLoadFontFile },
    { INDEX_DrvQueryFontFile, (PFN)DrvQueryFontFile },
    { INDEX_DrvUnloadFontFile, (PFN)DrvUnloadFontFile },

    /* Optional according to documentation, but required according to experimentation */
    { INDEX_DrvBitBlt, (PFN)DrvBitBlt },
};

BOOL
APIENTRY
DrvEnableDriver(
    IN ULONG iEngineVersion,
    IN ULONG cj,
    OUT PDRVENABLEDATA pded)
{
    DPRINT("DrvEnableDriver: iEngineVersion=0x%lx cj=%d pded=%p\n", iEngineVersion, cj, pded);

    /* Check parameter */
    if (iEngineVersion < DDI_DRIVER_VERSION_NT5 || cj < sizeof(DRVENABLEDATA))
    {
        EngSetLastError(ERROR_BAD_DRIVER_LEVEL);
        return FALSE;
    }

    /* Fill DRVENABLEDATA */
    pded->c = ARRAYSIZE(gadrvfn);
    pded->pdrvfn = gadrvfn;
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;

    /* Success */
    return TRUE;
}


static
BOOL
TextIsModeCompatible(
    _In_ PVIDEO_MODE_INFORMATION pModeInfo,
    _In_opt_ PDEVMODEW pdm)
{
    if (pModeInfo->AttributeFlags & VIDEO_MODE_GRAPHICS)
        return FALSE;
    if (pModeInfo->VisScreenWidth != FAKE_SCREEN_WIDTH || pModeInfo->VisScreenHeight != FAKE_SCREEN_HEIGHT)
        return FALSE;

    if (pdm)
    {
        if (pdm->dmPelsWidth != pModeInfo->VisScreenWidth)
            return FALSE;
        if (pdm->dmPelsHeight != pModeInfo->VisScreenHeight)
            return FALSE;
    }

    return TRUE;
}

ULONG
APIENTRY
DrvGetModes(
    IN HANDLE hDriver,
    IN ULONG cjSize,
    OUT DEVMODEW *pdm OPTIONAL)
{
    PVIDEO_MODE_INFORMATION pModeInformation, pCurrentMode;
    VIDEO_NUM_MODES VideoNumModes;
    ULONG ret, i, BytesReturned, SuitableModes = 0, AvailableCount, OutputSize = 0;

    DPRINT("DrvGetModes: hDriver=%p cjSize=%d pdm=%p\n", hDriver, cjSize, pdm);

    /* Get number of video modes */
    ret = EngDeviceIoControl(hDriver,
        IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
        NULL,
        0,
        &VideoNumModes,
        sizeof(VideoNumModes),
        &BytesReturned);
    if (ret != ERROR_SUCCESS)
    {
      DPRINT1("EngDeviceIoControl(IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES) failed with error 0x%x\n", ret);
      return 0;
    }

    /* Allocate memory to get video modes */
    pModeInformation = EngAllocMem(FL_ZERO_MEMORY, VideoNumModes.NumModes * VideoNumModes.ModeInformationLength, TAG);
    if (!pModeInformation)
    {
        DPRINT1("Failed to allocate %u * %u bytes\n", VideoNumModes.NumModes, VideoNumModes.ModeInformationLength);
        return 0;
    }

    /* Get list of video modes */
    ret = EngDeviceIoControl(hDriver,
        IOCTL_VIDEO_QUERY_AVAIL_MODES,
        NULL,
        0,
        pModeInformation,
        VideoNumModes.NumModes * VideoNumModes.ModeInformationLength,
        &BytesReturned);
    if (ret != ERROR_SUCCESS)
    {
        DPRINT1("EngDeviceIoControl(IOCTL_VIDEO_QUERY_AVAIL_MODES) failed with error 0x%x\n", ret);
        EngFreeMem(pModeInformation);
        return 0;
    }

    /* Count video modes we like */
    SuitableModes = 0;
    for (i = 0, pCurrentMode = pModeInformation;
         i < VideoNumModes.NumModes;
         i++, pCurrentMode = (PVIDEO_MODE_INFORMATION)((ULONG_PTR)pCurrentMode + VideoNumModes.ModeInformationLength))
    {
        if (TextIsModeCompatible(pCurrentMode, NULL))
            SuitableModes++;
        else
            pCurrentMode->Length = 0;
    }
    DPRINT("DrvGetModes: found %u suitables modes out of %u\n", SuitableModes, VideoNumModes.NumModes);

    if (!pdm)
    {
        /* Caller only wants to know how much memory is required */
        EngFreeMem(pModeInformation);
        return SuitableModes * sizeof(DEVMODEW);
    }

    AvailableCount = cjSize / sizeof(DEVMODEW);
    pCurrentMode = pModeInformation;
    while (AvailableCount > 0 && SuitableModes > 0)
    {
        if (pCurrentMode->Length != 0)
        {
            /* Copy mode to output buffer */
            RtlZeroMemory(pdm, sizeof(DEVMODEW));
            pdm->dmSpecVersion = DM_SPECVERSION;
            pdm->dmDriverVersion = DM_SPECVERSION;

            pdm->dmSize = sizeof(DEVMODEW);
            pdm->dmBitsPerPel = SCREEN_COLOR_BITS;
            pdm->dmPelsWidth = FAKE_SCREEN_WIDTH;
            pdm->dmPelsHeight = FAKE_SCREEN_HEIGHT;
            pdm->dmDisplayFrequency = 1;
            pdm->dmDisplayFlags = DMDISPLAYFLAGS_TEXTMODE;
            pdm->dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

            DPRINT("Good ModeIndex 0x%x (%dx%d)\n", pCurrentMode->ModeIndex, pdm->dmPelsWidth, pdm->dmPelsHeight);

            /* Go to next mode */
            OutputSize += sizeof(*pdm);
            pdm++;
            SuitableModes--;
            AvailableCount--;
        }

        pCurrentMode = (PVIDEO_MODE_INFORMATION)((PUCHAR)pCurrentMode + VideoNumModes.ModeInformationLength);
    }

    EngFreeMem(pModeInformation);
    return OutputSize;
}


DHPDEV
APIENTRY
DrvEnablePDEV(
    IN PDEVMODEW pdm,
    IN LPWSTR pwszLogAddress,
    IN ULONG cPat,
    OUT HSURF *phsurfPatterns OPTIONAL,
    IN ULONG cjCaps,
    OUT ULONG *pdevcaps,
    IN ULONG cjDevInfo,
    OUT DEVINFO *pdi,
    IN HDEV hdev,
    IN LPWSTR pwszDeviceName,
    IN HANDLE hDriver)
{
    PVIDEO_MODE_INFORMATION pModeInformation, pCurrentMode;
    VIDEO_NUM_MODES VideoNumModes;
    VIDEO_MODE VideoMode;
    VIDEO_MEMORY VideoMemory = {0};
    VIDEO_MEMORY_INFORMATION VideoMemoryInformation;
    ULONG i, BytesReturned;
    ULONG ret;
    GDIINFO GdiInfo;
    PPDEV pdev;

#define SYSTM_LOGFONT {72,72,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_DEFAULT_PRECIS|CLIP_EMBEDDED,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"TTY"}
    const DEVINFO devInfo = {
        0,
        SYSTM_LOGFONT,
        SYSTM_LOGFONT,
        SYSTM_LOGFONT,
        1,
        BMF_4BPP,
        0,
        0,
        0,
        GCAPS2_ALPHACURSOR
    };

    DPRINT("DrvEnablePDEV: hdev=%p hDriver=%p\n", hdev, hDriver);

    /* Get number of video modes */
    ret = EngDeviceIoControl(hDriver,
        IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
        NULL,
        0,
        &VideoNumModes,
        sizeof(VideoNumModes),
        &BytesReturned);
    if (ret != ERROR_SUCCESS)
    {
      DPRINT1("EngDeviceIoControl(IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES) failed with error 0x%x\n", ret);
      return NULL;
    }

    /* Allocate memory to get video modes */
    pModeInformation = EngAllocMem(FL_ZERO_MEMORY, VideoNumModes.NumModes * VideoNumModes.ModeInformationLength, TAG);
    if (!pModeInformation)
    {
        DPRINT1("Failed to allocate %u * %u bytes\n", VideoNumModes.NumModes, VideoNumModes.ModeInformationLength);
        return NULL;
    }

    /* Get list of video modes */
    ret = EngDeviceIoControl(hDriver,
        IOCTL_VIDEO_QUERY_AVAIL_MODES,
        NULL,
        0,
        pModeInformation,
        VideoNumModes.NumModes * VideoNumModes.ModeInformationLength,
        &BytesReturned);
    if (ret != ERROR_SUCCESS)
    {
        DPRINT1("EngDeviceIoControl(IOCTL_VIDEO_QUERY_AVAIL_MODES) failed with error 0x%x\n", ret);
        EngFreeMem(pModeInformation);
        return NULL;
    }

    /* Search requested mode */
    for (i = 0, pCurrentMode = pModeInformation;
         i < VideoNumModes.NumModes;
         i++, pCurrentMode = (PVIDEO_MODE_INFORMATION)((ULONG_PTR)pCurrentMode + VideoNumModes.ModeInformationLength))
    {
        if (TextIsModeCompatible(pCurrentMode, pdm))
          break;
    }
    if (i == VideoNumModes.NumModes)
    {
        DPRINT1("Failed to find a compatible mode\n");
        EngFreeMem(pModeInformation);
        return NULL;
    }

    /* Allocate and fill PDEV */
    pdev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), TAG);
    if (!pdev)
    {
        DPRINT1("Failed to allocate PDEV\n");
        EngFreeMem(pModeInformation);
        return NULL;
    }
    pdev->hDriver = hDriver;
    CURSOR_vInit(&pdev->cur);

    DPRINT("DrvEnablePDEV: requesting ModeIndex 0x%x\n", pCurrentMode->ModeIndex);
    VideoMode.RequestedMode = pCurrentMode->ModeIndex;
    ret = EngDeviceIoControl(hDriver,
        IOCTL_VIDEO_SET_CURRENT_MODE,
        &VideoMode,
        sizeof(VideoMode),
        NULL,
        0,
        &BytesReturned);
    if (ret != ERROR_SUCCESS)
    {
        DPRINT1("EngDeviceIoControl(IOCTL_VIDEO_SET_CURRENT_MODE) failed with error 0x%x\n", ret);
        EngFreeMem(pdev);
        EngFreeMem(pModeInformation);
        return NULL;
    }

    ret = EngDeviceIoControl(hDriver,
        IOCTL_VIDEO_MAP_VIDEO_MEMORY,
        &VideoMemory,
        sizeof(VideoMemory),
        &VideoMemoryInformation,
        sizeof(VideoMemoryInformation),
        &BytesReturned);
    if (ret != ERROR_SUCCESS)
    {
        DPRINT1("EngDeviceIoControl(IOCTL_VIDEO_MAP_VIDEO_MEMORY) failed with error 0x%x\n", ret);
        EngFreeMem(pdev);
        EngFreeMem(pModeInformation);
        return NULL;
    }

    pdev->szlDisplay.cx = pdm->dmPelsWidth;
    pdev->szlDisplay.cy = pdm->dmPelsHeight;
    pdev->iBitmapFormat = BMF_4BPP;
    pdev->FrameBuffer = VideoMemoryInformation.FrameBufferBase;
    pdev->PaletteEntries[0] = 0x000000;
    pdev->PaletteEntries[1] = 0x000080;
    pdev->PaletteEntries[2] = 0x008000;
    pdev->PaletteEntries[3] = 0x008080;
    pdev->PaletteEntries[4] = 0x800000;
    pdev->PaletteEntries[5] = 0x800080;
    pdev->PaletteEntries[6] = 0x808000;
    pdev->PaletteEntries[7] = 0xc0c0c0;
    pdev->PaletteEntries[8] = 0x808080;
    pdev->PaletteEntries[9] = 0x0000ff;
    pdev->PaletteEntries[10] = 0x00ff00;
    pdev->PaletteEntries[11] = 0x00ffff;
    pdev->PaletteEntries[12] = 0xff0000;
    pdev->PaletteEntries[13] = 0xff00ff;
    pdev->PaletteEntries[14] = 0xffff00;
    pdev->PaletteEntries[15] = 0xffffff;

    /* Fill GDIINFO */
    RtlZeroMemory(&GdiInfo, sizeof(GdiInfo));
    GdiInfo.ulVersion = 0x5000;
    GdiInfo.ulTechnology = DT_CHARSTREAM;
    GdiInfo.ulHorzSize = 320;
    GdiInfo.ulVertSize = 240;
    GdiInfo.ulHorzRes = pdm->dmPelsWidth;
    GdiInfo.ulVertRes = pdm->dmPelsHeight;
    GdiInfo.ulLogPixelsX = 32;
    GdiInfo.ulLogPixelsY = 96;
    GdiInfo.cBitsPixel = SCREEN_COLOR_BITS;
    GdiInfo.ulNumColors = SCREEN_COLORS;
    GdiInfo.cPlanes = 1;
    GdiInfo.xStyleStep = GdiInfo.yStyleStep = GdiInfo.denStyleStep = 1;
    if (cjCaps < sizeof(GDIINFO))
        RtlCopyMemory(pdevcaps, &GdiInfo, cjCaps);
    else
        RtlCopyMemory(pdevcaps, &GdiInfo, sizeof(GdiInfo));

    /* Fill DEVINFO */
    *pdi = devInfo;
    pdi->hpalDefault = EngCreatePalette(PAL_INDEXED, ARRAYSIZE(pdev->PaletteEntries), pdev->PaletteEntries, 0, 0, 0);
    if (!pdi->hpalDefault)
    {
        DPRINT1("EngCreatePalette() failed\n");
        EngFreeMem(pdev);
        EngFreeMem(pModeInformation);
        return NULL;
    }

    EngFreeMem(pModeInformation);
    return (DHPDEV)pdev;
}


VOID
APIENTRY
DrvCompletePDEV(
    IN DHPDEV dhpdev,
    IN HDEV hdev)
{
    PPDEV pdev = (PPDEV)dhpdev;
    DPRINT("DrvCompletePDEV: dhpdev=%p hdev=%p\n", dhpdev, hdev);
    pdev->hdev = hdev;
}


VOID
APIENTRY
DrvDisableSurface(
    IN DHPDEV dhpdev)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
}


HSURF
APIENTRY
DrvEnableSurface(
    IN DHPDEV dhpdev)
{
    PPDEV pdev = (PPDEV)dhpdev;

    DPRINT("DrvEnableSurface: dhpdev=%p\n", dhpdev);

    /* Create main surface */
    pdev->hsurf = EngCreateDeviceSurface(NULL, pdev->szlDisplay, pdev->iBitmapFormat);
    if (!pdev->hsurf)
    {
        DPRINT1("EngCreateDeviceSurface() failed\n");
        goto failure;
    }

    if (!EngAssociateSurface(pdev->hsurf, pdev->hdev, HOOK_COPYBITS | HOOK_BITBLT | HOOK_STROKEPATH | HOOK_TEXTOUT))
    {
        DPRINT1("EngAssociateSurface() failed\n");
        goto failure;
    }

    /* Create shadow surface */
    pdev->hsurfShadow = (HSURF)EngCreateBitmap(pdev->szlDisplay, pdev->szlDisplay.cx, BMF_4BPP, BMF_TOPDOWN, pdev->dataShadow);
    if (!pdev->hsurfShadow)
    {
        DPRINT1("EngCreateBitmap() failed\n");
        goto failure;
    }

    pdev->psoShadow = EngLockSurface(pdev->hsurfShadow);
    if (!pdev->psoShadow)
    {
        DPRINT1("EngLockSurface() failed\n");
        goto failure;
    }

    return pdev->hsurf;

failure:
    DrvDisableSurface(dhpdev);
    return NULL;
}


VOID
APIENTRY
DrvDisablePDEV(
    IN DHPDEV dhpdev)
{
    PPDEV pdev = (PPDEV)dhpdev;

    UNIMPLEMENTED;
    //ASSERT(FALSE);

    EngFreeMem(pdev);
}


VOID
APIENTRY
DrvDisableDriver(VOID)
{
    /* Nothing to do */
}


BOOL
APIENTRY
DrvAssertMode(
    IN DHPDEV dhpdev,
    IN BOOL bEnable)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
    return TRUE;
}


ULONG
APIENTRY
DrvResetDevice(
    IN DHPDEV dhpdev,
    PVOID Reserved)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
    return DRD_ERROR;
}


ULONG
APIENTRY
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
    PPDEV pdev = (PPDEV)pso->dhpdev;
    DPRINT("DrvSetPointerShape: pso=%p\n", pso);

    if (psoMask)
    {
        CURSOR_SetPosition(&pdev->cur, x, y);
        CURSOR_SetVisible(&pdev->cur, TRUE);
    }
    else
        CURSOR_SetVisible(&pdev->cur, FALSE);
    TextRefreshScreen(pdev);

    return SPS_ACCEPT_NOEXCLUDE;
}


VOID
APIENTRY
DrvMovePointer(
    IN SURFOBJ *pso,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl OPTIONAL)
{
    PPDEV pdev = (PPDEV)pso->dhpdev;
    //DPRINT("DrvMovePointer: x=%ld y=%ld\n", x, y);

    if (x >= 0)
    {
        CURSOR_SetPosition(&pdev->cur, x, y);
        CURSOR_SetVisible(&pdev->cur, TRUE);
    }
    else
        CURSOR_SetVisible(&pdev->cur, FALSE);
    TextRefreshScreen(pdev);
}


ULONG
APIENTRY
DrvGetGlyphMode(
    IN DHPDEV dhpdev,
    IN FONTOBJ *pfo)
{
    /* We do all the font caching */
    DPRINT("DrvGetGlyphMode\n");
    return FO_HGLYPHS;
}


#define TO_FIX(x) ((x) * 16)
static LONG
TextQueryMaxExtents(
    OUT PFD_DEVICEMETRICS pfddm,
    IN ULONG cjSize)
{
    if (pfddm)
    {
        if (cjSize < sizeof(FD_DEVICEMETRICS))
        {
            /* Not enough space, fail */
            return FD_ERROR;
        }

        /* Fill FD_DEVICEMETRICS */
        pfddm->flRealizedType = FDM_TYPE_BM_SIDE_CONST |
                                FDM_TYPE_CHAR_INC_EQUAL_BM_BASE |
                                FDM_TYPE_CONST_BEARINGS |
                                FDM_TYPE_MAXEXT_EQUAL_BM_SIDE |
                                FDM_TYPE_ZERO_BEARINGS;
        pfddm->pteBase.x = FLOATL_1;
        pfddm->pteSide.y = FLOATL_1;
        pfddm->lD = CHAR_WIDTH;
        pfddm->fxMaxAscender = TO_FIX(CHAR_HEIGHT);
        pfddm->cxMax = CHAR_WIDTH;
        pfddm->cyMax = CHAR_HEIGHT;
        pfddm->cjGlyphMax = FIELD_OFFSET(GLYPHBITS, aj) + (CHAR_WIDTH * CHAR_HEIGHT + 7) / 8;
        pfddm->fdxQuantized.eXX = 1;
        pfddm->fdxQuantized.eYY = 1;
    }

    /* Return the size of the structure */
    return sizeof(FD_DEVICEMETRICS);
}


static ULONG
TextQueryGlyphAndBitmap(
    IN HGLYPH hg,
    OUT GLYPHDATA *pgd,
    OUT GLYPHBITS *pgb OPTIONAL,
    IN ULONG cjSize)
{
    ASSERT(pgb == NULL); // we don't support glyph bitmaps
    if (pgd)
    {
        pgd->hg = hg;
        pgd->fxD = TO_FIX(CHAR_WIDTH);
        pgd->fxAB = TO_FIX(CHAR_WIDTH);
        pgd->fxInkTop = TO_FIX(CHAR_HEIGHT);
        pgd->rclInk.top = CHAR_HEIGHT;
        pgd->rclInk.right = CHAR_WIDTH * 16;
        pgd->ptqD.x.HighPart = pgd->fxD;
    }
    return FIELD_OFFSET(GLYPHBITS, aj) + (CHAR_WIDTH * CHAR_HEIGHT + 7) / 8;
}


LONG
APIENTRY
DrvQueryFontData(
    IN DHPDEV dhpdev,
    IN FONTOBJ *pfo,
    IN ULONG iMode,
    IN HGLYPH hg,
    IN GLYPHDATA *pgd,
    OUT PVOID pv,
    IN ULONG cjSize)
{
    DPRINT("DrvQueryFontData(iMode %u hg 0x%x '%c')\n", iMode, hg, hg);

    switch (iMode)
    {
        case QFD_GLYPHANDBITMAP:
            return TextQueryGlyphAndBitmap(hg, pgd, pv, cjSize);

        case QFD_MAXEXTENTS:
            return TextQueryMaxExtents(pv, cjSize);

            /* we support nothing else */
        default:
            return FD_ERROR;

    }

    return FD_ERROR;
}


PVOID
APIENTRY
DrvQueryFontTree(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    IN ULONG iMode,
    OUT ULONG_PTR *pid)
{
    static FD_GLYPHSET IdentityGlyphSet = { sizeof(FD_GLYPHSET), GS_UNICODE_HANDLES, 107, 1, {{ L' ', 107, NULL }} };
    static FD_KERNINGPAIR NullKerningPair = {0};

    DPRINT("DrvQueryFontTree(iMode %u)\n", iMode);

    /* We only support one font (as described in DEVINFO) */
    ASSERT(iFile == 0);
    ASSERT(iFace == 1);

    if (iMode == QFT_GLYPHSET)
        return &IdentityGlyphSet;
    else if (iMode == QFT_KERNPAIRS)
        return &NullKerningPair;
    else
        return NULL;
}


LONG
APIENTRY
DrvQueryFontCaps(
    IN ULONG culCaps,
    OUT ULONG *pulCaps)
{
    DPRINT("DrvQueryFontCaps(culCaps %u)\n", culCaps);

    if (culCaps < 2)
        return FD_ERROR;

    pulCaps[0] = 1;
    pulCaps[1] = QC_1BIT;
    return 2;
}


PIFIMETRICS
APIENTRY
DrvQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    OUT ULONG_PTR *pid)
{
    typedef struct tagDRVIFIMETRICS
    {
        IFIMETRICS m;
        CHAR ajCharSet[16]; // FIXME: IFIEXTRA e;
        WCHAR FamilyName[4];
        WCHAR StyleName[7];
    } DRVIFIMETRICS;
    static DRVIFIMETRICS s;

    DPRINT("DrvQueryFont(iFile %p iFace %u)\n", iFile, iFace);

    if (iFile == 0 && iFace == 0)
    {
        /* We support only one font */
        return (PIFIMETRICS)(ULONG_PTR)1;
    }

    /* We only support one font (as described in DEVINFO) */
    ASSERT(iFile == 0);
    ASSERT(iFace == 1);

    /* Fill IFIMETRICS */
    RtlZeroMemory(&s, sizeof(s));
    s.m.cjThis = sizeof(s);
    wcscpy(s.FamilyName, L"TTY");
    wcscpy(s.StyleName, L"Normal");
    s.m.dpwszFamilyName = FIELD_OFFSET(DRVIFIMETRICS, FamilyName);
    s.m.dpwszStyleName = FIELD_OFFSET(DRVIFIMETRICS, StyleName);
    s.m.dpwszFaceName = s.m.dpwszFamilyName;
    s.m.jWinCharSet = OEM_CHARSET;
    s.m.jWinPitchAndFamily = FF_MODERN | FIXED_PITCH;
    s.m.usWinWeight = 500;
    s.m.flInfo = FM_INFO_1BPP |
        FM_INFO_CONSTANT_WIDTH |
        FM_INFO_OPTICALLY_FIXED_PITCH |
        //FM_INFO_INTEGER_WIDTH |
        FM_INFO_NONNEGATIVE_AC |
        FM_INFO_TECH_BITMAP;
    s.m.fwdUnitsPerEm = 100;
    s.m.fwdWinAscender = 1;
    s.m.fwdWinDescender = s.m.fwdUnitsPerEm - s.m.fwdWinAscender;
    s.m.fwdMacAscender = s.m.fwdWinAscender;
    s.m.fwdMacDescender = s.m.fwdWinDescender;
    s.m.fwdTypoAscender = s.m.fwdWinAscender;
    s.m.fwdTypoDescender = s.m.fwdWinDescender;
    s.m.fwdAveCharWidth = CHAR_WIDTH;
    s.m.fwdMaxCharInc = 1;
    s.m.fwdCapHeight = s.m.fwdUnitsPerEm / 2;
    s.m.fwdXHeight = s.m.fwdUnitsPerEm / 4;
    s.m.chFirstChar = 20;
    s.m.chLastChar = 127;
    s.m.chDefaultChar = 'x';
    s.m.wcFirstChar = ' ';
    s.m.wcLastChar = 127;
    s.m.wcDefaultChar = 'x';
    s.m.ptlBaseline.x = 1;
    s.m.ptlCaret.y = 1;
    s.m.rclFontBox.right = s.m.fwdAveCharWidth;
    s.m.rclFontBox.right = s.m.fwdWinAscender;
    s.m.rclFontBox.bottom = s.m.fwdWinDescender;
    s.m.ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
    s.m.panose.bFamilyType = PAN_FAMILY_TEXT_DISPLAY;

    /* Set char sets */
    s.ajCharSet[0] = s.m.jWinCharSet;
    s.ajCharSet[1] = DEFAULT_CHARSET;

    return &s.m;
}


#define MASK1BPP(x) (1<<(7-((x)&7)))
static
ULONG
SURFOBJ_1BPP_GetPixel(
    IN SURFOBJ *pso,
    IN ULONG x,
    IN ULONG y)
{
    PBYTE p = (PBYTE)pso->pvScan0 + y * pso->lDelta + (x >> 3);
    return *p & MASK1BPP(x) ? 0 : 1;
}


#define SHIFT4BPP(x) ((1-(x&1))<<2)
static
ULONG
SURFOBJ_4BPP_GetPixel(
    IN SURFOBJ *pso,
    IN ULONG x,
    IN ULONG y)
{
    PBYTE p = (PBYTE)pso->pvScan0 + y * pso->lDelta + (x >> 1);
    return (*p >> SHIFT4BPP(x)) & 0xf;
}


static
ULONG
SURFOBJ_8BPP_GetPixel(
    IN SURFOBJ *pso,
    IN ULONG x,
    IN ULONG y)
{
    PBYTE p = (PBYTE)pso->pvScan0 + y * pso->lDelta + x;
    return *p;
}


static
BOOL
TextBitBlt(
    IN PPDEV pdev,
    IN OUT SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc OPTIONAL,
    IN SURFOBJ *psoMask OPTIONAL,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo OPTIONAL,
    IN RECTL* prclTrg,
    IN POINTL *pptlSrc OPTIONAL,
    IN POINTL *pptlMask OPTIONAL,
    IN BRUSHOBJ *pbo OPTIONAL,
    IN POINTL* pptlBrush OPTIONAL,
    IN ROP4 rop4)
{
    LONG fakeTrgX, fakeTrgY, trgX, trgY;

    /* Update output screen */
    for (fakeTrgX = prclTrg->left; fakeTrgX < prclTrg->right; /*fakeTrgX++*/ fakeTrgX += CHAR_WIDTH)
    {
        for (fakeTrgY = prclTrg->top; fakeTrgY < prclTrg->bottom; /*fakeTrgY++*/ fakeTrgY += CHAR_HEIGHT)
        {
            trgX = (fakeTrgX * REAL_SCREEN_WIDTH) / FAKE_SCREEN_WIDTH;
            trgY = (fakeTrgY * REAL_SCREEN_HEIGHT) / FAKE_SCREEN_HEIGHT;
            if (trgX >= 0 && trgY >= 0 && trgX < REAL_SCREEN_WIDTH && trgY < REAL_SCREEN_HEIGHT)
            {
                ULONG Color;

                if (rop4 == ROP3_TO_ROP4(PATCOPY))
                {
                    if (pbo->iSolidColor == 0xFFFFFFFF)
                        return FALSE;
                    Color = pbo->iSolidColor;
                }
                else if (!pptlSrc || !psoSrc)
                    continue;
                else
                {
                    ULONG Color2;
                    switch (psoSrc->iBitmapFormat)
                    {
                        case BMF_1BPP:
                            Color2 = SURFOBJ_1BPP_GetPixel(psoSrc, pptlSrc->x + fakeTrgX - prclTrg->left, pptlSrc->y + fakeTrgY - prclTrg->top);
                            Color = XLATEOBJ_iXlate(pxlo, Color2);
                            break;
                        case BMF_4BPP:
                            Color2 = SURFOBJ_4BPP_GetPixel(psoSrc, pptlSrc->x + fakeTrgX - prclTrg->left, pptlSrc->y + fakeTrgY - prclTrg->top);
                            Color = XLATEOBJ_iXlate(pxlo, Color2);
                            break;
                        case BMF_8BPP:
                            Color2 = SURFOBJ_8BPP_GetPixel(psoSrc, pptlSrc->x + fakeTrgX - prclTrg->left, pptlSrc->y + fakeTrgY - prclTrg->top);
                            Color = XLATEOBJ_iXlate(pxlo, Color2);
                            break;
                        default:
                            DPRINT1("Unknown SURFOBJ format %d\n", psoSrc->iBitmapFormat);
                            ASSERT(FALSE);
                            Color = 0;
                            continue;
                    }
                    if (Color < 0 || Color >= 16)
                    {
                        DPRINT1("iBitmapFormat=%d GetPixel=%d Color=%d\n", psoSrc->iBitmapFormat, Color2, Color);
                    }
                }
                ASSERT(Color >= 0 && Color < 16);
                pdev->charShadow[trgY][trgX].Char = ' ';
                pdev->charShadow[trgY][trgX].Attribute = (Color << 4) + (Color == 7 ? 0 : 7);
            }
        }
    }

    return TRUE;
}


BOOL
APIENTRY
DrvBitBlt(
    IN OUT SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc OPTIONAL,
    IN SURFOBJ *psoMask OPTIONAL,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo OPTIONAL,
    IN RECTL* prclTrg,
    IN POINTL *pptlSrc OPTIONAL,
    IN POINTL *pptlMask OPTIONAL,
    IN BRUSHOBJ *pbo OPTIONAL,
    IN POINTL* pptlBrush OPTIONAL,
    IN ROP4 rop4)
{
    PPDEV pdev;
    BOOL res;
    BOOL bNeedUpdate = FALSE;

    //DPRINT("DrvBitBlt (rop4 0x%x rclTrg=(%d %d)-(%d %d))\n", rop4, prclTrg->left, prclTrg->top, prclTrg->right, prclTrg->bottom);

    if (psoTrg->iType == STYPE_DEVICE)
    {
        pdev = (PPDEV)psoTrg->dhpdev;
        psoTrg = pdev->psoShadow;
        bNeedUpdate = TRUE;
    }
    if (psoSrc && psoSrc->iType == STYPE_DEVICE)
    {
        pdev = (PPDEV)psoSrc->dhpdev;
        psoSrc = pdev->psoShadow;
    }

    res = EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);

    if (res && bNeedUpdate)
    {
        RECTL rclTarget;
        POINTL ptlSource = {0};

        /* Update output screen */
        switch (pco ? pco->iDComplexity : DC_TRIVIAL)
        {
            case DC_TRIVIAL:
                res = TextBitBlt(pdev, psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc, pptlMask, pbo, pptlBrush, rop4);
                break;

            case DC_RECT:
            {
                if (RECTL_IntersectRect(&rclTarget, prclTrg, &pco->rclBounds))
                {
                    if (psoSrc && pptlSrc)
                        ptlSource = *pptlSrc;
                    ptlSource.x += rclTarget.left - prclTrg->left;
                    ptlSource.y += rclTarget.top - prclTrg->top;
                    res &= TextBitBlt(pdev, psoTrg, psoSrc, psoMask, pco, pxlo, &rclTarget, &ptlSource, pptlMask, pbo, pptlBrush, rop4);
                }
                break;
            }

            case DC_COMPLEX:
            {
                RECT_ENUM RectEnum;
                ULONG i;
                BOOL bEnumMore;

                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
                do
                {
                    bEnumMore = CLIPOBJ_bEnum(pco, sizeof(RectEnum), (PVOID)&RectEnum);
                    for (i = 0; i < RectEnum.c; i++)
                    {
                        if (RECTL_IntersectRect(&rclTarget, prclTrg, &RectEnum.arcl[i]))
                        {
                            if (psoSrc && pptlSrc)
                                ptlSource = *pptlSrc;
                            ptlSource.x += rclTarget.left - prclTrg->left;
                            ptlSource.y += rclTarget.top - prclTrg->top;
                            res &= TextBitBlt(pdev, psoTrg, psoSrc, psoMask, pco, pxlo, &rclTarget, &ptlSource, pptlMask, pbo, pptlBrush, rop4);
                        }
                    }
                } while (bEnumMore);
                break;
            }
        }

        TextRefreshScreen(pdev);
    }

    return res;
}


BOOL
APIENTRY
DrvCopyBits(
    _Inout_ SURFOBJ *psoDest,
    _In_opt_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ PRECTL prclDest,
    _In_opt_ PPOINTL pptlSrc)
{
    /* Easy. Just call the more general function DrvBitBlt */
    return DrvBitBlt(psoDest, psoSrc, NULL, pco, pxlo, prclDest, pptlSrc, NULL, NULL, NULL, ROP3_TO_ROP4(SRCCOPY));
}


BOOL
APIENTRY
DrvStrokePath(
     _Inout_ SURFOBJ *pso,
     _In_ PATHOBJ *ppo,
     _In_ CLIPOBJ *pco,
     _In_opt_ XFORMOBJ *pxo,
     _In_ BRUSHOBJ *pbo,
     _In_ PPOINTL pptlBrushOrg,
     _In_ PLINEATTRS plineattrs,
     _In_ MIX mix)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
    return FALSE;
}


BOOL
APIENTRY
DrvTextOut(
    IN SURFOBJ *pso,
    IN STROBJ *pstro,
    IN FONTOBJ *pfo,
    IN CLIPOBJ *pco,
    IN RECTL *prclExtra,
    IN RECTL *prclOpaque,
    IN BRUSHOBJ *fboFore,
    IN BRUSHOBJ *fboOpaque,
    IN POINTL *pptlOrg,
    IN MIX mix)
{
    PPDEV pdev = (PPDEV)pso->dhpdev;
    PGLYPHPOS pgp;
    ULONG cGlyphs;
    LONG x, y;
    LONG dx = 0, dy = 0;
    BOOL ret = TRUE, Continue;
    BOOL First = TRUE;
    LONG trgX, trgY;

    DPRINT1("DrvTextOut(iFile %p iFace %u sizLogResPpi %d ulStyleSize %u)\n", pfo->iFile, pfo->iFace, pfo->sizLogResPpi, pfo->ulStyleSize);

    STROBJ_vEnumStart(pstro);

    if (pstro->flAccel & SO_HORIZONTAL)
        dx = pstro->ulCharInc * (pstro->flAccel & SO_REVERSED ? -1 : 1);
    if (pstro->flAccel & SO_VERTICAL)
        dy = pstro->ulCharInc * (pstro->flAccel & SO_REVERSED ? -1 : 1);

    do
    {
        /* Get (part of) the string to display */
        if (pstro->pgp)
        {
            pgp = pstro->pgp;
            cGlyphs = pstro->cGlyphs;
            Continue = FALSE;
        }
        else
            Continue = STROBJ_bEnumPositionsOnly(pstro, &cGlyphs, &pgp);


        /* Display string */
        ret = (cGlyphs > 0);
        for (; cGlyphs > 0; cGlyphs--, pgp++)
        {
            if (First || (dx == 0 && dy == 0))
            {
                x = pgp->ptl.x;
                y = pgp->ptl.y;
                First = FALSE;
            }
            else
            {
                x += dx;
                y += dy;
            }
            trgX = (x * REAL_SCREEN_WIDTH) / FAKE_SCREEN_WIDTH;
            trgY = (y * REAL_SCREEN_HEIGHT) / FAKE_SCREEN_HEIGHT;
            pdev->charShadow[trgY][trgX].Char = pgp->hg;
        }
    } while (Continue && ret);

    TextRefreshScreen(pdev);

    return ret;
}


ULONG_PTR
APIENTRY
DrvLoadFontFile(
    IN ULONG cFiles,
    IN ULONG_PTR *piFiles,
    IN PVOID *ppvView,
    IN ULONG *pcjView,
    IN DESIGNVECTOR *pdv,
    IN ULONG ulLangID,
    IN ULONG ulFastCheckSum)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
    return HFF_INVALID;
}


LONG
APIENTRY
DrvQueryFontFile(
    IN ULONG_PTR iFile,
    IN ULONG ulMode,
    IN ULONG cjBuf,
    OUT ULONG *pulBuf)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
    return FD_ERROR;
}


BOOL
APIENTRY
DrvUnloadFontFile(
    IN ULONG_PTR iFile)
{
    UNIMPLEMENTED;
    //ASSERT(FALSE);
    return FALSE;
}
