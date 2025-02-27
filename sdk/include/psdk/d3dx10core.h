/*
 * Copyright 2015 Alistair Leslie-Hughes
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

#include "d3dx10.h"

DEFINE_GUID(IID_ID3DX10Font, 0xd79dbb70, 0x5f21, 0x4d36, 0xbb, 0xc2, 0xff, 0x52, 0x5c, 0x21, 0x3c, 0xdc);
DEFINE_GUID(IID_ID3DX10Sprite, 0xba0b762d, 0x8d28, 0x43ec, 0xb9, 0xdc, 0x2f, 0x84, 0x44, 0x3b, 0x06, 0x14);
DEFINE_GUID(IID_ID3DX10ThreadPump, 0xc93fecfa, 0x6967, 0x478a, 0xab, 0xbc, 0x40, 0x2d, 0x90, 0x62, 0x1f, 0xcb);

typedef enum _D3DX10_SPRITE_FLAG
{
    D3DX10_SPRITE_SORT_TEXTURE             = 0x01,
    D3DX10_SPRITE_SORT_DEPTH_BACK_TO_FRONT = 0x02,
    D3DX10_SPRITE_SORT_DEPTH_FRONT_TO_BACK = 0x04,
    D3DX10_SPRITE_SAVE_STATE               = 0x08,
    D3DX10_SPRITE_ADDREF_TEXTURES          = 0x10,
} D3DX10_SPRITE_FLAG;

typedef struct _D3DX10_SPRITE
{
    D3DXMATRIX matWorld;
    D3DXVECTOR2 TexCoord;
    D3DXVECTOR2 TexSize;
    D3DXCOLOR ColorModulate;
    ID3D10ShaderResourceView *pTexture;
    UINT TextureIndex;
} D3DX10_SPRITE;

typedef struct _D3DX10_FONT_DESCA
{
    INT Height;
    UINT Width;
    UINT Weight;
    UINT MipLevels;
    BOOL Italic;
    BYTE CharSet;
    BYTE OutputPrecision;
    BYTE Quality;
    BYTE PitchAndFamily;
    CHAR FaceName[LF_FACESIZE];
} D3DX10_FONT_DESCA, *LPD3DX10_FONT_DESCA;

typedef struct _D3DX10_FONT_DESCW
{
    INT Height;
    UINT Width;
    UINT Weight;
    UINT MipLevels;
    BOOL Italic;
    BYTE CharSet;
    BYTE OutputPrecision;
    BYTE Quality;
    BYTE PitchAndFamily;
    WCHAR FaceName[LF_FACESIZE];
} D3DX10_FONT_DESCW, *LPD3DX10_FONT_DESCW;

DECL_WINELIB_TYPE_AW(D3DX10_FONT_DESC)
DECL_WINELIB_TYPE_AW(LPD3DX10_FONT_DESC)

#define INTERFACE ID3DX10DataLoader
DECLARE_INTERFACE(ID3DX10DataLoader)
{
    STDMETHOD(Load)(THIS) PURE;
    STDMETHOD(Decompress)(THIS_ void **data, SIZE_T *bytes) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** ID3DX10DataLoader methods ***/
#define ID3DX10DataLoader_Load(p)           (p)->lpVtbl->Load(p)
#define ID3DX10DataLoader_Decompress(p,a,b) (p)->lpVtbl->Decompress(p,a,b)
#define ID3DX10DataLoader_Destroy(p)        (p)->lpVtbl->Destroy(p)
#else
/*** ID3DX10DataLoader methods ***/
#define ID3DX10DataLoader_Load(p)           (p)->Load()
#define ID3DX10DataLoader_Decompress(p,a,b) (p)->Decompress(a,b)
#define ID3DX10DataLoader_Destroy(p)        (p)->Destroy()
#endif

#define INTERFACE ID3DX10DataProcessor
DECLARE_INTERFACE(ID3DX10DataProcessor)
{
    STDMETHOD(Process)(THIS_ void *data, SIZE_T bytes) PURE;
    STDMETHOD(CreateDeviceObject)(THIS_ void **dataobject) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** ID3DX10DataProcessor methods ***/
#define ID3DX10DataProcessor_Process(p,a,b)          (p)->lpVtbl->Process(p,a,b)
#define ID3DX10DataProcessor_CreateDeviceObject(p,a) (p)->lpVtbl->CreateDeviceObject(p,a)
#define ID3DX10DataProcessor_Destroy(p)              (p)->lpVtbl->Destroy(p)
#else
/*** ID3DX10DataProcessor methods ***/
#define ID3DX10DataProcessor_Process(p,a,b)          (p)->Process(a,b)
#define ID3DX10DataProcessor_CreateDeviceObject(p,a) (p)->CreateDeviceObject(a)
#define ID3DX10DataProcessor_Destroy(p)              (p)->Destroy()
#endif

#define INTERFACE  ID3DX10ThreadPump
DECLARE_INTERFACE_(ID3DX10ThreadPump, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DX10ThreadPump methods ***/
    STDMETHOD(AddWorkItem)(THIS_ ID3DX10DataLoader *loader, ID3DX10DataProcessor *processor,
            HRESULT *result, void **object) PURE;
    STDMETHOD_(UINT, GetWorkItemCount)(THIS) PURE;
    STDMETHOD(WaitForAllItems)(THIS) PURE;
    STDMETHOD(ProcessDeviceWorkItems)(THIS_ UINT count) PURE;
    STDMETHOD(PurgeAllItems)(THIS) PURE;
    STDMETHOD(GetQueueStatus)(THIS_ UINT *queue, UINT *processqueue, UINT *devicequeue) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ID3DX10ThreadPump_QueryInterface(p,a,b)       (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DX10ThreadPump_AddRef(p)                   (p)->lpVtbl->AddRef(p)
#define ID3DX10ThreadPump_Release(p)                  (p)->lpVtbl->Release(p)
/*** ID3DX10ThreadPump methods ***/
#define ID3DX10ThreadPump_AddWorkItem(p,a,b,c,d)      (p)->lpVtbl->AddWorkItem(p,a,b,c,d)
#define ID3DX10ThreadPump_GetWorkItemCount(p)         (p)->lpVtbl->GetWorkItemCount(p)
#define ID3DX10ThreadPump_WaitForAllItems(p)          (p)->lpVtbl->WaitForAllItems(p)
#define ID3DX10ThreadPump_ProcessDeviceWorkItems(p,a) (p)->lpVtbl->ProcessDeviceWorkItems(p,a)
#define ID3DX10ThreadPump_PurgeAllItems(p)            (p)->lpVtbl->PurgeAllItems(p)
#define ID3DX10ThreadPump_GetQueueStatus(p,a,b,c)     (p)->lpVtbl->GetQueueStatus(p,a,b,c)
#else
/*** IUnknown methods ***/
#define ID3DX10ThreadPump_QueryInterface(p,a,b)       (p)->QueryInterface(a,b)
#define ID3DX10ThreadPump_AddRef(p)                   (p)->AddRef()
#define ID3DX10ThreadPump_Release(p)                  (p)->Release()
/*** ID3DX10ThreadPump methods ***/
#define ID3DX10ThreadPump_AddWorkItem(p,a,b,c,d)      (p)->AddWorkItem(a,b,c,d)
#define ID3DX10ThreadPump_GetWorkItemCount(p)         (p)->GetWorkItemCount()
#define ID3DX10ThreadPump_WaitForAllItems(p)          (p)->WaitForAllItems()
#define ID3DX10ThreadPump_ProcessDeviceWorkItems(p,a) (p)->ProcessDeviceWorkItems(a)
#define ID3DX10ThreadPump_PurgeAllItems(p)            (p)->PurgeAllItems()
#define ID3DX10ThreadPump_GetQueueStatus(p,a,b,c)     (p)->GetQueueStatus(a,b,c)
#endif

#define INTERFACE  ID3DX10Sprite
DECLARE_INTERFACE_(ID3DX10Sprite, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DX10Sprite methods ***/
    STDMETHOD(Begin)(THIS_ UINT flags) PURE;
    STDMETHOD(DrawSpritesBuffered)(THIS_ D3DX10_SPRITE *sprites, UINT count) PURE;
    STDMETHOD(Flush)(THIS) PURE;
    STDMETHOD(DrawSpritesImmediate)(THIS_ D3DX10_SPRITE *sprites,
            UINT count, UINT size, UINT flags) PURE;
    STDMETHOD(End)(THIS) PURE;
    STDMETHOD(GetViewTransform)(THIS_ D3DXMATRIX *transform) PURE;
    STDMETHOD(SetViewTransform)(THIS_ D3DXMATRIX *transform) PURE;
    STDMETHOD(GetProjectionTransform)(THIS_ D3DXMATRIX *transform) PURE;
    STDMETHOD(SetProjectionTransform)(THIS_ D3DXMATRIX *transform) PURE;
    STDMETHOD(GetDevice)(THIS_ ID3D10Device **device) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ID3DX10Sprite_QueryInterface(p,a,b)           (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DX10Sprite_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define ID3DX10Sprite_Release(p)                      (p)->lpVtbl->Release(p)
/*** ID3DX10Sprite methods ***/
#define ID3DX10Sprite_Begin(p,a)                      (p)->lpVtbl->Begin(p,a)
#define ID3DX10Sprite_DrawSpritesBuffered(p,a,b)      (p)->lpVtbl->DrawSpritesBuffered(p,a,b)
#define ID3DX10Sprite_Flush(p)                        (p)->lpVtbl->Flush(p)
#define ID3DX10Sprite_DrawSpritesImmediate(p,a,b,c,d) (p)->lpVtbl->DrawSpritesImmediate(p,a,b,c,d)
#define ID3DX10Sprite_End(p)                          (p)->lpVtbl->End(p)
#define ID3DX10Sprite_GetViewTransform(p,a)           (p)->lpVtbl->GetViewTransform(p,a)
#define ID3DX10Sprite_SetViewTransform(p,a)           (p)->lpVtbl->SetViewTransform(p,a)
#define ID3DX10Sprite_GetProjectionTransform(p,a)     (p)->lpVtbl->GetProjectionTransform(p,a)
#define ID3DX10Sprite_SetProjectionTransform(p,a)     (p)->lpVtbl->SetProjectionTransform(p,a)
#define ID3DX10Sprite_GetDevice(p,a)                  (p)->lpVtbl->GetDevice(p,a)
#else
/*** IUnknown methods ***/
#define ID3DX10Sprite_QueryInterface(p,a,b)           (p)->QueryInterface(a,b)
#define ID3DX10Sprite_AddRef(p)                       (p)->AddRef()
#define ID3DX10Sprite_Release(p)                      (p)->Release()
/*** ID3DX10Sprite methods ***/
#define ID3DX10Sprite_Begin(p,a)                      (p)->Begin(a)
#define ID3DX10Sprite_DrawSpritesBuffered(p,a,b)      (p)->DrawSpritesBuffered(a,b)
#define ID3DX10Sprite_Flush(p)                        (p)->Flush()
#define ID3DX10Sprite_DrawSpritesImmediate(p,a,b,c,d) (p)->DrawSpritesImmediate(a,b,c,d)
#define ID3DX10Sprite_End(p)                          (p)->End()
#define ID3DX10Sprite_GetViewTransform(p,a)           (p)->GetViewTransform(a)
#define ID3DX10Sprite_SetViewTransform(p,a)           (p)->SetViewTransform(a)
#define ID3DX10Sprite_GetProjectionTransform(p,a)     (p)->GetProjectionTransform(a)
#define ID3DX10Sprite_SetProjectionTransform(p,a)     (p)->SetProjectionTransform(a)
#define ID3DX10Sprite_GetDevice(p,a)                  (p)->GetDevice(a)
#endif

#define INTERFACE  ID3DX10Font
DECLARE_INTERFACE_(ID3DX10Font, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, void **out) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DX10Font methods ***/
    STDMETHOD(GetDevice)(THIS_ ID3D10Device **device) PURE;
    STDMETHOD(GetDescA)(THIS_ D3DX10_FONT_DESCA *desc) PURE;
    STDMETHOD(GetDescW)(THIS_ D3DX10_FONT_DESCW *desc) PURE;
    STDMETHOD_(BOOL, GetTextMetricsA)(THIS_ TEXTMETRICA *metrics) PURE;
    STDMETHOD_(BOOL, GetTextMetricsW)(THIS_ TEXTMETRICW *metrics) PURE;
    STDMETHOD_(HDC, GetDC)(THIS) PURE;
    STDMETHOD(GetGlyphData)(THIS_ UINT glyph, ID3D10ShaderResourceView **texture,
            RECT *blackbox, POINT *cellinc) PURE;
    STDMETHOD(PreloadCharacters)(THIS_ UINT first, UINT last) PURE;
    STDMETHOD(PreloadGlyphs)(THIS_ UINT first, UINT last) PURE;
    STDMETHOD(PreloadTextA)(THIS_ const char *text, INT count) PURE;
    STDMETHOD(PreloadTextW)(THIS_ const WCHAR *text, INT count) PURE;
    STDMETHOD_(INT, DrawTextA)(THIS_ ID3DX10Sprite *sprite, const char *text,
            INT count, RECT *rect, UINT format, D3DXCOLOR color) PURE;
    STDMETHOD_(INT, DrawTextW)(THIS_ ID3DX10Sprite *sprite, const WCHAR *text,
            INT count, RECT *rect, UINT format, D3DXCOLOR color) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ID3DX10Font_QueryInterface(p,a,b)    (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DX10Font_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define ID3DX10Font_Release(p)               (p)->lpVtbl->Release(p)
/*** ID3DX10Font methods ***/
#define ID3DX10Font_GetDevice(p,a)           (p)->lpVtbl->GetDevice(p,a)
#define ID3DX10Font_GetDescA(p,a)            (p)->lpVtbl->GetDescA(p,a)
#define ID3DX10Font_GetDescW(p,a)            (p)->lpVtbl->GetDescW(p,a)
#define ID3DX10Font_GetTextMetricsA(p,a)     (p)->lpVtbl->GetTextMetricsA(p,a)
#define ID3DX10Font_GetTextMetricsW(p,a)     (p)->lpVtbl->GetTextMetricsW(p,a)
#define ID3DX10Font_GetDC(p)                 (p)->lpVtbl->GetDC(p)
#define ID3DX10Font_GetGlyphData(p,a,b,c,d)  (p)->lpVtbl->GetGlyphData(p,a,b,c,d)
#define ID3DX10Font_PreloadCharacters(p,a,b) (p)->lpVtbl->PreloadCharacters(p,a,b)
#define ID3DX10Font_PreloadGlyphs(p,a,b)     (p)->lpVtbl->PreloadGlyphs(p,a,b)
#define ID3DX10Font_PreloadTextA(p,a,b)      (p)->lpVtbl->PreloadTextA(p,a,b)
#define ID3DX10Font_PreloadTextW(p,a,b)      (p)->lpVtbl->PreloadTextW(p,a,b)
#define ID3DX10Font_DrawTextA(p,a,b,c,d,e,f) (p)->lpVtbl->DrawTextA(p,a,b,c,d,e,f)
#define ID3DX10Font_DrawTextW(p,a,b,c,d,e,f) (p)->lpVtbl->DrawTextW(p,a,b,c,d,e,f)
#else
/*** IUnknown methods ***/
#define ID3DX10Font_QueryInterface(p,a,b)    (p)->QueryInterface(a,b)
#define ID3DX10Font_AddRef(p)                (p)->AddRef()
#define ID3DX10Font_Release(p)               (p)->Release()
/*** ID3DX10Font methods ***/
#define ID3DX10Font_GetDevice(p,a)           (p)->GetDevice(a)
#define ID3DX10Font_GetDescA(p,a)            (p)->GetDescA(a)
#define ID3DX10Font_GetDescW(p,a)            (p)->GetDescW(a)
#define ID3DX10Font_GetTextMetricsA(p,a)     (p)->GetTextMetricsA(a)
#define ID3DX10Font_GetTextMetricsW(p,a)     (p)->GetTextMetricsW(a)
#define ID3DX10Font_GetDC(p)                 (p)->GetDC()
#define ID3DX10Font_GetGlyphData(p,a,b,c,d)  (p)->GetGlyphData(a,b,c,d)
#define ID3DX10Font_PreloadCharacters(p,a,b) (p)->PreloadCharacters(a,b)
#define ID3DX10Font_PreloadGlyphs(p,a,b)     (p)->PreloadGlyphs(a,b)
#define ID3DX10Font_PreloadTextA(p,a,b)      (p)->PreloadTextA(a,b)
#define ID3DX10Font_PreloadTextW(p,a,b)      (p)->PreloadTextW(a,b)
#define ID3DX10Font_DrawTextA(p,a,b,c,d,e,f) (p)->DrawTextA(a,b,c,d,e,f)
#define ID3DX10Font_DrawTextW(p,a,b,c,d,e,f) (p)->DrawTextW(a,b,c,d,e,f)
#endif

HRESULT WINAPI D3DX10UnsetAllDeviceObjects(ID3D10Device *device);
HRESULT WINAPI D3DX10CreateDevice(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, ID3D10Device **device);
HRESULT WINAPI D3DX10CreateDeviceAndSwapChain(IDXGIAdapter *adapter, D3D10_DRIVER_TYPE driver_type,
        HMODULE swrast, unsigned int flags, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain,
        ID3D10Device **device);
interface ID3D10Device1;
HRESULT WINAPI D3DX10GetFeatureLevel1(ID3D10Device *device, interface ID3D10Device1 **device1);
HRESULT WINAPI D3DX10CreateFontIndirectA(ID3D10Device *device, const D3DX10_FONT_DESCA *desc, ID3DX10Font **font);
HRESULT WINAPI D3DX10CreateFontIndirectW(ID3D10Device *device, const D3DX10_FONT_DESCW *desc, ID3DX10Font **font);
HRESULT WINAPI D3DX10CreateFontA(ID3D10Device *device, INT height, UINT width, UINT weight,
        UINT miplevels, BOOL italic, UINT charset, UINT precision, UINT quality,
        UINT pitchandfamily, const char *facename, ID3DX10Font **font);
HRESULT WINAPI D3DX10CreateFontW(ID3D10Device *device, INT height, UINT width, UINT weight,
        UINT miplevels, BOOL italic, UINT charset, UINT precision, UINT quality,
        UINT pitchandfamily, const WCHAR *facename, ID3DX10Font **font);
HRESULT WINAPI D3DX10CreateSprite(ID3D10Device *device, UINT size, ID3DX10Sprite **sprite);
HRESULT WINAPI D3DX10CreateThreadPump(UINT io_threads, UINT proc_threads, ID3DX10ThreadPump **pump);
