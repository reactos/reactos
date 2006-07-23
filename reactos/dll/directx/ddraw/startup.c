/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"


HRESULT WINAPI 
StartDirectDraw(LPDIRECTDRAW* iface)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DWORD hal_ret;
    DWORD hel_ret;
    DEVMODE devmode;
    HBITMAP hbmp;
    const UINT bmiSize = sizeof(BITMAPINFOHEADER) + 0x10;
    UCHAR *pbmiData;
    BITMAPINFO *pbmi;    
    DWORD *pMasks;
    
    DX_WINDBG_trace();
	  
	RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));
	
	/* cObsolete is undoc in msdn it being use in CreateDCA */
	RtlCopyMemory(&This->mDDrawGlobal.cObsolete,&"DISPLAY",7);
	RtlCopyMemory(&This->mDDrawGlobal.cDriverName,&"DISPLAY",7);
	
    /* Same for HEL and HAL */
    This->mcModeInfos = 1;
    This->mpModeInfos = (DDHALMODEINFO*) DxHeapMemAlloc(This->mcModeInfos * sizeof(DDHALMODEINFO));  
   
    if (This->mpModeInfos == NULL)
    {
	   DX_STUB_str("DD_FALSE");
       return DD_FALSE;
    }

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    This->mpModeInfos[0].dwWidth      = devmode.dmPelsWidth;
    This->mpModeInfos[0].dwHeight     = devmode.dmPelsHeight;
    This->mpModeInfos[0].dwBPP        = devmode.dmBitsPerPel;
    This->mpModeInfos[0].lPitch       = (devmode.dmPelsWidth*devmode.dmBitsPerPel)/8;
    This->mpModeInfos[0].wRefreshRate = (WORD)devmode.dmDisplayFrequency;
   
    This->hdc = CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

    if (This->hdc == NULL)
    {
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY ;
    }

    hbmp = CreateCompatibleBitmap(This->hdc, 1, 1);  
    if (hbmp==NULL)
    {
       DxHeapMemFree(This->mpModeInfos);
       DeleteDC(This->hdc);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }
  
    pbmiData = (UCHAR *) DxHeapMemAlloc(bmiSize);
    pbmi = (BITMAPINFO*)pbmiData;

    if (pbmiData==NULL)
    {
       DxHeapMemFree(This->mpModeInfos);       
       DeleteDC(This->hdc);
       DeleteObject(hbmp);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = (WORD)devmode.dmBitsPerPel;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    pbmi->bmiHeader.biWidth = 1;
    pbmi->bmiHeader.biHeight = 1;

    GetDIBits(This->hdc, hbmp, 0, 0, NULL, pbmi, 0);
    DeleteObject(hbmp);

    pMasks = (DWORD*)(pbmiData + sizeof(BITMAPINFOHEADER));
    This->mpModeInfos[0].dwRBitMask = pMasks[0];
    This->mpModeInfos[0].dwGBitMask = pMasks[1];
    This->mpModeInfos[0].dwBBitMask = pMasks[2];
    This->mpModeInfos[0].dwAlphaBitMask = pMasks[3];

	DxHeapMemFree(pbmiData);

    /* Startup HEL and HAL */
    RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));
    RtlZeroMemory(&This->mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&This->mCallbacks, sizeof(DDHAL_CALLBACKS));

    This->mDDrawLocal.lpDDCB = &This->mCallbacks;
    This->mDDrawLocal.lpGbl = &This->mDDrawGlobal;
    This->mDDrawLocal.dwProcessId = GetCurrentProcessId();

    This->mDDrawGlobal.lpDDCBtmp = &This->mCallbacks;
    This->mDDrawGlobal.lpExclusiveOwner = &This->mDDrawLocal;

    hal_ret = Hal_DirectDraw_Initialize ((LPDIRECTDRAW7)iface);    
    hel_ret = Hel_DirectDraw_Initialize ((LPDIRECTDRAW7)iface); 
    if ((hal_ret!=DD_OK) &&  (hel_ret!=DD_OK))
    {
		DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT");
        return DDERR_NODIRECTDRAWSUPPORT; 
    }

    /* 
       Try figout which api we shall use, first we try see if HAL exits 
       if it does not we select HEL instead 
    */ 
            
    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE) 
    {
        This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;  
    }
    else
    {
        This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HELDD.CanCreateSurface;
    }
        
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;
        
    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CREATESURFACE) 
    {
        This->mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
    }
    else
    {
        This->mDdCreateSurface.CreateSurface = This->mCallbacks.HELDD.CreateSurface;
    }
    
    /* Setup calback struct so we do not need refill same info again */
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;    
    This->mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;  
                  
    return DD_OK;
}

HRESULT 
WINAPI 
Create_DirectDraw (LPGUID pGUID, 
				   LPDIRECTDRAW* pIface, 
				   REFIID id, 
				   BOOL ex)
{   
    IDirectDrawImpl* This = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawImpl));

	DX_WINDBG_trace();

	if (This == NULL) 
		return E_OUTOFMEMORY;

	ZeroMemory(This,sizeof(IDirectDrawImpl));

	This->lpVtbl = &DirectDraw7_Vtable;
	This->lpVtbl_v1 = &DDRAW_IDirectDraw_VTable;
	This->lpVtbl_v2 = &DDRAW_IDirectDraw2_VTable;
	This->lpVtbl_v4 = &DDRAW_IDirectDraw4_VTable;
	
	*pIface = (LPDIRECTDRAW)This;

	if(This->lpVtbl->QueryInterface ((LPDIRECTDRAW7)This, id, (void**)&pIface) != S_OK)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (StartDirectDraw((LPDIRECTDRAW*)This) == DD_OK);
    {
		return This->lpVtbl->Initialize ((LPDIRECTDRAW7)This, pGUID);
	}

	return DDERR_INVALIDPARAMS;
}

