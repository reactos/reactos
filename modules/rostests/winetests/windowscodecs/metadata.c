/*
 * Copyright 2011 Vincent Povirk for CodeWeavers
 * Copyright 2012,2017 Dmitry Timoshkov
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

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#define COBJMACROS
#ifdef __REACTOS__
#define CONST_VTABLE
#endif

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "propvarutil.h"
#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(IID_MdbrUnknown, 0x00240e6f,0x3f23,0x4432,0xb0,0xcc,0x48,0xd5,0xbb,0xff,0x6c,0x36);

#define expect_blob(propvar, data, length) do { \
    ok((propvar).vt == VT_BLOB, "unexpected vt: %i\n", (propvar).vt); \
    if ((propvar).vt == VT_BLOB) { \
        ok(U(propvar).blob.cbSize == (length), "expected size %u, got %u\n", (ULONG)(length), U(propvar).blob.cbSize); \
        if (U(propvar).blob.cbSize == (length)) { \
            ok(!memcmp(U(propvar).blob.pBlobData, (data), (length)), "unexpected data\n"); \
        } \
    } \
} while (0)

#define IFD_BYTE 1
#define IFD_ASCII 2
#define IFD_SHORT 3
#define IFD_LONG 4
#define IFD_RATIONAL 5
#define IFD_SBYTE 6
#define IFD_UNDEFINED 7
#define IFD_SSHORT 8
#define IFD_SLONG 9
#define IFD_SRATIONAL 10
#define IFD_FLOAT 11
#define IFD_DOUBLE 12
#define IFD_IFD 13

#include "pshpack2.h"
struct IFD_entry
{
    SHORT id;
    SHORT type;
    ULONG count;
    LONG  value;
};

struct IFD_rational
{
    LONG numerator;
    LONG denominator;
};

static const struct ifd_data
{
    USHORT number_of_entries;
    struct IFD_entry entry[40];
    ULONG next_IFD;
    struct IFD_rational xres;
    DOUBLE double_val;
    struct IFD_rational srational_val;
    char string[14];
    SHORT short_val[4];
    LONG long_val[2];
    FLOAT float_val[2];
    struct IFD_rational rational[3];
} IFD_data =
{
    28,
    {
        { 0xfe,  IFD_SHORT, 1, 1 }, /* NEWSUBFILETYPE */
        { 0x100, IFD_LONG, 1, 222 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 333 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 24 }, /* BITSPERSAMPLE */
        { 0x103, IFD_LONG, 1, 32773 }, /* COMPRESSION: packbits */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct ifd_data, xres) },
        { 0xf001, IFD_BYTE, 1, 0x11223344 },
        { 0xf002, IFD_BYTE, 4, 0x11223344 },
        { 0xf003, IFD_SBYTE, 1, 0x11223344 },
        { 0xf004, IFD_SSHORT, 1, 0x11223344 },
        { 0xf005, IFD_SSHORT, 2, 0x11223344 },
        { 0xf006, IFD_SLONG, 1, 0x11223344 },
        { 0xf007, IFD_FLOAT, 1, 0x11223344 },
        { 0xf008, IFD_DOUBLE, 1, FIELD_OFFSET(struct ifd_data, double_val) },
        { 0xf009, IFD_SRATIONAL, 1, FIELD_OFFSET(struct ifd_data, srational_val) },
        { 0xf00a, IFD_BYTE, 13, FIELD_OFFSET(struct ifd_data, string) },
        { 0xf00b, IFD_SSHORT, 4, FIELD_OFFSET(struct ifd_data, short_val) },
        { 0xf00c, IFD_SLONG, 2, FIELD_OFFSET(struct ifd_data, long_val) },
        { 0xf00d, IFD_FLOAT, 2, FIELD_OFFSET(struct ifd_data, float_val) },
        { 0xf00e, IFD_ASCII, 13, FIELD_OFFSET(struct ifd_data, string) },
        { 0xf00f, IFD_ASCII, 4, 'a' | 'b' << 8 | 'c' << 16 | 'd' << 24 },
        { 0xf010, IFD_UNDEFINED, 13, FIELD_OFFSET(struct ifd_data, string) },
        { 0xf011, IFD_UNDEFINED, 4, 'a' | 'b' << 8 | 'c' << 16 | 'd' << 24 },
        { 0xf012, IFD_BYTE, 0, 0x11223344 },
        { 0xf013, IFD_SHORT, 0, 0x11223344 },
        { 0xf014, IFD_LONG, 0, 0x11223344 },
        { 0xf015, IFD_FLOAT, 0, 0x11223344 },
        { 0xf016, IFD_SRATIONAL, 3, FIELD_OFFSET(struct ifd_data, rational) },
    },
    0,
    { 900, 3 },
    1234567890.0987654321,
    { 0x1a2b3c4d, 0x5a6b7c8d },
    "Hello World!",
    { 0x0101, 0x0202, 0x0303, 0x0404 },
    { 0x11223344, 0x55667788 },
    { (FLOAT)1234.5678, (FLOAT)8765.4321 },
    { { 0x01020304, 0x05060708 }, { 0x10203040, 0x50607080 }, { 0x11223344, 0x55667788 } },
};
#include "poppack.h"

static const char metadata_unknown[] = "lalala";

static const char metadata_tEXt[] = {
    0,0,0,14, /* chunk length */
    't','E','X','t', /* chunk type */
    'w','i','n','e','t','e','s','t',0, /* keyword */
    'v','a','l','u','e', /* text */
    0x3f,0x64,0x19,0xf3 /* chunk CRC */
};

static const char metadata_gAMA[] = {
    0,0,0,4, /* chunk length */
    'g','A','M','A', /* chunk type */
    0,0,130,53, /* gamma */
    0xff,0xff,0xff,0xff /* chunk CRC */
};

static const char metadata_cHRM[] = {
    0,0,0,32, /* chunk length */
    'c','H','R','M', /* chunk type */
    0,0,122,38, 0,0,128,132, /* white point */
    0,0,250,0, 0,0,128,232, /* red */
    0,0,117,48, 0,0,234,96, /* green */
    0,0,58,152, 0,0,23,112, /* blue */
    0xff,0xff,0xff,0xff /* chunk CRC */
};

static const char pngimage[285] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
0xde,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,
0x13,0x01,0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd5,
0x06,0x03,0x0f,0x07,0x2d,0x12,0x10,0xf0,0xfd,0x00,0x00,0x00,0x0c,0x49,0x44,0x41,
0x54,0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,
0xe7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

/* 1x1 pixel gif */
static const char gifimage[35] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
0x01,0x00,0x3b
};

/* 1x1 pixel gif, 2 frames; first frame is white, second is black */
static const char animatedgif[] = {
'G','I','F','8','9','a',0x01,0x00,0x01,0x00,0xA1,0x00,0x00,
0x6F,0x6F,0x6F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0x21,0xFF,0x0B,'N','E','T','S','C','A','P','E','2','.','0',*/
0x21,0xFF,0x0B,'A','N','I','M','E','X','T','S','1','.','0',
0x03,0x01,0x05,0x00,0x00,
0x21,0xFE,0x0C,'H','e','l','l','o',' ','W','o','r','l','d','!',0x00,
0x21,0x01,0x0D,'a','n','i','m','a','t','i','o','n','.','g','i','f',0x00,
0x21,0xF9,0x04,0x00,0x0A,0x00,0xFF,0x00,0x2C,
0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
0xDE,0xDE,0xDE,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x4C,0x01,0x00,
0x21,0xFE,0x08,'i','m','a','g','e',' ','#','1',0x00,
0x21,0x01,0x0C,'p','l','a','i','n','t','e','x','t',' ','#','1',0x00,
0x21,0xF9,0x04,0x01,0x0A,0x00,0x01,0x00,0x2C,
0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
0x4D,0x4D,0x4D,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x44,0x01,0x00,
0x21,0xFE,0x08,'i','m','a','g','e',' ','#','2',0x00,
0x21,0x01,0x0C,'p','l','a','i','n','t','e','x','t',' ','#','2',0x00,0x3B
};

static IStream *create_stream(const char *data, int data_size)
{
    HRESULT hr;
    IStream *stream;
    HGLOBAL hdata;
    void *locked_data;

    hdata = GlobalAlloc(GMEM_MOVEABLE, data_size);
    ok(hdata != 0, "GlobalAlloc failed\n");
    if (!hdata) return NULL;

    locked_data = GlobalLock(hdata);
    memcpy(locked_data, data, data_size);
    GlobalUnlock(hdata);

    hr = CreateStreamOnHGlobal(hdata, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed, hr=%x\n", hr);

    return stream;
}

static void load_stream(IUnknown *reader, const char *data, int data_size, DWORD persist_options)
{
    HRESULT hr;
    IWICPersistStream *persist;
    IStream *stream;
    LARGE_INTEGER pos;
    ULARGE_INTEGER cur_pos;

    stream = create_stream(data, data_size);
    if (!stream)
        return;

    hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void**)&persist);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICPersistStream_LoadEx(persist, stream, NULL, persist_options);
        ok(hr == S_OK, "LoadEx failed, hr=%x\n", hr);

        IWICPersistStream_Release(persist);
    }

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, SEEK_CUR, &cur_pos);
    ok(hr == S_OK, "IStream_Seek error %#x\n", hr);
    /* IFD metadata reader doesn't rewind the stream to the start */
    ok(cur_pos.QuadPart == 0 || cur_pos.QuadPart <= data_size,
       "current stream pos is at %x/%x, data size %x\n", cur_pos.u.LowPart, cur_pos.u.HighPart, data_size);

    IStream_Release(stream);
}

static void test_metadata_unknown(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    IWICMetadataBlockReader *blockreader;
    PROPVARIANT schema, id, value;
    ULONG items_returned;

    hr = CoCreateInstance(&CLSID_WICUnknownMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    load_stream((IUnknown*)reader, metadata_unknown, sizeof(metadata_unknown), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        PropVariantInit(&schema);
        PropVariantInit(&id);
        PropVariantInit(&value);

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next failed, hr=%x\n", hr);
        ok(items_returned == 1, "unexpected item count %i\n", items_returned);

        if (hr == S_OK && items_returned == 1)
        {
            ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
            ok(id.vt == VT_EMPTY, "unexpected vt: %i\n", id.vt);
            expect_blob(value, metadata_unknown, sizeof(metadata_unknown));

            PropVariantClear(&schema);
            PropVariantClear(&id);
            PropVariantClear(&value);
        }

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_FALSE, "Next failed, hr=%x\n", hr);
        ok(items_returned == 0, "unexpected item count %i\n", items_returned);

        IWICEnumMetadataItem_Release(enumerator);
    }

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
        IWICMetadataBlockReader_Release(blockreader);

    IWICMetadataReader_Release(reader);
}

static void test_metadata_tEXt(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    ULONG items_returned, count;
    GUID format;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngTextMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 0, "unexpected count %i\n", count);

    load_stream((IUnknown*)reader, metadata_tEXt, sizeof(metadata_tEXt), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 1, "unexpected count %i\n", count);

    hr = IWICMetadataReader_GetEnumerator(reader, NULL);
    ok(hr == E_INVALIDARG, "GetEnumerator failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next failed, hr=%x\n", hr);
        ok(items_returned == 1, "unexpected item count %i\n", items_returned);

        if (hr == S_OK && items_returned == 1)
        {
            ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
            ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
            ok(!strcmp(U(id).pszVal, "winetest"), "unexpected id: %s\n", U(id).pszVal);
            ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", value.vt);
            ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(value).pszVal);

            PropVariantClear(&schema);
            PropVariantClear(&id);
            PropVariantClear(&value);
        }

        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_FALSE, "Next failed, hr=%x\n", hr);
        ok(items_returned == 0, "unexpected item count %i\n", items_returned);

        IWICEnumMetadataItem_Release(enumerator);
    }

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataFormat(reader, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataFormat failed, hr=%x\n", hr);

    id.vt = VT_LPSTR;
    U(id).pszVal = CoTaskMemAlloc(strlen("winetest") + 1);
    strcpy(U(id).pszVal, "winetest");

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, &value);
    ok(hr == E_INVALIDARG, "GetValue failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(value).pszVal);
    PropVariantClear(&value);

    strcpy(U(id).pszVal, "test");

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "GetValue failed, hr=%x\n", hr);

    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, NULL);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);
    ok(id.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(id).pszVal, "winetest"), "unexpected id: %s\n", U(id).pszVal);
    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    ok(hr == S_OK, "GetValueByIndex failed, hr=%x\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", value.vt);
    ok(!strcmp(U(value).pszVal, "value"), "unexpected value: %s\n", U(value).pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, 1, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetValueByIndex failed, hr=%x\n", hr);

    IWICMetadataReader_Release(reader);
}

static void test_metadata_gAMA(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT schema, id, value;
    ULONG count;
    GUID format;
    static const WCHAR ImageGamma[] = {'I','m','a','g','e','G','a','m','m','a',0};

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngGamaMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /*winxp*/, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    load_stream((IUnknown*)reader, metadata_gAMA, sizeof(metadata_gAMA), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunkgAMA), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 1, "unexpected count %i\n", count);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, &id, &value);
    ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);

    ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
    PropVariantClear(&schema);

    ok(id.vt == VT_LPWSTR, "unexpected vt: %i\n", id.vt);
    ok(!lstrcmpW(U(id).pwszVal, ImageGamma), "unexpected value: %s\n", wine_dbgstr_w(U(id).pwszVal));
    PropVariantClear(&id);

    ok(value.vt == VT_UI4, "unexpected vt: %i\n", value.vt);
    ok(U(value).ulVal == 33333, "unexpected value: %u\n", U(value).ulVal);
    PropVariantClear(&value);

    IWICMetadataReader_Release(reader);
}

static void test_metadata_cHRM(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    PROPVARIANT schema, id, value;
    ULONG count;
    GUID format;
    int i;
    static const WCHAR expected_names[8][12] = {
        {'W','h','i','t','e','P','o','i','n','t','X',0},
        {'W','h','i','t','e','P','o','i','n','t','Y',0},
        {'R','e','d','X',0},
        {'R','e','d','Y',0},
        {'G','r','e','e','n','X',0},
        {'G','r','e','e','n','Y',0},
        {'B','l','u','e','X',0},
        {'B','l','u','e','Y',0},
    };
    static const ULONG expected_vals[8] = {
        31270,32900, 64000,33000, 30000,60000, 15000,6000
    };

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = CoCreateInstance(&CLSID_WICPngChrmMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG) /*winxp*/, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    load_stream((IUnknown*)reader, metadata_cHRM, sizeof(metadata_cHRM), WICPersistOptionDefault);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatChunkcHRM), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
    ok(count == 8, "unexpected count %i\n", count);

    for (i=0; i<8; i++)
    {
        hr = IWICMetadataReader_GetValueByIndex(reader, i, &schema, &id, &value);
        ok(hr == S_OK, "GetValue failed, hr=%x\n", hr);

        ok(schema.vt == VT_EMPTY, "unexpected vt: %i\n", schema.vt);
        PropVariantClear(&schema);

        ok(id.vt == VT_LPWSTR, "unexpected vt: %i\n", id.vt);
        ok(!lstrcmpW(U(id).pwszVal, expected_names[i]), "got %s, expected %s\n", wine_dbgstr_w(U(id).pwszVal), wine_dbgstr_w(expected_names[i]));
        PropVariantClear(&id);

        ok(value.vt == VT_UI4, "unexpected vt: %i\n", value.vt);
        ok(U(value).ulVal == expected_vals[i], "got %u, expected %u\n", U(value).ulVal, expected_vals[i]);
        PropVariantClear(&value);
    }

    IWICMetadataReader_Release(reader);
}

static inline USHORT ushort_bswap(USHORT s)
{
    return (s >> 8) | (s << 8);
}

static inline ULONG ulong_bswap(ULONG l)
{
    return ((ULONG)ushort_bswap((USHORT)l) << 16) | ushort_bswap((USHORT)(l >> 16));
}

static inline ULONGLONG ulonglong_bswap(ULONGLONG ll)
{
    return ((ULONGLONG)ulong_bswap((ULONG)ll) << 32) | ulong_bswap((ULONG)(ll >> 32));
}

static void byte_swap_ifd_data(char *data)
{
    USHORT number_of_entries, i;
    struct IFD_entry *entry;
    char *data_start = data;

    number_of_entries = *(USHORT *)data;
    *(USHORT *)data = ushort_bswap(*(USHORT *)data);
    data += sizeof(USHORT);

    for (i = 0; i < number_of_entries; i++)
    {
        entry = (struct IFD_entry *)data;

        switch (entry->type)
        {
        case IFD_BYTE:
        case IFD_SBYTE:
        case IFD_ASCII:
        case IFD_UNDEFINED:
            if (entry->count > 4)
                entry->value = ulong_bswap(entry->value);
            break;

        case IFD_SHORT:
        case IFD_SSHORT:
            if (entry->count > 2)
            {
                ULONG j, count = entry->count;
                USHORT *us = (USHORT *)(data_start + entry->value);
                if (!count) count = 1;
                for (j = 0; j < count; j++)
                    us[j] = ushort_bswap(us[j]);

                entry->value = ulong_bswap(entry->value);
            }
            else
            {
                ULONG j, count = entry->count;
                USHORT *us = (USHORT *)&entry->value;
                if (!count) count = 1;
                for (j = 0; j < count; j++)
                    us[j] = ushort_bswap(us[j]);
            }
            break;

        case IFD_LONG:
        case IFD_SLONG:
        case IFD_FLOAT:
            if (entry->count > 1)
            {
                ULONG j, count = entry->count;
                ULONG *ul = (ULONG *)(data_start + entry->value);
                if (!count) count = 1;
                for (j = 0; j < count; j++)
                    ul[j] = ulong_bswap(ul[j]);
            }
            entry->value = ulong_bswap(entry->value);
            break;

        case IFD_RATIONAL:
        case IFD_SRATIONAL:
            {
                ULONG j;
                ULONG *ul = (ULONG *)(data_start + entry->value);
                for (j = 0; j < entry->count * 2; j++)
                    ul[j] = ulong_bswap(ul[j]);
            }
            entry->value = ulong_bswap(entry->value);
            break;

        case IFD_DOUBLE:
            {
                ULONG j;
                ULONGLONG *ull = (ULONGLONG *)(data_start + entry->value);
                for (j = 0; j < entry->count; j++)
                    ull[j] = ulonglong_bswap(ull[j]);
            }
            entry->value = ulong_bswap(entry->value);
            break;

        default:
            assert(0);
            break;
        }

        entry->id = ushort_bswap(entry->id);
        entry->type = ushort_bswap(entry->type);
        entry->count = ulong_bswap(entry->count);
        data += sizeof(*entry);
    }
}

struct test_data
{
    ULONG type, id;
    int count; /* if VT_VECTOR */
    LONGLONG value[13];
    const char *string;
    const WCHAR id_string[32];
};

static void compare_metadata(IWICMetadataReader *reader, const struct test_data *td, ULONG count)
{
    HRESULT hr;
    IWICEnumMetadataItem *enumerator;
    PROPVARIANT schema, id, value;
    ULONG items_returned, i;

    hr = IWICMetadataReader_GetEnumerator(reader, NULL);
    ok(hr == E_INVALIDARG, "GetEnumerator error %#x\n", hr);

    hr = IWICMetadataReader_GetEnumerator(reader, &enumerator);
    ok(hr == S_OK, "GetEnumerator error %#x\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    for (i = 0; i < count; i++)
    {
        hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
        ok(hr == S_OK, "Next error %#x\n", hr);
        ok(items_returned == 1, "unexpected item count %u\n", items_returned);

        ok(schema.vt == VT_EMPTY, "%u: unexpected vt: %u\n", i, schema.vt);
        ok(id.vt == VT_UI2 || id.vt == VT_LPWSTR || id.vt == VT_EMPTY, "%u: unexpected vt: %u\n", i, id.vt);
        if (id.vt == VT_UI2)
            ok(U(id).uiVal == td[i].id, "%u: expected id %#x, got %#x\n", i, td[i].id, U(id).uiVal);
        else if (id.vt == VT_LPWSTR)
            ok(!lstrcmpW(td[i].id_string, U(id).pwszVal),
               "%u: expected %s, got %s\n", i, wine_dbgstr_w(td[i].id_string), wine_dbgstr_w(U(id).pwszVal));

        ok(value.vt == td[i].type, "%u: expected vt %#x, got %#x\n", i, td[i].type, value.vt);
        if (value.vt & VT_VECTOR)
        {
            ULONG j;
            switch (value.vt & ~VT_VECTOR)
            {
            case VT_I1:
            case VT_UI1:
                ok(td[i].count == U(value).caub.cElems, "%u: expected cElems %d, got %d\n", i, td[i].count, U(value).caub.cElems);
                for (j = 0; j < U(value).caub.cElems; j++)
                    ok(td[i].value[j] == U(value).caub.pElems[j], "%u: expected value[%d] %#x/%#x, got %#x\n", i, j, (ULONG)td[i].value[j], (ULONG)(td[i].value[j] >> 32), U(value).caub.pElems[j]);
                break;
            case VT_I2:
            case VT_UI2:
                ok(td[i].count == U(value).caui.cElems, "%u: expected cElems %d, got %d\n", i, td[i].count, U(value).caui.cElems);
                for (j = 0; j < U(value).caui.cElems; j++)
                    ok(td[i].value[j] == U(value).caui.pElems[j], "%u: expected value[%d] %#x/%#x, got %#x\n", i, j, (ULONG)td[i].value[j], (ULONG)(td[i].value[j] >> 32), U(value).caui.pElems[j]);
                break;
            case VT_I4:
            case VT_UI4:
            case VT_R4:
                ok(td[i].count == U(value).caul.cElems, "%u: expected cElems %d, got %d\n", i, td[i].count, U(value).caul.cElems);
                for (j = 0; j < U(value).caul.cElems; j++)
                    ok(td[i].value[j] == U(value).caul.pElems[j], "%u: expected value[%d] %#x/%#x, got %#x\n", i, j, (ULONG)td[i].value[j], (ULONG)(td[i].value[j] >> 32), U(value).caul.pElems[j]);
                break;
            case VT_I8:
            case VT_UI8:
            case VT_R8:
                ok(td[i].count == U(value).cauh.cElems, "%u: expected cElems %d, got %d\n", i, td[i].count, U(value).cauh.cElems);
                for (j = 0; j < U(value).cauh.cElems; j++)
                    ok(td[i].value[j] == U(value).cauh.pElems[j].QuadPart, "%u: expected value[%d] %08x/%08x, got %08x/%08x\n", i, j, (ULONG)td[i].value[j], (ULONG)(td[i].value[j] >> 32), U(value).cauh.pElems[j].u.LowPart, U(value).cauh.pElems[j].u.HighPart);
                break;
            case VT_LPSTR:
                ok(td[i].count == U(value).calpstr.cElems, "%u: expected cElems %d, got %d\n", i, td[i].count, U(value).caub.cElems);
                for (j = 0; j < U(value).calpstr.cElems; j++)
                    trace("%u: %s\n", j, U(value).calpstr.pElems[j]);
                /* fall through to not handled message */
            default:
                ok(0, "%u: array of type %d is not handled\n", i, value.vt & ~VT_VECTOR);
                break;
            }
        }
        else if (value.vt == VT_LPSTR)
        {
            ok(td[i].count == strlen(U(value).pszVal) ||
               broken(td[i].count == strlen(U(value).pszVal) + 1), /* before Win7 */
               "%u: expected count %d, got %d\n", i, td[i].count, lstrlenA(U(value).pszVal));
            if (td[i].count == strlen(U(value).pszVal))
                ok(!strcmp(td[i].string, U(value).pszVal),
                   "%u: expected %s, got %s\n", i, td[i].string, U(value).pszVal);
        }
        else if (value.vt == VT_BLOB)
        {
            ok(td[i].count == U(value).blob.cbSize, "%u: expected count %d, got %d\n", i, td[i].count, U(value).blob.cbSize);
            ok(!memcmp(td[i].string, U(value).blob.pBlobData, td[i].count), "%u: expected %s, got %s\n", i, td[i].string, U(value).blob.pBlobData);
        }
        else
            ok(U(value).uhVal.QuadPart == td[i].value[0], "%u: expected value %#x/%#x got %#x/%#x\n",
               i, (UINT)td[i].value[0], (UINT)(td[i].value[0] >> 32), U(value).uhVal.u.LowPart, U(value).uhVal.u.HighPart);

        PropVariantClear(&schema);
        PropVariantClear(&id);
        PropVariantClear(&value);
    }

    hr = IWICEnumMetadataItem_Next(enumerator, 1, &schema, &id, &value, &items_returned);
    ok(hr == S_FALSE, "Next should fail\n");
    ok(items_returned == 0, "unexpected item count %u\n", items_returned);

    IWICEnumMetadataItem_Release(enumerator);
}

static void test_metadata_IFD(void)
{
    static const struct test_data td[28] =
    {
        { VT_UI2, 0xfe, 0, { 1 } },
        { VT_UI4, 0x100, 0, { 222 } },
        { VT_UI4, 0x101, 0, { 333 } },
        { VT_UI2, 0x102, 0, { 24 } },
        { VT_UI4, 0x103, 0, { 32773 } },
        { VT_UI8, 0x11a, 0, { ((LONGLONG)3 << 32) | 900 } },
        { VT_UI1, 0xf001, 0, { 0x44 } },
        { VT_UI1|VT_VECTOR, 0xf002, 4, { 0x44, 0x33, 0x22, 0x11 } },
        { VT_I1, 0xf003, 0, { 0x44 } },
        { VT_I2, 0xf004, 0, { 0x3344 } },
        { VT_I2|VT_VECTOR, 0xf005, 2, { 0x3344, 0x1122 } },
        { VT_I4, 0xf006, 0, { 0x11223344 } },
        { VT_R4, 0xf007, 0, { 0x11223344 } },
        { VT_R8, 0xf008, 0, { ((LONGLONG)0x41d26580 << 32) | 0xb486522c } },
        { VT_I8, 0xf009, 0, { ((LONGLONG)0x5a6b7c8d << 32) | 0x1a2b3c4d } },
        { VT_UI1|VT_VECTOR, 0xf00a, 13, { 'H','e','l','l','o',' ','W','o','r','l','d','!',0 } },
        { VT_I2|VT_VECTOR, 0xf00b, 4, { 0x0101, 0x0202, 0x0303, 0x0404 } },
        { VT_I4|VT_VECTOR, 0xf00c, 2, { 0x11223344, 0x55667788 } },
        { VT_R4|VT_VECTOR, 0xf00d, 2, { 0x449a522b, 0x4608f5ba } },
        { VT_LPSTR, 0xf00e, 12, { 0 }, "Hello World!" },
        { VT_LPSTR, 0xf00f, 4, { 0 }, "abcd" },
        { VT_BLOB, 0xf010, 13, { 0 }, "Hello World!" },
        { VT_BLOB, 0xf011, 4, { 0 }, "abcd" },
        { VT_UI1, 0xf012, 0, { 0x44 } },
        { VT_UI2, 0xf013, 0, { 0x3344 } },
        { VT_UI4, 0xf014, 0, { 0x11223344 } },
        { VT_R4, 0xf015, 0, { 0x11223344 } },
        { VT_I8|VT_VECTOR, 0xf016, 3,
          { ((LONGLONG)0x05060708 << 32) | 0x01020304,
            ((LONGLONG)0x50607080 << 32) | 0x10203040,
            ((LONGLONG)0x55667788 << 32) | 0x11223344 } },
    };
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICMetadataBlockReader *blockreader;
    PROPVARIANT schema, id, value;
    ULONG count;
    GUID format;
    char *IFD_data_swapped;
#ifdef WORDS_BIGENDIAN
    DWORD persist_options = WICPersistOptionBigEndian;
#else
    DWORD persist_options = WICPersistOptionLittleEndian;
#endif

    hr = CoCreateInstance(&CLSID_WICIfdMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount error %#x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    load_stream((IUnknown*)reader, (const char *)&IFD_data, sizeof(IFD_data), persist_options);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    /* test IFD data with different endianness */
    if (persist_options == WICPersistOptionLittleEndian)
        persist_options = WICPersistOptionBigEndian;
    else
        persist_options = WICPersistOptionLittleEndian;

    IFD_data_swapped = HeapAlloc(GetProcessHeap(), 0, sizeof(IFD_data));
    memcpy(IFD_data_swapped, &IFD_data, sizeof(IFD_data));
    byte_swap_ifd_data(IFD_data_swapped);
    load_stream((IUnknown *)reader, IFD_data_swapped, sizeof(IFD_data), persist_options);
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);
    compare_metadata(reader, td, count);
    HeapFree(GetProcessHeap(), 0, IFD_data_swapped);

    hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
    ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_MetadataFormatIfd), "unexpected format %s\n", wine_dbgstr_guid(&format));

    hr = IWICMetadataReader_GetMetadataFormat(reader, NULL);
    ok(hr == E_INVALIDARG, "GetMetadataFormat should fail\n");

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, count - 1, NULL, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %u\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, count - 1, &schema, NULL, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(schema.vt == VT_EMPTY, "unexpected vt: %u\n", schema.vt);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, &id, NULL);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(id.vt == VT_UI2, "unexpected vt: %u\n", id.vt);
    ok(U(id).uiVal == 0xfe, "unexpected id: %#x\n", U(id).uiVal);
    PropVariantClear(&id);

    hr = IWICMetadataReader_GetValueByIndex(reader, 0, NULL, NULL, &value);
    ok(hr == S_OK, "GetValueByIndex error %#x\n", hr);
    ok(value.vt == VT_UI2, "unexpected vt: %u\n", value.vt);
    ok(U(value).uiVal == 1, "unexpected id: %#x\n", U(value).uiVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValueByIndex(reader, count, &schema, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "expected WINCODEC_ERR_PROPERTYNOTFOUND, got %#x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "expected WINCODEC_ERR_PROPERTYNOTFOUND, got %#x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, &id, NULL);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "expected WINCODEC_ERR_PROPERTYNOTFOUND, got %#x\n", hr);

    hr = IWICMetadataReader_GetValue(reader, &schema, NULL, &value);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

    id.vt = VT_UI2;
    U(id).uiVal = 0xf00e;
    hr = IWICMetadataReader_GetValue(reader, NULL, &id, NULL);
    ok(hr == S_OK, "GetValue error %#x\n", hr);

    /* schema is ignored by Ifd metadata reader */
    schema.vt = VT_UI4;
    U(schema).ulVal = 0xdeadbeef;
    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    ok(hr == S_OK, "GetValue error %#x\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(value).pszVal, "Hello World!"), "unexpected value: %s\n", U(value).pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
    ok(hr == S_OK, "GetValue error %#x\n", hr);
    ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
    ok(!strcmp(U(value).pszVal, "Hello World!"), "unexpected value: %s\n", U(value).pszVal);
    PropVariantClear(&value);

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
        IWICMetadataBlockReader_Release(blockreader);

    IWICMetadataReader_Release(reader);
}

static void test_metadata_Exif(void)
{
    HRESULT hr;
    IWICMetadataReader *reader;
    IWICMetadataBlockReader *blockreader;
    UINT count=0;

    hr = CoCreateInstance(&CLSID_WICExifMetadataReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICMetadataReader, (void**)&reader);
    todo_wine ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICMetadataReader_GetCount(reader, NULL);
    ok(hr == E_INVALIDARG, "GetCount error %#x\n", hr);

    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == 0, "unexpected count %u\n", count);

    hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
        IWICMetadataBlockReader_Release(blockreader);

    IWICMetadataReader_Release(reader);
}

static void test_create_reader(void)
{
    HRESULT hr;
    IWICComponentFactory *factory;
    IStream *stream;
    IWICMetadataReader *reader;
    UINT count=0;
    GUID format;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICComponentFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

    stream = create_stream(metadata_tEXt, sizeof(metadata_tEXt));

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        NULL, NULL, WICPersistOptionDefault,
        stream, &reader);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionDefault,
        NULL, &reader);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionDefault,
        stream, NULL);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionDefault,
        stream, &reader);
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %i\n", count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatChunktEXt), "unexpected format %s\n", wine_dbgstr_guid(&format));

        IWICMetadataReader_Release(reader);
    }

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatWmp, NULL, WICPersistOptionDefault,
        stream, &reader);
    ok(hr == S_OK, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %i\n", count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown), "unexpected format %s\n", wine_dbgstr_guid(&format));

        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);

    IWICComponentFactory_Release(factory);
}

static void test_metadata_png(void)
{
    static const struct test_data td[6] =
    {
        { VT_UI2, 0, 0, { 2005 }, NULL, { 'Y','e','a','r',0 } },
        { VT_UI1, 0, 0, { 6 }, NULL, { 'M','o','n','t','h',0 } },
        { VT_UI1, 0, 0, { 3 }, NULL, { 'D','a','y',0 } },
        { VT_UI1, 0, 0, { 15 }, NULL, { 'H','o','u','r',0 } },
        { VT_UI1, 0, 0, { 7 }, NULL, { 'M','i','n','u','t','e',0 } },
        { VT_UI1, 0, 0, { 45 }, NULL, { 'S','e','c','o','n','d',0 } }
    };
    IStream *stream;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICMetadataBlockReader *blockreader;
    IWICMetadataReader *reader;
    IWICMetadataQueryReader *queryreader;
    IWICComponentFactory *factory;
    GUID containerformat;
    HRESULT hr;
    UINT count=0xdeadbeef;

    hr = CoCreateInstance(&CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

    if (FAILED(hr)) return;

    stream = create_stream(pngimage, sizeof(pngimage));

    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Initialize failed, hr=%x\n", hr);

    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, hr=%x\n", hr);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame failed, hr=%x\n", hr);

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void**)&blockreader);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, NULL);
        ok(hr == E_INVALIDARG, "GetContainerFormat failed, hr=%x\n", hr);

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &containerformat);
        ok(hr == S_OK, "GetContainerFormat failed, hr=%x\n", hr);
        ok(IsEqualGUID(&containerformat, &GUID_ContainerFormatPng), "unexpected container format\n");

        hr = IWICMetadataBlockReader_GetCount(blockreader, NULL);
        ok(hr == E_INVALIDARG, "GetCount failed, hr=%x\n", hr);

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %d\n", count);

        if (0)
        {
            /* Crashes on Windows XP */
            hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, NULL);
            ok(hr == E_INVALIDARG, "GetReaderByIndex failed, hr=%x\n", hr);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex failed, hr=%x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &containerformat);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            todo_wine ok(IsEqualGUID(&containerformat, &GUID_MetadataFormatChunktIME) ||
               broken(IsEqualGUID(&containerformat, &GUID_MetadataFormatUnknown)) /* Windows XP */,
               "unexpected container format\n");

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            todo_wine ok(count == 6 || broken(count == 1) /* XP */, "expected 6, got %u\n", count);
            if (count == 6)
                compare_metadata(reader, td, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        todo_wine ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "GetReaderByIndex failed, hr=%x\n", hr);

        hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void**)&factory);
        ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);

        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, NULL, &queryreader);
        ok(hr == E_INVALIDARG, "CreateQueryReaderFromBlockReader should have failed: %08x\n", hr);

        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, blockreader, NULL);
        ok(hr == E_INVALIDARG, "CreateQueryReaderFromBlockReader should have failed: %08x\n", hr);

        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, blockreader, &queryreader);
        ok(hr == S_OK, "CreateQueryReaderFromBlockReader failed: %08x\n", hr);

        IWICMetadataQueryReader_Release(queryreader);

        IWICComponentFactory_Release(factory);

        IWICMetadataBlockReader_Release(blockreader);
    }

    hr = IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &queryreader);
    ok(hr == S_OK, "GetMetadataQueryReader failed: %08x\n", hr);

    if (SUCCEEDED(hr))
    {
        IWICMetadataQueryReader_Release(queryreader);
    }

    IWICBitmapFrameDecode_Release(frame);

    IWICBitmapDecoder_Release(decoder);

    IStream_Release(stream);
}

static void test_metadata_gif(void)
{
    static const struct test_data gif_LSD[9] =
    {
        { VT_UI1|VT_VECTOR, 0, 6, {'G','I','F','8','7','a'}, NULL, { 'S','i','g','n','a','t','u','r','e',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'C','o','l','o','r','R','e','s','o','l','u','t','i','o','n',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'B','a','c','k','g','r','o','u','n','d','C','o','l','o','r','I','n','d','e','x',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'P','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0 } }
    };
    static const struct test_data gif_IMD[8] =
    {
        { VT_UI2, 0, 0, { 0 }, NULL, { 'L','e','f','t',0 } },
        { VT_UI2, 0, 0, { 0 }, NULL, { 'T','o','p',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'I','n','t','e','r','l','a','c','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } }
    };
    static const struct test_data animated_gif_LSD[9] =
    {
        { VT_UI1|VT_VECTOR, 0, 6, {'G','I','F','8','9','a'}, NULL, { 'S','i','g','n','a','t','u','r','e',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 2 }, NULL, { 'C','o','l','o','r','R','e','s','o','l','u','t','i','o','n',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'B','a','c','k','g','r','o','u','n','d','C','o','l','o','r','I','n','d','e','x',0 } },
        { VT_UI1, 0, 0, { 0 }, NULL, { 'P','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0 } }
    };
    static const struct test_data animated_gif_IMD[8] =
    {
        { VT_UI2, 0, 0, { 0 }, NULL, { 'L','e','f','t',0 } },
        { VT_UI2, 0, 0, { 0 }, NULL, { 'T','o','p',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 1 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'I','n','t','e','r','l','a','c','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 1 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } }
    };
    static const struct test_data animated_gif_GCE[5] =
    {
        { VT_UI1, 0, 0, { 0 }, NULL, { 'D','i','s','p','o','s','a','l',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'U','s','e','r','I','n','p','u','t','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'T','r','a','n','s','p','a','r','e','n','c','y','F','l','a','g',0 } },
        { VT_UI2, 0, 0, { 10 }, NULL, { 'D','e','l','a','y',0 } },
        { VT_UI1, 0, 0, { 1 }, NULL, { 'T','r','a','n','s','p','a','r','e','n','t','C','o','l','o','r','I','n','d','e','x',0 } }
    };
    static const struct test_data animated_gif_APE[2] =
    {
        { VT_UI1|VT_VECTOR, 0, 11, { 'A','N','I','M','E','X','T','S','1','.','0' }, NULL, { 'A','p','p','l','i','c','a','t','i','o','n',0 } },
        { VT_UI1|VT_VECTOR, 0, 4, { 0x03,0x01,0x05,0x00 }, NULL, { 'D','a','t','a',0 } }
    };
    static const struct test_data animated_gif_comment_1[1] =
    {
        { VT_LPSTR, 0, 12, { 0 }, "Hello World!", { 'T','e','x','t','E','n','t','r','y',0 } }
    };
    static const struct test_data animated_gif_comment_2[1] =
    {
        { VT_LPSTR, 0, 8, { 0 }, "image #1", { 'T','e','x','t','E','n','t','r','y',0 } }
    };
    static const struct test_data animated_gif_plain_1[1] =
    {
        { VT_BLOB, 0, 17, { 0 }, "\x21\x01\x0d\x61nimation.gif" }
    };
    static const struct test_data animated_gif_plain_2[1] =
    {
        { VT_BLOB, 0, 16, { 0 }, "\x21\x01\x0cplaintext #1" }
    };
    IStream *stream;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICMetadataBlockReader *blockreader;
    IWICMetadataReader *reader;
    IWICMetadataQueryReader *queryreader;
    GUID format;
    HRESULT hr;
    UINT count;

    /* 1x1 pixel gif */
    stream = create_stream(gifimage, sizeof(gifimage));

    hr = CoCreateInstance(&CLSID_WICGifDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Initialize error %#x\n", hr);

    IStream_Release(stream);

    /* global metadata block */
    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == 1, "expected 1, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatLSD), /* Logical Screen Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(gif_LSD), "unexpected count %u\n", count);

            compare_metadata(reader, gif_LSD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    /* frame metadata block */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == 1, "expected 1, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), /* Image Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(gif_IMD), "unexpected count %u\n", count);

            compare_metadata(reader, gif_IMD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);

    /* 1x1 pixel gif, 2 frames */
    stream = create_stream(animatedgif, sizeof(animatedgif));

    hr = CoCreateInstance(&CLSID_WICGifDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    ok(hr == S_OK, "Initialize error %#x\n", hr);

    IStream_Release(stream);

    /* global metadata block */
    hr = IWICBitmapDecoder_QueryInterface(decoder, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatLSD), /* Logical Screen Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_LSD), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_LSD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatAPE), /* Application Extension */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_APE), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_APE, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 2, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatGifComment), /* Comment Extension */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_comment_1), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_comment_1, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 3, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown),
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_plain_1), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_plain_1, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 4, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    /* frame metadata block */
    hr = IWICBitmapDecoder_GetFrame(decoder, 1, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICMetadataBlockReader, (void **)&blockreader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* before Win7 */, "QueryInterface error %#x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        hr = IWICMetadataBlockReader_GetContainerFormat(blockreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataBlockReader_GetCount(blockreader, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        hr = IWICMetadataBlockReader_GetCount(blockreader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 0, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), /* Image Descriptor */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_IMD), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_IMD, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 1, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatGifComment), /* Comment Extension */
                "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_comment_2), "unexpected count %u\n", count);

            if (count == 1)
            compare_metadata(reader, animated_gif_comment_2, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 2, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatUnknown),
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_plain_2), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_plain_2, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 3, &reader);
        ok(hr == S_OK, "GetReaderByIndex error %#x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
            ok(hr == S_OK, "GetMetadataFormat failed, hr=%#x\n", hr);
            ok(IsEqualGUID(&format, &GUID_MetadataFormatGCE), /* Graphic Control Extension */
               "wrong metadata format %s\n", wine_dbgstr_guid(&format));

            hr = IWICMetadataReader_GetCount(reader, &count);
            ok(hr == S_OK, "GetCount error %#x\n", hr);
            ok(count == ARRAY_SIZE(animated_gif_GCE), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_GCE, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 4, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
    }

    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, &queryreader);
    ok(hr == S_OK || broken(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION) /* before Vista */,
       "GetMetadataQueryReader error %#x\n", hr);
    if (SUCCEEDED(hr))
    {
        static const struct
        {
            const char *query;
            HRESULT hr;
            UINT vt;
        } decoder_data[] =
        {
            { "/logscrdesc/Signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/[0]logscrdesc/Signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/logscrdesc/\\Signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/Logscrdesc/\\signature", S_OK, VT_UI1 | VT_VECTOR },
            { "/logscrdesc/{str=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/[0]logscrdesc/{str=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/logscrdesc/{wstr=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/[0]logscrdesc/{wstr=signature}", S_OK, VT_UI1 | VT_VECTOR },
            { "/appext/Application", S_OK, VT_UI1 | VT_VECTOR },
            { "/appext/{STR=APPlication}", S_OK, VT_UI1 | VT_VECTOR },
            { "/appext/{WSTR=APPlication}", S_OK, VT_UI1 | VT_VECTOR },
            { "/LogSCRdesC", S_OK, VT_UNKNOWN },
            { "/[0]LogSCRdesC", S_OK, VT_UNKNOWN },
            { "/appEXT", S_OK, VT_UNKNOWN },
            { "/[0]appEXT", S_OK, VT_UNKNOWN },
            { "grctlext", WINCODEC_ERR_PROPERTYNOTSUPPORTED, 0 },
            { "/imgdesc", WINCODEC_ERR_PROPERTYNOTFOUND, 0 },
        };
        static const WCHAR rootW[] = {'/',0};
        WCHAR name[256];
        UINT len, i, j;
        PROPVARIANT value;
        IWICMetadataQueryReader *meta_reader;

        hr = IWICMetadataQueryReader_GetContainerFormat(queryreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        name[0] = 0;
        len = 0xdeadbeef;
        hr = IWICMetadataQueryReader_GetLocation(queryreader, 256, name, &len);
        ok(hr == S_OK, "GetLocation error %#x\n", hr);
        ok(len == 2, "expected 2, got %u\n", len);
        ok(!lstrcmpW(name, rootW), "expected '/', got %s\n", wine_dbgstr_w(name));

        for (i = 0; i < ARRAY_SIZE(decoder_data); i++)
        {
            WCHAR queryW[256];

            if (winetest_debug > 1)
                trace("query: %s\n", decoder_data[i].query);
            MultiByteToWideChar(CP_ACP, 0, decoder_data[i].query, -1, queryW, 256);

            hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, NULL);
            ok(hr == decoder_data[i].hr, "GetMetadataByName(%s) returned %#x, expected %#x\n", wine_dbgstr_w(queryW), hr, decoder_data[i].hr);

            PropVariantInit(&value);
            hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, &value);
            ok(hr == decoder_data[i].hr, "GetMetadataByName(%s) returned %#x, expected %#x\n", wine_dbgstr_w(queryW), hr, decoder_data[i].hr);
            ok(value.vt == decoder_data[i].vt, "expected %#x, got %#x\n", decoder_data[i].vt, value.vt);
            if (hr == S_OK && value.vt == VT_UNKNOWN)
            {
                hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&meta_reader);
                ok(hr == S_OK, "QueryInterface error %#x\n", hr);

                name[0] = 0;
                len = 0xdeadbeef;
                hr = IWICMetadataQueryReader_GetLocation(meta_reader, 256, name, &len);
                ok(hr == S_OK, "GetLocation error %#x\n", hr);
                ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);
                ok(!lstrcmpW(name, queryW), "expected %s, got %s\n", wine_dbgstr_w(queryW), wine_dbgstr_w(name));

                for (j = 0; j < ARRAY_SIZE(decoder_data); j++)
                {
                    MultiByteToWideChar(CP_ACP, 0, decoder_data[j].query, -1, queryW, 256);

                    if (CompareStringW(LOCALE_NEUTRAL, NORM_IGNORECASE, queryW, len-1, name, len-1) == CSTR_EQUAL && decoder_data[j].query[len - 1] != 0)
                    {
                        if (winetest_debug > 1)
                            trace("query: %s\n", wine_dbgstr_w(queryW + len - 1));
                        PropVariantClear(&value);
                        hr = IWICMetadataQueryReader_GetMetadataByName(meta_reader, queryW + len - 1, &value);
                        ok(hr == decoder_data[j].hr, "GetMetadataByName(%s) returned %#x, expected %#x\n", wine_dbgstr_w(queryW + len - 1), hr, decoder_data[j].hr);
                        ok(value.vt == decoder_data[j].vt, "expected %#x, got %#x\n", decoder_data[j].vt, value.vt);
                    }
                }

                IWICMetadataQueryReader_Release(meta_reader);
            }

            PropVariantClear(&value);
        }

        IWICMetadataQueryReader_Release(queryreader);
    }

    hr = IWICBitmapFrameDecode_GetMetadataQueryReader(frame, &queryreader);
    ok(hr == S_OK || broken(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION) /* before Vista */,
       "GetMetadataQueryReader error %#x\n", hr);
    if (SUCCEEDED(hr))
    {
        static const struct
        {
            const char *query;
            HRESULT hr;
            UINT vt;
        } frame_data[] =
        {
            { "/grctlext/Delay", S_OK, VT_UI2 },
            { "/[0]grctlext/Delay", S_OK, VT_UI2 },
            { "/grctlext/{str=delay}", S_OK, VT_UI2 },
            { "/[0]grctlext/{str=delay}", S_OK, VT_UI2 },
            { "/grctlext/{wstr=delay}", S_OK, VT_UI2 },
            { "/[0]grctlext/{wstr=delay}", S_OK, VT_UI2 },
            { "/imgdesc/InterlaceFlag", S_OK, VT_BOOL },
            { "/imgdesc/{STR=interlaceFLAG}", S_OK, VT_BOOL },
            { "/imgdesc/{WSTR=interlaceFLAG}", S_OK, VT_BOOL },
            { "/grctlext", S_OK, VT_UNKNOWN },
            { "/[0]grctlext", S_OK, VT_UNKNOWN },
            { "/imgdesc", S_OK, VT_UNKNOWN },
            { "/[0]imgdesc", S_OK, VT_UNKNOWN },
            { "/LogSCRdesC", WINCODEC_ERR_PROPERTYNOTFOUND, 0 },
            { "/appEXT", WINCODEC_ERR_PROPERTYNOTFOUND, 0 },
            { "/grctlext/{\\str=delay}", WINCODEC_ERR_WRONGSTATE, 0 },
            { "/grctlext/{str=\\delay}", S_OK, VT_UI2 },
            { "grctlext/Delay", WINCODEC_ERR_PROPERTYNOTSUPPORTED, 0 },
        };
        static const WCHAR rootW[] = {'/',0};
        static const WCHAR guidW[] = {'/','{','g','u','i','d','=','\\',0};
        static const WCHAR imgdescW[] = {'i','m','g','d','e','s','c',0};
        static const WCHAR ImgDescW[] = {'I','m','g','D','e','s','c',0};
        WCHAR name[256], queryW[256];
        UINT len, i;
        PROPVARIANT value;
        IWICMetadataQueryReader *meta_reader;

        hr = IWICMetadataQueryReader_GetContainerFormat(queryreader, &format);
        ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        name[0] = 0;
        len = 0xdeadbeef;
        hr = IWICMetadataQueryReader_GetLocation(queryreader, 256, name, &len);
        ok(hr == S_OK, "GetLocation error %#x\n", hr);
        ok(len == 2, "expected 2, got %u\n", len);
        ok(!lstrcmpW(name, rootW), "expected '/', got %s\n", wine_dbgstr_w(name));

        for (i = 0; i < ARRAY_SIZE(frame_data); i++)
        {
            if (winetest_debug > 1)
                trace("query: %s\n", frame_data[i].query);
            MultiByteToWideChar(CP_ACP, 0, frame_data[i].query, -1, queryW, 256);
            PropVariantInit(&value);
            hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, &value);
            ok(hr == frame_data[i].hr, "GetMetadataByName(%s) returned %#x, expected %#x\n", wine_dbgstr_w(queryW), hr, frame_data[i].hr);
            ok(value.vt == frame_data[i].vt, "expected %#x, got %#x\n", frame_data[i].vt, value.vt);
            if (hr == S_OK && value.vt == VT_UNKNOWN)
            {
                hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&meta_reader);
                ok(hr == S_OK, "QueryInterface error %#x\n", hr);

                name[0] = 0;
                len = 0xdeadbeef;
                hr = IWICMetadataQueryReader_GetLocation(meta_reader, 256, name, &len);
                ok(hr == S_OK, "GetLocation error %#x\n", hr);
                ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);
                ok(!lstrcmpW(name, queryW), "expected %s, got %s\n", wine_dbgstr_w(queryW), wine_dbgstr_w(name));

                IWICMetadataQueryReader_Release(meta_reader);
            }

            PropVariantClear(&value);
        }

        name[0] = 0;
        len = 0xdeadbeef;
        hr = WICMapGuidToShortName(&GUID_MetadataFormatIMD, 256, name, &len);
        ok(hr == S_OK, "WICMapGuidToShortName error %#x\n", hr);
        ok(!lstrcmpW(name, imgdescW), "wrong short name %s\n", wine_dbgstr_w(name));

        format = GUID_NULL;
        hr = WICMapShortNameToGuid(imgdescW, &format);
        ok(hr == S_OK, "WICMapGuidToShortName error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), "wrong guid %s\n", wine_dbgstr_guid(&format));

        format = GUID_NULL;
        hr = WICMapShortNameToGuid(ImgDescW, &format);
        ok(hr == S_OK, "WICMapGuidToShortName error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), "wrong guid %s\n", wine_dbgstr_guid(&format));

        lstrcpyW(queryW, guidW);
        StringFromGUID2(&GUID_MetadataFormatIMD, queryW + lstrlenW(queryW) - 1, 39);
        memcpy(queryW, guidW, sizeof(guidW) - 2);
        if (winetest_debug > 1)
            trace("query: %s\n", wine_dbgstr_w(queryW));
        PropVariantInit(&value);
        hr = IWICMetadataQueryReader_GetMetadataByName(queryreader, queryW, &value);
        ok(hr == S_OK, "GetMetadataByName(%s) error %#x\n", wine_dbgstr_w(queryW), hr);
        ok(value.vt == VT_UNKNOWN, "expected VT_UNKNOWN, got %#x\n", value.vt);
        PropVariantClear(&value);

        IWICMetadataQueryReader_Release(queryreader);
    }

    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_metadata_LSD(void)
{
    static const WCHAR LSD_name[] = {'L','o','g','i','c','a','l',' ','S','c','r','e','e','n',' ','D','e','s','c','r','i','p','t','o','r',' ','R','e','a','d','e','r',0};
    static const char LSD_data[] = "hello world!\x1\x2\x3\x4\xab\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf";
    static const struct test_data td[9] =
    {
        { VT_UI1|VT_VECTOR, 0, 6, {'w','o','r','l','d','!'}, NULL, { 'S','i','g','n','a','t','u','r','e',0 } },
        { VT_UI2, 0, 0, { 0x201 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 0x403 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 2 }, NULL, { 'C','o','l','o','r','R','e','s','o','l','u','t','i','o','n',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 3 }, NULL, { 'G','l','o','b','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } },
        { VT_UI1, 0, 0, { 6 }, NULL, { 'B','a','c','k','g','r','o','u','n','d','C','o','l','o','r','I','n','d','e','x',0 } },
        { VT_UI1, 0, 0, { 7 }, NULL, { 'P','i','x','e','l','A','s','p','e','c','t','R','a','t','i','o',0 } }
    };
    LARGE_INTEGER pos;
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy;
    GUID format;
    CLSID id;

    hr = CoCreateInstance(&CLSID_WICLSDMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#x\n", hr);

    stream = create_stream(LSD_data, sizeof(LSD_data));

    if (SUCCEEDED(hr))
    {
        pos.QuadPart = 6;
        hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek error %#x\n", hr);

        hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
        ok(hr == S_OK, "QueryInterface error %#x\n", hr);

        hr = IWICPersistStream_Load(persist, stream);
        ok(hr == S_OK, "Load error %#x\n", hr);

        IWICPersistStream_Release(persist);
    }

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

        compare_metadata(reader, td, count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatLSD), "wrong format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
        ok(hr == S_OK, "GetMetadataHandlerInfo error %#x\n", hr);

        hr = IWICMetadataHandlerInfo_GetCLSID(info, &id);
        ok(hr == S_OK, "GetCLSID error %#x\n", hr);
        ok(IsEqualGUID(&id, &CLSID_WICLSDMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&id));

        hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
        ok(hr == S_OK, "GetFriendlyName error %#x\n", hr);
        ok(lstrcmpW(name, LSD_name) == 0, "wrong LSD reader name %s\n", wine_dbgstr_w(name));

        IWICMetadataHandlerInfo_Release(info);
        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);
}

static void test_metadata_IMD(void)
{
    static const WCHAR IMD_name[] = {'I','m','a','g','e',' ','D','e','s','c','r','i','p','t','o','r',' ','R','e','a','d','e','r',0};
    static const char IMD_data[] = "hello world!\x1\x2\x3\x4\x5\x6\x7\x8\xed\xa\xb\xc\xd\xe\xf";
    static const struct test_data td[8] =
    {
        { VT_UI2, 0, 0, { 0x201 }, NULL, { 'L','e','f','t',0 } },
        { VT_UI2, 0, 0, { 0x403 }, NULL, { 'T','o','p',0 } },
        { VT_UI2, 0, 0, { 0x605 }, NULL, { 'W','i','d','t','h',0 } },
        { VT_UI2, 0, 0, { 0x807 }, NULL, { 'H','e','i','g','h','t',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'I','n','t','e','r','l','a','c','e','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'S','o','r','t','F','l','a','g',0 } },
        { VT_UI1, 0, 0, { 5 }, NULL, { 'L','o','c','a','l','C','o','l','o','r','T','a','b','l','e','S','i','z','e',0 } }
    };
    LARGE_INTEGER pos;
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy;
    GUID format;
    CLSID id;

    hr = CoCreateInstance(&CLSID_WICIMDMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#x\n", hr);

    stream = create_stream(IMD_data, sizeof(IMD_data));

    if (SUCCEEDED(hr))
    {
        pos.QuadPart = 12;
        hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek error %#x\n", hr);

        hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
        ok(hr == S_OK, "QueryInterface error %#x\n", hr);

        hr = IWICPersistStream_Load(persist, stream);
        ok(hr == S_OK, "Load error %#x\n", hr);

        IWICPersistStream_Release(persist);
    }

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

        compare_metadata(reader, td, count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatIMD), "wrong format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
        ok(hr == S_OK, "GetMetadataHandlerInfo error %#x\n", hr);

        hr = IWICMetadataHandlerInfo_GetCLSID(info, &id);
        ok(hr == S_OK, "GetCLSID error %#x\n", hr);
        ok(IsEqualGUID(&id, &CLSID_WICIMDMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&id));

        hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
        ok(hr == S_OK, "GetFriendlyName error %#x\n", hr);
        ok(lstrcmpW(name, IMD_name) == 0, "wrong IMD reader name %s\n", wine_dbgstr_w(name));

        IWICMetadataHandlerInfo_Release(info);
        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);
}

static void test_metadata_GCE(void)
{
    static const WCHAR GCE_name[] = {'G','r','a','p','h','i','c',' ','C','o','n','t','r','o','l',' ','E','x','t','e','n','s','i','o','n',' ','R','e','a','d','e','r',0};
    static const char GCE_data[] = "hello world!\xa\x2\x3\x4\x5\x6\x7\x8\xed\xa\xb\xc\xd\xe\xf";
    static const struct test_data td[5] =
    {
        { VT_UI1, 0, 0, { 2 }, NULL, { 'D','i','s','p','o','s','a','l',0 } },
        { VT_BOOL, 0, 0, { 1 }, NULL, { 'U','s','e','r','I','n','p','u','t','F','l','a','g',0 } },
        { VT_BOOL, 0, 0, { 0 }, NULL, { 'T','r','a','n','s','p','a','r','e','n','c','y','F','l','a','g',0 } },
        { VT_UI2, 0, 0, { 0x302 }, NULL, { 'D','e','l','a','y',0 } },
        { VT_UI1, 0, 0, { 4 }, NULL, { 'T','r','a','n','s','p','a','r','e','n','t','C','o','l','o','r','I','n','d','e','x',0 } }
    };
    LARGE_INTEGER pos;
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy;
    GUID format;
    CLSID id;

    hr = CoCreateInstance(&CLSID_WICGCEMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#x\n", hr);

    stream = create_stream(GCE_data, sizeof(GCE_data));

    if (SUCCEEDED(hr))
    {
        pos.QuadPart = 12;
        hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek error %#x\n", hr);

        hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
        ok(hr == S_OK, "QueryInterface error %#x\n", hr);

        hr = IWICPersistStream_Load(persist, stream);
        ok(hr == S_OK, "Load error %#x\n", hr);

        IWICPersistStream_Release(persist);
    }

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

        compare_metadata(reader, td, count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatGCE), "wrong format %s\n", wine_dbgstr_guid(&format));

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
        ok(hr == S_OK, "GetMetadataHandlerInfo error %#x\n", hr);

        hr = IWICMetadataHandlerInfo_GetCLSID(info, &id);
        ok(hr == S_OK, "GetCLSID error %#x\n", hr);
        ok(IsEqualGUID(&id, &CLSID_WICGCEMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&id));

        hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
        ok(hr == S_OK, "GetFriendlyName error %#x\n", hr);
        ok(lstrcmpW(name, GCE_name) == 0, "wrong GCE reader name %s\n", wine_dbgstr_w(name));

        IWICMetadataHandlerInfo_Release(info);
        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);
}

static void test_metadata_APE(void)
{
    static const WCHAR APE_name[] = {'A','p','p','l','i','c','a','t','i','o','n',' ','E','x','t','e','n','s','i','o','n',' ','R','e','a','d','e','r',0};
    static const char APE_data[] = { 0x21,0xff,0x0b,'H','e','l','l','o',' ','W','o','r','l','d',
                                     /*sub-block*/1,0x11,
                                     /*sub-block*/2,0x22,0x33,
                                     /*sub-block*/4,0x44,0x55,0x66,0x77,
                                     /*terminator*/0 };
    static const struct test_data td[2] =
    {
        { VT_UI1|VT_VECTOR, 0, 11, { 'H','e','l','l','o',' ','W','o','r','l','d' }, NULL, { 'A','p','p','l','i','c','a','t','i','o','n',0 } },
        { VT_UI1|VT_VECTOR, 0, 10, { 1,0x11,2,0x22,0x33,4,0x44,0x55,0x66,0x77 }, NULL, { 'D','a','t','a',0 } }
    };
    WCHAR dataW[] = { 'd','a','t','a',0 };
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy, i;
    GUID format;
    CLSID clsid;
    PROPVARIANT id, value;

    hr = CoCreateInstance(&CLSID_WICAPEMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#x\n", hr);

    stream = create_stream(APE_data, sizeof(APE_data));

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
        ok(hr == S_OK, "QueryInterface error %#x\n", hr);

        hr = IWICPersistStream_Load(persist, stream);
        ok(hr == S_OK, "Load error %#x\n", hr);

        IWICPersistStream_Release(persist);
    }

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

        compare_metadata(reader, td, count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatAPE), "wrong format %s\n", wine_dbgstr_guid(&format));

        PropVariantInit(&value);
        id.vt = VT_LPWSTR;
        U(id).pwszVal = dataW;

        hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
        ok(hr == S_OK, "GetValue error %#x\n", hr);
        ok(value.vt == (VT_UI1|VT_VECTOR), "unexpected vt: %i\n", id.vt);
        ok(td[1].count == U(value).caub.cElems, "expected cElems %d, got %d\n", td[1].count, U(value).caub.cElems);
        for (i = 0; i < U(value).caub.cElems; i++)
            ok(td[1].value[i] == U(value).caub.pElems[i], "%u: expected value %#x/%#x, got %#x\n", i, (ULONG)td[1].value[i], (ULONG)(td[1].value[i] >> 32), U(value).caub.pElems[i]);
        PropVariantClear(&value);

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
        ok(hr == S_OK, "GetMetadataHandlerInfo error %#x\n", hr);

        hr = IWICMetadataHandlerInfo_GetCLSID(info, &clsid);
        ok(hr == S_OK, "GetCLSID error %#x\n", hr);
        ok(IsEqualGUID(&clsid, &CLSID_WICAPEMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&clsid));

        hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
        ok(hr == S_OK, "GetFriendlyName error %#x\n", hr);
        ok(lstrcmpW(name, APE_name) == 0, "wrong APE reader name %s\n", wine_dbgstr_w(name));

        IWICMetadataHandlerInfo_Release(info);
        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);
}

static void test_metadata_GIF_comment(void)
{
    static const WCHAR GIF_comment_name[] = {'C','o','m','m','e','n','t',' ','E','x','t','e','n','s','i','o','n',' ','R','e','a','d','e','r',0};
    static const char GIF_comment_data[] = { 0x21,0xfe,
                                             /*sub-block*/5,'H','e','l','l','o',
                                             /*sub-block*/1,' ',
                                             /*sub-block*/6,'W','o','r','l','d','!',
                                             /*terminator*/0 };
    static const struct test_data td[1] =
    {
        { VT_LPSTR, 0, 12, { 0 }, "Hello World!", { 'T','e','x','t','E','n','t','r','y',0 } }
    };
    WCHAR text_entryW[] = { 'T','E','X','T','E','N','T','R','Y',0 };
    HRESULT hr;
    IStream *stream;
    IWICPersistStream *persist;
    IWICMetadataReader *reader;
    IWICMetadataHandlerInfo *info;
    WCHAR name[64];
    UINT count, dummy;
    GUID format;
    CLSID clsid;
    PROPVARIANT id, value;

    hr = CoCreateInstance(&CLSID_WICGifCommentMetadataReader, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICMetadataReader, (void **)&reader);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE || hr == REGDB_E_CLASSNOTREG) /* before Win7 */,
       "CoCreateInstance error %#x\n", hr);

    stream = create_stream(GIF_comment_data, sizeof(GIF_comment_data));

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist);
        ok(hr == S_OK, "QueryInterface error %#x\n", hr);

        hr = IWICPersistStream_Load(persist, stream);
        ok(hr == S_OK, "Load error %#x\n", hr);

        IWICPersistStream_Release(persist);
    }

    if (SUCCEEDED(hr))
    {
        hr = IWICMetadataReader_GetCount(reader, &count);
        ok(hr == S_OK, "GetCount error %#x\n", hr);
        ok(count == ARRAY_SIZE(td), "unexpected count %u\n", count);

        compare_metadata(reader, td, count);

        hr = IWICMetadataReader_GetMetadataFormat(reader, &format);
        ok(hr == S_OK, "GetMetadataFormat error %#x\n", hr);
        ok(IsEqualGUID(&format, &GUID_MetadataFormatGifComment), "wrong format %s\n", wine_dbgstr_guid(&format));

        PropVariantInit(&value);
        id.vt = VT_LPWSTR;
        U(id).pwszVal = text_entryW;

        hr = IWICMetadataReader_GetValue(reader, NULL, &id, &value);
        ok(hr == S_OK, "GetValue error %#x\n", hr);
        ok(value.vt == VT_LPSTR, "unexpected vt: %i\n", id.vt);
        ok(!strcmp(U(value).pszVal, "Hello World!"), "unexpected value: %s\n", U(value).pszVal);
        PropVariantClear(&value);

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
        ok(hr == S_OK, "GetMetadataHandlerInfo error %#x\n", hr);

        hr = IWICMetadataHandlerInfo_GetCLSID(info, &clsid);
        ok(hr == S_OK, "GetCLSID error %#x\n", hr);
        ok(IsEqualGUID(&clsid, &CLSID_WICGifCommentMetadataReader), "wrong CLSID %s\n", wine_dbgstr_guid(&clsid));

        hr = IWICMetadataHandlerInfo_GetFriendlyName(info, 64, name, &dummy);
        ok(hr == S_OK, "GetFriendlyName error %#x\n", hr);
        ok(lstrcmpW(name, GIF_comment_name) == 0, "wrong APE reader name %s\n", wine_dbgstr_w(name));

        IWICMetadataHandlerInfo_Release(info);
        IWICMetadataReader_Release(reader);
    }

    IStream_Release(stream);
}

static void test_WICMapGuidToShortName(void)
{
    static const WCHAR unkW[] = { 'u','n','k',0 };
    static const WCHAR unknownW[] = { 'u','n','k','n','o','w','n',0 };
    HRESULT hr;
    UINT len;
    WCHAR name[16];

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, name, &len);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(len == 8, "got %u\n", len);
    ok(!lstrcmpW(name, unknownW), "got %s\n", wine_dbgstr_w(name));

    name[0] = 0;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, name, NULL);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(!lstrcmpW(name, unknownW), "got %s\n", wine_dbgstr_w(name));

    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, NULL, &len);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(len == 8, "got %u\n", len);

    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 0, NULL, &len);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(len == 8, "got %u\n", len);

    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 0, NULL, NULL);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 8, NULL, NULL);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = WICMapGuidToShortName(&GUID_NULL, 0, NULL, NULL);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#x\n", hr);

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 4, name, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "got %#x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(!lstrcmpW(name, unkW), "got %s\n", wine_dbgstr_w(name));

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapGuidToShortName(&GUID_MetadataFormatUnknown, 0, name, &len);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(!name[0], "got %s\n", wine_dbgstr_w(name));

    hr = WICMapGuidToShortName(NULL, 8, name, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);
}

static void test_WICMapShortNameToGuid(void)
{
    static const WCHAR unkW[] = { 'u','n','k',0 };
    static const WCHAR xmpW[] = { 'x','m','p',0 };
    static const WCHAR XmPW[] = { 'X','m','P',0 };
    static const WCHAR unknownW[] = { 'u','n','k','n','o','w','n',0 };
    HRESULT hr;
    GUID guid;

    hr = WICMapShortNameToGuid(NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    hr = WICMapShortNameToGuid(NULL, &guid);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    hr = WICMapShortNameToGuid(unknownW, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    hr = WICMapShortNameToGuid(unkW, &guid);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#x\n", hr);

    hr = WICMapShortNameToGuid(unknownW, &guid);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_MetadataFormatUnknown), "got %s\n", wine_dbgstr_guid(&guid));

    hr = WICMapShortNameToGuid(xmpW, &guid);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_MetadataFormatXMP), "got %s\n", wine_dbgstr_guid(&guid));

    guid = GUID_NULL;
    hr = WICMapShortNameToGuid(XmPW, &guid);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_MetadataFormatXMP), "got %s\n", wine_dbgstr_guid(&guid));
}

static const GUID *guid_list[] =
{
    &GUID_ContainerFormatBmp,
    &GUID_ContainerFormatPng,
    &GUID_ContainerFormatIco,
    &GUID_ContainerFormatJpeg,
    &GUID_ContainerFormatTiff,
    &GUID_ContainerFormatGif,
    &GUID_ContainerFormatWmp,
    &GUID_MetadataFormatUnknown,
    &GUID_MetadataFormatIfd,
    &GUID_MetadataFormatSubIfd,
    &GUID_MetadataFormatExif,
    &GUID_MetadataFormatGps,
    &GUID_MetadataFormatInterop,
    &GUID_MetadataFormatApp0,
    &GUID_MetadataFormatApp1,
    &GUID_MetadataFormatApp13,
    &GUID_MetadataFormatIPTC,
    &GUID_MetadataFormatIRB,
    &GUID_MetadataFormat8BIMIPTC,
    &GUID_MetadataFormat8BIMResolutionInfo,
    &GUID_MetadataFormat8BIMIPTCDigest,
    &GUID_MetadataFormatXMP,
    &GUID_MetadataFormatThumbnail,
    &GUID_MetadataFormatChunktEXt,
    &GUID_MetadataFormatXMPStruct,
    &GUID_MetadataFormatXMPBag,
    &GUID_MetadataFormatXMPSeq,
    &GUID_MetadataFormatXMPAlt,
    &GUID_MetadataFormatLSD,
    &GUID_MetadataFormatIMD,
    &GUID_MetadataFormatGCE,
    &GUID_MetadataFormatAPE,
    &GUID_MetadataFormatJpegChrominance,
    &GUID_MetadataFormatJpegLuminance,
    &GUID_MetadataFormatJpegComment,
    &GUID_MetadataFormatGifComment,
    &GUID_MetadataFormatChunkgAMA,
    &GUID_MetadataFormatChunkbKGD,
    &GUID_MetadataFormatChunkiTXt,
    &GUID_MetadataFormatChunkcHRM,
    &GUID_MetadataFormatChunkhIST,
    &GUID_MetadataFormatChunkiCCP,
    &GUID_MetadataFormatChunksRGB,
    &GUID_MetadataFormatChunktIME
};

static WCHAR rdf_scheme[] = { 'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g','/','1','9','9','9','/','0','2','/','2','2','-','r','d','f','-','s','y','n','t','a','x','-','n','s','#',0 };
static WCHAR dc_scheme[] = { 'h','t','t','p',':','/','/','p','u','r','l','.','o','r','g','/','d','c','/','e','l','e','m','e','n','t','s','/','1','.','1','/',0 };
static WCHAR xmp_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/',0 };
static WCHAR xmpidq_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','m','p','/','I','d','e','n','t','i','f','i','e','r','/','q','u','a','l','/','1','.','0','/',0 };
static WCHAR xmpRights_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','r','i','g','h','t','s','/',0 };
static WCHAR xmpMM_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','m','m','/',0 };
static WCHAR xmpBJ_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','b','j','/',0 };
static WCHAR xmpTPg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','t','/','p','g','/',0 };
static WCHAR pdf_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','p','d','f','/','1','.','3','/',0 };
static WCHAR photoshop_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','p','h','o','t','o','s','h','o','p','/','1','.','0','/',0 };
static WCHAR tiff_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','t','i','f','f','/','1','.','0','/',0 };
static WCHAR exif_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','e','x','i','f','/','1','.','0','/',0 };
static WCHAR stDim_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','D','i','m','e','n','s','i','o','n','s','#',0 };
static WCHAR xapGImg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','g','/','i','m','g','/',0 };
static WCHAR stEvt_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','R','e','s','o','u','r','c','e','E','v','e','n','t','#',0 };
static WCHAR stRef_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','R','e','s','o','u','r','c','e','R','e','f','#',0 };
static WCHAR stVer_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','V','e','r','s','i','o','n','#',0 };
static WCHAR stJob_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','J','o','b','#',0 };
static WCHAR aux_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','e','x','i','f','/','1','.','0','/','a','u','x','/',0 };
static WCHAR crs_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','c','a','m','e','r','a','-','r','a','w','-','s','e','t','t','i','n','g','s','/','1','.','0','/',0 };
static WCHAR xmpDM_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','m','p','/','1','.','0','/','D','y','n','a','m','i','c','M','e','d','i','a','/',0 };
static WCHAR Iptc4xmpCore_scheme[] = { 'h','t','t','p',':','/','/','i','p','t','c','.','o','r','g','/','s','t','d','/','I','p','t','c','4','x','m','p','C','o','r','e','/','1','.','0','/','x','m','l','n','s','/',0 };
static WCHAR MicrosoftPhoto_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','0','/',0 };
static WCHAR MP_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/',0 };
static WCHAR MPRI_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/','t','/','R','e','g','i','o','n','I','n','f','o','#',0 };
static WCHAR MPReg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/','t','/','R','e','g','i','o','n','#',0 };

static WCHAR *schema_list[] =
{
    aux_scheme,
    rdf_scheme,
    dc_scheme,
    xmp_scheme,
    xmpidq_scheme,
    xmpRights_scheme,
    xmpMM_scheme,
    xmpBJ_scheme,
    xmpTPg_scheme,
    pdf_scheme,
    photoshop_scheme,
    tiff_scheme,
    exif_scheme,
    stDim_scheme,
    xapGImg_scheme,
    stEvt_scheme,
    stRef_scheme,
    stVer_scheme,
    stJob_scheme,
    crs_scheme,
    xmpDM_scheme,
    Iptc4xmpCore_scheme,
    MicrosoftPhoto_scheme,
    MP_scheme,
    MPRI_scheme,
    MPReg_scheme
};

static void test_WICMapSchemaToName(void)
{
    static const WCHAR xmW[] = { 'x','m',0 };
    static const WCHAR xmpW[] = { 'x','m','p',0 };
    static WCHAR schemaW[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/',0 };
    static WCHAR SCHEMAW[] = { 'H','T','T','P',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/',0 };
    HRESULT hr;
    UINT len, i, j;
    WCHAR name[16];

    hr = WICMapSchemaToName(&GUID_MetadataFormatUnknown, NULL, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    hr = WICMapSchemaToName(&GUID_MetadataFormatUnknown, schemaW, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    hr = WICMapSchemaToName(&GUID_MetadataFormatUnknown, schemaW, 0, NULL, &len);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#x\n", hr);

    hr = WICMapSchemaToName(NULL, schemaW, 0, NULL, &len);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 0, NULL, &len);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(len == 4, "got %u\n", len);

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 4, NULL, &len);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(len == 4, "got %u\n", len);

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, SCHEMAW, 0, NULL, &len);
    ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "got %#x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 4, name, &len);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(len == 4, "got %u\n", len);
    ok(!lstrcmpW(name, xmpW), "got %s\n", wine_dbgstr_w(name));

    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 0, name, &len);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);

    name[0] = 0;
    len = 0xdeadbeef;
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 3, name, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "got %#x\n", hr);
    ok(len == 0xdeadbeef, "got %u\n", len);
    ok(!lstrcmpW(name, xmW), "got %s\n", wine_dbgstr_w(name));

    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schemaW, 4, name, NULL);
    ok(hr == E_INVALIDARG, "got %#x\n", hr);

    /* Check whether modern schemas are supported */
    hr = WICMapSchemaToName(&GUID_MetadataFormatXMP, schema_list[0], 0, NULL, &len);
    if (hr == WINCODEC_ERR_PROPERTYNOTFOUND)
    {
        win_skip("Modern schemas are not supported\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(guid_list); i++)
    {
        for (j = 0; j < ARRAY_SIZE(schema_list); j++)
        {
            hr = WICMapSchemaToName(guid_list[i], schema_list[j], 0, NULL, &len);
            if (IsEqualGUID(guid_list[i], &GUID_MetadataFormatXMP) ||
                IsEqualGUID(guid_list[i], &GUID_MetadataFormatXMPStruct))
            {
                ok(hr == S_OK, "%u: %u: format %s does not support schema %s\n",
                   i, j, wine_dbgstr_guid(guid_list[i]), wine_dbgstr_w(schema_list[j]));
            }
            else
            {
                ok(hr == WINCODEC_ERR_PROPERTYNOTFOUND, "%u: %u: format %s supports schema %s\n",
                   i, j, wine_dbgstr_guid(guid_list[i]), wine_dbgstr_w(schema_list[j]));
            }
        }
    }
}

struct metadata_item
{
    const char *schema, *id_str;
    UINT id, type, value;
};

struct metadata_block
{
    const GUID *metadata_format;
    UINT count;
    const struct metadata_item *item;
};

struct metadata
{
    const GUID *container_format;
    UINT count;
    const struct metadata_block *block;
};

static const struct metadata *current_metadata;
static const struct metadata_block *current_metadata_block;

static char the_best[] = "The Best";
static char the_worst[] = "The Worst";

static HRESULT WINAPI mdr_QueryInterface(IWICMetadataReader *iface, REFIID iid, void **out)
{
    trace("%p,%s,%p\n", iface, wine_dbgstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IWICMetadataReader))
    {
        *out = iface;
        return S_OK;
    }

    ok(0, "unknown iid %s\n", wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mdr_AddRef(IWICMetadataReader *iface)
{
    return 2;
}

static ULONG WINAPI mdr_Release(IWICMetadataReader *iface)
{
    return 1;
}

static HRESULT WINAPI mdr_GetMetadataFormat(IWICMetadataReader *iface, GUID *format)
{
    trace("%p,%p\n", iface, format);

    ok(current_metadata_block != NULL, "current_metadata_block can't be NULL\n");
    if (!current_metadata_block) return E_POINTER;

    *format = *current_metadata_block->metadata_format;
    return S_OK;
}

static HRESULT WINAPI mdr_GetMetadataHandlerInfo(IWICMetadataReader *iface, IWICMetadataHandlerInfo **handler)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI mdr_GetCount(IWICMetadataReader *iface, UINT *count)
{
    trace("%p,%p\n", iface, count);

    ok(current_metadata_block != NULL, "current_metadata_block can't be NULL\n");
    if (!current_metadata_block) return E_POINTER;

    *count = current_metadata_block->count;
    return S_OK;
}

static HRESULT WINAPI mdr_GetValueByIndex(IWICMetadataReader *iface, UINT index, PROPVARIANT *schema, PROPVARIANT *id, PROPVARIANT *value)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static char *get_temp_buffer(int size)
{
    static char buf[16][256];
    static int idx;
    char *p;

    assert(size < 256);

    p = buf[idx & 0x0f];
    idx++;
    return p;
}

static const char *wine_dbgstr_propvariant(const PROPVARIANT *var)
{
    char *ret;

    if (!var) return "(null)";

    switch (var->vt)
    {
    case VT_LPWSTR:
        ret = get_temp_buffer(lstrlenW(U(*var).pwszVal) + 16);
        sprintf(ret, "(VT_LPWSTR:%s)", wine_dbgstr_w(U(*var).pwszVal));
        break;

    case VT_LPSTR:
        ret = get_temp_buffer(lstrlenA(U(*var).pszVal) + 16);
        sprintf(ret, "(VT_LPSTR:%s)", U(*var).pszVal);
        break;

    default:
        ret = get_temp_buffer(16);
        sprintf(ret, "(vt:%u)", var->vt);
        break;
    }

    return ret;
}

static int propvar_cmp(const PROPVARIANT *v1, LONGLONG value2)
{
    LONGLONG value1;

    if (PropVariantToInt64(v1, &value1) != S_OK) return -1;

    value1 -= value2;
    if (value1) return value1 < 0 ? -1 : 1;
    return 0;
}

static HRESULT WINAPI mdr_GetValue(IWICMetadataReader *iface, const PROPVARIANT *schema, const PROPVARIANT *id, PROPVARIANT *value)
{
    UINT i;

    trace("%p,%s,%s,%s\n", iface, wine_dbgstr_propvariant(schema), wine_dbgstr_propvariant(id), wine_dbgstr_propvariant(value));

    ok(current_metadata_block != NULL, "current_metadata_block can't be NULL\n");
    if (!current_metadata_block) return E_POINTER;

    ok(schema != NULL && id != NULL && value != NULL, "%p, %p, %p should not be NULL\n", schema, id, value);

    for (i = 0; i < current_metadata_block->count; i++)
    {
        if (schema->vt != VT_EMPTY)
        {
            if (!current_metadata_block->item[i].schema)
                continue;

            switch (schema->vt)
            {
            case VT_LPSTR:
                if (lstrcmpA(U(*schema).pszVal, current_metadata_block->item[i].schema) != 0)
                    continue;
                break;

            case VT_LPWSTR:
            {
                char schemaA[256];
                WideCharToMultiByte(CP_ACP, 0, U(*schema).pwszVal, -1, schemaA, sizeof(schemaA), NULL, NULL);
                if (lstrcmpA(schemaA, current_metadata_block->item[i].schema) != 0)
                    continue;
                break;
            }

            default:
                ok(0, "unsupported schema vt %u\n", schema->vt);
                continue;
            }
        }
        else if (current_metadata_block->item[i].schema)
            continue;

        switch (id->vt)
        {
        case VT_LPSTR:
            if (current_metadata_block->item[i].id_str)
            {
                if (!lstrcmpA(U(*id).pszVal, current_metadata_block->item[i].id_str))
                {
                    value->vt = VT_LPSTR;
                    U(*value).pszVal = the_best;
                    return S_OK;
                }
                break;
            }
            break;

        case VT_LPWSTR:
            if (current_metadata_block->item[i].id_str)
            {
                char idA[256];
                WideCharToMultiByte(CP_ACP, 0, U(*id).pwszVal, -1, idA, sizeof(idA), NULL, NULL);
                if (!lstrcmpA(idA, current_metadata_block->item[i].id_str))
                {
                    value->vt = VT_LPSTR;
                    U(*value).pszVal = the_worst;
                    return S_OK;
                }
                break;
            }
            break;

        case VT_CLSID:
            if (IsEqualGUID(U(*id).puuid, &GUID_MetadataFormatXMP) ||
                IsEqualGUID(U(*id).puuid, &GUID_ContainerFormatTiff))
            {
                value->vt = VT_UNKNOWN;
                value->punkVal = (IUnknown *)iface;
                return S_OK;
            }
            break;

        default:
            if (!propvar_cmp(id, current_metadata_block->item[i].id))
            {
                value->vt = current_metadata_block->item[i].type;
                U(*value).uiVal = current_metadata_block->item[i].value;
                return S_OK;
            }
            break;
        }
    }

    return 0xdeadbeef;
}

static HRESULT WINAPI mdr_GetEnumerator(IWICMetadataReader *iface, IWICEnumMetadataItem **enumerator)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static const IWICMetadataReaderVtbl mdr_vtbl =
{
    mdr_QueryInterface,
    mdr_AddRef,
    mdr_Release,
    mdr_GetMetadataFormat,
    mdr_GetMetadataHandlerInfo,
    mdr_GetCount,
    mdr_GetValueByIndex,
    mdr_GetValue,
    mdr_GetEnumerator
};

static IWICMetadataReader mdr = { &mdr_vtbl };

static HRESULT WINAPI mdbr_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid, void **out)
{
    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IWICMetadataBlockReader))
    {
        *out = iface;
        return S_OK;
    }

    /* Windows 8/10 query for some undocumented IID */
    if (!IsEqualIID(iid, &IID_MdbrUnknown))
        ok(0, "unknown iid %s\n", wine_dbgstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mdbr_AddRef(IWICMetadataBlockReader *iface)
{
    return 2;
}

static ULONG WINAPI mdbr_Release(IWICMetadataBlockReader *iface)
{
    return 1;
}

static HRESULT WINAPI mdbr_GetContainerFormat(IWICMetadataBlockReader *iface, GUID *format)
{
    trace("%p,%p\n", iface, format);

    ok(current_metadata != NULL, "current_metadata can't be NULL\n");
    if (!current_metadata) return E_POINTER;

    *format = *current_metadata->container_format;
    return S_OK;
}

static HRESULT WINAPI mdbr_GetCount(IWICMetadataBlockReader *iface, UINT *count)
{
    trace("%p,%p\n", iface, count);

    ok(current_metadata != NULL, "current_metadata can't be NULL\n");
    if (!current_metadata) return E_POINTER;

    *count = current_metadata->count;
    return S_OK;
}

static HRESULT WINAPI mdbr_GetReaderByIndex(IWICMetadataBlockReader *iface, UINT index, IWICMetadataReader **out)
{
    trace("%p,%u,%p\n", iface, index, out);

    *out = NULL;

    ok(current_metadata != NULL, "current_metadata can't be NULL\n");
    if (!current_metadata) return E_POINTER;

    if (index < current_metadata->count)
    {
        current_metadata_block = &current_metadata->block[index];
        *out = &mdr;
        return S_OK;
    }

    current_metadata_block = NULL;
    return E_INVALIDARG;
}

static HRESULT WINAPI mdbr_GetEnumerator(IWICMetadataBlockReader *iface, IEnumUnknown **enumerator)
{
    ok(0, "not implemented\n");
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl mdbr_vtbl =
{
    mdbr_QueryInterface,
    mdbr_AddRef,
    mdbr_Release,
    mdbr_GetContainerFormat,
    mdbr_GetCount,
    mdbr_GetReaderByIndex,
    mdbr_GetEnumerator
};

static IWICMetadataBlockReader mdbr = { &mdbr_vtbl };

static const char xmp[] = "http://ns.adobe.com/xap/1.0/";
static const char dc[] = "http://purl.org/dc/elements/1.1/";
static const char tiff[] = "http://ns.adobe.com/tiff/1.0/";

static const struct metadata_item item1[] =
{
    { NULL, NULL, 1, 2, 3 }
};

static const struct metadata_item item2[] =
{
    { NULL, NULL, 1, 2, 3 },
    { "xmp", "Rating", 4, 5, 6 },
    { NULL, "Rating", 7, 8, 9 }
};

static const struct metadata_item item3[] =
{
    { NULL, NULL, 1, 2, 3 },
    { NULL, NULL, 4, 5, 6 },
    { NULL, NULL, 7, 8, 9 },
    { NULL, NULL, 10, 11, 12 }
};

static const struct metadata_item item4[] =
{
    { NULL, NULL, 1, 2, 3 },
    { xmp, "Rating", 4, 5, 6 },
    { dc, NULL, 7, 8, 9 },
    { tiff, NULL, 10, 11, 12 },
    { NULL, "RATING", 13, 14, 15 },
    { NULL, "R}ATING", 16, 17, 18 },
    { NULL, "xmp", 19, 20, 21 }
};

static const struct metadata_block block1[] =
{
    { &GUID_MetadataFormatIfd, 1, item1 }
};

static const struct metadata_block block2[] =
{
    { &GUID_MetadataFormatXMP, 1, item1 },
    { &GUID_MetadataFormatIfd, 3, item2 }
};

static const struct metadata_block block3[] =
{
    { &GUID_MetadataFormatXMP, 1, item1 },
    { &GUID_MetadataFormatIfd, 3, item2 },
    { &GUID_MetadataFormatXMP, 4, item3 },
    { &GUID_MetadataFormatXMP, 7, item4 },
    { &GUID_MetadataFormatIfd, 7, item4 }
};

static const struct metadata data1 =
{
    &GUID_ContainerFormatGif,
    1, block1
};

static const struct metadata data2 =
{
    &GUID_ContainerFormatTiff,
    2, block2
};

static const struct metadata data3 =
{
    &GUID_ContainerFormatPng,
    5, block3
};

static void test_queryreader(void)
{
    static const char q1[] = "/ifd/{uchar=1}";
    static const char q2[] = "/ifd/xmp:{long=4}";
    static const char q3[] = "/ifd/{str=xmp}:{uint=4}";
    static const char q4[] = "/xmp/{char=7}";
    static const char q5[] = "/[1]xmp/{short=7}";
    static const char q6[] = "/[1]ifd/{str=dc}:{uint=7}";
    static const char q7[] = "/[1]ifd/{str=http://purl.org/dc/elements/1.1/}:{longlong=7}";
    static const char q8[] = "/[1]ifd/{str=http://ns.adobe.com/tiff/1.0/}:{int=10}";
    static const char q9[] = "/[2]xmp/xmp:{ulong=4}";
    static const char q10[] = "/[2]xmp/{str=xmp}:{ulong=4}";
    static const char q11[] = "/xmp";
    static const char q12[] = "/ifd/xmp";
    static const char q13[] = "/ifd/xmp/tiff";
    static const char q14[] = "/[0]ifd/[0]xmp/[0]tiff";
    static const char q15[] = "/[*]xmp";

    static const char q20[] = "/ifd/\\Rating";
    static const char q21[] = "/[0]ifd/Rating";
    static const char q22[] = "/[2]xmp/xmp:{str=Rating}";
    static const char q23[] = "/[2]xmp/xmp:Rating";

    static const char q24[] = "/[1]ifd/{str=http://ns.adobe.com/xap/1.0/}:Rating";
    static const char q25[] = "/[1]ifd/{str=http://ns.adobe.com/xap/1.0/}:{str=Rating}";
    static const char q26[] = "/[1]ifd/{wstr=\\RATING}";
    static const char q27[] = "/[1]ifd/{str=R\\ATING}";
    static const char q28[] = "/[1]ifd/{str=R\\}ATING}";

    static const char q40[] = "[0]/ifd/Rating";
    static const char q41[] = "/[+1]ifd/Rating";
    static const char q42[] = "/[-1]ifd/Rating";
    static const char q43[] = "/ifd/{\\str=Rating}";
    static const char q44[] = "/ifd/{badtype=0}";
    static const char q45[] = "/ifd/{uint=0x1234}";
    static const char q46[] = "/ifd/[0]Rating";
    static const char q47[] = "/ifd/[*]Rating";
    static const struct
    {
        BOOL todo;
        const struct metadata *data;
        const char *query;
        HRESULT hr;
        UINT vt, value;
        const char *str_value;
    } test_data[] =
    {
        { FALSE, &data1, q1, S_OK, 2, 3, NULL },
        { FALSE, &data2, q2, S_OK, 5, 6, NULL },
        { FALSE, &data2, q3, S_OK, 5, 6, NULL },
        { FALSE, &data3, q4, 0xdeadbeef },
        { FALSE, &data3, q5, S_OK, 8, 9, NULL },
        { FALSE, &data3, q6, 0xdeadbeef },
        { FALSE, &data3, q7, S_OK, 8, 9, NULL },
        { FALSE, &data3, q8, S_OK, 11, 12, NULL },
        { FALSE, &data3, q9, S_OK, 5, 6, NULL },
        { FALSE, &data3, q10, 0xdeadbeef },

        { FALSE, &data3, q11, S_OK, VT_UNKNOWN, 0, NULL },
        { FALSE, &data3, q12, S_OK, VT_UNKNOWN, 0, NULL },
        { FALSE, &data3, q13, S_OK, VT_UNKNOWN, 0, NULL },
        { FALSE, &data3, q14, S_OK, VT_UNKNOWN, 0, NULL },
        { TRUE, &data3, q15, S_OK, VT_LPSTR, 0, the_worst },

        { FALSE, &data3, q20, S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, q21, S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, q22, S_OK, VT_LPSTR, 0, the_best },
        { FALSE, &data3, q23, S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, q24, S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, q25, S_OK, VT_LPSTR, 0, the_best },
        { FALSE, &data3, q26, S_OK, VT_LPSTR, 0, the_worst },
        { FALSE, &data3, q27, S_OK, VT_LPSTR, 0, the_best },
        { FALSE, &data3, q28, S_OK, VT_LPSTR, 0, the_best },

        { FALSE, &data1, q40, WINCODEC_ERR_PROPERTYNOTSUPPORTED },
        { TRUE, &data1, q41, WINCODEC_ERR_INVALIDQUERYCHARACTER },
        { TRUE, &data1, q42, WINCODEC_ERR_INVALIDQUERYCHARACTER },
        { FALSE, &data1, q43, WINCODEC_ERR_WRONGSTATE },
        { FALSE, &data1, q44, WINCODEC_ERR_WRONGSTATE },
        { TRUE, &data1, q45, DISP_E_TYPEMISMATCH },
        { TRUE, &data1, q46, E_INVALIDARG },
        { TRUE, &data1, q47, WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT },
    };
    WCHAR queryW[256];
    HRESULT hr;
    IWICComponentFactory *factory;
    IWICMetadataQueryReader *reader;
    GUID format;
    PROPVARIANT value;
    UINT i;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICComponentFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);

    hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, &mdbr, &reader);
    ok(hr == S_OK, "CreateQueryReaderFromBlockReader error %#x\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        current_metadata = test_data[i].data;

        hr = IWICMetadataQueryReader_GetContainerFormat(reader, &format);
        ok(hr == S_OK, "%u: GetContainerFormat error %#x\n", i, hr);
        ok(IsEqualGUID(&format, test_data[i].data->container_format), "%u: expected %s, got %s\n",
           i, wine_dbgstr_guid(test_data[i].data->container_format), wine_dbgstr_guid(&format));

        MultiByteToWideChar(CP_ACP, 0, test_data[i].query, -1, queryW, 256);
        PropVariantInit(&value);
        hr = IWICMetadataQueryReader_GetMetadataByName(reader, queryW, &value);
        todo_wine_if(test_data[i].todo)
        ok(hr == test_data[i].hr, "%u: expected %#x, got %#x\n", i, test_data[i].hr, hr);
        if (hr == S_OK)
        {
            ok(value.vt == test_data[i].vt, "%u: expected %u, got %u\n", i, test_data[i].vt, value.vt);
            if (test_data[i].vt == value.vt)
            {
                if (value.vt == VT_UNKNOWN)
                {
                    IWICMetadataQueryReader *new_reader;
                    WCHAR location[256];
                    UINT len;

                    hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataQueryReader, (void **)&new_reader);
                    ok(hr == S_OK, "QueryInterface error %#x\n", hr);

                    location[0] = 0;
                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 256, location, &len);
                    ok(hr == S_OK, "GetLocation error %#x\n", hr);
                    ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);
                    ok(!lstrcmpW(location, queryW), "expected %s, got %s\n", wine_dbgstr_w(queryW), wine_dbgstr_w(location));

                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 256, location, NULL);
                    ok(hr == E_INVALIDARG, "got %#x\n", hr);

                    location[0] = 0;
                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 3, location, &len);
                    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "got %#x\n", hr);
                    ok(len == 0xdeadbeef, "got %u\n", len);
                    ok(!location[0], "got %s\n", wine_dbgstr_w(location));

                    location[0] = 0;
                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 0, location, &len);
                    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "got %#x\n", hr);
                    ok(len == 0xdeadbeef, "got %u\n", len);
                    ok(!location[0], "got %s\n", wine_dbgstr_w(location));

                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 0, NULL, &len);
                    ok(hr == S_OK, "GetLocation error %#x\n", hr);
                    ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);

                    len = 0xdeadbeef;
                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 3, NULL, &len);
                    ok(hr == S_OK, "GetLocation error %#x\n", hr);
                    ok(len == lstrlenW(queryW) + 1, "expected %u, got %u\n", lstrlenW(queryW) + 1, len);

                    hr = IWICMetadataQueryReader_GetLocation(new_reader, 0, NULL, NULL);
                    ok(hr == E_INVALIDARG, "got %#x\n", hr);

                    IWICMetadataQueryReader_Release(new_reader);
                    PropVariantClear(&value);
                }
                else if (value.vt == VT_LPSTR)
                    ok(!lstrcmpA(U(value).pszVal, test_data[i].str_value), "%u: expected %s, got %s\n",
                       i, test_data[i].str_value, U(value).pszVal);
                else
                    ok(U(value).uiVal == test_data[i].value, "%u: expected %u, got %u\n",
                       i, test_data[i].value, U(value).uiVal);
            }

            /*
             * Do NOT call PropVariantClear(&value) for fake value types.
             */
        }
    }

    IWICMetadataQueryReader_Release(reader);
    IWICComponentFactory_Release(factory);
}

START_TEST(metadata)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_queryreader();
    test_WICMapGuidToShortName();
    test_WICMapShortNameToGuid();
    test_WICMapSchemaToName();
    test_metadata_unknown();
    test_metadata_tEXt();
    test_metadata_gAMA();
    test_metadata_cHRM();
    test_metadata_IFD();
    test_metadata_Exif();
    test_create_reader();
    test_metadata_png();
    test_metadata_gif();
    test_metadata_LSD();
    test_metadata_IMD();
    test_metadata_GCE();
    test_metadata_APE();
    test_metadata_GIF_comment();

    CoUninitialize();
}
