/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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

#include "wincodecs_private.h"

#include <ole2.h>

#include "ungif.h"

static LPWSTR strdupAtoW(const char *src)
{
    int len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    LPWSTR dst = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (dst) MultiByteToWideChar(CP_ACP, 0, src, -1, dst, len);
    return dst;
}

static HRESULT load_LSD_metadata(IStream *stream, const GUID *vendor, DWORD options,
                                 MetadataItem **items, DWORD *count)
{
#include "pshpack1.h"
    struct logical_screen_descriptor
    {
        char signature[6];
        USHORT width;
        USHORT height;
        BYTE packed;
        /* global_color_table_flag : 1;
         * color_resolution : 3;
         * sort_flag : 1;
         * global_color_table_size : 3;
         */
        BYTE background_color_index;
        BYTE pixel_aspect_ratio;
    } lsd_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    *items = NULL;
    *count = 0;

    hr = IStream_Read(stream, &lsd_data, sizeof(lsd_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(lsd_data)) return S_OK;

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem) * 9);
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 9; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    result[0].id.u.pwszVal = strdupAtoW("Signature");
    result[0].value.vt = VT_UI1|VT_VECTOR;
    result[0].value.u.caub.cElems = sizeof(lsd_data.signature);
    result[0].value.u.caub.pElems = HeapAlloc(GetProcessHeap(), 0, sizeof(lsd_data.signature));
    memcpy(result[0].value.u.caub.pElems, lsd_data.signature, sizeof(lsd_data.signature));

    result[1].id.vt = VT_LPWSTR;
    result[1].id.u.pwszVal = strdupAtoW("Width");
    result[1].value.vt = VT_UI2;
    result[1].value.u.uiVal = lsd_data.width;

    result[2].id.vt = VT_LPWSTR;
    result[2].id.u.pwszVal = strdupAtoW("Height");
    result[2].value.vt = VT_UI2;
    result[2].value.u.uiVal = lsd_data.height;

    result[3].id.vt = VT_LPWSTR;
    result[3].id.u.pwszVal = strdupAtoW("GlobalColorTableFlag");
    result[3].value.vt = VT_BOOL;
    result[3].value.u.boolVal = (lsd_data.packed >> 7) & 1;

    result[4].id.vt = VT_LPWSTR;
    result[4].id.u.pwszVal = strdupAtoW("ColorResolution");
    result[4].value.vt = VT_UI1;
    result[4].value.u.bVal = (lsd_data.packed >> 4) & 7;

    result[5].id.vt = VT_LPWSTR;
    result[5].id.u.pwszVal = strdupAtoW("SortFlag");
    result[5].value.vt = VT_BOOL;
    result[5].value.u.boolVal = (lsd_data.packed >> 3) & 1;

    result[6].id.vt = VT_LPWSTR;
    result[6].id.u.pwszVal = strdupAtoW("GlobalColorTableSize");
    result[6].value.vt = VT_UI1;
    result[6].value.u.bVal = lsd_data.packed & 7;

    result[7].id.vt = VT_LPWSTR;
    result[7].id.u.pwszVal = strdupAtoW("BackgroundColorIndex");
    result[7].value.vt = VT_UI1;
    result[7].value.u.bVal = lsd_data.background_color_index;

    result[8].id.vt = VT_LPWSTR;
    result[8].id.u.pwszVal = strdupAtoW("PixelAspectRatio");
    result[8].value.vt = VT_UI1;
    result[8].value.u.bVal = lsd_data.pixel_aspect_ratio;

    *items = result;
    *count = 9;

    return S_OK;
}

static const MetadataHandlerVtbl LSDReader_Vtbl = {
    0,
    &CLSID_WICLSDMetadataReader,
    load_LSD_metadata
};

HRESULT LSDReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&LSDReader_Vtbl, iid, ppv);
}

#include "pshpack1.h"
struct image_descriptor
{
    USHORT left;
    USHORT top;
    USHORT width;
    USHORT height;
    BYTE packed;
    /* local_color_table_flag : 1;
     * interlace_flag : 1;
     * sort_flag : 1;
     * reserved : 2;
     * local_color_table_size : 3;
     */
};
#include "poppack.h"

static HRESULT load_IMD_metadata(IStream *stream, const GUID *vendor, DWORD options,
                                 MetadataItem **items, DWORD *count)
{
    struct image_descriptor imd_data;
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    *items = NULL;
    *count = 0;

    hr = IStream_Read(stream, &imd_data, sizeof(imd_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(imd_data)) return S_OK;

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem) * 8);
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 8; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    result[0].id.u.pwszVal = strdupAtoW("Left");
    result[0].value.vt = VT_UI2;
    result[0].value.u.uiVal = imd_data.left;

    result[1].id.vt = VT_LPWSTR;
    result[1].id.u.pwszVal = strdupAtoW("Top");
    result[1].value.vt = VT_UI2;
    result[1].value.u.uiVal = imd_data.top;

    result[2].id.vt = VT_LPWSTR;
    result[2].id.u.pwszVal = strdupAtoW("Width");
    result[2].value.vt = VT_UI2;
    result[2].value.u.uiVal = imd_data.width;

    result[3].id.vt = VT_LPWSTR;
    result[3].id.u.pwszVal = strdupAtoW("Height");
    result[3].value.vt = VT_UI2;
    result[3].value.u.uiVal = imd_data.height;

    result[4].id.vt = VT_LPWSTR;
    result[4].id.u.pwszVal = strdupAtoW("LocalColorTableFlag");
    result[4].value.vt = VT_BOOL;
    result[4].value.u.boolVal = (imd_data.packed >> 7) & 1;

    result[5].id.vt = VT_LPWSTR;
    result[5].id.u.pwszVal = strdupAtoW("InterlaceFlag");
    result[5].value.vt = VT_BOOL;
    result[5].value.u.boolVal = (imd_data.packed >> 6) & 1;

    result[6].id.vt = VT_LPWSTR;
    result[6].id.u.pwszVal = strdupAtoW("SortFlag");
    result[6].value.vt = VT_BOOL;
    result[6].value.u.boolVal = (imd_data.packed >> 5) & 1;

    result[7].id.vt = VT_LPWSTR;
    result[7].id.u.pwszVal = strdupAtoW("LocalColorTableSize");
    result[7].value.vt = VT_UI1;
    result[7].value.u.bVal = imd_data.packed & 7;

    *items = result;
    *count = 8;

    return S_OK;
}

static const MetadataHandlerVtbl IMDReader_Vtbl = {
    0,
    &CLSID_WICIMDMetadataReader,
    load_IMD_metadata
};

HRESULT IMDReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&IMDReader_Vtbl, iid, ppv);
}

static HRESULT load_GCE_metadata(IStream *stream, const GUID *vendor, DWORD options,
                                 MetadataItem **items, DWORD *count)
{
#include "pshpack1.h"
    struct graphic_control_extenstion
    {
        BYTE packed;
        /* reservred: 3;
         * disposal : 3;
         * user_input_flag : 1;
         * transparency_flag : 1;
         */
         USHORT delay;
         BYTE transparent_color_index;
    } gce_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    *items = NULL;
    *count = 0;

    hr = IStream_Read(stream, &gce_data, sizeof(gce_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(gce_data)) return S_OK;

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem) * 5);
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 5; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    result[0].id.u.pwszVal = strdupAtoW("Disposal");
    result[0].value.vt = VT_UI1;
    result[0].value.u.bVal = (gce_data.packed >> 2) & 7;

    result[1].id.vt = VT_LPWSTR;
    result[1].id.u.pwszVal = strdupAtoW("UserInputFlag");
    result[1].value.vt = VT_BOOL;
    result[1].value.u.boolVal = (gce_data.packed >> 1) & 1;

    result[2].id.vt = VT_LPWSTR;
    result[2].id.u.pwszVal = strdupAtoW("TransparencyFlag");
    result[2].value.vt = VT_BOOL;
    result[2].value.u.boolVal = gce_data.packed & 1;

    result[3].id.vt = VT_LPWSTR;
    result[3].id.u.pwszVal = strdupAtoW("Delay");
    result[3].value.vt = VT_UI2;
    result[3].value.u.uiVal = gce_data.delay;

    result[4].id.vt = VT_LPWSTR;
    result[4].id.u.pwszVal = strdupAtoW("TransparentColorIndex");
    result[4].value.vt = VT_UI1;
    result[4].value.u.bVal = gce_data.transparent_color_index;

    *items = result;
    *count = 5;

    return S_OK;
}

static const MetadataHandlerVtbl GCEReader_Vtbl = {
    0,
    &CLSID_WICGCEMetadataReader,
    load_GCE_metadata
};

HRESULT GCEReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&GCEReader_Vtbl, iid, ppv);
}

static HRESULT load_APE_metadata(IStream *stream, const GUID *vendor, DWORD options,
                                 MetadataItem **items, DWORD *count)
{
#include "pshpack1.h"
    struct application_extenstion
    {
        BYTE extension_introducer;
        BYTE extension_label;
        BYTE block_size;
        BYTE application[11];
    } ape_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, data_size, i;
    MetadataItem *result;
    BYTE subblock_size;
    BYTE *data;

    *items = NULL;
    *count = 0;

    hr = IStream_Read(stream, &ape_data, sizeof(ape_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(ape_data)) return S_OK;
    if (ape_data.extension_introducer != 0x21 ||
        ape_data.extension_label != APPLICATION_EXT_FUNC_CODE ||
        ape_data.block_size != 11)
        return S_OK;

    data = NULL;
    data_size = 0;

    for (;;)
    {
        hr = IStream_Read(stream, &subblock_size, sizeof(subblock_size), &bytesread);
        if (FAILED(hr) || bytesread != sizeof(subblock_size))
        {
            HeapFree(GetProcessHeap(), 0, data);
            return S_OK;
        }
        if (!subblock_size) break;

        if (!data)
            data = HeapAlloc(GetProcessHeap(), 0, subblock_size + 1);
        else
        {
            BYTE *new_data = HeapReAlloc(GetProcessHeap(), 0, data, data_size + subblock_size + 1);
            if (!new_data)
            {
                HeapFree(GetProcessHeap(), 0, data);
                return S_OK;
            }
            data = new_data;
        }
        data[data_size] = subblock_size;
        hr = IStream_Read(stream, data + data_size + 1, subblock_size, &bytesread);
        if (FAILED(hr) || bytesread != subblock_size)
        {
            HeapFree(GetProcessHeap(), 0, data);
            return S_OK;
        }
        data_size += subblock_size + 1;
    }

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem) * 2);
    if (!result)
    {
        HeapFree(GetProcessHeap(), 0, data);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < 2; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    result[0].id.u.pwszVal = strdupAtoW("Application");
    result[0].value.vt = VT_UI1|VT_VECTOR;
    result[0].value.u.caub.cElems = sizeof(ape_data.application);
    result[0].value.u.caub.pElems = HeapAlloc(GetProcessHeap(), 0, sizeof(ape_data.application));
    memcpy(result[0].value.u.caub.pElems, ape_data.application, sizeof(ape_data.application));

    result[1].id.vt = VT_LPWSTR;
    result[1].id.u.pwszVal = strdupAtoW("Data");
    result[1].value.vt = VT_UI1|VT_VECTOR;
    result[1].value.u.caub.cElems = data_size;
    result[1].value.u.caub.pElems = data;

    *items = result;
    *count = 2;

    return S_OK;
}

static const MetadataHandlerVtbl APEReader_Vtbl = {
    0,
    &CLSID_WICAPEMetadataReader,
    load_APE_metadata
};

HRESULT APEReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&APEReader_Vtbl, iid, ppv);
}

static HRESULT load_GifComment_metadata(IStream *stream, const GUID *vendor, DWORD options,
                                        MetadataItem **items, DWORD *count)
{
#include "pshpack1.h"
    struct gif_extenstion
    {
        BYTE extension_introducer;
        BYTE extension_label;
    } ext_data;
#include "poppack.h"
    HRESULT hr;
    ULONG bytesread, data_size;
    MetadataItem *result;
    BYTE subblock_size;
    char *data;

    *items = NULL;
    *count = 0;

    hr = IStream_Read(stream, &ext_data, sizeof(ext_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(ext_data)) return S_OK;
    if (ext_data.extension_introducer != 0x21 ||
        ext_data.extension_label != COMMENT_EXT_FUNC_CODE)
        return S_OK;

    data = NULL;
    data_size = 0;

    for (;;)
    {
        hr = IStream_Read(stream, &subblock_size, sizeof(subblock_size), &bytesread);
        if (FAILED(hr) || bytesread != sizeof(subblock_size))
        {
            HeapFree(GetProcessHeap(), 0, data);
            return S_OK;
        }
        if (!subblock_size) break;

        if (!data)
            data = HeapAlloc(GetProcessHeap(), 0, subblock_size + 1);
        else
        {
            char *new_data = HeapReAlloc(GetProcessHeap(), 0, data, data_size + subblock_size + 1);
            if (!new_data)
            {
                HeapFree(GetProcessHeap(), 0, data);
                return S_OK;
            }
            data = new_data;
        }
        hr = IStream_Read(stream, data + data_size, subblock_size, &bytesread);
        if (FAILED(hr) || bytesread != subblock_size)
        {
            HeapFree(GetProcessHeap(), 0, data);
            return S_OK;
        }
        data_size += subblock_size;
    }

    data[data_size] = 0;

    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MetadataItem));
    if (!result)
    {
        HeapFree(GetProcessHeap(), 0, data);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result->schema);
    PropVariantInit(&result->id);
    PropVariantInit(&result->value);

    result->id.vt = VT_LPWSTR;
    result->id.u.pwszVal = strdupAtoW("TextEntry");
    result->value.vt = VT_LPSTR;
    result->value.u.pszVal = data;

    *items = result;
    *count = 1;

    return S_OK;
}

static const MetadataHandlerVtbl GifCommentReader_Vtbl = {
    0,
    &CLSID_WICGifCommentMetadataReader,
    load_GifComment_metadata
};

HRESULT GifCommentReader_CreateInstance(REFIID iid, void **ppv)
{
    return MetadataReader_Create(&GifCommentReader_Vtbl, iid, ppv);
}

static IStream *create_stream(const void *data, int data_size)
{
    HRESULT hr;
    IStream *stream;
    HGLOBAL hdata;
    void *locked_data;

    hdata = GlobalAlloc(GMEM_MOVEABLE, data_size);
    if (!hdata) return NULL;

    locked_data = GlobalLock(hdata);
    memcpy(locked_data, data, data_size);
    GlobalUnlock(hdata);

    hr = CreateStreamOnHGlobal(hdata, TRUE, &stream);
    return FAILED(hr) ? NULL : stream;
}

static HRESULT create_metadata_reader(const void *data, int data_size,
                                      class_constructor constructor,
                                      IWICMetadataReader **reader)
{
    HRESULT hr;
    IWICMetadataReader *metadata_reader;
    IWICPersistStream *persist;
    IStream *stream;

    /* FIXME: Use IWICComponentFactory_CreateMetadataReader once it's implemented */

    hr = constructor(&IID_IWICMetadataReader, (void**)&metadata_reader);
    if (FAILED(hr)) return hr;

    hr = IWICMetadataReader_QueryInterface(metadata_reader, &IID_IWICPersistStream, (void **)&persist);
    if (FAILED(hr))
    {
        IWICMetadataReader_Release(metadata_reader);
        return hr;
    }

    stream = create_stream(data, data_size);
    IWICPersistStream_LoadEx(persist, stream, NULL, WICPersistOptionsDefault);
    IStream_Release(stream);

    IWICPersistStream_Release(persist);

    *reader = metadata_reader;
    return S_OK;
}

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    IStream *stream;
    BYTE LSD_data[13]; /* Logical Screen Descriptor */
    LONG ref;
    BOOL initialized;
    GifFileType *gif;
    UINT current_frame;
    CRITICAL_SECTION lock;
} GifDecoder;

typedef struct {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    LONG ref;
    SavedImage *frame;
    GifDecoder *parent;
} GifFrameDecode;

static inline GifDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, GifDecoder, IWICBitmapDecoder_iface);
}

static inline GifDecoder *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, GifDecoder, IWICMetadataBlockReader_iface);
}

static inline GifFrameDecode *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, GifFrameDecode, IWICBitmapFrameDecode_iface);
}

static inline GifFrameDecode *frame_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, GifFrameDecode, IWICMetadataBlockReader_iface);
}

static HRESULT WINAPI GifFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockReader, iid))
    {
        *ppv = &This->IWICMetadataBlockReader_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI GifFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapDecoder_Release(&This->parent->IWICBitmapDecoder_iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI GifFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    *puiWidth = This->frame->ImageDesc.Width;
    *puiHeight = This->frame->ImageDesc.Height;

    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    memcpy(pPixelFormat, &GUID_WICPixelFormat8bppIndexed, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    const GifWord aspect_word = This->parent->gif->SAspectRatio;
    const double aspect = (aspect_word > 0) ? ((aspect_word + 15.0) / 64.0) : 1.0;
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    *pDpiX = 96.0 / aspect;
    *pDpiY = 96.0;

    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    WICColor colors[256];
    ColorMapObject *cm = This->frame->ImageDesc.ColorMap;
    int i, trans;
    ExtensionBlock *eb;
    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!cm) cm = This->parent->gif->SColorMap;

    if (cm->ColorCount > 256)
    {
        ERR("GIF contains %i colors???\n", cm->ColorCount);
        return E_FAIL;
    }

    for (i = 0; i < cm->ColorCount; i++) {
        colors[i] = 0xff000000| /* alpha */
                    cm->Colors[i].Red << 16|
                    cm->Colors[i].Green << 8|
                    cm->Colors[i].Blue;
    }

    /* look for the transparent color extension */
    for (i = 0; i < This->frame->Extensions.ExtensionBlockCount; ++i) {
	eb = This->frame->Extensions.ExtensionBlocks + i;
	if (eb->Function == GRAPHICS_EXT_FUNC_CODE && eb->ByteCount == 8) {
	    if (eb->Bytes[3] & 1) {
	        trans = (unsigned char)eb->Bytes[6];
	        colors[trans] &= 0xffffff; /* set alpha to 0 */
	        break;
	    }
	}
    }

    return IWICPalette_InitializeCustom(pIPalette, colors, cm->ColorCount);
}

static HRESULT copy_interlaced_pixels(const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride, const WICRect *rc,
    UINT dststride, UINT dstbuffersize, BYTE *dstbuffer)
{
    UINT row_offset; /* number of bytes into the source rows where the data starts */
    const BYTE *src;
    BYTE *dst;
    UINT y;
    WICRect rect;

    if (!rc)
    {
        rect.X = 0;
        rect.Y = 0;
        rect.Width = srcwidth;
        rect.Height = srcheight;
        rc = &rect;
    }
    else
    {
        if (rc->X < 0 || rc->Y < 0 || rc->X+rc->Width > srcwidth || rc->Y+rc->Height > srcheight)
            return E_INVALIDARG;
    }

    if (dststride < rc->Width)
        return E_INVALIDARG;

    if ((dststride * rc->Height) > dstbuffersize)
        return E_INVALIDARG;

    row_offset = rc->X;

    dst = dstbuffer;
    for (y=rc->Y; y-rc->Y < rc->Height; y++)
    {
        if (y%8 == 0)
            src = srcbuffer + srcstride * (y/8);
        else if (y%4 == 0)
            src = srcbuffer + srcstride * ((srcheight+7)/8 + y/8);
        else if (y%2 == 0)
            src = srcbuffer + srcstride * ((srcheight+3)/4 + y/4);
        else /* y%2 == 1 */
            src = srcbuffer + srcstride * ((srcheight+1)/2 + y/2);
        src += row_offset;
        memcpy(dst, src, rc->Width);
        dst += dststride;
    }
    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (This->frame->ImageDesc.Interlace)
    {
        return copy_interlaced_pixels(This->frame->RasterBits, This->frame->ImageDesc.Width,
            This->frame->ImageDesc.Height, This->frame->ImageDesc.Width,
            prc, cbStride, cbBufferSize, pbBuffer);
    }
    else
    {
        return copy_pixels(8, This->frame->RasterBits, This->frame->ImageDesc.Width,
            This->frame->ImageDesc.Height, This->frame->ImageDesc.Width,
            prc, cbStride, cbBufferSize, pbBuffer);
    }
}

static HRESULT WINAPI GifFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl GifFrameDecode_Vtbl = {
    GifFrameDecode_QueryInterface,
    GifFrameDecode_AddRef,
    GifFrameDecode_Release,
    GifFrameDecode_GetSize,
    GifFrameDecode_GetPixelFormat,
    GifFrameDecode_GetResolution,
    GifFrameDecode_CopyPalette,
    GifFrameDecode_CopyPixels,
    GifFrameDecode_GetMetadataQueryReader,
    GifFrameDecode_GetColorContexts,
    GifFrameDecode_GetThumbnail
};

static HRESULT WINAPI GifFrameDecode_Block_QueryInterface(IWICMetadataBlockReader *iface,
    REFIID iid, void **ppv)
{
    GifFrameDecode *This = frame_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI GifFrameDecode_Block_AddRef(IWICMetadataBlockReader *iface)
{
    GifFrameDecode *This = frame_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_AddRef(&This->IWICBitmapFrameDecode_iface);
}

static ULONG WINAPI GifFrameDecode_Block_Release(IWICMetadataBlockReader *iface)
{
    GifFrameDecode *This = frame_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_Release(&This->IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI GifFrameDecode_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *guid)
{
    TRACE("(%p,%p)\n", iface, guid);

    if (!guid) return E_INVALIDARG;

    *guid = GUID_ContainerFormatGif;
    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *count)
{
    GifFrameDecode *This = frame_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%p\n", iface, count);

    if (!count) return E_INVALIDARG;

    *count = This->frame->Extensions.ExtensionBlockCount + 1;
    return S_OK;
}

static HRESULT create_IMD_metadata_reader(GifFrameDecode *This, IWICMetadataReader **reader)
{
    HRESULT hr;
    IWICMetadataReader *metadata_reader;
    IWICPersistStream *persist;
    IStream *stream;
    struct image_descriptor IMD_data;

    /* FIXME: Use IWICComponentFactory_CreateMetadataReader once it's implemented */

    hr = IMDReader_CreateInstance(&IID_IWICMetadataReader, (void **)&metadata_reader);
    if (FAILED(hr)) return hr;

    hr = IWICMetadataReader_QueryInterface(metadata_reader, &IID_IWICPersistStream, (void **)&persist);
    if (FAILED(hr))
    {
        IWICMetadataReader_Release(metadata_reader);
        return hr;
    }

    /* recreate IMD structure from GIF decoder data */
    IMD_data.left = This->frame->ImageDesc.Left;
    IMD_data.top = This->frame->ImageDesc.Top;
    IMD_data.width = This->frame->ImageDesc.Width;
    IMD_data.height = This->frame->ImageDesc.Height;
    IMD_data.packed = 0;
    /* interlace_flag */
    IMD_data.packed |= This->frame->ImageDesc.Interlace ? (1 << 6) : 0;
    if (This->frame->ImageDesc.ColorMap)
    {
        /* local_color_table_flag */
        IMD_data.packed |= 1 << 7;
        /* local_color_table_size */
        IMD_data.packed |= This->frame->ImageDesc.ColorMap->BitsPerPixel - 1;
        /* sort_flag */
        IMD_data.packed |= This->frame->ImageDesc.ColorMap->SortFlag ? 0x20 : 0;
    }

    stream = create_stream(&IMD_data, sizeof(IMD_data));
    IWICPersistStream_LoadEx(persist, stream, NULL, WICPersistOptionsDefault);
    IStream_Release(stream);

    IWICPersistStream_Release(persist);

    *reader = metadata_reader;
    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT index, IWICMetadataReader **reader)
{
    GifFrameDecode *This = frame_from_IWICMetadataBlockReader(iface);
    int i, gce_index = -1, gce_skipped = 0;

    TRACE("(%p,%u,%p)\n", iface, index, reader);

    if (!reader) return E_INVALIDARG;

    if (index == 0)
        return create_IMD_metadata_reader(This, reader);

    if (index >= This->frame->Extensions.ExtensionBlockCount + 1)
        return E_INVALIDARG;

    for (i = 0; i < This->frame->Extensions.ExtensionBlockCount; i++)
    {
        class_constructor constructor;
        const void *data;
        int data_size;

        if (index != i + 1 - gce_skipped) continue;

        if (This->frame->Extensions.ExtensionBlocks[i].Function == GRAPHICS_EXT_FUNC_CODE)
        {
            gce_index = i;
            gce_skipped = 1;
            continue;
        }
        else if (This->frame->Extensions.ExtensionBlocks[i].Function == COMMENT_EXT_FUNC_CODE)
        {
            constructor = GifCommentReader_CreateInstance;
            data = This->frame->Extensions.ExtensionBlocks[i].Bytes;
            data_size = This->frame->Extensions.ExtensionBlocks[i].ByteCount;
        }
        else
        {
            constructor = UnknownMetadataReader_CreateInstance;
            data = This->frame->Extensions.ExtensionBlocks[i].Bytes;
            data_size = This->frame->Extensions.ExtensionBlocks[i].ByteCount;
        }
        return create_metadata_reader(data, data_size, constructor, reader);
    }

    if (gce_index == -1) return E_INVALIDARG;

    return create_metadata_reader(This->frame->Extensions.ExtensionBlocks[gce_index].Bytes + 3,
                                  This->frame->Extensions.ExtensionBlocks[gce_index].ByteCount - 4,
                                  GCEReader_CreateInstance, reader);
}

static HRESULT WINAPI GifFrameDecode_Block_GetEnumerator(IWICMetadataBlockReader *iface,
    IEnumUnknown **enumerator)
{
    FIXME("(%p,%p): stub\n", iface, enumerator);
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl GifFrameDecode_BlockVtbl =
{
    GifFrameDecode_Block_QueryInterface,
    GifFrameDecode_Block_AddRef,
    GifFrameDecode_Block_Release,
    GifFrameDecode_Block_GetContainerFormat,
    GifFrameDecode_Block_GetCount,
    GifFrameDecode_Block_GetReaderByIndex,
    GifFrameDecode_Block_GetEnumerator
};

static HRESULT WINAPI GifDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoder, iid))
    {
        *ppv = &This->IWICBitmapDecoder_iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockReader, iid))
    {
        *ppv = &This->IWICMetadataBlockReader_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI GifDecoder_AddRef(IWICBitmapDecoder *iface)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifDecoder_Release(IWICBitmapDecoder *iface)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream)
        {
            IStream_Release(This->stream);
            DGifCloseFile(This->gif);
        }
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI GifDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = WICBitmapDecoderCapabilityCanDecodeAllImages |
                  WICBitmapDecoderCapabilityCanDecodeSomeImages |
                  WICBitmapDecoderCapabilityCanEnumerateMetadata;
    return S_OK;
}

static int _gif_inputfunc(GifFileType *gif, GifByteType *data, int len) {
    IStream *stream = gif->UserData;
    ULONG bytesread;
    HRESULT hr;

    if (!stream)
    {
        ERR("attempting to read file after initialization\n");
        return 0;
    }

    hr = IStream_Read(stream, data, len, &bytesread);
    if (FAILED(hr)) bytesread = 0;
    return bytesread;
}

static HRESULT WINAPI GifDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    LARGE_INTEGER seek;
    int ret;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized || This->gif)
    {
        WARN("already initialized\n");
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* seek to start of stream */
    seek.QuadPart = 0;
    IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);

    /* read all data from the stream */
    This->gif = DGifOpen((void*)pIStream, _gif_inputfunc);
    if (!This->gif)
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    ret = DGifSlurp(This->gif);
    if (ret == GIF_ERROR)
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    /* make sure we don't use the stream after this method returns */
    This->gif->UserData = NULL;

    seek.QuadPart = 0;
    IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    IStream_Read(pIStream, This->LSD_data, sizeof(This->LSD_data), NULL);

    This->stream = pIStream;
    IStream_AddRef(This->stream);

    This->initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI GifDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatGif, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI GifDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    HRESULT hr;
    IWICComponentInfo *compinfo;

    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    hr = CreateComponentInfo(&CLSID_WICGifDecoder, &compinfo);
    if (FAILED(hr)) return hr;

    hr = IWICComponentInfo_QueryInterface(compinfo, &IID_IWICBitmapDecoderInfo,
        (void**)ppIDecoderInfo);

    IWICComponentInfo_Release(compinfo);

    return hr;
}

static HRESULT WINAPI GifDecoder_CopyPalette(IWICBitmapDecoder *iface, IWICPalette *palette)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    WICColor colors[256];
    ColorMapObject *cm;
    int i, trans, count;
    ExtensionBlock *eb;

    TRACE("(%p,%p)\n", iface, palette);

    cm = This->gif->SColorMap;
    if (cm)
    {
        if (cm->ColorCount > 256)
        {
            ERR("GIF contains invalid number of colors: %d\n", cm->ColorCount);
            return E_FAIL;
        }

        for (i = 0; i < cm->ColorCount; i++)
        {
            colors[i] = 0xff000000 | /* alpha */
                        cm->Colors[i].Red << 16 |
                        cm->Colors[i].Green << 8 |
                        cm->Colors[i].Blue;
        }

        count = cm->ColorCount;
    }
    else
    {
        colors[0] = 0xff000000;
        colors[1] = 0xffffffff;

        for (i = 2; i < 256; i++)
            colors[i] = 0xff000000;

        count = 256;
    }

    /* look for the transparent color extension */
    for (i = 0; i < This->gif->SavedImages[This->current_frame].Extensions.ExtensionBlockCount; i++)
    {
        eb = This->gif->SavedImages[This->current_frame].Extensions.ExtensionBlocks + i;
        if (eb->Function == GRAPHICS_EXT_FUNC_CODE && eb->ByteCount == 8)
        {
            if (eb->Bytes[3] & 1)
            {
                trans = (unsigned char)eb->Bytes[6];
                colors[trans] &= 0xffffff; /* set alpha to 0 */
                break;
            }
        }
    }

    return IWICPalette_InitializeCustom(palette, colors, count);
}

static HRESULT WINAPI GifDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI GifDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);

    if (!pCount) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pCount = This->gif ? This->gif->ImageCount : 0;
    LeaveCriticalSection(&This->lock);

    TRACE("(%p) <-- %d\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI GifDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    GifFrameDecode *result;
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_FRAMEMISSING;

    if (index >= This->gif->ImageCount) return E_INVALIDARG;

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(GifFrameDecode));
    if (!result) return E_OUTOFMEMORY;

    result->IWICBitmapFrameDecode_iface.lpVtbl = &GifFrameDecode_Vtbl;
    result->IWICMetadataBlockReader_iface.lpVtbl = &GifFrameDecode_BlockVtbl;
    result->ref = 1;
    result->frame = &This->gif->SavedImages[index];
    IWICBitmapDecoder_AddRef(iface);
    result->parent = This;
    This->current_frame = index;

    *ppIBitmapFrame = &result->IWICBitmapFrameDecode_iface;

    return S_OK;
}

static const IWICBitmapDecoderVtbl GifDecoder_Vtbl = {
    GifDecoder_QueryInterface,
    GifDecoder_AddRef,
    GifDecoder_Release,
    GifDecoder_QueryCapability,
    GifDecoder_Initialize,
    GifDecoder_GetContainerFormat,
    GifDecoder_GetDecoderInfo,
    GifDecoder_CopyPalette,
    GifDecoder_GetMetadataQueryReader,
    GifDecoder_GetPreview,
    GifDecoder_GetColorContexts,
    GifDecoder_GetThumbnail,
    GifDecoder_GetFrameCount,
    GifDecoder_GetFrame
};

static HRESULT WINAPI GifDecoder_Block_QueryInterface(IWICMetadataBlockReader *iface,
    REFIID iid, void **ppv)
{
    GifDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
}

static ULONG WINAPI GifDecoder_Block_AddRef(IWICMetadataBlockReader *iface)
{
    GifDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI GifDecoder_Block_Release(IWICMetadataBlockReader *iface)
{
    GifDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI GifDecoder_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *guid)
{
    TRACE("(%p,%p)\n", iface, guid);

    if (!guid) return E_INVALIDARG;

    *guid = GUID_ContainerFormatGif;
    return S_OK;
}

static HRESULT WINAPI GifDecoder_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *count)
{
    GifDecoder *This = impl_from_IWICMetadataBlockReader(iface);

    TRACE("%p,%p\n", iface, count);

    if (!count) return E_INVALIDARG;

    *count = This->gif->Extensions.ExtensionBlockCount + 1;
    return S_OK;
}

static HRESULT WINAPI GifDecoder_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT index, IWICMetadataReader **reader)
{
    GifDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    int i;

    TRACE("(%p,%u,%p)\n", iface, index, reader);

    if (!reader) return E_INVALIDARG;

    if (index == 0)
        return create_metadata_reader(This->LSD_data, sizeof(This->LSD_data),
                                      LSDReader_CreateInstance, reader);

    for (i = 0; i < This->gif->Extensions.ExtensionBlockCount; i++)
    {
        class_constructor constructor;

        if (index != i + 1) continue;

        if (This->gif->Extensions.ExtensionBlocks[i].Function == APPLICATION_EXT_FUNC_CODE)
            constructor = APEReader_CreateInstance;
        else if (This->gif->Extensions.ExtensionBlocks[i].Function == COMMENT_EXT_FUNC_CODE)
            constructor = GifCommentReader_CreateInstance;
        else
            constructor = UnknownMetadataReader_CreateInstance;

        return create_metadata_reader(This->gif->Extensions.ExtensionBlocks[i].Bytes,
                                      This->gif->Extensions.ExtensionBlocks[i].ByteCount,
                                      constructor, reader);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI GifDecoder_Block_GetEnumerator(IWICMetadataBlockReader *iface,
    IEnumUnknown **enumerator)
{
    FIXME("(%p,%p): stub\n", iface, enumerator);
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl GifDecoder_BlockVtbl =
{
    GifDecoder_Block_QueryInterface,
    GifDecoder_Block_AddRef,
    GifDecoder_Block_Release,
    GifDecoder_Block_GetContainerFormat,
    GifDecoder_Block_GetCount,
    GifDecoder_Block_GetReaderByIndex,
    GifDecoder_Block_GetEnumerator
};

HRESULT GifDecoder_CreateInstance(REFIID iid, void** ppv)
{
    GifDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(GifDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &GifDecoder_Vtbl;
    This->IWICMetadataBlockReader_iface.lpVtbl = &GifDecoder_BlockVtbl;
    This->stream = NULL;
    This->ref = 1;
    This->initialized = FALSE;
    This->gif = NULL;
    This->current_frame = 0;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": GifDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}
