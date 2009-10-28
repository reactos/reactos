/* $Id: palette.c $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/Palette/palette.c
 * PURPOSE:              IDirectDrawPalette Implementation
 * PROGRAMMER:           Kamil Hornicek
 *
 */

#include "rosdraw.h"

LPDDRAWI_DDRAWPALETTE_INT
internal_directdrawpalette_int_alloc(LPDDRAWI_DDRAWPALETTE_INT This)
{
    LPDDRAWI_DDRAWPALETTE_INT  newThis;
    DxHeapMemAlloc(newThis, sizeof(DDRAWI_DDRAWPALETTE_INT));
    if (newThis)
    {
        newThis->lpLcl = This->lpLcl;
        newThis->lpLink = This;
    }

    return  newThis;
}

ULONG WINAPI
Main_DirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE iface)
{
    LPDDRAWI_DDRAWPALETTE_INT This = (LPDDRAWI_DDRAWPALETTE_INT)iface;
    ULONG ref = 0;

    AcquireDDThreadLock();

    _SEH2_TRY
    {
        ref = ++This->dwIntRefCnt;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END

    ReleaseDDThreadLock();

    return ref;
}

HRESULT WINAPI
Main_DirectDrawPalette_QueryInterface(LPDIRECTDRAWPALETTE iface, REFIID refiid, LPVOID *ppObj)
{
    HRESULT retVal = DD_OK;
    *ppObj = NULL;
    LPDDRAWI_DDRAWPALETTE_INT This = (LPDDRAWI_DDRAWPALETTE_INT) iface;

    DX_WINDBG_trace();

    _SEH2_TRY
    {
        if (IsEqualGUID(refiid, &IID_IUnknown) || IsEqualGUID(refiid, &IID_IDirectDrawPalette))
        {
            if (This->lpVtbl != &DirectDrawPalette_Vtable)
            {
                This = internal_directdrawpalette_int_alloc(This);
                if (!This)
                {
                    retVal = DDERR_OUTOFVIDEOMEMORY;
                    _SEH2_LEAVE;
                }
            }
            This->lpVtbl = &DirectDrawPalette_Vtable;
            *ppObj = This;
            Main_DirectDrawPalette_AddRef((LPDIRECTDRAWPALETTE)This);
        }
        else
        {
            DX_STUB_str("E_NOINTERFACE\n");
            retVal = E_NOINTERFACE;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return retVal;
}

ULONG WINAPI
Main_DirectDrawPalette_Release(LPDIRECTDRAWPALETTE iface)
{
    ULONG ref = 0;
    LPDDRAWI_DDRAWPALETTE_INT This = (LPDDRAWI_DDRAWPALETTE_INT)iface;

    AcquireDDThreadLock();

    _SEH2_TRY
    {
        This->dwIntRefCnt--;
        ref = This->dwIntRefCnt;
        if(ref == 0)
        {
            DxHeapMemFree(This);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END

    ReleaseDDThreadLock();

    return ref;
}
