/* Direct3D Viewport
 * Copyright (c) 1998 Lionel ULMER
 * Copyright (c) 2006 Stefan DÖSINGER
 *
 * This file contains the implementation of Direct3DViewport2.
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
#define NONAMELESSUNION

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

WINE_DEFAULT_DEBUG_CHANNEL(d3d7);

/*****************************************************************************
 * Helper functions
 *****************************************************************************/

/*****************************************************************************
 * viewport_activate
 *
 * activates the viewport using IDirect3DDevice7::SetViewport
 *
 *****************************************************************************/
void viewport_activate(IDirect3DViewportImpl* This) {
    IDirect3DLightImpl* light;
    D3DVIEWPORT7 vp;
    
    /* Activate all the lights associated with this context */
    light = This->lights;

    while (light != NULL) {
        light->activate(light);
	light = light->next;
    }

    /* And copy the values in the structure used by the device */
    if (This->use_vp2) {
        vp.dwX = This->viewports.vp2.dwX;
	vp.dwY = This->viewports.vp2.dwY;
	vp.dwHeight = This->viewports.vp2.dwHeight;
	vp.dwWidth = This->viewports.vp2.dwWidth;
	vp.dvMinZ = This->viewports.vp2.dvMinZ;
	vp.dvMaxZ = This->viewports.vp2.dvMaxZ;
    } else {
        vp.dwX = This->viewports.vp1.dwX;
	vp.dwY = This->viewports.vp1.dwY;
	vp.dwHeight = This->viewports.vp1.dwHeight;
	vp.dwWidth = This->viewports.vp1.dwWidth;
	vp.dvMinZ = This->viewports.vp1.dvMinZ;
	vp.dvMaxZ = This->viewports.vp1.dvMaxZ;
    }
    
    /* And also set the viewport */
    IDirect3DDevice7_SetViewport(ICOM_INTERFACE(This->active_device, IDirect3DDevice7), &vp);
}

/*****************************************************************************
 * _dump_D3DVIEWPORT, _dump_D3DVIEWPORT2
 *
 * Writes viewport information to TRACE
 *
 *****************************************************************************/
static void _dump_D3DVIEWPORT(const D3DVIEWPORT *lpvp)
{
    TRACE("    - dwSize = %d   dwX = %d   dwY = %d\n",
	  lpvp->dwSize, lpvp->dwX, lpvp->dwY);
    TRACE("    - dwWidth = %d   dwHeight = %d\n",
	  lpvp->dwWidth, lpvp->dwHeight);
    TRACE("    - dvScaleX = %f   dvScaleY = %f\n",
	  lpvp->dvScaleX, lpvp->dvScaleY);
    TRACE("    - dvMaxX = %f   dvMaxY = %f\n",
	  lpvp->dvMaxX, lpvp->dvMaxY);
    TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	  lpvp->dvMinZ, lpvp->dvMaxZ);
}

static void _dump_D3DVIEWPORT2(const D3DVIEWPORT2 *lpvp)
{
    TRACE("    - dwSize = %d   dwX = %d   dwY = %d\n",
	  lpvp->dwSize, lpvp->dwX, lpvp->dwY);
    TRACE("    - dwWidth = %d   dwHeight = %d\n",
	  lpvp->dwWidth, lpvp->dwHeight);
    TRACE("    - dvClipX = %f   dvClipY = %f\n",
	  lpvp->dvClipX, lpvp->dvClipY);
    TRACE("    - dvClipWidth = %f   dvClipHeight = %f\n",
	  lpvp->dvClipWidth, lpvp->dvClipHeight);
    TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	  lpvp->dvMinZ, lpvp->dvMaxZ);
}

/*****************************************************************************
 * IUnknown Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DViewport3::QueryInterface
 *
 * A normal QueryInterface. Can query all interface versions and the
 * IUnknown interface. The VTables of the different versions
 * are equal
 *
 * Params:
 *  refiid: Interface id queried for
 *  obj: Address to write the interface pointer to
 *
 * Returns:
 *  S_OK on success.
 *  E_NOINTERFACE if the requested interface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_QueryInterface(IDirect3DViewport3 *iface,
                                     REFIID riid,
                                     void **obp)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID(&IID_IUnknown,  riid) ||
	 IsEqualGUID(&IID_IDirect3DViewport, riid) ||
	 IsEqualGUID(&IID_IDirect3DViewport2, riid) ||
	 IsEqualGUID(&IID_IDirect3DViewport3, riid) ) {
        IDirect3DViewport3_AddRef(ICOM_INTERFACE(This, IDirect3DViewport3));
        *obp = ICOM_INTERFACE(This, IDirect3DViewport3);
	TRACE("  Creating IDirect3DViewport1/2/3 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirect3DViewport3::AddRef
 *
 * Increases the refcount.
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DViewportImpl_AddRef(IDirect3DViewport3 *iface)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %u.\n", This, ref - 1);

    return ref;
}

/*****************************************************************************
 * IDirect3DViewport3::Release
 *
 * Reduces the refcount. If it falls to 0, the interface is released
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DViewportImpl_Release(IDirect3DViewport3 *iface)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (!ref) {
        HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

/*****************************************************************************
 * IDirect3DViewport Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DViewport3::Initialize
 *
 * No-op initialization.
 *
 * Params:
 *  Direct3D: The direct3D device this viewport is assigned to
 *
 * Returns:
 *  DDERR_ALREADYINITIALIZED
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_Initialize(IDirect3DViewport3 *iface,
                                 IDirect3D *Direct3D)
{
    TRACE("(%p)->(%p) no-op...\n", iface, Direct3D);
    return DDERR_ALREADYINITIALIZED;
}

/*****************************************************************************
 * IDirect3DViewport3::GetViewport
 *
 * Returns the viewport data assigned to this viewport interface
 *
 * Params:
 *  Data: Address to store the data
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_GetViewport(IDirect3DViewport3 *iface,
                                  D3DVIEWPORT *lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);
    if (This->use_vp2 != 0) {
        ERR("  Requesting to get a D3DVIEWPORT struct where a D3DVIEWPORT2 was set !\n");
	return DDERR_INVALIDPARAMS;
    }
    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    memcpy(lpData, &(This->viewports.vp1), dwSize);

    if (TRACE_ON(d3d7)) {
        TRACE("  returning D3DVIEWPORT :\n");
	_dump_D3DVIEWPORT(lpData);
    }
    
    return DD_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::SetViewport
 *
 * Sets the viewport information for this interface
 *
 * Params:
 *  lpData: Viewport to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_SetViewport(IDirect3DViewport3 *iface,
                                  D3DVIEWPORT *lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    LPDIRECT3DVIEWPORT3 current_viewport;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(d3d7)) {
        TRACE("  getting D3DVIEWPORT :\n");
	_dump_D3DVIEWPORT(lpData);
    }

    This->use_vp2 = 0;
    memset(&(This->viewports.vp1), 0, sizeof(This->viewports.vp1));
    memcpy(&(This->viewports.vp1), lpData, lpData->dwSize);

    /* Tests on two games show that these values are never used properly so override
       them with proper ones :-)
    */
    This->viewports.vp1.dvMinZ = 0.0;
    This->viewports.vp1.dvMaxZ = 1.0;

    if (This->active_device) {
      IDirect3DDevice3_GetCurrentViewport(ICOM_INTERFACE(This->active_device, IDirect3DDevice3), &current_viewport);
      if (ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, current_viewport) == This)
          This->activate(This);
      if(current_viewport) IDirect3DViewport3_Release(current_viewport);
    }

    return DD_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::TransformVertices
 *
 * Transforms vertices by the transformation matrix.
 *
 * Params:
 *  dwVertexCount: The number of vertices to be transformed
 *  lpData: Pointer to the vertex data
 *  dwFlags: D3DTRANSFORM_CLIPPED or D3DTRANSFORM_UNCLIPPED
 *  lpOffScreen: Is set to nonzero if all vertices are off-screen
 *
 * Returns:
 *  D3D_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_TransformVertices(IDirect3DViewport3 *iface,
                                        DWORD dwVertexCount,
                                        D3DTRANSFORMDATA *lpData,
                                        DWORD dwFlags,
                                        DWORD *lpOffScreen)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p)->(%08x,%p,%08x,%p): stub!\n", This, dwVertexCount, lpData, dwFlags, lpOffScreen);
    if (lpOffScreen)
	*lpOffScreen = 0;
    return DD_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::LightElements
 *
 * The DirectX 5.0 sdk says that it's not implemented
 *
 * Params:
 *  ?
 *
 * Returns:
 *  DDERR_UNSUPPORTED
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_LightElements(IDirect3DViewport3 *iface,
                                    DWORD dwElementCount,
                                    LPD3DLIGHTDATA lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p)->(%08x,%p): Unimplemented!\n", This, dwElementCount, lpData);
    return DDERR_UNSUPPORTED;
}

/*****************************************************************************
 * IDirect3DViewport3::SetBackground
 *
 * Sets tje background material
 *
 * Params:
 *  hMat: Handle from a IDirect3DMaterial interface
 *
 * Returns:
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_SetBackground(IDirect3DViewport3 *iface,
                                    D3DMATERIALHANDLE hMat)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p)->(%d)\n", This, (DWORD) hMat);

    if(hMat && hMat > This->ddraw->d3ddevice->numHandles)
    {
        WARN("Specified Handle %d out of range\n", hMat);
        return DDERR_INVALIDPARAMS;
    }
    else if(hMat && This->ddraw->d3ddevice->Handles[hMat - 1].type != DDrawHandle_Material)
    {
        WARN("Handle %d is not a material handle\n", hMat);
        return DDERR_INVALIDPARAMS;
    }

    if(hMat)
    {
        This->background = (IDirect3DMaterialImpl *) This->ddraw->d3ddevice->Handles[hMat - 1].ptr;
        TRACE(" setting background color : %f %f %f %f\n",
#ifndef WINE_NATIVEWIN32
              This->background->mat.u.diffuse.u1.r,
              This->background->mat.u.diffuse.u2.g,
              This->background->mat.u.diffuse.u3.b,
              This->background->mat.u.diffuse.u4.a);
#else
              This->background->mat.diffuse.r,
              This->background->mat.diffuse.g,
              This->background->mat.diffuse.b,
              This->background->mat.diffuse.a);
#endif
    }
    else
    {
        This->background = NULL;
        TRACE("Setting background to NULL\n");
    }

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::GetBackground
 *
 * Returns the material handle assigned to the background of the viewport
 *
 * Params:
 *  lphMat: Address to store the handle
 *  lpValid: is set to FALSE if no background is set, TRUE if one is set
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_GetBackground(IDirect3DViewport3 *iface,
                                    D3DMATERIALHANDLE *lphMat,
                                    BOOL *lpValid)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p)->(%p,%p)\n", This, lphMat, lpValid);

    if(lpValid)
    {
        *lpValid = This->background != NULL;
    }
    if(lphMat)
    {
        if(This->background)
        {
            *lphMat = This->background->Handle;
        }
        else
        {
            *lphMat = 0;
        }
    }

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::SetBackgroundDepth
 *
 * Sets a surface that represents the background depth. It's contents are
 * used to set the depth buffer in IDirect3DViewport3::Clear
 *
 * Params:
 *  lpDDSurface: Surface to set
 *
 * Returns: D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_SetBackgroundDepth(IDirect3DViewport3 *iface,
                                         IDirectDrawSurface *lpDDSurface)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p)->(%p): stub!\n", This, lpDDSurface);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::GetBackgroundDepth
 *
 * Returns the surface that represents the depth field
 *
 * Params:
 *  lplpDDSurface: Address to store the interface pointer
 *  lpValid: Set to TRUE if a depth is asigned, FALSE otherwise
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *  (DDERR_INVALIDPARAMS if DDSurface of Valid is NULL)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_GetBackgroundDepth(IDirect3DViewport3 *iface,
                                         IDirectDrawSurface **lplpDDSurface,
                                         LPBOOL lpValid)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p)->(%p,%p): stub!\n", This, lplpDDSurface, lpValid);
    return DD_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::Clear
 *
 * Clears the render target and / or the z buffer
 *
 * Params:
 *  dwCount: The amount of rectangles to clear. If 0, the whole buffer is
 *           cleared
 *  lpRects: Pointer to the array of rectangles. If NULL, Count must be 0
 *  dwFlags: D3DCLEAR_ZBUFFER and / or D3DCLEAR_TARGET
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_VIEWPORTHASNODEVICE if there's no active device
 *  The return value of IDirect3DDevice7::Clear
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_Clear(IDirect3DViewport3 *iface,
                            DWORD dwCount, 
                            D3DRECT *lpRects,
                            DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    DWORD color = 0x00000000;
    
    TRACE("(%p/%p)->(%08x,%p,%08x)\n", This, iface, dwCount, lpRects, dwFlags);
    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device !\n");
	return D3DERR_VIEWPORTHASNODEVICE;
    }
    if (dwFlags & D3DCLEAR_TARGET) {
        if (This->background == NULL) {
	    ERR(" Trying to clear the color buffer without background material !\n");
	} else {
	    color = 
#ifndef WINE_NATIVEWIN32
	      ((int) ((This->background->mat.u.diffuse.u1.r) * 255) << 16) |
	      ((int) ((This->background->mat.u.diffuse.u2.g) * 255) <<  8) |
	      ((int) ((This->background->mat.u.diffuse.u3.b) * 255) <<  0) |
	      ((int) ((This->background->mat.u.diffuse.u4.a) * 255) << 24);
#else
	      ((int) ((This->background->mat.diffuse.r) * 255) << 16) |
	      ((int) ((This->background->mat.diffuse.g) * 255) <<  8) |
	      ((int) ((This->background->mat.diffuse.b) * 255) <<  0) |
	      ((int) ((This->background->mat.diffuse.a) * 255) << 24);
#endif
	}
    }

    return IDirect3DDevice7_Clear(ICOM_INTERFACE(This->active_device, IDirect3DDevice7),
                                  dwCount,
                                  lpRects,
                                  dwFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET),
                                  color,
                                  1.0,
                                  0x00000000);
}

/*****************************************************************************
 * IDirect3DViewport3::AddLight
 *
 * Adds an light to the viewport
 *
 * Params:
 *  lpDirect3DLight: Interface of the light to add
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Direct3DLight is NULL
 *  DDERR_INVALIDPARAMS if there are 8 lights or more
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_AddLight(IDirect3DViewport3 *iface,
                               IDirect3DLight *lpDirect3DLight)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    IDirect3DLightImpl *lpDirect3DLightImpl = ICOM_OBJECT(IDirect3DLightImpl, IDirect3DLight, lpDirect3DLight);
    DWORD i = 0;
    DWORD map = This->map_lights;
    
    TRACE("(%p)->(%p)\n", This, lpDirect3DLight);
    
    if (This->num_lights >= 8)
        return DDERR_INVALIDPARAMS;

    /* Find a light number and update both light and viewports objects accordingly */
    while(map&1) {
        map>>=1;
	i++;
    }
    lpDirect3DLightImpl->dwLightIndex = i;
    This->num_lights++;
    This->map_lights |= 1<<i;

    /* Add the light in the 'linked' chain */
    lpDirect3DLightImpl->next = This->lights;
    This->lights = lpDirect3DLightImpl;

    /* Attach the light to the viewport */
    lpDirect3DLightImpl->active_viewport = This;
    
    /* If active, activate the light */
    if (This->active_device != NULL) {
        lpDirect3DLightImpl->activate(lpDirect3DLightImpl);
    }
    
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::DeleteLight
 *
 * Deletes a light from the viewports' light list
 *
 * Params:
 *  lpDirect3DLight: Light to delete
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if the light wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_DeleteLight(IDirect3DViewport3 *iface,
                                  IDirect3DLight *lpDirect3DLight)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    IDirect3DLightImpl *lpDirect3DLightImpl = ICOM_OBJECT(IDirect3DLightImpl, IDirect3DLight, lpDirect3DLight);
    IDirect3DLightImpl *cur_light, *prev_light = NULL;
    
    TRACE("(%p)->(%p)\n", This, lpDirect3DLight);
    cur_light = This->lights;
    while (cur_light != NULL) {
        if (cur_light == lpDirect3DLightImpl) {
	    lpDirect3DLightImpl->desactivate(lpDirect3DLightImpl);
	    if (prev_light == NULL) This->lights = cur_light->next;
	    else prev_light->next = cur_light->next;
	    /* Detach the light to the viewport */
	    cur_light->active_viewport = NULL;
	    This->num_lights--;
	    This->map_lights &= ~(1<<lpDirect3DLightImpl->dwLightIndex);
	    return D3D_OK;
	}
	prev_light = cur_light;
	cur_light = cur_light->next;
    }
    return DDERR_INVALIDPARAMS;
}

/*****************************************************************************
 * IDirect3DViewport::NextLight
 *
 * Enumerates the lights associated with the viewport
 *
 * Params:
 *  lpDirect3DLight: Light to start with
 *  lplpDirect3DLight: Address to store the successor to
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_NextLight(IDirect3DViewport3 *iface,
                                IDirect3DLight *lpDirect3DLight,
                                IDirect3DLight **lplpDirect3DLight,
                                DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p)->(%p,%p,%08x): stub!\n", This, lpDirect3DLight, lplpDirect3DLight, dwFlags);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport2 Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DViewport3::GetViewport2
 *
 * Returns the currently set viewport in a D3DVIEWPORT2 structure.
 * Similar to IDirect3DViewport3::GetViewport
 *
 * Params:
 *  lpData: Pointer to the structure to fill
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if the viewport was set with
 *                      IDirect3DViewport3::SetViewport
 *  DDERR_INVALIDPARAMS if Data is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_GetViewport2(IDirect3DViewport3 *iface,
                                   D3DVIEWPORT2 *lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    DWORD dwSize;
    TRACE("(%p)->(%p)\n", This, lpData);
    if (This->use_vp2 != 1) {
        ERR("  Requesting to get a D3DVIEWPORT2 struct where a D3DVIEWPORT was set !\n");
	return DDERR_INVALIDPARAMS;
    }
    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    memcpy(lpData, &(This->viewports.vp2), dwSize);

    if (TRACE_ON(d3d7)) {
        TRACE("  returning D3DVIEWPORT2 :\n");
	_dump_D3DVIEWPORT2(lpData);
    }
    
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::SetViewport2
 *
 * Sets the viewport from a D3DVIEWPORT2 structure
 *
 * Params:
 *  lpData: Viewport to set
 *
 * Returns:
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_SetViewport2(IDirect3DViewport3 *iface,
                                   D3DVIEWPORT2 *lpData)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    LPDIRECT3DVIEWPORT3 current_viewport;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(d3d7)) {
        TRACE("  getting D3DVIEWPORT2 :\n");
	_dump_D3DVIEWPORT2(lpData);
    }

    This->use_vp2 = 1;
    memset(&(This->viewports.vp2), 0, sizeof(This->viewports.vp2));
    memcpy(&(This->viewports.vp2), lpData, lpData->dwSize);

    if (This->active_device) {
      IDirect3DDevice3_GetCurrentViewport(ICOM_INTERFACE(This->active_device, IDirect3DDevice3), &current_viewport);
      if (ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, current_viewport) == This)
        This->activate(This);
      IDirect3DViewport3_Release(current_viewport);
    }

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3 Methods.
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DViewport3::SetBackgroundDepth2
 *
 * Sets a IDirectDrawSurface4 surface as the background depth surface
 *
 * Params:
 *  lpDDS: Surface to set
 *
 * Returns:
 *  D3D_OK, because it's stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_SetBackgroundDepth2(IDirect3DViewport3 *iface,
                                          IDirectDrawSurface4 *lpDDS)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p)->(%p): stub!\n", This, lpDDS);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::GetBackgroundDepth2
 *
 * Returns the IDirect3DSurface4 interface to the background depth surface
 *
 * Params:
 *  lplpDDS: Address to store the interface pointer at
 *  lpValid: Set to true if a surface is assigned
 *
 * Returns:
 *  D3D_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_GetBackgroundDepth2(IDirect3DViewport3 *iface,
                                          IDirectDrawSurface4 **lplpDDS,
                                          BOOL *lpValid)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lplpDDS, lpValid);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::Clear2
 *
 * Another clearing method
 *
 * Params:
 *  Count: Number of rectangles to clear
 *  Rects: Rectangle array to clear
 *  Flags: Some flags :)
 *  Color: Color to fill the render target with
 *  Z: Value to fill the depth buffer with
 *  Stencil: Value to fill the stencil bits with
 *
 * Returns:
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_Clear2(IDirect3DViewport3 *iface,
                             DWORD dwCount,
                             LPD3DRECT lpRects,
                             DWORD dwFlags,
                             DWORD dwColor,
                             D3DVALUE dvZ,
                             DWORD dwStencil)
{
    ICOM_THIS_FROM(IDirect3DViewportImpl, IDirect3DViewport3, iface);
    TRACE("(%p)->(%08x,%p,%08x,%08x,%f,%08x)\n", This, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device !\n");
	return D3DERR_VIEWPORTHASNODEVICE;
    }
    return IDirect3DDevice7_Clear(ICOM_INTERFACE(This->active_device, IDirect3DDevice7),
                                  dwCount,
                                  lpRects,
                                  dwFlags,
                                  dwColor,
                                  dvZ,
                                  dwStencil);
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/

const IDirect3DViewport3Vtbl IDirect3DViewport3_Vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DViewportImpl_QueryInterface,
    IDirect3DViewportImpl_AddRef,
    IDirect3DViewportImpl_Release,
    /*** IDirect3DViewport Methods */
    IDirect3DViewportImpl_Initialize,
    IDirect3DViewportImpl_GetViewport,
    IDirect3DViewportImpl_SetViewport,
    IDirect3DViewportImpl_TransformVertices,
    IDirect3DViewportImpl_LightElements,
    IDirect3DViewportImpl_SetBackground,
    IDirect3DViewportImpl_GetBackground,
    IDirect3DViewportImpl_SetBackgroundDepth,
    IDirect3DViewportImpl_GetBackgroundDepth,
    IDirect3DViewportImpl_Clear,
    IDirect3DViewportImpl_AddLight,
    IDirect3DViewportImpl_DeleteLight,
    IDirect3DViewportImpl_NextLight,
    /*** IDirect3DViewport2 Methods ***/
    IDirect3DViewportImpl_GetViewport2,
    IDirect3DViewportImpl_SetViewport2,
    /*** IDirect3DViewport3 Methods ***/
    IDirect3DViewportImpl_SetBackgroundDepth2,
    IDirect3DViewportImpl_GetBackgroundDepth2,
    IDirect3DViewportImpl_Clear2,
};
