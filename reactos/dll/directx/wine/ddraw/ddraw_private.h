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
typedef struct IDirectDrawClipperImpl     IDirectDrawClipperImpl;
typedef struct IDirectDrawPaletteImpl     IDirectDrawPaletteImpl;
typedef struct IDirect3DDeviceImpl        IDirect3DDeviceImpl;
typedef struct IDirect3DLightImpl         IDirect3DLightImpl;
typedef struct IDirect3DViewportImpl      IDirect3DViewportImpl;
typedef struct IDirect3DMaterialImpl      IDirect3DMaterialImpl;
typedef struct IDirect3DExecuteBufferImpl IDirect3DExecuteBufferImpl;
typedef struct IDirect3DVertexBufferImpl  IDirect3DVertexBufferImpl;

/* Global critical section */
extern CRITICAL_SECTION ddraw_cs DECLSPEC_HIDDEN;

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
    IDirectDraw3 IDirectDraw3_iface;
    IDirectDraw2 IDirectDraw2_iface;
    IDirectDraw IDirectDraw_iface;
    IDirect3D7 IDirect3D7_iface;
    IDirect3D3 IDirect3D3_iface;
    IDirect3D2 IDirect3D2_iface;
    IDirect3D IDirect3D_iface;
    struct wined3d_device_parent device_parent;

    /* See comment in IDirectDraw::AddRef */
    LONG                    ref7, ref4, ref2, ref3, ref1, numIfaces;

    /* WineD3D linkage */
    struct wined3d *wineD3D;
    struct wined3d_device *wined3d_device;
    BOOL                    d3d_initialized;

    /* Misc ddraw fields */
    UINT                    total_vidmem;
    DWORD                   cur_scanline;
    BOOL                    fake_vblank;
    BOOL                    initialized;

    /* DirectDraw things, which are not handled by WineD3D */
    DWORD                   cooperative_level;

    DWORD                   orig_width, orig_height;
    DWORD                   orig_bpp;

    /* D3D things */
    IDirectDrawSurfaceImpl  *d3d_target;
    HWND                    d3d_window;
    IDirect3DDeviceImpl     *d3ddevice;
    int                     d3dversion;

    /* Various HWNDs */
    HWND                    focuswindow;
    HWND                    devicewindow;
    HWND                    dest_window;

    /* The surface type to request */
    WINED3DSURFTYPE         ImplType;

    /* Helpers for surface creation */
    IDirectDrawSurfaceImpl *tex_root;
    BOOL                    depthstencil;

    /* For the dll unload cleanup code */
    struct list ddraw_list_entry;
    /* The surface list - can't relay this to WineD3D
     * because of IParent
     */
    struct list surface_list;
    LONG surfaces;

    /* FVF management */
    struct FvfToDecl       *decls;
    UINT                    numConvertedDecls, declArraySize;
};

#define DDRAW_WINDOW_CLASS_NAME "ddraw_wc"

HRESULT ddraw_init(IDirectDrawImpl *ddraw, WINED3DDEVTYPE device_type) DECLSPEC_HIDDEN;

/* Helper structures */
typedef struct EnumDisplayModesCBS
{
    void *context;
    LPDDENUMMODESCALLBACK2 callback;
} EnumDisplayModesCBS;

typedef struct EnumSurfacesCBS
{
    void *context;
    LPDDENUMSURFACESCALLBACK7 callback;
    LPDDSURFACEDESC2 pDDSD;
    DWORD Flags;
} EnumSurfacesCBS;

/* Utility functions */
void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS *pIn, DDSCAPS2 *pOut) DECLSPEC_HIDDEN;
void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2 *pIn, DDDEVICEIDENTIFIER *pOut) DECLSPEC_HIDDEN;
HRESULT WINAPI ddraw_recreate_surfaces_cb(IDirectDrawSurface7 *surf,
        DDSURFACEDESC2 *desc, void *Context) DECLSPEC_HIDDEN;
struct wined3d_vertex_declaration *ddraw_find_decl(IDirectDrawImpl *This, DWORD fvf) DECLSPEC_HIDDEN;

/* The default surface type */
extern WINED3DSURFTYPE DefaultSurfaceType DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectDrawSurface implementation structure
 *****************************************************************************/

struct IDirectDrawSurfaceImpl
{
    /* IUnknown fields */
    const IDirectDrawSurface7Vtbl *lpVtbl;
    const IDirectDrawSurface3Vtbl *IDirectDrawSurface3_vtbl;
    const IDirectDrawGammaControlVtbl *IDirectDrawGammaControl_vtbl;
    const IDirect3DTexture2Vtbl *IDirect3DTexture2_vtbl;
    const IDirect3DTextureVtbl *IDirect3DTexture_vtbl;

    LONG                     ref;
    IUnknown                *ifaceToRelease;

    int                     version;

    /* Connections to other Objects */
    IDirectDrawImpl         *ddraw;
    struct wined3d_surface *wined3d_surface;
    struct wined3d_texture *wined3d_texture;
    struct wined3d_swapchain *wined3d_swapchain;

    /* This implementation handles attaching surfaces to other surfaces */
    IDirectDrawSurfaceImpl  *next_attached;
    IDirectDrawSurfaceImpl  *first_attached;

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
    WINED3DSURFTYPE         ImplType;

    /* For D3DDevice creation */
    BOOL                    isRenderTarget;

    /* Clipper objects */
    IDirectDrawClipperImpl  *clipper;

    /* For the ddraw surface list */
    struct list             surface_list_entry;

    DWORD                   Handle;
};

HRESULT ddraw_surface_create_texture(IDirectDrawSurfaceImpl *surface) DECLSPEC_HIDDEN;
void ddraw_surface_destroy(IDirectDrawSurfaceImpl *surface) DECLSPEC_HIDDEN;
HRESULT ddraw_surface_init(IDirectDrawSurfaceImpl *surface, IDirectDrawImpl *ddraw,
        DDSURFACEDESC2 *desc, UINT mip_level, WINED3DSURFTYPE surface_type) DECLSPEC_HIDDEN;

static inline IDirectDrawSurfaceImpl *surface_from_texture1(IDirect3DTexture *iface)
{
    return (IDirectDrawSurfaceImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawSurfaceImpl, IDirect3DTexture_vtbl));
}

static inline IDirectDrawSurfaceImpl *surface_from_texture2(IDirect3DTexture2 *iface)
{
    return (IDirectDrawSurfaceImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawSurfaceImpl, IDirect3DTexture2_vtbl));
}

static inline IDirectDrawSurfaceImpl *surface_from_surface3(IDirectDrawSurface3 *iface)
{
    return (IDirectDrawSurfaceImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawSurfaceImpl, IDirectDrawSurface3_vtbl));
}

/* Get the number of bytes per pixel for a given surface */
#define PFGET_BPP(pf) (pf.dwFlags&DDPF_PALETTEINDEXED8?1:((pf.dwRGBBitCount+7)/8))
#define GET_BPP(desc) PFGET_BPP(desc.ddpfPixelFormat)

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
    const IDirect3DDevice7Vtbl *lpVtbl;
    const IDirect3DDevice3Vtbl *IDirect3DDevice3_vtbl;
    const IDirect3DDevice2Vtbl *IDirect3DDevice2_vtbl;
    const IDirect3DDeviceVtbl *IDirect3DDevice_vtbl;
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
WINED3DZBUFFERTYPE IDirect3DDeviceImpl_UpdateDepthStencil(IDirect3DDeviceImpl *This) DECLSPEC_HIDDEN;

static inline IDirect3DDeviceImpl *device_from_device1(IDirect3DDevice *iface)
{
    return (IDirect3DDeviceImpl *)((char*)iface - FIELD_OFFSET(IDirect3DDeviceImpl, IDirect3DDevice_vtbl));
}

static inline IDirect3DDeviceImpl *device_from_device2(IDirect3DDevice2 *iface)
{
    return (IDirect3DDeviceImpl *)((char*)iface - FIELD_OFFSET(IDirect3DDeviceImpl, IDirect3DDevice2_vtbl));
}

static inline IDirect3DDeviceImpl *device_from_device3(IDirect3DDevice3 *iface)
{
    return (IDirect3DDeviceImpl *)((char*)iface - FIELD_OFFSET(IDirect3DDeviceImpl, IDirect3DDevice3_vtbl));
}

/* Structures */
struct EnumTextureFormatsCBS
{
    LPD3DENUMTEXTUREFORMATSCALLBACK cbv2;
    LPD3DENUMPIXELFORMATSCALLBACK cbv7;
    void *Context;
};

/* Structure for EnumZBufferFormats */
struct EnumZBufferFormatsData
{
    LPD3DENUMPIXELFORMATSCALLBACK Callback;
    void *Context;
};

/*****************************************************************************
 * IDirectDrawClipper implementation structure
 *****************************************************************************/
struct IDirectDrawClipperImpl
{
    /* IUnknown fields */
    const IDirectDrawClipperVtbl *lpVtbl;
    LONG ref;

    struct wined3d_clipper *wineD3DClipper;
    BOOL initialized;
};

HRESULT ddraw_clipper_init(IDirectDrawClipperImpl *clipper) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 *****************************************************************************/
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    const IDirectDrawPaletteVtbl *lpVtbl;
    LONG ref;

    struct wined3d_palette *wineD3DPalette;

    /* IDirectDrawPalette fields */
    IUnknown                  *ifaceToRelease;
};

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
    const IDirect3DLightVtbl *lpVtbl;
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

/******************************************************************************
 * IDirect3DMaterial implementation structure - Wraps to D3D7
 ******************************************************************************/
struct IDirect3DMaterialImpl
{
    const IDirect3DMaterial3Vtbl *lpVtbl;
    const IDirect3DMaterial2Vtbl *IDirect3DMaterial2_vtbl;
    const IDirect3DMaterialVtbl *IDirect3DMaterial_vtbl;
    LONG  ref;

    /* IDirect3DMaterial2 fields */
    IDirectDrawImpl               *ddraw;
    IDirect3DDeviceImpl           *active_device;

    D3DMATERIAL mat;
    DWORD Handle;
};

/* Helper functions */
void material_activate(IDirect3DMaterialImpl* This) DECLSPEC_HIDDEN;
void d3d_material_init(IDirect3DMaterialImpl *material, IDirectDrawImpl *ddraw) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DViewport - Wraps to D3D7
 *****************************************************************************/
struct IDirect3DViewportImpl
{
    const IDirect3DViewport3Vtbl *lpVtbl;
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

/* Helper functions */
void viewport_activate(IDirect3DViewportImpl* This, BOOL ignore_lights) DECLSPEC_HIDDEN;
void d3d_viewport_init(IDirect3DViewportImpl *viewport, IDirectDrawImpl *ddraw) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DExecuteBuffer - Wraps to D3D7
 *****************************************************************************/
struct IDirect3DExecuteBufferImpl
{
    /* IUnknown */
    const IDirect3DExecuteBufferVtbl *lpVtbl;
    LONG                 ref;

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

/* The execute function */
HRESULT d3d_execute_buffer_execute(IDirect3DExecuteBufferImpl *execute_buffer,
        IDirect3DDeviceImpl *device, IDirect3DViewportImpl *viewport) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DVertexBuffer
 *****************************************************************************/
struct IDirect3DVertexBufferImpl
{
    /*** IUnknown Methods ***/
    const IDirect3DVertexBuffer7Vtbl *lpVtbl;
    const IDirect3DVertexBufferVtbl *IDirect3DVertexBuffer_vtbl;
    LONG                 ref;

    /*** WineD3D and ddraw links ***/
    struct wined3d_buffer *wineD3DVertexBuffer;
    struct wined3d_vertex_declaration *wineD3DVertexDeclaration;
    IDirectDrawImpl *ddraw;

    /*** Storage for D3D7 specific things ***/
    DWORD                Caps;
    DWORD                fvf;
};

HRESULT d3d_vertex_buffer_init(IDirect3DVertexBufferImpl *buffer,
        IDirectDrawImpl *ddraw, D3DVERTEXBUFFERDESC *desc) DECLSPEC_HIDDEN;

static inline IDirect3DVertexBufferImpl *vb_from_vb1(IDirect3DVertexBuffer *iface)
{
    return (IDirect3DVertexBufferImpl *)((char*)iface
            - FIELD_OFFSET(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer_vtbl));
}

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
        DWORD __copysize = min(__size, from_size);                \
        assert(to != from);                                       \
        memcpy(to, from, __copysize);                             \
        memset((char*)(to) + __copysize, 0, __size - __copysize); \
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
