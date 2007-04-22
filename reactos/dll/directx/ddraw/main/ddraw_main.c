/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/ddraw.c
 * PURPOSE:              IDirectDraw7 Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */


#include "../rosdraw.h"

HRESULT 
WINAPI 
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface, 
                                REFIID id, 
                                LPVOID *obj) 
{   
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();
    
    /* fixme 
       the D3D object cab be optain from here 
       Direct3D7 
    */
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        /* DirectDraw7 Vtable */
        This->lpVtbl = &DirectDraw7_Vtable;
        *obj = &This->lpVtbl;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    Main_DirectDraw_AddRef(iface);
    return S_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
ULONG
WINAPI 
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface) 
{    
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;

        if (This->lpLcl->lpGbl != NULL)
        {
            This->lpLcl->lpGbl->dwRefCnt++;
        }
    }
    return This->dwIntRefCnt;
}


ULONG 
WINAPI 
Main_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{    
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

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
            return 0;
        }
    }
    return This->dwIntRefCnt;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface) 
{
    /* MSDN say not implement but my question what does it return then */
    DX_WINDBG_trace();
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, 
                              DWORD dwFlags, 
                              LPDIRECTDRAWCLIPPER *ppClipper, 
                              IUnknown *pUnkOuter)
{
  DX_WINDBG_trace();

   DX_STUB;  
}

HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
    DX_WINDBG_trace();

    DX_STUB;
}


/*
 * stub
 * Status not done
 */
HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                                            LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter) 
{
    
  DX_WINDBG_trace();

   DX_STUB;  
   
}


/*
 * stub
 * Status not done
 */
HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
                 LPDIRECTDRAWSURFACE7* dst) 
{
    DX_WINDBG_trace();
    DX_STUB;    
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) 
{

   DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * stub
 * Status not done
 */
HRESULT WINAPI 
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
                 LPDDENUMSURFACESCALLBACK7 callback) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
  DX_WINDBG_trace();

   DX_STUB;  
}
 
/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
            LPDDCAPS pHELCaps) 
{

  DX_WINDBG_trace();

   DX_STUB;  
}


/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) 
{       
  //LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
                                             LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{      
  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{    		
  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL lpbIsInVB)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{          
  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
  DX_WINDBG_trace();

   DX_STUB;  
   return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
                                                                DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
                                                   HANDLE h)
{

  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD total, LPDWORD free)                                               
{
  DX_WINDBG_trace();

   DX_STUB;  
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
                                                LPDIRECTDRAWSURFACE7 *lpDDS)
{  
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
                   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{    
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
                  DWORD dwNumModes, DWORD dwFlags)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{  
    DX_WINDBG_trace();
    DX_STUB;
}


IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
