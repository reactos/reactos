/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/GetCaps.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen
 *
 */

/* TODO
 * lpddNLVHELCaps and lpddNLVCaps
 * Thouse two can be null or inviald in lpGBL
 * we need add code in startup to detect if we have lpddNLVCaps set
 * ms HEL does not support lpddNLVHELCaps shall we do that ?
 * ms HEL does not support dwVidMemTotal and dwVidMemFree shall we implement a emulate of them ?
 */

#include "rosdraw.h"

#include <string.h>

/* PSEH for SEH Support */
#include <pseh/pseh.h>


HRESULT WINAPI
Main_DirectDraw_GetCaps( LPDDRAWI_DIRECTDRAW_INT This, LPDDCAPS pDriverCaps,
                         LPDDCAPS pHELCaps)
{
    HRESULT retVal = DDERR_INVALIDPARAMS;

    DX_WINDBG_trace();

    EnterCriticalSection( &ddcs );

    _SEH_TRY
    {
        if ((!pDriverCaps) && (!pHELCaps))
        {
                retVal = DDERR_INVALIDPARAMS;
                _SEH_LEAVE;
        }

        /*
         * DDCAPS_DX6 and DDCAPS_DX7 have same size so
         * we do not need check both only one of them
         */
        if ( (pDriverCaps) &&
             (pDriverCaps->dwSize != sizeof(DDCAPS_DX1) ) &&
             (pDriverCaps->dwSize != sizeof(DDCAPS_DX3) ) &&
             (pDriverCaps->dwSize != sizeof(DDCAPS_DX5) ) &&
             (pDriverCaps->dwSize !=  sizeof(DDCAPS_DX7 )) )
        {
                retVal = DDERR_INVALIDPARAMS;
                _SEH_LEAVE;
        }

        /*
         * DDCAPS_DX6 and DDCAPS_DX7 have same size so
         * we do not need check both only one of them
         */
        if ( (pHELCaps) &&
             (pHELCaps->dwSize != sizeof(DDCAPS_DX1) ) &&
             (pHELCaps->dwSize != sizeof(DDCAPS_DX3) ) &&
             (pHELCaps->dwSize != sizeof(DDCAPS_DX5) ) &&
             (pHELCaps->dwSize != sizeof(DDCAPS_DX7 )) )
        {
                retVal = DDERR_INVALIDPARAMS;
                _SEH_LEAVE;
        }

        if (pDriverCaps)
        {
            /* Setup hardware caps */
            DDSCAPS2 ddscaps = { 0 };
            LPDDCORECAPS CoreCaps = (LPDDCORECAPS)&This->lpLcl->lpGbl->ddCaps;

            DWORD dwTotal = 0;
            DWORD dwFree = 0;

            Main_DirectDraw_GetAvailableVidMem4(This, &ddscaps, &dwTotal, &dwFree);

            switch (pDriverCaps->dwSize)
            {
                case sizeof(DDCAPS_DX1):
                {
                    LPDDCAPS_DX1 myCaps = (LPDDCAPS_DX1) pDriverCaps;

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(myCaps, CoreCaps, sizeof(DDCAPS_DX1));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;
                    myCaps->dwSize = sizeof(DDCAPS_DX1);

                    retVal = DD_OK;
                }
                break;

                case sizeof(DDCAPS_DX3):
                {
                    LPDDCAPS_DX3 myCaps = (LPDDCAPS_DX3) pDriverCaps;

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(&myCaps->dwCaps, &CoreCaps->dwCaps, sizeof(DDCAPS_DX3));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;
                    myCaps->dwSize = sizeof(DDCAPS_DX3);

                    retVal = DD_OK;
                }
                break;

                case sizeof(DDCAPS_DX5):
                {
                    LPDDCAPS_DX5 myCaps = (LPDDCAPS_DX5) pDriverCaps;

                    /* FIXME  This->lpLcl->lpGbl->lpddNLVCaps are not set in startup.c
                    if (This->lpLcl->lpGbl->lpddNLVCaps->dwSize == sizeof(DDNONLOCALVIDMEMCAPS))
                    {
                        memcpy(&myCaps->dwNLVBCaps, This->lpLcl->lpGbl->lpddNLVCaps, sizeof(DDNONLOCALVIDMEMCAPS));
                    }
                    */
                    memset(&myCaps->dwNLVBCaps,0,sizeof(DDNONLOCALVIDMEMCAPS));

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(&myCaps->dwCaps, &CoreCaps->dwCaps, sizeof(DDCORECAPS));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;
                    myCaps->dwSize = sizeof(DDCAPS_DX5);

                    retVal = DD_OK;
                }
                break;

                /* DDCAPS_DX6 is same as DDCAPS_DX7 */
                case sizeof(DDCAPS_DX7):
                {
                    LPDDCAPS_DX7 myCaps = (LPDDCAPS_DX7) pDriverCaps;

                    /* FIXME  This->lpLcl->lpGbl->lpddNLVCaps are not set in startup.c
                    if (This->lpLcl->lpGbl->lpddNLVCaps->dwSize == sizeof(DDNONLOCALVIDMEMCAPS))
                    {
                        memcpy(&myCaps->dwNLVBCaps, This->lpLcl->lpGbl->lpddNLVCaps, sizeof(DDNONLOCALVIDMEMCAPS));
                    }
                    */
                    memset(&myCaps->dwNLVBCaps,0,sizeof(DDNONLOCALVIDMEMCAPS));

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(&myCaps->dwCaps, &CoreCaps->dwCaps, sizeof(DDCORECAPS));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;

                    myCaps->ddsCaps.dwCaps = myCaps->ddsOldCaps.dwCaps;
                    myCaps->ddsCaps.dwCaps2 = 0;
                    myCaps->ddsCaps.dwCaps3 = 0;
                    myCaps->ddsCaps.dwCaps4 = 0;
                    myCaps->dwSize = sizeof(DDCAPS_DX7);

                    retVal = DD_OK;

                }
                break;

                default:
                    retVal = DDERR_INVALIDPARAMS;
                    break;
            }
        }

        if (pHELCaps)
        {
            /* Setup software caps */
            LPDDCORECAPS CoreCaps = (LPDDCORECAPS)&This->lpLcl->lpGbl->ddHELCaps;

            DWORD dwTotal = 0;
            DWORD dwFree = 0;

            switch (pHELCaps->dwSize)
            {
                case sizeof(DDCAPS_DX1):
                {
                    LPDDCAPS_DX1 myCaps = (LPDDCAPS_DX1) pHELCaps;

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(myCaps, CoreCaps, sizeof(DDCAPS_DX1));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;
                    myCaps->dwSize = sizeof(DDCAPS_DX1);

                    retVal = DD_OK;
                }
                break;

                case sizeof(DDCAPS_DX3):
                {
                    LPDDCAPS_DX3 myCaps = (LPDDCAPS_DX3) pHELCaps;

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(&myCaps->dwCaps, &CoreCaps->dwCaps, sizeof(DDCAPS_DX3));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;
                    myCaps->dwSize = sizeof(DDCAPS_DX3);

                    retVal = DD_OK;
                }
                break;

                case sizeof(DDCAPS_DX5):
                {
                    LPDDCAPS_DX5 myCaps = (LPDDCAPS_DX5) pHELCaps;

                    /* FIXME  This->lpLcl->lpGbl->lpddNLVHELCaps are not set in startup.c
                    if (This->lpLcl->lpGbl->lpddNLVHELCaps->dwSize == sizeof(DDNONLOCALVIDMEMCAPS))
                    {
                        memcpy(&myCaps->dwNLVBCaps, This->lpLcl->lpGbl->lpddNLVHELCaps, sizeof(DDNONLOCALVIDMEMCAPS));
                    }
                    */
                    memset(&myCaps->dwNLVBCaps,0,sizeof(DDNONLOCALVIDMEMCAPS));

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(&myCaps->dwCaps, &CoreCaps->dwCaps, sizeof(DDCORECAPS));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;
                    myCaps->dwSize = sizeof(DDCAPS_DX5);

                    retVal = DD_OK;
                }
                break;

                /* DDCAPS_DX6 is same as DDCAPS_DX7 */
                case sizeof(DDCAPS_DX7):
                {
                    LPDDCAPS_DX7 myCaps = (LPDDCAPS_DX7) pHELCaps;

                    /* FIXME  This->lpLcl->lpGbl->lpddNLVHELCaps are not set in startup.c
                    if (This->lpLcl->lpGbl->lpddNLVHELCaps->dwSize == sizeof(DDNONLOCALVIDMEMCAPS))
                    {
                        memcpy(&myCaps->dwNLVBCaps, This->lpLcl->lpGbl->lpddNLVHELCaps, sizeof(DDNONLOCALVIDMEMCAPS));
                    }
                    */
                    memset(&myCaps->dwNLVBCaps,0,sizeof(DDNONLOCALVIDMEMCAPS));

                    if (CoreCaps->dwSize == sizeof(DDCORECAPS))
                    {
                        memcpy(&myCaps->dwCaps, &CoreCaps->dwCaps, sizeof(DDCORECAPS));
                    }

                    myCaps->dwVidMemFree = dwFree;
                    myCaps->dwVidMemTotal = dwTotal;

                    myCaps->ddsCaps.dwCaps = myCaps->ddsOldCaps.dwCaps;
                    myCaps->ddsCaps.dwCaps2 = 0;
                    myCaps->ddsCaps.dwCaps3 = 0;
                    myCaps->ddsCaps.dwCaps4 = 0;
                    myCaps->dwSize = sizeof(DDCAPS_DX7);

                    retVal = DD_OK;

                }
                break;

                default:
                    retVal = DDERR_INVALIDPARAMS;
                    break;
            }
        }

    }
    _SEH_HANDLE
    {
        retVal = DD_FALSE;
    }
    _SEH_END;

    LeaveCriticalSection( &ddcs );
    return  retVal;
}

