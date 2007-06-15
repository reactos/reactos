/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_displaymode.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Maarten Bosma, Magnus Olsen (add seh support)
 *
 */


#include "rosdraw.h"

/* PSEH for SEH Support */
#include <pseh/pseh.h>

HRESULT WINAPI 
Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
                                  LPDDSURFACEDESC2 pDDSD, LPVOID pContext, LPDDENUMMODESCALLBACK2 pCallback)
{
    HRESULT ret = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    INT iMode = 0;
    DEVMODE DevMode;

    DX_WINDBG_trace();

    _SEH_TRY
    {
        DX_STUB_str("here\n");

        if 
        ((!IsBadReadPtr(pCallback,sizeof(LPDDENUMMODESCALLBACK2)))  ||
        (!IsBadWritePtr(pCallback,sizeof(LPDDENUMMODESCALLBACK2))) ||
        (!IsBadReadPtr(pDDSD,sizeof(DDSURFACEDESC2)))  ||
        (!IsBadWritePtr(pDDSD,sizeof(DDSURFACEDESC2))))
        {
            ret = DDERR_INVALIDPARAMS;
        }
        else
        {

            DX_STUB_str("here\n");

            DevMode.dmSize = sizeof(DEVMODE);
            DevMode.dmDriverExtra = 0;

            while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0) == TRUE)
            {
                DDSURFACEDESC2 SurfaceDesc; 

                DX_STUB_str("here\n");

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

                DX_STUB_str("here\n");

                // FIXME1: This->lpLcl->lpGbl->dwMonitorFrequency is not set !
                if(dwFlags & DDEDM_REFRESHRATES && SurfaceDesc.dwRefreshRate != This->lpLcl->lpGbl->dwMonitorFrequency)
                {
                    //continue;  // FIXME2: what is SurfaceDesc.dwRefreshRate supposed to be set to ?
                }

                // FIXME: Take case when DDEDM_STANDARDVGAMODES flag is not set in account

                if(pDDSD)
                {
                    DX_STUB_str("here\n");
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
    _SEH_HANDLE
    {
    }
    _SEH_END;

    DX_STUB_str("here\n");

    return ret;
}

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight,
                                                                DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    HRESULT ret = DD_OK;

    DX_STUB_str("here\n");

    _SEH_TRY
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
                DevMode.dmFields = 0;

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

                retval = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
                /* FIXME: Are we supposed to set CDS_SET_PRIMARY as well ? */

                if(retval == DISP_CHANGE_BADMODE)
                {
                    ret = DDERR_UNSUPPORTED;
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
                    StartDirectDraw((LPDIRECTDRAW)iface, 0, TRUE);
                }
            }
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    return ret;
}

HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode (LPDIRECTDRAW7 iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DX_WINDBG_trace();

    _SEH_TRY
    {
        BOOL ModeChanged;

        ChangeDisplaySettings(NULL, 0);

        // Update Interals

        
        This->lpLcl->lpGbl->hDD = This->lpLcl->hDD;
        DdReenableDirectDrawObject(This->lpLcl->lpGbl, &ModeChanged);
        StartDirectDraw((LPDIRECTDRAW)iface, 0, TRUE);
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;


    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency (LPDIRECTDRAW7 iface, LPDWORD lpFreq)
{
    HRESULT retVal = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DX_WINDBG_trace();

    _SEH_TRY
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
    _SEH_HANDLE
    {
      retVal = DD_FALSE;
    }
    _SEH_END;

    return retVal;
}

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD)
{
    HRESULT retVal = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DX_WINDBG_trace();

    _SEH_TRY
    {
        if(IsBadWritePtr(pDDSD,sizeof(LPDWORD)))
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
    _SEH_HANDLE
    {
    }
    _SEH_END;

    return retVal;
}
