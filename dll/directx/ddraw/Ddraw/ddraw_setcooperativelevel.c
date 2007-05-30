/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_main.c
 * PURPOSE:              IDirectDraw7::SetCooperativeLevel Implementation
 * PROGRAMMER:           Magnus Olsen
 *
 */

#include "rosdraw.h"

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

	if(hwnd && !IsWindow(hwnd))
		return DDERR_INVALIDPARAMS;

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
