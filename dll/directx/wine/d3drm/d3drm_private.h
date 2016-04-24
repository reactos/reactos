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

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <d3drm.h>
#include <dxfile.h>
#include <rmxfguid.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

struct d3drm_device;

HRESULT d3drm_device_create(struct d3drm_device **out) DECLSPEC_HIDDEN;
IDirect3DRMDevice *IDirect3DRMDevice_from_impl(struct d3drm_device *device) DECLSPEC_HIDDEN;
IDirect3DRMDevice2 *IDirect3DRMDevice2_from_impl(struct d3drm_device *device) DECLSPEC_HIDDEN;
IDirect3DRMDevice3 *IDirect3DRMDevice3_from_impl(struct d3drm_device *device) DECLSPEC_HIDDEN;
HRESULT Direct3DRMFace_create(REFIID riid, IUnknown** ret_iface) DECLSPEC_HIDDEN;
HRESULT Direct3DRMFrame_create(REFIID riid, IUnknown* parent_frame, IUnknown** ret_iface) DECLSPEC_HIDDEN;
HRESULT Direct3DRMLight_create(IUnknown** ppObj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMMesh_create(IDirect3DRMMesh** obj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMMeshBuilder_create(REFIID riid, IUnknown** ppObj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMViewport_create(REFIID riid, IUnknown** ppObj) DECLSPEC_HIDDEN;
HRESULT Direct3DRMMaterial_create(IDirect3DRMMaterial2** ret_iface) DECLSPEC_HIDDEN;
HRESULT Direct3DRMTexture_create(REFIID riid, IUnknown** ret_iface) DECLSPEC_HIDDEN;

HRESULT load_mesh_data(IDirect3DRMMeshBuilder3 *iface, IDirectXFileData *data,
                       D3DRMLOADTEXTURECALLBACK load_texture_proc, void *arg) DECLSPEC_HIDDEN;

void d3drm_device_destroy(struct d3drm_device *device) DECLSPEC_HIDDEN;

HRESULT d3drm_device_create_surfaces_from_clipper(struct d3drm_device *object, IDirectDraw *ddraw, IDirectDrawClipper *clipper, int width, int height,
            IDirectDrawSurface **surface) DECLSPEC_HIDDEN;

HRESULT d3drm_device_init(struct d3drm_device *device, UINT version, IDirect3DRM *d3drm, IDirectDraw *ddraw, IDirectDrawSurface *surface,
            BOOL create_z_surface) DECLSPEC_HIDDEN;

HRESULT d3drm_device_set_ddraw_device_d3d(struct d3drm_device *device, IDirect3DRM *d3drm, IDirect3D *d3d, IDirect3DDevice *d3d_device) DECLSPEC_HIDDEN;

struct d3drm_file_header
{
    WORD major;
    WORD minor;
    DWORD flags;
};

extern char templates[] DECLSPEC_HIDDEN;

#endif /* __D3DRM_PRIVATE_INCLUDED__ */
