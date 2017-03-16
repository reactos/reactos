/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             win32ss/reactx/dxg/eng.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 *                   Sebastian Gasiorek (sebastian.gasiorek@reactos.org)
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

PVOID
FASTCALL
intDdCreateDirectDrawLocal(HDEV hDev)
{
    PEDD_DIRECTDRAW_GLOBAL peDdGl = NULL;
    PEDD_DIRECTDRAW_LOCAL peDdL = NULL;
    PDD_ENTRY AllocRet;

    peDdGl = (PEDD_DIRECTDRAW_GLOBAL)gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_eddg);

    AllocRet = DdHmgAlloc(sizeof(EDD_DIRECTDRAW_LOCAL), ObjType_DDLOCAL_TYPE, TRUE);
    if (!AllocRet) 
        return NULL;

    peDdL = (PEDD_DIRECTDRAW_LOCAL)AllocRet->pobj;

    /* initialize DIRECTDRAW_LOCAL */
    peDdL->peDirectDrawLocal_prev = peDdGl->peDirectDrawLocalList;
    peDdL->UniqueProcess = PsGetCurrentThreadProcessId();
    peDdL->Process = PsGetCurrentProcess();

    // link DirectDrawGlobal and DirectDrawLocal
    peDdGl->peDirectDrawLocalList = peDdL;
    peDdL->peDirectDrawGlobal = peDdGl;
    peDdL->peDirectDrawGlobal2 = peDdGl;

    gpEngFuncs.DxEngReferenceHdev(hDev);

    InterlockedExchangeAdd((LONG*)&peDdL->pobj.cExclusiveLock, 0xFFFFFFFF);

    return peDdL->pobj.hHmgr;
}

/*++
* @name DxDdCreateDirectDrawObject
* @implemented
*
* Function creates new DirectDraw object
*
* @param HDC hDC
* Device context handle 
*
* @return
* Newly created DirectDraw object handle. 
*
* @remarks.
* Missing all AGP stuff
*--*/
DWORD
NTAPI
DxDdCreateDirectDrawObject(
    HDC hDC)
{
    PDC pDC = NULL;
    HDEV hDev = NULL;
    DWORD retVal = 0;

    pDC = gpEngFuncs.DxEngLockDC(hDC);
    if (!pDC)
        return 0;

    // get driver hDev from DC
    hDev = (HDEV)gpEngFuncs.DxEngGetDCState(hDC, 3);
    if (!hDev) {
        gpEngFuncs.DxEngUnlockDC(pDC);
        return 0;
    }

    // is this primary display?
    if (!gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_display))
    {
        gpEngFuncs.DxEngUnlockDC(pDC);
        return 0;
    }

    gpEngFuncs.DxEngLockHdev(hDev);

    // create object only for 8BPP and more
    if (gpEngFuncs.DxEngGetHdevData(hDev, DxEGShDevData_DitherFmt) >= BMF_8BPP)
        retVal = (DWORD)intDdCreateDirectDrawLocal(hDev);

    gpEngFuncs.DxEngUnlockHdev(hDev);
    gpEngFuncs.DxEngUnlockDC(pDC);
    return retVal;
}


/*++
* @name DxDdGetDriverInfo
* @implemented
*
* Function queries the driver for DirectDraw and Direct3D functionality
*
* @param HANDLE DdHandle
* Handle to DirectDraw object 
*
* @param PDD_GETDRIVERINFODATA drvInfoData
* Pointer to in/out driver info data structure 
*--*/
DWORD
NTAPI
DxDdGetDriverInfo(HANDLE DdHandle, PDD_GETDRIVERINFODATA drvInfoData)
{
    PEDD_DIRECTDRAW_LOCAL peDdL;
    PEDD_DIRECTDRAW_GLOBAL peDdGl;
    PVOID pInfo = NULL;
    DWORD dwInfoSize = 0;
    BYTE callbackStruct[1024];
    DWORD RetVal = FALSE;

    peDdL = (PEDD_DIRECTDRAW_LOCAL)DdHmgLock(DdHandle, ObjType_DDLOCAL_TYPE, FALSE);
    if (!peDdL)
        return RetVal;
    
    peDdGl = peDdL->peDirectDrawGlobal2;

    // check VideoPort related callbacks
    if (peDdGl->dwCallbackFlags & EDDDGBL_VIDEOPORTCALLBACKS)
    {
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_VideoPortCallbacks))
        {
            dwInfoSize = sizeof(DD_VIDEOPORTCALLBACKS);
            pInfo = (VOID*)&peDdGl->ddVideoPortCallback;
        }
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_VideoPortCaps))
        {
            pInfo = (VOID*)peDdGl->unk_000c[0];
            dwInfoSize = 72 * peDdGl->ddHalInfo.ddCaps.dwMaxVideoPorts;
        }
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_D3DCallbacks3))
        {
            dwInfoSize = sizeof(D3DNTHAL_CALLBACKS3);
            pInfo = (VOID*)&peDdGl->d3dNtHalCallbacks3;
        }
    }

    // check ColorControl related callbacks
    if (peDdGl->dwCallbackFlags & EDDDGBL_COLORCONTROLCALLBACKS)
    {
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_ColorControlCallbacks))
        {
            dwInfoSize = sizeof(DD_COLORCONTROLCALLBACKS);
            pInfo = (VOID*)&peDdGl->ddColorControlCallbacks;
        }
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_NTCallbacks))
        {
            dwInfoSize = sizeof(DD_NTCALLBACKS);
            pInfo = (VOID*)&peDdGl->ddNtCallbacks;
        }      
    }

    // check Miscellaneous callbacks
    if (peDdGl->dwCallbackFlags & EDDDGBL_MISCCALLBACKS)
    {
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_MiscellaneousCallbacks))
        {
            dwInfoSize = sizeof(DD_MISCELLANEOUSCALLBACKS);
            pInfo = (VOID*)&peDdGl->ddMiscellanousCallbacks;
        }
        if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_DDMoreCaps))
        {
            dwInfoSize = sizeof(DD_MORECAPS);
            pInfo = &peDdGl->ddMoreCaps;
        }
    }

    if (peDdGl->dwCallbackFlags & EDDDGBL_MISC2CALLBACKS && 
        InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_Miscellaneous2Callbacks))
    {
        dwInfoSize = sizeof(DD_MISCELLANEOUS2CALLBACKS);
        pInfo = (VOID*)&peDdGl->ddMiscellanous2Callbacks;
    }

    if (peDdGl->dwCallbackFlags & EDDDGBL_MOTIONCOMPCALLBACKS && 
        InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_MotionCompCallbacks))
    {
        dwInfoSize = sizeof(DD_MOTIONCOMPCALLBACKS);
        pInfo = (VOID*)&peDdGl->ddMotionCompCallbacks;
    }

    if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_KernelCaps) )
    {
        dwInfoSize = sizeof(DD_KERNELCALLBACKS);
        pInfo = &peDdGl->ddKernelCaps;
    }

    if (InlineIsEqualGUID(&drvInfoData->guidInfo, &GUID_DDMoreSurfaceCaps))
    {
        dwInfoSize = sizeof(DDMORESURFACECAPS);
        pInfo = &peDdGl->ddMoreSurfaceCaps;
    }

    if (dwInfoSize && pInfo)
    {
        gpEngFuncs.DxEngLockHdev(peDdGl->hDev);
        intDdGetDriverInfo(peDdGl, drvInfoData->guidInfo, &callbackStruct, dwInfoSize, &dwInfoSize);
        gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
        memcpy(drvInfoData->lpvData, callbackStruct, dwInfoSize);
    }

    InterlockedDecrement((VOID*)&peDdL->pobj.cExclusiveLock);

    return TRUE;
}

/*++
* @name DxDdQueryDirectDrawObject
* @implemented
*
* Function queries the DirectDraw object for its functionality
*  
* @return
* TRUE on success. 
*--*/
BOOL
NTAPI
DxDdQueryDirectDrawObject(
    HANDLE DdHandle,
    DD_HALINFO* pDdHalInfo,
    DWORD* pCallBackFlags,
    LPD3DNTHAL_CALLBACKS pd3dNtHalCallbacks,
    LPD3DNTHAL_GLOBALDRIVERDATA pd3dNtGlobalDriverData,
    PDD_D3DBUFCALLBACKS pd3dBufCallbacks,
    LPDDSURFACEDESC pTextureFormats,
    DWORD* p8,
    VIDEOMEMORY* p9,
    DWORD* pdwNumFourCC,
    DWORD* pdwFourCC)
{
    PEDD_DIRECTDRAW_LOCAL peDdL;
    PEDD_DIRECTDRAW_GLOBAL peDdGl;
    BOOL RetVal = FALSE;

    if (!DdHandle)
        return RetVal;

    if (!pDdHalInfo)
        return RetVal;

    if (!gpEngFuncs.DxEngScreenAccessCheck())
        return RetVal;

    peDdL = (PEDD_DIRECTDRAW_LOCAL)DdHmgLock(DdHandle, ObjType_DDLOCAL_TYPE, FALSE);
    if (peDdL)
    {
        peDdGl = peDdL->peDirectDrawGlobal2;
        gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

        memcpy(pDdHalInfo, &peDdGl->ddHalInfo, sizeof(DD_HALINFO));

        if (pCallBackFlags)
        {
            *(DWORD*)pCallBackFlags = peDdGl->ddCallbacks.dwFlags;
            *(DWORD*)((ULONG)pCallBackFlags + 4) = peDdGl->ddSurfaceCallbacks.dwFlags;
            *(DWORD*)((ULONG)pCallBackFlags + 8) = peDdGl->ddPaletteCallbacks.dwFlags;
        }

        if ( pd3dNtHalCallbacks )
            memcpy(pd3dNtHalCallbacks, &peDdGl->d3dNtHalCallbacks, sizeof(peDdGl->d3dNtHalCallbacks));

        if ( pd3dNtGlobalDriverData )
            memcpy(pd3dNtGlobalDriverData, &peDdGl->d3dNtGlobalDriverData, sizeof(peDdGl->d3dNtGlobalDriverData));

        if ( pd3dBufCallbacks )
            memcpy(pd3dBufCallbacks, &peDdGl->d3dBufCallbacks, sizeof(peDdGl->d3dBufCallbacks));

        if (pTextureFormats)
            memcpy(pTextureFormats, &peDdGl->d3dNtGlobalDriverData.lpTextureFormats, peDdGl->d3dNtGlobalDriverData.dwNumTextureFormats * sizeof(DDSURFACEDESC2));

        if (pdwNumFourCC)
            *pdwNumFourCC = peDdGl->dwNumFourCC;

        if (pdwFourCC)
            memcpy(pdwFourCC, &peDdGl->pdwFourCC, 4 * peDdGl->dwNumFourCC);

        RetVal = TRUE;

        gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
  
        InterlockedDecrement((VOID*)&peDdL->pobj.cExclusiveLock);
    }

    return RetVal;
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

/*++
* @name DxDdReenableDirectDrawObject
* @implemented
*
* Function re-enables DirectDraw object after mode switch
*  
* @param HANDLE DdHandle
* DirectDraw object handle 
*
* @param PVOID p2
* ??? 
*
* @return
* TRUE on success. 
*
* @remarks
* Missing all AGP stuff and second parameter handling
*--*/
DWORD
NTAPI
DxDdReenableDirectDrawObject(
    HANDLE DdHandle,
    PVOID p2)
{
    PEDD_DIRECTDRAW_LOCAL peDdL;
    PEDD_DIRECTDRAW_GLOBAL peDdGl;
    HDC hDC;                     
    DWORD RetVal = FALSE;

    peDdL = (PEDD_DIRECTDRAW_LOCAL)DdHmgLock(DdHandle, ObjType_DDLOCAL_TYPE, FALSE);

    if (!peDdL)
        return RetVal;

    peDdGl = peDdL->peDirectDrawGlobal2;

    hDC = gpEngFuncs.DxEngGetDesktopDC(0, FALSE, FALSE);

    gpEngFuncs.DxEngLockShareSem();
    gpEngFuncs.DxEngLockHdev(peDdGl->hDev);

    if (peDdGl->fl & 1 &&
        gpEngFuncs.DxEngGetDCState(hDC, 2) != 1 &&
        !(gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_OpenRefs)) &&
        !(gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_disable)) &&
        !(gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_dd_nCount)) &&
        gpEngFuncs.DxEngGetHdevData(peDdGl->hDev, DxEGShDevData_DitherFmt) >= BMF_8BPP)
    {
        // reset acceleration and suspend flags
        peDdGl->fl &= 0xFFFFFFFD;
        peDdGl->bSuspended = 0;

        RetVal = TRUE;
        // FIXME AGP Stuff
    }

    gpEngFuncs.DxEngUnlockHdev(peDdGl->hDev);
    gpEngFuncs.DxEngUnlockShareSem();

    InterlockedDecrement((VOID*)&peDdL->pobj.cExclusiveLock);

    return RetVal;
}
