
#include <windows.h>
#include <ndk/ntndk.h>
#include <stdio.h>
#include <dciman.h>
#include <ddraw.h>
#include <ddrawi.h>
#include <d3dhal.h>
#include <ddrawgdi.h>
#include <pseh/pseh.h>

/* ToDO protect some function with pseh */

/* Winwatch internal struct */
typedef struct _WINWATCH_INT
{
	LPVOID prev; 
    HWND hWnd;
    BOOL WinWatchStatus;
    LPRGNDATA lpRgnData;
    UINT RgnDataSize;
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


/* Global value */
LPWINWATCH_INT gpWinWatchList;
CRITICAL_SECTION gcsWinWatchLock;

/* Function */
BOOL
WINAPI
DllMain( HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID reserved )
{
    switch(ul_reason_for_call)
    {
        case DLL_PROCESS_DETACH:
            DeleteCriticalSection( &gcsWinWatchLock );
            break;

        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection( &gcsWinWatchLock );
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

/*++
* @name DWORD WINAPI WinWatchOpen(HWND hwnd)
* @implemented
*

* @return
*
* @remarks.
* None
*/
HWINWATCH WINAPI
WinWatchOpen(HWND hwnd)
{
    LPWINWATCH_INT pWinwatch_int;

    EnterCriticalSection(&gcsWinWatchLock);

    if ( (pWinwatch_int = (LPWINWATCH_INT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINWATCH_INT) )) != NULL )
    {
        pWinwatch_int->hWnd = hwnd;
		pWinwatch_int->prev = (LPVOID) gpWinWatchList;

		gpWinWatchList = pWinwatch_int;
    }
    LeaveCriticalSection(&gcsWinWatchLock);
    return (HWINWATCH) pWinwatch_int;
}

/*++
* @name DWORD WINAPI WinWatchClose(HWND hwnd)
* @implemented
*

* @return
*
* @remarks.
* None
*/
void WINAPI
WinWatchClose(HWINWATCH hWW)
{
	LPWINWATCH_INT pWinwatch_int = (LPWINWATCH_INT)hWW;
	LPWINWATCH_INT tmp_gpWinWatchList ;
	LPWINWATCH_INT delete_gpWinWatchList;

	EnterCriticalSection(&gcsWinWatchLock);

	/* Start search see if our hWW exists in the gpWinWatchList */
	tmp_gpWinWatchList = gpWinWatchList;
	delete_gpWinWatchList = gpWinWatchList;

	while (delete_gpWinWatchList != pWinwatch_int )
	{
		if ( delete_gpWinWatchList == NULL )
		{
			/* Noting to delete */
			break;
		}

		/* Have we found our entry ? */
		if ( tmp_gpWinWatchList->prev == pWinwatch_int )
		{
			/* Yes we found our entry */

			/* Now we save it to delete_gpWinWatchList */ 
			delete_gpWinWatchList = tmp_gpWinWatchList->prev;

			/* Remove it from the gpWinWatchList list */
			if  ( tmp_gpWinWatchList->prev != NULL )
			{
				tmp_gpWinWatchList->prev = ((LPWINWATCH_INT)tmp_gpWinWatchList->prev)->prev ;
			}
			break;
		}	
		else
		{
			/* No so we keep looking */
			tmp_gpWinWatchList = tmp_gpWinWatchList->prev ;
		}
	}

	/* now we can delete our entry */

	if  ( (delete_gpWinWatchList != NULL) && 
		  (pWinwatch_int == delete_gpWinWatchList) )
	{
		/* Check see if we got any region data and free it */	
		if (pWinwatch_int->lpRgnData != NULL)
		{
			HeapFree(GetProcessHeap(), 0, delete_gpWinWatchList->lpRgnData);
		}

		/* Free the delete_gpWinWatchList */
		HeapFree(GetProcessHeap(), 0, delete_gpWinWatchList);    					
	}

	LeaveCriticalSection(&gcsWinWatchLock);	
}

/*++
* @name WinWatchDidStatusChange(HWINWATCH hWW)
* @implemented
*

* @return
*
* @remarks.
* None
*/
BOOL WINAPI
WinWatchDidStatusChange(HWINWATCH hWW)
{
    LPWINWATCH_INT pWinwatch_int = (LPWINWATCH_INT)hWW;
    return pWinwatch_int->WinWatchStatus;
}

/*++
* @name WinWatchGetClipList(HWINWATCH hWW)
* @implemented
*

* @return
*
* @remarks.
* None
*/
UINT WINAPI
WinWatchGetClipList(HWINWATCH hWW,
                    LPRECT prc,
                    UINT size,
                    LPRGNDATA prd)
{

    LPWINWATCH_INT pWinwatch_int = (LPWINWATCH_INT) hWW;
    DWORD BufferRequestSize;
    DWORD NewBufferRequestSize;

    /* Check see if the WinWatch status have changes, it can only be change from DciBeginAccess and DciEndAcess */
    if (pWinwatch_int->WinWatchStatus != FALSE)
    {
        /* The status have change, Rest some internal value */
        pWinwatch_int->WinWatchStatus = FALSE;
        pWinwatch_int->RgnDataSize = 0;

        /* Get the buffer size */
        BufferRequestSize = GetWindowRegionData(pWinwatch_int->hWnd, 0, NULL);

        /* We got a buffer size */
        if (BufferRequestSize != 0)
        {
            /* We need endless loop to get the buffer size, for we need waiting on gdi when it is ready, to give us that */
            while (1)
            {
                /* Check see if we got a buffer already for GetWindowRegionData */
                if (pWinwatch_int->lpRgnData != NULL)
                {
                    /* Free our buffer for GetWindowRegionData */
                    HeapFree(GetProcessHeap(), 0, pWinwatch_int->lpRgnData);
                }

                /* Free our buffer for GetWindowRegionData */
                if ( (pWinwatch_int->lpRgnData = (LPRGNDATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferRequestSize )) == NULL )
                {
                    /* Fail to alloc our buffer for GetWindowRegionData */
                    break;
                }

                /* Get our data from GetWindowRegionData */
                NewBufferRequestSize = GetWindowRegionData(pWinwatch_int->hWnd, BufferRequestSize, pWinwatch_int->lpRgnData);

                /* Check see if the buffer size match */
                if (NewBufferRequestSize == BufferRequestSize)
                {
                    /* The buffer size match */
                    pWinwatch_int->RgnDataSize = BufferRequestSize;
                    break;
                }
                else if (BufferRequestSize == 0)
                {
                    /* No buffer size was given */
                    break;
                }
                BufferRequestSize = NewBufferRequestSize;
            }
        }
    }

    /* Setup return data */
    if (size == pWinwatch_int->RgnDataSize)
    {
        if (pWinwatch_int->RgnDataSize != 0)
        {
            memcpy(prd,pWinwatch_int->lpRgnData,pWinwatch_int->RgnDataSize);
        }
    }

    /* return our value */
    return pWinwatch_int->RgnDataSize;
}



/* Note DCIBeginAccess never return DCI_OK MSDN say it does, but it dose not return it */

DCIRVAL 
WINAPI
DCIBeginAccess(LPDCISURFACEINFO pdci, int x, int y, int dx, int dy)
{
	int retValue = DCI_FAIL_GENERIC;
	int myRetValue = 0;
	DDHALINFO ddHalInfo;
	BOOL bNewMode = FALSE;
	DDHAL_LOCKDATA DdLockData;
	DDSURFACEDESC DdSurfaceDesc;	
	LPDCISURFACE_INT pDciSurface_int;
	LPDDRAWI_DDRAWSURFACE_LCL tmp_DdSurfLcl;
	DDHAL_CREATESURFACEDATA DdCreateSurfaceData;
	DDHAL_DESTROYSURFACEDATA DdDestorySurfaceData;							
				
	pDciSurface_int = (LPDCISURFACE_INT) (((DWORD) pdci) - sizeof(DCISURFACE_LCL)) ;

	/* Check see if we have lost surface or not */
	if ( pDciSurface_int->DciSurface_lcl.LostSurface != FALSE)
	{
		/* Surface was lost, so we set return error code and return */
		retValue = DCI_FAIL_INVALIDSURFACE;
	}
	else
	{	
		/* Surface was not lost  */
		
		/* Setup DdLock when we need get our surface pointer */
		DdLockData.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;		
		DdLockData.lpDDSurface = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;
		DdLockData.bHasRect = 1;
		DdLockData.dwFlags = 0;
		DdLockData.rArea.top = y;
		DdLockData.rArea.left = x;
		DdLockData.rArea.right = dx + x;
		DdLockData.rArea.bottom = y + dy;
				
		/* if we lost the surface or if the display have been change we need restart from here */
		ReStart:

		DdLockData.ddRVal = DDERR_GENERIC;
				
		EnterCriticalSection(&gcsWinWatchLock);
		
		/* Try lock our surface here 
		 *
		 * Note MS DCIMAN32.DLL does not check see if the driver support Lock or not or if it NULL or the flag
		 * it will not crash it will return a error code instead, for SEH will capture it.  
		 */

		pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.Lock( &DdLockData );
		

		/* We need wait until DdLock is ready and finish to draw */
		while ( DdLockData.ddRVal == DDERR_WASSTILLDRAWING )
		{
			pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.Lock( &DdLockData );
		}

		/* DdLock is now ready to continue */
		
		LeaveCriticalSection(&gcsWinWatchLock);
		
		/* Check DdLock return value */
		if ( DdLockData.ddRVal == DD_OK)
		{
			/* Check DdLock sussess so we fill in few members into the struct pdci */
														
			pdci->dwOffSurface = (ULONG_PTR)  ( ( ((DWORD)DdLockData.lpSurfData) - ( ( pdci->dwBitCount / 8 ) * x ) ) - ( pdci->lStride * y ) );
									
			retValue = DCI_STATUS_POINTERCHANGED;
		}
		else if ( DdLockData.ddRVal == DDERR_SURFACELOST)
		{				
			/* Check DdLock did not sussess so we need recreate it, maybe becose the display have been change ? */

			/* Restart the DirectX hardware acclartions */
			if ( DdReenableDirectDrawObject( &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal, &bNewMode ) == 0)
			{
				/* We fail restart it, so we return error code */
				retValue = DCI_ERR_SURFACEISOBSCURED;			
			}
			else
			{							
				/* Get HalInfo from DirectX hardware accalation */
				myRetValue = DdQueryDirectDrawObject ( &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal, 
					                                   &ddHalInfo, 
												       NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
																		
				if ( myRetValue == FALSE ) 
				{
					if ( ( pDciSurface_int->DciSurfaceInfo.dwWidth != ddHalInfo.vmiData.dwDisplayWidth) ||
					     ( pDciSurface_int->DciSurfaceInfo.dwHeight != ddHalInfo.vmiData.dwDisplayHeight ) ||
					     ( pDciSurface_int->DciSurfaceInfo.lStride != ddHalInfo.vmiData.lDisplayPitch ) ||
					     ( pDciSurface_int->DciSurfaceInfo.dwBitCount != ddHalInfo.vmiData.ddpfDisplay.dwRGBBitCount ) )
					{
				
						/* Here we try recreate the lost surface in follow step 
						 * 1. Destory the old surface 
						 * 2. Create the new surface
						 * 3. ResetVisrgn
						 */

						/* 
						* Setup Destorysurface the lost surface so we do not lost any 
						* hDC or hDD handles and memory leaks and destory it
						*/

						DdDestorySurfaceData.lpDD =  &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;					
						DdDestorySurfaceData.lpDDSurface = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;					
						DdDestorySurfaceData.ddRVal = DDERR_GENERIC;
						DdDestorySurfaceData.DestroySurface = pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface;
																				
						if ( (pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) != DDHAL_SURFCB32_DESTROYSURFACE )
						{					
							if ( pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface != 0 )
							{																						
								pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface( &DdDestorySurfaceData ) ;
							}
						}					

						/* 
						 * Setup our new surface to create it 
						 */
															
						tmp_DdSurfLcl = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;
										
						memset(&DdSurfaceDesc,0, sizeof(DDSURFACEDESC));															
						DdSurfaceDesc.dwSize =  sizeof(DDSURFACEDESC);					
						DdSurfaceDesc.dwFlags = DDSD_CAPS;
						DdSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VISIBLE;
					
						DdCreateSurfaceData.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;
						DdCreateSurfaceData.lpDDSurfaceDesc = &DdSurfaceDesc;
						DdCreateSurfaceData.lplpSList = &tmp_DdSurfLcl;				
						DdCreateSurfaceData.dwSCnt = 1;
						DdCreateSurfaceData.ddRVal = DDERR_GENERIC;
						DdCreateSurfaceData.CreateSurface = pDciSurface_int->DciSurface_lcl.DDCallbacks.CreateSurface;
																																
						if ( ( pDciSurface_int->DciSurface_lcl.DDCallbacks.CreateSurface != NULL) &&
							((pDciSurface_int->DciSurface_lcl.DDCallbacks.dwFlags & DDHAL_CB32_CREATESURFACE) == DDHAL_CB32_CREATESURFACE))
						{																									
							if ( ( pDciSurface_int->DciSurface_lcl.DDCallbacks.CreateSurface( &DdCreateSurfaceData ) == 1 ) &&
								( DdCreateSurfaceData.ddRVal == DD_OK ) ) 
							{													
								/* 
								 * RestVisrgn 
								 */
								if ( DdResetVisrgn( &pDciSurface_int->DciSurface_lcl.SurfaceLocal, (HWND)-1 ) != 0)
								{
									/* The surface was lost, so we need restart DCIBeginAccess,
									 * we can retstart it in two ways
									 * 1. call on DCIBeginAccess each time 
									 * 2. use a goto. 
									 *
									 * Why I decide not to call it again,
									 * is it will use more stack space and we do not known 
									 * how many times it will restart it self, and 
									 * it will cost cpu time and stack memory. 
									 * 
									 * The goto did seam best slovtions to this issue, and
									 * it will not cost us extra stack space or cpu time,
									 * and we do not need setup DdLockData again, and do other 
									 * check as well.
									 */
									goto ReStart;
								}						
							}
						}															
					}
				}

				/* Get HalInfo from DirectX hardware accalation  fail or something else  so we lost the surface */
				pDciSurface_int->DciSurface_lcl.LostSurface = TRUE;
				
				retValue = DCI_FAIL_INVALIDSURFACE;
					
				/* 
				 * Setup Destorysurface the lost surface so we do not lost any hDC or hDD  handles and memory leaks 
			     * and destory it and free all DirectX rescures 
				 */
				DdDestorySurfaceData.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;					
				DdDestorySurfaceData.lpDDSurface = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;					
				DdDestorySurfaceData.ddRVal = DDERR_GENERIC;
				DdDestorySurfaceData.DestroySurface = pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface;
																				
				if ( (pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) != DDHAL_SURFCB32_DESTROYSURFACE )
				{					
					if ( pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface != 0 )
					{																						
						if ( pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.DestroySurface( &DdDestorySurfaceData ) == DDHAL_DRIVER_HANDLED )
						{						
							if ( DdDestorySurfaceData.ddRVal == DD_OK )
							{
								DdDeleteDirectDrawObject(&pDciSurface_int->DciSurface_lcl.DirectDrawGlobal);
							}
						}
					}					
				}

			}

		}									
	}
	
	return retValue;
}

void
WINAPI
DCIEndAccess(LPDCISURFACEINFO pdci)
{
	DDHAL_UNLOCKDATA DdUnlockData;
	LPDCISURFACE_INT pDciSurface_int;
	
	pDciSurface_int = (LPDCISURFACE_INT) (((DWORD) pdci) - sizeof(DCISURFACE_LCL)) ;
	
	if ( pDciSurface_int->DciSurface_lcl.LostSurface == 0 )
	{				
		DdUnlockData.lpDD = &pDciSurface_int->DciSurface_lcl.DirectDrawGlobal;
		DdUnlockData.lpDDSurface = &pDciSurface_int->DciSurface_lcl.SurfaceLocal;
		
		EnterCriticalSection(&gcsWinWatchLock);
	
		pDciSurface_int->DciSurface_lcl.DDSurfaceCallbacks.Unlock(&DdUnlockData);		
	
		LeaveCriticalSection(&gcsWinWatchLock);
	}
	
	pDciSurface_int->DciSurfaceInfo.wSelSurface = 0;
	pDciSurface_int->DciSurfaceInfo.dwOffSurface = 0;
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





