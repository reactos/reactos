

#include "../rosdraw.h"



HRESULT WINAPI
ThunkDirectDraw_QueryInterface(LPDIRECTDRAW iface, REFIID iid, LPVOID *ppObj)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_QueryInterface ((LPDIRECTDRAW7) iface, iid, ppObj);
}

HRESULT WINAPI
ThunkDirectDraw2_QueryInterface(LPDIRECTDRAW2 iface, REFIID iid, LPVOID *obj)
{
	DX_WINDBG_trace();

	return Main_DirectDraw_QueryInterface ((LPDIRECTDRAW7) iface, iid, obj);
}


HRESULT WINAPI
ThunkDirectDraw4_QueryInterface(LPDIRECTDRAW4 iface, REFIID iid, LPVOID *ppObj)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_QueryInterface ((LPDIRECTDRAW7) iface, iid, ppObj);
}

ULONG WINAPI
ThunkDirectDraw_AddRef(LPDIRECTDRAW iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_AddRef((LPDIRECTDRAW7) iface);
}

ULONG WINAPI
ThunkDirectDraw2_AddRef(LPDIRECTDRAW2 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_AddRef((LPDIRECTDRAW7) iface);
}

ULONG WINAPI
ThunkDirectDraw4_AddRef(LPDIRECTDRAW4 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_AddRef((LPDIRECTDRAW7) iface);
}

ULONG WINAPI
ThunkDirectDraw_Release(LPDIRECTDRAW iface)
{
	DX_WINDBG_trace();

	return Main_DirectDraw_Release ((LPDIRECTDRAW7) iface);
}

ULONG WINAPI
ThunkDirectDraw2_Release(LPDIRECTDRAW2 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_Release ((LPDIRECTDRAW7) iface);
}

ULONG WINAPI
ThunkDirectDraw4_Release(LPDIRECTDRAW4 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_Release ((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw_Compact(LPDIRECTDRAW iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_Compact((LPDIRECTDRAW7) iface) ;
}

HRESULT WINAPI
ThunkDirectDraw2_Compact(LPDIRECTDRAW2 iface)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_Compact((LPDIRECTDRAW7) iface) ;
}

HRESULT WINAPI
ThunkDirectDraw4_Compact(LPDIRECTDRAW4 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_Compact((LPDIRECTDRAW7) iface) ;
}

HRESULT WINAPI
ThunkDirectDraw_CreateClipper(LPDIRECTDRAW iface,DWORD dwFlags,LPDIRECTDRAWCLIPPER *ppClipper,IUnknown *pUnkOuter)
{
	DX_WINDBG_trace();

	return Main_DirectDraw_CreateClipper( (LPDIRECTDRAW7) iface, dwFlags, ppClipper, pUnkOuter);
}

HRESULT WINAPI
ThunkDirectDraw2_CreateClipper(LPDIRECTDRAW2 iface,DWORD dwFlags,LPDIRECTDRAWCLIPPER *ppClipper,IUnknown *pUnkOuter)
{
    DX_WINDBG_trace();

	return Main_DirectDraw_CreateClipper( (LPDIRECTDRAW7) iface, dwFlags, ppClipper, pUnkOuter);
}

HRESULT WINAPI
ThunkDirectDraw4_CreateClipper(LPDIRECTDRAW4 iface,DWORD dwFlags,LPDIRECTDRAWCLIPPER *ppClipper,IUnknown *pUnkOuter)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_CreateClipper( (LPDIRECTDRAW7) iface, dwFlags, ppClipper, pUnkOuter);
}

HRESULT WINAPI
ThunkDirectDraw_CreatePalette(LPDIRECTDRAW iface, DWORD dwFlags,
			      LPPALETTEENTRY pEntries,
			      LPDIRECTDRAWPALETTE *ppPalette,
			      IUnknown *pUnkOuter)
{
    DX_WINDBG_trace();

	return Main_DirectDraw_CreatePalette( (LPDIRECTDRAW7) iface,  dwFlags, pEntries,  ppPalette, pUnkOuter);
}

HRESULT WINAPI
ThunkDirectDraw2_CreatePalette(LPDIRECTDRAW2 iface, DWORD dwFlags,
			       LPPALETTEENTRY pEntries,
			       LPDIRECTDRAWPALETTE *ppPalette,
			       IUnknown *pUnkOuter)
{
    DX_WINDBG_trace();

	return Main_DirectDraw_CreatePalette( (LPDIRECTDRAW7) iface,  dwFlags, pEntries,  ppPalette, pUnkOuter);
}

HRESULT WINAPI
ThunkDirectDraw4_CreatePalette(LPDIRECTDRAW4 iface, DWORD dwFlags, LPPALETTEENTRY pEntries,
							   LPDIRECTDRAWPALETTE *ppPalette, IUnknown *pUnkOuter)
{
    DX_WINDBG_trace();

	return Main_DirectDraw_CreatePalette( (LPDIRECTDRAW7) iface,  dwFlags, pEntries,  ppPalette, pUnkOuter);
}

HRESULT WINAPI
ThunkDirectDraw_CreateSurface(LPDIRECTDRAW iface, LPDDSURFACEDESC pDDSD,
			      LPDIRECTDRAWSURFACE *ppSurf,
			      IUnknown *pUnkOuter)
{
	HRESULT retValue = DDERR_GENERIC;
	LPDDRAWI_DDRAWSURFACE_INT That;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue =  Main_DirectDraw_CreateSurface ((LPDIRECTDRAW7) iface, &pDDSD2,
		                                          (LPDIRECTDRAWSURFACE7 *) ppSurf, pUnkOuter);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =  Main_DirectDraw_CreateSurface ((LPDIRECTDRAW7) iface, NULL,
			                                       (LPDIRECTDRAWSURFACE7 *) ppSurf, pUnkOuter);
	}

	That  = (LPDDRAWI_DDRAWSURFACE_INT) *ppSurf;
	That->lpVtbl = &DirectDrawSurface3_VTable;

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw2_CreateSurface(LPDIRECTDRAW2 iface, LPDDSURFACEDESC pDDSD,
			                   LPDIRECTDRAWSURFACE *ppSurface, IUnknown *pUnkOuter)
{
    HRESULT retValue = DDERR_GENERIC;
	LPDDRAWI_DDRAWSURFACE_INT That;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue =  Main_DirectDraw_CreateSurface ((LPDIRECTDRAW7) iface, &pDDSD2,
		                                          (LPDIRECTDRAWSURFACE7 *) ppSurface, pUnkOuter);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =  Main_DirectDraw_CreateSurface ((LPDIRECTDRAW7) iface, NULL,
			                                       (LPDIRECTDRAWSURFACE7 *) ppSurface, pUnkOuter);
	}

	That  = (LPDDRAWI_DDRAWSURFACE_INT) *ppSurface;
	That->lpVtbl = &DirectDrawSurface3_VTable;

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw4_CreateSurface(LPDIRECTDRAW4 iface, LPDDSURFACEDESC2 pDDSD2,
			                   LPDIRECTDRAWSURFACE4 *ppSurface, IUnknown *pUnkOuter)
{
	HRESULT retValue;
	LPDDRAWI_DDRAWSURFACE_INT That;

	DX_WINDBG_trace();


	retValue = Main_DirectDraw_CreateSurface ((LPDIRECTDRAW7) iface, pDDSD2,
		                                   (LPDIRECTDRAWSURFACE7 *) ppSurface, pUnkOuter);

	That  = (LPDDRAWI_DDRAWSURFACE_INT) *ppSurface;
	That->lpVtbl = &DirectDrawSurface3_VTable;

	return retValue;
}

HRESULT WINAPI
ThunkDirectDraw_DuplicateSurface(LPDIRECTDRAW iface, LPDIRECTDRAWSURFACE src, LPDIRECTDRAWSURFACE *dst)
{
	DX_WINDBG_trace();

    return  Main_DirectDraw_DuplicateSurface((LPDIRECTDRAW7) iface, (LPDIRECTDRAWSURFACE7) src,
                                             (LPDIRECTDRAWSURFACE7*) dst);
}

HRESULT WINAPI
ThunkDirectDraw2_DuplicateSurface(LPDIRECTDRAW2 iface, LPDIRECTDRAWSURFACE src, LPDIRECTDRAWSURFACE *dst)
{
    DX_WINDBG_trace();

    return  Main_DirectDraw_DuplicateSurface((LPDIRECTDRAW7) iface, (LPDIRECTDRAWSURFACE7) src,
                                             (LPDIRECTDRAWSURFACE7*) dst);
}

HRESULT WINAPI
ThunkDirectDraw4_DuplicateSurface(LPDIRECTDRAW4 iface, LPDIRECTDRAWSURFACE4 src, LPDIRECTDRAWSURFACE4 *dst)
{
    DX_WINDBG_trace();

    return  Main_DirectDraw_DuplicateSurface((LPDIRECTDRAW7) iface, (LPDIRECTDRAWSURFACE7) src,
                                             (LPDIRECTDRAWSURFACE7*) dst);
}

HRESULT WINAPI
ThunkDirectDraw_EnumDisplayModes(LPDIRECTDRAW iface, DWORD dwFlags,
				 LPDDSURFACEDESC pDDSD, LPVOID context,
				 LPDDENUMMODESCALLBACK cb)
{
    HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue =  Main_DirectDraw_EnumDisplayModes((LPDIRECTDRAW7) iface, dwFlags, &pDDSD2,
		                                             context, (LPDDENUMMODESCALLBACK2)cb);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =   Main_DirectDraw_EnumDisplayModes((LPDIRECTDRAW7) iface, dwFlags, NULL,
			                                           context, (LPDDENUMMODESCALLBACK2)cb);
	}

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw2_EnumDisplayModes(LPDIRECTDRAW2 iface, DWORD dwFlags,
				  LPDDSURFACEDESC pDDSD, LPVOID context,
				  LPDDENUMMODESCALLBACK cb)
{
    HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue =  Main_DirectDraw_EnumDisplayModes((LPDIRECTDRAW7) iface, dwFlags, &pDDSD2,
		                                             context, (LPDDENUMMODESCALLBACK2)cb);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =   Main_DirectDraw_EnumDisplayModes((LPDIRECTDRAW7) iface, dwFlags, NULL,
			                                           context, (LPDDENUMMODESCALLBACK2)cb);
	}

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw4_EnumDisplayModes(LPDIRECTDRAW4 iface, DWORD dwFlags,
				  LPDDSURFACEDESC2 pDDSD, LPVOID context,
				  LPDDENUMMODESCALLBACK2 cb)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_EnumDisplayModes((LPDIRECTDRAW7) iface, dwFlags, pDDSD, context, cb);
}





HRESULT WINAPI
ThunkDirectDraw_EnumSurfaces(LPDIRECTDRAW iface, DWORD dwFlags,
			     LPDDSURFACEDESC pDDSD, LPVOID context,
			     LPDDENUMSURFACESCALLBACK cb)
{
    HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue =  Main_DirectDraw_EnumSurfaces((LPDIRECTDRAW7) iface, dwFlags, &pDDSD2,
		                                         context, (LPDDENUMSURFACESCALLBACK7)cb);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =   Main_DirectDraw_EnumSurfaces((LPDIRECTDRAW7) iface, dwFlags, NULL,
			                                       context, (LPDDENUMSURFACESCALLBACK7)cb);
	}

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw2_EnumSurfaces(LPDIRECTDRAW2 iface, DWORD dwFlags,
			      LPDDSURFACEDESC pDDSD, LPVOID context,
			      LPDDENUMSURFACESCALLBACK cb)
{
    HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue =  Main_DirectDraw_EnumSurfaces((LPDIRECTDRAW7) iface, dwFlags, &pDDSD2,
		                                         context, (LPDDENUMSURFACESCALLBACK7)cb);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =   Main_DirectDraw_EnumSurfaces((LPDIRECTDRAW7) iface, dwFlags, NULL,
			                                       context, (LPDDENUMSURFACESCALLBACK7)cb);
	}

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw4_EnumSurfaces(LPDIRECTDRAW4 iface, DWORD dwFlags,
			      LPDDSURFACEDESC2 pDDSD, LPVOID context,
			      LPDDENUMSURFACESCALLBACK2 cb)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_EnumSurfaces((LPDIRECTDRAW7) iface, dwFlags, pDDSD, context, (LPDDENUMSURFACESCALLBACK7)cb);
}



HRESULT WINAPI
ThunkDirectDraw_FlipToGDISurface(LPDIRECTDRAW iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_FlipToGDISurface((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw2_FlipToGDISurface(LPDIRECTDRAW2 iface)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_FlipToGDISurface((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw4_FlipToGDISurface(LPDIRECTDRAW4 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_FlipToGDISurface((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw_GetCaps(LPDIRECTDRAW iface, LPDDCAPS pDDC1, LPDDCAPS pDDC2)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetCaps((LPDIRECTDRAW7) iface, pDDC1, pDDC2);
}

HRESULT WINAPI
ThunkDirectDraw2_GetCaps(LPDIRECTDRAW2 iface, LPDDCAPS pDDC1, LPDDCAPS pDDC2)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetCaps((LPDIRECTDRAW7) iface, pDDC1, pDDC2);
}

HRESULT WINAPI
ThunkDirectDraw4_GetCaps(LPDIRECTDRAW4 iface, LPDDCAPS pDDC1, LPDDCAPS pDDC2)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_GetCaps((LPDIRECTDRAW7) iface, pDDC1, pDDC2);
}

HRESULT WINAPI
ThunkDirectDraw_GetDisplayMode(LPDIRECTDRAW iface, LPDDSURFACEDESC pDDSD)
{

	HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue = Main_DirectDraw_GetDisplayMode((LPDIRECTDRAW7) iface, &pDDSD2);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =   Main_DirectDraw_GetDisplayMode((LPDIRECTDRAW7) iface, NULL);
	}

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw2_GetDisplayMode(LPDIRECTDRAW2 iface, LPDDSURFACEDESC pDDSD)
{
    HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDSD != NULL)
	{
	   DDSURFACEDESC2 pDDSD2;
	   ZeroMemory(&pDDSD2,sizeof(DDSURFACEDESC2));
	   memcpy(&pDDSD2, pDDSD, sizeof(DDSURFACEDESC));
	   pDDSD2.dwSize = sizeof(DDSURFACEDESC2);
	   retValue = Main_DirectDraw_GetDisplayMode((LPDIRECTDRAW7) iface, &pDDSD2);
       memcpy(pDDSD, &pDDSD2, sizeof(DDSURFACEDESC));
	   pDDSD->dwSize = sizeof(DDSURFACEDESC);
	}
	else
	{
		retValue =   Main_DirectDraw_GetDisplayMode((LPDIRECTDRAW7) iface, NULL);
	}

    return retValue;
}

HRESULT WINAPI
ThunkDirectDraw4_GetDisplayMode(LPDIRECTDRAW4 iface, LPDDSURFACEDESC2 pDDSD2)
{
	DX_WINDBG_trace();

    return  Main_DirectDraw_GetDisplayMode((LPDIRECTDRAW7) iface, pDDSD2);
}

HRESULT WINAPI
ThunkDirectDraw_GetFourCCCodes(LPDIRECTDRAW iface, LPDWORD pNumCodes,
			       LPDWORD pCodes)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetFourCCCodes((LPDIRECTDRAW7) iface, pNumCodes, pCodes);
}

HRESULT WINAPI
ThunkDirectDraw2_GetFourCCCodes(LPDIRECTDRAW2 iface, LPDWORD pNumCodes,
				LPDWORD pCodes)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_GetFourCCCodes((LPDIRECTDRAW7) iface, pNumCodes, pCodes);
}

HRESULT WINAPI
ThunkDirectDraw4_GetFourCCCodes(LPDIRECTDRAW4 iface, LPDWORD pNumCodes,
				LPDWORD pCodes)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetFourCCCodes((LPDIRECTDRAW7) iface, pNumCodes, pCodes);
}

HRESULT WINAPI
ThunkDirectDraw_GetGDISurface(LPDIRECTDRAW iface, LPDIRECTDRAWSURFACE *lplpGDIDDSSurface)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_GetGDISurface((LPDIRECTDRAW7) iface, (LPDIRECTDRAWSURFACE7 *) lplpGDIDDSSurface);
}

HRESULT WINAPI
ThunkDirectDraw2_GetGDISurface(LPDIRECTDRAW2 iface, LPDIRECTDRAWSURFACE *lplpGDIDDSSurface)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_GetGDISurface((LPDIRECTDRAW7) iface, (LPDIRECTDRAWSURFACE7 *) lplpGDIDDSSurface);
}

HRESULT WINAPI
ThunkDirectDraw4_GetGDISurface(LPDIRECTDRAW4 iface, LPDIRECTDRAWSURFACE4 *lplpGDIDDSSurface)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_GetGDISurface((LPDIRECTDRAW7) iface, (LPDIRECTDRAWSURFACE7 *) lplpGDIDDSSurface);
}

HRESULT WINAPI
ThunkDirectDraw_GetMonitorFrequency(LPDIRECTDRAW iface, LPDWORD pdwFreq)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetMonitorFrequency((LPDIRECTDRAW7) iface, pdwFreq);
}

HRESULT WINAPI
ThunkDirectDraw2_GetMonitorFrequency(LPDIRECTDRAW2 iface, LPDWORD pdwFreq)
{
 	DX_WINDBG_trace();

    return Main_DirectDraw_GetMonitorFrequency((LPDIRECTDRAW7) iface, pdwFreq);
}

HRESULT WINAPI
ThunkDirectDraw4_GetMonitorFrequency(LPDIRECTDRAW4 iface, LPDWORD pdwFreq)
{
  	DX_WINDBG_trace();

    return Main_DirectDraw_GetMonitorFrequency((LPDIRECTDRAW7) iface, pdwFreq);
}

HRESULT WINAPI
ThunkDirectDraw_GetScanLine(LPDIRECTDRAW iface, LPDWORD lpdwScanLine)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetScanLine((LPDIRECTDRAW7) iface, lpdwScanLine);
}

HRESULT WINAPI
ThunkDirectDraw2_GetScanLine(LPDIRECTDRAW2 iface, LPDWORD lpdwScanLine)
{
   DX_WINDBG_trace();

   return Main_DirectDraw_GetScanLine((LPDIRECTDRAW7) iface, lpdwScanLine);
}

HRESULT WINAPI
ThunkDirectDraw4_GetScanLine(LPDIRECTDRAW4 iface, LPDWORD lpdwScanLine)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetScanLine((LPDIRECTDRAW7) iface, lpdwScanLine);
}

HRESULT WINAPI
ThunkDirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW iface, LPBOOL lpbIsInVB)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetVerticalBlankStatus((LPDIRECTDRAW7) iface, lpbIsInVB);
}

HRESULT WINAPI
ThunkDirectDraw2_GetVerticalBlankStatus(LPDIRECTDRAW2 iface, LPBOOL lpbIsInVB)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_GetVerticalBlankStatus((LPDIRECTDRAW7) iface, lpbIsInVB);
}

HRESULT WINAPI
ThunkDirectDraw4_GetVerticalBlankStatus(LPDIRECTDRAW4 iface, LPBOOL lpbIsInVB)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_GetVerticalBlankStatus((LPDIRECTDRAW7) iface, lpbIsInVB);
}

HRESULT WINAPI
ThunkDirectDraw_Initialize(LPDIRECTDRAW iface, LPGUID pGUID)
{
    DX_WINDBG_trace();

	if (iface==NULL)
	{
		return DDERR_NOTINITIALIZED;
	}

    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI
ThunkDirectDraw2_Initialize(LPDIRECTDRAW2 iface, LPGUID pGUID)
{
    DX_WINDBG_trace();

	if (iface==NULL)
	{
		return DDERR_NOTINITIALIZED;
	}

    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI
ThunkDirectDraw4_Initialize(LPDIRECTDRAW4 iface, LPGUID pGUID)
{
    DX_WINDBG_trace();

	if (iface==NULL)
	{
		return DDERR_NOTINITIALIZED;
	}

    return DDERR_ALREADYINITIALIZED;
}


HRESULT WINAPI
ThunkDirectDraw_RestoreDisplayMode(LPDIRECTDRAW iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_RestoreDisplayMode((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_RestoreDisplayMode((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw4_RestoreDisplayMode(LPDIRECTDRAW4 iface)
{
 	DX_WINDBG_trace();

    return Main_DirectDraw_RestoreDisplayMode((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw_SetCooperativeLevel(LPDIRECTDRAW iface, HWND hwnd, DWORD dwFlags)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_SetCooperativeLevel ((LPDIRECTDRAW7) iface,  hwnd, dwFlags);
}

HRESULT WINAPI
ThunkDirectDraw2_SetCooperativeLevel(LPDIRECTDRAW2 iface, HWND hwnd, DWORD dwFlags)
{
   	DX_WINDBG_trace();

    return Main_DirectDraw_SetCooperativeLevel ((LPDIRECTDRAW7) iface,  hwnd, dwFlags);
}

HRESULT WINAPI
ThunkDirectDraw4_SetCooperativeLevel(LPDIRECTDRAW4 iface, HWND hwnd, DWORD dwFlags)
{
   	DX_WINDBG_trace();

    return Main_DirectDraw_SetCooperativeLevel ((LPDIRECTDRAW7) iface,  hwnd, dwFlags);
}

HRESULT WINAPI
ThunkDirectDraw_SetDisplayMode(LPDIRECTDRAW iface, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	DX_WINDBG_trace();

	return Main_DirectDraw_SetDisplayMode ((LPDIRECTDRAW7) iface, dwWidth, dwHeight, dwBPP, 0, 0);
}

HRESULT WINAPI
ThunkDirectDraw2_SetDisplayMode(LPDIRECTDRAW2 iface, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD d, DWORD e)
{
    DX_WINDBG_trace();

	return Main_DirectDraw_SetDisplayMode ((LPDIRECTDRAW7) iface, dwWidth, dwHeight, dwBPP, 0, 0);
}

HRESULT WINAPI
ThunkDirectDraw4_SetDisplayMode(LPDIRECTDRAW4 iface, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD d, DWORD e)
{
    DX_WINDBG_trace();

	return Main_DirectDraw_SetDisplayMode ((LPDIRECTDRAW7) iface, dwWidth, dwHeight, dwBPP, 0, 0);
}

HRESULT WINAPI
ThunkDirectDraw_WaitForVerticalBlank(LPDIRECTDRAW iface, DWORD dwFlags, HANDLE hEvent)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_WaitForVerticalBlank((LPDIRECTDRAW7) iface, dwFlags, hEvent);
}

HRESULT WINAPI
ThunkDirectDraw2_WaitForVerticalBlank(LPDIRECTDRAW2 iface, DWORD dwFlags, HANDLE hEvent)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_WaitForVerticalBlank((LPDIRECTDRAW7) iface, dwFlags, hEvent);
}

HRESULT WINAPI
ThunkDirectDraw4_WaitForVerticalBlank(LPDIRECTDRAW4 iface, DWORD dwFlags, HANDLE hEvent)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_WaitForVerticalBlank((LPDIRECTDRAW7) iface, dwFlags, hEvent);
}





HRESULT WINAPI
ThunkDirectDraw4_GetSurfaceFromDC(LPDIRECTDRAW4 iface, HDC hdc, LPDIRECTDRAWSURFACE4 *pSurf)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_GetSurfaceFromDC((LPDIRECTDRAW7) iface,  hdc, (LPDIRECTDRAWSURFACE7 *) pSurf);
}

HRESULT WINAPI
ThunkDirectDraw4_RestoreAllSurfaces(LPDIRECTDRAW4 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_RestoreAllSurfaces((LPDIRECTDRAW7) iface);
}

HRESULT WINAPI
ThunkDirectDraw4_TestCooperativeLevel(LPDIRECTDRAW4 iface)
{
	DX_WINDBG_trace();

    return Main_DirectDraw_TestCooperativeLevel((LPDIRECTDRAW7) iface);
}



HRESULT WINAPI
ThunkDirectDraw4_GetDeviceIdentifier(LPDIRECTDRAW4 iface,
				     LPDDDEVICEIDENTIFIER pDDDI, DWORD dwFlags)
{
    HRESULT retValue = DDERR_GENERIC;

	DX_WINDBG_trace();

	if (pDDDI != NULL)
	{

	   DDDEVICEIDENTIFIER2 pDDDI2;
	   ZeroMemory(&pDDDI2,sizeof(DDDEVICEIDENTIFIER2));
	   memcpy(&pDDDI2, pDDDI, sizeof(DDDEVICEIDENTIFIER));
	   retValue = Main_DirectDraw_GetDeviceIdentifier((LPDIRECTDRAW7) iface, &pDDDI2, dwFlags);
       memcpy(pDDDI, &pDDDI2, sizeof(DDDEVICEIDENTIFIER));
	}
	else
	{
		retValue = Main_DirectDraw_GetDeviceIdentifier((LPDIRECTDRAW7) iface, NULL, dwFlags);
	}

    return retValue;
}



IDirectDrawVtbl DirectDraw_Vtable =
{
    ThunkDirectDraw_QueryInterface,
    ThunkDirectDraw_AddRef,
    ThunkDirectDraw_Release,
    ThunkDirectDraw_Compact,
    ThunkDirectDraw_CreateClipper,
    ThunkDirectDraw_CreatePalette,
    ThunkDirectDraw_CreateSurface,
    ThunkDirectDraw_DuplicateSurface,
    ThunkDirectDraw_EnumDisplayModes,
    ThunkDirectDraw_EnumSurfaces,
    ThunkDirectDraw_FlipToGDISurface,
    ThunkDirectDraw_GetCaps,
    ThunkDirectDraw_GetDisplayMode,
    ThunkDirectDraw_GetFourCCCodes,
    ThunkDirectDraw_GetGDISurface,
    ThunkDirectDraw_GetMonitorFrequency,
    ThunkDirectDraw_GetScanLine,
    ThunkDirectDraw_GetVerticalBlankStatus,
    ThunkDirectDraw_Initialize,
    ThunkDirectDraw_RestoreDisplayMode,
    ThunkDirectDraw_SetCooperativeLevel,
    ThunkDirectDraw_SetDisplayMode,
    ThunkDirectDraw_WaitForVerticalBlank,
};

IDirectDraw2Vtbl DirectDraw2_Vtable =
{
    ThunkDirectDraw2_QueryInterface,
    ThunkDirectDraw2_AddRef,
    ThunkDirectDraw2_Release,
    ThunkDirectDraw2_Compact,
    ThunkDirectDraw2_CreateClipper,
    ThunkDirectDraw2_CreatePalette,
    ThunkDirectDraw2_CreateSurface,
    ThunkDirectDraw2_DuplicateSurface,
    ThunkDirectDraw2_EnumDisplayModes,
    ThunkDirectDraw2_EnumSurfaces,
    ThunkDirectDraw2_FlipToGDISurface,
    ThunkDirectDraw2_GetCaps,
    ThunkDirectDraw2_GetDisplayMode,
    ThunkDirectDraw2_GetFourCCCodes,
    ThunkDirectDraw2_GetGDISurface,
    ThunkDirectDraw2_GetMonitorFrequency,
    ThunkDirectDraw2_GetScanLine,
    ThunkDirectDraw2_GetVerticalBlankStatus,
    ThunkDirectDraw2_Initialize,
    ThunkDirectDraw2_RestoreDisplayMode,
    ThunkDirectDraw2_SetCooperativeLevel,
    ThunkDirectDraw2_SetDisplayMode,
    ThunkDirectDraw2_WaitForVerticalBlank,
    ThunkDirectDraw2_GetAvailableVidMem
};

IDirectDraw4Vtbl DirectDraw4_Vtable =
{
    ThunkDirectDraw4_QueryInterface,
    ThunkDirectDraw4_AddRef,
    ThunkDirectDraw4_Release,
    ThunkDirectDraw4_Compact,
    ThunkDirectDraw4_CreateClipper,
    ThunkDirectDraw4_CreatePalette,
    ThunkDirectDraw4_CreateSurface,
    ThunkDirectDraw4_DuplicateSurface,
    ThunkDirectDraw4_EnumDisplayModes,
    ThunkDirectDraw4_EnumSurfaces,
    ThunkDirectDraw4_FlipToGDISurface,
    ThunkDirectDraw4_GetCaps,
    ThunkDirectDraw4_GetDisplayMode,
    ThunkDirectDraw4_GetFourCCCodes,
    ThunkDirectDraw4_GetGDISurface,
    ThunkDirectDraw4_GetMonitorFrequency,
    ThunkDirectDraw4_GetScanLine,
    ThunkDirectDraw4_GetVerticalBlankStatus,
    ThunkDirectDraw4_Initialize,
    ThunkDirectDraw4_RestoreDisplayMode,
    ThunkDirectDraw4_SetCooperativeLevel,
    ThunkDirectDraw4_SetDisplayMode,
    ThunkDirectDraw4_WaitForVerticalBlank,
    ThunkDirectDraw4_GetAvailableVidMem,
    ThunkDirectDraw4_GetSurfaceFromDC,
    ThunkDirectDraw4_RestoreAllSurfaces,
    ThunkDirectDraw4_TestCooperativeLevel,
    ThunkDirectDraw4_GetDeviceIdentifier
};
