/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/eng.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       30/12-2007   Magnus Olsen
 */

#include <dxg_int.h>

/*++
* @name intDdGetDriverInfo
* @implemented
*
* The function intDdGetDriverInfo is used internally in dxg.sys
* It retrieves driver information structures
*
* @param PEDD_DIRECTDRAW_GLOBAL peDdGl
* DirectDraw global structure
*
* @param GUID guid
* GUID of InfoData to read
*
* @param PVOID callbackStruct
* Callback structure pointer
*
* @param ULONG callbackSize
* Size of allocated callback structure
*
* @param ULONG *returnSize
* Desired structure size returned by driver
*
* @return
* Returns true on successful execution, false when error. 
*
* @remarks.
* Only used internally in dxg.sys
*--*/
BOOL intDdGetDriverInfo(PEDD_DIRECTDRAW_GLOBAL peDdGl, GUID guid, PVOID callbackStruct, ULONG callbackSize, ULONG *returnSize)
{
    DD_GETDRIVERINFODATA ddGetDriverInfoData;

    if (peDdGl->ddHalInfo.dwFlags & DDHALINFO_GETDRIVERINFOSET && peDdGl->ddHalInfo.GetDriverInfo)
    {
        memset(&ddGetDriverInfoData, 0, sizeof(DD_GETDRIVERINFODATA));
        ddGetDriverInfoData.dwSize = sizeof(DD_GETDRIVERINFODATA);
        ddGetDriverInfoData.dhpdev = peDdGl->dhpdev;
        memcpy(&ddGetDriverInfoData.guidInfo, &guid, sizeof(GUID));
        ddGetDriverInfoData.dwExpectedSize = callbackSize;
        ddGetDriverInfoData.lpvData = callbackStruct;
        ddGetDriverInfoData.ddRVal = DDERR_CURRENTLYNOTAVAIL;
        if (peDdGl->ddHalInfo.GetDriverInfo(&ddGetDriverInfoData) && !ddGetDriverInfoData.ddRVal)
        {
            if (returnSize)
                *returnSize = ddGetDriverInfoData.dwActualSize;
            return TRUE;
        }

    }

    /* cleanup on error */
    memset(callbackStruct, 0, callbackSize); 
    if (returnSize)
        *returnSize = 0;
    return FALSE;
}


PDD_SURFACE_LOCAL
NTAPI
DxDdLockDirectDrawSurface(HANDLE hDdSurface)
{
   PEDD_SURFACE pEDDSurface = NULL;
   PDD_SURFACE_LOCAL pSurfacelcl = NULL;

   pEDDSurface = DdHmgLock(hDdSurface, ObjType_DDSURFACE_TYPE, FALSE);
   if (pEDDSurface != NULL)
   {
        pSurfacelcl = &pEDDSurface->ddsSurfaceLocal;
   }

   return pSurfacelcl;
}

BOOL
NTAPI
DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    BOOL retVal = FALSE;
    //PEDD_SURFACE pEDDSurface  = NULL;

    if (pSurface)
    {
        // pEDDSurface = (PEDD_SURFACE)( ((PBYTE)pSurface) - sizeof(DD_BASEOBJECT));
        // InterlockedDecrement(&pEDDSurface->pobj.cExclusiveLock);
        retVal = TRUE;
    }

    return retVal;
}

/*++
* @name intDdGetAllDriverInfo
* @implemented
*
* The function intDdGetAllDriverInfo is used internally in dxg.sys
* It retrieves all possible driver information structures
*
* @param PEDD_DIRECTDRAW_GLOBAL peDdGl
* Pointer to destination DirectDrawGlobal structure 
*
* @remarks.
* Only used internally in dxg.sys
* Missing some callbacks (VideoPort, DxApi, AGP)
*--*/
VOID intDdGetAllDriverInfo(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    if (peDdGl->ddHalInfo.GetDriverInfo && peDdGl->ddHalInfo.dwFlags & DDHALINFO_GETDRIVERINFOSET)
    {
        intDdGetDriverInfo(peDdGl, GUID_KernelCaps, &peDdGl->ddKernelCaps, sizeof(peDdGl->ddKernelCaps), 0);
        intDdGetDriverInfo(peDdGl, GUID_KernelCallbacks, &peDdGl->ddKernelCallbacks, sizeof(peDdGl->ddKernelCallbacks), 0);

        if (intDdGetDriverInfo(peDdGl, GUID_D3DCallbacks3, &peDdGl->d3dNtHalCallbacks3, sizeof(peDdGl->d3dNtHalCallbacks3), 0))
            peDdGl->dwCallbackFlags |= EDDDGBL_D3DCALLBACKS3;

        if (intDdGetDriverInfo(peDdGl, GUID_ColorControlCallbacks, &peDdGl->ddColorControlCallbacks, sizeof(peDdGl->ddColorControlCallbacks), 0))
            peDdGl->dwCallbackFlags |= EDDDGBL_COLORCONTROLCALLBACKS;

        if (intDdGetDriverInfo(peDdGl, GUID_MiscellaneousCallbacks, &peDdGl->ddMiscellanousCallbacks, sizeof(peDdGl->ddMiscellanousCallbacks), 0))
            peDdGl->dwCallbackFlags |= EDDDGBL_MISCCALLBACKS;

        if (intDdGetDriverInfo(peDdGl, GUID_Miscellaneous2Callbacks, &peDdGl->ddMiscellanous2Callbacks, sizeof(peDdGl->ddMiscellanous2Callbacks), 0))
            peDdGl->dwCallbackFlags |= EDDDGBL_MISC2CALLBACKS;

        if (intDdGetDriverInfo(peDdGl, GUID_NTCallbacks, &peDdGl->ddNtCallbacks, sizeof(peDdGl->ddNtCallbacks), 0) )
            peDdGl->dwCallbackFlags |= EDDDGBL_NTCALLBACKS;

        if (intDdGetDriverInfo(peDdGl, GUID_DDMoreCaps, &peDdGl->ddMoreCaps, sizeof(peDdGl->ddMoreCaps), 0) )
            peDdGl->dwCallbackFlags |= EDDDGBL_DDMORECAPS;

        if (intDdGetDriverInfo(peDdGl, GUID_NTPrivateDriverCaps, &peDdGl->ddNtPrivateDriverCaps, sizeof(peDdGl->ddNtPrivateDriverCaps), 0) )
            peDdGl->dwCallbackFlags |= EDDDGBL_PRIVATEDRIVERCAPS;

        if (intDdGetDriverInfo(peDdGl, GUID_MotionCompCallbacks, &peDdGl->ddMotionCompCallbacks, sizeof(peDdGl->ddMotionCompCallbacks), 0) )
            peDdGl->dwCallbackFlags |= EDDDGBL_MOTIONCOMPCALLBACKS;
    }
}


/*++
* @name intDdEnableDriver
* @implemented
*
* The function intDdEnableDriver is used internally in dxg.sys
* Fills in all EDD_DIRECTDRAW_GLOBAL structures and enables DirectDraw acceleration when possible
*
* @param PEDD_DIRECTDRAW_GLOBAL peDdGl
* Pointer to destination DirectDrawGlobal structure 
*
* @remarks.
* Only used internally in dxg.sys
*--*/
VOID intDdEnableDriver(PEDD_DIRECTDRAW_GLOBAL peDdGl)
{
    PDRIVER_FUNCTIONS  DriverFunctions;
    LPD3DNTHAL_GLOBALDRIVERDATA GlobalDriverData;
    LPD3DNTHAL_CALLBACKS HalCallbacks;
    PDD_D3DBUFCALLBACKS D3DBufCallbacks;


    gpEngFuncs.DxEngLockHdev(peDdGl->hDev);
    DriverFunctions = (PDRIVER_FUNCTIONS)gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_DrvFuncs);

    // check if driver has DirectDraw functions
    if ((!DriverFunctions->GetDirectDrawInfo)||(!DriverFunctions->EnableDirectDraw)||(!DriverFunctions->DisableDirectDraw))
        peDdGl->dhpdev = 0;


    // reset acceleration flag
    peDdGl->fl = peDdGl->fl & 0xFFFFFFFE;

    // ask for structure sizes
    if ((peDdGl->dhpdev)&&(DriverFunctions->GetDirectDrawInfo(peDdGl->dhpdev, &peDdGl->ddHalInfo, &peDdGl->dwNumHeaps, NULL, &peDdGl->dwNumFourCC, NULL)))
    {
        // allocate memory for DX data
        if (peDdGl->dwNumHeaps)
            peDdGl->pvmList = EngAllocMem(FL_ZERO_MEMORY, peDdGl->dwNumHeaps*sizeof(VIDEOMEMORY), TAG_GDDV);
        if (peDdGl->dwNumFourCC)
            peDdGl->pdwFourCC = EngAllocMem(FL_ZERO_MEMORY, peDdGl->dwNumFourCC * 4, TAG_GDDF);

        // get data from driver
        if (!DriverFunctions->GetDirectDrawInfo(peDdGl->dhpdev, &peDdGl->ddHalInfo, &peDdGl->dwNumHeaps, peDdGl->pvmList, &peDdGl->dwNumFourCC, peDdGl->pdwFourCC))
        {
            // failed - cleanup and exit
            if (peDdGl->pvmList)
                EngFreeMem(peDdGl->pvmList);
            if (peDdGl->pdwFourCC)
                EngFreeMem(peDdGl->pdwFourCC);
            gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
            return;
        }

        // check if we can enable DirectDraw acceleration
        if ((peDdGl->ddHalInfo.vmiData.pvPrimary) &&
            (DriverFunctions->EnableDirectDraw(peDdGl->dhpdev, &peDdGl->ddCallbacks, &peDdGl->ddSurfaceCallbacks, &peDdGl->ddPaletteCallbacks))&&
            !(gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_dd_flags) & CapOver_DisableD3DAccel)&&
            (peDdGl->ddHalInfo.dwSize == sizeof(DD_HALINFO)))
        {
            GlobalDriverData = peDdGl->ddHalInfo.lpD3DGlobalDriverData;
            HalCallbacks = peDdGl->ddHalInfo.lpD3DHALCallbacks;
            D3DBufCallbacks = peDdGl->ddHalInfo.lpD3DHALCallbacks;

            if (GlobalDriverData && GlobalDriverData->dwSize == sizeof(D3DNTHAL_GLOBALDRIVERDATA))
                memcpy(&peDdGl->d3dNtGlobalDriverData, GlobalDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));

            if (HalCallbacks && HalCallbacks->dwSize == sizeof(D3DNTHAL_CALLBACKS))
                memcpy(&peDdGl->d3dNtHalCallbacks, HalCallbacks, sizeof(D3DNTHAL_CALLBACKS));

            if (D3DBufCallbacks && D3DBufCallbacks->dwSize == sizeof(DD_D3DBUFCALLBACKS))
                memcpy(&peDdGl->d3dBufCallbacks, D3DBufCallbacks, sizeof(DD_D3DBUFCALLBACKS));

            intDdGetAllDriverInfo(peDdGl);

            // enable DirectDraw acceleration
            peDdGl->fl |= 1;
        }
        else
        {
            // failed - cleanup and exit
            if (peDdGl->pvmList)
                EngFreeMem(peDdGl->pvmList);
            if (peDdGl->pdwFourCC)
                EngFreeMem(peDdGl->pdwFourCC);
        }
    }

    gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
}

/*++
* @name DxDdEnableDirectDraw
* @implemented
*
* Function enables DirectDraw
*
* @param PEDD_DIRECTDRAW_GLOBAL peDdGl
* Pointer to destination DirectDrawGlobal structure 
*--*/
BOOL
NTAPI
DxDdEnableDirectDraw(HANDLE hDev, BOOL arg2/*What for?*/)
{
    PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;

    if (gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_display))
    {
        peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);
        peDdGl->hDev = hDev;
        peDdGl->bSuspended = FALSE;
        peDdGl->dhpdev = (PVOID)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_dhpdev);
        intDdEnableDriver(peDdGl);
        return TRUE;
    }

    return FALSE;
}
