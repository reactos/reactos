/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/callbacks_dd_hel.c
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

DWORD CALLBACK  HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette)
{
    DX_STUB;
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

