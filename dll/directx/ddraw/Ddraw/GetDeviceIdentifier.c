/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/GetDeviceIdentifier.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen
 *
 */

/* TODO
 * We need adding digtial signarture detections for the drivers
 * and count out which level the signtature driver got, the value
 * shall be save to pDDDI->dwWHQLLevel, But I do not known how todo
 * this part yet, That is only missing feature in this functions
 *
 * Write a UML digram for this api
 */

#include "rosdraw.h"

#include <string.h>

/* For DirectDraw 4 - 6 */
HRESULT WINAPI
Main_DirectDraw_GetDeviceIdentifier(LPDDRAWI_DIRECTDRAW_INT This,
                                    LPDDDEVICEIDENTIFIER pDDDI, DWORD dwFlags)
{
    HRESULT retVal = DD_OK;
    DDDEVICEIDENTIFIER2 pDDDI2;

    ZeroMemory(&pDDDI2,sizeof(DDDEVICEIDENTIFIER2));

    _SEH2_TRY
    {
        memcpy(&pDDDI2 , pDDDI, sizeof(DDDEVICEIDENTIFIER));

        retVal = Main_DirectDraw_GetDeviceIdentifier7(This, &pDDDI2, dwFlags);

        if (IsBadWritePtr(pDDDI, sizeof(DDDEVICEIDENTIFIER)))
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            memcpy(pDDDI , &pDDDI2, sizeof(DDDEVICEIDENTIFIER) );
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
Main_DirectDraw_GetDeviceIdentifier7(LPDDRAWI_DIRECTDRAW_INT This,
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

    DX_WINDBG_trace();

    EnterCriticalSection( &ddcs );

    _SEH2_TRY
    {
        if ( (IsBadWritePtr( pDDDI, sizeof(DDDEVICEIDENTIFIER2) ) ) ||
             (dwFlags & ~DDGDI_GETHOSTIDENTIFIER))
        {
            retVal = DDERR_INVALIDPARAMS;
            _SEH2_LEAVE;
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

            if (_stricmp(DisplayDeviceA.DeviceName, This->lpLcl->lpGbl->cDriverName) != 0)
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

                    pDDDI->guidDeviceIdentifier.Data1 ^= pDDDI->dwVendorId;

                    lpdata = (long *)&pDDDI->guidDeviceIdentifier.Data2;
                    *lpdata ^= pDDDI->dwDeviceId;

                    lpdata = (long *)&pDDDI->guidDeviceIdentifier.Data4;
                    *lpdata = (*lpdata ^ pDDDI->dwSubSysId) ^ pDDDI->liDriverVersion.LowPart;

                    lpdata = (long *)&pDDDI->guidDeviceIdentifier.Data4[4];
                    *lpdata = (*lpdata ^ pDDDI->dwRevision) ^ pDDDI->liDriverVersion.HighPart;

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
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        retVal = DD_FALSE;
    }
    _SEH2_END;

    LeaveCriticalSection( &ddcs );
    return retVal;
}
