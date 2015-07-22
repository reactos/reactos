/*
 * Copyright 2011 Vincent Povirk for CodeWeavers
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

#include <stdio.h>
//#include <stdarg.h>
//#include <math.h>
#include <assert.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
//#include "wincodec.h"
#include <wincodecsdk.h>
#include <wine/test.h>

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

    load_stream((IUnknown*)reader, metadata_unknown, sizeof(metadata_unknown), WICPersistOptionsDefault);

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

    load_stream((IUnknown*)reader, metadata_tEXt, sizeof(metadata_tEXt), WICPersistOptionsDefault);

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

    load_stream((IUnknown*)reader, metadata_gAMA, sizeof(metadata_gAMA), WICPersistOptionsDefault);

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
    DWORD persist_options = WICPersistOptionsBigEndian;
#else
    DWORD persist_options = WICPersistOptionsLittleEndian;
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
    ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);

    compare_metadata(reader, td, count);

    /* test IFD data with different endianness */
    if (persist_options == WICPersistOptionsLittleEndian)
        persist_options = WICPersistOptionsBigEndian;
    else
        persist_options = WICPersistOptionsLittleEndian;

    IFD_data_swapped = HeapAlloc(GetProcessHeap(), 0, sizeof(IFD_data));
    memcpy(IFD_data_swapped, &IFD_data, sizeof(IFD_data));
    byte_swap_ifd_data(IFD_data_swapped);
    load_stream((IUnknown *)reader, IFD_data_swapped, sizeof(IFD_data), persist_options);
    hr = IWICMetadataReader_GetCount(reader, &count);
    ok(hr == S_OK, "GetCount error %#x\n", hr);
    ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);
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
        NULL, NULL, WICPersistOptionsDefault,
        stream, &reader);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionsDefault,
        NULL, &reader);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionsDefault,
        stream, NULL);
    ok(hr == E_INVALIDARG, "CreateMetadataReaderFromContainer failed, hr=%x\n", hr);

    hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
        &GUID_ContainerFormatPng, NULL, WICPersistOptionsDefault,
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
        &GUID_ContainerFormatWmp, NULL, WICPersistOptionsDefault,
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

        IWICMetadataBlockReader_Release(blockreader);
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
            ok(count == sizeof(gif_LSD)/sizeof(gif_LSD[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(gif_IMD)/sizeof(gif_IMD[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_LSD)/sizeof(animated_gif_LSD[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_APE)/sizeof(animated_gif_APE[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_comment_1)/sizeof(animated_gif_comment_1[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_plain_1)/sizeof(animated_gif_plain_1[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_IMD)/sizeof(animated_gif_IMD[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_comment_2)/sizeof(animated_gif_comment_2[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_plain_2)/sizeof(animated_gif_plain_2[0]), "unexpected count %u\n", count);

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
            ok(count == sizeof(animated_gif_GCE)/sizeof(animated_gif_GCE[0]), "unexpected count %u\n", count);

            compare_metadata(reader, animated_gif_GCE, count);

            IWICMetadataReader_Release(reader);
        }

        hr = IWICMetadataBlockReader_GetReaderByIndex(blockreader, 4, &reader);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#x\n", hr);

        IWICMetadataBlockReader_Release(blockreader);
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
        ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);

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
        ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);

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
        ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);

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
        ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);

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
        ok(count == sizeof(td)/sizeof(td[0]), "unexpected count %u\n", count);

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

START_TEST(metadata)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_metadata_unknown();
    test_metadata_tEXt();
    test_metadata_gAMA();
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
