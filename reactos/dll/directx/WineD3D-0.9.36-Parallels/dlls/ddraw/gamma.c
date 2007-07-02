/* DirectDrawGammaControl implementation
 *
 * Copyright 2001 TransGaming Technologies Inc.
 * Copyright 2006 Stefan Dösinger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

#ifndef WINE_NATIVEWIN32
# include "windef.h"
# include "winbase.h"
# include "winnls.h"
# include "winerror.h"
# include "wingdi.h"
#endif
#include "wine/exception.h"
#include "excpt.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_thunk);

/**********************************************************
 * IUnknown parts follow
 **********************************************************/

/**********************************************************
 * IDirectDrawGammaControl::QueryInterface
 *
 * QueryInterface, thunks to IDirectDrawSurface
 *
 * Params:
 *  riid: Interface id queried for
 *  obj: Returns the interface pointer
 *
 * Returns:
 *  S_OK or E_NOINTERFACE: See IDirectDrawSurface7::QueryInterface
 *
 **********************************************************/
static HRESULT WINAPI
IDirectDrawGammaControlImpl_QueryInterface(IDirectDrawGammaControl *iface, REFIID riid,
                                           void **obj)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p): Thunking to IDirectDrawSurface7\n", This, debugstr_guid(riid), obj);

    return IDirectDrawSurface7_QueryInterface(ICOM_INTERFACE(This, IDirectDrawSurface7),
                                              riid,
                                              obj);
}

/**********************************************************
 * IDirectDrawGammaControl::AddRef
 *
 * Addref, thunks to IDirectDrawSurface
 *
 * Returns:
 *  The new refcount
 *
 **********************************************************/
static ULONG WINAPI
IDirectDrawGammaControlImpl_AddRef(IDirectDrawGammaControl *iface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE_(ddraw_thunk)("(%p)->() Thunking to IDirectDrawSurface7\n", This);

    return IDirectDrawSurface7_AddRef(ICOM_INTERFACE(This, IDirectDrawSurface7));
}

/**********************************************************
 * IDirectDrawGammaControl::Release
 *
 * Release, thunks to IDirectDrawSurface
 *
 * Returns:
 *  The new refcount
 *
 **********************************************************/
static ULONG WINAPI
IDirectDrawGammaControlImpl_Release(IDirectDrawGammaControl *iface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE_(ddraw_thunk)("(%p)->() Thunking to IDirectDrawSurface7\n", This);

    return IDirectDrawSurface7_Release(ICOM_INTERFACE(This, IDirectDrawSurface7));
}

/**********************************************************
 * IDirectDrawGammaControl
 **********************************************************/

/**********************************************************
 * IDirectDrawGammaControl::GetGammaRamp
 *
 * Returns the current gamma ramp for a surface
 *
 * Params:
 *  Flags: Ignored
 *  GammaRamp: Address to write the ramp to
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if GammaRamp is NULL
 *
 **********************************************************/
static HRESULT WINAPI
IDirectDrawGammaControlImpl_GetGammaRamp(IDirectDrawGammaControl *iface,
                                         DWORD Flags,
                                         DDGAMMARAMP *GammaRamp)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE("(%p)->(%08x,%p)\n", This,Flags,GammaRamp);

    /* This looks sane */
    if(!GammaRamp)
    {
        ERR("(%p) GammaRamp is NULL, returning DDERR_INVALIDPARAMS\n", This);
        return DDERR_INVALIDPARAMS;
    }

    if(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        /* Note: DDGAMMARAMP is compatible with WINED3DGAMMARAMP */
        IWineD3DDevice_GetGammaRamp(This->ddraw->wineD3DDevice,
                                    0 /* Swapchain */,
                                    (WINED3DGAMMARAMP *) GammaRamp);
    }
    else
    {
        ERR("(%p) Unimplemented for non-primary surfaces\n", This);
    }

    return DD_OK;
}

/**********************************************************
 * IDirectDrawGammaControl::SetGammaRamp
 *
 * Sets the red, green and blue gamma ramps for
 *
 * Params:
 *  Flags: Can be DDSGR_CALIBRATE to request calibration
 *  GammaRamp: Structure containing the new gamma ramp
 *
 * Returns:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if GammaRamp is NULL
 *
 **********************************************************/
static HRESULT WINAPI
IDirectDrawGammaControlImpl_SetGammaRamp(IDirectDrawGammaControl *iface,
                                         DWORD Flags,
                                         DDGAMMARAMP *GammaRamp)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE("(%p)->(%08x,%p)\n", This,Flags,GammaRamp);

    /* This looks sane */
    if(!GammaRamp)
    {
        ERR("(%p) GammaRamp is NULL, returning DDERR_INVALIDPARAMS\n", This);
        return DDERR_INVALIDPARAMS;
    }

    if(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {

        /* Note: DDGAMMARAMP is compatible with WINED3DGAMMARAMP */
        IWineD3DDevice_SetGammaRamp(This->ddraw->wineD3DDevice,
                                    0 /* Swapchain */,
                                    Flags,
                                    (WINED3DGAMMARAMP *) GammaRamp);
    }
    else
    {
        ERR("(%p) Unimplemented for non-primary surfaces\n", This);
    }

    return DD_OK;
}

const IDirectDrawGammaControlVtbl IDirectDrawGammaControl_Vtbl =
{
    IDirectDrawGammaControlImpl_QueryInterface,
    IDirectDrawGammaControlImpl_AddRef,
    IDirectDrawGammaControlImpl_Release,
    IDirectDrawGammaControlImpl_GetGammaRamp,
    IDirectDrawGammaControlImpl_SetGammaRamp
};
