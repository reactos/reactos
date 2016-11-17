/*
 *      Direct3DRM private interfaces (D3DRM.DLL)
 *
 * Copyright 2010 Christian Costa
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

#ifndef __D3DRM_PRIVATE_INCLUDED__
#define __D3DRM_PRIVATE_INCLUDED__

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION

#include <assert.h>
#include <math.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <d3drm.h>
#include <dxfile.h>
#include <d3drmwin.h>
#include <rmxfguid.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

#include <wine/list.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

struct d3drm_object
{
    LONG ref;
    DWORD appdata;
    struct list destroy_callbacks;
};

struct d3drm_texture
{
    struct d3drm_object obj;
    IDirect3DRMTexture IDirect3DRMTexture_iface;
    IDirect3DRMTexture2 IDirect3DRMTexture2_iface;
    IDirect3DRMTexture3 IDirect3DRMTexture3_iface;
    IDirect3DRM *d3drm;
    D3DRMIMAGE *image;
};

struct d3drm_frame
{
    IDirect3DRMFrame IDirect3DRMFrame_iface;
    IDirect3DRMFrame2 IDirect3DRMFrame2_iface;
    IDirect3DRMFrame3 IDirect3DRMFrame3_iface;
    IDirect3DRM *d3drm;
    LONG ref;
    struct d3drm_frame *parent;
    ULONG nb_children;
    ULONG children_capacity;
    IDirect3DRMFrame3 **children;
    ULONG nb_visuals;
    ULONG visuals_capacity;
    IDirect3DRMVisual **visuals;
    ULONG nb_lights;
    ULONG lights_capacity;
    IDirect3DRMLight **lights;
    D3DRMMATRIX4D transform;
    D3DCOLOR scenebackground;
};

struct d3drm_viewport
{
    struct d3drm_object obj;
    struct d3drm_device *device;
    IDirect3DRMFrame *camera;
    IDirect3DRMViewport IDirect3DRMViewport_iface;
    IDirect3DRMViewport2 IDirect3DRMViewport2_iface;
    IDirect3DViewport *d3d_viewport;
    IDirect3DMaterial *material;
    IDirect3DRM *d3drm;
    D3DVALUE back;
    D3DVALUE front;
    D3DVALUE field;
    D3DRMPROJECTIONTYPE projection;
};

struct d3drm_device
{
    struct d3drm_object obj;
    IDirect3DRMDevice IDirect3DRMDevice_iface;
    IDirect3DRMDevice2 IDirect3DRMDevice2_iface;
    IDirect3DRMDevice3 IDirect3DRMDevice3_iface;
    IDirect3DRMWinDevice IDirect3DRMWinDevice_iface;
    IDirect3DRM *d3drm;
    IDirectDraw *ddraw;
    IDirectDrawSurface *primary_surface, *render_target;
    IDirectDrawClipper *clipper;
    IDirect3DDevice *device;
    BOOL dither;
    D3DRMRENDERQUALITY quality;
    DWORD rendermode;
    DWORD height;
    DWORD width;
};

HRESULT d3drm_device_create(struct d3drm_device **device, IDirect3DRM *d3drm) DECLSPEC_HIDDEN;
HRESULT d3drm_device_create_surfaces_from_clipper(struct d3drm_device *object, IDirectDraw *ddraw,
        IDirectDrawClipper *clipper, int width, int height, IDirectDrawSurface **surface) DECLSPEC_HIDDEN;
void d3drm_device_destroy(struct d3drm_device *device) DECLSPEC_HIDDEN;
HRESULT d3drm_device_init(struct d3drm_device *device, UINT version, IDirectDraw *ddraw,
        IDirectDrawSurface *surface, BOOL create_z_surface) DECLSPEC_HIDDEN;

void d3drm_object_init(struct d3drm_object *object) DECLSPEC_HIDDEN;
HRESULT d3drm_object_add_destroy_callback(struct d3drm_object *object, D3DRMOBJECTCALLBACK cb, void *ctx) DECLSPEC_HIDDEN;
HRESULT d3drm_object_delete_destroy_callback(struct d3drm_object *object, D3DRMOBJECTCALLBACK cb, void *ctx) DECLSPEC_HIDDEN;
void d3drm_object_cleanup(IDirect3DRMObject *iface, struct d3drm_object *object) DECLSPEC_HIDDEN;

struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame(IDirect3DRMFrame *iface) DECLSPEC_HIDDEN;
struct d3drm_device *unsafe_impl_from_IDirect3DRMDevice3(IDirect3DRMDevice3 *iface) DECLSPEC_HIDDEN;

HRESULT d3drm_texture_create(struct d3drm_texture **texture, IDirect3DRM *d3drm) DECLSPEC_HIDDEN;
HRESULT d3drm_frame_create(struct d3drm_frame **frame, IUnknown *parent_frame, IDirect3DRM *d3drm) DECLSPEC_HIDDEN;
HRESULT d3drm_viewport_create(struct d3drm_viewport **viewport, IDirect3DRM *d3drm) DECLSPEC_HIDDEN;
HRESULT Direct3DRMFace_create(REFIID riid, IUnknown** ret_iface) DECLSPEC_HIDDEN;
HRESULT Direct3DRMLight_create(IUnknown** ppObj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMMesh_create(IDirect3DRMMesh** obj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMMeshBuilder_create(REFIID riid, IUnknown** ppObj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMMaterial_create(IDirect3DRMMaterial2** ret_iface) DECLSPEC_HIDDEN;

HRESULT load_mesh_data(IDirect3DRMMeshBuilder3 *iface, IDirectXFileData *data,
                       D3DRMLOADTEXTURECALLBACK load_texture_proc, void *arg) DECLSPEC_HIDDEN;

struct d3drm_file_header
{
    WORD major;
    WORD minor;
    DWORD flags;
};

extern char templates[] DECLSPEC_HIDDEN;

static inline BYTE d3drm_color_component(float c)
{
    if (c <= 0.0f)
        return 0u;
    if (c >= 1.0f)
        return 0xffu;
    return floor(c * 255.0f);
}

static inline void d3drm_set_color(D3DCOLOR *color, float r, float g, float b, float a)
{
    *color = RGBA_MAKE(d3drm_color_component(r), d3drm_color_component(g),
            d3drm_color_component(b), d3drm_color_component(a));
}

#endif /* __D3DRM_PRIVATE_INCLUDED__ */
