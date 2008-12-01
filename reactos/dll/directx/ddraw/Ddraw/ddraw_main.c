/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_main.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

/* TODO
 * add warper functions for dx 1 - 6
 * map the  DirectDraw4_Vtable, DirectDraw2_Vtable, DirectDraw_Vtable
 * table to right version of the functions
 */



#include "rosdraw.h"

#include <string.h>

LPDDRAWI_DIRECTDRAW_INT
internal_directdraw_int_alloc(LPDDRAWI_DIRECTDRAW_INT This)
{
    LPDDRAWI_DIRECTDRAW_INT  newThis;
    DxHeapMemAlloc(newThis, sizeof(DDRAWI_DIRECTDRAW_INT));
    if (newThis)
    {
        newThis->lpLcl = This->lpLcl;
        newThis->lpLink = This;
    }

    return  newThis;
}

HRESULT WINAPI
Main_DirectDraw_QueryInterface (LPDDRAWI_DIRECTDRAW_INT This,
                                REFIID id,
                                LPVOID *obj)
{
    HRESULT retVal = DD_OK;

    DX_WINDBG_trace();

    _SEH2_TRY
    {
        /* FIXME
            the D3D object can be optained from here
            Direct3D7
        */
        if (IsEqualGUID(&IID_IDirectDraw7, id))
        {
            if (This->lpVtbl != &DirectDraw7_Vtable)
            {
                This = internal_directdraw_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }

            This->lpVtbl = &DirectDraw7_Vtable;
            *obj = This;
            Main_DirectDraw_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDraw4, id))
        {
            if (This->lpVtbl != &DirectDraw4_Vtable)
            {
                This = internal_directdraw_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }

            This->lpVtbl = &DirectDraw4_Vtable;
            *obj = This;
            Main_DirectDraw_AddRef(This);
        }

        else if (IsEqualGUID(&IID_IDirectDraw2, id))
        {
            if (This->lpVtbl != &DirectDraw2_Vtable)
            {
                This = internal_directdraw_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }

            This->lpVtbl = &DirectDraw2_Vtable;
            *obj = This;
            Main_DirectDraw_AddRef(This);
        }
        else if (IsEqualGUID(&IID_IDirectDraw, id))
        {
            if (This->lpVtbl != &DirectDraw_Vtable)
            {
                This = internal_directdraw_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }

            This->lpVtbl = &DirectDraw_Vtable;
            *obj = This;
            Main_DirectDraw_AddRef(This);
        }
        else
        {
            *obj = NULL;
            DX_STUB_str("E_NOINTERFACE");
            retVal = E_NOINTERFACE;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return retVal;
}

/*++
* @name DDraw->AddRef
* @implemented
*
* The function DDraw->AddRef count of all ref counter in the COM object DDraw->

* @return
* Returns the local Ref counter value for the COM object
*
* @remarks.
* none
*
*--*/
ULONG WINAPI
Main_DirectDraw_AddRef (LPDDRAWI_DIRECTDRAW_INT This)
{
    ULONG retValue = 0;

    DX_WINDBG_trace();

    /* Lock the thread so nothing can change the COM while we updating it */
    AcquireDDThreadLock();

    _SEH2_TRY
    {
        /* Count up the internal ref counter */
        This->dwIntRefCnt++;

        /* Count up the internal local ref counter */
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            /* Count up the internal gobal ref counter */
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    _SEH2_TRY
    {
        retValue = This->dwIntRefCnt;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        retValue = 0;
    }
    _SEH2_END;

    /* Release the thread lock */
    ReleaseDDThreadLock();

    /* Return the local Ref counter */
    return retValue;
}




ULONG WINAPI
Main_DirectDraw_Release (LPDDRAWI_DIRECTDRAW_INT This)
{
    ULONG Counter = 0;

    DX_WINDBG_trace();

    /* Lock the thread so nothing can change the COM while we updating it */
    AcquireDDThreadLock();

    _SEH2_TRY
    {
        if (This!=NULL)
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

                Cleanup(This);
            }

            /* FIXME cleanup being not call why ?? */
            Counter = This->dwIntRefCnt;
        }
        else
        {
            Counter = This->dwIntRefCnt;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    /* Release the thread lock */
    ReleaseDDThreadLock();

    return Counter;
}


HRESULT WINAPI
Main_DirectDraw_Initialize (LPDDRAWI_DIRECTDRAW_INT This, LPGUID lpGUID)
{
	return DDERR_ALREADYINITIALIZED;
}


/*++
* @name DDraw->Compact
* @implemented
*
* The function DDraw->Compact only return two diffent return value, they are DD_OK and DERR_NOEXCLUSIVEMODE
* if we are in Exclusive mode we return DERR_NOEXCLUSIVEMODE, other wise we return DD_OK

* @return
* Returns only Error code DD_OK or DERR_NOEXCLUSIVEMODE
*
* @remarks.
*  Microsoft say Compact is not implement in ddraw.dll, but Compact return  DDERR_NOEXCLUSIVEMODE or DD_OK
*
*--*/
HRESULT WINAPI
Main_DirectDraw_Compact(LPDDRAWI_DIRECTDRAW_INT This)
{
    HRESULT retVal = DD_OK;

    DX_WINDBG_trace();

    /* Lock the thread so nothing can change the COM while we updating it */
    AcquireDDThreadLock();

    _SEH2_TRY
    {
        /* Check see if Exclusive mode have been activate */
        if (This->lpLcl->lpGbl->lpExclusiveOwner != This->lpLcl)
        {
            retVal = DDERR_NOEXCLUSIVEMODE;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    /* Release the thread lock */
    ReleaseDDThreadLock();

    return retVal;
}

HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem(LPDDRAWI_DIRECTDRAW_INT This, LPDDSCAPS ddscaps, LPDWORD dwTotal, LPDWORD dwFree)
{
    DDSCAPS2 myddscaps;
    HRESULT retValue = DD_OK;

    ZeroMemory(&myddscaps, sizeof(DDSCAPS2));

    _SEH2_TRY
    {
        myddscaps.dwCaps =  ddscaps->dwCaps;
        retValue = Main_DirectDraw_GetAvailableVidMem4(This, &myddscaps, dwTotal, dwFree);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
         retValue = DDERR_INVALIDPARAMS;
    }
    _SEH2_END;

    return retValue;
}

HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem4(LPDDRAWI_DIRECTDRAW_INT This, LPDDSCAPS2 ddscaps,
                   LPDWORD dwTotal, LPDWORD dwFree)
{
    HRESULT retVal = DD_OK;
    DDHAL_GETAVAILDRIVERMEMORYDATA  memdata;

    DX_WINDBG_trace();

    _SEH2_TRY
    {
        // There is no HEL implentation of this api
        if (!(This->lpLcl->lpDDCB->HALDDMiscellaneous.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY) ||
            (This->lpLcl->lpGbl->dwFlags & DDRAWI_NOHARDWARE) )
        {
            retVal = DDERR_NODIRECTDRAWHW;
        }
        else
        {
            if ((!dwTotal && !dwFree) || !ddscaps)
            {
                retVal = DDERR_INVALIDPARAMS;
                _SEH2_LEAVE;
            }

            if ( ddscaps->dwCaps & (DDSCAPS_BACKBUFFER  | DDSCAPS_COMPLEX   | DDSCAPS_FLIP |
                                    DDSCAPS_FRONTBUFFER | DDSCAPS_PALETTE   | DDSCAPS_SYSTEMMEMORY |
                                    DDSCAPS_VISIBLE     | DDSCAPS_WRITEONLY | DDSCAPS_OWNDC))
            {
                retVal = DDERR_INVALIDPARAMS;
                _SEH2_LEAVE;
            }


            /*   ddscaps->dwCaps2 & 0x01
                this flag is outdate and are
                set to 0 in ms dxsdk  the name of
                this flag is DDSCAPS2_HARDWAREDEINTERLACE
            */

            if ( ddscaps->dwCaps2 & 0x01)
            {
                retVal = DDERR_INVALIDCAPS;
                _SEH2_LEAVE;
            }

            if ( ddscaps->dwCaps3 & ~( DDSCAPS3_MULTISAMPLE_QUALITY_MASK | DDSCAPS3_MULTISAMPLE_MASK |
                                       DDSCAPS3_RESERVED1                | DDSCAPS3_RESERVED2        |
                                       DDSCAPS3_LIGHTWEIGHTMIPMAP        | DDSCAPS3_AUTOGENMIPMAP    |
                                       DDSCAPS3_DMAP))
            {
                retVal = DDERR_INVALIDCAPS;
                _SEH2_LEAVE;
            }

            if ( ddscaps->dwCaps4)
            {
                retVal = DDERR_INVALIDCAPS;
                _SEH2_LEAVE;
            }

            ZeroMemory(&memdata, sizeof(DDHAL_GETAVAILDRIVERMEMORYDATA));
            memdata.lpDD = This->lpLcl->lpGbl;
            memdata.ddRVal = DDERR_INVALIDPARAMS;

            memdata.ddsCapsEx.dwCaps2 = ddscaps->dwCaps2;
            memdata.ddsCapsEx.dwCaps3 = ddscaps->dwCaps3;

            This->lpLcl->lpGbl->hDD = This->lpLcl->hDD;

            if (This->lpLcl->lpDDCB->HALDDMiscellaneous.GetAvailDriverMemory(&memdata) == DDHAL_DRIVER_NOTHANDLED)
            {
                retVal = DDERR_NODIRECTDRAWHW;

                if (dwTotal)
                    *dwTotal = 0;

                if (dwFree)
                    *dwFree = 0;
            }
            else
            {
                if (dwTotal)
                    *dwTotal = memdata.dwTotal;

                if (dwFree)
                    *dwFree = memdata.dwFree;

                retVal = memdata.ddRVal;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return retVal;
}

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(LPDDRAWI_DIRECTDRAW_INT This, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
    HRESULT retVal = DD_OK;

    DX_WINDBG_trace();


     // EnterCriticalSection(&ddcs);

    _SEH2_TRY
    {
        if(IsBadWritePtr(lpNumCodes,sizeof(LPDWORD)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            if(!(IsBadWritePtr(lpNumCodes,sizeof(LPDWORD))))
            {
                DWORD size;

                if (*lpNumCodes > This->lpLcl->lpGbl->dwNumFourCC)
                {
                    *lpNumCodes = This->lpLcl->lpGbl->dwNumFourCC;
                }

                size =  *lpNumCodes * sizeof(DWORD);

                if(!IsBadWritePtr(lpCodes, size ))
                {
                    memcpy(lpCodes, This->lpLcl->lpGbl->lpdwFourCC, size );
                }
                else
                {
                    *lpNumCodes = This->lpLcl->lpGbl->dwNumFourCC;
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    //LeaveCriticalSection(&ddcs);
    return retVal;
}


/*
 * We can optain the version of the directdraw object by compare the
 * vtl table pointer from iface we do not need pass which version
 * we whant to use
 *
 * Main_DirectDraw_CreateSurface is dead at moment we do only support
 * directdraw 7 at moment
 */

/* For DirectDraw 1 - 3 */
HRESULT WINAPI
Main_DirectDraw_CreateSurface (LPDDRAWI_DIRECTDRAW_INT This, LPDDSURFACEDESC pDDSD,
                               LPDDRAWI_DDRAWSURFACE_INT *ppSurf, IUnknown *pUnkOuter)
{
   HRESULT ret = DDERR_GENERIC;
   DDSURFACEDESC2 dd_desc_v2;

   DX_WINDBG_trace();

    EnterCriticalSection(&ddcs);
    *ppSurf = NULL;

    _SEH2_TRY
    {
        if (pDDSD->dwSize == sizeof(DDSURFACEDESC))
        {
            CopyDDSurfDescToDDSurfDesc2(&dd_desc_v2, (LPDDSURFACEDESC)pDDSD);
            ret = Internal_CreateSurface(This,
                                         &dd_desc_v2,
                                         ppSurf,
                                         pUnkOuter);
        }
        else
        {
            ret = DDERR_INVALIDPARAMS;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = DDERR_INVALIDPARAMS;
    }
    _SEH2_END;
    LeaveCriticalSection(&ddcs);
    return ret;
}


/* For DirectDraw 4 - 7 */
HRESULT WINAPI
Main_DirectDraw_CreateSurface4(LPDDRAWI_DIRECTDRAW_INT This, LPDDSURFACEDESC2 pDDSD,
                               LPDDRAWI_DDRAWSURFACE_INT *ppSurf, IUnknown *pUnkOuter)
{
    HRESULT ret = DD_OK;
    DX_WINDBG_trace();

    EnterCriticalSection(&ddcs);
    *ppSurf = NULL;

    _SEH2_TRY
    {
        ret = Internal_CreateSurface(This, pDDSD, ppSurf, pUnkOuter);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = DDERR_INVALIDPARAMS;
    }
    _SEH2_END;

    LeaveCriticalSection(&ddcs);
    return ret;
}

/* 5 of 31 DirectDraw7_Vtable api are working simluare to windows */
/* 8 of 31 DirectDraw7_Vtable api are under devloping / testing */









