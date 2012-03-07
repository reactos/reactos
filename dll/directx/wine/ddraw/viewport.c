/* Direct3D Viewport
 * Copyright (c) 1998 Lionel ULMER
 * Copyright (c) 2006-2007 Stefan DÖSINGER
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * Helper functions
 *****************************************************************************/

static void update_clip_space(IDirect3DDeviceImpl *device,
        struct wined3d_vec3 *scale, struct wined3d_vec3 *offset)
{
    D3DMATRIX clip_space =
    {
        scale->x,  0.0f,      0.0f,      0.0f,
        0.0f,      scale->y,  0.0f,      0.0f,
        0.0f,      0.0f,      scale->z,  0.0f,
        offset->x, offset->y, offset->z, 1.0f,
    };
    D3DMATRIX projection;
    HRESULT hr;

    multiply_matrix(&projection, &clip_space, &device->legacy_projection);
    hr = wined3d_device_set_transform(device->wined3d_device,
            WINED3D_TS_PROJECTION, (struct wined3d_matrix *)&projection);
    if (SUCCEEDED(hr))
        device->legacy_clipspace = clip_space;
}

/*****************************************************************************
 * viewport_activate
 *
 * activates the viewport using IDirect3DDevice7::SetViewport
 *
 *****************************************************************************/
void viewport_activate(IDirect3DViewportImpl *This, BOOL ignore_lights)
{
    struct wined3d_vec3 scale, offset;
    D3DVIEWPORT7 vp;

    if (!ignore_lights)
    {
        IDirect3DLightImpl *light;

        /* Activate all the lights associated with this context */
        LIST_FOR_EACH_ENTRY(light, &This->light_list, IDirect3DLightImpl, entry)
        {
            light_activate(light);
        }
    }

    /* And copy the values in the structure used by the device */
    if (This->use_vp2)
    {
        vp.dwX = This->viewports.vp2.dwX;
        vp.dwY = This->viewports.vp2.dwY;
        vp.dwHeight = This->viewports.vp2.dwHeight;
        vp.dwWidth = This->viewports.vp2.dwWidth;
        vp.dvMinZ = 0.0f;
        vp.dvMaxZ = 1.0f;

        scale.x = 2.0f / This->viewports.vp2.dvClipWidth;
        scale.y = 2.0f / This->viewports.vp2.dvClipHeight;
        scale.z = 1.0f / (This->viewports.vp2.dvMaxZ - This->viewports.vp2.dvMinZ);
        offset.x = -2.0f * This->viewports.vp2.dvClipX / This->viewports.vp2.dvClipWidth - 1.0f;
        offset.y = -2.0f * This->viewports.vp2.dvClipY / This->viewports.vp2.dvClipHeight + 1.0f;
        offset.z = -This->viewports.vp2.dvMinZ / (This->viewports.vp2.dvMaxZ - This->viewports.vp2.dvMinZ);
    }
    else
    {
        vp.dwX = This->viewports.vp1.dwX;
        vp.dwY = This->viewports.vp1.dwY;
        vp.dwHeight = This->viewports.vp1.dwHeight;
        vp.dwWidth = This->viewports.vp1.dwWidth;
        vp.dvMinZ = 0.0f;
        vp.dvMaxZ = 1.0f;

        scale.x = 2.0f * This->viewports.vp1.dvScaleX / This->viewports.vp1.dwWidth;
        scale.y = 2.0f * This->viewports.vp1.dvScaleY / This->viewports.vp1.dwHeight;
        scale.z = 1.0f;
        offset.x = 0.0f;
        offset.y = 0.0f;
        offset.z = 0.0f;
    }

    update_clip_space(This->active_device, &scale, &offset);
    IDirect3DDevice7_SetViewport(&This->active_device->IDirect3DDevice7_iface, &vp);
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

static inline IDirect3DViewportImpl *impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DViewportImpl, IDirect3DViewport3_iface);
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
static HRESULT WINAPI IDirect3DViewportImpl_QueryInterface(IDirect3DViewport3 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(&IID_IDirect3DViewport3, riid)
            || IsEqualGUID(&IID_IDirect3DViewport2, riid)
            || IsEqualGUID(&IID_IDirect3DViewport, riid)
            || IsEqualGUID(&IID_IUnknown, riid))
    {
        IDirect3DViewport3_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

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
    TRACE("iface %p, d3d %p.\n", iface, Direct3D);

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    DWORD dwSize;

    TRACE("iface %p, data %p.\n", iface, lpData);

    wined3d_mutex_lock();

    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    if (!This->use_vp2)
        memcpy(lpData, &(This->viewports.vp1), dwSize);
    else {
        D3DVIEWPORT vp1;
        vp1.dwSize = sizeof(vp1);
        vp1.dwX = This->viewports.vp2.dwX;
        vp1.dwY = This->viewports.vp2.dwY;
        vp1.dwWidth = This->viewports.vp2.dwWidth;
        vp1.dwHeight = This->viewports.vp2.dwHeight;
        vp1.dvMaxX = 0.0;
        vp1.dvMaxY = 0.0;
        vp1.dvScaleX = 0.0;
        vp1.dvScaleY = 0.0;
        vp1.dvMinZ = This->viewports.vp2.dvMinZ;
        vp1.dvMaxZ = This->viewports.vp2.dvMaxZ;
        memcpy(lpData, &vp1, dwSize);
    }

    if (TRACE_ON(ddraw))
    {
        TRACE("  returning D3DVIEWPORT :\n");
        _dump_D3DVIEWPORT(lpData);
    }

    wined3d_mutex_unlock();

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    IDirect3DViewport3 *current_viewport;

    TRACE("iface %p, data %p.\n", iface, lpData);

    if (TRACE_ON(ddraw))
    {
        TRACE("  getting D3DVIEWPORT :\n");
        _dump_D3DVIEWPORT(lpData);
    }

    wined3d_mutex_lock();

    This->use_vp2 = 0;
    memset(&(This->viewports.vp1), 0, sizeof(This->viewports.vp1));
    memcpy(&(This->viewports.vp1), lpData, lpData->dwSize);

    /* Tests on two games show that these values are never used properly so override
       them with proper ones :-)
    */
    This->viewports.vp1.dvMinZ = 0.0;
    This->viewports.vp1.dvMaxZ = 1.0;

    if (This->active_device) {
        IDirect3DDevice3 *d3d_device3 = &This->active_device->IDirect3DDevice3_iface;
        IDirect3DDevice3_GetCurrentViewport(d3d_device3, &current_viewport);
        if (current_viewport)
        {
            if (current_viewport == iface) viewport_activate(This, FALSE);
            IDirect3DViewport3_Release(current_viewport);
        }
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::TransformVertices
 *
 * Transforms vertices by the transformation matrix.
 *
 * This function is pretty similar to IDirect3DVertexBuffer7::ProcessVertices,
 * so it's tempting to forward it to there. However, there are some
 * tiny differences. First, the lpOffscreen flag that is reported back,
 * then there is the homogeneous vertex that is generated. Also there's a lack
 * of FVFs, but still a custom stride. Last, the d3d1 - d3d3 viewport has some
 * settings (scale) that d3d7 and wined3d do not have. All in all wrapping to
 * ProcessVertices doesn't pay of in terms of wrapper code needed and code
 * reused.
 *
 * Params:
 *  dwVertexCount: The number of vertices to be transformed
 *  lpData: Pointer to the vertex data
 *  dwFlags: D3DTRANSFORM_CLIPPED or D3DTRANSFORM_UNCLIPPED
 *  lpOffScreen: Set to the clipping plane clipping the vertex, if only one
 *               vertex is transformed and clipping is on. 0 otherwise
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_VIEWPORTHASNODEVICE if the viewport is not assigned to a device
 *  DDERR_INVALIDPARAMS if no clipping flag is specified
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DViewportImpl_TransformVertices(IDirect3DViewport3 *iface,
                                        DWORD dwVertexCount,
                                        D3DTRANSFORMDATA *lpData,
                                        DWORD dwFlags,
                                        DWORD *lpOffScreen)
{
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    D3DMATRIX view_mat, world_mat, mat;
    float *in;
    float *out;
    float x, y, z, w;
    unsigned int i;
    D3DVIEWPORT vp = This->viewports.vp1;
    D3DHVERTEX *outH;

    TRACE("iface %p, vertex_count %u, vertex_data %p, flags %#x, clip_plane %p.\n",
            iface, dwVertexCount, lpData, dwFlags, lpOffScreen);

    /* Tests on windows show that Windows crashes when this occurs,
     * so don't return the (intuitive) return value
    if(!This->active_device)
    {
        WARN("No device active, returning D3DERR_VIEWPORTHASNODEVICE\n");
        return D3DERR_VIEWPORTHASNODEVICE;
    }
     */

    if(!(dwFlags & (D3DTRANSFORM_UNCLIPPED | D3DTRANSFORM_CLIPPED)))
    {
        WARN("No clipping flag passed, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }


    wined3d_mutex_lock();
    wined3d_device_get_transform(This->active_device->wined3d_device,
            D3DTRANSFORMSTATE_VIEW, (struct wined3d_matrix *)&view_mat);
    wined3d_device_get_transform(This->active_device->wined3d_device,
            WINED3D_TS_WORLD_MATRIX(0), (struct wined3d_matrix *)&world_mat);
    multiply_matrix(&mat, &view_mat, &world_mat);
    multiply_matrix(&mat, &This->active_device->legacy_projection, &mat);

    in = lpData->lpIn;
    out = lpData->lpOut;
    outH = lpData->lpHOut;
    for(i = 0; i < dwVertexCount; i++)
    {
        x = (in[0] * mat._11) + (in[1] * mat._21) + (in[2] * mat._31) + (1.0 * mat._41);
        y = (in[0] * mat._12) + (in[1] * mat._22) + (in[2] * mat._32) + (1.0 * mat._42);
        z = (in[0] * mat._13) + (in[1] * mat._23) + (in[2] * mat._33) + (1.0 * mat._43);
        w = (in[0] * mat._14) + (in[1] * mat._24) + (in[2] * mat._34) + (1.0 * mat._44);

        if(dwFlags & D3DTRANSFORM_CLIPPED)
        {
            /* If clipping is enabled, Windows assumes that outH is
             * a valid pointer
             */
            outH[i].u1.hx = x; outH[i].u2.hy = y; outH[i].u3.hz = z;

            outH[i].dwFlags = 0;
            if(x * vp.dvScaleX > ((float) vp.dwWidth * 0.5))
                outH[i].dwFlags |= D3DCLIP_RIGHT;
            if(x * vp.dvScaleX <= -((float) vp.dwWidth) * 0.5)
                outH[i].dwFlags |= D3DCLIP_LEFT;
            if(y * vp.dvScaleY > ((float) vp.dwHeight * 0.5))
                outH[i].dwFlags |= D3DCLIP_TOP;
            if(y * vp.dvScaleY <= -((float) vp.dwHeight) * 0.5)
                outH[i].dwFlags |= D3DCLIP_BOTTOM;
            if(z < 0.0)
                outH[i].dwFlags |= D3DCLIP_FRONT;
            if(z > 1.0)
                outH[i].dwFlags |= D3DCLIP_BACK;

            if(outH[i].dwFlags)
            {
                /* Looks like native just drops the vertex, leaves whatever data
                 * it has in the output buffer and goes on with the next vertex.
                 * The exact scheme hasn't been figured out yet, but windows
                 * definitely writes something there.
                 */
                out[0] = x;
                out[1] = y;
                out[2] = z;
                out[3] = w;
                in = (float *) ((char *) in + lpData->dwInSize);
                out = (float *) ((char *) out + lpData->dwOutSize);
                continue;
            }
        }

        w = 1 / w;
        x *= w; y *= w; z *= w;

        out[0] = vp.dwWidth / 2 + vp.dwX + x * vp.dvScaleX;
        out[1] = vp.dwHeight / 2 + vp.dwY - y * vp.dvScaleY;
        out[2] = z;
        out[3] = w;
        in = (float *) ((char *) in + lpData->dwInSize);
        out = (float *) ((char *) out + lpData->dwOutSize);
    }

    /* According to the d3d test, the offscreen flag is set only
     * if exactly one vertex is transformed. Its not documented,
     * but the test shows that the lpOffscreen flag is set to the
     * flag combination of clipping planes that clips the vertex.
     *
     * If clipping is requested, Windows assumes that the offscreen
     * param is a valid pointer.
     */
    if(dwVertexCount == 1 && dwFlags & D3DTRANSFORM_CLIPPED)
    {
        *lpOffScreen = outH[0].dwFlags;
    }
    else if(*lpOffScreen)
    {
        *lpOffScreen = 0;
    }
    wined3d_mutex_unlock();

    TRACE("All done\n");
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
    TRACE("iface %p, element_count %u, data %p.\n", iface, dwElementCount, lpData);

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    IDirect3DMaterialImpl *m;

    TRACE("iface %p, material %#x.\n", iface, hMat);

    wined3d_mutex_lock();

    if (!hMat)
    {
        This->background = NULL;
        TRACE("Setting background to NULL\n");
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    m = ddraw_get_object(&This->ddraw->d3ddevice->handle_table, hMat - 1, DDRAW_HANDLE_MATERIAL);
    if (!m)
    {
        WARN("Invalid material handle.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    TRACE("Setting background color : %.8e %.8e %.8e %.8e.\n",
            m->mat.u.diffuse.u1.r, m->mat.u.diffuse.u2.g,
            m->mat.u.diffuse.u3.b, m->mat.u.diffuse.u4.a);
    This->background = m;

    wined3d_mutex_unlock();

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);

    TRACE("iface %p, material %p, valid %p.\n", iface, lphMat, lpValid);

    wined3d_mutex_lock();
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
    wined3d_mutex_unlock();

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
    FIXME("iface %p, surface %p stub!\n", iface, lpDDSurface);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::GetBackgroundDepth
 *
 * Returns the surface that represents the depth field
 *
 * Params:
 *  lplpDDSurface: Address to store the interface pointer
 *  lpValid: Set to TRUE if a depth is assigned, FALSE otherwise
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
    FIXME("iface %p, surface %p, valid %p stub!\n", iface, lplpDDSurface, lpValid);

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
static HRESULT WINAPI IDirect3DViewportImpl_Clear(IDirect3DViewport3 *iface,
        DWORD dwCount, D3DRECT *lpRects, DWORD dwFlags)
{
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    DWORD color = 0x00000000;
    HRESULT hr;
    IDirect3DViewport3 *current_viewport;
    IDirect3DDevice3 *d3d_device3;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x.\n", iface, dwCount, lpRects, dwFlags);

    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device !\n");
        return D3DERR_VIEWPORTHASNODEVICE;
    }
    d3d_device3 = &This->active_device->IDirect3DDevice3_iface;

    wined3d_mutex_lock();

    if (dwFlags & D3DCLEAR_TARGET) {
        if (This->background == NULL) {
            ERR(" Trying to clear the color buffer without background material !\n");
        }
        else
        {
            color = ((int)((This->background->mat.u.diffuse.u1.r) * 255) << 16)
                    | ((int) ((This->background->mat.u.diffuse.u2.g) * 255) <<  8)
                    | ((int) ((This->background->mat.u.diffuse.u3.b) * 255) <<  0)
                    | ((int) ((This->background->mat.u.diffuse.u4.a) * 255) << 24);
        }
    }

    /* Need to temporarily activate viewport to clear it. Previously active one will be restored
        afterwards. */
    viewport_activate(This, TRUE);

    hr = IDirect3DDevice7_Clear(&This->active_device->IDirect3DDevice7_iface, dwCount, lpRects,
            dwFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET), color, 1.0, 0x00000000);

    IDirect3DDevice3_GetCurrentViewport(d3d_device3, &current_viewport);
    if(current_viewport) {
        IDirect3DViewportImpl *vp = impl_from_IDirect3DViewport3(current_viewport);
        viewport_activate(vp, TRUE);
        IDirect3DViewport3_Release(current_viewport);
    }

    wined3d_mutex_unlock();

    return hr;
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
static HRESULT WINAPI IDirect3DViewportImpl_AddLight(IDirect3DViewport3 *iface,
        IDirect3DLight *lpDirect3DLight)
{
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    IDirect3DLightImpl *lpDirect3DLightImpl = unsafe_impl_from_IDirect3DLight(lpDirect3DLight);
    DWORD i = 0;
    DWORD map = This->map_lights;

    TRACE("iface %p, light %p.\n", iface, lpDirect3DLight);

    wined3d_mutex_lock();

    if (This->num_lights >= 8)
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* Find a light number and update both light and viewports objects accordingly */
    while (map & 1)
    {
        map >>= 1;
        ++i;
    }
    lpDirect3DLightImpl->dwLightIndex = i;
    This->num_lights++;
    This->map_lights |= 1<<i;

    /* Add the light in the 'linked' chain */
    list_add_head(&This->light_list, &lpDirect3DLightImpl->entry);
    IDirect3DLight_AddRef(lpDirect3DLight);

    /* Attach the light to the viewport */
    lpDirect3DLightImpl->active_viewport = This;

    /* If active, activate the light */
    if (This->active_device)
        light_activate(lpDirect3DLightImpl);

    wined3d_mutex_unlock();

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
static HRESULT WINAPI IDirect3DViewportImpl_DeleteLight(IDirect3DViewport3 *iface,
        IDirect3DLight *lpDirect3DLight)
{
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    IDirect3DLightImpl *l = unsafe_impl_from_IDirect3DLight(lpDirect3DLight);

    TRACE("iface %p, light %p.\n", iface, lpDirect3DLight);

    wined3d_mutex_lock();

    if (l->active_viewport != This)
    {
        WARN("Light %p active viewport is %p.\n", l, l->active_viewport);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    light_deactivate(l);
    list_remove(&l->entry);
    l->active_viewport = NULL;
    IDirect3DLight_Release(lpDirect3DLight);
    --This->num_lights;
    This->map_lights &= ~(1 << l->dwLightIndex);

    wined3d_mutex_unlock();

    return D3D_OK;
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
static HRESULT WINAPI IDirect3DViewportImpl_NextLight(IDirect3DViewport3 *iface,
        IDirect3DLight *lpDirect3DLight, IDirect3DLight **lplpDirect3DLight, DWORD dwFlags)
{
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    IDirect3DLightImpl *l = unsafe_impl_from_IDirect3DLight(lpDirect3DLight);
    struct list *entry;
    HRESULT hr;

    TRACE("iface %p, light %p, next_light %p, flags %#x.\n",
            iface, lpDirect3DLight, lplpDirect3DLight, dwFlags);

    if (!lplpDirect3DLight)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    switch (dwFlags)
    {
        case D3DNEXT_NEXT:
            if (!l || l->active_viewport != This)
            {
                if (l)
                    WARN("Light %p active viewport is %p.\n", l, l->active_viewport);
                entry = NULL;
            }
            else
                entry = list_next(&This->light_list, &l->entry);
            break;

        case D3DNEXT_HEAD:
            entry = list_head(&This->light_list);
            break;

        case D3DNEXT_TAIL:
            entry = list_tail(&This->light_list);
            break;

        default:
            entry = NULL;
            WARN("Invalid flags %#x.\n", dwFlags);
            break;
    }

    if (entry)
    {
        *lplpDirect3DLight = (IDirect3DLight *)LIST_ENTRY(entry, IDirect3DLightImpl, entry);
        IDirect3DLight_AddRef(*lplpDirect3DLight);
        hr = D3D_OK;
    }
    else
    {
        *lplpDirect3DLight = NULL;
        hr = DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_unlock();

    return hr;
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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    DWORD dwSize;

    TRACE("iface %p, data %p.\n", iface, lpData);

    wined3d_mutex_lock();
    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    if (This->use_vp2)
        memcpy(lpData, &(This->viewports.vp2), dwSize);
    else {
        D3DVIEWPORT2 vp2;
        vp2.dwSize = sizeof(vp2);
        vp2.dwX = This->viewports.vp1.dwX;
        vp2.dwY = This->viewports.vp1.dwY;
        vp2.dwWidth = This->viewports.vp1.dwWidth;
        vp2.dwHeight = This->viewports.vp1.dwHeight;
        vp2.dvClipX = 0.0;
        vp2.dvClipY = 0.0;
        vp2.dvClipWidth = 0.0;
        vp2.dvClipHeight = 0.0;
        vp2.dvMinZ = This->viewports.vp1.dvMinZ;
        vp2.dvMaxZ = This->viewports.vp1.dvMaxZ;
        memcpy(lpData, &vp2, dwSize);
    }

    if (TRACE_ON(ddraw))
    {
        TRACE("  returning D3DVIEWPORT2 :\n");
        _dump_D3DVIEWPORT2(lpData);
    }

    wined3d_mutex_unlock();

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    IDirect3DViewport3 *current_viewport;

    TRACE("iface %p, data %p.\n", iface, lpData);

    if (TRACE_ON(ddraw))
    {
        TRACE("  getting D3DVIEWPORT2 :\n");
        _dump_D3DVIEWPORT2(lpData);
    }

    wined3d_mutex_lock();

    This->use_vp2 = 1;
    memset(&(This->viewports.vp2), 0, sizeof(This->viewports.vp2));
    memcpy(&(This->viewports.vp2), lpData, lpData->dwSize);

    if (This->active_device) {
        IDirect3DDevice3 *d3d_device3 = &This->active_device->IDirect3DDevice3_iface;
        IDirect3DDevice3_GetCurrentViewport(d3d_device3, &current_viewport);
        if (current_viewport)
        {
            if (current_viewport == iface) viewport_activate(This, FALSE);
            IDirect3DViewport3_Release(current_viewport);
        }
    }

    wined3d_mutex_unlock();

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
    FIXME("iface %p, surface %p stub!\n", iface, lpDDS);

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
    FIXME("iface %p, surface %p, valid %p stub!\n", iface, lplpDDS, lpValid);

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
    IDirect3DViewportImpl *This = impl_from_IDirect3DViewport3(iface);
    HRESULT hr;
    IDirect3DViewport3 *current_viewport;
    IDirect3DDevice3 *d3d_device3;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x, color 0x%08x, depth %.8e, stencil %u.\n",
            iface, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);

    wined3d_mutex_lock();

    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device !\n");
        wined3d_mutex_unlock();
        return D3DERR_VIEWPORTHASNODEVICE;
    }
    d3d_device3 = &This->active_device->IDirect3DDevice3_iface;
    /* Need to temporarily activate viewport to clear it. Previously active
     * one will be restored afterwards. */
    viewport_activate(This, TRUE);

    hr = IDirect3DDevice7_Clear(&This->active_device->IDirect3DDevice7_iface,
            dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
    IDirect3DDevice3_GetCurrentViewport(d3d_device3, &current_viewport);
    if(current_viewport) {
        IDirect3DViewportImpl *vp = impl_from_IDirect3DViewport3(current_viewport);
        viewport_activate(vp, TRUE);
        IDirect3DViewport3_Release(current_viewport);
    }

    wined3d_mutex_unlock();

    return hr;
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/

static const struct IDirect3DViewport3Vtbl d3d_viewport_vtbl =
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

IDirect3DViewportImpl *unsafe_impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_viewport_vtbl);
    return CONTAINING_RECORD(iface, IDirect3DViewportImpl, IDirect3DViewport3_iface);
}

IDirect3DViewportImpl *unsafe_impl_from_IDirect3DViewport2(IDirect3DViewport2 *iface)
{
    /* IDirect3DViewport and IDirect3DViewport3 use the same iface. */
    if (!iface) return NULL;
    assert(iface->lpVtbl == (IDirect3DViewport2Vtbl *)&d3d_viewport_vtbl);
    return CONTAINING_RECORD(iface, IDirect3DViewportImpl, IDirect3DViewport3_iface);
}

IDirect3DViewportImpl *unsafe_impl_from_IDirect3DViewport(IDirect3DViewport *iface)
{
    /* IDirect3DViewport and IDirect3DViewport3 use the same iface. */
    if (!iface) return NULL;
    assert(iface->lpVtbl == (IDirect3DViewportVtbl *)&d3d_viewport_vtbl);
    return CONTAINING_RECORD(iface, IDirect3DViewportImpl, IDirect3DViewport3_iface);
}

void d3d_viewport_init(IDirect3DViewportImpl *viewport, IDirectDrawImpl *ddraw)
{
    viewport->IDirect3DViewport3_iface.lpVtbl = &d3d_viewport_vtbl;
    viewport->ref = 1;
    viewport->ddraw = ddraw;
    viewport->use_vp2 = 0xff;
    list_init(&viewport->light_list);
}
