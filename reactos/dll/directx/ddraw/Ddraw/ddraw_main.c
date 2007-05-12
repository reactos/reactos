/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/ddraw.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */


#include "../rosdraw.h"

HRESULT
WINAPI
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface,
                                REFIID id,
                                LPVOID *obj)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    /* fixme
       the D3D object cab be optain from here
       Direct3D7
    */
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        /* DirectDraw7 Vtable */
        This->lpVtbl = &DirectDraw7_Vtable;
        *obj = &This->lpVtbl;
    }
    else
    {
        *obj = NULL;
        DX_STUB_str("E_NOINTERFACE");
        return E_NOINTERFACE;
    }


    Main_DirectDraw_AddRef(iface);
    DX_STUB_str("DD_OK");
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
ULONG
WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    return This->dwIntRefCnt;
}


ULONG
WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW7 iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->lpLcl->dwLocalRefCnt--;
        This->dwIntRefCnt--;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt--;
        }

        if ( This->lpLcl->lpGbl->dwRefCnt == 0)
        {
            // set resoltion back to the one in registry
            /*if(This->cooperative_level & DDSCL_EXCLUSIVE)
            {
                ChangeDisplaySettings(NULL, 0);
            }*/

            Cleanup(iface);
            return 0;
        }
    }
    return This->dwIntRefCnt;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT
WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface)
{
    /* MSDN say not implement but my question what does it return then */
    DX_WINDBG_trace();
    return DD_OK;
}


/*
 */
HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                                            LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{

  DX_WINDBG_trace();
  EnterCriticalSection(&ddcs);

  /* code here */

  LeaveCriticalSection(&ddcs);

  DX_STUB;
}

/*
 */
HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
  DX_WINDBG_trace();

/*
 * Code from wine, this functions have been cut and paste from wine 0.9.35
 * and have been modify allot and are still in devloping so it match with
 * msdn document struct and flags
 */

    HWND window;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

   /* Get the old window */
    window = (HWND) This->lpLcl->hWnd;
#if 0 // this check is totally invalid if you ask me - mbosma
    if(!window)
    {
        return DDERR_NOHWND;
    }
#endif

    /* Tests suggest that we need one of them: */
    if(!(cooplevel & (DDSCL_SETFOCUSWINDOW |
                      DDSCL_NORMAL         |
                      DDSCL_EXCLUSIVE      )))
    {
        return DDERR_INVALIDPARAMS;
    }

    /* Handle those levels first which set various hwnds */
    if(cooplevel & DDSCL_SETFOCUSWINDOW)
    {
        /* This isn't compatible with a lot of flags */
        if(cooplevel & ( DDSCL_MULTITHREADED   |
                         DDSCL_FPUSETUP        |
                         DDSCL_FPUPRESERVE     |
                         DDSCL_ALLOWREBOOT     |
                         DDSCL_ALLOWMODEX      |
                         DDSCL_SETDEVICEWINDOW |
                         DDSCL_NORMAL          |
                         DDSCL_EXCLUSIVE       |
                         DDSCL_FULLSCREEN      ) )
        {
            return DDERR_INVALIDPARAMS;
        }

        else if(This->lpLcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED)
        {
             return DDERR_HWNDALREADYSET;
        }
        else if( (This->lpLcl->dwLocalFlags & DDRAWILCL_ISFULLSCREEN) && window)
        {
            return DDERR_HWNDALREADYSET;
        }

        This->lpLcl->hFocusWnd = (ULONG_PTR) hwnd;


        /* Won't use the hwnd param for anything else */
        hwnd = NULL;

        /* Use the focus window for drawing too */
        This->lpLcl->hWnd = This->lpLcl->hFocusWnd;

    }

    /* DDSCL_NORMAL or DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE */
    if(cooplevel & DDSCL_NORMAL)
    {
        /* Can't coexist with fullscreen or exclusive */
        if(cooplevel & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) )
            return DDERR_INVALIDPARAMS;


        /* Switching from fullscreen? */
        if(This->lpLcl->dwLocalFlags & DDRAWILCL_ISFULLSCREEN)
        {
            /* Restore the display mode */
            Main_DirectDraw_RestoreDisplayMode(iface);

            This->lpLcl->dwLocalFlags &= ~DDRAWILCL_ISFULLSCREEN;
            This->lpLcl->dwLocalFlags &= ~DDRAWILCL_HASEXCLUSIVEMODE;
            This->lpLcl->dwLocalFlags &= ~DDRAWILCL_ALLOWMODEX;
        }

        /* Don't override focus windows or private device windows */
        if( hwnd &&
            !(This->lpLcl->hFocusWnd) &&
            !(This->lpLcl->dwObsolete1) &&
            (hwnd != window) )
        {
            This->lpLcl->hWnd = (ULONG_PTR)hwnd;
        }

        /* FIXME GL
        IWineD3DDevice_SetFullscreen(This->wineD3DDevice,
                                     FALSE);
        */
    }
    else if(cooplevel & DDSCL_FULLSCREEN)
    {
        /* Needs DDSCL_EXCLUSIVE */
        if(!(cooplevel & DDSCL_EXCLUSIVE) )
            return DDERR_INVALIDPARAMS;

        /* Switch from normal to full screen mode? */
        if (!(This->lpLcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE))
        {
            /* FIXME GL
            IWineD3DDevice_SetFullscreen(This->wineD3DDevice,
                                         TRUE);
            */
        }

        /* Don't override focus windows or private device windows */
        if( hwnd &&
            !(This->lpLcl->hFocusWnd) &&
            !(This->lpLcl->dwObsolete1) &&
            (hwnd != window) )
        {
            This->lpLcl->hWnd =  (ULONG_PTR) hwnd;
        }
    }
    else if(cooplevel & DDSCL_EXCLUSIVE)
    {
        return DDERR_INVALIDPARAMS;
    }

    if(cooplevel & DDSCL_CREATEDEVICEWINDOW)
    {
        /* Don't create a device window if a focus window is set */
        if( !This->lpLcl->hFocusWnd)
        {
            HWND devicewindow = CreateWindowExW(0, classname, L"DDraw device window",
                                                WS_POPUP, 0, 0,
                                                GetSystemMetrics(SM_CXSCREEN),
                                                GetSystemMetrics(SM_CYSCREEN),
                                                NULL, NULL, GetModuleHandleW(0), NULL);

            ShowWindow(devicewindow, SW_SHOW);   /* Just to be sure */

            This->lpLcl->dwObsolete1 = (DWORD)devicewindow;
        }
    }

    if(cooplevel & DDSCL_MULTITHREADED && !(This->lpLcl->dwLocalFlags & DDRAWILCL_MULTITHREADED))
    {
        /* FIXME GL
         * IWineD3DDevice_SetMultithreaded(This->wineD3DDevice);
         */
    }



    /* Store the cooperative_level */

    /* FIXME GL
     * This->cooperative_level |= cooplevel;
     */

    return DD_OK;

}

IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
