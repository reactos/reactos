/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Ddraw/callbacks_dd_hel.c
 * PURPOSE:              HEL Callbacks for Direct Draw
 * PROGRAMMER:           Magnus Olsen
 *
 */

#include "rosdraw.h"


extern  DDPIXELFORMAT pixelformats[];
extern DWORD pixelformatsCount;

/*
 * Direct Draw Callbacks
 */

DWORD CALLBACK  HelDdDestroyDriver(LPDDHAL_DESTROYDRIVERDATA lpDestroyDriver)
{
    DX_STUB;
}

DWORD CALLBACK  HelDdCreateSurface(LPDDHAL_CREATESURFACEDATA lpCreateSurface)
{
    DX_STUB;
}


DWORD CALLBACK  HelDdSetColorKey(LPDDHAL_DRVSETCOLORKEYDATA lpDrvSetColorKey)
{
    DX_STUB;
}


DWORD CALLBACK  HelDdSetMode(LPDDHAL_SETMODEDATA SetMode)
{
    DEVMODE DevMode;

    DX_STUB_str("in hel");

    DevMode.dmSize = (WORD)sizeof(DEVMODE);
    DevMode.dmDriverExtra = 0;

    SetMode->ddRVal = DDERR_UNSUPPORTEDMODE;

    if (EnumDisplaySettingsEx(NULL, SetMode->dwModeIndex, &DevMode, 0 ) != 0)
    {


        if (ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            DX_STUB_str("FAIL");
            SetMode->ddRVal = DDERR_UNSUPPORTEDMODE;
        }
        else
        {
            DX_STUB_str("OK");
            SetMode->ddRVal = DD_OK;
        }
    }

    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK  HelDdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    DX_STUB;
}

DWORD CALLBACK  HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurface)
{
    DX_STUB;
}

DWORD CALLBACK HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette)
{
    DDRAWI_DDRAWPALETTE_GBL* ddPalGbl = lpCreatePalette->lpDDPalette;
    LOGPALETTE* logPal ;
    WORD size=1;

    if(ddPalGbl->dwFlags & DDRAWIPAL_2)
        size = 2;
    else if(ddPalGbl->dwFlags & DDRAWIPAL_4)
        size = 4;
    else if(ddPalGbl->dwFlags & DDRAWIPAL_16)
        size = 16;
    else if(ddPalGbl->dwFlags & DDRAWIPAL_256)
        size = 256;

    DxHeapMemAlloc(logPal, sizeof(LOGPALETTE) + size*sizeof(PALETTEENTRY));
    if(logPal == NULL)
    {
        lpCreatePalette->ddRVal = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }

    logPal->palVersion = 0x300;
    logPal->palNumEntries = size;
    CopyMemory(&logPal->palPalEntry[0], lpCreatePalette->lpColorTable, size*sizeof(PALETTEENTRY));

    ddPalGbl->hHELGDIPalette = CreatePalette(logPal);

    if (ddPalGbl->hHELGDIPalette == NULL)
    {
        DxHeapMemFree(logPal);
        lpCreatePalette->ddRVal = DDERR_INVALIDOBJECT;
        return DDHAL_DRIVER_HANDLED;
    }

    DxHeapMemFree(logPal);
    ddPalGbl->lpColorTable = lpCreatePalette->lpColorTable;
    ddPalGbl->dwFlags |= DDRAWIPAL_INHEL | DDRAWIPAL_GDI ;
    lpCreatePalette->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK  HelDdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine)
{
    DX_STUB;
}

DWORD CALLBACK  HelDdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveMode)
{
    DX_WINDBG_trace();
    DX_STUB_str("Not implement yet, return DD_OK for not bsod\n");
    lpSetExclusiveMode->ddRVal = DD_OK;

    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK  HelDdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface)
{
    DX_STUB;
}

