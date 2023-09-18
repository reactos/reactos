/* Direct3D Viewport
 * Copyright (c) 1998 Lionel ULMER
 * Copyright (c) 2006-2007 Stefan DÖSINGER
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * Helper functions
 *****************************************************************************/

static void update_clip_space(struct d3d_device *device,
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

    multiply_matrix(&projection, &clip_space, &device->legacy_projection);
    wined3d_device_set_transform(device->wined3d_device,
            WINED3D_TS_PROJECTION, (struct wined3d_matrix *)&projection);
    device->legacy_clipspace = clip_space;
}

/*****************************************************************************
 * viewport_activate
 *
 * activates the viewport using IDirect3DDevice7::SetViewport
 *
 *****************************************************************************/
void viewport_activate(struct d3d_viewport *This, BOOL ignore_lights)
{
    struct wined3d_vec3 scale, offset;
    D3DVIEWPORT7 vp;

    if (!ignore_lights)
    {
        struct d3d_light *light;

        /* Activate all the lights associated with this context */
        LIST_FOR_EACH_ENTRY(light, &This->light_list, struct d3d_light, entry)
        {
            light_activate(light);
        }
    }

    if (This->version == DDRAW_VIEWPORT_VERSION_NONE)
    {
        TRACE("Viewport data was not set.\n");
        return;
    }

    /* And copy the values in the structure used by the device */
    if (This->version == DDRAW_VIEWPORT_VERSION_2)
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

void viewport_deactivate(struct d3d_viewport *viewport)
{
    struct d3d_light *light;

    LIST_FOR_EACH_ENTRY(light, &viewport->light_list, struct d3d_light, entry)
    {
        light_deactivate(light);
    }
}

void viewport_alloc_active_light_index(struct d3d_light *light)
{
    struct d3d_viewport *vp = light->active_viewport;
    unsigned int i;
    DWORD map;

    TRACE("vp %p, light %p, index %u, active_lights_count %u.\n",
            vp, light, light->active_light_index, vp->active_lights_count);

    if (light->active_light_index)
        return;

    if (vp->active_lights_count >= DDRAW_MAX_ACTIVE_LIGHTS)
    {
        struct d3d_light *l;

        LIST_FOR_EACH_ENTRY(l, &vp->light_list, struct d3d_light, entry)
        {
            if (l->active_light_index)
            {
                WARN("Too many active lights, viewport %p, light %p, deactivating %p.\n", vp, light, l);
                light_deactivate(l);

                /* Recycle active lights in a FIFO way. */
                list_remove(&light->entry);
                list_add_tail(&vp->light_list, &light->entry);
                break;
            }
        }
    }

    map = ~vp->map_lights;
    assert(vp->active_lights_count < DDRAW_MAX_ACTIVE_LIGHTS && map);
    i = wined3d_bit_scan(&map);
    light->active_light_index = i + 1;
    ++vp->active_lights_count;
    vp->map_lights |= 1u << i;
}

void viewport_free_active_light_index(struct d3d_light *light)
{
    struct d3d_viewport *vp = light->active_viewport;

    TRACE("vp %p, light %p, index %u, active_lights_count %u, map_lights %#x.\n",
            vp, light, light->active_light_index, vp->active_lights_count, vp->map_lights);

    if (!light->active_light_index)
        return;

    assert(vp->map_lights & (1u << (light->active_light_index - 1)));

    --vp->active_lights_count;
    vp->map_lights &= ~(1u << (light->active_light_index - 1));
    light->active_light_index = 0;
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

static inline struct d3d_viewport *impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_viewport, IDirect3DViewport3_iface);
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
static HRESULT WINAPI d3d_viewport_QueryInterface(IDirect3DViewport3 *iface, REFIID riid, void **object)
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
static ULONG WINAPI d3d_viewport_AddRef(IDirect3DViewport3 *iface)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    ULONG ref = InterlockedIncrement(&viewport->ref);

    TRACE("%p increasing refcount to %u.\n", viewport, ref);

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
static ULONG WINAPI d3d_viewport_Release(IDirect3DViewport3 *iface)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    ULONG ref = InterlockedDecrement(&viewport->ref);

    TRACE("%p decreasing refcount to %u.\n", viewport, ref);

    if (!ref)
        heap_free(viewport);

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
static HRESULT WINAPI d3d_viewport_Initialize(IDirect3DViewport3 *iface, IDirect3D *d3d)
{
    TRACE("iface %p, d3d %p.\n", iface, d3d);

    return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI d3d_viewport_GetViewport(IDirect3DViewport3 *iface, D3DVIEWPORT *vp)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    DWORD size;

    TRACE("iface %p, vp %p.\n", iface, vp);

    if (!vp)
        return DDERR_INVALIDPARAMS;

    if (viewport->version == DDRAW_VIEWPORT_VERSION_NONE)
    {
        WARN("Viewport data was not set.\n");
        return D3DERR_VIEWPORTDATANOTSET;
    }

    wined3d_mutex_lock();

    size = vp->dwSize;
    if (viewport->version == DDRAW_VIEWPORT_VERSION_1)
    {
        memcpy(vp, &viewport->viewports.vp1, size);
    }
    else
    {
        D3DVIEWPORT vp1;

        vp1.dwSize = sizeof(vp1);
        vp1.dwX = viewport->viewports.vp2.dwX;
        vp1.dwY = viewport->viewports.vp2.dwY;
        vp1.dwWidth = viewport->viewports.vp2.dwWidth;
        vp1.dwHeight = viewport->viewports.vp2.dwHeight;
        vp1.dvMaxX = 0.0f;
        vp1.dvMaxY = 0.0f;
        vp1.dvScaleX = 0.0f;
        vp1.dvScaleY = 0.0f;
        vp1.dvMinZ = viewport->viewports.vp2.dvMinZ;
        vp1.dvMaxZ = viewport->viewports.vp2.dvMaxZ;
        memcpy(vp, &vp1, size);
    }

    if (TRACE_ON(ddraw))
    {
        TRACE("  returning D3DVIEWPORT :\n");
        _dump_D3DVIEWPORT(vp);
    }

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_viewport_SetViewport(IDirect3DViewport3 *iface, D3DVIEWPORT *vp)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    struct d3d_device *device = viewport->active_device;
    struct wined3d_sub_resource_desc rt_desc;
    struct wined3d_rendertarget_view *rtv;
    IDirect3DViewport3 *current_viewport;
    struct ddraw_surface *surface;

    TRACE("iface %p, vp %p.\n", iface, vp);

    if (!vp)
        return DDERR_INVALIDPARAMS;

    if (vp->dwSize != sizeof(*vp))
    {
        WARN("Invalid D3DVIEWPORT size %u.\n", vp->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    if (TRACE_ON(ddraw))
    {
        TRACE("  getting D3DVIEWPORT :\n");
        _dump_D3DVIEWPORT(vp);
    }

    if (!device)
    {
        WARN("Viewport not bound to a device, returning D3DERR_VIEWPORTHASNODEVICE.\n");
        return D3DERR_VIEWPORTHASNODEVICE;
    }

    wined3d_mutex_lock();

    if (device->version > 1)
    {
        if (!(rtv = wined3d_device_get_rendertarget_view(device->wined3d_device, 0)))
        {
            wined3d_mutex_unlock();
            return DDERR_INVALIDCAPS;
        }
        surface = wined3d_rendertarget_view_get_sub_resource_parent(rtv);
        wined3d_texture_get_sub_resource_desc(surface->wined3d_texture, surface->sub_resource_idx, &rt_desc);

        if (vp->dwX > rt_desc.width || vp->dwWidth > rt_desc.width - vp->dwX
            || vp->dwY > rt_desc.height || vp->dwHeight > rt_desc.height - vp->dwY)
        {
            WARN("Invalid viewport, returning DDERR_INVALIDPARAMS.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
    }

    viewport->version = DDRAW_VIEWPORT_VERSION_1;
    viewport->viewports.vp1 = *vp;

    /* Empirical testing on a couple of d3d1 games showed that these values
     * should be ignored. */
    viewport->viewports.vp1.dvMinZ = 0.0f;
    viewport->viewports.vp1.dvMaxZ = 1.0f;

    if (SUCCEEDED(IDirect3DDevice3_GetCurrentViewport(&device->IDirect3DDevice3_iface, &current_viewport)))
    {
        if (current_viewport == iface)
            viewport_activate(viewport, FALSE);
        IDirect3DViewport3_Release(current_viewport);
    }

    wined3d_mutex_unlock();

    return D3D_OK;
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
 *  data: Pointer to the vertex input / output data.
 *  dwFlags: D3DTRANSFORM_CLIPPED or D3DTRANSFORM_UNCLIPPED
 *  offscreen: Logical AND of the planes that clipped the vertices if clipping
 *          is on. 0 if clipping is off.
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_VIEWPORTHASNODEVICE if the viewport is not assigned to a device
 *  DDERR_INVALIDPARAMS if no clipping flag is specified
 *
 *****************************************************************************/
struct transform_vertices_vertex
{
    float x, y, z, w; /* w is unused in input data. */
    struct
    {
        DWORD p[4];
    } payload;
};

static HRESULT WINAPI d3d_viewport_TransformVertices(IDirect3DViewport3 *iface,
        DWORD dwVertexCount, D3DTRANSFORMDATA *data, DWORD dwFlags, DWORD *offscreen)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    D3DVIEWPORT vp = viewport->viewports.vp1;
    D3DMATRIX view_mat, world_mat, proj_mat, mat;
    struct transform_vertices_vertex *in, *out;
    float x, y, z, w;
    unsigned int i;
    D3DHVERTEX *outH;
    struct d3d_device *device = viewport->active_device;
    BOOL activate = device->current_viewport != viewport;

    TRACE("iface %p, vertex_count %u, data %p, flags %#x, offscreen %p.\n",
            iface, dwVertexCount, data, dwFlags, offscreen);

    /* Tests on windows show that Windows crashes when this occurs,
     * so don't return the (intuitive) return value
    if (!device)
    {
        WARN("No device active, returning D3DERR_VIEWPORTHASNODEVICE\n");
        return D3DERR_VIEWPORTHASNODEVICE;
    }
     */

    if (!data || data->dwSize != sizeof(*data))
    {
        WARN("Transform data is NULL or size is incorrect, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }
    if (!(dwFlags & (D3DTRANSFORM_UNCLIPPED | D3DTRANSFORM_CLIPPED)))
    {
        WARN("No clipping flag passed, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    if (activate)
        viewport_activate(viewport, TRUE);

    wined3d_device_get_transform(device->wined3d_device,
            D3DTRANSFORMSTATE_VIEW, (struct wined3d_matrix *)&view_mat);
    wined3d_device_get_transform(device->wined3d_device,
            WINED3D_TS_WORLD_MATRIX(0), (struct wined3d_matrix *)&world_mat);
    wined3d_device_get_transform(device->wined3d_device,
            WINED3D_TS_PROJECTION, (struct wined3d_matrix *)&proj_mat);
    multiply_matrix(&mat, &view_mat, &world_mat);
    multiply_matrix(&mat, &proj_mat, &mat);

    /* The pointer is not tested against NULL on Windows. */
    if (dwFlags & D3DTRANSFORM_CLIPPED)
        *offscreen = ~0U;
    else
        *offscreen = 0;

    outH = data->lpHOut;
    for(i = 0; i < dwVertexCount; i++)
    {
        in = (struct transform_vertices_vertex *)((char *)data->lpIn + data->dwInSize * i);
        out = (struct transform_vertices_vertex *)((char *)data->lpOut + data->dwOutSize * i);

        x = (in->x * mat._11) + (in->y * mat._21) + (in->z * mat._31) + mat._41;
        y = (in->x * mat._12) + (in->y * mat._22) + (in->z * mat._32) + mat._42;
        z = (in->x * mat._13) + (in->y * mat._23) + (in->z * mat._33) + mat._43;
        w = (in->x * mat._14) + (in->y * mat._24) + (in->z * mat._34) + mat._44;

        if(dwFlags & D3DTRANSFORM_CLIPPED)
        {
            /* If clipping is enabled, Windows assumes that outH is
             * a valid pointer. */
            outH[i].u1.hx = (x - device->legacy_clipspace._41 * w) / device->legacy_clipspace._11;
            outH[i].u2.hy = (y - device->legacy_clipspace._42 * w) / device->legacy_clipspace._22;
            outH[i].u3.hz = (z - device->legacy_clipspace._43 * w) / device->legacy_clipspace._33;

            outH[i].dwFlags = 0;
            if (x > w)
                outH[i].dwFlags |= D3DCLIP_RIGHT;
            if (x < -w)
                outH[i].dwFlags |= D3DCLIP_LEFT;
            if (y > w)
                outH[i].dwFlags |= D3DCLIP_TOP;
            if (y < -w)
                outH[i].dwFlags |= D3DCLIP_BOTTOM;
            if (z < 0.0f)
                outH[i].dwFlags |= D3DCLIP_FRONT;
            if (z > w)
                outH[i].dwFlags |= D3DCLIP_BACK;

            *offscreen &= outH[i].dwFlags;

            if(outH[i].dwFlags)
            {
                /* Looks like native just drops the vertex, leaves whatever data
                 * it has in the output buffer and goes on with the next vertex.
                 * The exact scheme hasn't been figured out yet, but windows
                 * definitely writes something there.
                 */
                out->x = x;
                out->y = y;
                out->z = z;
                out->w = w;
                continue;
            }
        }

        w = 1 / w;
        x *= w; y *= w; z *= w;

        out->x = (x + 1.0f) * vp.dwWidth * 0.5 + vp.dwX;
        out->y = (-y + 1.0f) * vp.dwHeight * 0.5 + vp.dwY;
        out->z = z;
        out->w = w;
        out->payload = in->payload;
    }

    if (activate && device->current_viewport)
        viewport_activate(device->current_viewport, TRUE);

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
static HRESULT WINAPI d3d_viewport_LightElements(IDirect3DViewport3 *iface,
        DWORD element_count, D3DLIGHTDATA *data)
{
    TRACE("iface %p, element_count %u, data %p.\n", iface, element_count, data);

    return DDERR_UNSUPPORTED;
}

static HRESULT WINAPI d3d_viewport_SetBackground(IDirect3DViewport3 *iface, D3DMATERIALHANDLE material)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    struct d3d_material *m;

    TRACE("iface %p, material %#x.\n", iface, material);

    wined3d_mutex_lock();

    if (!(m = ddraw_get_object(&viewport->ddraw->d3ddevice->handle_table, material - 1, DDRAW_HANDLE_MATERIAL)))
    {
        WARN("Invalid material handle %#x.\n", material);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    TRACE("Setting background color : %.8e %.8e %.8e %.8e.\n",
            m->mat.u.diffuse.u1.r, m->mat.u.diffuse.u2.g,
            m->mat.u.diffuse.u3.b, m->mat.u.diffuse.u4.a);
    viewport->background = m;

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
static HRESULT WINAPI d3d_viewport_GetBackground(IDirect3DViewport3 *iface,
        D3DMATERIALHANDLE *material, BOOL *valid)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);

    TRACE("iface %p, material %p, valid %p.\n", iface, material, valid);

    wined3d_mutex_lock();
    if (valid)
        *valid = !!viewport->background;
    if (material)
        *material = viewport->background ? viewport->background->Handle : 0;
    wined3d_mutex_unlock();

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DViewport3::SetBackgroundDepth
 *
 * Sets a surface that represents the background depth. Its contents are
 * used to set the depth buffer in IDirect3DViewport3::Clear
 *
 * Params:
 *  lpDDSurface: Surface to set
 *
 * Returns: D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_viewport_SetBackgroundDepth(IDirect3DViewport3 *iface, IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

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
static HRESULT WINAPI d3d_viewport_GetBackgroundDepth(IDirect3DViewport3 *iface,
        IDirectDrawSurface **surface, BOOL *valid)
{
    FIXME("iface %p, surface %p, valid %p stub!\n", iface, surface, valid);

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
static HRESULT WINAPI d3d_viewport_Clear(IDirect3DViewport3 *iface,
        DWORD rect_count, D3DRECT *rects, DWORD flags)
{
    struct d3d_viewport *This = impl_from_IDirect3DViewport3(iface);
    DWORD color = 0x00000000;
    HRESULT hr;
    IDirect3DViewport3 *current_viewport;
    IDirect3DDevice3 *d3d_device3;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x.\n", iface, rect_count, rects, flags);

    if (!rects || !rect_count)
    {
        WARN("rect_count = %u, rects = %p, ignoring clear\n", rect_count, rects);
        return D3D_OK;
    }

    if (This->active_device == NULL) {
        ERR(" Trying to clear a viewport not attached to a device!\n");
        return D3DERR_VIEWPORTHASNODEVICE;
    }
    d3d_device3 = &This->active_device->IDirect3DDevice3_iface;

    wined3d_mutex_lock();

    if (flags & D3DCLEAR_TARGET)
    {
        if (!This->background)
            WARN("No background material set.\n");
        else
            color = D3DRGBA(This->background->mat.u.diffuse.u1.r,
                    This->background->mat.u.diffuse.u2.g,
                    This->background->mat.u.diffuse.u3.b,
                    This->background->mat.u.diffuse.u4.a);
    }

    /* Need to temporarily activate the viewport to clear it. The previously
     * active one will be restored afterwards. */
    viewport_activate(This, TRUE);

    hr = IDirect3DDevice7_Clear(&This->active_device->IDirect3DDevice7_iface, rect_count, rects,
            flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET), color, 1.0, 0x00000000);

    if (SUCCEEDED(IDirect3DDevice3_GetCurrentViewport(d3d_device3, &current_viewport)))
    {
        struct d3d_viewport *vp = impl_from_IDirect3DViewport3(current_viewport);
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
static HRESULT WINAPI d3d_viewport_AddLight(IDirect3DViewport3 *viewport, IDirect3DLight *light)
{
    struct d3d_light *light_impl = unsafe_impl_from_IDirect3DLight(light);
    struct d3d_viewport *vp = impl_from_IDirect3DViewport3(viewport);

    TRACE("viewport %p, light %p.\n", viewport, light);

    wined3d_mutex_lock();

    if (light_impl->active_viewport)
    {
        wined3d_mutex_unlock();
        WARN("Light %p is active in viewport %p.\n", light_impl, light_impl->active_viewport);
        return D3DERR_LIGHTHASVIEWPORT;
    }

    light_impl->active_viewport = vp;

    /* Add the light in the 'linked' chain */
    list_add_tail(&vp->light_list, &light_impl->entry);
    IDirect3DLight_AddRef(light);

    light_activate(light_impl);

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
static HRESULT WINAPI d3d_viewport_DeleteLight(IDirect3DViewport3 *iface, IDirect3DLight *lpDirect3DLight)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    struct d3d_light *l = unsafe_impl_from_IDirect3DLight(lpDirect3DLight);

    TRACE("iface %p, light %p.\n", iface, lpDirect3DLight);

    wined3d_mutex_lock();

    if (l->active_viewport != viewport)
    {
        WARN("Light %p active viewport is %p.\n", l, l->active_viewport);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    light_deactivate(l);
    list_remove(&l->entry);
    l->active_viewport = NULL;
    IDirect3DLight_Release(lpDirect3DLight);

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
static HRESULT WINAPI d3d_viewport_NextLight(IDirect3DViewport3 *iface,
        IDirect3DLight *lpDirect3DLight, IDirect3DLight **lplpDirect3DLight, DWORD flags)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    struct d3d_light *l = unsafe_impl_from_IDirect3DLight(lpDirect3DLight);
    struct list *entry;
    HRESULT hr;

    TRACE("iface %p, light %p, next_light %p, flags %#x.\n",
            iface, lpDirect3DLight, lplpDirect3DLight, flags);

    if (!lplpDirect3DLight)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    switch (flags)
    {
        case D3DNEXT_NEXT:
            if (!l || l->active_viewport != viewport)
            {
                if (l)
                    WARN("Light %p active viewport is %p.\n", l, l->active_viewport);
                entry = NULL;
            }
            else
                entry = list_next(&viewport->light_list, &l->entry);
            break;

        case D3DNEXT_HEAD:
            entry = list_head(&viewport->light_list);
            break;

        case D3DNEXT_TAIL:
            entry = list_tail(&viewport->light_list);
            break;

        default:
            entry = NULL;
            WARN("Invalid flags %#x.\n", flags);
            break;
    }

    if (entry)
    {
        *lplpDirect3DLight = (IDirect3DLight *)LIST_ENTRY(entry, struct d3d_light, entry);
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

static HRESULT WINAPI d3d_viewport_GetViewport2(IDirect3DViewport3 *iface, D3DVIEWPORT2 *vp)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    DWORD size;

    TRACE("iface %p, vp %p.\n", iface, vp);

    if (!vp)
        return DDERR_INVALIDPARAMS;

    if (viewport->version == DDRAW_VIEWPORT_VERSION_NONE)
    {
        WARN("Viewport data was not set.\n");
        return D3DERR_VIEWPORTDATANOTSET;
    }

    wined3d_mutex_lock();
    size = vp->dwSize;
    if (viewport->version == DDRAW_VIEWPORT_VERSION_2)
    {
        memcpy(vp, &viewport->viewports.vp2, size);
    }
    else
    {
        D3DVIEWPORT2 vp2;

        vp2.dwSize = sizeof(vp2);
        vp2.dwX = viewport->viewports.vp1.dwX;
        vp2.dwY = viewport->viewports.vp1.dwY;
        vp2.dwWidth = viewport->viewports.vp1.dwWidth;
        vp2.dwHeight = viewport->viewports.vp1.dwHeight;
        vp2.dvClipX = 0.0f;
        vp2.dvClipY = 0.0f;
        vp2.dvClipWidth = 0.0f;
        vp2.dvClipHeight = 0.0f;
        vp2.dvMinZ = viewport->viewports.vp1.dvMinZ;
        vp2.dvMaxZ = viewport->viewports.vp1.dvMaxZ;
        memcpy(vp, &vp2, size);
    }

    if (TRACE_ON(ddraw))
    {
        TRACE("  returning D3DVIEWPORT2 :\n");
        _dump_D3DVIEWPORT2(vp);
    }

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_viewport_SetViewport2(IDirect3DViewport3 *iface, D3DVIEWPORT2 *vp)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    struct d3d_device *device = viewport->active_device;
    struct wined3d_sub_resource_desc rt_desc;
    struct wined3d_rendertarget_view *rtv;
    IDirect3DViewport3 *current_viewport;
    struct ddraw_surface *surface;

    TRACE("iface %p, vp %p.\n", iface, vp);

    if (!vp)
        return DDERR_INVALIDPARAMS;

    if (vp->dwSize != sizeof(*vp))
    {
        WARN("Invalid D3DVIEWPORT2 size %u.\n", vp->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    if (TRACE_ON(ddraw))
    {
        TRACE("  getting D3DVIEWPORT2 :\n");
        _dump_D3DVIEWPORT2(vp);
    }

    if (!device)
    {
        WARN("Viewport not bound to a device, returning D3DERR_VIEWPORTHASNODEVICE.\n");
        return D3DERR_VIEWPORTHASNODEVICE;
    }

    wined3d_mutex_lock();

    if (device->version > 1)
    {
        if (!(rtv = wined3d_device_get_rendertarget_view(device->wined3d_device, 0)))
        {
            wined3d_mutex_unlock();
            return DDERR_INVALIDCAPS;
        }
        surface = wined3d_rendertarget_view_get_sub_resource_parent(rtv);
        wined3d_texture_get_sub_resource_desc(surface->wined3d_texture, surface->sub_resource_idx, &rt_desc);

        if (vp->dwX > rt_desc.width || vp->dwWidth > rt_desc.width - vp->dwX
            || vp->dwY > rt_desc.height || vp->dwHeight > rt_desc.height - vp->dwY)
        {
            WARN("Invalid viewport, returning DDERR_INVALIDPARAMS.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }
    }

    viewport->version = DDRAW_VIEWPORT_VERSION_2;
    viewport->viewports.vp2 = *vp;

    if (SUCCEEDED(IDirect3DDevice3_GetCurrentViewport(&device->IDirect3DDevice3_iface, &current_viewport)))
    {
        if (current_viewport == iface)
            viewport_activate(viewport, FALSE);
        IDirect3DViewport3_Release(current_viewport);
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
static HRESULT WINAPI d3d_viewport_SetBackgroundDepth2(IDirect3DViewport3 *iface,
        IDirectDrawSurface4 *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

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
static HRESULT WINAPI d3d_viewport_GetBackgroundDepth2(IDirect3DViewport3 *iface,
        IDirectDrawSurface4 **surface, BOOL *valid)
{
    FIXME("iface %p, surface %p, valid %p stub!\n", iface, surface, valid);

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
static HRESULT WINAPI d3d_viewport_Clear2(IDirect3DViewport3 *iface, DWORD rect_count,
        D3DRECT *rects, DWORD flags, DWORD color, D3DVALUE depth, DWORD stencil)
{
    struct d3d_viewport *viewport = impl_from_IDirect3DViewport3(iface);
    HRESULT hr;
    IDirect3DViewport3 *current_viewport;
    IDirect3DDevice3 *d3d_device3;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x, color 0x%08x, depth %.8e, stencil %u.\n",
            iface, rect_count, rects, flags, color, depth, stencil);

    if (!rects || !rect_count)
    {
        WARN("rect_count = %u, rects = %p, ignoring clear\n", rect_count, rects);
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (!viewport->active_device)
    {
        WARN("Trying to clear a viewport not attached to a device.\n");
        wined3d_mutex_unlock();
        return D3DERR_VIEWPORTHASNODEVICE;
    }
    d3d_device3 = &viewport->active_device->IDirect3DDevice3_iface;
    /* Need to temporarily activate viewport to clear it. Previously active
     * one will be restored afterwards. */
    viewport_activate(viewport, TRUE);

    hr = IDirect3DDevice7_Clear(&viewport->active_device->IDirect3DDevice7_iface,
            rect_count, rects, flags, color, depth, stencil);
    if (SUCCEEDED(IDirect3DDevice3_GetCurrentViewport(d3d_device3, &current_viewport)))
    {
        struct d3d_viewport *vp = impl_from_IDirect3DViewport3(current_viewport);
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
    d3d_viewport_QueryInterface,
    d3d_viewport_AddRef,
    d3d_viewport_Release,
    /*** IDirect3DViewport Methods */
    d3d_viewport_Initialize,
    d3d_viewport_GetViewport,
    d3d_viewport_SetViewport,
    d3d_viewport_TransformVertices,
    d3d_viewport_LightElements,
    d3d_viewport_SetBackground,
    d3d_viewport_GetBackground,
    d3d_viewport_SetBackgroundDepth,
    d3d_viewport_GetBackgroundDepth,
    d3d_viewport_Clear,
    d3d_viewport_AddLight,
    d3d_viewport_DeleteLight,
    d3d_viewport_NextLight,
    /*** IDirect3DViewport2 Methods ***/
    d3d_viewport_GetViewport2,
    d3d_viewport_SetViewport2,
    /*** IDirect3DViewport3 Methods ***/
    d3d_viewport_SetBackgroundDepth2,
    d3d_viewport_GetBackgroundDepth2,
    d3d_viewport_Clear2,
};

struct d3d_viewport *unsafe_impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_viewport_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_viewport, IDirect3DViewport3_iface);
}

struct d3d_viewport *unsafe_impl_from_IDirect3DViewport2(IDirect3DViewport2 *iface)
{
    /* IDirect3DViewport and IDirect3DViewport3 use the same iface. */
    if (!iface) return NULL;
    assert(iface->lpVtbl == (IDirect3DViewport2Vtbl *)&d3d_viewport_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_viewport, IDirect3DViewport3_iface);
}

struct d3d_viewport *unsafe_impl_from_IDirect3DViewport(IDirect3DViewport *iface)
{
    /* IDirect3DViewport and IDirect3DViewport3 use the same iface. */
    if (!iface) return NULL;
    assert(iface->lpVtbl == (IDirect3DViewportVtbl *)&d3d_viewport_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_viewport, IDirect3DViewport3_iface);
}

void d3d_viewport_init(struct d3d_viewport *viewport, struct ddraw *ddraw)
{
    viewport->IDirect3DViewport3_iface.lpVtbl = &d3d_viewport_vtbl;
    viewport->ref = 1;
    viewport->ddraw = ddraw;
    viewport->version = DDRAW_VIEWPORT_VERSION_NONE;
    list_init(&viewport->light_list);
}
