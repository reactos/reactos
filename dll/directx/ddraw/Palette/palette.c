/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Palette/palette.c
 * PURPOSE:              IDirectDrawPalette Implementation
 * PROGRAMMER:           Jérôme Gardou
 *
 */

#include "rosdraw.h"

/*****************************************************************************
 * IDirectDrawPalette::QueryInterface
 *
 * A usual QueryInterface implementation. Can only Query IUnknown and
 * IDirectDrawPalette
 *
 * Params:
 *  refiid: The interface id queried for
 *  obj: Address to return the interface pointer at
 *
 * Returns:
 *  S_OK on success
 *  E_NOINTERFACE if the requested interface wasn't found
 *****************************************************************************/
static HRESULT WINAPI
DirectDrawPalette_QueryInterface(IDirectDrawPalette *iface,
                                      REFIID refiid,
                                      void **obj)
{
    if (IsEqualGUID(refiid, &IID_IUnknown)
        || IsEqualGUID(refiid, &IID_IDirectDrawPalette))
    {
        *obj = iface;
        IDirectDrawPalette_AddRef(iface);
        return S_OK;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

/*****************************************************************************
 * IDirectDrawPaletteImpl::AddRef
 *
 * Increases the refcount.
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
DirectDrawPalette_AddRef(IDirectDrawPalette *iface)
{
    LPDDRAWI_DDRAWPALETTE_INT This = (LPDDRAWI_DDRAWPALETTE_INT)iface;
    ULONG ref = 0;

    _SEH2_TRY
    {
        ref = ++This->dwIntRefCnt;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END

    return ref;
}

/*****************************************************************************
 * IDirectDrawPaletteImpl::Release
 *
 * Reduces the refcount. If the refcount falls to 0, the object is destroyed
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
DirectDrawPalette_Release(IDirectDrawPalette *iface)
{
    LPDDRAWI_DDRAWPALETTE_INT This = (LPDDRAWI_DDRAWPALETTE_INT)iface;
    ULONG ref = 0;

    _SEH2_TRY
    {
        ref = --This->dwIntRefCnt;
        if(ref == 0)
        {
            AcquireDDThreadLock();
            if(((LPDDRAWI_DIRECTDRAW_INT)This->lpLcl->dwReserved1)->lpVtbl == &DirectDraw7_Vtable
                    || ((LPDDRAWI_DIRECTDRAW_INT)This->lpLcl->dwReserved1)->lpVtbl == &DirectDraw4_Vtable)
                Main_DirectDraw_Release((LPDDRAWI_DIRECTDRAW_INT)This->lpLcl->dwReserved1) ;
            DxHeapMemFree(This); //HUGE FIXME!!!
            ReleaseDDThreadLock();
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END

    return ref;
}

static HRESULT WINAPI
DirectDrawPalette_Initialize( LPDIRECTDRAWPALETTE iface,
				              LPDIRECTDRAW ddraw,
                              DWORD dwFlags,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

static HRESULT WINAPI
DirectDrawPalette_GetEntries( LPDIRECTDRAWPALETTE iface,
                              DWORD dwFlags,
				              DWORD dwStart, DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

static HRESULT WINAPI
DirectDrawPalette_SetEntries( LPDIRECTDRAWPALETTE iface,
                              DWORD dwFlags,
				              DWORD dwStart,
                              DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

static HRESULT WINAPI
DirectDrawPalette_GetCaps( LPDIRECTDRAWPALETTE iface,
                           LPDWORD lpdwCaps)
{
   DX_WINDBG_trace();
   DX_STUB;
}

const IDirectDrawPaletteVtbl DirectDrawPalette_Vtable =
{
    DirectDrawPalette_QueryInterface,
    DirectDrawPalette_AddRef,
    DirectDrawPalette_Release,
    DirectDrawPalette_GetCaps,
    DirectDrawPalette_GetEntries,
    DirectDrawPalette_Initialize,
    DirectDrawPalette_SetEntries
};
