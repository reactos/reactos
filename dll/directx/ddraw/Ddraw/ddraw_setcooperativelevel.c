/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Ddraw/ddraw_setcooperativelevel.c
 * PURPOSE:              IDirectDraw7::SetCooperativeLevel Implementation
 * PROGRAMMER:           Magnus Olsen
 *
 */

#include "rosdraw.h"

HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel (LPDDRAWI_DIRECTDRAW_INT This, HWND hwnd, DWORD cooplevel)
{
    HRESULT retVal = DD_OK;


    DX_WINDBG_trace();

    _SEH2_TRY
    {

        if (hwnd && !IsWindow(hwnd))
        {
            retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        // FIXME test if 0x20 exists as a flag and what thuse it do
        if ( cooplevel & (~(DDSCL_FPUPRESERVE | DDSCL_FPUSETUP | DDSCL_MULTITHREADED | DDSCL_CREATEDEVICEWINDOW |
                            DDSCL_SETDEVICEWINDOW | DDSCL_SETFOCUSWINDOW | DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE |
                            DDSCL_NORMAL | DDSCL_NOWINDOWCHANGES | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN)))
        {

              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        if (!( cooplevel & (DDSCL_NORMAL | DDSCL_EXCLUSIVE | DDSCL_SETFOCUSWINDOW)))
        {
              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        if ((cooplevel & DDSCL_FPUSETUP) && (cooplevel & DDSCL_FPUPRESERVE))
        {
              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        if ((cooplevel & DDSCL_EXCLUSIVE) && (!(cooplevel & DDSCL_FULLSCREEN)))
        {
              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        if ((cooplevel & DDSCL_ALLOWMODEX) &&  (!(cooplevel & DDSCL_FULLSCREEN)))
        {
              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        if ((cooplevel & (DDSCL_CREATEDEVICEWINDOW | DDSCL_SETFOCUSWINDOW)))
        {
              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }

        if (!cooplevel)
        {
              retVal = DDERR_INVALIDPARAMS;
             _SEH2_LEAVE;
        }


        /* NORMAL MODE */
        if(!(cooplevel & (~DDSCL_NORMAL)))
        {
            /* FIXME in setup.c  set DDRAWI_UMODELOADED | DDRAWI_DISPLAYDRV | DDRAWI_EMULATIONINITIALIZED | DDRAWI_GDIDRV  | DDRAWI_ATTACHEDTODESKTOP */
            /* FIXME in setup.c This->lpLcl->lpGbl->dwFlags =  */

            This->lpLcl->dwLocalFlags = DDRAWILCL_SETCOOPCALLED | DDRAWILCL_DIRECTDRAW7 ;
            This->lpLcl->hWnd = (ULONG_PTR) hwnd;
            This->lpLcl->hFocusWnd = (ULONG_PTR) hwnd;
            This->lpLcl->lpGbl->lpExclusiveOwner=NULL;

            retVal = DD_OK;
            _SEH2_LEAVE;
        }

        /* FULLSCREEN */
        if ((!(cooplevel & (~(DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)))) ||
           (!(cooplevel & (~(DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX)))))

        {
            /* FIXME in setup.c This->lpLcl->lpGbl->dwFlags =  */

            if (hwnd == NULL)
            {
                retVal = DDERR_INVALIDPARAMS;
                _SEH2_LEAVE;
            }

            if( (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD) )
            {
                retVal = DDERR_INVALIDPARAMS;
                _SEH2_LEAVE;
            }

            if( (This->lpLcl->lpGbl->lpExclusiveOwner != NULL) &&
                (This->lpLcl->lpGbl->lpExclusiveOwner != This->lpLcl) )
            {
                retVal = DDERR_INVALIDPARAMS;
                _SEH2_LEAVE;
            }

            This->lpLcl->lpGbl->lpExclusiveOwner = This-> lpLcl;

            This->lpLcl->dwLocalFlags = DDRAWILCL_SETCOOPCALLED     | DDRAWILCL_DIRECTDRAW7  | DDRAWILCL_HOOKEDHWND |
                                        DDRAWILCL_HASEXCLUSIVEMODE  | DDRAWILCL_ISFULLSCREEN | DDRAWILCL_ACTIVEYES |
                                        DDRAWILCL_CURSORCLIPPED;

            if (cooplevel & DDSCL_ALLOWMODEX)
            {
                This->lpLcl->dwLocalFlags = This->lpLcl->dwLocalFlags | DDRAWILCL_ALLOWMODEX;
            }

            This->lpLcl->hWnd = (ULONG_PTR) hwnd;
            This->lpLcl->hFocusWnd = (ULONG_PTR) hwnd;


            /* FIXME fullscreen are not finuish */

            retVal = DD_OK;
            _SEH2_LEAVE;
        }

    /*
 * Code from wine, this functions have been cut and paste from wine 0.9.35
 * and have been modify allot and are still in devloping so it match with
 * msdn document struct and flags
 */



        ///* Handle those levels first which set various hwnds */
        //if(cooplevel & DDSCL_SETFOCUSWINDOW)
        //{
        //

        //    if(This->lpLcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED)
        //    {
        //        retVal = DDERR_HWNDALREADYSET;
        //         _SEH2_LEAVE;
        //    }
        //    else if( (This->lpLcl->dwLocalFlags & DDRAWILCL_ISFULLSCREEN) && window)
        //    {
        //        retVal = DDERR_HWNDALREADYSET;
        //        _SEH2_LEAVE;
        //    }

        //    This->lpLcl->hFocusWnd = (ULONG_PTR) hwnd;


        //    /* Won't use the hwnd param for anything else */
        //    hwnd = NULL;

        //    /* Use the focus window for drawing too */
        //    This->lpLcl->hWnd = This->lpLcl->hFocusWnd;

        //}

        ///* DDSCL_NORMAL or DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE */
        //if(cooplevel & DDSCL_NORMAL)
        //{
        //    /* Can't coexist with fullscreen or exclusive */
        //    if(cooplevel & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) )
        //    {
        //        retVal = DDERR_INVALIDPARAMS;
        //         _SEH2_LEAVE;
        //    }

        //    /* Switching from fullscreen? */
        //    if(This->lpLcl->dwLocalFlags & DDRAWILCL_ISFULLSCREEN)
        //    {
        //        /* Restore the display mode */
        //        Main_DirectDraw_RestoreDisplayMode(iface);

        //        This->lpLcl->dwLocalFlags &= ~DDRAWILCL_ISFULLSCREEN;
        //        This->lpLcl->dwLocalFlags &= ~DDRAWILCL_HASEXCLUSIVEMODE;
        //        This->lpLcl->dwLocalFlags &= ~DDRAWILCL_ALLOWMODEX;
        //    }

        //    /* Don't override focus windows or private device windows */
        //    if( hwnd &&
        //        !(This->lpLcl->hFocusWnd) &&
        //        !(This->lpLcl->dwObsolete1) &&
        //        (hwnd != window) )
        //    {
        //        This->lpLcl->hWnd = (ULONG_PTR)hwnd;
        //    }

        //    /* FIXME GL
        //        IWineD3DDevice_SetFullscreen(This->wineD3DDevice,
        //                             FALSE);
        //    */
        //    }
        //    else if(cooplevel & DDSCL_FULLSCREEN)
        //    {
        //        /* Needs DDSCL_EXCLUSIVE */
        //        if(!(cooplevel & DDSCL_EXCLUSIVE) )
        //        {
        //            retVal = DDERR_INVALIDPARAMS;
        //            _SEH2_LEAVE;
        //        }

        //        /* Switch from normal to full screen mode? */
        //        if (!(This->lpLcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE))
        //        {
        //            /* FIXME GL
        //            IWineD3DDevice_SetFullscreen(This->wineD3DDevice,
        //                                 TRUE);
        //            */
        //        }

        //        /* Don't override focus windows or private device windows */
        //        if( hwnd &&
        //            !(This->lpLcl->hFocusWnd) &&
        //            !(This->lpLcl->dwObsolete1) &&
        //            (hwnd != window) )
        //        {
        //            This->lpLcl->hWnd =  (ULONG_PTR) hwnd;
        //        }
        //    }
        //    else if(cooplevel & DDSCL_EXCLUSIVE)
        //    {
        //        retVal = DDERR_INVALIDPARAMS;
        //        _SEH2_LEAVE;
        //    }

        //    if(cooplevel & DDSCL_CREATEDEVICEWINDOW)
        //    {
        //        /* Don't create a device window if a focus window is set */
        //        if( !This->lpLcl->hFocusWnd)
        //        {
        //            HWND devicewindow = CreateWindowExW(0, classname, L"DDraw device window",
        //                                        WS_POPUP, 0, 0,
        //                                        GetSystemMetrics(SM_CXSCREEN),
        //                                        GetSystemMetrics(SM_CYSCREEN),
        //                                        NULL, NULL, GetModuleHandleW(0), NULL);

        //            ShowWindow(devicewindow, SW_SHOW);   /* Just to be sure */

        //            This->lpLcl->dwObsolete1 = (DWORD)devicewindow;
        //        }
        //    }

        //    if(cooplevel & DDSCL_MULTITHREADED && !(This->lpLcl->dwLocalFlags & DDRAWILCL_MULTITHREADED))
        //    {
        //        /* FIXME GL
        //         * IWineD3DDevice_SetMultithreaded(This->wineD3DDevice);
        //         */
        //    }



        //    /* Store the cooperative_level */

        //    /* FIXME GL
        //     * This->cooperative_level |= cooplevel;
        //     */
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;


    return retVal;
}
