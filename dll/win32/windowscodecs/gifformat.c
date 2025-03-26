/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012,2016 Dmitry Timoshkov
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlwapi.h"

#include "ungif.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#ifdef __REACTOS__
#include <ole2.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

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
};

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

static HRESULT load_LSD_metadata(IStream *stream, const GUID *vendor, DWORD options,
                                 MetadataItem **items, DWORD *count)
{
    struct logical_screen_descriptor lsd_data;
    HRESULT hr;
    ULONG bytesread, i;
    MetadataItem *result;

    *items = NULL;
    *count = 0;

    hr = IStream_Read(stream, &lsd_data, sizeof(lsd_data), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(lsd_data)) return S_OK;

    result = calloc(9, sizeof(MetadataItem));
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 9; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Signature", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI1|VT_VECTOR;
    result[0].value.caub.cElems = sizeof(lsd_data.signature);
    result[0].value.caub.pElems = CoTaskMemAlloc(sizeof(lsd_data.signature));
    memcpy(result[0].value.caub.pElems, lsd_data.signature, sizeof(lsd_data.signature));

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"Width", &result[1].id.pwszVal);
    result[1].value.vt = VT_UI2;
    result[1].value.uiVal = lsd_data.width;

    result[2].id.vt = VT_LPWSTR;
    SHStrDupW(L"Height", &result[2].id.pwszVal);
    result[2].value.vt = VT_UI2;
    result[2].value.uiVal = lsd_data.height;

    result[3].id.vt = VT_LPWSTR;
    SHStrDupW(L"GlobalColorTableFlag", &result[3].id.pwszVal);
    result[3].value.vt = VT_BOOL;
    result[3].value.boolVal = (lsd_data.packed >> 7) & 1;

    result[4].id.vt = VT_LPWSTR;
    SHStrDupW(L"ColorResolution", &result[4].id.pwszVal);
    result[4].value.vt = VT_UI1;
    result[4].value.bVal = (lsd_data.packed >> 4) & 7;

    result[5].id.vt = VT_LPWSTR;
    SHStrDupW(L"SortFlag", &result[5].id.pwszVal);
    result[5].value.vt = VT_BOOL;
    result[5].value.boolVal = (lsd_data.packed >> 3) & 1;

    result[6].id.vt = VT_LPWSTR;
    SHStrDupW(L"GlobalColorTableSize", &result[6].id.pwszVal);
    result[6].value.vt = VT_UI1;
    result[6].value.bVal = lsd_data.packed & 7;

    result[7].id.vt = VT_LPWSTR;
    SHStrDupW(L"BackgroundColorIndex", &result[7].id.pwszVal);
    result[7].value.vt = VT_UI1;
    result[7].value.bVal = lsd_data.background_color_index;

    result[8].id.vt = VT_LPWSTR;
    SHStrDupW(L"PixelAspectRatio", &result[8].id.pwszVal);
    result[8].value.vt = VT_UI1;
    result[8].value.bVal = lsd_data.pixel_aspect_ratio;

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

    result = calloc(8, sizeof(MetadataItem));
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 8; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Left", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI2;
    result[0].value.uiVal = imd_data.left;

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"Top", &result[1].id.pwszVal);
    result[1].value.vt = VT_UI2;
    result[1].value.uiVal = imd_data.top;

    result[2].id.vt = VT_LPWSTR;
    SHStrDupW(L"Width", &result[2].id.pwszVal);
    result[2].value.vt = VT_UI2;
    result[2].value.uiVal = imd_data.width;

    result[3].id.vt = VT_LPWSTR;
    SHStrDupW(L"Height", &result[3].id.pwszVal);
    result[3].value.vt = VT_UI2;
    result[3].value.uiVal = imd_data.height;

    result[4].id.vt = VT_LPWSTR;
    SHStrDupW(L"LocalColorTableFlag", &result[4].id.pwszVal);
    result[4].value.vt = VT_BOOL;
    result[4].value.boolVal = (imd_data.packed >> 7) & 1;

    result[5].id.vt = VT_LPWSTR;
    SHStrDupW(L"InterlaceFlag", &result[5].id.pwszVal);
    result[5].value.vt = VT_BOOL;
    result[5].value.boolVal = (imd_data.packed >> 6) & 1;

    result[6].id.vt = VT_LPWSTR;
    SHStrDupW(L"SortFlag", &result[6].id.pwszVal);
    result[6].value.vt = VT_BOOL;
    result[6].value.boolVal = (imd_data.packed >> 5) & 1;

    result[7].id.vt = VT_LPWSTR;
    SHStrDupW(L"LocalColorTableSize", &result[7].id.pwszVal);
    result[7].value.vt = VT_UI1;
    result[7].value.bVal = imd_data.packed & 7;

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
    struct graphic_control_extension
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

    result = calloc(5, sizeof(MetadataItem));
    if (!result) return E_OUTOFMEMORY;

    for (i = 0; i < 5; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Disposal", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI1;
    result[0].value.bVal = (gce_data.packed >> 2) & 7;

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"UserInputFlag", &result[1].id.pwszVal);
    result[1].value.vt = VT_BOOL;
    result[1].value.boolVal = (gce_data.packed >> 1) & 1;

    result[2].id.vt = VT_LPWSTR;
    SHStrDupW(L"TransparencyFlag", &result[2].id.pwszVal);
    result[2].value.vt = VT_BOOL;
    result[2].value.boolVal = gce_data.packed & 1;

    result[3].id.vt = VT_LPWSTR;
    SHStrDupW(L"Delay", &result[3].id.pwszVal);
    result[3].value.vt = VT_UI2;
    result[3].value.uiVal = gce_data.delay;

    result[4].id.vt = VT_LPWSTR;
    SHStrDupW(L"TransparentColorIndex", &result[4].id.pwszVal);
    result[4].value.vt = VT_UI1;
    result[4].value.bVal = gce_data.transparent_color_index;

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
    struct application_extension
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
            CoTaskMemFree(data);
            return S_OK;
        }
        if (!subblock_size) break;

        if (!data)
            data = CoTaskMemAlloc(subblock_size + 1);
        else
        {
            BYTE *new_data = CoTaskMemRealloc(data, data_size + subblock_size + 1);
            if (!new_data)
            {
                CoTaskMemFree(data);
                return S_OK;
            }
            data = new_data;
        }
        data[data_size] = subblock_size;
        hr = IStream_Read(stream, data + data_size + 1, subblock_size, &bytesread);
        if (FAILED(hr) || bytesread != subblock_size)
        {
            CoTaskMemFree(data);
            return S_OK;
        }
        data_size += subblock_size + 1;
    }

    result = calloc(2, sizeof(MetadataItem));
    if (!result)
    {
        CoTaskMemFree(data);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < 2; i++)
    {
        PropVariantInit(&result[i].schema);
        PropVariantInit(&result[i].id);
        PropVariantInit(&result[i].value);
    }

    result[0].id.vt = VT_LPWSTR;
    SHStrDupW(L"Application", &result[0].id.pwszVal);
    result[0].value.vt = VT_UI1|VT_VECTOR;
    result[0].value.caub.cElems = sizeof(ape_data.application);
    result[0].value.caub.pElems = CoTaskMemAlloc(sizeof(ape_data.application));
    memcpy(result[0].value.caub.pElems, ape_data.application, sizeof(ape_data.application));

    result[1].id.vt = VT_LPWSTR;
    SHStrDupW(L"Data", &result[1].id.pwszVal);
    result[1].value.vt = VT_UI1|VT_VECTOR;
    result[1].value.caub.cElems = data_size;
    result[1].value.caub.pElems = data;

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
    struct gif_extension
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
            CoTaskMemFree(data);
            return S_OK;
        }
        if (!subblock_size) break;

        if (!data)
            data = CoTaskMemAlloc(subblock_size + 1);
        else
        {
            char *new_data = CoTaskMemRealloc(data, data_size + subblock_size + 1);
            if (!new_data)
            {
                CoTaskMemFree(data);
                return S_OK;
            }
            data = new_data;
        }
        hr = IStream_Read(stream, data + data_size, subblock_size, &bytesread);
        if (FAILED(hr) || bytesread != subblock_size)
        {
            CoTaskMemFree(data);
            return S_OK;
        }
        data_size += subblock_size;
    }

    data[data_size] = 0;

    result = calloc(1, sizeof(MetadataItem));
    if (!result)
    {
        CoTaskMemFree(data);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result->schema);
    PropVariantInit(&result->id);
    PropVariantInit(&result->value);

    result->id.vt = VT_LPWSTR;
    SHStrDupW(L"TextEntry", &result->id.pwszVal);
    result->value.vt = VT_LPSTR;
    result->value.pszVal = data;

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
    IWICPersistStream_LoadEx(persist, stream, NULL, WICPersistOptionDefault);
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

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapDecoder_Release(&This->parent->IWICBitmapDecoder_iface);
        free(This);
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

static void copy_palette(ColorMapObject *cm, Extensions *extensions, int count, WICColor *colors)
{
    int i;

    if (cm)
    {
        for (i = 0; i < count; i++)
        {
            colors[i] = 0xff000000 | /* alpha */
                        cm->Colors[i].Red << 16 |
                        cm->Colors[i].Green << 8 |
                        cm->Colors[i].Blue;
        }
    }
    else
    {
        colors[0] = 0xff000000;
        colors[1] = 0xffffffff;
        for (i = 2; i < count; i++)
            colors[i] = 0xff000000;
    }

    /* look for the transparent color extension */
    for (i = 0; i < extensions->ExtensionBlockCount; i++)
    {
        ExtensionBlock *eb = extensions->ExtensionBlocks + i;
        if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
            eb->ByteCount == 8 && eb->Bytes[3] & 1)
        {
            int trans = (unsigned char)eb->Bytes[6];
            colors[trans] &= 0xffffff; /* set alpha to 0 */
            break;
        }
    }
}

static HRESULT WINAPI GifFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    WICColor colors[256];
    ColorMapObject *cm = This->frame->ImageDesc.ColorMap;
    int count;

    TRACE("(%p,%p)\n", iface, pIPalette);

    if (cm)
        count = cm->ColorCount;
    else
    {
        cm = This->parent->gif->SColorMap;
        count = This->parent->gif->SColorTableSize;
    }

    if (count > 256)
    {
        ERR("GIF contains %i colors???\n", count);
        return E_FAIL;
    }

    copy_palette(cm, &This->frame->Extensions, count, colors);

    return IWICPalette_InitializeCustom(pIPalette, colors, count);
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
    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

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
    GifFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);

    if (!ppIMetadataQueryReader)
        return E_INVALIDARG;

    return MetadataQueryReader_CreateInstance(&This->IWICMetadataBlockReader_iface, NULL, ppIMetadataQueryReader);
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
    IWICPersistStream_LoadEx(persist, stream, NULL, WICPersistOptionDefault);
    IStream_Release(stream);

    IWICPersistStream_Release(persist);

    *reader = metadata_reader;
    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT index, IWICMetadataReader **reader)
{
    GifFrameDecode *This = frame_from_IWICMetadataBlockReader(iface);
    class_constructor constructor;
    ExtensionBlock *ext;
    const void *data;
    int data_size;

    TRACE("(%p,%u,%p)\n", iface, index, reader);

    if (!reader) return E_INVALIDARG;

    if (index == 0)
        return create_IMD_metadata_reader(This, reader);

    if (index >= This->frame->Extensions.ExtensionBlockCount + 1)
        return E_INVALIDARG;

    ext = This->frame->Extensions.ExtensionBlocks + index - 1;
    if (ext->Function == GRAPHICS_EXT_FUNC_CODE)
    {
        constructor = GCEReader_CreateInstance;
        data = ext->Bytes + 3;
        data_size = ext->ByteCount - 4;
    }
    else if (ext->Function == COMMENT_EXT_FUNC_CODE)
    {
        constructor = GifCommentReader_CreateInstance;
        data = ext->Bytes;
        data_size = ext->ByteCount;
    }
    else
    {
        constructor = UnknownMetadataReader_CreateInstance;
        data = ext->Bytes;
        data_size = ext->ByteCount;
    }

    return create_metadata_reader(data, data_size, constructor, reader);
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

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifDecoder_Release(IWICBitmapDecoder *iface)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream)
        {
            IStream_Release(This->stream);
            DGifCloseFile(This->gif);
        }
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        free(This);
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
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WICGifDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI GifDecoder_CopyPalette(IWICBitmapDecoder *iface, IWICPalette *palette)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);
    WICColor colors[256];
    ColorMapObject *cm;
    int count;

    TRACE("(%p,%p)\n", iface, palette);

    if (!This->gif)
        return WINCODEC_ERR_WRONGSTATE;

    cm = This->gif->SColorMap;
    count = This->gif->SColorTableSize;

    if (count > 256)
    {
        ERR("GIF contains invalid number of colors: %d\n", count);
        return E_FAIL;
    }

    copy_palette(cm, &This->gif->SavedImages[This->current_frame].Extensions, count, colors);

    return IWICPalette_InitializeCustom(palette, colors, count);
}

static HRESULT WINAPI GifDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    GifDecoder *This = impl_from_IWICBitmapDecoder(iface);

    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);

    if (!ppIMetadataQueryReader) return E_INVALIDARG;

    return MetadataQueryReader_CreateInstance(&This->IWICMetadataBlockReader_iface, NULL, ppIMetadataQueryReader);
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

    result = malloc(sizeof(GifFrameDecode));
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

    This = malloc(sizeof(GifDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &GifDecoder_Vtbl;
    This->IWICMetadataBlockReader_iface.lpVtbl = &GifDecoder_BlockVtbl;
    This->stream = NULL;
    This->ref = 1;
    This->initialized = FALSE;
    This->gif = NULL;
    This->current_frame = 0;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": GifDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}

typedef struct GifEncoder
{
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock;
    BOOL initialized, info_written, committed;
    UINT n_frames;
    WICColor palette[256];
    UINT colors;
} GifEncoder;

static inline GifEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, GifEncoder, IWICBitmapEncoder_iface);
}

typedef struct GifFrameEncode
{
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    IWICMetadataBlockWriter IWICMetadataBlockWriter_iface;
    LONG ref;
    GifEncoder *encoder;
    BOOL initialized, interlace, committed;
    UINT width, height, lines;
    double xres, yres;
    WICColor palette[256];
    UINT colors;
    BYTE *image_data;
} GifFrameEncode;

static inline GifFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, GifFrameEncode, IWICBitmapFrameEncode_iface);
}

static inline GifFrameEncode *impl_from_IWICMetadataBlockWriter(IWICMetadataBlockWriter *iface)
{
    return CONTAINING_RECORD(iface, GifFrameEncode, IWICMetadataBlockWriter_iface);
}

static HRESULT WINAPI GifFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid, void **ppv)
{
    GifFrameEncode *encoder = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("%p,%s,%p\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockWriter, iid))
    {
        *ppv = &encoder->IWICMetadataBlockWriter_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI GifFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI GifFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);

    if (!ref)
    {
        IWICBitmapEncoder_Release(&This->encoder->IWICBitmapEncoder_iface);
        free(This->image_data);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI GifFrameEncode_Initialize(IWICBitmapFrameEncode *iface, IPropertyBag2 *options)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, options);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized)
    {
        This->initialized = TRUE;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetSize(IWICBitmapFrameEncode *iface, UINT width, UINT height)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%u,%u\n", iface, width, height);

    if (!width || !height) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        free(This->image_data);

        This->image_data = malloc(width * height);
        if (This->image_data)
        {
            This->width = width;
            This->height = height;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetResolution(IWICBitmapFrameEncode *iface, double xres, double yres)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%f,%f\n", iface, xres, yres);

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        This->xres = xres;
        This->yres = yres;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface, WICPixelFormatGUID *format)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%s\n", iface, debugstr_guid(format));

    if (!format) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        *format = GUID_WICPixelFormat8bppIndexed;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);

    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface, UINT count, IWICColorContext **context)
{
    FIXME("%p,%u,%p: stub\n", iface, count, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifFrameEncode_SetPalette(IWICBitmapFrameEncode *iface, IWICPalette *palette)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI GifFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface, IWICBitmapSource *thumbnail)
{
    FIXME("%p,%p: stub\n", iface, thumbnail);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifFrameEncode_WritePixels(IWICBitmapFrameEncode *iface, UINT lines, UINT stride, UINT size, BYTE *pixels)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%u,%u,%u,%p\n", iface, lines, stride, size, pixels);

    if (!pixels) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized && This->image_data)
    {
        if (This->lines + lines <= This->height)
        {
            UINT i;
            BYTE *src, *dst;

            src = pixels;
            dst = This->image_data + This->lines * This->width;

            for (i = 0; i < lines; i++)
            {
                memcpy(dst, src, This->width);
                src += stride;
                dst += This->width;
            }

            This->lines += lines;
            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI GifFrameEncode_WriteSource(IWICBitmapFrameEncode *iface, IWICBitmapSource *source, WICRect *rc)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p,%p,%p\n", iface, source, rc);

    if (!source) return E_INVALIDARG;

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        const GUID *format = &GUID_WICPixelFormat8bppIndexed;

        hr = configure_write_source(iface, source, rc, format,
                                    This->width, This->height, This->xres, This->yres);
        if (hr == S_OK)
            hr = write_source(iface, source, rc, format, 8, !This->colors, This->width, This->height);
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

#define LZW_DICT_SIZE (1 << 12)

struct lzw_dict
{
    short prefix[LZW_DICT_SIZE];
    unsigned char suffix[LZW_DICT_SIZE];
};

struct lzw_state
{
    struct lzw_dict dict;
    short init_code_bits, code_bits, next_code, clear_code, eof_code;
    unsigned bits_buf;
    int bits_count;
    int (*user_write_data)(void *user_ptr, void *data, int length);
    void *user_ptr;
};

struct input_stream
{
    unsigned len;
    const BYTE *in;
};

struct output_stream
{
    struct
    {
        unsigned char len;
        char data[255];
    } gif_block;
    IStream *out;
};

static int lzw_output_code(struct lzw_state *state, short code)
{
    state->bits_buf |= code << state->bits_count;
    state->bits_count += state->code_bits;

    while (state->bits_count >= 8)
    {
        unsigned char byte = (unsigned char)state->bits_buf;
        if (state->user_write_data(state->user_ptr, &byte, 1) != 1)
            return 0;
        state->bits_buf >>= 8;
        state->bits_count -= 8;
    }

    return 1;
}

static inline int lzw_output_clear_code(struct lzw_state *state)
{
    return lzw_output_code(state, state->clear_code);
}

static inline int lzw_output_eof_code(struct lzw_state *state)
{
    return lzw_output_code(state, state->eof_code);
}

static int lzw_flush_bits(struct lzw_state *state)
{
    unsigned char byte;

    while (state->bits_count >= 8)
    {
        byte = (unsigned char)state->bits_buf;
        if (state->user_write_data(state->user_ptr, &byte, 1) != 1)
            return 0;
        state->bits_buf >>= 8;
        state->bits_count -= 8;
    }

    if (state->bits_count)
    {
        static const char mask[8] = { 0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f };

        byte = (unsigned char)state->bits_buf & mask[state->bits_count];
        if (state->user_write_data(state->user_ptr, &byte, 1) != 1)
            return 0;
    }

    state->bits_buf = 0;
    state->bits_count = 0;

    return 1;
}

static void lzw_dict_reset(struct lzw_state *state)
{
    int i;

    state->code_bits = state->init_code_bits + 1;
    state->next_code = (1 << state->init_code_bits) + 2;

    for(i = 0; i < LZW_DICT_SIZE; i++)
    {
        state->dict.prefix[i] = 1 << 12; /* impossible LZW code value */
        state->dict.suffix[i] = 0;
    }
}

static void lzw_state_init(struct lzw_state *state, short init_code_bits, void *user_write_data, void *user_ptr)
{
    state->init_code_bits = init_code_bits;
    state->clear_code = 1 << init_code_bits;
    state->eof_code = state->clear_code + 1;
    state->bits_buf = 0;
    state->bits_count = 0;
    state->user_write_data = user_write_data;
    state->user_ptr = user_ptr;

    lzw_dict_reset(state);
}

static int lzw_dict_add(struct lzw_state *state, short prefix, unsigned char suffix)
{
    if (state->next_code < LZW_DICT_SIZE)
    {
        state->dict.prefix[state->next_code] = prefix;
        state->dict.suffix[state->next_code] = suffix;

        if ((state->next_code & (state->next_code - 1)) == 0)
            state->code_bits++;

        state->next_code++;
        return state->next_code;
    }

    return -1;
}

static short lzw_dict_lookup(const struct lzw_state *state, short prefix, unsigned char suffix)
{
    short i;

    for (i = 0; i < state->next_code; i++)
    {
        if (state->dict.prefix[i] == prefix && state->dict.suffix[i] == suffix)
            return i;
    }

    return -1;
}

static inline int write_byte(struct output_stream *out, char byte)
{
    if (out->gif_block.len == 255)
    {
        if (IStream_Write(out->out, &out->gif_block, sizeof(out->gif_block), NULL) != S_OK)
            return 0;

        out->gif_block.len = 0;
    }

    out->gif_block.data[out->gif_block.len++] = byte;

    return 1;
}

static int write_data(void *user_ptr, void *user_data, int length)
{
    unsigned char *data = user_data;
    struct output_stream *out = user_ptr;
    int len = length;

    while (len-- > 0)
    {
        if (!write_byte(out, *data++)) return 0;
    }

    return length;
}

static int flush_output_data(void *user_ptr)
{
    struct output_stream *out = user_ptr;

    if (out->gif_block.len)
    {
        if (IStream_Write(out->out, &out->gif_block, out->gif_block.len + sizeof(out->gif_block.len), NULL) != S_OK)
            return 0;
    }

    /* write GIF block terminator */
    out->gif_block.len = 0;
    return IStream_Write(out->out, &out->gif_block, sizeof(out->gif_block.len), NULL) == S_OK;
}

static inline int read_byte(struct input_stream *in, unsigned char *byte)
{
    if (in->len)
    {
        in->len--;
        *byte = *in->in++;
        return 1;
    }

    return 0;
}

static HRESULT gif_compress(IStream *out_stream, const BYTE *in_data, ULONG in_size)
{
    struct input_stream in;
    struct output_stream out;
    struct lzw_state state;
    short init_code_bits, prefix, code;
    unsigned char suffix;

    in.in = in_data;
    in.len = in_size;

    out.gif_block.len = 0;
    out.out = out_stream;

    init_code_bits = suffix = 8;
    if (IStream_Write(out.out, &suffix, sizeof(suffix), NULL) != S_OK)
        return E_FAIL;

    lzw_state_init(&state, init_code_bits, write_data, &out);

    if (!lzw_output_clear_code(&state))
        return E_FAIL;

    if (read_byte(&in, &suffix))
    {
        prefix = suffix;

        while (read_byte(&in, &suffix))
        {
            code = lzw_dict_lookup(&state, prefix, suffix);
            if (code == -1)
            {
                if (!lzw_output_code(&state, prefix))
                    return E_FAIL;

                if (lzw_dict_add(&state, prefix, suffix) == -1)
                {
                    if (!lzw_output_clear_code(&state))
                        return E_FAIL;
                    lzw_dict_reset(&state);
                }

                prefix = suffix;
            }
            else
                prefix = code;
        }

        if (!lzw_output_code(&state, prefix))
            return E_FAIL;
        if (!lzw_output_eof_code(&state))
            return E_FAIL;
        if (!lzw_flush_bits(&state))
            return E_FAIL;
    }

    return flush_output_data(&out) ? S_OK : E_FAIL;
}

static HRESULT WINAPI GifFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    GifFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("%p\n", iface);

    EnterCriticalSection(&This->encoder->lock);

    if (This->image_data && This->lines == This->height && !This->committed)
    {
        BYTE gif_palette[256][3];

        hr = S_OK;

        if (!This->encoder->info_written)
        {
            struct logical_screen_descriptor lsd;

            /* Logical Screen Descriptor */
            memcpy(lsd.signature, "GIF89a", 6);
            lsd.width = This->width;
            lsd.height = This->height;
            lsd.packed = 0;
            if (This->encoder->colors)
                lsd.packed |= 0x80; /* global color table flag */
            lsd.packed |= 0x07 << 4; /* color resolution */
            lsd.packed |= 0x07; /* global color table size */
            lsd.background_color_index = 0; /* FIXME */
            lsd.pixel_aspect_ratio = 0;
            hr = IStream_Write(This->encoder->stream, &lsd, sizeof(lsd), NULL);
            if (hr == S_OK && This->encoder->colors)
            {
                UINT i;

                /* Global Color Table */
                memset(gif_palette, 0, sizeof(gif_palette));
                for (i = 0; i < This->encoder->colors; i++)
                {
                    gif_palette[i][0] = (This->encoder->palette[i] >> 16) & 0xff;
                    gif_palette[i][1] = (This->encoder->palette[i] >> 8) & 0xff;
                    gif_palette[i][2] = This->encoder->palette[i] & 0xff;
                }
                hr = IStream_Write(This->encoder->stream, gif_palette, sizeof(gif_palette), NULL);
            }

            /* FIXME: write GCE, APE, etc. GIF extensions */

            if (hr == S_OK)
                This->encoder->info_written = TRUE;
        }

        if (hr == S_OK)
        {
            char image_separator = 0x2c;

            hr = IStream_Write(This->encoder->stream, &image_separator, sizeof(image_separator), NULL);
            if (hr == S_OK)
            {
                struct image_descriptor imd;

                /* Image Descriptor */
                imd.left = 0;
                imd.top = 0;
                imd.width = This->width;
                imd.height = This->height;
                imd.packed = 0;
                if (This->colors)
                {
                    imd.packed |= 0x80; /* local color table flag */
                    imd.packed |= 0x07; /* local color table size */
                }
                /* FIXME: interlace flag */
                hr = IStream_Write(This->encoder->stream, &imd, sizeof(imd), NULL);
                if (hr == S_OK && This->colors)
                {
                    UINT i;

                    /* Local Color Table */
                    memset(gif_palette, 0, sizeof(gif_palette));
                    for (i = 0; i < This->colors; i++)
                    {
                        gif_palette[i][0] = (This->palette[i] >> 16) & 0xff;
                        gif_palette[i][1] = (This->palette[i] >> 8) & 0xff;
                        gif_palette[i][2] = This->palette[i] & 0xff;
                    }
                    hr = IStream_Write(This->encoder->stream, gif_palette, sizeof(gif_palette), NULL);
                    if (hr == S_OK)
                    {
                        /* Image Data */
                        hr = gif_compress(This->encoder->stream, This->image_data, This->width * This->height);
                        if (hr == S_OK)
                            This->committed = TRUE;
                    }
                }
            }
        }
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI GifFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface, IWICMetadataQueryWriter **writer)
{
    GifFrameEncode *encode = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("iface, %p, writer %p.\n", iface, writer);

    if (!writer)
        return E_INVALIDARG;

    if (!encode->initialized)
        return WINCODEC_ERR_NOTINITIALIZED;

    return MetadataQueryWriter_CreateInstance(&encode->IWICMetadataBlockWriter_iface, NULL, writer);
}

static const IWICBitmapFrameEncodeVtbl GifFrameEncode_Vtbl =
{
    GifFrameEncode_QueryInterface,
    GifFrameEncode_AddRef,
    GifFrameEncode_Release,
    GifFrameEncode_Initialize,
    GifFrameEncode_SetSize,
    GifFrameEncode_SetResolution,
    GifFrameEncode_SetPixelFormat,
    GifFrameEncode_SetColorContexts,
    GifFrameEncode_SetPalette,
    GifFrameEncode_SetThumbnail,
    GifFrameEncode_WritePixels,
    GifFrameEncode_WriteSource,
    GifFrameEncode_Commit,
    GifFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI GifEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid, void **ppv)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        IWICBitmapEncoder_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI GifEncoder_AddRef(IWICBitmapEncoder *iface)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI GifEncoder_Release(IWICBitmapEncoder *iface)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p -> %lu\n", iface, ref);

    if (!ref)
    {
        if (This->stream) IStream_Release(This->stream);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI GifEncoder_Initialize(IWICBitmapEncoder *iface, IStream *stream, WICBitmapEncoderCacheOption option)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p,%p,%#x\n", iface, stream, option);

    if (!stream) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->initialized)
    {
        IStream_AddRef(stream);
        This->stream = stream;
        This->initialized = TRUE;
        hr = S_OK;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI GifEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    if (!format) return E_INVALIDARG;

    *format = GUID_ContainerFormatGif;
    return S_OK;
}

static HRESULT WINAPI GifEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICGifEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI GifEncoder_SetColorContexts(IWICBitmapEncoder *iface, UINT count, IWICColorContext **context)
{
    FIXME("%p,%u,%p: stub\n", iface, count, context);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI GifEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *thumbnail)
{
    TRACE("%p,%p\n", iface, thumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *preview)
{
    TRACE("%p,%p\n", iface, preview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifEncoderFrame_Block_QueryInterface(IWICMetadataBlockWriter *iface, REFIID iid, void **ppv)
{
    GifFrameEncode *frame_encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_QueryInterface(&frame_encoder->IWICBitmapFrameEncode_iface, iid, ppv);
}

static ULONG WINAPI GifEncoderFrame_Block_AddRef(IWICMetadataBlockWriter *iface)
{
    GifFrameEncode *frame_encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_AddRef(&frame_encoder->IWICBitmapFrameEncode_iface);
}

static ULONG WINAPI GifEncoderFrame_Block_Release(IWICMetadataBlockWriter *iface)
{
    GifFrameEncode *frame_encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_Release(&frame_encoder->IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI GifEncoderFrame_Block_GetContainerFormat(IWICMetadataBlockWriter *iface, GUID *container_format)
{
    FIXME("iface %p, container_format %p stub.\n", iface, container_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetCount(IWICMetadataBlockWriter *iface, UINT *count)
{
    FIXME("iface %p, count %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetReaderByIndex(IWICMetadataBlockWriter *iface,
        UINT index, IWICMetadataReader **metadata_reader)
{
    FIXME("iface %p, index %d, metadata_reader %p stub.\n", iface, index, metadata_reader);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetEnumerator(IWICMetadataBlockWriter *iface, IEnumUnknown **enum_metadata)
{
    FIXME("iface %p, enum_metadata %p stub.\n", iface, enum_metadata);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_InitializeFromBlockReader(IWICMetadataBlockWriter *iface,
        IWICMetadataBlockReader *block_reader)
{
    FIXME("iface %p, block_reader %p stub.\n", iface, block_reader);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_GetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter **metadata_writer)
{
    FIXME("iface %p, index %u, metadata_writer %p stub.\n", iface, index, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_AddWriter(IWICMetadataBlockWriter *iface, IWICMetadataWriter *metadata_writer)
{
    FIXME("iface %p, metadata_writer %p stub.\n", iface, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_SetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter *metadata_writer)
{
    FIXME("iface %p, index %u, metadata_writer %p stub.\n", iface, index, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI GifEncoderFrame_Block_RemoveWriterByIndex(IWICMetadataBlockWriter *iface, UINT index)
{
    FIXME("iface %p, index %u stub.\n", iface, index);

    return E_NOTIMPL;
}

static const IWICMetadataBlockWriterVtbl GifFrameEncode_BlockVtbl = {
    GifEncoderFrame_Block_QueryInterface,
    GifEncoderFrame_Block_AddRef,
    GifEncoderFrame_Block_Release,
    GifEncoderFrame_Block_GetContainerFormat,
    GifEncoderFrame_Block_GetCount,
    GifEncoderFrame_Block_GetReaderByIndex,
    GifEncoderFrame_Block_GetEnumerator,
    GifEncoderFrame_Block_InitializeFromBlockReader,
    GifEncoderFrame_Block_GetWriterByIndex,
    GifEncoderFrame_Block_AddWriter,
    GifEncoderFrame_Block_SetWriterByIndex,
    GifEncoderFrame_Block_RemoveWriterByIndex,
};

static HRESULT WINAPI GifEncoder_CreateNewFrame(IWICBitmapEncoder *iface, IWICBitmapFrameEncode **frame, IPropertyBag2 **options)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p,%p,%p\n", iface, frame, options);

    if (!frame) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->initialized && !This->committed)
    {
        GifFrameEncode *ret = malloc(sizeof(*ret));
        if (ret)
        {
            This->n_frames++;

            ret->IWICBitmapFrameEncode_iface.lpVtbl = &GifFrameEncode_Vtbl;
            ret->IWICMetadataBlockWriter_iface.lpVtbl = &GifFrameEncode_BlockVtbl;

            ret->ref = 1;
            ret->encoder = This;
            ret->initialized = FALSE;
            ret->interlace = FALSE; /* FIXME: read from the properties */
            ret->committed = FALSE;
            ret->width = 0;
            ret->height = 0;
            ret->lines = 0;
            ret->xres = 0.0;
            ret->yres = 0.0;
            ret->colors = 0;
            ret->image_data = NULL;
            IWICBitmapEncoder_AddRef(iface);
            *frame = &ret->IWICBitmapFrameEncode_iface;

            hr = S_OK;

            if (options)
            {
                hr = CreatePropertyBag2(NULL, 0, options);
                if (hr != S_OK)
                {
                    IWICBitmapFrameEncode_Release(*frame);
                    *frame = NULL;
                }
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->lock);

    return hr;

}

static HRESULT WINAPI GifEncoder_Commit(IWICBitmapEncoder *iface)
{
    GifEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("%p\n", iface);

    EnterCriticalSection(&This->lock);

    if (This->initialized && !This->committed)
    {
        char gif_trailer = 0x3b;

        /* FIXME: write text, comment GIF extensions */

        hr = IStream_Write(This->stream, &gif_trailer, sizeof(gif_trailer), NULL);
        if (hr == S_OK)
            This->committed = TRUE;
    }
    else
        hr = WINCODEC_ERR_WRONGSTATE;

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI GifEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface, IWICMetadataQueryWriter **writer)
{
    FIXME("%p,%p: stub\n", iface, writer);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl GifEncoder_Vtbl =
{
    GifEncoder_QueryInterface,
    GifEncoder_AddRef,
    GifEncoder_Release,
    GifEncoder_Initialize,
    GifEncoder_GetContainerFormat,
    GifEncoder_GetEncoderInfo,
    GifEncoder_SetColorContexts,
    GifEncoder_SetPalette,
    GifEncoder_SetThumbnail,
    GifEncoder_SetPreview,
    GifEncoder_CreateNewFrame,
    GifEncoder_Commit,
    GifEncoder_GetMetadataQueryWriter
};

HRESULT GifEncoder_CreateInstance(REFIID iid, void **ppv)
{
    GifEncoder *This;
    HRESULT ret;

    TRACE("%s,%p\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &GifEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": GifEncoder.lock");
    This->initialized = FALSE;
    This->info_written = FALSE;
    This->committed = FALSE;
    This->n_frames = 0;
    This->colors = 0;

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}
