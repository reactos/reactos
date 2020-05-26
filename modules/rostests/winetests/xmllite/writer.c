/*
 * XMLLite IXmlWriter tests
 *
 * Copyright 2011 (C) Alistair Leslie-Hughes
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

#ifdef __REACTOS__
#define CONST_VTABLE
#endif

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "xmllite.h"

#include "wine/heap.h"
#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(IID_IXmlWriterOutput, 0xc1131708, 0x0f59, 0x477f, 0x93, 0x59, 0x7d, 0x33, 0x24, 0x51, 0xbc, 0x1a);

static const WCHAR aW[] = {'a',0};

#define EXPECT_REF(obj, ref) _expect_ref((IUnknown *)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == ref, "expected refcount %d, got %d\n", ref, refcount);
}

static void check_output_raw(IStream *stream, const void *expected, SIZE_T size, int line)
{
    SIZE_T content_size;
    HGLOBAL hglobal;
    HRESULT hr;
    WCHAR *ptr;

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get the stream handle, hr %#x.\n", hr);

    content_size = GlobalSize(hglobal);
    ok_(__FILE__, line)(size == content_size, "Unexpected test output size %ld.\n", content_size);
    ptr = GlobalLock(hglobal);
    if (size <= content_size)
        ok_(__FILE__, line)(!memcmp(expected, ptr, size), "Unexpected output content.\n");
    if (size != content_size && *ptr == 0xfeff)
        ok_(__FILE__, line)(0, "Content: %s.\n", wine_dbgstr_wn(ptr, content_size / sizeof(WCHAR)));

    GlobalUnlock(hglobal);
}

static void check_output(IStream *stream, const char *expected, BOOL todo, int line)
{
    int len = strlen(expected), size;
    HGLOBAL hglobal;
    HRESULT hr;
    char *ptr;

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_(__FILE__, line)(hr == S_OK, "got 0x%08x\n", hr);

    size = GlobalSize(hglobal);
    ptr = GlobalLock(hglobal);
    todo_wine_if(todo)
    {
        if (size != len)
        {
            ok_(__FILE__, line)(0, "data size mismatch, expected %u, got %u\n", len, size);
            ok_(__FILE__, line)(0, "got |%s|, expected |%s|\n", ptr, expected);
        }
        else
            ok_(__FILE__, line)(!strncmp(ptr, expected, len), "got |%s|, expected |%s|\n", ptr, expected);
    }
    GlobalUnlock(hglobal);
}
#define CHECK_OUTPUT(stream, expected) check_output(stream, expected, FALSE, __LINE__)
#define CHECK_OUTPUT_TODO(stream, expected) check_output(stream, expected, TRUE, __LINE__)
#define CHECK_OUTPUT_RAW(stream, expected, size) check_output_raw(stream, expected, size, __LINE__)

static WCHAR *strdupAtoW(const char *str)
{
    WCHAR *ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = heap_alloc(len * sizeof(WCHAR));
    if (ret)
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static void writer_set_property(IXmlWriter *writer, XmlWriterProperty property)
{
    HRESULT hr;

    hr = IXmlWriter_SetProperty(writer, property, TRUE);
    ok(hr == S_OK, "Failed to set writer property, hr %#x.\n", hr);
}

/* used to test all Write* methods for consistent error state */
static void check_writer_state(IXmlWriter *writer, HRESULT exp_hr)
{
    static const WCHAR aW[] = {'a',0};
    HRESULT hr;

    /* FIXME: add WriteAttributes */

    hr = IXmlWriter_WriteAttributeString(writer, NULL, aW, NULL, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteCData(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteCharEntity(writer, aW[0]);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteChars(writer, aW, 1);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteComment(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteDocType(writer, aW, NULL, NULL, NULL);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, aW, NULL, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteEntityRef(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteName(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteNmToken(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    /* FIXME: add WriteNode */
    /* FIXME: add WriteNodeShallow */

    hr = IXmlWriter_WriteProcessingInstruction(writer, aW, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteQualifiedName(writer, aW, NULL);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteRaw(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteRawChars(writer, aW, 1);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    hr = IXmlWriter_WriteString(writer, aW);
    ok(hr == exp_hr, "got 0x%08x, expected 0x%08x\n", hr, exp_hr);

    /* FIXME: add WriteSurrogateCharEntity */
    /* FIXME: add WriteWhitespace */
}

static IStream *writer_set_output(IXmlWriter *writer)
{
    IStream *stream;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, (IUnknown*)stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    return stream;
}

static HRESULT WINAPI testoutput_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)) {
        *obj = iface;
        return S_OK;
    }
    else {
        ok(0, "unknown riid=%s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI testoutput_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI testoutput_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl testoutputvtbl = {
    testoutput_QueryInterface,
    testoutput_AddRef,
    testoutput_Release
};

static IUnknown testoutput = { &testoutputvtbl };

static HRESULT WINAPI teststream_QueryInterface(ISequentialStream *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ISequentialStream))
    {
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI teststream_AddRef(ISequentialStream *iface)
{
    return 2;
}

static ULONG WINAPI teststream_Release(ISequentialStream *iface)
{
    return 1;
}

static HRESULT WINAPI teststream_Read(ISequentialStream *iface, void *pv, ULONG cb, ULONG *pread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG g_write_len;
static HRESULT WINAPI teststream_Write(ISequentialStream *iface, const void *pv, ULONG cb, ULONG *written)
{
    g_write_len = cb;
    *written = cb;
    return S_OK;
}

static const ISequentialStreamVtbl teststreamvtbl =
{
    teststream_QueryInterface,
    teststream_AddRef,
    teststream_Release,
    teststream_Read,
    teststream_Write
};

static ISequentialStream teststream = { &teststreamvtbl };

static void test_writer_create(void)
{
    HRESULT hr;
    IXmlWriter *writer;
    LONG_PTR value;
    IUnknown *unk;

    /* crashes native */
    if (0)
    {
        CreateXmlWriter(&IID_IXmlWriter, NULL, NULL);
        CreateXmlWriter(NULL, (void**)&writer, NULL);
    }

    hr = CreateXmlWriter(&IID_IStream, (void **)&unk, NULL);
    ok(hr == E_NOINTERFACE, "got %08x\n", hr);

    hr = CreateXmlWriter(&IID_IUnknown, (void **)&unk, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IXmlWriter, (void **)&writer);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(unk == (IUnknown *)writer, "unexpected interface pointer\n");
    IUnknown_Release(unk);
    IXmlWriter_Release(writer);

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* check default properties values */
    value = 0;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_ByteOrderMark, &value);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(value == TRUE, "got %ld\n", value);

    value = TRUE;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_Indent, &value);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(value == FALSE, "got %ld\n", value);

    value = TRUE;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_OmitXmlDeclaration, &value);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(value == FALSE, "got %ld\n", value);

    value = XmlConformanceLevel_Auto;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_ConformanceLevel, &value);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(value == XmlConformanceLevel_Document, "got %ld\n", value);

    IXmlWriter_Release(writer);
}

static void test_invalid_output_encoding(IXmlWriter *writer, IUnknown *output)
{
    HRESULT hr;

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Failed to set output, hr %#x.\n", hr);

    /* TODO: WriteAttributes */

    hr = IXmlWriter_WriteAttributeString(writer, NULL, aW, NULL, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteCData(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteCharEntity(writer, 0x100);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteChars(writer, aW, 1);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteComment(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteDocType(writer, aW, NULL, NULL, NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, aW, NULL, NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteEntityRef(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteName(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteNmToken(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    /* TODO: WriteNode */
    /* TODO: WriteNodeShallow */

    hr = IXmlWriter_WriteProcessingInstruction(writer, aW, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteQualifiedName(writer, aW, NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, aW, 1);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteString(writer, aW);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#x.\n", hr);

    /* TODO: WriteSurrogateCharEntity */
    /* ًُُTODO: WriteWhitespace */

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);
}

static void test_writeroutput(void)
{
    static const WCHAR utf16W[] = {'u','t','f','-','1','6',0};
    static const WCHAR usasciiW[] = {'u','s','-','a','s','c','i','i',0};
    static const WCHAR dummyW[] = {'d','u','m','m','y',0};
    static const WCHAR utf16_outputW[] = {0xfeff,'<','a'};
    IXmlWriterOutput *output;
    IXmlWriter *writer;
    IStream *stream;
    IUnknown *unk;
    HRESULT hr;

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName(&testoutput, NULL, NULL, &output);
    ok(hr == S_OK, "got %08x\n", hr);
    EXPECT_REF(output, 1);
    IUnknown_Release(output);

    hr = CreateXmlWriterOutputWithEncodingName(&testoutput, NULL, utf16W, &output);
    ok(hr == S_OK, "got %08x\n", hr);
    unk = NULL;
    hr = IUnknown_QueryInterface(output, &IID_IXmlWriterOutput, (void**)&unk);
    ok(hr == S_OK, "got %08x\n", hr);
todo_wine
    ok(unk != NULL && unk != output, "got %p, output %p\n", unk, output);
    EXPECT_REF(output, 2);
    /* releasing 'unk' crashes on native */
    IUnknown_Release(output);
    EXPECT_REF(output, 1);
    IUnknown_Release(output);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingCodePage(&testoutput, NULL, ~0u, &output);
    ok(hr == S_OK, "got %08x\n", hr);
    IUnknown_Release(output);

    hr = CreateXmlWriterOutputWithEncodingCodePage(&testoutput, NULL, CP_UTF8, &output);
    ok(hr == S_OK, "got %08x\n", hr);
    unk = NULL;
    hr = IUnknown_QueryInterface(output, &IID_IXmlWriterOutput, (void**)&unk);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(unk != NULL, "got %p\n", unk);
    /* releasing 'unk' crashes on native */
    IUnknown_Release(output);
    IUnknown_Release(output);

    /* create with us-ascii */
    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName(&testoutput, NULL, usasciiW, &output);
    ok(hr == S_OK, "got %08x\n", hr);
    IUnknown_Release(output);

    /* Output with codepage 1200. */
    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Failed to create writer, hr %#x.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create stream, hr %#x.\n", hr);

    hr = CreateXmlWriterOutputWithEncodingCodePage((IUnknown *)stream, NULL, 1200, &output);
    ok(hr == S_OK, "Failed to create writer output, hr %#x.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Failed to set writer output, hr %#x.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "Write failed, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT_RAW(stream, utf16_outputW, sizeof(utf16_outputW));

    IStream_Release(stream);
    IUnknown_Release(output);

    /* Create output with meaningless code page value. */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create stream, hr %#x.\n", hr);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingCodePage((IUnknown *)stream, NULL, ~0u, &output);
    ok(hr == S_OK, "Failed to create writer output, hr %#x.\n", hr);

    test_invalid_output_encoding(writer, output);
    CHECK_OUTPUT(stream, "");

    IStream_Release(stream);
    IUnknown_Release(output);

    /* Same, with invalid encoding name. */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName((IUnknown *)stream, NULL, dummyW, &output);
    ok(hr == S_OK, "got %08x\n", hr);

    test_invalid_output_encoding(writer, output);
    CHECK_OUTPUT(stream, "");

    IStream_Release(stream);
    IUnknown_Release(output);

    IXmlWriter_Release(writer);
}

static void test_writestartdocument(void)
{
    static const char fullprolog[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
    static const char *prologversion2 = "<?xml version=\"1.0\" encoding=\"uS-asCii\"?>";
    static const char prologversion[] = "<?xml version=\"1.0\"?>";
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n','=','"','1','.','0','"',0};
    static const WCHAR usasciiW[] = {'u','S','-','a','s','C','i','i',0};
    static const WCHAR xmlW[] = {'x','m','l',0};
    IXmlWriterOutput *output;
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* output not set */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, xmlW, versionW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    /* nothing written yet */
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, fullprolog);

    /* one more time */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);
    IStream_Release(stream);

    /* now add PI manually, and try to start a document */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteProcessingInstruction(writer, xmlW, versionW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    /* another attempt to add 'xml' PI */
    hr = IXmlWriter_WriteProcessingInstruction(writer, xmlW, versionW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    IStream_Release(stream);
    IXmlWriter_Release(writer);

    /* create with us-ascii */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName((IUnknown *)stream, NULL, usasciiW, &output);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, prologversion2);

    IStream_Release(stream);
    IXmlWriter_Release(writer);
    IUnknown_Release(output);
}

static void test_flush(void)
{
    IXmlWriter *writer;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, (IUnknown*)&teststream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    g_write_len = 0;
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_write_len > 0, "got %d\n", g_write_len);

    g_write_len = 1;
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(g_write_len == 0, "got %d\n", g_write_len);

    /* Release() flushes too */
    g_write_len = 1;
    IXmlWriter_Release(writer);
    ok(g_write_len == 0, "got %d\n", g_write_len);
}

static void test_omitxmldeclaration(void)
{
    static const char prologversion[] = "<?xml version=\"1.0\"?>";
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n','=','"','1','.','0','"',0};
    static const WCHAR xmlW[] = {'x','m','l',0};
    IXmlWriter *writer;
    HGLOBAL hglobal;
    IStream *stream;
    HRESULT hr;
    char *ptr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ptr = GlobalLock(hglobal);
    ok(!ptr, "got %p\n", ptr);
    GlobalUnlock(hglobal);

    /* one more time */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    IStream_Release(stream);

    /* now add PI manually, and try to start a document */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteProcessingInstruction(writer, xmlW, versionW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    /* another attempt to add 'xml' PI */
    hr = IXmlWriter_WriteProcessingInstruction(writer, xmlW, versionW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IStream_Release(stream);
    IXmlWriter_Release(writer);
}

static void test_bom(void)
{
    static const WCHAR piW[] = {0xfeff,'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"','1','.','0','"','?','>'};
    static const WCHAR aopenW[] = {0xfeff,'<','a'};
    static const WCHAR afullW[] = {0xfeff,'<','a',' ','/','>'};
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n','=','"','1','.','0','"',0};
    static const WCHAR utf16W[] = {'u','t','f','-','1','6',0};
    static const WCHAR xmlW[] = {'x','m','l',0};
    static const WCHAR bomW[] = {0xfeff};
    IXmlWriterOutput *output;
    IXmlWriter *writer;
    IStream *stream;
    HGLOBAL hglobal;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, utf16W, &output);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* BOM is on by default */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT_RAW(stream, bomW, sizeof(bomW));

    IStream_Release(stream);
    IUnknown_Release(output);

    /* start with PI */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, utf16W, &output);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, xmlW, versionW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT_RAW(stream, piW, sizeof(piW));

    IUnknown_Release(output);
    IStream_Release(stream);

    /* start with element */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, utf16W, &output);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT_RAW(stream, aopenW, sizeof(aopenW));

    IUnknown_Release(output);
    IStream_Release(stream);

    /* WriteElementString */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, utf16W, &output);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteElementString(writer, NULL, aW, NULL, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT_RAW(stream, afullW, sizeof(afullW));

    IUnknown_Release(output);
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static HRESULT write_start_element(IXmlWriter *writer, const char *prefix, const char *local,
        const char *uri)
{
    WCHAR *prefixW, *localW, *uriW;
    HRESULT hr;

    prefixW = strdupAtoW(prefix);
    localW = strdupAtoW(local);
    uriW = strdupAtoW(uri);

    hr = IXmlWriter_WriteStartElement(writer, prefixW, localW, uriW);

    heap_free(prefixW);
    heap_free(localW);
    heap_free(uriW);

    return hr;
}

static HRESULT write_element_string(IXmlWriter *writer, const char *prefix, const char *local,
        const char *uri, const char *value)
{
    WCHAR *prefixW, *localW, *uriW, *valueW;
    HRESULT hr;

    prefixW = strdupAtoW(prefix);
    localW = strdupAtoW(local);
    uriW = strdupAtoW(uri);
    valueW = strdupAtoW(value);

    hr = IXmlWriter_WriteElementString(writer, prefixW, localW, uriW, valueW);

    heap_free(prefixW);
    heap_free(localW);
    heap_free(uriW);
    heap_free(valueW);

    return hr;
}

static HRESULT write_string(IXmlWriter *writer, const char *str)
{
    WCHAR *strW;
    HRESULT hr;

    strW = strdupAtoW(str);

    hr = IXmlWriter_WriteString(writer, strW);

    heap_free(strW);

    return hr;
}

static void test_WriteStartElement(void)
{
    static const struct
    {
        const char *prefix;
        const char *local;
        const char *uri;
        const char *output;
        const char *output_partial;
        HRESULT hr;
        int todo;
        int todo_partial;
    }
    start_element_tests[] =
    {
        { "prefix", "local", "uri", "<prefix:local xmlns:prefix=\"uri\" />", "<prefix:local" },
        { NULL, "local", "uri", "<local xmlns=\"uri\" />", "<local" },
        { "", "local", "uri", "<local xmlns=\"uri\" />", "<local" },
        { "", "local", "uri", "<local xmlns=\"uri\" />", "<local" },

        { "prefix", NULL, NULL, NULL, NULL, E_INVALIDARG },
        { NULL, NULL, "uri", NULL, NULL, E_INVALIDARG },
        { NULL, NULL, NULL, NULL, NULL, E_INVALIDARG },
        { NULL, "prefix:local", "uri", NULL, NULL, WC_E_NAMECHARACTER },
        { "pre:fix", "local", "uri", NULL, NULL, WC_E_NAMECHARACTER },
        { NULL, ":local", "uri", NULL, NULL, WC_E_NAMECHARACTER },
        { ":", "local", "uri", NULL, NULL, WC_E_NAMECHARACTER },
        { NULL, "local", "http://www.w3.org/2000/xmlns/", NULL, NULL, WR_E_XMLNSPREFIXDECLARATION },
        { "prefix", "local", "http://www.w3.org/2000/xmlns/", NULL, NULL, WR_E_XMLNSURIDECLARATION },
    };
    static const WCHAR aW[] = {'a',0};
    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = write_start_element(writer, NULL, "a", NULL);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, NULL, "a", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, "<a");

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, aW, aW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    IStream_Release(stream);
    IXmlWriter_Release(writer);

    /* WriteElementString */
    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = write_element_string(writer, NULL, "b", NULL, "value");
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, "prefix", "a", "uri");
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_element_string(writer, NULL, "b", NULL, "value");
    ok(hr == S_OK, "Failed to write element string, hr %#x.\n", hr);

    hr = write_element_string(writer, NULL, "c", NULL, NULL);
    ok(hr == S_OK, "Failed to write element string, hr %#x.\n", hr);

    hr = write_start_element(writer, NULL, "d", "uri");
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_start_element(writer, "", "e", "uri");
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_start_element(writer, "prefix2", "f", "uri");
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<prefix:a xmlns:prefix=\"uri\">"
          "<b>value</b>"
          "<c />"
          "<prefix:d>"
          "<e xmlns=\"uri\">"
          "<prefix2:f");

    IStream_Release(stream);

    /* WriteStartElement */
    for (i = 0; i < ARRAY_SIZE(start_element_tests); ++i)
    {
        stream = writer_set_output(writer);

        writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

        hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
        ok(hr == S_OK, "Failed to start document, hr %#x.\n", hr);

        hr = write_start_element(writer, start_element_tests[i].prefix, start_element_tests[i].local,
                start_element_tests[i].uri);
        ok(hr == start_element_tests[i].hr, "%u: unexpected hr %#x.\n", i, hr);

        if (SUCCEEDED(start_element_tests[i].hr))
        {
            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

            check_output(stream, start_element_tests[i].output_partial, start_element_tests[i].todo_partial, __LINE__);

            hr = IXmlWriter_WriteEndDocument(writer);
            ok(hr == S_OK, "Failed to end document, hr %#x.\n", hr);

            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

            check_output(stream, start_element_tests[i].output, start_element_tests[i].todo, __LINE__);
        }

        IStream_Release(stream);
    }

    IXmlWriter_Release(writer);
}

static void test_WriteElementString(void)
{
    static const struct
    {
        const char *prefix;
        const char *local;
        const char *uri;
        const char *value;
        const char *output;
        HRESULT hr;
        int todo;
    }
    element_string_tests[] =
    {
        { "prefix", "local", "uri", "value", "<prefix:local xmlns:prefix=\"uri\">value</prefix:local>" },
        { NULL, "local", "uri", "value", "<local xmlns=\"uri\">value</local>" },
        { "", "local", "uri", "value", "<local xmlns=\"uri\">value</local>" },
        { "prefix", "local", "uri", NULL, "<prefix:local xmlns:prefix=\"uri\" />" },
        { NULL, "local", "uri", NULL, "<local xmlns=\"uri\" />" },
        { "", "local", "uri", NULL, "<local xmlns=\"uri\" />" },
        { NULL, "local", NULL, NULL, "<local />" },
        { "prefix", "local", "uri", "", "<prefix:local xmlns:prefix=\"uri\"></prefix:local>" },
        { NULL, "local", "uri", "", "<local xmlns=\"uri\"></local>" },
        { "", "local", "uri", "", "<local xmlns=\"uri\"></local>" },
        { NULL, "local", NULL, "", "<local></local>" },
        { "", "local", "http://www.w3.org/2000/xmlns/", NULL, "<local xmlns=\"http://www.w3.org/2000/xmlns/\" />" },

        { "prefix", NULL, NULL, "value", NULL, E_INVALIDARG },
        { NULL, NULL, "uri", "value", NULL, E_INVALIDARG },
        { NULL, NULL, NULL, "value", NULL, E_INVALIDARG },
        { NULL, "prefix:local", "uri", "value", NULL, WC_E_NAMECHARACTER },
        { NULL, ":local", "uri", "value", NULL, WC_E_NAMECHARACTER },
        { ":", "local", "uri", "value", NULL, WC_E_NAMECHARACTER },
        { "prefix", "local", NULL, "value", NULL, WR_E_NSPREFIXWITHEMPTYNSURI },
        { "prefix", "local", "", "value", NULL, WR_E_NSPREFIXWITHEMPTYNSURI },
        { NULL, "local", "http://www.w3.org/2000/xmlns/", "value", NULL, WR_E_XMLNSPREFIXDECLARATION },
        { "prefix", "local", "http://www.w3.org/2000/xmlns/", "value", NULL, WR_E_XMLNSURIDECLARATION },
    };
    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = write_element_string(writer, NULL, "b", NULL, "value");
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, NULL, "a", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_element_string(writer, NULL, "b", NULL, "value");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_element_string(writer, NULL, "b", NULL, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_element_string(writer, "prefix", "b", "uri", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_start_element(writer, "prefix", "c", "uri");
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_element_string(writer, "prefix", "d", NULL, NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, "prefix2", "d", "uri", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, NULL, "e", "uri", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, "prefix", "f", "uri2", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, NULL, "g", "uri3", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, "prefix", "h", NULL, NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, "prefix_i", "i", NULL, NULL);
    ok(hr == WR_E_NSPREFIXWITHEMPTYNSURI, "Failed to write element, hr %#x.\n", hr);

    hr = write_element_string(writer, "", "j", "uri", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a><b>value</b><b />"
        "<prefix:b xmlns:prefix=\"uri\" />"
        "<prefix:c xmlns:prefix=\"uri\">"
          "<prefix:d />"
          "<prefix2:d xmlns:prefix2=\"uri\" />"
          "<prefix:e />"
          "<prefix:f xmlns:prefix=\"uri2\" />"
          "<g xmlns=\"uri3\" />"
          "<prefix:h />"
          "<j xmlns=\"uri\" />");

    IStream_Release(stream);

    for (i = 0; i < ARRAY_SIZE(element_string_tests); ++i)
    {
        stream = writer_set_output(writer);

        writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

        hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
        ok(hr == S_OK, "Failed to start document, hr %#x.\n", hr);

        hr = write_element_string(writer, element_string_tests[i].prefix, element_string_tests[i].local,
                element_string_tests[i].uri, element_string_tests[i].value);
        ok(hr == element_string_tests[i].hr, "%u: unexpected hr %#x.\n", i, hr);

        if (SUCCEEDED(element_string_tests[i].hr))
        {
            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

            check_output(stream, element_string_tests[i].output, element_string_tests[i].todo, __LINE__);

            hr = IXmlWriter_WriteEndDocument(writer);
            ok(hr == S_OK, "Failed to end document, hr %#x.\n", hr);

            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

            check_output(stream, element_string_tests[i].output, element_string_tests[i].todo, __LINE__);
        }

        IStream_Release(stream);
    }

    IXmlWriter_Release(writer);
}

static void test_WriteEndElement(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, NULL, "a", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_start_element(writer, NULL, "b", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, "<a><b /></a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_writeenddocument(void)
{
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HGLOBAL hglobal;
    HRESULT hr;
    char *ptr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    /* WriteEndDocument resets it to initial state */
    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_SetOutput(writer, (IUnknown*)stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ptr = GlobalLock(hglobal);
    ok(ptr == NULL, "got %p\n", ptr);

    /* we still need to flush manually, WriteEndDocument doesn't do that */
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, "<a><b /></a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteComment(void)
{
    static const WCHAR closeW[] = {'-','-','>',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteComment(writer, aW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, aW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, aW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, closeW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, "<!--a--><b><!--a--><!----><!--- ->-->");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteCData(void)
{
    static const WCHAR closeW[] = {']',']','>',0};
    static const WCHAR close2W[] = {'a',']',']','>','b',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteCData(writer, aW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteCData(writer, aW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteCData(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteCData(writer, closeW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteCData(writer, close2W);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b>"
        "<![CDATA[a]]>"
        "<![CDATA[]]>"
        "<![CDATA[]]]]>"
        "<![CDATA[>]]>"
        "<![CDATA[a]]]]>"
        "<![CDATA[>b]]>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteRaw(void)
{
    static const WCHAR rawW[] = {'a','<',':',0};
    static const WCHAR aW[] = {'a',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXmlWriter_WriteRaw(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteRaw(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, rawW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, aW, NULL, aW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, rawW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>a<:a<:<!--a<:-->a<:<a>a</a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_writer_state(void)
{
    static const WCHAR aW[] = {'a',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* initial state */
    check_writer_state(writer, E_UNEXPECTED);

    /* set output and call 'wrong' method, WriteEndElement */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteAttributeString */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, aW, NULL, aW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteEndDocument */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteFullEndElement */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteCData */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteCData(writer, aW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteName */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteName(writer, aW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteNmToken */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteNmToken(writer, aW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteString */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteString(writer, aW);
    ok(hr == WR_E_INVALIDACTION, "got 0x%08x\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_indentation(void)
{
    static const WCHAR commentW[] = {'c','o','m','m','e','n','t',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);
    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteComment(writer, commentW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <!--comment-->\r\n"
        "  <b />\r\n"
        "</a>");

    IStream_Release(stream);

    /* WriteElementString */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, bW, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, bW, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <b />\r\n"
        "</a>");

    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static HRESULT write_attribute_string(IXmlWriter *writer, const char *prefix, const char *local,
        const char *uri, const char *value)
{
    WCHAR *prefixW, *localW, *uriW, *valueW;
    HRESULT hr;

    prefixW = strdupAtoW(prefix);
    localW = strdupAtoW(local);
    uriW = strdupAtoW(uri);
    valueW = strdupAtoW(value);

    hr = IXmlWriter_WriteAttributeString(writer, prefixW, localW, uriW, valueW);

    heap_free(prefixW);
    heap_free(localW);
    heap_free(uriW);
    heap_free(valueW);

    return hr;
}

static void test_WriteAttributeString(void)
{
    static const struct
    {
        const char *prefix;
        const char *local;
        const char *uri;
        const char *value;
        const char *output;
        const char *output_partial;
        HRESULT hr;
        int todo;
        int todo_partial;
        int todo_hr;
    }
    attribute_tests[] =
    {
        { NULL, "a", NULL, "b", "<e a=\"b\" />", "<e a=\"b\"" },
        { "", "a", NULL, "b", "<e a=\"b\" />", "<e a=\"b\"" },
        { NULL, "a", "", "b", "<e a=\"b\" />", "<e a=\"b\"" },
        { "", "a", "", "b", "<e a=\"b\" />", "<e a=\"b\"" },
        { "prefix", "local", "uri", "b", "<e prefix:local=\"b\" xmlns:prefix=\"uri\" />", "<e prefix:local=\"b\"" },
        { NULL, "a", "http://www.w3.org/2000/xmlns/", "defuri", "<e xmlns:a=\"defuri\" />", "<e xmlns:a=\"defuri\"" },
        { "xmlns", "a", NULL, "uri", "<e xmlns:a=\"uri\" />", "<e xmlns:a=\"uri\"" },
        { "xmlns", "a", "", "uri", "<e xmlns:a=\"uri\" />", "<e xmlns:a=\"uri\"" },
        { "prefix", "xmlns", "uri", "value", "<e prefix:xmlns=\"value\" xmlns:prefix=\"uri\" />", "<e prefix:xmlns=\"value\"" },
        { "prefix", "xmlns", "uri", NULL, "<e prefix:xmlns=\"\" xmlns:prefix=\"uri\" />", "<e prefix:xmlns=\"\"" },
        { "prefix", "xmlns", "uri", "", "<e prefix:xmlns=\"\" xmlns:prefix=\"uri\" />", "<e prefix:xmlns=\"\"" },
        { "prefix", "xmlns", NULL, "uri", "<e xmlns=\"uri\" />", "<e xmlns=\"uri\"" },
        { "prefix", "xmlns", "", "uri", "<e xmlns=\"uri\" />", "<e xmlns=\"uri\"" },
        { "xml", "space", NULL, "preserve", "<e xml:space=\"preserve\" />", "<e xml:space=\"preserve\"" },
        { "xml", "space", "", "preserve", "<e xml:space=\"preserve\" />", "<e xml:space=\"preserve\"" },
        { "xml", "space", NULL, "default", "<e xml:space=\"default\" />", "<e xml:space=\"default\"" },
        { "xml", "space", "", "default", "<e xml:space=\"default\" />", "<e xml:space=\"default\"" },
        { "xml", "a", NULL, "value", "<e xml:a=\"value\" />", "<e xml:a=\"value\"" },
        { "xml", "a", "", "value", "<e xml:a=\"value\" />", "<e xml:a=\"value\"" },

        /* Autogenerated prefix names. */
        { NULL, "a", "defuri", NULL, "<e p1:a=\"\" xmlns:p1=\"defuri\" />", "<e p1:a=\"\"", S_OK, 1, 1, 1 },
        { NULL, "a", "defuri", "b", "<e p1:a=\"b\" xmlns:p1=\"defuri\" />", "<e p1:a=\"b\"", S_OK, 1, 1, 1 },
        { "", "a", "defuri", NULL, "<e p1:a=\"\" xmlns:p1=\"defuri\" />", "<e p1:a=\"\"", S_OK, 1, 1, 1 },
        { NULL, "a", "defuri", "", "<e p1:a=\"\" xmlns:p1=\"defuri\" />", "<e p1:a=\"\"", S_OK, 1, 1, 1 },
        { "", "a", "defuri", "b", "<e p1:a=\"b\" xmlns:p1=\"defuri\" />", "<e p1:a=\"b\"", S_OK, 1, 1, 1 },

        /* Failing cases. */
        { NULL, NULL, "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", E_INVALIDARG },
        { "", "a", "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION, 1, 1, 1 },
        { "", NULL, "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", E_INVALIDARG },
        { "", "", "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", E_INVALIDARG, 1, 1, 1 },
        { NULL, "", "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", E_INVALIDARG, 1, 1, 1 },
        { "prefix", "a", "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", WR_E_XMLNSURIDECLARATION, 1, 1, 1 },
        { "prefix", NULL, "http://www.w3.org/2000/xmlns/", "defuri", "<e />", "<e", E_INVALIDARG },
        { "prefix", NULL, NULL, "b", "<e />", "<e", E_INVALIDARG },
        { "prefix", NULL, "uri", NULL, "<e />", "<e", E_INVALIDARG },
        { "xml", NULL, NULL, "value", "<e />", "<e", E_INVALIDARG },
        { "xmlns", "a", "defuri", NULL, "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION },
        { "xmlns", "a", "b", "uri", "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION },
        { NULL, "xmlns", "uri", NULL, "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION, 0, 0, 1 },
        { "xmlns", NULL, "uri", NULL, "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION, 0, 0, 1 },
        { "pre:fix", "local", "uri", "b", "<e />", "<e", WC_E_NAMECHARACTER },
        { "pre:fix", NULL, "uri", "b", "<e />", "<e", E_INVALIDARG },
        { "prefix", "lo:cal", "uri", "b", "<e />", "<e", WC_E_NAMECHARACTER },
        { "xmlns", NULL, NULL, "uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { "xmlns", NULL, "", "uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { "xmlns", "", NULL, "uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { "xmlns", "", "", "uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { "xml", "space", "", "value", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { "xml", "space", NULL, "value", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { "xml", "a", "uri", "value", "<e />", "<e", WR_E_XMLPREFIXDECLARATION },
        { "xml", "space", NULL, "preServe", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { "xml", "space", NULL, "defAult", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { "xml", "space", NULL, NULL, "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { "xml", "space", NULL, "", "<e />", "<e", WR_E_INVALIDXMLSPACE },
    };

    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    for (i = 0; i < ARRAY_SIZE(attribute_tests); ++i)
    {
        stream = writer_set_output(writer);

        hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
        ok(hr == S_OK, "Failed to start document, hr %#x.\n", hr);

        hr = write_start_element(writer, NULL, "e", NULL);
        ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

        hr = write_attribute_string(writer, attribute_tests[i].prefix, attribute_tests[i].local,
                attribute_tests[i].uri, attribute_tests[i].value);
    todo_wine_if(attribute_tests[i].todo_hr)
        ok(hr == attribute_tests[i].hr, "%u: unexpected hr %#x, expected %#x.\n", i, hr, attribute_tests[i].hr);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

        check_output(stream, attribute_tests[i].output_partial, attribute_tests[i].todo_partial, __LINE__);

        hr = IXmlWriter_WriteEndDocument(writer);
        ok(hr == S_OK, "Failed to end document, hr %#x.\n", hr);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

        check_output(stream, attribute_tests[i].output, attribute_tests[i].todo, __LINE__);
        IStream_Release(stream);
    }

    /* With namespaces */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_start_element(writer, "p", "a", "outeruri");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, "prefix", "local", "uri", "b");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, NULL, "a", NULL, "b");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, "xmlns", "prefix", NULL, "uri");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, "p", "attr", NULL, "value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#x.\n", hr);

    hr = write_attribute_string(writer, "prefix", "local", NULL, "b");
todo_wine
    ok(hr == WR_E_DUPLICATEATTRIBUTE, "got 0x%08x\n", hr);

    hr = write_start_element(writer, NULL, "b", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, NULL, "attr2", "outeruri", "value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#x.\n", hr);

    hr = write_attribute_string(writer, "pr", "attr3", "outeruri", "value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#x.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT_TODO(stream,
        "<p:a prefix:local=\"b\" a=\"b\" xmlns:prefix=\"uri\" p:attr=\"value\" xmlns:p=\"outeruri\">"
          "<b p:attr2=\"value\" pr:attr3=\"value\" xmlns:pr=\"outeruri\" />"
        "</p:a>");

    IStream_Release(stream);

    /* Define prefix, write attribute with it. */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_start_element(writer, NULL, "e", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, "xmlns", "prefix", NULL, "uri");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_attribute_string(writer, "prefix", "attr", NULL, "value");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<e xmlns:prefix=\"uri\" prefix:attr=\"value\" />");

    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteFullEndElement(void)
{
    static const WCHAR aW[] = {'a',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* standalone element */
    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);
    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a></a>");
    IStream_Release(stream);

    /* nested elements */
    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);
    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <a></a>\r\n"
        "</a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteCharEntity(void)
{
    static const WCHAR aW[] = {'a',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* without indentation */
    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteCharEntity(writer, 0x100);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a>&#x100;<a /></a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteString(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = write_string(writer, "a");
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    hr = write_string(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_string(writer, "");
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, NULL, "b", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_string(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_string(writer, "");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_string(writer, "a");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* WriteString automatically escapes markup characters */
    hr = write_string(writer, "<&\">=");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b>a&lt;&amp;\"&gt;=");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, NULL, "b", NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = write_string(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b");

    hr = write_string(writer, "");
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b>");

    IStream_Release(stream);
    IXmlWriter_Release(writer);

    /* With indentation */
    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Failed to create a writer, hr %#x.\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = write_start_element(writer, NULL, "a", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_start_element(writer, NULL, "b", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_string(writer, "text");
    ok(hr == S_OK, "Failed to write a string, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b>text");

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b>text</b>");

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b>text</b>\r\n"
        "</a>");

    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = write_start_element(writer, NULL, "a", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_start_element(writer, NULL, "b", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />");

    hr = write_start_element(writer, NULL, "c", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_attribute_string(writer, NULL, "attr", NULL, "value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\"");

    hr = write_string(writer, "text");
    ok(hr == S_OK, "Failed to write a string, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text");

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text</c>");

    hr = write_start_element(writer, NULL, "d", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#x.\n", hr);

    hr = write_string(writer, "");
    ok(hr == S_OK, "Failed to write a string, hr %#x.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text</c>\r\n"
        "  <d></d>");

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#x.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text</c>\r\n"
        "  <d></d>\r\n"
        "</a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static HRESULT write_doctype(IXmlWriter *writer, const char *name, const char *pubid, const char *sysid,
        const char *subset)
{
    WCHAR *nameW, *pubidW, *sysidW, *subsetW;
    HRESULT hr;

    nameW = strdupAtoW(name);
    pubidW = strdupAtoW(pubid);
    sysidW = strdupAtoW(sysid);
    subsetW = strdupAtoW(subset);

    hr = IXmlWriter_WriteDocType(writer, nameW, pubidW, sysidW, subsetW);

    heap_free(nameW);
    heap_free(pubidW);
    heap_free(sysidW);
    heap_free(subsetW);

    return hr;
}

static void test_WriteDocType(void)
{
    static const struct
    {
        const char *name;
        const char *pubid;
        const char *sysid;
        const char *subset;
        const char *output;
    }
    doctype_tests[] =
    {
        { "a", "", NULL, NULL, "<!DOCTYPE a PUBLIC \"\" \"\">" },
        { "a", NULL, NULL, NULL, "<!DOCTYPE a>" },
        { "a", NULL, "", NULL, "<!DOCTYPE a SYSTEM \"\">" },
        { "a", "", "", NULL, "<!DOCTYPE a PUBLIC \"\" \"\">" },
        { "a", "pubid", "", NULL, "<!DOCTYPE a PUBLIC \"pubid\" \"\">" },
        { "a", "pubid", NULL, NULL, "<!DOCTYPE a PUBLIC \"pubid\" \"\">" },
        { "a", "", "sysid", NULL, "<!DOCTYPE a PUBLIC \"\" \"sysid\">" },
        { "a", NULL, NULL, "", "<!DOCTYPE a []>" },
        { "a", NULL, NULL, "subset", "<!DOCTYPE a [subset]>" },
        { "a", "", NULL, "subset", "<!DOCTYPE a PUBLIC \"\" \"\" [subset]>" },
        { "a", NULL, "", "subset", "<!DOCTYPE a SYSTEM \"\" [subset]>" },
        { "a", "", "", "subset", "<!DOCTYPE a PUBLIC \"\" \"\" [subset]>" },
        { "a", "pubid", NULL, "subset", "<!DOCTYPE a PUBLIC \"pubid\" \"\" [subset]>" },
        { "a", "pubid", "", "subset", "<!DOCTYPE a PUBLIC \"pubid\" \"\" [subset]>" },
        { "a", NULL, "sysid", "subset", "<!DOCTYPE a SYSTEM \"sysid\" [subset]>" },
        { "a", "", "sysid", "subset", "<!DOCTYPE a PUBLIC \"\" \"sysid\" [subset]>" },
        { "a", "pubid", "sysid", "subset", "<!DOCTYPE a PUBLIC \"pubid\" \"sysid\" [subset]>" },
    };
    static const WCHAR pubidW[] = {'p',0x100,'i','d',0};
    static const WCHAR nameW[] = {'-','a',0};
    static const WCHAR emptyW[] = { 0 };
    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Failed to create writer instance, hr %#x.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteDocType(writer, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IXmlWriter_WriteDocType(writer, emptyW, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    /* Name validation. */
    hr = IXmlWriter_WriteDocType(writer, nameW, NULL, NULL, NULL);
    ok(hr == WC_E_NAMECHARACTER, "Unexpected hr %#x.\n", hr);

    /* Pubid validation. */
    hr = IXmlWriter_WriteDocType(writer, aW, pubidW, NULL, NULL);
    ok(hr == WC_E_PUBLICID, "Unexpected hr %#x.\n", hr);

    IStream_Release(stream);

    for (i = 0; i < ARRAY_SIZE(doctype_tests); i++)
    {
        stream = writer_set_output(writer);

        hr = write_doctype(writer, doctype_tests[i].name, doctype_tests[i].pubid, doctype_tests[i].sysid,
                doctype_tests[i].subset);
        ok(hr == S_OK, "%u: failed to write doctype, hr %#x.\n", i, hr);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#x.\n", hr);

        CHECK_OUTPUT(stream, doctype_tests[i].output);

        hr = write_doctype(writer, doctype_tests[i].name, doctype_tests[i].pubid, doctype_tests[i].sysid,
                doctype_tests[i].subset);
        ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#x.\n", hr);

        IStream_Release(stream);
    }

    IXmlWriter_Release(writer);
}

START_TEST(writer)
{
    test_writer_create();
    test_writer_state();
    test_writeroutput();
    test_writestartdocument();
    test_WriteStartElement();
    test_WriteElementString();
    test_WriteEndElement();
    test_flush();
    test_omitxmldeclaration();
    test_bom();
    test_writeenddocument();
    test_WriteComment();
    test_WriteCData();
    test_WriteRaw();
    test_indentation();
    test_WriteAttributeString();
    test_WriteFullEndElement();
    test_WriteCharEntity();
    test_WriteString();
    test_WriteDocType();
}
