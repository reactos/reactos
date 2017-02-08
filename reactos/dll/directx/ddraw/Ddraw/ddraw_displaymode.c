/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Ddraw/ddraw_displaymode.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Maarten Bosma, Magnus Olsen (add seh support)
 *
 */


#include "rosdraw.h"

HRESULT WINAPI
Main_DirectDraw_EnumDisplayModes(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
                                  LPDDSURFACEDESC pDDSD, LPVOID pContext, LPDDENUMMODESCALLBACK pCallback)
{
    HRESULT ret = DD_OK;
    INT iMode = 0;
    DEVMODE DevMode;

    DX_WINDBG_trace();

    ZeroMemory(&DevMode, sizeof(DEVMODE));

    _SEH2_TRY
    {

        if (pDDSD != NULL)
        {
            if (pDDSD->dwSize != sizeof(DDSURFACEDESC))
            {
                 ret = DDERR_INVALIDPARAMS;
            }
        }

        if (IsBadCodePtr((LPVOID)pCallback))
        {
            ret = DDERR_INVALIDPARAMS;
        }
        else if ( ret == DD_OK)
        {

            DevMode.dmSize = sizeof(DEVMODE);

            while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0) == TRUE)
            {
                DDSURFACEDESC SurfaceDesc;

                ZeroMemory(&SurfaceDesc, sizeof(DDSURFACEDESC));

                iMode++;

                SurfaceDesc.dwSize = sizeof (DDSURFACEDESC);
                SurfaceDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_REFRESHRATE | DDSD_WIDTH | DDSD_PIXELFORMAT;
                SurfaceDesc.dwHeight = DevMode.dmPelsHeight;
                SurfaceDesc.dwWidth = DevMode.dmPelsWidth;
                SurfaceDesc.lPitch = DevMode.dmPelsWidth * DevMode.dmBitsPerPel / 8;
                SurfaceDesc.dwRefreshRate = DevMode.dmDisplayFrequency;

                SurfaceDesc.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
                SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
                // FIXME: get these
                /*
                    SurfaceDesc.ddpfPixelFormat.dwRBitMask =
                    SurfaceDesc.ddpfPixelFormat.dwGBitMask =
                    SurfaceDesc.ddpfPixelFormat.dwBBitMask =
                    SurfaceDesc.ddpfPixelFormat.dwRGBAlphaBitMask =
                */
                SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = DevMode.dmBitsPerPel;

                // FIXME1: This->lpLcl->lpGbl->dwMonitorFrequency is not set !
                if(dwFlags & DDEDM_REFRESHRATES && SurfaceDesc.dwRefreshRate != This->lpLcl->lpGbl->dwMonitorFrequency)
                {
                    //continue;  // FIXME2: what is SurfaceDesc.dwRefreshRate supposed to be set to ?
                }

                // FIXME: Take case when DDEDM_STANDARDVGAMODES flag is not set in account

                if(pDDSD)
                {
                    if(pDDSD->dwFlags & DDSD_HEIGHT && pDDSD->dwHeight != SurfaceDesc.dwHeight)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_WIDTH && pDDSD->dwWidth != SurfaceDesc.dwWidth)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_PITCH && pDDSD->lPitch != SurfaceDesc.lPitch)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_REFRESHRATE && pDDSD->dwRefreshRate != SurfaceDesc.dwRefreshRate)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_PIXELFORMAT && pDDSD->ddpfPixelFormat.dwRGBBitCount != SurfaceDesc.ddpfPixelFormat.dwRGBBitCount)
                        continue;  // FIXME: test for the other members of ddpfPixelFormat as well
                }

                if((*pCallback)(&SurfaceDesc, pContext) == DDENUMRET_CANCEL)
                    break;
            }
        }

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return ret;
}

HRESULT WINAPI
Main_DirectDraw_EnumDisplayModes4(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
                                  LPDDSURFACEDESC2 pDDSD, LPVOID pContext, LPDDENUMMODESCALLBACK2 pCallback)
{
    HRESULT ret = DD_OK;
    INT iMode = 0;
    DEVMODE DevMode;

    DX_WINDBG_trace();

    ZeroMemory(&DevMode, sizeof(DEVMODE));

    _SEH2_TRY
    {

        if (pDDSD != NULL)
        {
            if (pDDSD->dwSize != sizeof(DDSURFACEDESC2))
            {
                 ret = DDERR_INVALIDPARAMS;
            }
        }

        if (IsBadCodePtr((LPVOID)pCallback))
        {
            ret = DDERR_INVALIDPARAMS;
        }
        else if ( ret == DD_OK)
        {

            DevMode.dmSize = sizeof(DEVMODE);

            while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0) == TRUE)
            {
                DDSURFACEDESC2 SurfaceDesc;

                ZeroMemory(&SurfaceDesc, sizeof(DDSURFACEDESC2));

                iMode++;

                SurfaceDesc.dwSize = sizeof (DDSURFACEDESC2);
                SurfaceDesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_REFRESHRATE | DDSD_WIDTH | DDSD_PIXELFORMAT;
                SurfaceDesc.dwHeight = DevMode.dmPelsHeight;
                SurfaceDesc.dwWidth = DevMode.dmPelsWidth;
                SurfaceDesc.lPitch = DevMode.dmPelsWidth * DevMode.dmBitsPerPel / 8;
                SurfaceDesc.dwRefreshRate = DevMode.dmDisplayFrequency;

                SurfaceDesc.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
                SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
                // FIXME: get these
                /*
                    SurfaceDesc.ddpfPixelFormat.dwRBitMask =
                    SurfaceDesc.ddpfPixelFormat.dwGBitMask =
                    SurfaceDesc.ddpfPixelFormat.dwBBitMask =
                    SurfaceDesc.ddpfPixelFormat.dwRGBAlphaBitMask =
                */
                SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = DevMode.dmBitsPerPel;

                // FIXME1: This->lpLcl->lpGbl->dwMonitorFrequency is not set !
                if(dwFlags & DDEDM_REFRESHRATES && SurfaceDesc.dwRefreshRate != This->lpLcl->lpGbl->dwMonitorFrequency)
                {
                    //continue;  // FIXME2: what is SurfaceDesc.dwRefreshRate supposed to be set to ?
                }

                // FIXME: Take case when DDEDM_STANDARDVGAMODES flag is not set in account

                if(pDDSD)
                {
                    if(pDDSD->dwFlags & DDSD_HEIGHT && pDDSD->dwHeight != SurfaceDesc.dwHeight)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_WIDTH && pDDSD->dwWidth != SurfaceDesc.dwWidth)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_PITCH && pDDSD->lPitch != SurfaceDesc.lPitch)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_REFRESHRATE && pDDSD->dwRefreshRate != SurfaceDesc.dwRefreshRate)
                        continue;

                    else if(pDDSD->dwFlags & DDSD_PIXELFORMAT && pDDSD->ddpfPixelFormat.dwRGBBitCount != SurfaceDesc.ddpfPixelFormat.dwRGBBitCount)
                        continue;  // FIXME: test for the other members of ddpfPixelFormat as well
                }

                if((*pCallback)(&SurfaceDesc, pContext) == DDENUMRET_CANCEL)
                    break;
            }
        }

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return ret;
}

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode (LPDDRAWI_DIRECTDRAW_INT This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_SetDisplayMode2 (This, dwWidth, dwHeight, dwBPP, 0, 0 );
}

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode2 (LPDDRAWI_DIRECTDRAW_INT This, DWORD dwWidth, DWORD dwHeight,
                                 DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    HRESULT ret = DD_OK;
    DX_WINDBG_trace();

    _SEH2_TRY
    {
        // FIXME: Check primary if surface is locked / busy etc.

        // Check Parameters
        if(dwFlags != 0)
        {
            ret = DDERR_INVALIDPARAMS;
        }
        else
        {
            if ((!dwHeight || This->lpLcl->lpGbl->vmiData.dwDisplayHeight == dwHeight) &&
                (!dwWidth || This->lpLcl->lpGbl->vmiData.dwDisplayWidth == dwWidth)  &&
                (!dwBPP || This->lpLcl->lpGbl->vmiData.ddpfDisplay.dwRGBBitCount == dwBPP) &&
                (!dwRefreshRate || This->lpLcl->lpGbl->dwMonitorFrequency == dwRefreshRate))
            {
                ret = DD_OK; // nothing to do here for us
            }
            else
            {
                LONG retval;
                // Here we go
                DEVMODE DevMode;
                ZeroMemory(&DevMode, sizeof(DEVMODE));
                DevMode.dmSize = sizeof(DEVMODE);

                if (dwHeight)
                    DevMode.dmFields |= DM_PELSHEIGHT;
                if (dwWidth)
                    DevMode.dmFields |= DM_PELSWIDTH;
                if (dwBPP)
                    DevMode.dmFields |= DM_BITSPERPEL;
                if (dwRefreshRate)
                    DevMode.dmFields |= DM_DISPLAYFREQUENCY;

                DevMode.dmPelsHeight = dwHeight;
                DevMode.dmPelsWidth = dwWidth;
                DevMode.dmBitsPerPel = dwBPP;
                DevMode.dmDisplayFrequency = dwRefreshRate;

                DX_WINDBG_trace_res(dwWidth, dwHeight,dwBPP, dwRefreshRate);

                retval = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
                /* FIXME: Are we supposed to set CDS_SET_PRIMARY as well ? */

                if(retval == DISP_CHANGE_BADMODE)
                {
                    /* Note : it seam ms ddraw ignore this and try using the bad mode any case.
                     * tested with Ati HD2400 that only support 16 and 32 Bpp in windows
                     */
                    DX_STUB_str("Warning ChangeDisplaySettings return DISP_CHANGE_BADMODE, but ddraw.dll ignore it\n");

                    //ret = DDERR_UNSUPPORTED;
                    BOOL ModeChanged;
                    This->lpLcl->lpGbl->hDD = This->lpLcl->hDD;
                    DdReenableDirectDrawObject(This->lpLcl->lpGbl, &ModeChanged);
                    StartDirectDraw((LPDIRECTDRAW)This, 0, TRUE);
                }
                else if(retval != DISP_CHANGE_SUCCESSFUL)
                {
                    ret = DDERR_GENERIC;
                }
                else
                {
                    // Update Interals
                    BOOL ModeChanged;
                    This->lpLcl->lpGbl->hDD = This->lpLcl->hDD;
                    DdReenableDirectDrawObject(This->lpLcl->lpGbl, &ModeChanged);
                    StartDirectDraw((LPDIRECTDRAW)This, 0, TRUE);
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return ret;
}

HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode (LPDDRAWI_DIRECTDRAW_INT This)
{
    DX_WINDBG_trace();

    _SEH2_TRY
    {
        BOOL ModeChanged;

        ChangeDisplaySettings(NULL, 0);

        // Update Interals


        This->lpLcl->lpGbl->hDD = This->lpLcl->hDD;
        DdReenableDirectDrawObject(This->lpLcl->lpGbl, &ModeChanged);
        StartDirectDraw((LPDIRECTDRAW)This, 0, TRUE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;


    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency (LPDDRAWI_DIRECTDRAW_INT This, LPDWORD lpFreq)
{
    HRESULT retVal = DD_OK;
    DX_WINDBG_trace();

    _SEH2_TRY
    {
        if(IsBadWritePtr(lpFreq,sizeof(LPDWORD)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            if (This->lpLcl->lpGbl->dwMonitorFrequency)
            {
                *lpFreq = This->lpLcl->lpGbl->dwMonitorFrequency;
            }
            else
            {
                retVal = DDERR_UNSUPPORTED;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      retVal = DD_FALSE;
    }
    _SEH2_END;

    return retVal;
}

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode (LPDDRAWI_DIRECTDRAW_INT This, LPDDSURFACEDESC pDDSD)
{
    HRESULT retVal = DD_OK;
    DX_WINDBG_trace();

    _SEH2_TRY
    {
        if(IsBadWritePtr(pDDSD,sizeof(LPDDSURFACEDESC)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else if (pDDSD->dwSize != sizeof(DDSURFACEDESC))
        {
             retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            // FIXME: More stucture members might need to be filled

            pDDSD->dwFlags |= DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
            pDDSD->dwHeight = This->lpLcl->lpGbl->vmiData.dwDisplayHeight;
            pDDSD->dwWidth = This->lpLcl->lpGbl->vmiData.dwDisplayWidth;
            pDDSD->ddpfPixelFormat = This->lpLcl->lpGbl->vmiData.ddpfDisplay;
            pDDSD->dwRefreshRate = This->lpLcl->lpGbl->dwMonitorFrequency;
            pDDSD->lPitch = This->lpLcl->lpGbl->vmiData.lDisplayPitch;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return retVal;
}

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode4 (LPDDRAWI_DIRECTDRAW_INT This, LPDDSURFACEDESC2 pDDSD)
{
    HRESULT retVal = DD_OK;
    DX_WINDBG_trace();

    _SEH2_TRY
    {
        if(IsBadWritePtr(pDDSD,sizeof(LPDDSURFACEDESC2)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else if (pDDSD->dwSize != sizeof(DDSURFACEDESC2))
        {
             retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            // FIXME: More stucture members might need to be filled

            pDDSD->dwFlags |= DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_REFRESHRATE;
            pDDSD->dwHeight = This->lpLcl->lpGbl->vmiData.dwDisplayHeight;
            pDDSD->dwWidth = This->lpLcl->lpGbl->vmiData.dwDisplayWidth;
            pDDSD->ddpfPixelFormat = This->lpLcl->lpGbl->vmiData.ddpfDisplay;
            pDDSD->dwRefreshRate = This->lpLcl->lpGbl->dwMonitorFrequency;
            pDDSD->lPitch = This->lpLcl->lpGbl->vmiData.lDisplayPitch;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return retVal;
}
