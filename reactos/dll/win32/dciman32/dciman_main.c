
#include <windows.h>
#include <ndk/ntndk.h>
#include <stdio.h>
#include <dciman.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>
#include <pseh/pseh.h>

CRITICAL_SECTION ddcs;


/* Winwatch internal struct */
typedef struct _WINWATCH_INT
{
    HWND hWnd;
    BOOL WinWtachStatus;
} WINWATCH_INT, *LPWINWATCH_INT;


/* DCI internal struct */
typedef struct _DCISURFACE_LCL
{
    BOOL LostSurface;
    DDRAWI_DIRECTDRAW_GBL DirectDrawGlobal;
    DDRAWI_DDRAWSURFACE_GBL SurfaceGlobal;
    DDRAWI_DDRAWSURFACE_LCL SurfaceLocal;
    DDHAL_DDCALLBACKS DDCallbacks;
    DDHAL_DDSURFACECALLBACKS DDSurfaceCallbacks;

} DCISURFACE_LCL, *LPDCISURFACE_LCL;

typedef struct _DCISURFACE_INT
{
    DCISURFACE_LCL DciSurface_lcl;
    DCISURFACEINFO DciSurfaceInfo;
} DCISURFACE_INT, *LPDCISURFACE_INT;


BOOL
WINAPI
DllMain( HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID reserved )
{
    switch(ul_reason_for_call)
    {
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection( &ddcs );
            break;

        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection( &ddcs );
            break;
    }
    return TRUE;
}

/*++
* @name HDC WINAPI DCIOpenProvider(void)
* @implemented
*
* The function DCIOpenProvider are almost same as CreateDC, it is more simple to use,
* you do not need type which monitor or graphic card you want to use, it will use
*  the current one that the program is running on, so it simple create a hdc, and
*  you can use gdi32 api without any problem

* @return
* Returns a hdc that it have create by gdi32.dll CreateDC
*
* @remarks.
* None
*
*--*/
HDC WINAPI
DCIOpenProvider(void)
{
    HDC hDC;
    if (GetSystemMetrics(SM_CMONITORS) > 1)
    {
        /* FIXME we need add more that one mointor support for DCIOpenProvider, need exaime how */
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return NULL;
    }
    else
    {
        /* Note Windows using the word "Display" not with big letter to create the hdc
         * MS gdi32 does not care if the Display spell with big or small letters */

        /* Note it only create a hDC handle and send it back. */
        hDC = CreateDCW(L"Display",NULL, NULL, NULL);
    }

    return hDC;
}

/*++
* @name void WINAPI DCICloseProvider(HDC hdc)
* @implemented
*
* Same as DeleteDC, it call indirecly to DeleteDC, it simple free and destory the hdc.

* @return
* None
*
* @remarks.
* None
*
*--*/
void WINAPI
DCICloseProvider(HDC hDC)
{
    /* Note it only delete the hdc */
    DeleteDC(hDC);
}

/*++
* @name int WINAPI DCICreatePrimary(HDC hDC, LPDCISURFACEINFO *pDciSurfaceInfo)
* @implemented
*
* Create a primary directdraw surface.
*
* @return
* if it fail it return DCI_FAIL_* error codes, if it sussess it return DCI_OK.
*
* @remarks.
* none
*
*--*/
int WINAPI 
DCICreatePrimary(HDC hDC, LPDCISURFACEINFO *pDciSurfaceInfo)
{
    int retvalue = DCI_FAIL_GENERIC;
    BOOL bNewMode = FALSE;
    LPDCISURFACE_INT pDciSurface_int;
    DDHALINFO HalInfo;
    DDHAL_DDPALETTECALLBACKS DDPaletteCallbacks;
    DDHAL_CREATESURFACEDATA lpcsd;
    DDSURFACEDESC DDSurfaceDesc;
    LPDDRAWI_DDRAWSURFACE_LCL SurfaceLocal_List[1];

    /* Alloc memory for the internal struct, rember this internal struct are not compatible with windows xp */
    if ( (pDciSurface_int = (LPDCISURFACE_INT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DCISURFACE_INT) )) == NULL )
    {
        /* Fail alloc memmory */
        retvalue = DCI_ERR_OUTOFMEMORY;
    }
    else
    {
        /* Activate the DirectX support from HAL, if any of these function fail, we was not avail setup infomrations we need to create our frame */
        if ( ( DdCreateDirectDrawObject( &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal,
                                         hDC) == FALSE) ||
             ( DdReenableDirectDrawObject(&pDciSurface_int->DciSurface_lcl.DirectDrawGlobal,
                                      &bNewMode) == FALSE) ||
             ( DdQueryDirectDrawObject( &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal,
                                        &HalInfo,
                                        &pDciSurface_int->DciSurface_lcl.DDCallbacks, 
                                        &pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks,
                                        &DDPaletteCallbacks,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL) == FALSE) )
        {
            /* Fail to get importment Directx informations */
            retvalue = DCI_FAIL_UNSUPPORTED;
        }
        else
        {
            /* We got all informations to start create our primary surface */

            /* Will be use later, it check see if we lost the surface or not */
            pDciSurface_int->DciSurface_lcl.LostSurface = FALSE;

            /* Setup DirectDrawGlobal */
            memcpy(&pDciSurface_int->DciSurface_lcl.DirectDrawGlobal.vmiData,&HalInfo.vmiData,sizeof(VIDMEMINFO));

            /* Setup SurfaceLocal */
            pDciSurface_int->DciSurface_lcl.SurfaceLocal.lpGbl = &pDciSurface_int->DciSurface_lcl.SurfaceGlobal;
            pDciSurface_int->DciSurface_lcl.SurfaceLocal.hDDSurface = 0;
            pDciSurface_int->DciSurface_lcl.SurfaceLocal.ddsCaps.dwCaps = DDSCAPS_VISIBLE;

            /* Setup SurfaceGlobal */
            pDciSurface_int->DciSurface_lcl.SurfaceGlobal.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;
            pDciSurface_int->DciSurface_lcl.SurfaceGlobal.wHeight = HalInfo.vmiData.dwDisplayHeight;
            pDciSurface_int->DciSurface_lcl.SurfaceGlobal.wWidth = HalInfo.vmiData.dwDisplayWidth;
            pDciSurface_int->DciSurface_lcl.SurfaceGlobal.lPitch = HalInfo.vmiData.lDisplayPitch;

            /* Clear some internal struct before we fill them */
            memset(&lpcsd,0,sizeof(DDHAL_CREATESURFACEDATA));
            memset(&DDSurfaceDesc,0,sizeof(DDSURFACEDESC));

            /* Setup DDSURFACEDESC for createsurface */
            DDSurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
            DDSurfaceDesc.dwFlags = 1;
            DDSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VISIBLE ;

            /* Setup DDHAL_CREATESURFACEDATA for createsurface */
            lpcsd.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;
            lpcsd.lpDDSurfaceDesc = &DDSurfaceDesc;
            SurfaceLocal_List[0] = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;
            lpcsd.lplpSList = SurfaceLocal_List;
            lpcsd.dwSCnt = 1;
            lpcsd.ddRVal = DDERR_GENERIC;

            /* Check see if dx hal support CreateSurface */
            if ( (pDciSurface_int->DciSurface_lcl.DDCallbacks.dwFlags & DDHAL_CB32_CREATESURFACE) == DDHAL_CB32_CREATESURFACE)
            {
                lpcsd.CreateSurface = pDciSurface_int->DciSurface_lcl.DDCallbacks.CreateSurface;
            }

            /* Now try create our surface */
            if (lpcsd.CreateSurface != NULL )
            {
                retvalue = lpcsd.CreateSurface(&lpcsd);
            }

            if ( (lpcsd.CreateSurface == NULL) ||
                 (retvalue == DDHAL_DRIVER_NOTHANDLED) ||
                 (lpcsd.ddRVal != DD_OK) )
            {
                /* Fail create the surface, return DCI_FAIL_GENERIC */
            }
            else
            {
                /* Sussess to create the surface */
                retvalue = DCI_OK;

                /* Rest visrgn, with no windows */
                DdResetVisrgn(&pDciSurface_int->DciSurface_lcl.SurfaceLocal, (HWND)-1);

                /* FIXME we need deal with if DdResetVisrgn fail */

                /* Fill in the size of DCISURFACEINFO struct */
                pDciSurface_int->DciSurfaceInfo.dwSize = sizeof(DCISURFACEINFO);

                /* Fill in the surface is visible, for it always are the primary surface and it is visible */
                pDciSurface_int->DciSurfaceInfo.dwDCICaps = DCI_VISIBLE;

                /* for primary screen res lower that 256Color, ms dciman.dll set this value to BI_RGB 
                 * old note I found in ddk header comment follow dwMask is type for BI_BITMASK surfaces
                 * and for dwCompression it was which format the surface was create in, I also tested with older graphic card like S3
                 * that support 16Color or lower for the primary display. and saw this member change betwin 0 (BI_RGB) and 3 (BI_BITFIELDS).
                 */
                if (HalInfo.vmiData.ddpfDisplay.dwRGBBitCount >= 8)
                {
                    pDciSurface_int->DciSurfaceInfo.dwCompression = BI_BITFIELDS;
                }
                else
                {
                    pDciSurface_int->DciSurfaceInfo.dwCompression = BI_RGB;
                }

                /* Fill in the RGB mask */
                pDciSurface_int->DciSurfaceInfo.dwMask[0] = HalInfo.vmiData.ddpfDisplay.dwRBitMask;
                pDciSurface_int->DciSurfaceInfo.dwMask[1] = HalInfo.vmiData.ddpfDisplay.dwGBitMask;
                pDciSurface_int->DciSurfaceInfo.dwMask[2] = HalInfo.vmiData.ddpfDisplay.dwBBitMask;

                /* Fill in the width and height of the primary surface */
                pDciSurface_int->DciSurfaceInfo.dwWidth = HalInfo.vmiData.dwDisplayWidth;
                pDciSurface_int->DciSurfaceInfo.dwHeight = HalInfo.vmiData.dwDisplayHeight;

                /* Fill in the color deep and stride of the primary surface */
                pDciSurface_int->DciSurfaceInfo.dwBitCount = HalInfo.vmiData.ddpfDisplay.dwRGBBitCount;
                pDciSurface_int->DciSurfaceInfo.lStride = HalInfo.vmiData.lDisplayPitch;

                /* Send back only LPDCISURFACEINFO struct and other are only internal use for this object */
                *pDciSurfaceInfo = (LPDCISURFACEINFO) &pDciSurface_int->DciSurfaceInfo;
            }
        }
    }

/* Cleanup code when we fail */
    if (retvalue < DCI_OK)
    {
        if (pDciSurface_int->DciSurface_lcl.DirectDrawGlobal.hDD != 0)
        {
            DdDeleteDirectDrawObject(&pDciSurface_int->DciSurface_lcl.DirectDrawGlobal);
        }

        if (pDciSurface_int != NULL)
        {
            HeapFree(GetProcessHeap(), 0, pDciSurface_int);
        }
    }

    return retvalue;
}

/*++
* @name void WINAPI DCIDestroy(LPDCISURFACEINFO pDciSurfaceInfo)
* @implemented
*
* It destory the primary surface and all data that have been alloc from DCICreatePrimary

* @return
* None
*
* @remarks.
* None
*
*--*/
void WINAPI
DCIDestroy(LPDCISURFACEINFO pDciSurfaceInfo)
{
    DDHAL_DESTROYSURFACEDATA lpcsd;
    LPDCISURFACE_INT pDciSurface_int = NULL;
    if (pDciSurfaceInfo != NULL)
    {
        /* Get the internal data for our pdci struct */
        pDciSurface_int = (LPDCISURFACE_INT) (((DWORD) pDciSurfaceInfo) - sizeof(DCISURFACE_LCL)) ;

        /* If we lost the primary surface we do not destory it, */
        if (pDciSurface_int->DciSurface_lcl.LostSurface == FALSE)
        {

            /* Check see if we have a Ddraw object or not */
            if (pDciSurface_int->DciSurface_lcl.DirectDrawGlobal.hDD != 0)
            {
                
                /* Destory the surface */
                if ( (( pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) == DDHAL_SURFCB32_DESTROYSURFACE) &&
                     (pDciSurface_int->DciSurface_lcl.SurfaceLocal.hDDSurface != 0) )
                {
                    /* setup data to destory primary surface if we got one */
                    lpcsd.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;
                    lpcsd.lpDDSurface = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;
                    lpcsd.ddRVal = DDERR_GENERIC;
                    lpcsd.DestroySurface = pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface;

                    /* Destory the surface */
                    lpcsd.DestroySurface(&lpcsd);

                    /* Do noting if we fail to destory the surface */
                }

                /* Destroy the Ddraw object */
                DdDeleteDirectDrawObject(&pDciSurface_int->DciSurface_lcl.DirectDrawGlobal);
            }

            /* Free the alloc memmory for the internal and for pDciSurfaceInfo */
            HeapFree(GetProcessHeap(), 0, pDciSurface_int);
        }
    }
}

/*++
* @name DWORD WINAPI GetDCRegionData(HDC hdc, DWORD size, LPRGNDATA prd)
* @implemented
*
* it give the region data, simple it fill in the prd

* @return
* return value 0 meain it fails,
* if the sussess the size value and return value are same, 
* if the prd is null the return value contains the number of bytes needed for the region data. 
*
* @remarks.
* None
*/
DWORD WINAPI
GetDCRegionData(HDC hdc, DWORD size, LPRGNDATA prd)
{
    DWORD retvalue = 0;
    HRGN hRgn = CreateRectRgn(0,0,0,0);

    if (hRgn != NULL)
    {
        GetRandomRgn(hdc,hRgn,SYSRGN);
        retvalue = GetRegionData(hRgn,size,prd);
        DeleteObject(hRgn);
    }

    return retvalue;
}

/*++
* @name DWORD WINAPI GetWindowRegionData(HWND hwnd, DWORD size, LPRGNDATA prd);
* @implemented
*

* @return
*
* @remarks.
* None
*/
DWORD WINAPI
GetWindowRegionData(HWND hwnd, DWORD size, LPRGNDATA prd)
{
    DWORD retvalue = 0;

    HDC hDC = GetDC(hwnd);

    if (hDC != NULL)
    {
        retvalue = GetDCRegionData(hDC,size,prd);
        ReleaseDC(hwnd,hDC);
    }

    return retvalue;
}

HWINWATCH WINAPI
WinWatchOpen(HWND hwnd)
{
    LPWINWATCH_INT pWinwatch_int;

    EnterCriticalSection(&ddcs);

    if ( (pWinwatch_int = (LPWINWATCH_INT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINWATCH_INT) )) != NULL )
    {
        pWinwatch_int->hWnd = hwnd;
    }
    LeaveCriticalSection(&ddcs);
    return (HWINWATCH) pWinwatch_int;
}

void WINAPI
WinWatchClose(HWINWATCH hWW)
{
    EnterCriticalSection(&ddcs);

    if (hWW != NULL)
    {
        HeapFree(GetProcessHeap(), 0, hWW);
    }

    LeaveCriticalSection(&ddcs);
}

BOOL WINAPI
WinWatchDidStatusChange(HWINWATCH hWW)
{
    LPWINWATCH_INT pWinwatch_int = (LPWINWATCH_INT)hWW;
    return pWinwatch_int->WinWtachStatus;
}


/***********************************************************************************************************/
/***********************************************************************************************************/
/***********************************************************************************************************/
/* All function under here are not supported in windows nt / ReactOS, they only return error code or false */
/***********************************************************************************************************/

/*++
* @name int WINAPI DCICreateOffscreen(HDC hdc)
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
int WINAPI 
DCICreateOffscreen(HDC hdc,
                   DWORD dwCompression,
                   DWORD dwRedMask,
                   DWORD dwGreenMask,
                   DWORD dwBlueMask,
                   DWORD dwWidth,
                   DWORD dwHeight,
                   DWORD dwDCICaps,
                   DWORD dwBitCount,
                   LPDCIOFFSCREEN *lplpSurface)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*++
* @name int WINAPI DCICreateOverlay(HDC hdc, LPVOID lpOffscreenSurf, LPDCIOVERLAY *lplpSurface)
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
int WINAPI 
DCICreateOverlay(HDC hdc,
                 LPVOID lpOffscreenSurf,
                 LPDCIOVERLAY *lplpSurface)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*++
* @name DCIRVAL WINAPI DCISetDestination(LPDCIOFFSCREEN pdci, LPRECT dst, LPRECT src)
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
DCIRVAL WINAPI
DCISetDestination(LPDCIOFFSCREEN pdci, LPRECT dst, LPRECT src)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*++
* @name DCIRVAL WINAPI DCIDraw(LPDCIOFFSCREEN pdci)
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
DCIRVAL WINAPI
DCIDraw(LPDCIOFFSCREEN pdci)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*++
* @name int WINAPI DCIEnum(HDC hdc, LPRECT lprDst, LPRECT lprSrc, LPVOID lpFnCallback, LPVOID lpContext);
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
int WINAPI
DCIEnum(HDC hdc,
        LPRECT lprDst,
        LPRECT lprSrc,
        LPVOID lpFnCallback,
        LPVOID lpContext)
{
    return DCI_FAIL_UNSUPPORTED;
}
/*++
* @name DCIRVAL WINAPI DCISetClipList(LPDCIOFFSCREEN pdci, LPRGNDATA prd)
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
DCIRVAL WINAPI
DCISetClipList(LPDCIOFFSCREEN pdci,
               LPRGNDATA prd)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*++
* @name DCIRVAL WINAPI DCISetSrcDestClip(LPDCIOFFSCREEN pdci, LPRECT srcrc, LPRECT destrc, LPRGNDATA prd)
* @implemented
*
* Not supported in windows, it only return DCI_FAIL_UNSUPPORTED.

* @return
* DCI_FAIL_UNSUPPORTED
*
* @remarks.
* None
*
*--*/
DCIRVAL WINAPI
DCISetSrcDestClip(LPDCIOFFSCREEN pdci,
                  LPRECT srcrc,
                  LPRECT destrc,
                  LPRGNDATA prd)
{
    return DCI_FAIL_UNSUPPORTED;
}

/*++
* @name BOOL WINAPI WinWatchNotify(HWINWATCH hWW, WINWATCHNOTIFYPROC NotifyCallback, LPARAM NotifyParam );
* @implemented
*
* Not supported in windows, it only return FALSE.

* @return
* FALSE
*
* @remarks.
* None
*
*--*/
BOOL WINAPI
WinWatchNotify(HWINWATCH hWW,
               WINWATCHNOTIFYPROC NotifyCallback,
               LPARAM NotifyParam )
{
    return FALSE;
}





