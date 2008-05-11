

#include "../rosdraw.h"



HRESULT WINAPI
Thunk_DDrawSurface3_QueryInterface(LPDIRECTDRAWSURFACE3 iface, REFIID iid,
				       LPVOID *ppObj)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE7) iface,  iid, ppObj);
}

ULONG WINAPI
Thunk_DDrawSurface3_AddRef(LPDIRECTDRAWSURFACE3 iface)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_AddRef((LPDIRECTDRAWSURFACE7) iface);
}

ULONG WINAPI
Thunk_DDrawSurface3_Release(LPDIRECTDRAWSURFACE3 iface)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_Release((LPDIRECTDRAWSURFACE7) iface);
}

HRESULT WINAPI
Thunk_DDrawSurface3_AddAttachedSurface(LPDIRECTDRAWSURFACE3 iface, LPDIRECTDRAWSURFACE3 pAttach)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_AddAttachedSurface((LPDIRECTDRAWSURFACE7) iface, (LPDIRECTDRAWSURFACE7) pAttach);
}

HRESULT WINAPI
Thunk_DDrawSurface3_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE3 iface, LPRECT pRect)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_AddOverlayDirtyRect((LPDIRECTDRAWSURFACE7) iface, pRect);
}

HRESULT WINAPI
Thunk_DDrawSurface3_Blt(LPDIRECTDRAWSURFACE3 iface, LPRECT rdst,
			    LPDIRECTDRAWSURFACE3 src, LPRECT rsrc,
			    DWORD dwFlags, LPDDBLTFX pFX)
{
    DX_WINDBG_trace();

	return Main_DDrawSurface_Blt((LPDIRECTDRAWSURFACE7) iface, rdst,(LPDIRECTDRAWSURFACE7) src, rsrc, dwFlags, pFX);
}

HRESULT WINAPI
Thunk_DDrawSurface3_BltBatch(LPDIRECTDRAWSURFACE3 iface, LPDDBLTBATCH pBatch, DWORD dwCount, DWORD dwFlags)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_BltBatch((LPDIRECTDRAWSURFACE7) iface, pBatch, dwCount, dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_BltFast(LPDIRECTDRAWSURFACE3 iface, DWORD dstx, DWORD dsty,
							LPDIRECTDRAWSURFACE3 src, LPRECT rsrc, DWORD dwTrans)
{
	DX_WINDBG_trace();

   	return Main_DDrawSurface_BltFast((LPDIRECTDRAWSURFACE7) iface, dstx, dsty,
		                             (LPDIRECTDRAWSURFACE7)src, rsrc, dwTrans);
}

HRESULT WINAPI
Thunk_DDrawSurface3_DeleteAttachedSurface(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags, LPDIRECTDRAWSURFACE3 pAttached)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_DeleteAttachedSurface((LPDIRECTDRAWSURFACE7) iface, dwFlags,
		                                           (LPDIRECTDRAWSURFACE7) pAttached);
}



HRESULT WINAPI
Thunk_DDrawSurface3_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE3 iface,
					     LPVOID context,
					     LPDDENUMSURFACESCALLBACK cb)
{
    DX_WINDBG_trace();

    return Main_DDrawSurface_EnumAttachedSurfaces((LPDIRECTDRAWSURFACE7) iface,
		                                           context, (LPDDENUMSURFACESCALLBACK7)cb);
}

HRESULT WINAPI
Thunk_DDrawSurface3_EnumOverlayZOrders(LPDIRECTDRAWSURFACE3 iface,
					   DWORD dwFlags, LPVOID context,
					   LPDDENUMSURFACESCALLBACK cb)
{
    DX_WINDBG_trace();

	return Main_DDrawSurface_EnumOverlayZOrders( (LPDIRECTDRAWSURFACE7) iface, dwFlags, context,
		                                         (LPDDENUMSURFACESCALLBACK7) cb);
}

HRESULT WINAPI
Thunk_DDrawSurface3_Flip(LPDIRECTDRAWSURFACE3 iface,
			     LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_Flip( (LPDIRECTDRAWSURFACE7)iface, (LPDIRECTDRAWSURFACE7) lpDDSurfaceTargetOverride,
		                            dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetAttachedSurface(LPDIRECTDRAWSURFACE3 iface,
					   LPDDSCAPS pCaps,
					   LPDIRECTDRAWSURFACE3* ppAttached)
{
    DDSCAPS2 caps;
	HRESULT hr = DDERR_GENERIC;

	DX_WINDBG_trace();

	ZeroMemory(&caps,sizeof(DDSCAPS2));

	if (pCaps != NULL)
	{
        caps.dwCaps  = pCaps->dwCaps;

	    hr = Main_DDrawSurface_GetAttachedSurface( (LPDIRECTDRAWSURFACE7) iface,
		                                       &caps, (LPDIRECTDRAWSURFACE7 *) ppAttached);
	    pCaps->dwCaps = caps.dwCaps;
	}
	else
	{
		hr = Main_DDrawSurface_GetAttachedSurface( (LPDIRECTDRAWSURFACE7) iface,
		                                       NULL, (LPDIRECTDRAWSURFACE7 *) ppAttached);
	}

    return hr;
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetBltStatus(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

	return Main_DDrawSurface_GetBltStatus((LPDIRECTDRAWSURFACE7) iface, dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetCaps(LPDIRECTDRAWSURFACE3 iface, LPDDSCAPS pCaps)
{
    DDSCAPS2 caps;
    HRESULT hr = DDERR_GENERIC;

	DX_WINDBG_trace();

	ZeroMemory(&caps,sizeof(DDSCAPS2));

	if (pCaps != NULL)
	{
        hr = Main_DDrawSurface_GetCaps((LPDIRECTDRAWSURFACE7) iface, &caps);
        pCaps->dwCaps = caps.dwCaps;
	}
	else
	{
		hr = Main_DDrawSurface_GetCaps((LPDIRECTDRAWSURFACE7) iface, NULL);
	}

    return hr;
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetClipper(LPDIRECTDRAWSURFACE3 iface,
				   LPDIRECTDRAWCLIPPER* ppClipper)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_GetClipper((LPDIRECTDRAWSURFACE7) iface, ppClipper);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetColorKey(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags,
				    LPDDCOLORKEY pCKey)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_GetColorKey( (LPDIRECTDRAWSURFACE7) iface, dwFlags, pCKey);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetDC(LPDIRECTDRAWSURFACE3 iface, HDC* phDC)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_GetDC((LPDIRECTDRAWSURFACE7) iface, phDC);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetFlipStatus(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_GetFlipStatus((LPDIRECTDRAWSURFACE7) iface, dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetOverlayPosition(LPDIRECTDRAWSURFACE3 iface, LPLONG pX, LPLONG pY)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_GetOverlayPosition((LPDIRECTDRAWSURFACE7) iface,  pX, pY);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetPalette(LPDIRECTDRAWSURFACE3 iface,
				   LPDIRECTDRAWPALETTE* ppPalette)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_GetPalette((LPDIRECTDRAWSURFACE7) iface, ppPalette);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetPixelFormat(LPDIRECTDRAWSURFACE3 iface,
				       LPDDPIXELFORMAT pDDPixelFormat)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_GetPixelFormat((LPDIRECTDRAWSURFACE7) iface, pDDPixelFormat);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetSurfaceDesc(LPDIRECTDRAWSURFACE3 iface,
				       LPDDSURFACEDESC pDDSD)
{
    HRESULT retValue = DDERR_GENERIC;

    DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue = Main_DDrawSurface_GetSurfaceDesc((LPDIRECTDRAWSURFACE7) iface, &pDDSD2);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue = Main_DDrawSurface_GetSurfaceDesc((LPDIRECTDRAWSURFACE7) iface, NULL);
	}

    return retValue;
}

HRESULT WINAPI
Thunk_DDrawSurface3_Initialize(LPDIRECTDRAWSURFACE3 iface, LPDIRECTDRAW pDD,
				   LPDDSURFACEDESC pDDSD)
{
	HRESULT retValue = DDERR_GENERIC;
	DDSURFACEDESC2 pDDSD2;

	DX_WINDBG_trace();

	ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue = Main_DDrawSurface_Initialize((LPDIRECTDRAWSURFACE7) iface, pDD, &pDDSD2);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue = Main_DDrawSurface_Initialize ((LPDIRECTDRAWSURFACE7) iface, pDD, NULL);

	}

    return retValue;
}

HRESULT WINAPI
Thunk_DDrawSurface3_IsLost(LPDIRECTDRAWSURFACE3 iface)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_IsLost((LPDIRECTDRAWSURFACE7) iface);
}

HRESULT WINAPI
Thunk_DDrawSurface3_Lock(LPDIRECTDRAWSURFACE3 iface, LPRECT pRect,
			     LPDDSURFACEDESC pDDSD, DWORD dwFlags, HANDLE events)
{

	HRESULT retValue = DDERR_GENERIC;
	DDSURFACEDESC2 pDDSD2;

	DX_WINDBG_trace();

	ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue = Main_DDrawSurface_Lock ( (LPDIRECTDRAWSURFACE7) iface, pRect, &pDDSD2, dwFlags, events);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue = Main_DDrawSurface_Lock ( (LPDIRECTDRAWSURFACE7) iface, pRect, NULL, dwFlags, events);
	}

    return retValue;

}

HRESULT WINAPI
Thunk_DDrawSurface3_ReleaseDC(LPDIRECTDRAWSURFACE3 iface, HDC hDC)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_ReleaseDC((LPDIRECTDRAWSURFACE7) iface, hDC);
}

HRESULT WINAPI
Thunk_DDrawSurface3_Restore(LPDIRECTDRAWSURFACE3 iface)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_Restore((LPDIRECTDRAWSURFACE7) iface);
}

HRESULT WINAPI
Thunk_DDrawSurface3_SetClipper(LPDIRECTDRAWSURFACE3 iface, LPDIRECTDRAWCLIPPER pDDClipper)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_SetClipper ((LPDIRECTDRAWSURFACE7) iface, pDDClipper);
}

HRESULT WINAPI
Thunk_DDrawSurface3_SetColorKey(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags,
				    LPDDCOLORKEY pCKey)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_SetColorKey ((LPDIRECTDRAWSURFACE7) iface, dwFlags, pCKey);
}

HRESULT WINAPI
Thunk_DDrawSurface3_SetOverlayPosition(LPDIRECTDRAWSURFACE3 iface, LONG X,LONG Y)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_SetOverlayPosition ((LPDIRECTDRAWSURFACE7) iface, X, Y);
}

HRESULT WINAPI
Thunk_DDrawSurface3_SetPalette(LPDIRECTDRAWSURFACE3 iface,
				   LPDIRECTDRAWPALETTE pPalette)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_SetPalette ((LPDIRECTDRAWSURFACE7) iface, pPalette);
}

/*
HRESULT WINAPI
Thunk_DDrawSurface3_Unlock(LPDIRECTDRAWSURFACE3 iface, LPRECT pRect)
{
	DX_WINDBG_trace();

	return Main_DDrawSurface_Unlock ((LPDIRECTDRAWSURFACE7) iface, pRect);
} */

HRESULT WINAPI
Thunk_DDrawSurface3_Unlock(LPVOID iface, LPVOID pRect)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Thunk_DDrawSurface3_UpdateOverlay(LPDIRECTDRAWSURFACE3 iface, LPRECT pSrcRect, LPDIRECTDRAWSURFACE3 pDstSurface,
				                  LPRECT pDstRect, DWORD dwFlags, LPDDOVERLAYFX pFX)
{

	DX_WINDBG_trace();

	return Main_DDrawSurface_UpdateOverlay ( (LPDIRECTDRAWSURFACE7) iface, pSrcRect,
		                                     (LPDIRECTDRAWSURFACE7) pDstSurface, pDstRect, dwFlags, pFX);
}

HRESULT WINAPI
Thunk_DDrawSurface3_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_UpdateOverlayDisplay((LPDIRECTDRAWSURFACE7) iface, dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE3 iface,
					    DWORD dwFlags,
					    LPDIRECTDRAWSURFACE3 pSurfReference)
{
	DX_WINDBG_trace();
    return Main_DDrawSurface_UpdateOverlayZOrder((LPDIRECTDRAWSURFACE7) iface, dwFlags,
						                         (LPDIRECTDRAWSURFACE7) pSurfReference);
}

HRESULT WINAPI
Thunk_DDrawSurface3_GetDDInterface(LPDIRECTDRAWSURFACE3 iface, LPVOID* ppDD)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_GetDDInterface((LPDIRECTDRAWSURFACE7) iface, ppDD);
}

HRESULT WINAPI
Thunk_DDrawSurface3_PageLock(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_PageLock((LPDIRECTDRAWSURFACE7) iface, dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_PageUnlock(LPDIRECTDRAWSURFACE3 iface, DWORD dwFlags)
{
	DX_WINDBG_trace();

    return Main_DDrawSurface_PageUnlock((LPDIRECTDRAWSURFACE7) iface, dwFlags);
}

HRESULT WINAPI
Thunk_DDrawSurface3_SetSurfaceDesc(LPDIRECTDRAWSURFACE3 iface,
				       LPDDSURFACEDESC pDDSD, DWORD dwFlags)
{
	HRESULT retValue = DDERR_GENERIC;

    DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue = Main_DDrawSurface_SetSurfaceDesc((LPDIRECTDRAWSURFACE7) iface, &pDDSD2, dwFlags);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue = Main_DDrawSurface_SetSurfaceDesc((LPDIRECTDRAWSURFACE7) iface, NULL, dwFlags);
	}


    return retValue;
}

IDirectDrawSurface3Vtbl DirectDrawSurface3_VTable =
{
    Thunk_DDrawSurface3_QueryInterface,
    Thunk_DDrawSurface3_AddRef,
    Thunk_DDrawSurface3_Release,
    Thunk_DDrawSurface3_AddAttachedSurface,
    Thunk_DDrawSurface3_AddOverlayDirtyRect,
    Thunk_DDrawSurface3_Blt,
    Thunk_DDrawSurface3_BltBatch,
    Thunk_DDrawSurface3_BltFast,
    Thunk_DDrawSurface3_DeleteAttachedSurface,
    Thunk_DDrawSurface3_EnumAttachedSurfaces,
    Thunk_DDrawSurface3_EnumOverlayZOrders,
    Thunk_DDrawSurface3_Flip,
    Thunk_DDrawSurface3_GetAttachedSurface,
    Thunk_DDrawSurface3_GetBltStatus,
    Thunk_DDrawSurface3_GetCaps,
    Thunk_DDrawSurface3_GetClipper,
    Thunk_DDrawSurface3_GetColorKey,
    Thunk_DDrawSurface3_GetDC,
    Thunk_DDrawSurface3_GetFlipStatus,
    Thunk_DDrawSurface3_GetOverlayPosition,
    Thunk_DDrawSurface3_GetPalette,
    Thunk_DDrawSurface3_GetPixelFormat,
    Thunk_DDrawSurface3_GetSurfaceDesc,
    Thunk_DDrawSurface3_Initialize,
    Thunk_DDrawSurface3_IsLost,
    Thunk_DDrawSurface3_Lock,
    Thunk_DDrawSurface3_ReleaseDC,
    Thunk_DDrawSurface3_Restore,
    Thunk_DDrawSurface3_SetClipper,
    Thunk_DDrawSurface3_SetColorKey,
    Thunk_DDrawSurface3_SetOverlayPosition,
    Thunk_DDrawSurface3_SetPalette,
    Thunk_DDrawSurface3_Unlock,
    Thunk_DDrawSurface3_UpdateOverlay,
    Thunk_DDrawSurface3_UpdateOverlayDisplay,
    Thunk_DDrawSurface3_UpdateOverlayZOrder,
    Thunk_DDrawSurface3_GetDDInterface,
    Thunk_DDrawSurface3_PageLock,
    Thunk_DDrawSurface3_PageUnlock,
    Thunk_DDrawSurface3_SetSurfaceDesc
};
