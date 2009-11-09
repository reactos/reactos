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

/* MAY NOT CONTAIN X11 or DGA specific includes/defines/structs! */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddraw.h"
#include "ddrawi.h"
#include "d3d.h"

#include "wine/list.h"
#ifdef DDRAW_INIT_GUID
#include "initguid.h"
#endif
#include "wine/wined3d.h"

/*****************************************************************************
 * IParent - a helper interface
 *****************************************************************************/
DEFINE_GUID(IID_IParent, 0xc20e4c88, 0x74e7, 0x4940, 0xba, 0x9f, 0x2e, 0x32, 0x3f, 0x9d, 0xc9, 0x81);
typedef struct IParent *LPPARENT, *PPARENT;

#define INTERFACE IParent
DECLARE_INTERFACE_(IParent,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IParent_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IParent_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IParent_Release(p)            (p)->lpVtbl->Release(p)
#endif


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
typedef struct IParentImpl                IParentImpl;

/* Callbacks for implicit object destruction */
extern ULONG WINAPI D3D7CB_DestroySwapChain(IWineD3DSwapChain *pSwapChain);

extern ULONG WINAPI D3D7CB_DestroyDepthStencilSurface(IWineD3DSurface *pSurface);

/* Global critical section */
extern CRITICAL_SECTION ddraw_cs;

extern DWORD force_refresh_rate;

/*****************************************************************************
 * IDirectDraw implementation structure
 *****************************************************************************/
struct FvfToDecl
{
    DWORD fvf;
    IWineD3DVertexDeclaration *decl;
};

struct IDirectDrawImpl
{
    /* IUnknown fields */
    const IDirectDraw7Vtbl *lpVtbl;
    const IDirectDraw4Vtbl *IDirectDraw4_vtbl;
    const IDirectDraw2Vtbl *IDirectDraw2_vtbl;
    const IDirectDrawVtbl *IDirectDraw_vtbl;
    const IDirect3D7Vtbl *IDirect3D7_vtbl;
    const IDirect3D3Vtbl *IDirect3D3_vtbl;
    const IDirect3D2Vtbl *IDirect3D2_vtbl;
    const IDirect3DVtbl *IDirect3D_vtbl;
    const IWineD3DDeviceParentVtbl *device_parent_vtbl;

    /* See comment in IDirectDraw::AddRef */
    LONG                    ref7, ref4, ref2, ref3, ref1, numIfaces;

    /* WineD3D linkage */
    IWineD3D                *wineD3D;
    IWineD3DDevice          *wineD3DDevice;
    IDirectDrawSurfaceImpl  *DepthStencilBuffer;
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


    /* Our private window class */
    char classname[32];
    WNDCLASSA wnd_class;

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

/* Declare the VTables. They can be found ddraw.c */
extern const IDirectDraw7Vtbl IDirectDraw7_Vtbl;
extern const IDirectDraw4Vtbl IDirectDraw4_Vtbl;
extern const IDirectDraw2Vtbl IDirectDraw2_Vtbl;
extern const IDirectDrawVtbl  IDirectDraw1_Vtbl;
extern const IWineD3DDeviceParentVtbl ddraw_wined3d_device_parent_vtbl;

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
void
DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS* pIn,
                             DDSCAPS2* pOut);
void
DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2* pIn,
                                        DDDEVICEIDENTIFIER* pOut);
void
IDirectDrawImpl_Destroy(IDirectDrawImpl *This);

HRESULT WINAPI
IDirectDrawImpl_RecreateSurfacesCallback(IDirectDrawSurface7 *surf,
                                         DDSURFACEDESC2 *desc,
                                         void *Context);
IWineD3DVertexDeclaration *
IDirectDrawImpl_FindDecl(IDirectDrawImpl *This,
                         DWORD fvf);

static inline IDirectDrawImpl *ddraw_from_d3d1(IDirect3D *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirect3D_vtbl));
}

static inline IDirectDrawImpl *ddraw_from_d3d2(IDirect3D2 *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirect3D2_vtbl));
}

static inline IDirectDrawImpl *ddraw_from_d3d3(IDirect3D3 *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirect3D3_vtbl));
}

static inline IDirectDrawImpl *ddraw_from_d3d7(IDirect3D7 *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirect3D7_vtbl));
}

static inline IDirectDrawImpl *ddraw_from_ddraw1(IDirectDraw *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirectDraw_vtbl));
}

static inline IDirectDrawImpl *ddraw_from_ddraw2(IDirectDraw2 *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirectDraw2_vtbl));
}

static inline IDirectDrawImpl *ddraw_from_ddraw4(IDirectDraw4 *iface)
{
    return (IDirectDrawImpl *)((char*)iface - FIELD_OFFSET(IDirectDrawImpl, IDirectDraw4_vtbl));
}

/* The default surface type */
extern WINED3DSURFTYPE DefaultSurfaceType;

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
    IWineD3DSurface         *WineD3DSurface;
    IWineD3DBaseTexture     *wineD3DTexture;
    IWineD3DSwapChain       *wineD3DSwapChain;

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

/* VTable declaration. It's located in surface.c / surface_thunks.c */
extern const IDirectDrawSurface7Vtbl IDirectDrawSurface7_Vtbl;
extern const IDirectDrawSurface3Vtbl IDirectDrawSurface3_Vtbl;
extern const IDirectDrawGammaControlVtbl IDirectDrawGammaControl_Vtbl;
extern const IDirect3DTexture2Vtbl IDirect3DTexture2_Vtbl;
extern const IDirect3DTextureVtbl IDirect3DTexture1_Vtbl;

HRESULT WINAPI IDirectDrawSurfaceImpl_AddAttachedSurface(IDirectDrawSurfaceImpl *This, IDirectDrawSurfaceImpl *Surf);
void IDirectDrawSurfaceImpl_Destroy(IDirectDrawSurfaceImpl *This);

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

/*****************************************************************************
 * IParent Implementation
 *****************************************************************************/
struct IParentImpl
{
    /* IUnknown fields */
    const IParentVtbl *lpVtbl;
    LONG                    ref;

    /* IParentImpl fields */
    IUnknown      *child;

};

extern const IParentVtbl IParent_Vtbl;

/*****************************************************************************
 * IDirect3DDevice implementation
 *****************************************************************************/
typedef enum
{
    DDrawHandle_Unknown       = 0,
    DDrawHandle_Texture       = 1,
    DDrawHandle_Material      = 2,
    DDrawHandle_Matrix        = 3,
    DDrawHandle_StateBlock    = 4
} DDrawHandleTypes;

struct HandleEntry
{
    void    *ptr;
    DDrawHandleTypes      type;
};

struct IDirect3DDeviceImpl
{
    /* IUnknown */
    const IDirect3DDevice7Vtbl *lpVtbl;
    const IDirect3DDevice3Vtbl *IDirect3DDevice3_vtbl;
    const IDirect3DDevice2Vtbl *IDirect3DDevice2_vtbl;
    const IDirect3DDeviceVtbl *IDirect3DDevice_vtbl;
    LONG                    ref;

    /* Other object connections */
    IWineD3DDevice          *wineD3DDevice;
    IDirectDrawImpl         *ddraw;
    IWineD3DBuffer          *indexbuffer;
    IDirectDrawSurfaceImpl  *target;
    BOOL                    OffScreenTarget;

    /* Viewport management */
    IDirect3DViewportImpl *viewport_list;
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
    struct HandleEntry      *Handles;
    DWORD                    numHandles;
    D3DMATRIXHANDLE          world, proj, view;
};

/* Vtables in various versions */
extern const IDirect3DDevice7Vtbl IDirect3DDevice7_FPUSetup_Vtbl;
extern const IDirect3DDevice7Vtbl IDirect3DDevice7_FPUPreserve_Vtbl;
extern const IDirect3DDevice3Vtbl IDirect3DDevice3_Vtbl;
extern const IDirect3DDevice2Vtbl IDirect3DDevice2_Vtbl;
extern const IDirect3DDeviceVtbl  IDirect3DDevice1_Vtbl;

/* The IID */
extern const GUID IID_D3DDEVICE_WineD3D;

/* Helper functions */
HRESULT IDirect3DImpl_GetCaps(IWineD3D *WineD3D, D3DDEVICEDESC *Desc123, D3DDEVICEDESC7 *Desc7);
DWORD IDirect3DDeviceImpl_CreateHandle(IDirect3DDeviceImpl *This);
WINED3DZBUFFERTYPE IDirect3DDeviceImpl_UpdateDepthStencil(IDirect3DDeviceImpl *This);

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

/*****************************************************************************
 * IDirect3D implementation
 *****************************************************************************/

/* No implementation structure as this is only another interface to DirectDraw */

/* the Vtables */
extern const IDirect3DVtbl  IDirect3D1_Vtbl;
extern const IDirect3D2Vtbl IDirect3D2_Vtbl;
extern const IDirect3D3Vtbl IDirect3D3_Vtbl;
extern const IDirect3D7Vtbl IDirect3D7_Vtbl;

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

    IWineD3DClipper           *wineD3DClipper;
    IDirectDrawImpl           *ddraw_owner;
};

extern const IDirectDrawClipperVtbl IDirectDrawClipper_Vtbl;

typeof(WineDirect3DCreateClipper) *pWineDirect3DCreateClipper;

/*****************************************************************************
 * IDirectDrawPalette implementation structure
 *****************************************************************************/
struct IDirectDrawPaletteImpl
{
    /* IUnknown fields */
    const IDirectDrawPaletteVtbl *lpVtbl;
    LONG ref;

    /* WineD3D uplink */
    IWineD3DPalette           *wineD3DPalette;

    /* IDirectDrawPalette fields */
    IDirectDrawImpl           *ddraw_owner;
    IUnknown                  *ifaceToRelease;
};
extern const IDirectDrawPaletteVtbl IDirectDrawPalette_Vtbl;

/******************************************************************************
 * DirectDraw ClassFactory implementation - incomplete
 ******************************************************************************/
typedef struct
{
    const IClassFactoryVtbl *lpVtbl;

    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, REFIID iid, LPVOID *ppObj);
} IClassFactoryImpl;

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

    /* Chained list used for adding / removing from viewports */
    IDirect3DLightImpl        *next;

    /* Activation function */
    void                      (*activate)(IDirect3DLightImpl*);
    void                      (*desactivate)(IDirect3DLightImpl*);
    void                      (*update)(IDirect3DLightImpl*);
};

/* Vtable */
extern const IDirect3DLightVtbl IDirect3DLight_Vtbl;

/* Helper functions */
void light_update(IDirect3DLightImpl* This);
void light_activate(IDirect3DLightImpl* This);
void light_desactivate(IDirect3DLightImpl* This);

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

    void (*activate)(IDirect3DMaterialImpl* this);
};

/* VTables in various versions */
extern const IDirect3DMaterialVtbl IDirect3DMaterial_Vtbl;
extern const IDirect3DMaterial2Vtbl IDirect3DMaterial2_Vtbl;
extern const IDirect3DMaterial3Vtbl IDirect3DMaterial3_Vtbl;

/* Helper functions */
void material_activate(IDirect3DMaterialImpl* This);

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

    /* Activation function */
    void                      (*activate)(IDirect3DViewportImpl*, BOOL);

    /* Field used to chain viewports together */
    IDirect3DViewportImpl     *next;

    /* Lights list */
    IDirect3DLightImpl        *lights;

    /* Background material */
    IDirect3DMaterialImpl     *background;
};

/* Vtable */
extern const IDirect3DViewport3Vtbl IDirect3DViewport3_Vtbl;

/* Helper functions */
void viewport_activate(IDirect3DViewportImpl* This, BOOL ignore_lights);

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

/* The VTable */
extern const IDirect3DExecuteBufferVtbl IDirect3DExecuteBuffer_Vtbl;

/* The execute function */
void
IDirect3DExecuteBufferImpl_Execute(IDirect3DExecuteBufferImpl *This,
                                   IDirect3DDeviceImpl *Device,
                                   IDirect3DViewportImpl *ViewportImpl);

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
    IWineD3DBuffer *wineD3DVertexBuffer;
    IWineD3DVertexDeclaration *wineD3DVertexDeclaration;
    IDirectDrawImpl *ddraw;

    /*** Storage for D3D7 specific things ***/
    DWORD                Caps;
    DWORD                fvf;
};

/* The Vtables */
extern const IDirect3DVertexBuffer7Vtbl IDirect3DVertexBuffer7_Vtbl;
extern const IDirect3DVertexBufferVtbl IDirect3DVertexBuffer1_Vtbl;

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

void PixelFormat_WineD3DtoDD(DDPIXELFORMAT *DDPixelFormat, WINED3DFORMAT WineD3DFormat);
WINED3DFORMAT PixelFormat_DD2WineD3D(const DDPIXELFORMAT *DDPixelFormat);
void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd);
void dump_D3DMATRIX(const D3DMATRIX *mat);
void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps);
DWORD get_flexible_vertex_size(DWORD d3dvtVertexType);
void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in);
void DDRAW_dump_cooperativelevel(DWORD cooplevel);

/* This only needs to be here as long the processvertices functionality of
 * IDirect3DExecuteBuffer isn't in WineD3D */
void multiply_matrix(LPD3DMATRIX dest, const D3DMATRIX *src1, const D3DMATRIX *src2);

void multiply_matrix_D3D_way(LPD3DMATRIX result, const D3DMATRIX *m1, const D3DMATRIX *m2);

/* Helper function in main.c */
BOOL LoadWineD3D(void);

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

#define DD_STRUCT_COPY_BYSIZE(to,from)                  \
        do {                                            \
                DWORD __size = (to)->dwSize;            \
                DWORD __copysize = __size;              \
                DWORD __resetsize = __size;             \
                assert(to != from);                     \
                if (__resetsize > sizeof(*to))          \
                    __resetsize = sizeof(*to);          \
                memset(to,0,__resetsize);               \
                if ((from)->dwSize < __size)            \
                    __copysize = (from)->dwSize;        \
                memcpy(to,from,__copysize);             \
                (to)->dwSize = __size;/*restore size*/  \
        } while (0)


#endif

HRESULT hr_ddraw_from_wined3d(HRESULT hr);
