/*
 * Copyright 2009 Vincent Povirk
 * Copyright 2016 Dmitry Timoshkov
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

#include "precomp.h"

static IWICImagingFactory *factory;

typedef struct bitmap_data {
    const WICPixelFormatGUID *format;
    UINT bpp;
    const BYTE *bits;
    UINT width;
    UINT height;
    double xres;
    double yres;
    const struct bitmap_data *alt_data;
} bitmap_data;

typedef struct BitmapTestSrc {
    IWICBitmapSource IWICBitmapSource_iface;
    LONG ref;
    const bitmap_data *data;
} BitmapTestSrc;

extern HRESULT STDMETHODCALLTYPE IWICBitmapFrameEncode_WriteSource_Proxy(IWICBitmapFrameEncode* This,
    IWICBitmapSource *pIBitmapSource, WICRect *prc);

static BOOL near_equal(float a, float b)
{
    return fabsf(a - b) < 0.001;
}

static inline BitmapTestSrc *impl_from_IWICBitmapSource(IWICBitmapSource *iface)
{
    return CONTAINING_RECORD(iface, BitmapTestSrc, IWICBitmapSource_iface);
}

static HRESULT WINAPI BitmapTestSrc_QueryInterface(IWICBitmapSource *iface, REFIID iid,
    void **ppv)
{
    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapTestSrc_AddRef(IWICBitmapSource *iface)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI BitmapTestSrc_Release(IWICBitmapSource *iface)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    return ref;
}

static HRESULT WINAPI BitmapTestSrc_GetSize(IWICBitmapSource *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    *puiWidth = This->data->width;
    *puiHeight = This->data->height;
    return S_OK;
}

static HRESULT WINAPI BitmapTestSrc_GetPixelFormat(IWICBitmapSource *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    memcpy(pPixelFormat, This->data->format, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI BitmapTestSrc_GetResolution(IWICBitmapSource *iface,
    double *pDpiX, double *pDpiY)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    *pDpiX = This->data->xres;
    *pDpiY = This->data->yres;
    return S_OK;
}

static HRESULT WINAPI BitmapTestSrc_CopyPalette(IWICBitmapSource *iface,
    IWICPalette *pIPalette)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapTestSrc_CopyPixels(IWICBitmapSource *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    BitmapTestSrc *This = impl_from_IWICBitmapSource(iface);
    UINT bytesperrow;
    UINT srcstride;
    UINT row_offset;
    WICRect rc;

    if (!prc)
    {
        rc.X = 0;
        rc.Y = 0;
        rc.Width = This->data->width;
        rc.Height = This->data->height;
        prc = &rc;
    }
    else
    {
        if (prc->X < 0 || prc->Y < 0 || prc->X+prc->Width > This->data->width || prc->Y+prc->Height > This->data->height)
            return E_INVALIDARG;
    }

    bytesperrow = ((This->data->bpp * prc->Width)+7)/8;
    srcstride = ((This->data->bpp * This->data->width)+7)/8;

    if (cbStride < bytesperrow)
        return E_INVALIDARG;

    if ((cbStride * prc->Height) > cbBufferSize)
        return E_INVALIDARG;

    row_offset = prc->X * This->data->bpp;

    if (row_offset % 8 == 0)
    {
        UINT row;
        const BYTE *src;
        BYTE *dst;

        src = This->data->bits + (row_offset / 8) + prc->Y * srcstride;
        dst = pbBuffer;
        for (row=0; row < prc->Height; row++)
        {
            memcpy(dst, src, bytesperrow);
            src += srcstride;
            dst += cbStride;
        }
        return S_OK;
    }
    else
    {
        ok(0, "bitmap %p was asked to copy pixels not aligned on a byte boundary\n", iface);
        return E_FAIL;
    }
}

static const IWICBitmapSourceVtbl BitmapTestSrc_Vtbl = {
    BitmapTestSrc_QueryInterface,
    BitmapTestSrc_AddRef,
    BitmapTestSrc_Release,
    BitmapTestSrc_GetSize,
    BitmapTestSrc_GetPixelFormat,
    BitmapTestSrc_GetResolution,
    BitmapTestSrc_CopyPalette,
    BitmapTestSrc_CopyPixels
};

static void CreateTestBitmap(const bitmap_data *data, BitmapTestSrc **This)
{
    *This = HeapAlloc(GetProcessHeap(), 0, sizeof(**This));

    if (*This)
    {
        (*This)->IWICBitmapSource_iface.lpVtbl = &BitmapTestSrc_Vtbl;
        (*This)->ref = 1;
        (*This)->data = data;
    }
}

static void DeleteTestBitmap(BitmapTestSrc *This)
{
    ok(This->IWICBitmapSource_iface.lpVtbl == &BitmapTestSrc_Vtbl, "test bitmap %p deleted with incorrect vtable\n", This);
    ok(This->ref == 1, "test bitmap %p deleted with %i references instead of 1\n", This, This->ref);
    HeapFree(GetProcessHeap(), 0, This);
}

static BOOL compare_bits(const struct bitmap_data *expect, UINT buffersize, const BYTE *converted_bits)
{
    BOOL equal;

    if (IsEqualGUID(expect->format, &GUID_WICPixelFormat32bppBGR))
    {
        /* ignore the padding byte when comparing data */
        UINT i;
        const DWORD *a=(const DWORD*)expect->bits, *b=(const DWORD*)converted_bits;
        equal=TRUE;
        for (i=0; i<(buffersize/4); i++)
            if ((a[i]&0xffffff) != (b[i]&0xffffff))
            {
                equal = FALSE;
                break;
            }
    }
    else if (IsEqualGUID(expect->format, &GUID_WICPixelFormat32bppGrayFloat))
    {
        UINT i;
        const float *a=(const float*)expect->bits, *b=(const float*)converted_bits;
        equal=TRUE;
        for (i=0; i<(buffersize/4); i++)
            if (!near_equal(a[i], b[i]))
            {
                equal = FALSE;
                break;
            }
    }
    else if (IsEqualGUID(expect->format, &GUID_WICPixelFormatBlackWhite) ||
             IsEqualGUID(expect->format, &GUID_WICPixelFormat1bppIndexed))
    {
        UINT i;
        const BYTE *a=(const BYTE*)expect->bits, *b=(const BYTE*)converted_bits;
        equal=TRUE;
        for (i=0; i<buffersize; i++)
            if (a[i] != b[i] && b[i] != 0xff /* BMP encoder B&W */)
            {
                equal = FALSE;
                break;
            }
    }
    else
        equal = (memcmp(expect->bits, converted_bits, buffersize) == 0);

    if (!equal && expect->alt_data)
        equal = compare_bits(expect->alt_data, buffersize, converted_bits);

    return equal;
}

static void compare_bitmap_data(const struct bitmap_data *expect, IWICBitmapSource *source, const char *name)
{
    BYTE *converted_bits;
    UINT width, height;
    double xres, yres;
    WICRect prc;
    UINT stride, buffersize;
    GUID dst_pixelformat;
    HRESULT hr;

    hr = IWICBitmapSource_GetSize(source, &width, &height);
    ok(SUCCEEDED(hr), "GetSize(%s) failed, hr=%x\n", name, hr);
    ok(width == expect->width, "expecting %u, got %u (%s)\n", expect->width, width, name);
    ok(height == expect->height, "expecting %u, got %u (%s)\n", expect->height, height, name);

    hr = IWICBitmapSource_GetResolution(source, &xres, &yres);
    ok(SUCCEEDED(hr), "GetResolution(%s) failed, hr=%x\n", name, hr);
    ok(fabs(xres - expect->xres) < 0.02, "expecting %0.2f, got %0.2f (%s)\n", expect->xres, xres, name);
    ok(fabs(yres - expect->yres) < 0.02, "expecting %0.2f, got %0.2f (%s)\n", expect->yres, yres, name);

    hr = IWICBitmapSource_GetPixelFormat(source, &dst_pixelformat);
    ok(SUCCEEDED(hr), "GetPixelFormat(%s) failed, hr=%x\n", name, hr);
    ok(IsEqualGUID(&dst_pixelformat, expect->format), "got unexpected pixel format %s (%s)\n", wine_dbgstr_guid(&dst_pixelformat), name);

    prc.X = 0;
    prc.Y = 0;
    prc.Width = expect->width;
    prc.Height = expect->height;

    stride = (expect->bpp * expect->width + 7) / 8;
    buffersize = stride * expect->height;

    converted_bits = HeapAlloc(GetProcessHeap(), 0, buffersize);
    hr = IWICBitmapSource_CopyPixels(source, &prc, stride, buffersize, converted_bits);
    ok(SUCCEEDED(hr), "CopyPixels(%s) failed, hr=%x\n", name, hr);
    ok(compare_bits(expect, buffersize, converted_bits), "unexpected pixel data (%s)\n", name);

    /* Test with NULL rectangle - should copy the whole bitmap */
    memset(converted_bits, 0xaa, buffersize);
    hr = IWICBitmapSource_CopyPixels(source, NULL, stride, buffersize, converted_bits);
    ok(SUCCEEDED(hr), "CopyPixels(%s,rc=NULL) failed, hr=%x\n", name, hr);
    ok(compare_bits(expect, buffersize, converted_bits), "unexpected pixel data (%s)\n", name);

    HeapFree(GetProcessHeap(), 0, converted_bits);
}

/* some encoders (like BMP) require data to be 4-bytes aligned */
static const BYTE bits_1bpp[] = {
    0x55,0x55,0x55,0x55,  /*01010101*/
    0xaa,0xaa,0xaa,0xaa}; /*10101010*/
static const struct bitmap_data testdata_BlackWhite = {
    &GUID_WICPixelFormatBlackWhite, 1, bits_1bpp, 32, 2, 96.0, 96.0};
static const struct bitmap_data testdata_1bppIndexed = {
    &GUID_WICPixelFormat1bppIndexed, 1, bits_1bpp, 32, 2, 96.0, 96.0};

static const BYTE bits_8bpp[] = {
    0,1,2,3,
    4,5,6,7};
static const struct bitmap_data testdata_8bppIndexed = {
    &GUID_WICPixelFormat8bppIndexed, 8, bits_8bpp, 4, 2, 96.0, 96.0};

static const BYTE bits_24bppBGR[] = {
    255,0,0, 0,255,0, 0,0,255, 0,0,0,
    0,255,255, 255,0,255, 255,255,0, 255,255,255};
static const struct bitmap_data testdata_24bppBGR = {
    &GUID_WICPixelFormat24bppBGR, 24, bits_24bppBGR, 4, 2, 96.0, 96.0};

static const BYTE bits_24bppRGB[] = {
    0,0,255, 0,255,0, 255,0,0, 0,0,0,
    255,255,0, 255,0,255, 0,255,255, 255,255,255};
static const struct bitmap_data testdata_24bppRGB = {
    &GUID_WICPixelFormat24bppRGB, 24, bits_24bppRGB, 4, 2, 96.0, 96.0};

static const BYTE bits_32bppBGR[] = {
    255,0,0,80, 0,255,0,80, 0,0,255,80, 0,0,0,80,
    0,255,255,80, 255,0,255,80, 255,255,0,80, 255,255,255,80};
static const struct bitmap_data testdata_32bppBGR = {
    &GUID_WICPixelFormat32bppBGR, 32, bits_32bppBGR, 4, 2, 96.0, 96.0};

static const BYTE bits_32bppBGRA[] = {
    255,0,0,255, 0,255,0,255, 0,0,255,255, 0,0,0,255,
    0,255,255,255, 255,0,255,255, 255,255,0,255, 255,255,255,255};
static const struct bitmap_data testdata_32bppBGRA = {
    &GUID_WICPixelFormat32bppBGRA, 32, bits_32bppBGRA, 4, 2, 96.0, 96.0};

/* XP and 2003 use linear color conversion, later versions use sRGB gamma */
static const float bits_32bppGrayFloat_xp[] = {
    0.114000f,0.587000f,0.299000f,0.000000f,
    0.886000f,0.413000f,0.701000f,1.000000f};
static const struct bitmap_data testdata_32bppGrayFloat_xp = {
    &GUID_WICPixelFormat32bppGrayFloat, 32, (const BYTE *)bits_32bppGrayFloat_xp, 4, 2, 96.0, 96.0};

static const float bits_32bppGrayFloat[] = {
    0.072200f,0.715200f,0.212600f,0.000000f,
    0.927800f,0.284800f,0.787400f,1.000000f};
static const struct bitmap_data testdata_32bppGrayFloat = {
    &GUID_WICPixelFormat32bppGrayFloat, 32, (const BYTE *)bits_32bppGrayFloat, 4, 2, 96.0, 96.0, &testdata_32bppGrayFloat_xp};

static const BYTE bits_8bppGray_xp[] = {
    29,150,76,0,
    226,105,179,255};
static const struct bitmap_data testdata_8bppGray_xp = {
    &GUID_WICPixelFormat8bppGray, 8, bits_8bppGray_xp, 4, 2, 96.0, 96.0};

static const BYTE bits_8bppGray[] = {
    76,220,127,0,
    247,145,230,255};
static const struct bitmap_data testdata_8bppGray = {
    &GUID_WICPixelFormat8bppGray, 8, bits_8bppGray, 4, 2, 96.0, 96.0, &testdata_8bppGray_xp};

static const BYTE bits_24bppBGR_gray[] = {
    76,76,76, 220,220,220, 127,127,127, 0,0,0,
    247,247,247, 145,145,145, 230,230,230, 255,255,255};
static const struct bitmap_data testdata_24bppBGR_gray = {
    &GUID_WICPixelFormat24bppBGR, 24, bits_24bppBGR_gray, 4, 2, 96.0, 96.0};

static void test_conversion(const struct bitmap_data *src, const struct bitmap_data *dst, const char *name, BOOL todo)
{
    BitmapTestSrc *src_obj;
    IWICBitmapSource *dst_bitmap;
    HRESULT hr;

    CreateTestBitmap(src, &src_obj);

    hr = WICConvertBitmapSource(dst->format, &src_obj->IWICBitmapSource_iface, &dst_bitmap);
    todo_wine_if (todo)
        ok(SUCCEEDED(hr), "WICConvertBitmapSource(%s) failed, hr=%x\n", name, hr);

    if (SUCCEEDED(hr))
    {
        compare_bitmap_data(dst, dst_bitmap, name);

        IWICBitmapSource_Release(dst_bitmap);
    }

    DeleteTestBitmap(src_obj);
}

static void test_invalid_conversion(void)
{
    BitmapTestSrc *src_obj;
    IWICBitmapSource *dst_bitmap;
    HRESULT hr;

    CreateTestBitmap(&testdata_32bppBGRA, &src_obj);

    /* convert to a non-pixel-format GUID */
    hr = WICConvertBitmapSource(&GUID_VendorMicrosoft, &src_obj->IWICBitmapSource_iface, &dst_bitmap);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "WICConvertBitmapSource returned %x\n", hr);

    DeleteTestBitmap(src_obj);
}

static void test_default_converter(void)
{
    BitmapTestSrc *src_obj;
    IWICFormatConverter *converter;
    BOOL can_convert = TRUE;
    HRESULT hr;

    CreateTestBitmap(&testdata_32bppBGRA, &src_obj);

    hr = CoCreateInstance(&CLSID_WICDefaultFormatConverter, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICFormatConverter, (void**)&converter);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICFormatConverter_CanConvert(converter, &GUID_WICPixelFormat32bppBGRA,
            &GUID_WICPixelFormat32bppBGR, &can_convert);
        ok(SUCCEEDED(hr), "CanConvert returned %x\n", hr);
        ok(can_convert, "expected TRUE, got %i\n", can_convert);

        hr = IWICFormatConverter_Initialize(converter, &src_obj->IWICBitmapSource_iface,
            &GUID_WICPixelFormat32bppBGR, WICBitmapDitherTypeNone, NULL, 0.0,
            WICBitmapPaletteTypeCustom);
        ok(SUCCEEDED(hr), "Initialize returned %x\n", hr);

        if (SUCCEEDED(hr))
            compare_bitmap_data(&testdata_32bppBGR, (IWICBitmapSource*)converter, "default converter");

        IWICFormatConverter_Release(converter);
    }

    DeleteTestBitmap(src_obj);
}

typedef struct property_opt_test_data
{
    LPCOLESTR name;
    VARTYPE var_type;
    VARTYPE initial_var_type;
    int i_init_val;
    float f_init_val;
    BOOL skippable;
} property_opt_test_data;

static const WCHAR wszTiffCompressionMethod[] = {'T','i','f','f','C','o','m','p','r','e','s','s','i','o','n','M','e','t','h','o','d',0};
static const WCHAR wszCompressionQuality[] = {'C','o','m','p','r','e','s','s','i','o','n','Q','u','a','l','i','t','y',0};
static const WCHAR wszInterlaceOption[] = {'I','n','t','e','r','l','a','c','e','O','p','t','i','o','n',0};
static const WCHAR wszFilterOption[] = {'F','i','l','t','e','r','O','p','t','i','o','n',0};
static const WCHAR wszImageQuality[] = {'I','m','a','g','e','Q','u','a','l','i','t','y',0};
static const WCHAR wszBitmapTransform[] = {'B','i','t','m','a','p','T','r','a','n','s','f','o','r','m',0};
static const WCHAR wszLuminance[] = {'L','u','m','i','n','a','n','c','e',0};
static const WCHAR wszChrominance[] = {'C','h','r','o','m','i','n','a','n','c','e',0};
static const WCHAR wszJpegYCrCbSubsampling[] = {'J','p','e','g','Y','C','r','C','b','S','u','b','s','a','m','p','l','i','n','g',0};
static const WCHAR wszSuppressApp0[] = {'S','u','p','p','r','e','s','s','A','p','p','0',0};

static const struct property_opt_test_data testdata_tiff_props[] = {
    { wszTiffCompressionMethod, VT_UI1,         VT_UI1,  WICTiffCompressionDontCare },
    { wszCompressionQuality,    VT_R4,          VT_EMPTY },
    { NULL }
};

static const struct property_opt_test_data testdata_png_props[] = {
    { wszInterlaceOption, VT_BOOL, VT_BOOL, 0 },
    { wszFilterOption,    VT_UI1,  VT_UI1, WICPngFilterUnspecified, 0.0f, TRUE /* not supported on XP/2k3 */},
    { NULL }
};

static const struct property_opt_test_data testdata_jpeg_props[] = {
    { wszImageQuality,         VT_R4,           VT_EMPTY },
    { wszBitmapTransform,      VT_UI1,          VT_UI1, WICBitmapTransformRotate0 },
    { wszLuminance,            VT_I4|VT_ARRAY,  VT_EMPTY },
    { wszChrominance,          VT_I4|VT_ARRAY,  VT_EMPTY },
    { wszJpegYCrCbSubsampling, VT_UI1,          VT_UI1, WICJpegYCrCbSubsamplingDefault, 0.0f, TRUE }, /* not supported on XP/2k3 */
    { wszSuppressApp0,         VT_BOOL,         VT_BOOL, FALSE },
    { NULL }
};

static int find_property_index(const WCHAR* name, PROPBAG2* all_props, int all_prop_cnt)
{
    int i;
    for (i=0; i < all_prop_cnt; i++)
    {
        if (lstrcmpW(name, all_props[i].pstrName) == 0)
            return i;
    }
    return -1;
}

static void test_specific_encoder_properties(IPropertyBag2 *options, const property_opt_test_data* data, PROPBAG2* all_props, int all_prop_cnt)
{
    HRESULT hr;
    int i = 0;
    VARIANT pvarValue;
    HRESULT phrError = S_OK;

    while (data[i].name)
    {
        int idx = find_property_index(data[i].name, all_props, all_prop_cnt);
        PROPBAG2 pb = {0};
        pb.pstrName = (LPOLESTR)data[i].name;

        hr = IPropertyBag2_Read(options, 1, &pb, NULL, &pvarValue, &phrError);

        if (data[i].skippable && idx == -1)
        {
            win_skip("Property %s is not supported on this machine.\n", wine_dbgstr_w(data[i].name));
            i++;
            continue;
        }

        ok(idx >= 0, "Property %s not in output of GetPropertyInfo\n",
           wine_dbgstr_w(data[i].name));
        if (idx >= 0)
        {
            ok(all_props[idx].vt == data[i].var_type, "Property %s has unexpected vt type, vt=%i\n",
               wine_dbgstr_w(data[i].name), all_props[idx].vt);
            ok(all_props[idx].dwType == PROPBAG2_TYPE_DATA, "Property %s has unexpected dw type, vt=%i\n",
               wine_dbgstr_w(data[i].name), all_props[idx].dwType);
            ok(all_props[idx].cfType == 0, "Property %s has unexpected cf type, vt=%i\n",
               wine_dbgstr_w(data[i].name), all_props[idx].cfType);
        }

        ok(SUCCEEDED(hr), "Reading property %s from bag failed, hr=%x\n",
           wine_dbgstr_w(data[i].name), hr);

        if (SUCCEEDED(hr))
        {
            /* On XP the initial type is always VT_EMPTY */
            ok(V_VT(&pvarValue) == data[i].initial_var_type || V_VT(&pvarValue) == VT_EMPTY,
               "Property %s has unexpected initial type, V_VT=%i\n",
               wine_dbgstr_w(data[i].name), V_VT(&pvarValue));

            if(V_VT(&pvarValue) == data[i].initial_var_type)
            {
                switch (data[i].initial_var_type)
                {
                    case VT_BOOL:
                    case VT_UI1:
                        ok(V_UNION(&pvarValue, bVal) == data[i].i_init_val, "Property %s has an unexpected initial value, pvarValue=%i\n",
                           wine_dbgstr_w(data[i].name), V_UNION(&pvarValue, bVal));
                        break;
                    case VT_R4:
                        ok(V_UNION(&pvarValue, fltVal) == data[i].f_init_val, "Property %s has an unexpected initial value, pvarValue=%f\n",
                           wine_dbgstr_w(data[i].name), V_UNION(&pvarValue, fltVal));
                        break;
                    default:
                        break;
                }
            }

            VariantClear(&pvarValue);
        }

        i++;
    }
}

static void test_encoder_properties(const CLSID* clsid_encoder, IPropertyBag2 *options)
{
    HRESULT hr;
    ULONG cProperties = 0;
    ULONG cProperties2 = 0;
    PROPBAG2 all_props[64] = {{0}}; /* Should be enough for every encoder out there */
    int i;

    /* CountProperties */
    {
        hr = IPropertyBag2_CountProperties(options, &cProperties);
        ok(SUCCEEDED(hr), "Reading property count, hr=%x\n", hr);
    }

    /* GetPropertyInfo */
    {
        hr = IPropertyBag2_GetPropertyInfo(options, cProperties, 1, all_props, &cProperties2);
        ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "IPropertyBag2::GetPropertyInfo - iProperty out of bounce handled wrong, hr=%x\n", hr);

        hr = IPropertyBag2_GetPropertyInfo(options, 0, cProperties+1, all_props, &cProperties2);
        ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "IPropertyBag2::GetPropertyInfo - cProperty out of bounce handled wrong, hr=%x\n", hr);

        if (cProperties == 0) /* GetPropertyInfo can be called for zero items on Windows 8 but not on Windows 7 (wine behaves like Win8) */
        {
            cProperties2 = cProperties;
            hr = S_OK;
        }
        else
        {
            hr = IPropertyBag2_GetPropertyInfo(options, 0, min(64, cProperties), all_props, &cProperties2);
            ok(SUCCEEDED(hr), "Reading infos from property bag failed, hr=%x\n", hr);
        }

        if (FAILED(hr))
            return;

        ok(cProperties == cProperties2, "Mismatch of property count (IPropertyBag2::CountProperties=%i, IPropertyBag2::GetPropertyInfo=%i)\n",
           (int)cProperties, (int)cProperties2);
    }

    if (IsEqualCLSID(clsid_encoder, &CLSID_WICTiffEncoder))
        test_specific_encoder_properties(options, testdata_tiff_props, all_props, cProperties2);
    else if (IsEqualCLSID(clsid_encoder, &CLSID_WICPngEncoder))
        test_specific_encoder_properties(options, testdata_png_props, all_props, cProperties2);
    else if (IsEqualCLSID(clsid_encoder, &CLSID_WICJpegEncoder))
        test_specific_encoder_properties(options, testdata_jpeg_props, all_props, cProperties2);

    for (i=0; i < cProperties2; i++)
    {
        ok(all_props[i].pstrName != NULL, "Unset property name in output of IPropertyBag2::GetPropertyInfo\n");
        CoTaskMemFree(all_props[i].pstrName);
    }
}

static void check_bmp_format(IStream *stream, const WICPixelFormatGUID *format)
{
    /* FIXME */
}

static void check_tiff_format(IStream *stream, const WICPixelFormatGUID *format)
{
    /* FIXME */
}

static unsigned be_uint(unsigned val)
{
    union
    {
        unsigned val;
        char c[4];
    } u;

    u.val = val;
    return (u.c[0] << 24) | (u.c[1] << 16) | (u.c[2] << 8) | u.c[3];
}

static void check_png_format(IStream *stream, const WICPixelFormatGUID *format)
{
    static const char png_sig[8] = {0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a};
    static const char png_IHDR[8] = {0,0,0,0x0d,'I','H','D','R'};
    HRESULT hr;
    struct
    {
        char png_sig[8];
        char ihdr_sig[8];
        unsigned width, height;
        char bit_depth, color_type, compression, filter, interlace;
    } png;

    memset(&png, 0, sizeof(png));
    hr = IStream_Read(stream, &png, sizeof(png), NULL);
    ok(hr == S_OK, "IStream_Read error %#x\n", hr);

    ok(!memcmp(png.png_sig, png_sig, sizeof(png_sig)), "expected PNG signature\n");
    ok(!memcmp(png.ihdr_sig, png_IHDR, sizeof(png_IHDR)), "expected PNG IHDR\n");

    if (IsEqualGUID(format, &GUID_WICPixelFormatBlackWhite))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 1, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 0, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat1bppIndexed))
    {
        ok(be_uint(png.width) == 32, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 1, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 3, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat8bppIndexed))
    {
        ok(be_uint(png.width) == 4, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 8, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 3, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0, "wrong interlace %d\n", png.interlace);
    }
    else if (IsEqualGUID(format, &GUID_WICPixelFormat24bppBGR))
    {
        ok(be_uint(png.width) == 4, "wrong width %u\n", be_uint(png.width));
        ok(be_uint(png.height) == 2, "wrong height %u\n", be_uint(png.height));

        ok(png.bit_depth == 8, "wrong bit_depth %d\n", png.bit_depth);
        ok(png.color_type == 2, "wrong color_type %d\n", png.color_type);
        ok(png.compression == 0, "wrong compression %d\n", png.compression);
        ok(png.filter == 0, "wrong filter %d\n", png.filter);
        ok(png.interlace == 0 || png.interlace == 1, "wrong interlace %d\n", png.interlace);
    }
    else
        ok(0, "unknown PNG pixel format %s\n", wine_dbgstr_guid(format));
}

static void check_bitmap_format(IStream *stream, const CLSID *encoder, const WICPixelFormatGUID *format)
{
    HRESULT hr;
    LARGE_INTEGER pos;

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, SEEK_SET, (ULARGE_INTEGER *)&pos);
    ok(hr == S_OK, "IStream_Seek error %#x\n", hr);

    if (IsEqualGUID(encoder, &CLSID_WICPngEncoder))
        check_png_format(stream, format);
    else if (IsEqualGUID(encoder, &CLSID_WICBmpEncoder))
        check_bmp_format(stream, format);
    else if (IsEqualGUID(encoder, &CLSID_WICTiffEncoder))
        check_tiff_format(stream, format);
    else
        ok(0, "unknown encoder %s\n", wine_dbgstr_guid(encoder));

    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#x\n", hr);
}

struct setting {
    const WCHAR *name;
    PROPBAG2_TYPE type;
    VARTYPE vt;
    void *value;
};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static void test_set_frame_palette(IWICBitmapFrameEncode *frameencode)
{
    IWICComponentFactory *factory;
    IWICPalette *palette;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

    hr = IWICBitmapFrameEncode_SetPalette(frameencode, NULL);
    ok(hr == E_INVALIDARG, "SetPalette failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette failed, hr=%x\n", hr);

    hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
todo_wine
    ok(hr == WINCODEC_ERR_NOTINITIALIZED, "Unexpected hr=%x\n", hr);

    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeFixedHalftone256, FALSE);
    ok(hr == S_OK, "InitializePredefined failed, hr=%x\n", hr);

    EXPECT_REF(palette, 1);
    hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
    ok(hr == S_OK, "SetPalette failed, hr=%x\n", hr);
    EXPECT_REF(palette, 1);

    hr = IWICBitmapFrameEncode_SetPalette(frameencode, NULL);
    ok(hr == E_INVALIDARG, "SetPalette failed, hr=%x\n", hr);

    IWICPalette_Release(palette);
    IWICComponentFactory_Release(factory);
}

static void test_multi_encoder(const struct bitmap_data **srcs, const CLSID* clsid_encoder,
    const struct bitmap_data **dsts, const CLSID *clsid_decoder, WICRect *rc,
    const struct setting *settings, const char *name, IWICPalette *palette)
{
    HRESULT hr;
    IWICBitmapEncoder *encoder;
    BitmapTestSrc *src_obj;
    HGLOBAL hglobal;
    IStream *stream;
    IWICBitmapFrameEncode *frameencode;
    IPropertyBag2 *options=NULL;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *framedecode;
    WICPixelFormatGUID pixelformat;
    int i;

    hr = CoCreateInstance(clsid_encoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void**)&encoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        hglobal = GlobalAlloc(GMEM_MOVEABLE, 0);
        ok(hglobal != NULL, "GlobalAlloc failed\n");
        if (hglobal)
        {
            hr = CreateStreamOnHGlobal(hglobal, TRUE, &stream);
            ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%x\n", hr);
        }

        if (hglobal && SUCCEEDED(hr))
        {
            if (palette)
            {
                hr = IWICBitmapEncoder_SetPalette(encoder, palette);
                ok(hr == WINCODEC_ERR_NOTINITIALIZED, "wrong error %#x (%s)\n", hr, name);
            }

            hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
            ok(SUCCEEDED(hr), "Initialize failed, hr=%x\n", hr);

            if (palette)
            {
                hr = IWICBitmapEncoder_SetPalette(encoder, palette);
                ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "wrong error %#x\n", hr);
                hr = S_OK;
            }

            i=0;
            while (SUCCEEDED(hr) && srcs[i])
            {
                CreateTestBitmap(srcs[i], &src_obj);

                hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frameencode, &options);
                ok(SUCCEEDED(hr), "CreateFrame failed, hr=%x\n", hr);
                if (SUCCEEDED(hr))
                {
                    ok(options != NULL, "Encoder initialization has not created an property bag\n");
                    if(options)
                        test_encoder_properties(clsid_encoder, options);

                    if (settings)
                    {
                        int j;
                        for (j=0; settings[j].name; j++)
                        {
                            PROPBAG2 propbag;
                            VARIANT var;

                            memset(&propbag, 0, sizeof(propbag));
                            memset(&var, 0, sizeof(var));
                            propbag.pstrName = (LPOLESTR)settings[j].name;
                            propbag.dwType = settings[j].type;
                            V_VT(&var) = settings[j].vt;
                            V_UNKNOWN(&var) = settings[j].value;

                            hr = IPropertyBag2_Write(options, 1, &propbag, &var);
                            ok(SUCCEEDED(hr), "Writing property %s failed, hr=%x\n", wine_dbgstr_w(settings[j].name), hr);
                        }
                    }

                    if (palette)
                    {
                        hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
                        ok(hr == WINCODEC_ERR_NOTINITIALIZED, "wrong error %#x\n", hr);
                    }

                    hr = IWICBitmapFrameEncode_Initialize(frameencode, options);
                    ok(SUCCEEDED(hr), "Initialize failed, hr=%x\n", hr);

                    memcpy(&pixelformat, srcs[i]->format, sizeof(GUID));
                    hr = IWICBitmapFrameEncode_SetPixelFormat(frameencode, &pixelformat);
                    ok(SUCCEEDED(hr), "SetPixelFormat failed, hr=%x\n", hr);
                    ok(IsEqualGUID(&pixelformat, dsts[i]->format), "SetPixelFormat changed the format to %s (%s)\n",
                       wine_dbgstr_guid(&pixelformat), name);

                    hr = IWICBitmapFrameEncode_SetSize(frameencode, srcs[i]->width, srcs[i]->height);
                    ok(SUCCEEDED(hr), "SetSize failed, hr=%x\n", hr);

                    if (IsEqualGUID(clsid_encoder, &CLSID_WICPngEncoder))
                        test_set_frame_palette(frameencode);

                    if (palette)
                    {
                        WICColor colors[256];

                        hr = IWICBitmapFrameEncode_SetPalette(frameencode, palette);
                        ok(SUCCEEDED(hr), "SetPalette failed, hr=%x (%s)\n", hr, name);

                        /* trash the assigned palette */
                        memset(colors, 0, sizeof(colors));
                        hr = IWICPalette_InitializeCustom(palette, colors, 256);
                        ok(hr == S_OK, "InitializeCustom error %#x\n", hr);
                    }

                    hr = IWICBitmapFrameEncode_WriteSource(frameencode, &src_obj->IWICBitmapSource_iface, rc);
                    if (rc && (rc->Width <= 0 || rc->Height <= 0))
                    {
                        /* WriteSource fails but WriteSource_Proxy succeeds. */
                        ok(hr == E_INVALIDARG, "WriteSource should fail, hr=%x (%s)\n", hr, name);
                        hr = IWICBitmapFrameEncode_WriteSource_Proxy(frameencode, &src_obj->IWICBitmapSource_iface, rc);
                        ok(SUCCEEDED(hr), "WriteSource_Proxy failed, %dx%d, hr=%x (%s)\n", rc->Width, rc->Height, hr, name);
                    }
                    else
                    {
                        if (rc)
                            ok(SUCCEEDED(hr), "WriteSource(%dx%d) failed, hr=%x (%s)\n", rc->Width, rc->Height, hr, name);
                        else
                            ok(hr == S_OK ||
                               broken(hr == E_INVALIDARG && IsEqualGUID(clsid_encoder, &CLSID_WICBmpEncoder) && IsEqualGUID(srcs[i]->format, &GUID_WICPixelFormatBlackWhite)) /* XP */,
                               "WriteSource(NULL) failed, hr=%x (%s)\n", hr, name);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapFrameEncode_Commit(frameencode);
                        ok(SUCCEEDED(hr), "Commit failed, hr=%x (%s)\n", hr, name);
                    }

                    IWICBitmapFrameEncode_Release(frameencode);
                    IPropertyBag2_Release(options);
                }

                DeleteTestBitmap(src_obj);

                i++;
            }

            if (clsid_decoder == NULL)
            {
                IStream_Release(stream);
                IWICBitmapEncoder_Release(encoder);
                return;
            }

            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapEncoder_Commit(encoder);
                ok(SUCCEEDED(hr), "Commit failed, hr=%x\n", hr);

                check_bitmap_format(stream, clsid_encoder, dsts[0]->format);
            }

            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(clsid_decoder, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICBitmapDecoder, (void**)&decoder);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
            }

            if (SUCCEEDED(hr))
            {
                IWICPalette *frame_palette;

                hr = IWICImagingFactory_CreatePalette(factory, &frame_palette);
                ok(hr == S_OK, "CreatePalette error %#x\n", hr);

                hr = IWICBitmapDecoder_CopyPalette(decoder, frame_palette);
                ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "wrong error %#x\n", hr);

                hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand);
                ok(SUCCEEDED(hr), "Initialize failed, hr=%x\n", hr);

                hr = IWICBitmapDecoder_CopyPalette(decoder, frame_palette);
                ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "wrong error %#x\n", hr);

                hr = S_OK;
                i=0;
                while (SUCCEEDED(hr) && dsts[i])
                {
                    hr = IWICBitmapDecoder_GetFrame(decoder, i, &framedecode);
                    ok(SUCCEEDED(hr), "GetFrame failed, hr=%x (%s)\n", hr, name);

                    if (SUCCEEDED(hr))
                    {
                        compare_bitmap_data(dsts[i], (IWICBitmapSource*)framedecode, name);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, frame_palette);
                        if (winetest_debug > 1)
                            trace("%s, bpp %d, %s, hr %#x\n", name, dsts[i]->bpp, wine_dbgstr_guid(dsts[i]->format), hr);
                        if (dsts[i]->bpp > 8 || IsEqualGUID(dsts[i]->format, &GUID_WICPixelFormatBlackWhite))
                            ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "wrong error %#x\n", hr);
                        else
                        {
                            UINT count, ret;
                            WICColor colors[256];

                            ok(hr == S_OK, "CopyPalette error %#x (%s)\n", hr, name);

                            count = 0;
                            hr = IWICPalette_GetColorCount(frame_palette, &count);
                            ok(hr == S_OK, "GetColorCount error %#x\n", hr);

                            memset(colors, 0, sizeof(colors));
                            ret = 0;
                            hr = IWICPalette_GetColors(frame_palette, count, colors, &ret);
                            ok(hr == S_OK, "GetColors error %#x\n", hr);
                            ok(ret == count, "expected %u, got %u\n", count, ret);
                            if (IsEqualGUID(clsid_decoder, &CLSID_WICPngDecoder))
                            {
                                ok(count == 256 || count == 2 /* newer libpng versions */, "expected 256, got %u (%s)\n", count, name);

                                ok(colors[0] == 0x11111111, "got %08x (%s)\n", colors[0], name);
                                ok(colors[1] == 0x22222222, "got %08x (%s)\n", colors[1], name);
                                if (count > 2)
                                {
                                    ok(colors[2] == 0x33333333, "got %08x (%s)\n", colors[2], name);
                                    ok(colors[3] == 0x44444444, "got %08x (%s)\n", colors[3], name);
                                    ok(colors[4] == 0x55555555, "got %08x (%s)\n", colors[4], name);
                                    ok(colors[5] == 0, "got %08x (%s)\n", colors[5], name);
                                }
                            }
                            else if (IsEqualGUID(clsid_decoder, &CLSID_WICBmpDecoder) ||
                                     IsEqualGUID(clsid_decoder, &CLSID_WICTiffDecoder))
                            {
                                if (IsEqualGUID(dsts[i]->format, &GUID_WICPixelFormatBlackWhite) ||
                                    IsEqualGUID(dsts[i]->format, &GUID_WICPixelFormat8bppIndexed))
                                {
                                    ok(count == 256, "expected 256, got %u (%s)\n", count, name);

                                    ok(colors[0] == 0xff111111, "got %08x (%s)\n", colors[0], name);
                                    ok(colors[1] == 0xff222222, "got %08x (%s)\n", colors[1], name);
                                    ok(colors[2] == 0xff333333, "got %08x (%s)\n", colors[2], name);
                                    ok(colors[3] == 0xff444444, "got %08x (%s)\n", colors[3], name);
                                    ok(colors[4] == 0xff555555, "got %08x (%s)\n", colors[4], name);
                                    ok(colors[5] == 0xff000000, "got %08x (%s)\n", colors[5], name);
                                }
                                else
                                {
                                    ok(count == 2, "expected 2, got %u (%s)\n", count, name);

                                    ok(colors[0] == 0xff111111, "got %08x (%s)\n", colors[0], name);
                                    ok(colors[1] == 0xff222222, "got %08x (%s)\n", colors[1], name);
                                }
                            }
                            else
                            {
                                ok(count == 2, "expected 2, got %u (%s)\n", count, name);

                                ok(colors[0] == 0xff111111, "got %08x\n", colors[0]);
                                ok(colors[1] == 0xff222222, "got %08x\n", colors[1]);
                            }
                        }

                        IWICBitmapFrameDecode_Release(framedecode);
                    }

                    i++;
                }

                IWICPalette_Release(frame_palette);
                IWICBitmapDecoder_Release(decoder);
            }

            IStream_Release(stream);
        }

        IWICBitmapEncoder_Release(encoder);
    }
}

static void test_encoder(const struct bitmap_data *src, const CLSID* clsid_encoder,
    const struct bitmap_data *dst, const CLSID *clsid_decoder, const char *name)
{
    const struct bitmap_data *srcs[2];
    const struct bitmap_data *dsts[2];
    WICColor colors[256];
    IWICPalette *palette;
    HRESULT hr;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    memset(colors, 0, sizeof(colors));
    colors[0] = 0x11111111;
    colors[1] = 0x22222222;
    colors[2] = 0x33333333;
    colors[3] = 0x44444444;
    colors[4] = 0x55555555;
    /* TIFF decoder fails to decode a 8bpp frame if palette has less than 256 colors */
    hr = IWICPalette_InitializeCustom(palette, colors, 256);
    ok(hr == S_OK, "InitializeCustom error %#x\n", hr);

    srcs[0] = src;
    srcs[1] = NULL;
    dsts[0] = dst;
    dsts[1] = NULL;

    test_multi_encoder(srcs, clsid_encoder, dsts, clsid_decoder, NULL, NULL, name, palette);

    IWICPalette_Release(palette);
}

static void test_encoder_rects(void)
{
    const struct bitmap_data *srcs[2];
    const struct bitmap_data *dsts[2];
    WICRect rc;

    srcs[0] = &testdata_24bppBGR;
    srcs[1] = NULL;
    dsts[0] = &testdata_24bppBGR;
    dsts[1] = NULL;

    rc.X = 0;
    rc.Y = 0;
    rc.Width = 4;
    rc.Height = 2;

    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects full", NULL);

    rc.Width = 0;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects width=0", NULL);

    rc.Width = -1;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects width=-1", NULL);

    rc.Width = 4;
    rc.Height = 0;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects height=0", NULL);

    rc.Height = -1;
    test_multi_encoder(srcs, &CLSID_WICTiffEncoder, dsts, &CLSID_WICTiffDecoder, &rc, NULL, "test_encoder_rects height=-1", NULL);
}

static const struct bitmap_data *multiple_frames[3] = {
    &testdata_24bppBGR,
    &testdata_24bppBGR,
    NULL};

static const struct bitmap_data *single_frame[2] = {
    &testdata_24bppBGR,
    NULL};

static const struct setting png_interlace_settings[] = {
    {wszInterlaceOption, PROPBAG2_TYPE_DATA, VT_BOOL, (void*)VARIANT_TRUE},
    {NULL}
};

START_TEST(converter)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "failed to create factory: %#x\n", hr);

    test_conversion(&testdata_32bppBGRA, &testdata_32bppBGR, "BGRA -> BGR", FALSE);
    test_conversion(&testdata_32bppBGR, &testdata_32bppBGRA, "BGR -> BGRA", FALSE);
    test_conversion(&testdata_32bppBGRA, &testdata_32bppBGRA, "BGRA -> BGRA", FALSE);

    test_conversion(&testdata_24bppBGR, &testdata_24bppBGR, "24bppBGR -> 24bppBGR", FALSE);
    test_conversion(&testdata_24bppBGR, &testdata_24bppRGB, "24bppBGR -> 24bppRGB", FALSE);

    test_conversion(&testdata_24bppRGB, &testdata_24bppRGB, "24bppRGB -> 24bppRGB", FALSE);
    test_conversion(&testdata_24bppRGB, &testdata_24bppBGR, "24bppRGB -> 24bppBGR", FALSE);

    test_conversion(&testdata_32bppBGR, &testdata_24bppRGB, "32bppBGR -> 24bppRGB", FALSE);
    test_conversion(&testdata_24bppRGB, &testdata_32bppBGR, "24bppRGB -> 32bppBGR", FALSE);
    test_conversion(&testdata_32bppBGRA, &testdata_24bppRGB, "32bppBGRA -> 24bppRGB", FALSE);

    test_conversion(&testdata_24bppRGB, &testdata_32bppGrayFloat, "24bppRGB -> 32bppGrayFloat", FALSE);
    test_conversion(&testdata_32bppBGR, &testdata_32bppGrayFloat, "32bppBGR -> 32bppGrayFloat", FALSE);

    test_conversion(&testdata_24bppBGR, &testdata_8bppGray, "24bppBGR -> 8bppGray", FALSE);
    test_conversion(&testdata_32bppBGR, &testdata_8bppGray, "32bppBGR -> 8bppGray", FALSE);
    test_conversion(&testdata_32bppGrayFloat, &testdata_24bppBGR_gray, "32bppGrayFloat -> 24bppBGR gray", FALSE);
    test_conversion(&testdata_32bppGrayFloat, &testdata_8bppGray, "32bppGrayFloat -> 8bppGray", FALSE);

    test_invalid_conversion();
    test_default_converter();

    test_encoder(&testdata_BlackWhite, &CLSID_WICPngEncoder,
                 &testdata_BlackWhite, &CLSID_WICPngDecoder, "PNG encoder BlackWhite");
    test_encoder(&testdata_1bppIndexed, &CLSID_WICPngEncoder,
                 &testdata_1bppIndexed, &CLSID_WICPngDecoder, "PNG encoder 1bppIndexed");
    test_encoder(&testdata_8bppIndexed, &CLSID_WICPngEncoder,
                 &testdata_8bppIndexed, &CLSID_WICPngDecoder, "PNG encoder 8bppIndexed");
    test_encoder(&testdata_24bppBGR, &CLSID_WICPngEncoder,
                 &testdata_24bppBGR, &CLSID_WICPngDecoder, "PNG encoder 24bppBGR");

if (!strcmp(winetest_platform, "windows")) /* FIXME: enable once implemented in Wine */
{
    test_encoder(&testdata_BlackWhite, &CLSID_WICBmpEncoder,
                 &testdata_1bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder BlackWhite");
    test_encoder(&testdata_1bppIndexed, &CLSID_WICBmpEncoder,
                 &testdata_1bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder 1bppIndexed");
    test_encoder(&testdata_8bppIndexed, &CLSID_WICBmpEncoder,
                 &testdata_8bppIndexed, &CLSID_WICBmpDecoder, "BMP encoder 8bppIndexed");
}
    test_encoder(&testdata_32bppBGR, &CLSID_WICBmpEncoder,
                 &testdata_32bppBGR, &CLSID_WICBmpDecoder, "BMP encoder 32bppBGR");

    test_encoder(&testdata_BlackWhite, &CLSID_WICTiffEncoder,
                 &testdata_BlackWhite, &CLSID_WICTiffDecoder, "TIFF encoder BlackWhite");
if (!strcmp(winetest_platform, "windows")) /* FIXME: enable once implemented in Wine */
{
    test_encoder(&testdata_1bppIndexed, &CLSID_WICTiffEncoder,
                 &testdata_1bppIndexed, &CLSID_WICTiffDecoder, "TIFF encoder 1bppIndexed");
    test_encoder(&testdata_8bppIndexed, &CLSID_WICTiffEncoder,
                 &testdata_8bppIndexed, &CLSID_WICTiffDecoder, "TIFF encoder 8bppIndexed");
}
    test_encoder(&testdata_24bppBGR, &CLSID_WICTiffEncoder,
                 &testdata_24bppBGR, &CLSID_WICTiffDecoder, "TIFF encoder 24bppBGR");

    test_encoder(&testdata_24bppBGR, &CLSID_WICJpegEncoder,
                 &testdata_24bppBGR, NULL, "JPEG encoder 24bppBGR");

    test_multi_encoder(multiple_frames, &CLSID_WICTiffEncoder,
                       multiple_frames, &CLSID_WICTiffDecoder, NULL, NULL, "TIFF encoder multi-frame", NULL);

    test_encoder_rects();

    test_multi_encoder(single_frame, &CLSID_WICPngEncoder,
                       single_frame, &CLSID_WICPngDecoder, NULL, png_interlace_settings, "PNG encoder interlaced", NULL);

    IWICImagingFactory_Release(factory);

    CoUninitialize();
}
