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

#define COBJMACROS
#include <assert.h>
#include <math.h>
#include "dxfile.h"
#include "d3drmwin.h"
#include "rmxfguid.h"
#include "wine/debug.h"
#include "wine/list.h"

struct d3drm_matrix
{
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
};

static inline struct d3drm_matrix *d3drm_matrix(D3DRMMATRIX4D m)
{
    return (struct d3drm_matrix *)m;
}

struct d3drm_object
{
    LONG ref;
    DWORD appdata;
    struct list destroy_callbacks;
    const char *classname;
    char *name;
};

struct d3drm_texture
{
    struct d3drm_object obj;
    IDirect3DRMTexture IDirect3DRMTexture_iface;
    IDirect3DRMTexture2 IDirect3DRMTexture2_iface;
    IDirect3DRMTexture3 IDirect3DRMTexture3_iface;
    IDirect3DRM *d3drm;
    D3DRMIMAGE *image;
    IDirectDrawSurface *surface;
    LONG decal_x;
    LONG decal_y;
    DWORD max_colors;
    DWORD max_shades;
    BOOL transparency;
    D3DVALUE decal_width;
    D3DVALUE decal_height;
};

struct d3drm_frame
{
    struct d3drm_object obj;
    IDirect3DRMFrame IDirect3DRMFrame_iface;
    IDirect3DRMFrame2 IDirect3DRMFrame2_iface;
    IDirect3DRMFrame3 IDirect3DRMFrame3_iface;
    IDirect3DRM *d3drm;
    LONG ref;
    struct d3drm_frame *parent;
    SIZE_T nb_children;
    SIZE_T children_size;
    IDirect3DRMFrame3 **children;
    SIZE_T nb_visuals;
    SIZE_T visuals_size;
    IDirect3DRMVisual **visuals;
    SIZE_T nb_lights;
    SIZE_T lights_size;
    IDirect3DRMLight **lights;
    struct d3drm_matrix transform;
    D3DCOLOR scenebackground;
    DWORD traversal_options;
};

struct d3drm_box
{
    float left;
    float top;
    float right;
    float bottom;
    float front;
    float back;
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
    struct d3drm_box clip;
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

struct d3drm_face
{
    struct d3drm_object obj;
    IDirect3DRMFace IDirect3DRMFace_iface;
    IDirect3DRMFace2 IDirect3DRMFace2_iface;
    LONG ref;
    D3DCOLOR color;
};

struct d3drm_mesh_builder
{
    struct d3drm_object obj;
    IDirect3DRMMeshBuilder2 IDirect3DRMMeshBuilder2_iface;
    IDirect3DRMMeshBuilder3 IDirect3DRMMeshBuilder3_iface;
    LONG ref;
    IDirect3DRM *d3drm;
    SIZE_T nb_vertices;
    SIZE_T vertices_size;
    D3DVECTOR *vertices;
    SIZE_T nb_normals;
    SIZE_T normals_size;
    D3DVECTOR *normals;
    DWORD nb_faces;
    DWORD face_data_size;
    void *pFaceData;
    DWORD nb_coords2d;
    struct coords_2d *pCoords2d;
    D3DCOLOR color;
    IDirect3DRMMaterial2 *material;
    IDirect3DRMTexture3 *texture;
    DWORD nb_materials;
    struct mesh_material *materials;
    DWORD *material_indices;
    D3DRMRENDERQUALITY quality;
};

struct mesh_group
{
    unsigned nb_vertices;
    D3DRMVERTEX* vertices;
    unsigned nb_faces;
    unsigned vertex_per_face;
    DWORD face_data_size;
    unsigned* face_data;
    D3DCOLOR color;
    IDirect3DRMMaterial2* material;
    IDirect3DRMTexture3* texture;
};

struct d3drm_mesh
{
    struct d3drm_object obj;
    IDirect3DRMMesh IDirect3DRMMesh_iface;
    LONG ref;
    IDirect3DRM *d3drm;
    SIZE_T nb_groups;
    SIZE_T groups_size;
    struct mesh_group *groups;
};

struct d3drm_light
{
    struct d3drm_object obj;
    IDirect3DRMLight IDirect3DRMLight_iface;
    LONG ref;
    IDirect3DRM *d3drm;
    D3DRMLIGHTTYPE type;
    D3DCOLOR color;
    D3DVALUE range;
    D3DVALUE cattenuation;
    D3DVALUE lattenuation;
    D3DVALUE qattenuation;
    D3DVALUE umbra;
    D3DVALUE penumbra;
};

struct color_rgb
{
    D3DVALUE r;
    D3DVALUE g;
    D3DVALUE b;
};

struct d3drm_material
{
    struct d3drm_object obj;
    IDirect3DRMMaterial2 IDirect3DRMMaterial2_iface;
    LONG ref;
    IDirect3DRM *d3drm;
    struct color_rgb emissive;
    struct color_rgb specular;
    D3DVALUE power;
    struct color_rgb ambient;
};

struct d3drm_animation_key
{
    D3DVALUE time;
    union
    {
        D3DVECTOR position;
        D3DVECTOR scale;
        D3DRMQUATERNION rotate;
    } u;
};

struct d3drm_animation_keys
{
    struct d3drm_animation_key *keys;
    SIZE_T count;
    SIZE_T size;
};

struct d3drm_animation
{
    struct d3drm_object obj;
    IDirect3DRMAnimation2 IDirect3DRMAnimation2_iface;
    IDirect3DRMAnimation IDirect3DRMAnimation_iface;
    LONG ref;
    IDirect3DRM *d3drm;
    IDirect3DRMFrame3 *frame;
    D3DRMANIMATIONOPTIONS options;
    struct d3drm_animation_keys position;
    struct d3drm_animation_keys scale;
    struct d3drm_animation_keys rotate;
};

struct d3drm_wrap
{
    struct d3drm_object obj;
    IDirect3DRMWrap IDirect3DRMWrap_iface;
    LONG ref;
};

HRESULT d3drm_device_create(struct d3drm_device **device, IDirect3DRM *d3drm);
HRESULT d3drm_device_create_surfaces_from_clipper(struct d3drm_device *object, IDirectDraw *ddraw,
        IDirectDrawClipper *clipper, int width, int height, IDirectDrawSurface **surface);
void d3drm_device_destroy(struct d3drm_device *device);
HRESULT d3drm_device_init(struct d3drm_device *device, UINT version, IDirectDraw *ddraw,
        IDirectDrawSurface *surface, BOOL create_z_surface);

void d3drm_object_init(struct d3drm_object *object, const char *classname);
HRESULT d3drm_object_add_destroy_callback(struct d3drm_object *object, D3DRMOBJECTCALLBACK cb, void *ctx);
HRESULT d3drm_object_delete_destroy_callback(struct d3drm_object *object, D3DRMOBJECTCALLBACK cb, void *ctx);
HRESULT d3drm_object_get_class_name(struct d3drm_object *object, DWORD *size, char *name);
HRESULT d3drm_object_get_name(struct d3drm_object *object, DWORD *size, char *name);
HRESULT d3drm_object_set_name(struct d3drm_object *object, const char *name);
void d3drm_object_cleanup(IDirect3DRMObject *iface, struct d3drm_object *object);

struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame(IDirect3DRMFrame *iface);
struct d3drm_frame *unsafe_impl_from_IDirect3DRMFrame3(IDirect3DRMFrame3 *iface);

struct d3drm_device *unsafe_impl_from_IDirect3DRMDevice3(IDirect3DRMDevice3 *iface);

HRESULT d3drm_texture_create(struct d3drm_texture **texture, IDirect3DRM *d3drm);
HRESULT d3drm_frame_create(struct d3drm_frame **frame, IUnknown *parent_frame, IDirect3DRM *d3drm);
HRESULT d3drm_face_create(struct d3drm_face **face);
HRESULT d3drm_viewport_create(struct d3drm_viewport **viewport, IDirect3DRM *d3drm);
HRESULT d3drm_mesh_builder_create(struct d3drm_mesh_builder **mesh_builder, IDirect3DRM *d3drm);
HRESULT d3drm_light_create(struct d3drm_light **light, IDirect3DRM *d3drm);
HRESULT d3drm_material_create(struct d3drm_material **material, IDirect3DRM *d3drm);
HRESULT d3drm_mesh_create(struct d3drm_mesh **mesh, IDirect3DRM *d3drm);
HRESULT d3drm_animation_create(struct d3drm_animation **animation, IDirect3DRM *d3drm);
HRESULT d3drm_wrap_create(struct d3drm_wrap **wrap, IDirect3DRM *d3drm);

HRESULT load_mesh_data(IDirect3DRMMeshBuilder3 *iface, IDirectXFileData *data,
                       D3DRMLOADTEXTURECALLBACK load_texture_proc, void *arg);

struct d3drm_file_header
{
    WORD major;
    WORD minor;
    DWORD flags;
};

extern char templates[];

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

BOOL d3drm_array_reserve(void **elements, SIZE_T *capacity, SIZE_T element_count, SIZE_T element_size);

#endif /* __D3DRM_PRIVATE_INCLUDED__ */
