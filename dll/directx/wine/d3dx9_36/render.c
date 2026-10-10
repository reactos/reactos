/*
 * Copyright (C) 2012 Józef Kucia
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
 *
 */


#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct device_state
{
    DWORD num_render_targets;
    IDirect3DSurface9 **render_targets;
    IDirect3DSurface9 *depth_stencil;
    D3DVIEWPORT9 viewport;
};

static HRESULT device_state_init(IDirect3DDevice9 *device, struct device_state *state)
{
    HRESULT hr;
    D3DCAPS9 caps;
    unsigned int i;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (FAILED(hr)) return hr;

    state->num_render_targets = caps.NumSimultaneousRTs;
    state->render_targets = malloc(state->num_render_targets * sizeof(IDirect3DSurface9 *));
    if (!state->render_targets)
        return E_OUTOFMEMORY;

    for (i = 0; i < state->num_render_targets; i++)
        state->render_targets[i] = NULL;
    state->depth_stencil = NULL;
    return D3D_OK;
}

static void device_state_capture(IDirect3DDevice9 *device, struct device_state *state)
{
    HRESULT hr;
    unsigned int i;

    IDirect3DDevice9_GetViewport(device, &state->viewport);

    for (i = 0; i < state->num_render_targets; i++)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &state->render_targets[i]);
        if (FAILED(hr)) state->render_targets[i] = NULL;
    }

    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &state->depth_stencil);
    if (FAILED(hr)) state->depth_stencil = NULL;
}

static void device_state_restore(IDirect3DDevice9 *device, struct device_state *state)
{
    unsigned int i;

    for (i = 0; i < state->num_render_targets; i++)
    {
        IDirect3DDevice9_SetRenderTarget(device, i, state->render_targets[i]);
        if (state->render_targets[i])
            IDirect3DSurface9_Release(state->render_targets[i]);
        state->render_targets[i] = NULL;
    }

    IDirect3DDevice9_SetDepthStencilSurface(device, state->depth_stencil);
    if (state->depth_stencil)
    {
        IDirect3DSurface9_Release(state->depth_stencil);
        state->depth_stencil = NULL;
    }

    IDirect3DDevice9_SetViewport(device, &state->viewport);
}

static void device_state_release(struct device_state *state)
{
    unsigned int i;

    for (i = 0; i < state->num_render_targets; i++)
    {
        if (state->render_targets[i])
            IDirect3DSurface9_Release(state->render_targets[i]);
    }

    free(state->render_targets);

    if (state->depth_stencil) IDirect3DSurface9_Release(state->depth_stencil);
}

struct render_to_surface
{
    ID3DXRenderToSurface ID3DXRenderToSurface_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXRTS_DESC desc;

    IDirect3DSurface9 *dst_surface;

    IDirect3DSurface9 *render_target;
    IDirect3DSurface9 *depth_stencil;

    struct device_state previous_state;
};

static inline struct render_to_surface *impl_from_ID3DXRenderToSurface(ID3DXRenderToSurface *iface)
{
    return CONTAINING_RECORD(iface, struct render_to_surface, ID3DXRenderToSurface_iface);
}

static HRESULT WINAPI D3DXRenderToSurface_QueryInterface(ID3DXRenderToSurface *iface,
                                                         REFIID riid,
                                                         void **out)
{
    TRACE("iface %p, riid %s, out %p\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXRenderToSurface)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI D3DXRenderToSurface_AddRef(ID3DXRenderToSurface *iface)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    ULONG ref = InterlockedIncrement(&render->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI D3DXRenderToSurface_Release(ID3DXRenderToSurface *iface)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    ULONG ref = InterlockedDecrement(&render->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, ref);

    if (!ref)
    {
        if (render->dst_surface) IDirect3DSurface9_Release(render->dst_surface);

        if (render->render_target) IDirect3DSurface9_Release(render->render_target);
        if (render->depth_stencil) IDirect3DSurface9_Release(render->depth_stencil);

        device_state_release(&render->previous_state);

        IDirect3DDevice9_Release(render->device);

        free(render);
    }

    return ref;
}

static HRESULT WINAPI D3DXRenderToSurface_GetDevice(ID3DXRenderToSurface *iface,
                                                    IDirect3DDevice9 **device)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device) return D3DERR_INVALIDCALL;

    IDirect3DDevice9_AddRef(render->device);
    *device = render->device;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_GetDesc(ID3DXRenderToSurface *iface,
                                                  D3DXRTS_DESC *desc)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc) return D3DERR_INVALIDCALL;

    *desc = render->desc;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_BeginScene(ID3DXRenderToSurface *iface,
                                                     IDirect3DSurface9 *surface,
                                                     const D3DVIEWPORT9 *viewport)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    unsigned int i;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC surface_desc;
    HRESULT hr = D3DERR_INVALIDCALL;
    D3DMULTISAMPLE_TYPE multi_sample_type = D3DMULTISAMPLE_NONE;
    DWORD multi_sample_quality = 0;

    TRACE("iface %p, surface %p, viewport %p.\n", iface, surface, viewport);

    if (!surface || render->dst_surface) return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(surface, &surface_desc);
    if (surface_desc.Format != render->desc.Format
            || surface_desc.Width != render->desc.Width
            || surface_desc.Height != render->desc.Height)
        return D3DERR_INVALIDCALL;

    if (viewport)
    {
        if (viewport->X > render->desc.Width || viewport->Y > render->desc.Height
                || viewport->X + viewport->Width > render->desc.Width
                || viewport->Y + viewport->Height > render->desc.Height)
            return D3DERR_INVALIDCALL;

        if (!(surface_desc.Usage & D3DUSAGE_RENDERTARGET)
                && (viewport->X != 0 || viewport->Y != 0
                || viewport->Width != render->desc.Width
                || viewport->Height != render->desc.Height))
            return D3DERR_INVALIDCALL;
    }

    device = render->device;

    device_state_capture(device, &render->previous_state);

    /* prepare for rendering to surface */
    for (i = 1; i < render->previous_state.num_render_targets; i++)
        IDirect3DDevice9_SetRenderTarget(device, i, NULL);

    if (surface_desc.Usage & D3DUSAGE_RENDERTARGET)
    {
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
        multi_sample_type = surface_desc.MultiSampleType;
        multi_sample_quality = surface_desc.MultiSampleQuality;
    }
    else
    {
        hr = IDirect3DDevice9_CreateRenderTarget(device, render->desc.Width, render->desc.Height,
                render->desc.Format, multi_sample_type, multi_sample_quality, FALSE,
                &render->render_target, NULL);
        if (FAILED(hr)) goto cleanup;
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, render->render_target);
    }

    if (FAILED(hr)) goto cleanup;

    if (render->desc.DepthStencil)
    {
        hr = IDirect3DDevice9_CreateDepthStencilSurface(device, render->desc.Width, render->desc.Height,
                render->desc.DepthStencilFormat, multi_sample_type, multi_sample_quality, TRUE,
                &render->depth_stencil, NULL);
    }
    else render->depth_stencil = NULL;

    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, render->depth_stencil);
    if (FAILED(hr)) goto cleanup;

    if (viewport) IDirect3DDevice9_SetViewport(device, viewport);

    IDirect3DSurface9_AddRef(surface);
    render->dst_surface = surface;
    return IDirect3DDevice9_BeginScene(device);

cleanup:
    device_state_restore(device, &render->previous_state);

    if (render->dst_surface) IDirect3DSurface9_Release(render->dst_surface);
    render->dst_surface = NULL;

    if (render->render_target) IDirect3DSurface9_Release(render->render_target);
    render->render_target = NULL;
    if (render->depth_stencil) IDirect3DSurface9_Release(render->depth_stencil);
    render->depth_stencil = NULL;

    return hr;
}

static HRESULT WINAPI D3DXRenderToSurface_EndScene(ID3DXRenderToSurface *iface,
                                                   DWORD filter)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    HRESULT hr;

    TRACE("iface %p, filter %#lx.\n", iface, filter);

    if (!render->dst_surface) return D3DERR_INVALIDCALL;

    hr = IDirect3DDevice9_EndScene(render->device);

    /* copy render target data to destination surface, if needed */
    if (render->render_target)
    {
        hr = D3DXLoadSurfaceFromSurface(render->dst_surface, NULL, NULL,
                render->render_target, NULL, NULL, filter, 0);
        if (FAILED(hr))
            ERR("Copying render target data to surface failed, hr %#lx.\n", hr);
    }

    device_state_restore(render->device, &render->previous_state);

    /* release resources */
    if (render->render_target)
    {
        IDirect3DSurface9_Release(render->render_target);
        render->render_target = NULL;
    }

    if (render->depth_stencil)
    {
        IDirect3DSurface9_Release(render->depth_stencil);
        render->depth_stencil = NULL;
    }

    IDirect3DSurface9_Release(render->dst_surface);
    render->dst_surface = NULL;

    return hr;
}

static HRESULT WINAPI D3DXRenderToSurface_OnLostDevice(ID3DXRenderToSurface *iface)
{
    FIXME("iface %p stub!\n", iface);
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_OnResetDevice(ID3DXRenderToSurface *iface)
{
    FIXME("iface %p stub!\n", iface);
    return D3D_OK;
}

static const ID3DXRenderToSurfaceVtbl render_to_surface_vtbl =
{
    /* IUnknown methods */
    D3DXRenderToSurface_QueryInterface,
    D3DXRenderToSurface_AddRef,
    D3DXRenderToSurface_Release,
    /* ID3DXRenderToSurface methods */
    D3DXRenderToSurface_GetDevice,
    D3DXRenderToSurface_GetDesc,
    D3DXRenderToSurface_BeginScene,
    D3DXRenderToSurface_EndScene,
    D3DXRenderToSurface_OnLostDevice,
    D3DXRenderToSurface_OnResetDevice
};

HRESULT WINAPI D3DXCreateRenderToSurface(IDirect3DDevice9 *device,
                                         UINT width,
                                         UINT height,
                                         D3DFORMAT format,
                                         BOOL depth_stencil,
                                         D3DFORMAT depth_stencil_format,
                                         ID3DXRenderToSurface **out)
{
    HRESULT hr;
    struct render_to_surface *render;

    TRACE("device %p, width %u, height %u, format %#x, depth_stencil %#x, depth_stencil_format %#x, out %p.\n",
            device, width, height, format, depth_stencil, depth_stencil_format, out);

    if (!device || !out) return D3DERR_INVALIDCALL;

    render = malloc(sizeof(struct render_to_surface));
    if (!render) return E_OUTOFMEMORY;

    render->ID3DXRenderToSurface_iface.lpVtbl = &render_to_surface_vtbl;
    render->ref = 1;

    render->desc.Width = width;
    render->desc.Height = height;
    render->desc.Format = format;
    render->desc.DepthStencil = depth_stencil;
    render->desc.DepthStencilFormat = depth_stencil_format;

    render->dst_surface = NULL;
    render->render_target = NULL;
    render->depth_stencil = NULL;

    hr = device_state_init(device, &render->previous_state);
    if (FAILED(hr))
    {
        free(render);
        return hr;
    }

    IDirect3DDevice9_AddRef(device);
    render->device = device;

    *out = &render->ID3DXRenderToSurface_iface;
    return D3D_OK;
}


enum render_state
{
    INITIAL,

    CUBE_BEGIN,
    CUBE_FACE
};

struct render_to_envmap
{
    ID3DXRenderToEnvMap ID3DXRenderToEnvMap_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXRTE_DESC desc;

    enum render_state state;
    struct device_state previous_device_state;

    D3DCUBEMAP_FACES face;
    DWORD filter;

    IDirect3DSurface9 *render_target;
    IDirect3DSurface9 *depth_stencil;

    IDirect3DCubeTexture9 *dst_cube_texture;
};

static void copy_render_target_to_cube_texture_face(IDirect3DCubeTexture9 *cube_texture,
        D3DCUBEMAP_FACES face, IDirect3DSurface9 *render_target, DWORD filter)
{
    HRESULT hr;
    IDirect3DSurface9 *cube_surface;

    IDirect3DCubeTexture9_GetCubeMapSurface(cube_texture, face, 0, &cube_surface);

    hr = D3DXLoadSurfaceFromSurface(cube_surface, NULL, NULL, render_target, NULL, NULL, filter, 0);
    if (FAILED(hr))
        ERR("Copying render target data to surface failed, hr %#lx.\n", hr);

    IDirect3DSurface9_Release(cube_surface);
}

static inline struct render_to_envmap *impl_from_ID3DXRenderToEnvMap(ID3DXRenderToEnvMap *iface)
{
    return CONTAINING_RECORD(iface, struct render_to_envmap, ID3DXRenderToEnvMap_iface);
}

static HRESULT WINAPI D3DXRenderToEnvMap_QueryInterface(ID3DXRenderToEnvMap *iface,
                                                        REFIID riid,
                                                        void **out)
{
    TRACE("iface %p, riid %s, out %p\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXRenderToEnvMap)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI D3DXRenderToEnvMap_AddRef(ID3DXRenderToEnvMap *iface)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);
    ULONG ref = InterlockedIncrement(&render->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI D3DXRenderToEnvMap_Release(ID3DXRenderToEnvMap *iface)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);
    ULONG ref = InterlockedDecrement(&render->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, ref);

    if (!ref)
    {
        if (render->dst_cube_texture)
            IDirect3DCubeTexture9_Release(render->dst_cube_texture);

        if (render->render_target) IDirect3DSurface9_Release(render->render_target);
        if (render->depth_stencil) IDirect3DSurface9_Release(render->depth_stencil);

        device_state_release(&render->previous_device_state);

        IDirect3DDevice9_Release(render->device);

        free(render);
    }

    return ref;
}

static HRESULT WINAPI D3DXRenderToEnvMap_GetDevice(ID3DXRenderToEnvMap *iface,
                                                   IDirect3DDevice9 **device)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device) return D3DERR_INVALIDCALL;

    IDirect3DDevice9_AddRef(render->device);
    *device = render->device;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToEnvMap_GetDesc(ID3DXRenderToEnvMap *iface,
                                                 D3DXRTE_DESC *desc)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc) return D3DERR_INVALIDCALL;

    *desc = render->desc;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToEnvMap_BeginCube(ID3DXRenderToEnvMap *iface,
                                                   IDirect3DCubeTexture9 *texture)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);
    HRESULT hr;
    D3DSURFACE_DESC level_desc;

    TRACE("iface %p, texture %p.\n", iface, texture);

    if (!texture) return D3DERR_INVALIDCALL;

    if (render->state != INITIAL) return D3DERR_INVALIDCALL;

    IDirect3DCubeTexture9_GetLevelDesc(texture, 0, &level_desc);
    if (level_desc.Format != render->desc.Format || level_desc.Width != render->desc.Size)
        return D3DERR_INVALIDCALL;

    if (!(level_desc.Usage & D3DUSAGE_RENDERTARGET))
    {
        hr = IDirect3DDevice9_CreateRenderTarget(render->device, level_desc.Width, level_desc.Height,
                level_desc.Format, level_desc.MultiSampleType, level_desc.MultiSampleQuality,
                TRUE, &render->render_target, NULL);
        if (FAILED(hr)) goto cleanup;
        IDirect3DCubeTexture9_GetLevelDesc(texture, 0, &level_desc);
    }

    if (render->desc.DepthStencil)
    {
        hr = IDirect3DDevice9_CreateDepthStencilSurface(render->device, level_desc.Width, level_desc.Height,
                render->desc.DepthStencilFormat, level_desc.MultiSampleType, level_desc.MultiSampleQuality,
                TRUE, &render->depth_stencil, NULL);
        if (FAILED(hr)) goto cleanup;
    }

    IDirect3DCubeTexture9_AddRef(texture);
    render->dst_cube_texture = texture;
    render->state = CUBE_BEGIN;
    return D3D_OK;

cleanup:
    if (render->dst_cube_texture)
        IDirect3DCubeTexture9_Release(render->dst_cube_texture);
    render->dst_cube_texture = NULL;

    if (render->render_target) IDirect3DSurface9_Release(render->render_target);
    render->render_target = NULL;
    if (render->depth_stencil) IDirect3DSurface9_Release(render->depth_stencil);
    render->depth_stencil = NULL;

    return hr;
}

static HRESULT WINAPI D3DXRenderToEnvMap_BeginSphere(ID3DXRenderToEnvMap *iface,
                                                     IDirect3DTexture9 *texture)
{
    FIXME("iface %p, texture %p stub!\n", iface, texture);
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DXRenderToEnvMap_BeginHemisphere(ID3DXRenderToEnvMap *iface,
                                                         IDirect3DTexture9 *pos_z_texture,
                                                         IDirect3DTexture9 *neg_z_texture)
{
    FIXME("iface %p, pos_z_texture %p, neg_z_texture %p stub!\n", iface, pos_z_texture, neg_z_texture);
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DXRenderToEnvMap_BeginParabolic(ID3DXRenderToEnvMap *iface,
                                                        IDirect3DTexture9 *pos_z_texture,
                                                        IDirect3DTexture9 *neg_z_texture)
{
    FIXME("iface %p, pos_z_texture %p, neg_z_texture %p stub!\n", iface, pos_z_texture, neg_z_texture);
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DXRenderToEnvMap_Face(ID3DXRenderToEnvMap *iface,
                                              D3DCUBEMAP_FACES face,
                                              DWORD filter)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);
    HRESULT hr;
    unsigned int i;

    TRACE("iface %p, face %u, filter %#lx.\n", iface, face, filter);

    if (render->state == CUBE_FACE)
    {
        IDirect3DDevice9_EndScene(render->device);
        if (render->render_target)
            copy_render_target_to_cube_texture_face(render->dst_cube_texture, render->face,
                    render->render_target, render->filter);

        device_state_restore(render->device, &render->previous_device_state);

        render->state = CUBE_BEGIN;
    }
    else if (render->state != CUBE_BEGIN)
        return D3DERR_INVALIDCALL;

    device_state_capture(render->device, &render->previous_device_state);

    for (i = 1; i < render->previous_device_state.num_render_targets; i++)
        IDirect3DDevice9_SetRenderTarget(render->device, i, NULL);

    if (!render->render_target)
    {
        IDirect3DSurface9 *render_target;
        IDirect3DCubeTexture9_GetCubeMapSurface(render->dst_cube_texture, face, 0, &render_target);
        hr = IDirect3DDevice9_SetRenderTarget(render->device, 0, render_target);
        IDirect3DSurface9_Release(render_target);
    }
    else hr = IDirect3DDevice9_SetRenderTarget(render->device, 0, render->render_target);

    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice9_SetDepthStencilSurface(render->device, render->depth_stencil);
    if (FAILED(hr)) goto cleanup;

    render->state = CUBE_FACE;
    render->face = face;
    render->filter = filter;
    return IDirect3DDevice9_BeginScene(render->device);

cleanup:
    device_state_restore(render->device, &render->previous_device_state);
    return hr;
}

static HRESULT WINAPI D3DXRenderToEnvMap_End(ID3DXRenderToEnvMap *iface,
                                             DWORD filter)
{
    struct render_to_envmap *render = impl_from_ID3DXRenderToEnvMap(iface);

    TRACE("iface %p, filter %#lx.\n", iface, filter);

    if (render->state == INITIAL) return D3DERR_INVALIDCALL;

    if (render->state == CUBE_FACE)
    {
        IDirect3DDevice9_EndScene(render->device);
        if (render->render_target)
            copy_render_target_to_cube_texture_face(render->dst_cube_texture, render->face,
                    render->render_target, render->filter);

        device_state_restore(render->device, &render->previous_device_state);
    }

    D3DXFilterTexture((IDirect3DBaseTexture9 *)render->dst_cube_texture, NULL, 0, filter);

    if (render->render_target)
    {
        IDirect3DSurface9_Release(render->render_target);
        render->render_target = NULL;
    }

    if (render->depth_stencil)
    {
        IDirect3DSurface9_Release(render->depth_stencil);
        render->depth_stencil = NULL;
    }

    IDirect3DCubeTexture9_Release(render->dst_cube_texture);
    render->dst_cube_texture = NULL;

    render->state = INITIAL;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToEnvMap_OnLostDevice(ID3DXRenderToEnvMap *iface)
{
    FIXME("iface %p stub!\n", iface);
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToEnvMap_OnResetDevice(ID3DXRenderToEnvMap *iface)
{
    FIXME("iface %p stub!\n", iface);
    return D3D_OK;
}

static const ID3DXRenderToEnvMapVtbl render_to_envmap_vtbl =
{
    /* IUnknown methods */
    D3DXRenderToEnvMap_QueryInterface,
    D3DXRenderToEnvMap_AddRef,
    D3DXRenderToEnvMap_Release,
    /* ID3DXRenderToEnvMap methods */
    D3DXRenderToEnvMap_GetDevice,
    D3DXRenderToEnvMap_GetDesc,
    D3DXRenderToEnvMap_BeginCube,
    D3DXRenderToEnvMap_BeginSphere,
    D3DXRenderToEnvMap_BeginHemisphere,
    D3DXRenderToEnvMap_BeginParabolic,
    D3DXRenderToEnvMap_Face,
    D3DXRenderToEnvMap_End,
    D3DXRenderToEnvMap_OnLostDevice,
    D3DXRenderToEnvMap_OnResetDevice
};

HRESULT WINAPI D3DXCreateRenderToEnvMap(IDirect3DDevice9 *device,
                                        UINT size,
                                        UINT mip_levels,
                                        D3DFORMAT format,
                                        BOOL depth_stencil,
                                        D3DFORMAT depth_stencil_format,
                                        ID3DXRenderToEnvMap **out)
{
    HRESULT hr;
    struct render_to_envmap *render;

    TRACE("(%p, %u, %u, %#x, %d, %#x, %p)\n", device, size, mip_levels,
            format, depth_stencil, depth_stencil_format, out);

    if (!device || !out) return D3DERR_INVALIDCALL;

    hr = D3DXCheckTextureRequirements(device, &size, &size, &mip_levels,
            D3DUSAGE_RENDERTARGET, &format, D3DPOOL_DEFAULT);
    if (FAILED(hr)) return hr;

    render = malloc(sizeof(*render));
    if (!render) return E_OUTOFMEMORY;

    render->ID3DXRenderToEnvMap_iface.lpVtbl = &render_to_envmap_vtbl;
    render->ref = 1;

    render->desc.Size = size;
    render->desc.MipLevels = mip_levels;
    render->desc.Format = format;
    render->desc.DepthStencil = depth_stencil;
    render->desc.DepthStencilFormat = depth_stencil_format;

    render->state = INITIAL;
    render->render_target = NULL;
    render->depth_stencil = NULL;
    render->dst_cube_texture = NULL;

    hr = device_state_init(device, &render->previous_device_state);
    if (FAILED(hr))
    {
        free(render);
        return hr;
    }

    IDirect3DDevice9_AddRef(device);
    render->device = device;

    *out = &render->ID3DXRenderToEnvMap_iface;
    return D3D_OK;
}
