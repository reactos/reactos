/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_main.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */


#include "rosdraw.h"

#include <string.h>

/* PSEH for SEH Support */
#include <pseh/pseh.h>

HRESULT WINAPI
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface,
                                REFIID id,
                                LPVOID *obj)
{
    HRESULT retVal = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    _SEH_TRY
    {
        /* FIXME
            the D3D object can be optained from here
            Direct3D7
        */
        if (IsEqualGUID(&IID_IDirectDraw7, id))
        {
            /* DirectDraw7 Vtable */
            This->lpVtbl = &DirectDraw7_Vtable;
            This->lpLcl->dwLocalFlags = This->lpLcl->dwLocalFlags + DDRAWILCL_DIRECTDRAW7;
            *obj = &This->lpVtbl;
            Main_DirectDraw_AddRef(iface);
        }
        else
        {
            *obj = NULL;
            DX_STUB_str("E_NOINTERFACE");
            retVal = E_NOINTERFACE;
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    return retVal;
}

ULONG WINAPI
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface)
{
    ULONG retValue = 0;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    _SEH_TRY
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    _SEH_TRY
    {
        retValue = This->dwIntRefCnt;
    }
    _SEH_HANDLE
    {
        retValue = 0;
    }
    _SEH_END;

    return retValue;
}




ULONG WINAPI
Main_DirectDraw_Release (LPDIRECTDRAW7 iface)
{
    ULONG Counter = 0;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();
    _SEH_TRY
    {
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
            }

            /* FIXME cleanup being not call why ?? */
            Counter = This->dwIntRefCnt;
        }
        else
        {
            Counter = This->dwIntRefCnt;
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;
    return Counter;
}

HRESULT WINAPI
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{
	return DDERR_ALREADYINITIALIZED;
}

/* 
 * Main_DirectDraw_Compact
 * ms say this one is not implement but it return  DDERR_NOEXCLUSIVEMODE
 * when no exclusive owner are set in corpativelevel 
 */
HRESULT WINAPI
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface)
{
    HRESULT retVal = DD_OK;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT) iface;

    DX_WINDBG_trace();
    // EnterCriticalSection(&ddcs);

    _SEH_TRY
    {
        if (This->lpLcl->lpGbl->lpExclusiveOwner == This->lpLcl)
        {
            retVal = DDERR_NOEXCLUSIVEMODE;
        }
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;
    // LeaveCriticalSection(&ddcs);
    return retVal;
}

HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW2 iface, LPDDSCAPS ddscaps, LPDWORD dwTotal, LPDWORD dwFree)
{
    DDSCAPS2 myddscaps;
    HRESULT retValue = DD_OK;

    ZeroMemory(&myddscaps, sizeof(DDSCAPS2));

    _SEH_TRY
    {
        myddscaps.dwCaps =  ddscaps->dwCaps;
        retValue = Main_DirectDraw_GetAvailableVidMem4((LPDIRECTDRAW7)iface, &myddscaps, dwTotal, dwFree);
    }
    _SEH_HANDLE
    {
         retValue = DDERR_INVALIDPARAMS;
    }
    _SEH_END;

    return retValue;
}

HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem4(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD dwTotal, LPDWORD dwFree)
{
    HRESULT retVal = DD_OK;
    DDHAL_GETAVAILDRIVERMEMORYDATA  memdata;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DX_WINDBG_trace();

    _SEH_TRY
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
                _SEH_LEAVE;
            }

            if ( ddscaps->dwCaps & (DDSCAPS_BACKBUFFER  | DDSCAPS_COMPLEX   | DDSCAPS_FLIP | 
                                    DDSCAPS_FRONTBUFFER | DDSCAPS_PALETTE   | DDSCAPS_SYSTEMMEMORY |
                                    DDSCAPS_VISIBLE     | DDSCAPS_WRITEONLY | DDSCAPS_OWNDC))
            {
                retVal = DDERR_INVALIDPARAMS;
                _SEH_LEAVE;
            }

            
            /*   ddscaps->dwCaps2 & 0x01
                this flag is outdate and are 
                set to 0 in ms dxsdk  the name of 
                this flag is DDSCAPS2_HARDWAREDEINTERLACE
            */

            if ( ddscaps->dwCaps2 & 0x01)
            {
                retVal = DDERR_INVALIDCAPS;
                _SEH_LEAVE;
            }

            if ( ddscaps->dwCaps3 & ~( DDSCAPS3_MULTISAMPLE_QUALITY_MASK | DDSCAPS3_MULTISAMPLE_MASK |
                                       DDSCAPS3_RESERVED1                | DDSCAPS3_RESERVED2        |
                                       DDSCAPS3_LIGHTWEIGHTMIPMAP        | DDSCAPS3_AUTOGENMIPMAP    |
                                       DDSCAPS3_DMAP))
            {
                retVal = DDERR_INVALIDCAPS;
                _SEH_LEAVE;
            }

            if ( ddscaps->dwCaps4)
            {
                retVal = DDERR_INVALIDCAPS;
                _SEH_LEAVE;
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
    _SEH_HANDLE
    {
    }
    _SEH_END;

    return retVal;
}

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    HRESULT retVal = DD_OK;

    DX_WINDBG_trace();

    
     // EnterCriticalSection(&ddcs);

    _SEH_TRY
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
    _SEH_HANDLE
    {
    }
    _SEH_END;

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
Main_DirectDraw_CreateSurface (LPDIRECTDRAW iface, LPDDSURFACEDESC pDDSD,
                               LPDIRECTDRAWSURFACE *ppSurf, IUnknown *pUnkOuter)
{
   HRESULT ret = DDERR_GENERIC;
   DDSURFACEDESC2 dd_desc_v2;

   DX_WINDBG_trace();

    // EnterCriticalSection(&ddcs);
    _SEH_TRY
    {
        if (pDDSD->dwSize == sizeof(DDSURFACEDESC))
        {
            CopyDDSurfDescToDDSurfDesc2(&dd_desc_v2, (LPDDSURFACEDESC)pDDSD);
            ret = Internal_CreateSurface((LPDDRAWI_DIRECTDRAW_INT)iface,
                                         &dd_desc_v2,
                                         (LPDIRECTDRAWSURFACE7 *)ppSurf,
                                         pUnkOuter);
        }
        else
        {
            ret = DDERR_INVALIDPARAMS;
        }
    }
    _SEH_HANDLE
    {
        ret = DDERR_GENERIC;
    }
    _SEH_END;
  // LeaveCriticalSection(&ddcs);
  return ret;
}


/* For DirectDraw 4 - 7 */
HRESULT WINAPI 
Main_DirectDraw_CreateSurface4(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                               LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{
   HRESULT ret;
    DX_WINDBG_trace();
    // EnterCriticalSection(&ddcs);
    _SEH_TRY
    {
        ret = Internal_CreateSurface( (LPDDRAWI_DIRECTDRAW_INT)iface,pDDSD, ppSurf,pUnkOuter);
    }
        _SEH_HANDLE
    {
        ret = DDERR_GENERIC;
    }
    _SEH_END;

    // LeaveCriticalSection(&ddcs);
    return ret;
}

/* For DirectDraw 4 - 6 */
HRESULT WINAPI
Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW4 iface,
                                    LPDDDEVICEIDENTIFIER pDDDI, DWORD dwFlags)
{
    HRESULT retVal = DD_OK;
    DDDEVICEIDENTIFIER2 pDDDI2;

    ZeroMemory(&pDDDI2,sizeof(DDDEVICEIDENTIFIER2));

    _SEH_TRY
    {
        memcpy(&pDDDI2 , pDDDI, sizeof(DDDEVICEIDENTIFIER));

        retVal = Main_DirectDraw_GetDeviceIdentifier7((LPDIRECTDRAW7)iface, &pDDDI2, dwFlags);

        if (IsBadWritePtr(pDDDI, sizeof(DDDEVICEIDENTIFIER)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            memcpy(pDDDI , &pDDDI2, sizeof(DDDEVICEIDENTIFIER) );
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
Main_DirectDraw_GetDeviceIdentifier7(LPDIRECTDRAW7 iface,
                                     LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{
    HRESULT retVal = DDERR_INVALIDPARAMS;

    BOOL found = FALSE;
    DWORD iDevNum = 0;
    DISPLAY_DEVICEA DisplayDeviceA;
    HKEY hKey;
    DWORD lpType = 0;
    DWORD strSize = MAX_DDDEVICEID_STRING;
    char *pdest;
    char* pcCnvEnd;
    long *lpdata;

    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT) iface;

    DX_WINDBG_trace();

    _SEH_TRY
    {
        if ( (IsBadWritePtr( pDDDI, sizeof(DDDEVICEIDENTIFIER2) ) ) ||
             (dwFlags & ~DDGDI_GETHOSTIDENTIFIER))
        {
            retVal = DDERR_INVALIDPARAMS;
            _SEH_LEAVE;
        }

        /* now we can start getting the driver data */

        while (1)
        {
            ZeroMemory(&DisplayDeviceA,sizeof(DISPLAY_DEVICEA));

            DisplayDeviceA.cb = sizeof(DISPLAY_DEVICEA);

            if ( EnumDisplayDevicesA( NULL, iDevNum, &DisplayDeviceA, 0) == 0)
            {
                retVal = DDERR_INVALIDPARAMS;
                break;
            }

            if (!_stricmp(DisplayDeviceA.DeviceName, This->lpLcl->lpGbl->cDriverName))
            {
                /* if we got another device like hardware mpeg decoder or video card or another drv */
                found = TRUE;
            }
            else if (DisplayDeviceA.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            {
                /* double check if it primary driver we just found */
                if (!_stricmp( This->lpLcl->lpGbl->cDriverName, "DISPLAY"))
                {
                    /* yeah we found it */
                    found = TRUE;
                }
            }

            if (found == TRUE)
            {
                /* we found our driver now we start setup it */
                if (!_strnicmp(DisplayDeviceA.DeviceKey,"\\REGISTRY\\Machine\\",18))
                {
                    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, &DisplayDeviceA.DeviceKey[18], 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS )
                    {

                        if (RegQueryValueExA(hKey, "InstalledDisplayDrivers",0, &lpType, (LPBYTE)pDDDI->szDriver, &strSize) != ERROR_SUCCESS)
                        {
                            ZeroMemory(pDDDI->szDriver,MAX_DDDEVICEID_STRING);
                        }
                        else
                        {
                            strcat(pDDDI->szDriver,".dll");
                        }
                        RegCloseKey(hKey);
                    }

                    strcpy( pDDDI->szDescription, DisplayDeviceA.DeviceString);
                    pDDDI->liDriverVersion.HighPart = 0;
                    pDDDI->liDriverVersion.LowPart = 0;

                    pdest = strstr(DisplayDeviceA.DeviceID,"REV_");
                    pDDDI->dwRevision =  strtol ( &pdest[4], &pcCnvEnd, 16);

                    pdest = strstr(DisplayDeviceA.DeviceID,"SUBSYS_");
                    pDDDI->dwSubSysId =  strtol ( &pdest[7], &pcCnvEnd, 16);

                    pdest = strstr(DisplayDeviceA.DeviceID,"DEV_");
                    pDDDI->dwDeviceId = strtol ( &pdest[4], &pcCnvEnd, 16);

                    pdest = strstr(DisplayDeviceA.DeviceID,"VEN_");
                    pDDDI->dwVendorId =strtol ( &pdest[4], &pcCnvEnd, 16);

                    /* Count out the guidDeviceIdentifier */
                    memcpy(&pDDDI->guidDeviceIdentifier, &CLSID_DirectDraw,sizeof(GUID));
                   
                    pDDDI->guidDeviceIdentifier.Data1 = pDDDI->guidDeviceIdentifier.Data1 ^ pDDDI->dwVendorId;

                    lpdata = (long *)&pDDDI->guidDeviceIdentifier.Data2;
                    *lpdata = *lpdata ^ pDDDI->dwDeviceId;

                    lpdata = (long *)&pDDDI->guidDeviceIdentifier.Data4;
                    *lpdata = *lpdata ^ pDDDI->dwSubSysId;
                    *lpdata = *lpdata ^ pDDDI->liDriverVersion.LowPart;

                    lpdata = (long *)&pDDDI->guidDeviceIdentifier.Data4[4];
                    *lpdata = *lpdata ^ pDDDI->dwRevision;
                    *lpdata = *lpdata ^ pDDDI->liDriverVersion.HighPart;

                    /* FIXME pDDDI->dwWHQLLevel
                     * we leave this with no informations, I do not known 
                     * if program care for it, I mark this api done, and 
                     * tested, no bugs was found in it
                     */
                    pDDDI->dwWHQLLevel = 0;
                    retVal = DD_OK;
                }

                break;
            }

            iDevNum++;
         }

    }
    _SEH_HANDLE
    {
        retVal = DD_FALSE;
    }
    _SEH_END;

    return retVal;
}

/* 5 of 31 DirectDraw7_Vtable api are working simluare to windows */
/* 8 of 31 DirectDraw7_Vtable api are under devloping / testing */

IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,             /* (QueryInterface testing / devloping) */
    Main_DirectDraw_AddRef,                     /* (AddRef done) */
    Main_DirectDraw_Release,                    /* (QueryInterface testing / devloping) */
    Main_DirectDraw_Compact,                    /* (Compact done) */
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface4,             /* (CreateSurface4 testing / devloping) */
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,           /* (EnumDisplayModes testing / devloping) */
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,             /* (GetDisplayMode testing / devloping) */
    Main_DirectDraw_GetFourCCCodes,             /* (GetFourCCCodes done) */
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,        /* (GetMonitorFrequency done) */
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,                 /* (Initialize done) */
    Main_DirectDraw_RestoreDisplayMode,         /* (RestoreDisplayMode testing / devloping) */
    Main_DirectDraw_SetCooperativeLevel,        /* (SetCooperativeLevel testing / devloping) */
    Main_DirectDraw_SetDisplayMode,             /* (SetDisplayMode testing / devloping) */
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem4,        /* (GetAvailableVidMem4 done) */
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier7,       /* (GetDeviceIdentifier done) */
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
