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
#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
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
#include "legacy.h"

extern const struct wined3d_parent_ops ddraw_surface_wined3d_parent_ops DECLSPEC_HIDDEN;
extern const struct wined3d_parent_ops ddraw_null_wined3d_parent_ops DECLSPEC_HIDDEN;

/* Typdef the interfaces */
typedef struct IDirectDrawImpl            IDirectDrawImpl;
typedef struct IDirectDrawSurfaceImpl     IDirectDrawSurfaceImpl;
typedef struct IDirectDrawPaletteImpl     IDirectDrawPaletteImpl;
typedef struct IDirect3DDeviceImpl        IDirect3DDeviceImpl;
typedef struct IDirect3DLightImpl         IDirect3DLightImpl;
typedef struct IDirect3DViewportImpl      IDirect3DViewportImpl;
typedef struct IDirect3DMaterialImpl      IDirect3DMaterialImpl;
typedef struct IDirect3DExecuteBufferImpl IDirect3DExecuteBufferImpl;
typedef struct IDirect3DVertexBufferImpl  IDirect3DVertexBufferImpl;

extern DWORD force_refresh_rate DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectDraw implementation structure
 *****************************************************************************/
struct FvfToDecl
{
    DWORD fvf;
    struct wined3d_vertex_declaration *decl;
};

struct IDirectDrawImpl
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
    BOOL initialized;

    struct wined3d *wined3d;
    struct wined3d_device *wined3d_device;
    BOOL                    d3d_initialized;

    IDirectDrawSurfaceImpl *primary;
    RECT primary_lock;
    struct wined3d_surface *wined3d_frontbuffer;
    struct wined3d_swapchain *wined3d_swapchain;
    HWND swapchain_window;

    /* DirectDraw things, which are not handled by WineD3D */
    DWORD                   cooperative_level;

    DWORD                   orig_width, orig_height;
    DWORD                   orig_bpp;

    /* D3D things */
    HWND                    d3d_window;
    IDirect3DDeviceImpl     *d3ddevice;
    int                     d3dversion;

    /* Various HWNDs */
    HWND                    focuswindow;
    HWND                    devicewindow;
    HWND                    dest_window;

    /* Helpers for surface creation */
    IDirectDrawSurfaceImpl *tex_root;

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

HRESULT ddraw_init(IDirectDrawImpl *ddraw, enum wined3d_device_type device_type) DECLSPEC_HIDDEN;
void ddraw_destroy_swapchain(IDirectDrawImpl *ddraw) DECLSPEC_HIDDEN;

static inline void ddraw_set_swapchain_window(struct IDirectDrawImpl *ddraw, HWND window)
{
    if (window == GetDesktopWindow())
        window = NULL;
    ddraw->swapchain_window = window;
}

/* Utility functions */
void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS *pIn, DDSCAPS2 *pOut) DECLSPEC_HIDDEN;
void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2 *pIn, DDDEVICEIDENTIFIER *pOut) DECLSPEC_HIDDEN;
struct wined3d_vertex_declaration *ddraw_find_decl(IDirectDrawImpl *This, DWORD fvf) DECLSPEC_HIDDEN;

/* The default surface type */
extern WINED3DSURFTYPE DefaultSurfaceType DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectDrawSurface implementation structure
 *****************************************************************************/

struct IDirectDrawSurfaceImpl
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

    int                     version;

    /* Connections to other Objects */
    IDirectDrawImpl         *ddraw;
    struct wined3d_surface *wined3d_surface;
    struct wined3d_texture *wined3d_texture;

    /* This implementation handles attaching surfaces to other surfaces */
    IDirectDrawSurfaceImpl  *next_attached;
    IDirectDrawSurfaceImpl  *first_attached;
    IUnknown                *attached_iface;

    /* Complex surfaces are organized in a tree, although the tree is degenerated to a list in most cases.
     * In mipmap and primary surfaces each level has only one attachment, which is the next surface level.
     * Only the cube texture root has 6 surfaces attached, which then have a normal mipmap chain attached
     * to them. So hardcode the array to 6, a dynamic array or a list would be an overkill.
     */
#define MAX_COMPLEX_ATTACHED 6
    IDirectDrawSurfaceImpl  *complex_array[MAX_COMPLEX_ATTACHED];
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

    /* For the ddraw surface list */
    struct list             surface_list_entry;

    DWORD                   Handle;
};

HRESULT ddraw_surface_create_texture(IDirectDrawSurfaceImpl *surface) DECLSPEC_HIDDEN;
HRESULT ddraw_surface_init(IDirectDrawSurfaceImpl *surface, IDirectDrawImpl *ddraw,
        DDSURFACEDESC2 *desc, UINT mip_level, UINT version) DECLSPEC_HIDDEN;
ULONG ddraw_surface_release_iface(IDirectDrawSurfaceImpl *This) DECLSPEC_HIDDEN;

static inline IDirectDrawSurfaceImpl *impl_from_IDirect3DTexture(IDirect3DTexture *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirect3DTexture_iface);
}

static inline IDirectDrawSurfaceImpl *impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirect3DTexture2_iface);
}

static inline IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface(IDirectDrawSurface *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface_iface);
}

static inline IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface2(IDirectDrawSurface2 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface2_iface);
}

static inline IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface3_iface);
}

static inline IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface4_iface);
}

static inline IDirectDrawSurfaceImpl *impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawSurfaceImpl, IDirectDrawSurface7_iface);
}

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface(IDirectDrawSurface *iface) DECLSPEC_HIDDEN;
IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface) DECLSPEC_HIDDEN;
IDirectDrawSurfaceImpl *unsafe_impl_from_IDirectDrawSurface7(IDirectDrawSurface7 *iface) DECLSPEC_HIDDEN;

IDirectDrawSurfaceImpl *unsafe_impl_from_IDirect3DTexture(IDirect3DTexture *iface) DECLSPEC_HIDDEN;
IDirectDrawSurfaceImpl *unsafe_impl_from_IDirect3DTexture2(IDirect3DTexture2 *iface) DECLSPEC_HIDDEN;

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

struct IDirect3DDeviceImpl
{
    /* IUnknown */
    IDirect3DDevice7 IDirect3DDevice7_iface;
    IDirect3DDevice3 IDirect3DDevice3_iface;
    IDirect3DDevice2 IDirect3DDevice2_iface;
    IDirect3DDevice IDirect3DDevice_iface;
    LONG                    ref;

    /* Other object connections */
    struct wined3d_device *wined3d_device;
    IDirectDrawImpl         *ddraw;
    struct wined3d_buffer *indexbuffer;
    IDirectDrawSurfaceImpl  *target;

    /* Viewport management */
    struct list viewport_list;
    IDirect3DViewportImpl *current_viewport;
    D3DVIEWPORT7 active_viewport;

    /* Required to keep track which of two available texture blending modes in d3ddevice3 is used */
    BOOL legacyTextureBlending;
    BOOL from_surface;

    D3DMATRIX legacy_projection;
    D3DMATRIX legacy_clipspace;

    /* Light state */
    DWORD material;

    /* Rendering functions to wrap D3D(1-3) to D3D7 */
    D3DPRIMITIVETYPE primitive_type;
    DWORD vertex_type;
    DWORD render_flags;
    DWORD nb_vertices;
    LPBYTE vertex_buffer;
    DWORD vertex_size;
    DWORD buffer_size;

    /* Handle management */
    struct ddraw_handle_table handle_table;
    D3DMATRIXHANDLE          world, proj, view;
};

HRESULT d3d_device_init(IDirect3DDeviceImpl *device, IDirectDrawImpl *ddraw,
        IDirectDrawSurfaceImpl *target) DECLSPEC_HIDDEN;

/* The IID */
extern const GUID IID_D3DDEVICE_WineD3D DECLSPEC_HIDDEN;

/* Helper functions */
HRESULT IDirect3DImpl_GetCaps(const struct wined3d *wined3d,
        D3DDEVICEDESC *Desc123, D3DDEVICEDESC7 *Desc7) DECLSPEC_HIDDEN;
enum wined3d_depth_buffer_type IDirect3DDeviceImpl_UpdateDepthStencil(IDirect3DDeviceImpl *device) DECLSPEC_HIDDEN;

static inline IDirect3DDeviceImpl *impl_from_IDirect3DDevice(IDirect3DDevice *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice_iface);
}

static inline IDirect3DDeviceImpl *impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice2_iface);
}

static inline IDirect3DDeviceImpl *impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice3_iface);
}

static inline IDirect3DDeviceImpl *impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice7_iface);
}

IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice(IDirect3DDevice *iface) DECLSPEC_HIDDEN;
IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface) DECLSPEC_HIDDEN;
IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface) DECLSPEC_HIDDEN;
IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface) DECLSPEC_HIDDEN;

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
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    IDirectDrawPalette IDirectDrawPalette_iface;
    LONG ref;

    struct wined3d_palette *wineD3DPalette;

    /* IDirectDrawPalette fields */
    IUnknown                  *ifaceToRelease;
};

static inline IDirectDrawPaletteImpl *impl_from_IDirectDrawPalette(IDirectDrawPalette *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawPaletteImpl, IDirectDrawPalette_iface);
}

IDirectDrawPaletteImpl *unsafe_impl_from_IDirectDrawPalette(IDirectDrawPalette *iface) DECLSPEC_HIDDEN;

HRESULT ddraw_palette_init(IDirectDrawPaletteImpl *palette,
        IDirectDrawImpl *ddraw, DWORD flags, PALETTEENTRY *entries) DECLSPEC_HIDDEN;

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
struct IDirect3DLightImpl
{
    IDirect3DLight IDirect3DLight_iface;
    LONG ref;

    /* IDirect3DLight fields */
    IDirectDrawImpl           *ddraw;

    /* If this light is active for one viewport, put the viewport here */
    IDirect3DViewportImpl     *active_viewport;

    D3DLIGHT2 light;
    D3DLIGHT7 light7;

    DWORD dwLightIndex;

    struct list entry;
};

/* Helper functions */
void light_activate(IDirect3DLightImpl *light) DECLSPEC_HIDDEN;
void light_deactivate(IDirect3DLightImpl *light) DECLSPEC_HIDDEN;
void d3d_light_init(IDirect3DLightImpl *light, IDirectDrawImpl *ddraw) DECLSPEC_HIDDEN;
IDirect3DLightImpl *unsafe_impl_from_IDirect3DLight(IDirect3DLight *iface) DECLSPEC_HIDDEN;

/******************************************************************************
 * IDirect3DMaterial implementation structure - Wraps to D3D7
 ******************************************************************************/
struct IDirect3DMaterialImpl
{
    IDirect3DMaterial3 IDirect3DMaterial3_iface;
    IDirect3DMaterial2 IDirect3DMaterial2_iface;
    IDirect3DMaterial IDirect3DMaterial_iface;
    LONG  ref;

    /* IDirect3DMaterial2 fields */
    IDirectDrawImpl               *ddraw;
    IDirect3DDeviceImpl           *active_device;

    D3DMATERIAL mat;
    DWORD Handle;
};

/* Helper functions */
void material_activate(IDirect3DMaterialImpl* This) DECLSPEC_HIDDEN;
IDirect3DMaterialImpl *d3d_material_create(IDirectDrawImpl *ddraw) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DViewport - Wraps to D3D7
 *****************************************************************************/
struct IDirect3DViewportImpl
{
    IDirect3DViewport3 IDirect3DViewport3_iface;
    LONG ref;

    /* IDirect3DViewport fields */
    IDirectDrawImpl           *ddraw;

    /* If this viewport is active for one device, put the device here */
    IDirect3DDeviceImpl       *active_device;

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

    /* Background material */
    IDirect3DMaterialImpl     *background;
};

IDirect3DViewportImpl *unsafe_impl_from_IDirect3DViewport3(IDirect3DViewport3 *iface) DECLSPEC_HIDDEN;
IDirect3DViewportImpl *unsafe_impl_from_IDirect3DViewport2(IDirect3DViewport2 *iface) DECLSPEC_HIDDEN;
IDirect3DViewportImpl *unsafe_impl_from_IDirect3DViewport(IDirect3DViewport *iface) DECLSPEC_HIDDEN;

/* Helper functions */
void viewport_activate(IDirect3DViewportImpl* This, BOOL ignore_lights) DECLSPEC_HIDDEN;
void d3d_viewport_init(IDirect3DViewportImpl *viewport, IDirectDrawImpl *ddraw) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DExecuteBuffer - Wraps to D3D7
 *****************************************************************************/
struct IDirect3DExecuteBufferImpl
{
    IDirect3DExecuteBuffer IDirect3DExecuteBuffer_iface;
    LONG ref;
    /* IDirect3DExecuteBuffer fields */
    IDirectDrawImpl      *ddraw;
    IDirect3DDeviceImpl  *d3ddev;

    D3DEXECUTEBUFFERDESC desc;
    D3DEXECUTEDATA       data;

    /* This buffer will store the transformed vertices */
    void                 *vertex_data;
    WORD                 *indices;
    int                  nb_indices;

    /* This flags is set to TRUE if we allocated ourselves the
     * data buffer
     */
    BOOL                 need_free;
};

HRESULT d3d_execute_buffer_init(IDirect3DExecuteBufferImpl *execute_buffer,
        IDirect3DDeviceImpl *device, D3DEXECUTEBUFFERDESC *desc) DECLSPEC_HIDDEN;
IDirect3DExecuteBufferImpl *unsafe_impl_from_IDirect3DExecuteBuffer(IDirect3DExecuteBuffer *iface) DECLSPEC_HIDDEN;

/* The execute function */
HRESULT d3d_execute_buffer_execute(IDirect3DExecuteBufferImpl *execute_buffer,
        IDirect3DDeviceImpl *device, IDirect3DViewportImpl *viewport) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DVertexBuffer
 *****************************************************************************/
struct IDirect3DVertexBufferImpl
{
    IDirect3DVertexBuffer7 IDirect3DVertexBuffer7_iface;
    IDirect3DVertexBuffer IDirect3DVertexBuffer_iface;
    LONG ref;

    /*** WineD3D and ddraw links ***/
    struct wined3d_buffer *wineD3DVertexBuffer;
    struct wined3d_vertex_declaration *wineD3DVertexDeclaration;
    IDirectDrawImpl *ddraw;

    /*** Storage for D3D7 specific things ***/
    DWORD                Caps;
    DWORD                fvf;
};

HRESULT d3d_vertex_buffer_create(IDirect3DVertexBufferImpl **vertex_buf, IDirectDrawImpl *ddraw,
        D3DVERTEXBUFFERDESC *desc) DECLSPEC_HIDDEN;
IDirect3DVertexBufferImpl *unsafe_impl_from_IDirect3DVertexBuffer(IDirect3DVertexBuffer *iface) DECLSPEC_HIDDEN;
IDirect3DVertexBufferImpl *unsafe_impl_from_IDirect3DVertexBuffer7(IDirect3DVertexBuffer7 *iface) DECLSPEC_HIDDEN;

/*****************************************************************************
 * Helper functions from utils.c
 *****************************************************************************/

#define GET_TEXCOUNT_FROM_FVF(d3dvtVertexType) \
    (((d3dvtVertexType) & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT)

#define GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_num) \
    (((((d3dvtVertexType) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1)

void PixelFormat_WineD3DtoDD(DDPIXELFORMAT *DDPixelFormat, enum wined3d_format_id WineD3DFormat) DECLSPEC_HIDDEN;
enum wined3d_format_id PixelFormat_DD2WineD3D(const DDPIXELFORMAT *DDPixelFormat) DECLSPEC_HIDDEN;
void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd) DECLSPEC_HIDDEN;
void dump_D3DMATRIX(const D3DMATRIX *mat) DECLSPEC_HIDDEN;
void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps) DECLSPEC_HIDDEN;
DWORD get_flexible_vertex_size(DWORD d3dvtVertexType) DECLSPEC_HIDDEN;
void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in) DECLSPEC_HIDDEN;
void DDRAW_dump_cooperativelevel(DWORD cooplevel) DECLSPEC_HIDDEN;
void DDSD_to_DDSD2(const DDSURFACEDESC *in, DDSURFACEDESC2 *out) DECLSPEC_HIDDEN;
void DDSD2_to_DDSD(const DDSURFACEDESC2 *in, DDSURFACEDESC *out) DECLSPEC_HIDDEN;

/* This only needs to be here as long the processvertices functionality of
 * IDirect3DExecuteBuffer isn't in WineD3D */
void multiply_matrix(LPD3DMATRIX dest, const D3DMATRIX *src1, const D3DMATRIX *src2) DECLSPEC_HIDDEN;

/* Used for generic dumping */
typedef struct
{
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }

typedef struct
{
    DWORD val;
    const char* name;
    void (*func)(const void *);
    ptrdiff_t offset;
} member_info;

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

#define SIZEOF_END_PADDING(type, last_field) \
    (sizeof(type) - offsetof(type, last_field) - sizeof(((type *)0)->last_field))

static inline void copy_to_surfacedesc2(DDSURFACEDESC2 *to, DDSURFACEDESC2 *from)
{
    DWORD from_size = from->dwSize;
    if (from_size == sizeof(DDSURFACEDESC))
        from_size -= SIZEOF_END_PADDING(DDSURFACEDESC, ddsCaps);
    to->dwSize = sizeof(DDSURFACEDESC2); /* for struct copy */
    DD_STRUCT_COPY_BYSIZE_(to, from, from_size);
}

HRESULT hr_ddraw_from_wined3d(HRESULT hr) DECLSPEC_HIDDEN;

#endif
