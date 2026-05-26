/*
 * PROJECT:     Xbox NV2A accelerated GDI display driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Screen / mode helpers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "xboxdisp.h"

static LOGFONTW SystemFont       = { 16, 7, 0, 0, 700, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"System" };
static LOGFONTW AnsiVariableFont = { 12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif" };
static LOGFONTW AnsiFixedFont    = { 12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DONTCARE, L"Courier" };

static DWORD
GetAvailableModes(HANDLE hDriver,
                  PVIDEO_MODE_INFORMATION *ModeInfo,
                  DWORD *ModeInfoSize)
{
    ULONG ulTemp;
    VIDEO_NUM_MODES Modes;
    PVIDEO_MODE_INFORMATION ModeInfoPtr;

    if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES, NULL, 0,
                           &Modes, sizeof(VIDEO_NUM_MODES), &ulTemp))
        return 0;
    if (Modes.NumModes == 0)
        return 0;

    *ModeInfoSize = Modes.ModeInformationLength;
    *ModeInfo = EngAllocMem(0, Modes.NumModes * Modes.ModeInformationLength, ALLOC_TAG);
    if (*ModeInfo == NULL)
        return 0;

    if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_AVAIL_MODES, NULL, 0,
                           *ModeInfo, Modes.NumModes * Modes.ModeInformationLength,
                           &ulTemp))
    {
        EngFreeMem(*ModeInfo);
        *ModeInfo = NULL;
        return 0;
    }

    ulTemp = Modes.NumModes;
    ModeInfoPtr = *ModeInfo;
    while (ulTemp--)
    {
        if ((ModeInfoPtr->NumberOfPlanes != 1) ||
            !(ModeInfoPtr->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((ModeInfoPtr->BitsPerPlane != 8) &&
             (ModeInfoPtr->BitsPerPlane != 16) &&
             (ModeInfoPtr->BitsPerPlane != 24) &&
             (ModeInfoPtr->BitsPerPlane != 32)))
        {
            ModeInfoPtr->Length = 0;
        }
        ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((PUCHAR)ModeInfoPtr) + Modes.ModeInformationLength);
    }
    return Modes.NumModes;
}

BOOL
IntInitScreenInfo(PPDEV ppdev, LPDEVMODEW pDevMode,
                  PGDIINFO pGdiInfo, PDEVINFO pDevInfo)
{
    ULONG ModeCount;
    ULONG ModeInfoSize;
    PVIDEO_MODE_INFORMATION ModeInfo, ModeInfoPtr, SelectedMode = NULL;

    ModeCount = GetAvailableModes(ppdev->hDriver, &ModeInfo, &ModeInfoSize);
    if (ModeCount == 0)
        return FALSE;

    if (pDevMode->dmPelsWidth == 0 && pDevMode->dmPelsHeight == 0 &&
        pDevMode->dmBitsPerPel == 0 && pDevMode->dmDisplayFrequency == 0)
    {
        ModeInfoPtr = ModeInfo;
        while (ModeCount-- > 0)
        {
            if (ModeInfoPtr->Length == 0)
            {
                ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((PUCHAR)ModeInfoPtr) + ModeInfoSize);
                continue;
            }
            SelectedMode = ModeInfoPtr;
            break;
        }
    }
    else
    {
        ModeInfoPtr = ModeInfo;
        while (ModeCount-- > 0)
        {
            if (ModeInfoPtr->Length > 0 &&
                pDevMode->dmPelsWidth == ModeInfoPtr->VisScreenWidth &&
                pDevMode->dmPelsHeight == ModeInfoPtr->VisScreenHeight &&
                pDevMode->dmBitsPerPel == (ModeInfoPtr->BitsPerPlane * ModeInfoPtr->NumberOfPlanes) &&
                pDevMode->dmDisplayFrequency == ModeInfoPtr->Frequency)
            {
                SelectedMode = ModeInfoPtr;
                break;
            }
            ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((PUCHAR)ModeInfoPtr) + ModeInfoSize);
        }
    }

    if (SelectedMode == NULL)
    {
        EngFreeMem(ModeInfo);
        return FALSE;
    }

    ppdev->ModeIndex   = SelectedMode->ModeIndex;
    ppdev->ScreenWidth = SelectedMode->VisScreenWidth;
    ppdev->ScreenHeight= SelectedMode->VisScreenHeight;
    ppdev->ScreenDelta = SelectedMode->ScreenStride;
    ppdev->BitsPerPixel= (UCHAR)(SelectedMode->BitsPerPlane * SelectedMode->NumberOfPlanes);
    ppdev->RedMask     = SelectedMode->RedMask;
    ppdev->GreenMask   = SelectedMode->GreenMask;
    ppdev->BlueMask    = SelectedMode->BlueMask;

    pGdiInfo->ulVersion         = GDI_DRIVER_VERSION;
    pGdiInfo->ulTechnology      = DT_RASDISPLAY;
    pGdiInfo->ulHorzSize        = SelectedMode->XMillimeter;
    pGdiInfo->ulVertSize        = SelectedMode->YMillimeter;
    pGdiInfo->ulHorzRes         = SelectedMode->VisScreenWidth;
    pGdiInfo->ulVertRes         = SelectedMode->VisScreenHeight;
    pGdiInfo->ulPanningHorzRes  = SelectedMode->VisScreenWidth;
    pGdiInfo->ulPanningVertRes  = SelectedMode->VisScreenHeight;
    pGdiInfo->cBitsPixel        = SelectedMode->BitsPerPlane;
    pGdiInfo->cPlanes           = SelectedMode->NumberOfPlanes;
    pGdiInfo->ulVRefresh        = SelectedMode->Frequency;
    pGdiInfo->ulBltAlignment    = 1;
    pGdiInfo->ulLogPixelsX      = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY      = pDevMode->dmLogPixels;
    pGdiInfo->flTextCaps        = TC_RA_ABLE;
    pGdiInfo->flRaster          = 0;
    pGdiInfo->ulDACRed          = SelectedMode->NumberRedBits;
    pGdiInfo->ulDACGreen        = SelectedMode->NumberGreenBits;
    pGdiInfo->ulDACBlue         = SelectedMode->NumberBlueBits;
    pGdiInfo->ulAspectX         = 0x24;
    pGdiInfo->ulAspectY         = 0x24;
    pGdiInfo->ulAspectXY        = 0x33;
    pGdiInfo->xStyleStep        = 1;
    pGdiInfo->yStyleStep        = 1;
    pGdiInfo->denStyleStep      = 3;
    pGdiInfo->ptlPhysOffset.x   = 0;
    pGdiInfo->ptlPhysOffset.y   = 0;
    pGdiInfo->szlPhysSize.cx    = 0;
    pGdiInfo->szlPhysSize.cy    = 0;

    /* Sensible default chromaticities (sRGB-ish). */
    pGdiInfo->ciDevice.Red.x = 6700;   pGdiInfo->ciDevice.Red.y = 3300;
    pGdiInfo->ciDevice.Green.x = 2100; pGdiInfo->ciDevice.Green.y = 7100;
    pGdiInfo->ciDevice.Blue.x = 1400;  pGdiInfo->ciDevice.Blue.y = 800;
    pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
    pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
    pGdiInfo->ciDevice.AlignmentWhite.Y = 0;
    pGdiInfo->ciDevice.RedGamma   = 20000;
    pGdiInfo->ciDevice.GreenGamma = 20000;
    pGdiInfo->ciDevice.BlueGamma  = 20000;
    pGdiInfo->ulDevicePelsDPI     = 0;
    pGdiInfo->ulPrimaryOrder      = PRIMARY_ORDER_CBA;
    pGdiInfo->ulHTPatternSize     = HT_PATSIZE_4x4_M;
    pGdiInfo->flHTFlags           = HT_FLAG_ADDITIVE_PRIMS;

    pDevInfo->flGraphicsCaps = 0;
    pDevInfo->lfDefaultFont  = SystemFont;
    pDevInfo->lfAnsiVarFont  = AnsiVariableFont;
    pDevInfo->lfAnsiFixFont  = AnsiFixedFont;
    pDevInfo->cFonts         = 0;
    pDevInfo->cxDither       = 0;
    pDevInfo->cyDither       = 0;
    pDevInfo->hpalDefault    = 0;
    pDevInfo->flGraphicsCaps2= 0;

    if (ppdev->BitsPerPixel == 8)
    {
        pGdiInfo->ulNumColors      = 20;
        pGdiInfo->ulNumPalReg      = 1 << ppdev->BitsPerPixel;
        pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;
        pDevInfo->flGraphicsCaps  |= GCAPS_PALMANAGED;
        pDevInfo->iDitherFormat    = BMF_8BPP;
        ppdev->PaletteShift        = (UCHAR)(8 - pGdiInfo->ulDACRed);
    }
    else
    {
        pGdiInfo->ulNumColors = (ULONG)(-1);
        pGdiInfo->ulNumPalReg = 0;
        switch (ppdev->BitsPerPixel)
        {
            case 16: pGdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP; pDevInfo->iDitherFormat = BMF_16BPP; break;
            case 24: pGdiInfo->ulHTOutputFormat = HT_FORMAT_24BPP; pDevInfo->iDitherFormat = BMF_24BPP; break;
            default: pGdiInfo->ulHTOutputFormat = HT_FORMAT_32BPP; pDevInfo->iDitherFormat = BMF_32BPP; break;
        }
    }

    EngFreeMem(ModeInfo);
    return TRUE;
}

ULONG APIENTRY
DrvGetModes(IN HANDLE hDriver, IN ULONG cjSize, OUT DEVMODEW *pdm)
{
    ULONG ModeCount;
    ULONG ModeInfoSize;
    PVIDEO_MODE_INFORMATION ModeInfo, ModeInfoPtr;
    ULONG OutputSize;

    ModeCount = GetAvailableModes(hDriver, &ModeInfo, &ModeInfoSize);
    if (ModeCount == 0)
        return 0;

    if (pdm == NULL)
    {
        EngFreeMem(ModeInfo);
        return ModeCount * sizeof(DEVMODEW);
    }

    OutputSize  = 0;
    ModeInfoPtr = ModeInfo;
    while (ModeCount-- > 0)
    {
        if (ModeInfoPtr->Length == 0)
        {
            ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((ULONG_PTR)ModeInfoPtr) + ModeInfoSize);
            continue;
        }

        memset(pdm, 0, sizeof(DEVMODEW));
        memcpy(pdm->dmDeviceName, DEVICE_NAME, sizeof(DEVICE_NAME));
        pdm->dmSpecVersion = pdm->dmDriverVersion = DM_SPECVERSION;
        pdm->dmSize          = sizeof(DEVMODEW);
        pdm->dmDriverExtra   = 0;
        pdm->dmBitsPerPel    = ModeInfoPtr->NumberOfPlanes * ModeInfoPtr->BitsPerPlane;
        pdm->dmPelsWidth     = ModeInfoPtr->VisScreenWidth;
        pdm->dmPelsHeight    = ModeInfoPtr->VisScreenHeight;
        pdm->dmDisplayFrequency = ModeInfoPtr->Frequency;
        pdm->dmDisplayFlags  = 0;
        pdm->dmFields        = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                               DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

        ModeInfoPtr = (PVIDEO_MODE_INFORMATION)(((ULONG_PTR)ModeInfoPtr) + ModeInfoSize);
        pdm = (LPDEVMODEW)(((ULONG_PTR)pdm) + sizeof(DEVMODEW));
        OutputSize += sizeof(DEVMODEW);
    }

    EngFreeMem(ModeInfo);
    return OutputSize;
}
