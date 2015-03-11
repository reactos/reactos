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

#include <config.h>
#include <wine/port.h>

#include <assert.h>
#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOW_H

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <d3d.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/wined3d.h>

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

extern const struct wined3d_parent_ops ddraw_null_wined3d_parent_ops DECLSPEC_HIDDEN;
extern DWORD force_refresh_rate DECLSPEC_HIDDEN;

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

#define DDRAW_STRIDE_ALIGNMENT  8

enum ddraw_device_state
{
    DDRAW_DEVICE_STATE_OK,
    DDRAW_DEVICE_STATE_LOST,
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
    struct wined3d_device *wined3d_device;
    DWORD flags;
    LONG device_state;

    struct ddraw_surface *primary;
    RECT primary_lock;
    struct wined3d_surface *wined3d_frontbuffer;
    struct wined3d_swapchain *wined3d_swapchain;
    HWND swapchain_window;

    /* DirectDraw things, which are not handled by WineD3D */
    DWORD                   cooperative_level;

    /* D3D things */
    HWND                    d3d_window;
    struct d3d_device *d3ddevice;
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
};

#define DDRAW_WINDOW_CLASS_NAME "DirectDrawDeviceWnd"

HRESULT ddraw_init(struct ddraw *ddraw, enum wined3d_device_type device_type) DECLSPEC_HIDDEN;
void ddraw_d3dcaps1_from_7(D3DDEVICEDESC *caps1, D3DDEVICEDESC7 *caps7) DECLSPEC_HIDDEN;
void ddraw_destroy_swapchain(struct ddraw *ddraw) DECLSPEC_HIDDEN;
HRESULT ddraw_get_d3dcaps(const struct ddraw *ddraw, D3DDEVICEDESC7 *caps) DECLSPEC_HIDDEN;

static inline void ddraw_set_swapchain_window(struct ddraw *ddraw, HWND window)
{
    if (window == GetDesktopWindow())
        window = NULL;
    ddraw->swapchain_window = window;
}

/* Utility functions */
void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS *pIn, DDSCAPS2 *pOut) DECLSPEC_HIDDEN;
void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2 *pIn, DDDEVICEIDENTIFIER *pOut) DECLSPEC_HIDDEN;
struct wined3d_vertex_declaration *ddraw_find_decl(struct ddraw *ddraw, DWORD fvf) DECLSPEC_HIDDEN;

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
    struct wined3d_surface *wined3d_surface;
    struct wined3d_texture *wined3d_texture;
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
    /* You can't traverse the tree upwards. Only a flag for Surface::Release because its needed there,
     * but no pointer to prevent temptations to traverse it in the wrong direction.
     */
    BOOL                    is_complex_root;

    /* Surface description, for GetAttachedSurface */
    DDSURFACEDESC2          surface_desc;

    /* Misc things */
    DWORD                   uniqueness_value;
    UINT                    mipmap_level;

    /* Clipper objects */
    struct ddraw_clipper *clipper;
    struct ddraw_palette *palette;

    /* For the ddraw surface list */
    struct list             surface_list_entry;

    DWORD                   Handle;
};

struct ddraw_texture
{
    unsigned int version;
    DDSURFACEDESC2 surface_desc;

    struct ddraw_surface *root;
};

HRESULT ddraw_surface_create(struct ddraw *ddraw, const DDSURFACEDESC2 *surface_desc,
        struct ddraw_surface **surface, IUnknown *outer_unknown, unsigned int version) DECLSPEC_HIDDEN;
struct wined3d_rendertarget_view *ddraw_surface_get_rendertarget_view(struct ddraw_surface *surface) DECLSPEC_HIDDEN;
void ddraw_surface_init(struct ddraw_surface *surface, struct ddraw *ddraw, struct ddraw_texture *texture,
        struct wined3d_surface *wined3d_surface, const struct wined3d_parent_ops **parent_ops) DECLSPEC_HIDDEN;
ULONG ddraw_surface_release_iface(struct ddraw_surface *This) DECLSPEC_HIDDEN;
HRESULT ddraw_surface_update_frontbuffer(struct ddraw_surface *surface,
        const RECT *rect, BOOL read) DECLSPEC_HIDDEN;

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

struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface(IDirectDrawSurface *iface) DECLSPEC_HIDDEN;
struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface) DECLSPEC_HIDDEN;
struct ddraw_surface *unsafe_impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface) DECLSPEC_HIDDEN;

struct ddraw_surface *unsafe_impl_from_IDirect3DTexture(IDirect3DTexture *iface) DECLSPEC_HIDDEN;
struct ddraw_surface *unsafe_impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface) DECLSPEC_HIDDEN;

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

BOOL ddraw_handle_table_init(struct ddraw_handle_table *t, UINT initial_size) DECLSPEC_HIDDEN;
void ddraw_handle_table_destroy(struct ddraw_handle_table *t) DECLSPEC_HIDDEN;
DWORD ddraw_allocate_handle(struct ddraw_handle_table *t, void *object, enum ddraw_handle_type type) DECLSPEC_HIDDEN;
void *ddraw_free_handle(struct ddraw_handle_table *t, DWORD handle, enum ddraw_handle_type type) DECLSPEC_HIDDEN;
void *ddraw_get_object(struct ddraw_handle_table *t, DWORD handle, enum ddraw_handle_type type) DECLSPEC_HIDDEN;

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

    IUnknown *outer_unknown;
    struct wined3d_device *wined3d_device;
    struct ddraw *ddraw;
    IUnknown *rt_iface;

    struct wined3d_buffer *index_buffer;
    UINT index_buffer_size;
    UINT index_buffer_pos;

    struct wined3d_buffer *vertex_buffer;
    UINT vertex_buffer_size;
    UINT vertex_buffer_pos;

    /* Viewport management */
    struct list viewport_list;
    struct d3d_viewport *current_viewport;
    D3DVIEWPORT7 active_viewport;

    /* Required to keep track which of two available texture blending modes in d3ddevice3 is used */
    BOOL legacyTextureBlending;

    D3DMATRIX legacy_projection;
    D3DMATRIX legacy_clipspace;

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
};

HRESULT d3d_device_create(struct ddraw *ddraw, struct ddraw_surface *target, IUnknown *rt_iface,
        UINT version, struct d3d_device **device, IUnknown *outer_unknown) DECLSPEC_HIDDEN;
enum wined3d_depth_buffer_type d3d_device_update_depth_stencil(struct d3d_device *device) DECLSPEC_HIDDEN;

/* The IID */
extern const GUID IID_D3DDEVICE_WineD3D DECLSPEC_HIDDEN;

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

struct d3d_device *unsafe_impl_from_IDirect3DDevice(IDirect3DDevice *iface) DECLSPEC_HIDDEN;
struct d3d_device *unsafe_impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface) DECLSPEC_HIDDEN;
struct d3d_device *unsafe_impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface) DECLSPEC_HIDDEN;
struct d3d_device *unsafe_impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface) DECLSPEC_HIDDEN;

struct ddraw_clipper
{
    IDirectDrawClipper IDirectDrawClipper_iface;
    LONG ref;
    HWND window;
    HRGN region;
    BOOL initialized;
};

HRESULT ddraw_clipper_init(struct ddraw_clipper *clipper) DECLSPEC_HIDDEN;
struct ddraw_clipper *unsafe_impl_from_IDirectDrawClipper(IDirectDrawClipper *iface) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 *****************************************************************************/
struct ddraw_palette
{
    /* IUnknown fields */
    IDirectDrawPalette IDirectDrawPalette_iface;
    LONG ref;

    struct wined3d_palette *wineD3DPalette;
    struct ddraw *ddraw;
    IUnknown *ifaceToRelease;
    DWORD flags;
};

static inline struct ddraw_palette *impl_from_IDirectDrawPalette(IDirectDrawPalette *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_palette, IDirectDrawPalette_iface);
}

struct ddraw_palette *unsafe_impl_from_IDirectDrawPalette(IDirectDrawPalette *iface) DECLSPEC_HIDDEN;

HRESULT ddraw_palette_init(struct ddraw_palette *palette,
        struct ddraw *ddraw, DWORD flags, PALETTEENTRY *entries) DECLSPEC_HIDDEN;

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

    DWORD dwLightIndex;

    struct list entry;
};

/* Helper functions */
void light_activate(struct d3d_light *light) DECLSPEC_HIDDEN;
void light_deactivate(struct d3d_light *light) DECLSPEC_HIDDEN;
void d3d_light_init(struct d3d_light *light, struct ddraw *ddraw) DECLSPEC_HIDDEN;
struct d3d_light *unsafe_impl_from_IDirect3DLight(IDirect3DLight *iface) DECLSPEC_HIDDEN;

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
void material_activate(struct d3d_material *material) DECLSPEC_HIDDEN;
struct d3d_material *d3d_material_create(struct ddraw *ddraw) DECLSPEC_HIDDEN;

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

    DWORD                     num_lights;
    DWORD                     map_lights;

    int                       use_vp2;

    union
    {
        D3DVIEWPORT vp1;
        D3DVIEWPORT2 vp2;
    } viewports;

    struct list entry;
    struct list light_list;
    struct d3d_material *background;
};

struct d3d_viewport *unsafe_impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface) DECLSPEC_HIDDEN;
struct d3d_viewport *unsafe_impl_from_IDirect3DViewport2(IDirect3DViewport2 *iface) DECLSPEC_HIDDEN;
struct d3d_viewport *unsafe_impl_from_IDirect3DViewport(IDirect3DViewport *iface) DECLSPEC_HIDDEN;

/* Helper functions */
void viewport_activate(struct d3d_viewport *viewport, BOOL ignore_lights) DECLSPEC_HIDDEN;
void d3d_viewport_init(struct d3d_viewport *viewport, struct ddraw *ddraw) DECLSPEC_HIDDEN;

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
    void                 *vertex_data;
    WORD                 *indices;
    unsigned int         nb_indices;
    unsigned int         nb_vertices;

    /* This flags is set to TRUE if we allocated ourselves the
     * data buffer
     */
    BOOL                 need_free;
};

HRESULT d3d_execute_buffer_init(struct d3d_execute_buffer *execute_buffer,
        struct d3d_device *device, D3DEXECUTEBUFFERDESC *desc) DECLSPEC_HIDDEN;
struct d3d_execute_buffer *unsafe_impl_from_IDirect3DExecuteBuffer(IDirect3DExecuteBuffer *iface) DECLSPEC_HIDDEN;

/* The execute function */
HRESULT d3d_execute_buffer_execute(struct d3d_execute_buffer *execute_buffer,
        struct d3d_device *device, struct d3d_viewport *viewport) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DVertexBuffer
 *****************************************************************************/
struct d3d_vertex_buffer
{
    IDirect3DVertexBuffer7 IDirect3DVertexBuffer7_iface;
    IDirect3DVertexBuffer IDirect3DVertexBuffer_iface;
    LONG ref;

    /*** WineD3D and ddraw links ***/
    struct wined3d_buffer *wineD3DVertexBuffer;
    struct wined3d_vertex_declaration *wineD3DVertexDeclaration;
    struct ddraw *ddraw;

    /*** Storage for D3D7 specific things ***/
    DWORD                Caps;
    DWORD                fvf;
    DWORD                size;
    BOOL                 dynamic;
};

HRESULT d3d_vertex_buffer_create(struct d3d_vertex_buffer **buffer, struct ddraw *ddraw,
        D3DVERTEXBUFFERDESC *desc) DECLSPEC_HIDDEN;
struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer(IDirect3DVertexBuffer *iface) DECLSPEC_HIDDEN;
struct d3d_vertex_buffer *unsafe_impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Helper functions from utils.c
 *****************************************************************************/

#define GET_TEXCOUNT_FROM_FVF(d3dvtVertexType) \
    (((d3dvtVertexType) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

void ddrawformat_from_wined3dformat(DDPIXELFORMAT *ddraw_format,
        enum wined3d_format_id wined3d_format) DECLSPEC_HIDDEN;
enum wined3d_format_id wined3dformat_from_ddrawformat(const DDPIXELFORMAT *format) DECLSPEC_HIDDEN;
void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd) DECLSPEC_HIDDEN;
void dump_D3DMATRIX(const D3DMATRIX *mat) DECLSPEC_HIDDEN;
void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps) DECLSPEC_HIDDEN;
DWORD get_flexible_vertex_size(DWORD d3dvtVertexType) DECLSPEC_HIDDEN;
void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in) DECLSPEC_HIDDEN;
void DDRAW_dump_cooperativelevel(DWORD cooplevel) DECLSPEC_HIDDEN;
void DDSD_to_DDSD2(const DDSURFACEDESC *in, DDSURFACEDESC2 *out) DECLSPEC_HIDDEN;
void DDSD2_to_DDSD(const DDSURFACEDESC2 *in, DDSURFACEDESC *out) DECLSPEC_HIDDEN;

void multiply_matrix(D3DMATRIX *dst, const D3DMATRIX *src1, const D3DMATRIX *src2) DECLSPEC_HIDDEN;

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

#define DD_STRUCT_COPY_BYSIZE_(to,from,from_size)                 \
    do {                                                          \
        DWORD __size = (to)->dwSize;                              \
        DWORD __resetsize = min(__size, sizeof(*to));             \
        DWORD __copysize = min(__resetsize, from_size);           \
        assert(to != from);                                       \
        memcpy(to, from, __copysize);                             \
        memset((char*)(to) + __copysize, 0, __resetsize - __copysize); \
        (to)->dwSize = __size; /* restore size */                 \
    } while (0)

#define DD_STRUCT_COPY_BYSIZE(to,from) DD_STRUCT_COPY_BYSIZE_(to,from,(from)->dwSize)

HRESULT hr_ddraw_from_wined3d(HRESULT hr) DECLSPEC_HIDDEN;

#endif
