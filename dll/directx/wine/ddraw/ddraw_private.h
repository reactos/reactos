/*
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

#ifndef __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H
#define __WINE_DLLS_DDRAW_DDRAW_PRIVATE_H

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#define COBJMACROS
#include "wine/debug.h"

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "d3d.h"
#include "ddraw.h"
#ifdef DDRAW_INIT_GUID
#include "initguid.h"
#endif
#include "wine/list.h"
#include "wine/wined3d.h"

extern const struct wined3d_parent_ops ddraw_null_wined3d_parent_ops;
extern DWORD force_refresh_rate;

/*****************************************************************************
 * IDirectDraw implementation structure
 *****************************************************************************/
struct FvfToDecl
{
    DWORD fvf;
    struct wined3d_vertex_declaration *decl;
};

#define DDRAW_INITIALIZED       0x00000001
#define DDRAW_D3D_INITIALIZED   0x00000002
#define DDRAW_RESTORE_MODE      0x00000004
#define DDRAW_NO3D              0x00000008
#define DDRAW_SCL_DDRAW1        0x00000010
#define DDRAW_SCL_RECURSIVE     0x00000020
#define DDRAW_SWAPPED           0x00000040

#define DDRAW_STRIDE_ALIGNMENT  8

#define DDRAW_WINED3D_FLAGS     (WINED3D_LEGACY_DEPTH_BIAS | WINED3D_RESTORE_MODE_ON_ACTIVATE \
        | WINED3D_FOCUS_MESSAGES | WINED3D_PIXEL_CENTER_INTEGER | WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR \
        | WINED3D_NO_PRIMITIVE_RESTART | WINED3D_LEGACY_CUBEMAP_FILTERING | WINED3D_NO_DRAW_INDIRECT)

#define DDRAW_WINED3D_SWAPCHAIN_FLAGS (WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH \
        | WINED3D_SWAPCHAIN_IMPLICIT | WINED3D_SWAPCHAIN_REGISTER_TOPMOST_TIMER)

#define DDRAW_MAX_ACTIVE_LIGHTS 32
#define DDRAW_MAX_TEXTURES 8

enum ddraw_device_state
{
    DDRAW_DEVICE_STATE_OK,
    DDRAW_DEVICE_STATE_LOST,
    DDRAW_DEVICE_STATE_NOT_RESTORED,
};

struct ddraw
{
    /* Interfaces */
    IDirectDraw7 IDirectDraw7_iface;
    IDirectDraw4 IDirectDraw4_iface;
    IDirectDraw2 IDirectDraw2_iface;
    IDirectDraw IDirectDraw_iface;
    IDirect3D7 IDirect3D7_iface;
    IDirect3D3 IDirect3D3_iface;
    IDirect3D2 IDirect3D2_iface;
    IDirect3D IDirect3D_iface;
    struct wined3d_device_parent device_parent;

    /* See comment in IDirectDraw::AddRef */
    LONG                    ref7, ref4, ref2, ref3, ref1, numIfaces;

    struct wined3d *wined3d;
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_output *wined3d_output;
    struct wined3d_device *wined3d_device;
    struct wined3d_device_context *immediate_context;
    DWORD flags;
    LONG device_state;

    struct ddraw_surface *primary;
    RECT primary_lock;
    struct wined3d_texture *gdi_surface;
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_state_parent state_parent;
    HWND swapchain_window;

    /* DirectDraw things, which are not handled by WineD3D */
    DWORD                   cooperative_level;

    /* D3D things */
    HWND                    d3d_window;
    struct list             d3ddevice_list;
    struct d3d_device      *device_last_applied_state;
    int                     d3dversion;

    /* Various HWNDs */
    HWND                    focuswindow;
    HWND                    devicewindow;
    HWND                    dest_window;

    /* For the dll unload cleanup code */
    struct list ddraw_list_entry;
    /* The surface list - can't relay this to WineD3D
     * because of IParent
     */
    struct list surface_list;

    /* FVF management */
    struct FvfToDecl       *decls;
    UINT                    numConvertedDecls, declArraySize;

    unsigned int frames;
    DWORD prev_frame_time;
};

#define DDRAW_WINDOW_CLASS_NAME "DirectDrawDeviceWnd"

HRESULT ddraw_init(struct ddraw *ddraw, DWORD flags, enum wined3d_device_type device_type);
void ddraw_d3dcaps1_from_7(D3DDEVICEDESC *caps1, D3DDEVICEDESC7 *caps7);
HRESULT ddraw_get_d3dcaps(const struct ddraw *ddraw, D3DDEVICEDESC7 *caps);
void ddraw_update_lost_surfaces(struct ddraw *ddraw);

static inline void ddraw_set_swapchain_window(struct ddraw *ddraw, HWND window)
{
    if (window == GetDesktopWindow())
        window = NULL;
    ddraw->swapchain_window = window;
}

/* Utility functions */
void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS *pIn, DDSCAPS2 *pOut);
void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2 *pIn, DDDEVICEIDENTIFIER *pOut);
struct wined3d_vertex_declaration *ddraw_find_decl(struct ddraw *ddraw, DWORD fvf);

#define DDRAW_SURFACE_LOCATION_DEFAULT 0x00000001
#define DDRAW_SURFACE_LOCATION_DRAW    0x00000002

struct ddraw_surface
{
    /* IUnknown fields */
    IDirectDrawSurface7 IDirectDrawSurface7_iface;
    IDirectDrawSurface4 IDirectDrawSurface4_iface;
    IDirectDrawSurface3 IDirectDrawSurface3_iface;
    IDirectDrawSurface2 IDirectDrawSurface2_iface;
    IDirectDrawSurface IDirectDrawSurface_iface;
    IDirectDrawGammaControl IDirectDrawGammaControl_iface;
    IDirect3DTexture2 IDirect3DTexture2_iface;
    IDirect3DTexture IDirect3DTexture_iface;

    LONG                     ref7, ref4, ref3, ref2, ref1, iface_count, gamma_count;
    IUnknown                *ifaceToRelease;
    IUnknown *texture_outer;

    int                     version;

    /* Connections to other Objects */
    struct ddraw *ddraw;
    unsigned int texture_location;
    struct wined3d_texture *wined3d_texture;
    struct wined3d_texture *draw_texture;
    unsigned int sub_resource_idx;
    struct wined3d_rendertarget_view *wined3d_rtv;
    struct wined3d_private_store private_store;
    struct d3d_device *device1;

    /* This implementation handles attaching surfaces to other surfaces */
    struct ddraw_surface *next_attached;
    struct ddraw_surface *first_attached;
    IUnknown                *attached_iface;

    /* Complex surfaces are organized in a tree, although the tree is degenerated to a list in most cases.
     * In mipmap and primary surfaces each level has only one attachment, which is the next surface level.
     * Only the cube texture root has 6 surfaces attached, which then have a normal mipmap chain attached
     * to them. So hardcode the array to 6, a dynamic array or a list would be an overkill.
     */
#define MAX_COMPLEX_ATTACHED 6
    struct ddraw_surface *complex_array[MAX_COMPLEX_ATTACHED];
    /* You can't traverse the tree upwards. Only a flag for Surface::Release because it's needed there,
     * but no pointer to prevent temptations to traverse it in the wrong direction.
     */
    unsigned int is_root : 1;
    unsigned int is_lost : 1;
    unsigned int sysmem_fallback : 1;

    /* Surface description, for GetAttachedSurface */
    DDSURFACEDESC2          surface_desc;

    /* Clipper objects */
    struct ddraw_clipper *clipper;
    struct ddraw_palette *palette;

    /* For the ddraw surface list */
    struct list             surface_list_entry;

    DWORD                   Handle;
    HDC dc;
};

struct ddraw_texture
{
    unsigned int version;
    DDSURFACEDESC2 surface_desc;

    struct ddraw_surface *root;
    struct wined3d_device *wined3d_device;

    void *texture_memory;
};

HRESULT ddraw_surface_create(struct ddraw *ddraw, const DDSURFACEDESC2 *surface_desc,
        struct ddraw_surface **surface, IUnknown *outer_unknown, unsigned int version);
struct wined3d_rendertarget_view *ddraw_surface_get_rendertarget_view(struct ddraw_surface *surface);
HRESULT ddraw_surface_update_frontbuffer(struct ddraw_surface *surface,
        const RECT *rect, BOOL read, unsigned int swap_interval);

static inline struct ddraw_surface *impl_from_IDirect3DTexture(IDirect3DTexture *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirect3DTexture_iface);
}

static inline struct ddraw_surface *impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirect3DTexture2_iface);
}

static inline struct ddraw_surface *impl_from_IDirectDrawSurface(IDirectDrawSurface *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface_iface);
}

static inline struct ddraw_surface *impl_from_IDirectDrawSurface2(IDirectDrawSurface2 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface2_iface);
}

static inline struct ddraw_surface *impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface3_iface);
}

static inline struct ddraw_surface *impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface4_iface);
}

static inline struct ddraw_surface *impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_surface, IDirectDrawSurface7_iface);
}

struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface(IDirectDrawSurface *iface);
struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface);
struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface);

struct ddraw_surface *unsafe_impl_from_IDirect3DTexture(IDirect3DTexture *iface);
struct ddraw_surface *unsafe_impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface);

#define DDRAW_INVALID_HANDLE ~0U

enum ddraw_handle_type
{
    DDRAW_HANDLE_FREE,
    DDRAW_HANDLE_MATERIAL,
    DDRAW_HANDLE_MATRIX,
    DDRAW_HANDLE_STATEBLOCK,
    DDRAW_HANDLE_SURFACE,
};

struct ddraw_handle_entry
{
    void *object;
    enum ddraw_handle_type type;
};

struct ddraw_handle_table
{
    struct ddraw_handle_entry *entries;
    struct ddraw_handle_entry *free_entries;
    UINT table_size;
    UINT entry_count;
};

BOOL ddraw_handle_table_init(struct ddraw_handle_table *t, UINT initial_size);
void ddraw_handle_table_destroy(struct ddraw_handle_table *t);
DWORD ddraw_allocate_handle(struct ddraw_handle_table *t, void *object, enum ddraw_handle_type type);
void *ddraw_free_handle(struct ddraw_handle_table *t, DWORD handle, enum ddraw_handle_type type);
void *ddraw_get_object(struct ddraw_handle_table *t, DWORD handle, enum ddraw_handle_type type);
extern struct ddraw_handle_table global_handle_table;

struct d3d_device
{
    /* IUnknown */
    IDirect3DDevice7 IDirect3DDevice7_iface;
    IDirect3DDevice3 IDirect3DDevice3_iface;
    IDirect3DDevice2 IDirect3DDevice2_iface;
    IDirect3DDevice IDirect3DDevice_iface;
    IUnknown IUnknown_inner;
    LONG ref;
    UINT version;
    BOOL hardware_device;
    BOOL have_draw_textures;

    IUnknown *outer_unknown;
    struct wined3d_device *wined3d_device;
    struct wined3d_device_context *immediate_context;
    struct ddraw *ddraw;
    struct list ddraw_entry;
    IUnknown *rt_iface;
    struct ddraw_surface *target, *target_ds;

    struct wined3d_streaming_buffer vertex_buffer, index_buffer;

    /* Viewport management */
    struct list viewport_list;
    struct d3d_viewport *current_viewport;
    D3DVIEWPORT7 active_viewport;

    /* Required to keep track which of two available texture blending modes in d3ddevice3 is used */
    BOOL legacyTextureBlending;
    D3DTEXTUREBLEND texture_map_blend;

    struct wined3d_matrix legacy_projection, legacy_clipspace;

    /* Light state */
    DWORD material;

    /* Rendering functions to wrap D3D(1-3) to D3D7 */
    D3DPRIMITIVETYPE primitive_type;
    DWORD vertex_type;
    DWORD render_flags;
    DWORD nb_vertices;
    BYTE *sysmem_vertex_buffer;
    DWORD vertex_size;
    DWORD buffer_size;

    /* Handle management */
    struct ddraw_handle_table handle_table;
    D3DMATRIXHANDLE          world, proj, view;

    struct wined3d_vec4 user_clip_planes[D3DMAXUSERCLIPPLANES];

    struct wined3d_stateblock *recording, *state, *update_state;
    const struct wined3d_stateblock_state *stateblock_state;

    /* For temporary saving state during reset. */
    struct wined3d_stateblock *saved_state;
};

HRESULT d3d_device_create(struct ddraw *ddraw, const GUID *guid, struct ddraw_surface *target, IUnknown *rt_iface,
        UINT version, struct d3d_device **device, IUnknown *outer_unknown);
enum wined3d_depth_buffer_type d3d_device_update_depth_stencil(struct d3d_device *device);

/* The IID */
extern const GUID IID_D3DDEVICE_WineD3D;

static inline struct d3d_device *impl_from_IDirect3DDevice(IDirect3DDevice *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice_iface);
}

static inline struct d3d_device *impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice2_iface);
}

static inline struct d3d_device *impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice3_iface);
}

static inline struct d3d_device *impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice7_iface);
}

struct d3d_device *unsafe_impl_from_IDirect3DDevice(IDirect3DDevice *iface);
struct d3d_device *unsafe_impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface);
struct d3d_device *unsafe_impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface);
struct d3d_device *unsafe_impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface);

struct ddraw_clipper
{
    IDirectDrawClipper IDirectDrawClipper_iface;
    LONG ref;
    HWND window;
    HRGN region;
    BOOL initialized;
};

HRESULT ddraw_clipper_init(struct ddraw_clipper *clipper);
struct ddraw_clipper *unsafe_impl_from_IDirectDrawClipper(IDirectDrawClipper *iface);
BOOL ddraw_clipper_is_valid(const struct ddraw_clipper *clipper);

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 *****************************************************************************/
struct ddraw_palette
{
    /* IUnknown fields */
    IDirectDrawPalette IDirectDrawPalette_iface;
    LONG ref;

    struct wined3d_palette *wined3d_palette;
    struct ddraw *ddraw;
    IUnknown *ifaceToRelease;
    DWORD flags;
};

static inline struct ddraw_palette *impl_from_IDirectDrawPalette(IDirectDrawPalette *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_palette, IDirectDrawPalette_iface);
}

struct ddraw_palette *unsafe_impl_from_IDirectDrawPalette(IDirectDrawPalette *iface);

HRESULT ddraw_palette_init(struct ddraw_palette *palette,
        struct ddraw *ddraw, DWORD flags, PALETTEENTRY *entries);

/* Helper structures */
struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, REFIID riid,
                                 void **ppObj);
};

/******************************************************************************
 * IDirect3DLight implementation structure - Wraps to D3D7
 ******************************************************************************/
struct d3d_light
{
    IDirect3DLight IDirect3DLight_iface;
    LONG ref;

    /* IDirect3DLight fields */
    struct ddraw *ddraw;

    /* If this light is active for one viewport, put the viewport here */
    struct d3d_viewport *active_viewport;

    D3DLIGHT2 light;
    D3DLIGHT7 light7;

    DWORD active_light_index;

    struct list entry;
};

/* Helper functions */
void light_activate(struct d3d_light *light);
void light_deactivate(struct d3d_light *light);
void d3d_light_init(struct d3d_light *light, struct ddraw *ddraw);
struct d3d_light *unsafe_impl_from_IDirect3DLight(IDirect3DLight *iface);

/******************************************************************************
 * IDirect3DMaterial implementation structure - Wraps to D3D7
 ******************************************************************************/
struct d3d_material
{
    IDirect3DMaterial3 IDirect3DMaterial3_iface;
    IDirect3DMaterial2 IDirect3DMaterial2_iface;
    IDirect3DMaterial IDirect3DMaterial_iface;
    LONG  ref;

    /* IDirect3DMaterial2 fields */
    struct ddraw *ddraw;
    struct d3d_device *active_device;

    D3DMATERIAL mat;
    DWORD Handle;
};

/* Helper functions */
void material_activate(struct d3d_material *material);
struct d3d_material *d3d_material_create(struct ddraw *ddraw);

enum ddraw_viewport_version
{
    DDRAW_VIEWPORT_VERSION_NONE,
    DDRAW_VIEWPORT_VERSION_1,
    DDRAW_VIEWPORT_VERSION_2,
};

/*****************************************************************************
 * IDirect3DViewport - Wraps to D3D7
 *****************************************************************************/
struct d3d_viewport
{
    IDirect3DViewport3 IDirect3DViewport3_iface;
    LONG ref;

    /* IDirect3DViewport fields */
    struct ddraw *ddraw;

    /* If this viewport is active for one device, put the device here */
    struct d3d_device *active_device;

    DWORD                     active_lights_count;
    DWORD                     map_lights;

    enum ddraw_viewport_version version;

    union
    {
        D3DVIEWPORT vp1;
        D3DVIEWPORT2 vp2;
    } viewports;

    struct list entry;
    struct list light_list;
    struct d3d_material *background;
};

struct d3d_viewport *unsafe_impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface);
struct d3d_viewport *unsafe_impl_from_IDirect3DViewport2(IDirect3DViewport2 *iface);
struct d3d_viewport *unsafe_impl_from_IDirect3DViewport(IDirect3DViewport *iface);

/* Helper functions */
void viewport_activate(struct d3d_viewport *viewport, BOOL ignore_lights);
void viewport_deactivate(struct d3d_viewport *viewport);
void d3d_viewport_init(struct d3d_viewport *viewport, struct ddraw *ddraw);

/*****************************************************************************
 * IDirect3DExecuteBuffer - Wraps to D3D7
 *****************************************************************************/
struct d3d_execute_buffer
{
    IDirect3DExecuteBuffer IDirect3DExecuteBuffer_iface;
    LONG ref;
    /* IDirect3DExecuteBuffer fields */
    struct ddraw *ddraw;
    struct d3d_device *d3ddev;

    D3DEXECUTEBUFFERDESC desc;
    D3DEXECUTEDATA       data;

    /* This buffer will store the transformed vertices */
    unsigned int         index_size, index_pos;
    unsigned int         vertex_size, src_vertex_pos;
    struct wined3d_buffer *src_vertex_buffer, *dst_vertex_buffer, *index_buffer;

    /* This flags is set to TRUE if we allocated ourselves the
     * data buffer
     */
    BOOL                 need_free;
};

HRESULT d3d_execute_buffer_init(struct d3d_execute_buffer *execute_buffer,
        struct d3d_device *device, D3DEXECUTEBUFFERDESC *desc);
struct d3d_execute_buffer *unsafe_impl_from_IDirect3DExecuteBuffer(IDirect3DExecuteBuffer *iface);

/* The execute function */
HRESULT d3d_execute_buffer_execute(struct d3d_execute_buffer *execute_buffer,
        struct d3d_device *device);

/*****************************************************************************
 * IDirect3DVertexBuffer
 *****************************************************************************/
struct d3d_vertex_buffer
{
    IDirect3DVertexBuffer7 IDirect3DVertexBuffer7_iface;
    LONG ref;
    unsigned int version;

    /*** WineD3D and ddraw links ***/
    struct wined3d_buffer *wined3d_buffer;
    struct wined3d_vertex_declaration *wined3d_declaration;
    struct ddraw *ddraw;

    /*** Storage for D3D7 specific things ***/
    DWORD                Caps;
    DWORD                fvf;
    DWORD                size;
    BOOL                 dynamic;
    bool discarded;
    bool sysmem;
};

HRESULT d3d_vertex_buffer_create(struct d3d_vertex_buffer **buffer, struct ddraw *ddraw,
        D3DVERTEXBUFFERDESC *desc);
struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer(IDirect3DVertexBuffer *iface);
struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface);

/*****************************************************************************
 * Helper functions from utils.c
 *****************************************************************************/

#define GET_TEXCOUNT_FROM_FVF(d3dvtVertexType) \
    (((d3dvtVertexType) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

void ddrawformat_from_wined3dformat(DDPIXELFORMAT *ddraw_format,
        enum wined3d_format_id wined3d_format);
BOOL wined3d_colour_from_ddraw_colour(const DDPIXELFORMAT *pf, const struct ddraw_palette *palette,
        DWORD colour, struct wined3d_color *wined3d_colour);
enum wined3d_format_id wined3dformat_from_ddrawformat(const DDPIXELFORMAT *format);
unsigned int wined3dmapflags_from_ddrawmapflags(unsigned int flags);
void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd);
void dump_D3DMATRIX(const D3DMATRIX *mat);
void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps);
DWORD get_flexible_vertex_size(DWORD d3dvtVertexType);
void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in);
void DDRAW_dump_cooperativelevel(DWORD cooplevel);
void DDSD_to_DDSD2(const DDSURFACEDESC *in, DDSURFACEDESC2 *out);
void DDSD2_to_DDSD(const DDSURFACEDESC2 *in, DDSURFACEDESC *out);

void multiply_matrix(struct wined3d_matrix *dst, const struct wined3d_matrix *src1,
        const struct wined3d_matrix *src2);

static inline BOOL format_is_compressed(const DDPIXELFORMAT *format)
{
    return (format->dwFlags & DDPF_FOURCC) && (format->dwFourCC == WINED3DFMT_DXT1
            || format->dwFourCC == WINED3DFMT_DXT2 || format->dwFourCC == WINED3DFMT_DXT3
            || format->dwFourCC == WINED3DFMT_DXT4 || format->dwFourCC == WINED3DFMT_DXT5);
}

static inline BOOL format_is_paletteindexed(const DDPIXELFORMAT *fmt)
{
    DWORD flags = DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4
            | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8;
    return !!(fmt->dwFlags & flags);
}

static inline BOOL ddraw_surface_can_be_lost(const struct ddraw_surface *surface)
{
    DWORD caps = surface->surface_desc.ddsCaps.dwCaps;

    /* Testing with DDCREATE_EMULATIONONLY showed that primary surfaces and Z buffers can
     * be lost even if created with explicit DDCAPS_SYSTEMMEMORY. Textures can or cannot be lost
     * depending on whether _SYSTEMMEMORY was given explicitly by the application. */
    if (!(caps & DDSCAPS_SYSTEMMEMORY) || caps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_ZBUFFER))
        return TRUE;

    return surface->sysmem_fallback;
}

#define DDRAW_SURFACE_READ   0x00000001
#define DDRAW_SURFACE_WRITE  0x00000002
#define DDRAW_SURFACE_RW (DDRAW_SURFACE_READ | DDRAW_SURFACE_WRITE)

static inline struct wined3d_texture *ddraw_surface_get_default_texture(struct ddraw_surface *surface, unsigned int flags)
{
    if (surface->draw_texture)
    {
        if (flags & DDRAW_SURFACE_READ && !(surface->texture_location & DDRAW_SURFACE_LOCATION_DEFAULT))
        {
            wined3d_device_context_copy_sub_resource_region(surface->ddraw->immediate_context,
                    wined3d_texture_get_resource(surface->wined3d_texture), surface->sub_resource_idx, 0, 0, 0,
                    wined3d_texture_get_resource(surface->draw_texture), surface->sub_resource_idx, NULL, 0);
            surface->texture_location |= DDRAW_SURFACE_LOCATION_DEFAULT;
        }

        if (flags & DDRAW_SURFACE_WRITE)
            surface->texture_location = DDRAW_SURFACE_LOCATION_DEFAULT;
    }
    return surface->wined3d_texture;
}

static inline struct wined3d_texture *ddraw_surface_get_draw_texture(struct ddraw_surface *surface, unsigned int flags)
{
    if (!surface->draw_texture)
        return surface->wined3d_texture;

    if (flags & DDRAW_SURFACE_READ && !(surface->texture_location & DDRAW_SURFACE_LOCATION_DRAW))
    {
        wined3d_device_context_copy_sub_resource_region(surface->ddraw->immediate_context,
                wined3d_texture_get_resource(surface->draw_texture), surface->sub_resource_idx, 0, 0, 0,
                wined3d_texture_get_resource(surface->wined3d_texture), surface->sub_resource_idx, NULL, 0);
        surface->texture_location |= DDRAW_SURFACE_LOCATION_DRAW;
    }

    if (flags & DDRAW_SURFACE_WRITE)
        surface->texture_location = DDRAW_SURFACE_LOCATION_DRAW;

    return surface->draw_texture;
}

static inline struct wined3d_texture *ddraw_surface_get_any_texture(struct ddraw_surface *surface, unsigned int flags)
{
    if ((surface->texture_location & DDRAW_SURFACE_LOCATION_DEFAULT)
            || (surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
        return ddraw_surface_get_default_texture(surface, flags);

    assert(surface->texture_location & DDRAW_SURFACE_LOCATION_DRAW);
    return ddraw_surface_get_draw_texture(surface, flags);
}

void d3d_device_sync_surfaces(struct d3d_device *device);
void d3d_device_apply_state(struct d3d_device *device, BOOL clear_state);

/* Used for generic dumping */
struct flag_info
{
    DWORD val;
    const char *name;
};

#define FE(x) { x, #x }

struct member_info
{
    DWORD val;
    const char *name;
    void (*func)(const void *);
    ptrdiff_t offset;
};

/* Structure copy */
#define ME(x,f,e) { x, #x, (void (*)(const void *))(f), offsetof(STRUCT, e) }

#define DD_STRUCT_COPY_BYSIZE_(to,from,to_size,from_size)         \
    do {                                                          \
        DWORD __size = (to)->dwSize;                              \
        DWORD __resetsize = min(to_size, sizeof(*to));            \
        DWORD __copysize = min(__resetsize, from_size);           \
        assert(to != from);                                       \
        memcpy(to, from, __copysize);                             \
        memset((char*)(to) + __copysize, 0, __resetsize - __copysize); \
        (to)->dwSize = __size; /* restore size */                 \
    } while (0)

#define DD_STRUCT_COPY_BYSIZE(to,from) DD_STRUCT_COPY_BYSIZE_(to,from,(to)->dwSize,(from)->dwSize)

HRESULT hr_ddraw_from_wined3d(HRESULT hr);

void viewport_alloc_active_light_index(struct d3d_light *light);
void viewport_free_active_light_index(struct d3d_light *light);

#endif
