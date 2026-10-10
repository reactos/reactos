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

#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(IID_IXmlWriterOutput, 0xc1131708, 0x0f59, 0x477f, 0x93, 0x59, 0x7d, 0x33, 0x24, 0x51, 0xbc, 0x1a);

static IStream *create_stream_on_data(const void *data, unsigned int size)
{
    IStream *stream = NULL;
    HGLOBAL hglobal;
    void *ptr;
    HRESULT hr;

    hglobal = GlobalAlloc(GHND, size);
    ptr = GlobalLock(hglobal);

    memcpy(ptr, data, size);

    hr = CreateStreamOnHGlobal(hglobal, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(stream != NULL, "Expected non-NULL stream\n");

    GlobalUnlock(hglobal);

    return stream;
}

#define reader_set_input(a, b) _reader_set_input(__LINE__, a, b)
static void _reader_set_input(unsigned line, IXmlReader *reader, const char *xml)
{
    IStream *stream;
    HRESULT hr;

    stream = create_stream_on_data(xml, strlen(xml));

    hr = IXmlReader_SetInput(reader, (IUnknown *)stream);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);
}

#define EXPECT_REF(obj, ref) _expect_ref((IUnknown *)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG refcount;
    IUnknown_AddRef(obj);
    refcount = IUnknown_Release(obj);
    ok_(__FILE__, line)(refcount == ref, "expected refcount %lu, got %lu.\n", ref, refcount);
}

static void check_output_raw(IStream *stream, const void *expected, SIZE_T size, int line)
{
    SIZE_T content_size;
    HGLOBAL hglobal;
    HRESULT hr;
    WCHAR *ptr;

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get the stream handle, hr %#lx.\n", hr);

    content_size = GlobalSize(hglobal);
    ok_(__FILE__, line)(size == content_size, "Unexpected test output size %Id.\n", content_size);
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
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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

static void writer_set_property(IXmlWriter *writer, XmlWriterProperty property)
{
    HRESULT hr;

    hr = IXmlWriter_SetProperty(writer, property, TRUE);
    ok(hr == S_OK, "Failed to set writer property, hr %#lx.\n", hr);
}

/* used to test all Write* methods for consistent error state */
static void check_writer_state(IXmlWriter *writer, HRESULT exp_hr)
{
    IXmlReader *reader;
    HRESULT hr;
    WCHAR low = 0xdcef;
    WCHAR high = 0xdaff;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* FIXME: add WriteAttributes */

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"a", NULL, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteCData(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteCharEntity(writer, 'a');
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteChars(writer, L"a", 1);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteComment(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteDocType(writer, L"a", NULL, NULL, NULL);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"a", NULL, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteEntityRef(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteName(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteNmToken(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteNode(writer, NULL, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    reader_set_input(reader, "<a/>");
    hr = IXmlWriter_WriteNode(writer, reader, FALSE);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    reader_set_input(reader, "<a/>");
    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"a", L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteQualifiedName(writer, L"a", NULL);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteRaw(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteRawChars(writer, L"a", 1);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteString(writer, L"a");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, low, high);
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    hr = IXmlWriter_WriteWhitespace(writer, L" ");
    ok(hr == exp_hr, "Unexpected hr %#lx, expected %#lx.\n", hr, exp_hr);

    IXmlReader_Release(reader);
}

static IStream *writer_set_output(IXmlWriter *writer)
{
    IStream *stream;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, (IUnknown*)stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriter(&IID_IUnknown, (void **)&unk, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IXmlWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)writer, "unexpected interface pointer\n");
    IUnknown_Release(unk);
    IXmlWriter_Release(writer);

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* check default properties values */
    value = 0;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_ByteOrderMark, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == TRUE, "got %Id.\n", value);

    value = TRUE;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_Indent, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == FALSE, "got %Id.\n", value);

    value = TRUE;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_OmitXmlDeclaration, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == FALSE, "got %Id.\n", value);

    value = XmlConformanceLevel_Auto;
    hr = IXmlWriter_GetProperty(writer, XmlWriterProperty_ConformanceLevel, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == XmlConformanceLevel_Document, "got %Id.\n", value);

    IXmlWriter_Release(writer);
}

static void test_invalid_output_encoding(IXmlWriter *writer, IUnknown *output)
{
    IXmlReader *reader;
    HRESULT hr;
    WCHAR low = 0xdcef;
    WCHAR high = 0xdaff;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Failed to set output, hr %#lx.\n", hr);

    /* TODO: WriteAttributes */

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"a", NULL, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCData(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCharEntity(writer, 0x100);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"a", 1);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteDocType(writer, L"a", NULL, NULL, NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"a", NULL, NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEntityRef(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteName(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteNmToken(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    reader_set_input(reader, "<a/>");
    hr = IXmlWriter_WriteNode(writer, reader, FALSE);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"a", L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteQualifiedName(writer, L"a", NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"a", 1);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"a");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, low, high);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteWhitespace(writer, L" ");
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"a", 1);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"a", 1);
    ok(hr == MX_E_ENCODING, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    IXmlReader_Release(reader);
}

static void test_writeroutput(void)
{
    static const WCHAR utf16_outputW[] = {0xfeff,'<','a'};
    IXmlWriterOutput *output;
    IXmlWriter *writer;
    IStream *stream;
    IUnknown *unk;
    HRESULT hr;

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName(&testoutput, NULL, NULL, &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(output, 1);
    IUnknown_Release(output);

    hr = CreateXmlWriterOutputWithEncodingName(&testoutput, NULL, L"utf-16", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    unk = NULL;
    hr = IUnknown_QueryInterface(output, &IID_IXmlWriterOutput, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(unk != NULL && unk != output, "got %p, output %p\n", unk, output);
    EXPECT_REF(output, 2);
    /* releasing 'unk' crashes on native */
    IUnknown_Release(output);
    EXPECT_REF(output, 1);
    IUnknown_Release(output);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingCodePage(&testoutput, NULL, ~0u, &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(output);

    hr = CreateXmlWriterOutputWithEncodingCodePage(&testoutput, NULL, CP_UTF8, &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    unk = NULL;
    hr = IUnknown_QueryInterface(output, &IID_IXmlWriterOutput, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk != NULL, "got %p\n", unk);
    /* releasing 'unk' crashes on native */
    IUnknown_Release(output);
    IUnknown_Release(output);

    /* create with us-ascii */
    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName(&testoutput, NULL, L"us-ascii", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(output);

    /* Output with codepage 1200. */
    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Failed to create writer, hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create stream, hr %#lx.\n", hr);

    hr = CreateXmlWriterOutputWithEncodingCodePage((IUnknown *)stream, NULL, 1200, &output);
    ok(hr == S_OK, "Failed to create writer output, hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Failed to set writer output, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Write failed, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT_RAW(stream, utf16_outputW, sizeof(utf16_outputW));

    IStream_Release(stream);
    IUnknown_Release(output);

    /* Create output with meaningless code page value. */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Failed to create stream, hr %#lx.\n", hr);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingCodePage((IUnknown *)stream, NULL, ~0u, &output);
    ok(hr == S_OK, "Failed to create writer output, hr %#lx.\n", hr);

    test_invalid_output_encoding(writer, output);
    CHECK_OUTPUT(stream, "");

    IStream_Release(stream);
    IUnknown_Release(output);

    /* Same, with invalid encoding name. */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName((IUnknown *)stream, NULL, L"dummy", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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
    IXmlWriterOutput *output;
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* output not set */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"xml", L"version=\"1.0\"");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    /* nothing written yet */
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, fullprolog);

    /* one more time */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);
    IStream_Release(stream);

    /* now add PI manually, and try to start a document */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"xml", L"version=\"1.0\"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    /* another attempt to add 'xml' PI */
    hr = IXmlWriter_WriteProcessingInstruction(writer, L"xml", L"version=\"1.0\"");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    IStream_Release(stream);
    IXmlWriter_Release(writer);

    /* create with us-ascii */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    output = NULL;
    hr = CreateXmlWriterOutputWithEncodingName((IUnknown *)stream, NULL, L"uS-asCii", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, (IUnknown*)&teststream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    g_write_len = 0;
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(g_write_len > 0, "Unexpected length %lu.\n", g_write_len);

    g_write_len = 1;
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(g_write_len == 0, "Unexpected length %lu.\n", g_write_len);

    /* Release() flushes too */
    g_write_len = 1;
    IXmlWriter_Release(writer);
    ok(g_write_len == 0, "Unexpected length %lu.\n", g_write_len);
}

static void test_omitxmldeclaration(void)
{
    static const char prologversion[] = "<?xml version=\"1.0\"?>";
    IXmlWriter *writer;
    HGLOBAL hglobal;
    IStream *stream;
    HRESULT hr;
    char *ptr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ptr = GlobalLock(hglobal);
    ok(!ptr, "got %p\n", ptr);
    GlobalUnlock(hglobal);

    /* one more time */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    /* now add PI manually, and try to start a document */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"xml", L"version=\"1.0\"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, prologversion);

    /* another attempt to add 'xml' PI */
    hr = IXmlWriter_WriteProcessingInstruction(writer, L"xml", L"version=\"1.0\"");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);
    IXmlWriter_Release(writer);
}

static void test_bom(void)
{
    static const WCHAR piW[] = {0xfeff,'<','?','x','m','l',' ','v','e','r','s','i','o','n','=','"','1','.','0','"','?','>'};
    static const WCHAR aopenW[] = {0xfeff,'<','a'};
    static const WCHAR afullW[] = {0xfeff,'<','a',' ','/','>'};
    static const WCHAR bomW[] = {0xfeff};
    IXmlWriterOutput *output;
    IXmlWriter *writer;
    IStream *stream;
    HGLOBAL hglobal;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, L"utf-16", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* BOM is on by default */
    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT_RAW(stream, bomW, sizeof(bomW));

    IStream_Release(stream);
    IUnknown_Release(output);

    /* start with PI */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, L"utf-16", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"xml", L"version=\"1.0\"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT_RAW(stream, piW, sizeof(piW));

    IUnknown_Release(output);
    IStream_Release(stream);

    /* start with element */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, L"utf-16", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT_RAW(stream, aopenW, sizeof(aopenW));

    IUnknown_Release(output);
    IStream_Release(stream);

    /* WriteElementString */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlWriterOutputWithEncodingName((IUnknown*)stream, NULL, L"utf-16", &output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, output);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"a", NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT_RAW(stream, afullW, sizeof(afullW));

    IUnknown_Release(output);
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteStartElement(void)
{
    static const struct
    {
        const WCHAR *prefix;
        const WCHAR *local;
        const WCHAR *uri;
        const char *output;
        const char *output_partial;
        HRESULT hr;
        int todo;
        int todo_partial;
    }
    start_element_tests[] =
    {
        { L"prefix", L"local", L"uri", "<prefix:local xmlns:prefix=\"uri\" />", "<prefix:local" },
        { NULL, L"local", L"uri", "<local xmlns=\"uri\" />", "<local" },
        { L"", L"local", L"uri", "<local xmlns=\"uri\" />", "<local" },
        { L"", L"local", L"uri", "<local xmlns=\"uri\" />", "<local" },
        { NULL, L"local", NULL, "<local />", "<local" },
        { NULL, L"local", L"", "<local />", "<local" },

        { L"prefix", NULL, NULL, NULL, NULL, E_INVALIDARG },
        { NULL, NULL, L"uri", NULL, NULL, E_INVALIDARG },
        { NULL, NULL, NULL, NULL, NULL, E_INVALIDARG },
        { NULL, L"prefix:local", L"uri", NULL, NULL, WC_E_NAMECHARACTER },
        { L"pre:fix", L"local", L"uri", NULL, NULL, WC_E_NAMECHARACTER },
        { NULL, L":local", L"uri", NULL, NULL, WC_E_NAMECHARACTER },
        { L":", L"local", L"uri", NULL, NULL, WC_E_NAMECHARACTER },
        { NULL, L"local", L"http://www.w3.org/2000/xmlns/", NULL, NULL, WR_E_XMLNSPREFIXDECLARATION },
        { L"prefix", L"local", L"http://www.w3.org/2000/xmlns/", NULL, NULL, WR_E_XMLNSURIDECLARATION },
    };
    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<a");

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteProcessingInstruction(writer, L"a", L"a");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);
    IXmlWriter_Release(writer);

    /* WriteElementString */
    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, L"value");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, L"prefix", L"a", L"uri");
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, L"value");
    ok(hr == S_OK, "Failed to write element string, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"c", NULL, NULL);
    ok(hr == S_OK, "Failed to write element string, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"d", L"uri");
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, L"", L"e", L"uri");
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, L"prefix2", L"f", L"uri");
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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
        ok(hr == S_OK, "Failed to start document, hr %#lx.\n", hr);

        hr = IXmlWriter_WriteStartElement(writer, start_element_tests[i].prefix, start_element_tests[i].local,
                start_element_tests[i].uri);
        ok(hr == start_element_tests[i].hr, "%u: unexpected hr %#lx.\n", i, hr);

        if (SUCCEEDED(start_element_tests[i].hr))
        {
            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

            check_output(stream, start_element_tests[i].output_partial, start_element_tests[i].todo_partial, __LINE__);

            hr = IXmlWriter_WriteEndDocument(writer);
            ok(hr == S_OK, "Failed to end document, hr %#lx.\n", hr);

            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

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
        const WCHAR *prefix;
        const WCHAR *local;
        const WCHAR *uri;
        const WCHAR *value;
        const char *output;
        HRESULT hr;
        int todo;
    }
    element_string_tests[] =
    {
        { L"prefix", L"local", L"uri", L"value", "<prefix:local xmlns:prefix=\"uri\">value</prefix:local>" },
        { NULL, L"local", L"uri", L"value", "<local xmlns=\"uri\">value</local>" },
        { L"", L"local", L"uri", L"value", "<local xmlns=\"uri\">value</local>" },
        { L"prefix", L"local", L"uri", NULL, "<prefix:local xmlns:prefix=\"uri\" />" },
        { NULL, L"local", L"uri", NULL, "<local xmlns=\"uri\" />" },
        { L"", L"local", L"uri", NULL, "<local xmlns=\"uri\" />" },
        { NULL, L"local", NULL, NULL, "<local />" },
        { L"prefix", L"local", L"uri", L"", "<prefix:local xmlns:prefix=\"uri\"></prefix:local>" },
        { NULL, L"local", L"uri", L"", "<local xmlns=\"uri\"></local>" },
        { L"", L"local", L"uri", L"", "<local xmlns=\"uri\"></local>" },
        { NULL, L"local", NULL, L"", "<local></local>" },
        { L"", L"local", L"http://www.w3.org/2000/xmlns/", NULL, "<local xmlns=\"http://www.w3.org/2000/xmlns/\" />" },

        { L"prefix", NULL, NULL, L"value", NULL, E_INVALIDARG },
        { NULL, NULL, L"uri", L"value", NULL, E_INVALIDARG },
        { NULL, NULL, NULL, L"value", NULL, E_INVALIDARG },
        { NULL, L"prefix:local", L"uri", L"value", NULL, WC_E_NAMECHARACTER },
        { NULL, L":local", L"uri", L"value", NULL, WC_E_NAMECHARACTER },
        { L":", L"local", L"uri", L"value", NULL, WC_E_NAMECHARACTER },
        { L"prefix", L"local", NULL, L"value", NULL, WR_E_NSPREFIXWITHEMPTYNSURI },
        { L"prefix", L"local", L"", L"value", NULL, WR_E_NSPREFIXWITHEMPTYNSURI },
        { NULL, L"local", L"http://www.w3.org/2000/xmlns/", L"value", NULL, WR_E_XMLNSPREFIXDECLARATION },
        { L"prefix", L"local", L"http://www.w3.org/2000/xmlns/", L"value", NULL, WR_E_XMLNSURIDECLARATION },
    };
    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, L"value");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, L"value");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"prefix", L"b", L"uri", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, L"prefix", L"c", L"uri");
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"prefix", L"d", NULL, NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"prefix2", L"d", L"uri", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"e", L"uri", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"prefix", L"f", L"uri2", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"g", L"uri3", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"prefix", L"h", NULL, NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"prefix_i", L"i", NULL, NULL);
    ok(hr == WR_E_NSPREFIXWITHEMPTYNSURI, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, L"", L"j", L"uri", NULL);
    ok(hr == S_OK, "Failed to write element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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
        ok(hr == S_OK, "Failed to start document, hr %#lx.\n", hr);

        hr = IXmlWriter_WriteElementString(writer, element_string_tests[i].prefix, element_string_tests[i].local,
                element_string_tests[i].uri, element_string_tests[i].value);
        ok(hr == element_string_tests[i].hr, "%u: unexpected hr %#lx.\n", i, hr);

        if (SUCCEEDED(element_string_tests[i].hr))
        {
            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

            check_output(stream, element_string_tests[i].output, element_string_tests[i].todo, __LINE__);

            hr = IXmlWriter_WriteEndDocument(writer);
            ok(hr == S_OK, "Failed to end document, hr %#lx.\n", hr);

            hr = IXmlWriter_Flush(writer);
            ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

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
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<a><b /></a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_writeenddocument(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HGLOBAL hglobal;
    HRESULT hr;
    char *ptr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    /* WriteEndDocument resets it to initial state */
    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_SetOutput(writer, (IUnknown*)stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ptr = GlobalLock(hglobal);
    ok(ptr == NULL, "got %p\n", ptr);

    /* we still need to flush manually, WriteEndDocument doesn't do that */
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<a><b /></a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteComment(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteComment(writer, L"a");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, L"a");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, L"a");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, L"-->");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<!--a--><b><!--a--><!----><!--- ->-->");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteCData(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteCData(writer, L"a");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCData(writer, L"a");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCData(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCData(writer, L"]]>");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCData(writer, L"a]]>b");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

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
    static const WCHAR surrogates[] = {0xdc00, 0xd800, '\0'};
    static const WCHAR invalid[] = {0x8, '\0'};
    static const WCHAR rawW[] = L"a<:";
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteRaw(writer, surrogates);
    ok(hr == WR_E_INVALIDSURROGATEPAIR, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, L"\uffff");
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, invalid);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, rawW);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"a", NULL, L"a");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Yes);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, rawW);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRaw(writer, rawW);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>a<:a<:<!--a<:-->a<:<a>a</a>");
    IStream_Release(stream);

    /* With open element. */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteRaw(writer, L"text");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<w>text");
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_writer_state(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* initial state */
    check_writer_state(writer, E_UNEXPECTED);

    /* set output and call 'wrong' method, WriteEndElement */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteAttributeString */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"a", NULL, L"a");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteEndDocument */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteFullEndElement */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteCData */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteCData(writer, L"a");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteName */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteName(writer, L"a");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteNmToken */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteNmToken(writer, L"a");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteString */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteString(writer, L"a");
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);
    IStream_Release(stream);

    /* WriteChars */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteChars(writer, L"a", 1);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    check_writer_state(writer, WR_E_INVALIDACTION);

    IStream_Release(stream);
    IXmlWriter_Release(writer);
}

static void test_indentation(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);
    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteComment(writer, L"comment");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <!--comment-->\r\n"
        "  <b />\r\n"
        "</a>");

    IStream_Release(stream);

    /* WriteElementString */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, L"b", NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <b />\r\n"
        "</a>");

    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteAttributeString(void)
{
    static const struct
    {
        const WCHAR *prefix;
        const WCHAR *local;
        const WCHAR *uri;
        const WCHAR *value;
        const char *output;
        const char *output_partial;
        HRESULT hr;
        int todo;
        int todo_partial;
        int todo_hr;
    }
    attribute_tests[] =
    {
        { NULL, L"a", NULL, L"b", "<e a=\"b\" />", "<e a=\"b\"" },
        { L"", L"a", NULL, L"b", "<e a=\"b\" />", "<e a=\"b\"" },
        { NULL, L"a", L"", L"b", "<e a=\"b\" />", "<e a=\"b\"" },
        { L"", L"a", L"", L"b", "<e a=\"b\" />", "<e a=\"b\"" },
        { L"prefix", L"local", L"uri", L"b", "<e prefix:local=\"b\" xmlns:prefix=\"uri\" />", "<e prefix:local=\"b\"" },
        { NULL, L"a", L"http://www.w3.org/2000/xmlns/", L"defuri", "<e xmlns:a=\"defuri\" />", "<e xmlns:a=\"defuri\"" },
        { L"xmlns", L"a", NULL, L"uri", "<e xmlns:a=\"uri\" />", "<e xmlns:a=\"uri\"" },
        { L"xmlns", L"a", L"", L"uri", "<e xmlns:a=\"uri\" />", "<e xmlns:a=\"uri\"" },
        { L"prefix", L"xmlns", L"uri", L"value", "<e prefix:xmlns=\"value\" xmlns:prefix=\"uri\" />", "<e prefix:xmlns=\"value\"" },
        { L"prefix", L"xmlns", L"uri", NULL, "<e prefix:xmlns=\"\" xmlns:prefix=\"uri\" />", "<e prefix:xmlns=\"\"" },
        { L"prefix", L"xmlns", L"uri", L"", "<e prefix:xmlns=\"\" xmlns:prefix=\"uri\" />", "<e prefix:xmlns=\"\"" },
        { L"prefix", L"xmlns", NULL, L"uri", "<e xmlns=\"uri\" />", "<e xmlns=\"uri\"" },
        { L"prefix", L"xmlns", L"", L"uri", "<e xmlns=\"uri\" />", "<e xmlns=\"uri\"" },
        { L"xml", L"space", NULL, L"preserve", "<e xml:space=\"preserve\" />", "<e xml:space=\"preserve\"" },
        { L"xml", L"space", L"", L"preserve", "<e xml:space=\"preserve\" />", "<e xml:space=\"preserve\"" },
        { L"xml", L"space", NULL, L"default", "<e xml:space=\"default\" />", "<e xml:space=\"default\"" },
        { L"xml", L"space", L"", L"default", "<e xml:space=\"default\" />", "<e xml:space=\"default\"" },
        { L"xml", L"a", NULL, L"value", "<e xml:a=\"value\" />", "<e xml:a=\"value\"" },
        { L"xml", L"a", L"", L"value", "<e xml:a=\"value\" />", "<e xml:a=\"value\"" },

        /* Autogenerated prefix names. */
        { NULL, L"a", L"defuri", NULL, "<e p1:a=\"\" xmlns:p1=\"defuri\" />", "<e p1:a=\"\"", S_OK, 1, 1, 1 },
        { NULL, L"a", L"defuri", L"b", "<e p1:a=\"b\" xmlns:p1=\"defuri\" />", "<e p1:a=\"b\"", S_OK, 1, 1, 1 },
        { L"", L"a", L"defuri", NULL, "<e p1:a=\"\" xmlns:p1=\"defuri\" />", "<e p1:a=\"\"", S_OK, 1, 1, 1 },
        { NULL, L"a", L"defuri", L"", "<e p1:a=\"\" xmlns:p1=\"defuri\" />", "<e p1:a=\"\"", S_OK, 1, 1, 1 },
        { L"", L"a", L"defuri", L"b", "<e p1:a=\"b\" xmlns:p1=\"defuri\" />", "<e p1:a=\"b\"", S_OK, 1, 1, 1 },

        /* Failing cases. */
        { NULL, NULL, L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", E_INVALIDARG },
        { L"", L"a", L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION, 1, 1, 1 },
        { L"", NULL, L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", E_INVALIDARG },
        { L"", L"", L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", E_INVALIDARG },
        { NULL, L"", L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", E_INVALIDARG },
        { L"prefix", L"a", L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", WR_E_XMLNSURIDECLARATION, 1, 1, 1 },
        { L"prefix", NULL, L"http://www.w3.org/2000/xmlns/", L"defuri", "<e />", "<e", E_INVALIDARG },
        { L"prefix", NULL, NULL, L"b", "<e />", "<e", E_INVALIDARG },
        { L"prefix", NULL, L"uri", NULL, "<e />", "<e", E_INVALIDARG },
        { L"xml", NULL, NULL, L"value", "<e />", "<e", E_INVALIDARG },
        { L"xmlns", L"a", L"defuri", NULL, "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION },
        { L"xmlns", L"a", L"b", L"uri", "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION },
        { NULL, L"xmlns", L"uri", NULL, "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION, 0, 0, 1 },
        { L"xmlns", NULL, L"uri", NULL, "<e />", "<e", WR_E_XMLNSPREFIXDECLARATION, 0, 0, 1 },
        { L"pre:fix", L"local", L"uri", L"b", "<e />", "<e", WC_E_NAMECHARACTER },
        { L"pre:fix", NULL, L"uri", L"b", "<e />", "<e", E_INVALIDARG },
        { L"prefix", L"lo:cal", L"uri", L"b", "<e />", "<e", WC_E_NAMECHARACTER },
        { L"xmlns", NULL, NULL, L"uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { L"xmlns", NULL, L"", L"uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { L"xmlns", L"", NULL, L"uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { L"xmlns", L"", L"", L"uri", "<e />", "<e", WR_E_NSPREFIXDECLARED },
        { L"xml", L"space", L"", L"value", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { L"xml", L"space", NULL, L"value", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { L"xml", L"a", L"uri", L"value", "<e />", "<e", WR_E_XMLPREFIXDECLARATION },
        { L"xml", L"space", NULL, L"preServe", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { L"xml", L"space", NULL, L"defAult", "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { L"xml", L"space", NULL, NULL, "<e />", "<e", WR_E_INVALIDXMLSPACE },
        { L"xml", L"space", NULL, L"", "<e />", "<e", WR_E_INVALIDXMLSPACE },
    };

    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    for (i = 0; i < ARRAY_SIZE(attribute_tests); ++i)
    {
        winetest_push_context("Test %u", i);

        stream = writer_set_output(writer);

        hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
        ok(hr == S_OK, "Failed to start document, hr %#lx.\n", hr);

        hr = IXmlWriter_WriteStartElement(writer, NULL, L"e", NULL);
        ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

        hr = IXmlWriter_WriteAttributeString(writer, attribute_tests[i].prefix, attribute_tests[i].local,
                attribute_tests[i].uri, attribute_tests[i].value);
        todo_wine_if(attribute_tests[i].todo_hr)
        ok(hr == attribute_tests[i].hr, "Unexpected hr %#lx, expected %#lx.\n", hr, attribute_tests[i].hr);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

        check_output(stream, attribute_tests[i].output_partial, attribute_tests[i].todo_partial, __LINE__);

        hr = IXmlWriter_WriteEndDocument(writer);
        ok(hr == S_OK, "Failed to end document, hr %#lx.\n", hr);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

        check_output(stream, attribute_tests[i].output, attribute_tests[i].todo, __LINE__);
        IStream_Release(stream);

        winetest_pop_context();
    }

    /* With namespaces */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, L"p", L"a", L"outeruri");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"prefix", L"local", L"uri", L"b");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"a", NULL, L"b");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"xmlns", L"prefix", NULL, L"uri");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"p", L"attr", NULL, L"value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"prefix", L"local", NULL, L"b");
    todo_wine
    ok(hr == WR_E_DUPLICATEATTRIBUTE, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"attr2", L"outeruri", L"value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"pr", L"attr3", L"outeruri", L"value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT_TODO(stream,
        "<p:a prefix:local=\"b\" a=\"b\" xmlns:prefix=\"uri\" p:attr=\"value\" xmlns:p=\"outeruri\">"
          "<b p:attr2=\"value\" pr:attr3=\"value\" xmlns:pr=\"outeruri\" />"
        "</p:a>");

    IStream_Release(stream);

    /* Define prefix, write attribute with it. */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"e", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"xmlns", L"prefix", NULL, L"uri");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, L"prefix", L"attr", NULL, L"value");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<e xmlns:prefix=\"uri\" prefix:attr=\"value\" />");

    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteFullEndElement(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* standalone element */
    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);
    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a></a>");
    IStream_Release(stream);

    /* nested elements */
    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);
    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <a></a>\r\n"
        "</a>");
    IStream_Release(stream);

    /* Empty strings for prefix and uri. */
    stream = writer_set_output(writer);

    hr = IXmlWriter_SetProperty(writer, XmlWriterProperty_Indent, FALSE);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, L"", L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", L"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteStartElement(writer, L"", L"c", L"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<a><b><c></c></b></a>");
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteCharEntity(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* without indentation */
    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteCharEntity(writer, 0x100);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>&#x100;<a /></a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteRawChars(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;
    static WCHAR surrogates[] = {0xd800, 0xdc00, 0};

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, NULL, 5);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"", 6);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"sub", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"<rawChars>", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<sub><rawC");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteRawChars(writer, L"a", 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"sub", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"<;;>", 10);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>a<sub><;;>");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"sub", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, surrogates, 1);
    ok(hr == WR_E_INVALIDSURROGATEPAIR, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, surrogates, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<sub>\U00010000");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"sub", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"", 5);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<sub>");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"sub", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<sub");

    IStream_Release(stream);

    stream = writer_set_output(writer);

    /* Force document close */
    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteRawChars(writer, L"a", 1);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);
    IXmlWriter_Release(writer);
}

static void test_WriteSurrogateCharEntity(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;
    WCHAR low = 0xdcef;
    WCHAR high = 0xdaff;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, high, low);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, low, high);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"root", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, high, low);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<root");

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, low, high);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<root>&#xCFCEF;</root>");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteSurrogateCharEntity(writer, low, high);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteString(void)
{
    static const WCHAR surrogates[] = {0xd800, 0xdc00, 'x', 'y', '\0'};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteString(writer, L"a");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"");
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"sub", NULL);
    ok(hr == S_OK, "Unexpected hr #%lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"\v");
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"\ufffe");
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, surrogates);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<sub>\U00010000xy");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"a");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* WriteString automatically escapes markup characters */
    hr = IXmlWriter_WriteString(writer, L"<&\">=");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<b>a&lt;&amp;\"&gt;=");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<b");

    hr = IXmlWriter_WriteString(writer, L"");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<b>");

    IStream_Release(stream);
    IXmlWriter_Release(writer);

    /* With indentation */
    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Failed to create a writer, hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_Indent);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"text");
    ok(hr == S_OK, "Failed to write a string, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b>text");

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b>text</b>");

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b>text</b>\r\n"
        "</a>");

    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"a", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"b", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />");

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"c", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, L"attr", NULL, L"value");
    ok(hr == S_OK, "Failed to write attribute string, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\"");

    hr = IXmlWriter_WriteString(writer, L"text");
    ok(hr == S_OK, "Failed to write a string, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text");

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text</c>");

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"d", NULL);
    ok(hr == S_OK, "Failed to start element, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteString(writer, L"");
    ok(hr == S_OK, "Failed to write a string, hr %#lx.\n", hr);

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text</c>\r\n"
        "  <d></d>");

    hr = IXmlWriter_WriteEndElement(writer);
    ok(hr == S_OK, "Failed to end element, hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<a>\r\n"
        "  <b />\r\n"
        "  <c attr=\"value\">text</c>\r\n"
        "  <d></d>\r\n"
        "</a>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteChars(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;
    static WCHAR raw[] = {'s', 'a', 'm', 0xd800, 0xdc00, 'p', 'l', 'e', 0};

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, NULL, 5);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"", 5);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteChars(writer, L"a", 1);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"chars", NULL);
    ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"chars", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, NULL, 5);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"<chars>", 20);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"<chars>", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<chars>&lt;chars&gt;&lt;chars&gt;");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"chars", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, raw, 8);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    raw[3] = 0xdc00;
    raw[4] = 0xd800;
    hr = IXmlWriter_WriteChars(writer, raw, 8);
    ok(hr == WR_E_INVALIDSURROGATEPAIR, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream, "<chars>sam\U00010000plesam");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"chars", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteFullEndElement(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<chars></chars>");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"chars", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<chars");

    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"c", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteChars(writer, L"", 5);
    ok(hr == WC_E_XMLCHARACTER, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_OUTPUT(stream,
        "<c>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

static void test_WriteDocType(void)
{
    static const struct
    {
        const WCHAR *name;
        const WCHAR *pubid;
        const WCHAR *sysid;
        const WCHAR *subset;
        const char *output;
    }
    doctype_tests[] =
    {
        { L"a", L"", NULL, NULL, "<!DOCTYPE a PUBLIC \"\" \"\">" },
        { L"a", NULL, NULL, NULL, "<!DOCTYPE a>" },
        { L"a", NULL, L"", NULL, "<!DOCTYPE a SYSTEM \"\">" },
        { L"a", L"", L"", NULL, "<!DOCTYPE a PUBLIC \"\" \"\">" },
        { L"a", L"pubid", L"", NULL, "<!DOCTYPE a PUBLIC \"pubid\" \"\">" },
        { L"a", L"pubid", NULL, NULL, "<!DOCTYPE a PUBLIC \"pubid\" \"\">" },
        { L"a", L"", L"sysid", NULL, "<!DOCTYPE a PUBLIC \"\" \"sysid\">" },
        { L"a", NULL, NULL, L"", "<!DOCTYPE a []>" },
        { L"a", NULL, NULL, L"subset", "<!DOCTYPE a [subset]>" },
        { L"a", L"", NULL, L"subset", "<!DOCTYPE a PUBLIC \"\" \"\" [subset]>" },
        { L"a", NULL, L"", L"subset", "<!DOCTYPE a SYSTEM \"\" [subset]>" },
        { L"a", L"", L"", L"subset", "<!DOCTYPE a PUBLIC \"\" \"\" [subset]>" },
        { L"a", L"pubid", NULL, L"subset", "<!DOCTYPE a PUBLIC \"pubid\" \"\" [subset]>" },
        { L"a", L"pubid", L"", L"subset", "<!DOCTYPE a PUBLIC \"pubid\" \"\" [subset]>" },
        { L"a", NULL, L"sysid", L"subset", "<!DOCTYPE a SYSTEM \"sysid\" [subset]>" },
        { L"a", L"", L"sysid", L"subset", "<!DOCTYPE a PUBLIC \"\" \"sysid\" [subset]>" },
        { L"a", L"pubid", L"sysid", L"subset", "<!DOCTYPE a PUBLIC \"pubid\" \"sysid\" [subset]>" },
    };
    static const WCHAR pubidW[] = {'p',0x100,'i','d',0};
    IXmlWriter *writer;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Failed to create writer instance, hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteDocType(writer, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteDocType(writer, L"", NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Name validation. */
    hr = IXmlWriter_WriteDocType(writer, L"-a", NULL, NULL, NULL);
    ok(hr == WC_E_NAMECHARACTER, "Unexpected hr %#lx.\n", hr);

    /* Pubid validation. */
    hr = IXmlWriter_WriteDocType(writer, L"a", pubidW, NULL, NULL);
    ok(hr == WC_E_PUBLICID, "Unexpected hr %#lx.\n", hr);

    /* Invalid multi-character string */
    hr = IXmlWriter_WriteDocType(writer, L":ax>m", NULL, NULL, NULL);
    ok(hr == WC_E_NAMECHARACTER, "Unexpected hr %#lx.\n", hr);

    /* Valid multi-character string */
    hr = IXmlWriter_WriteDocType(writer, L"root", NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    for (i = 0; i < ARRAY_SIZE(doctype_tests); i++)
    {
        stream = writer_set_output(writer);

        hr = IXmlWriter_WriteDocType(writer, doctype_tests[i].name, doctype_tests[i].pubid, doctype_tests[i].sysid,
                doctype_tests[i].subset);
        ok(hr == S_OK, "%u: failed to write doctype, hr %#lx.\n", i, hr);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

        CHECK_OUTPUT(stream, doctype_tests[i].output);

        hr = IXmlWriter_WriteDocType(writer, doctype_tests[i].name, doctype_tests[i].pubid, doctype_tests[i].sysid,
                doctype_tests[i].subset);
        ok(hr == WR_E_INVALIDACTION, "Unexpected hr %#lx.\n", hr);

        IStream_Release(stream);
    }

    IXmlWriter_Release(writer);
}

static void test_WriteWhitespace(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteWhitespace(writer, L" ");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteWhitespace(writer, L"ab");
    ok(hr == WR_E_NONWHITESPACE, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteWhitespace(writer, L"\t");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, " <w>\t");
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteProcessingInstruction(void)
{
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteProcessingInstruction(writer, L"pi", L"content");
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<w><?pi content?>");
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_WriteAttributes(void)
{
    XmlNodeType node_type;
    IXmlWriter *writer;
    IXmlReader *reader;
    const WCHAR *name;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* No attributes. */
    reader_set_input(reader, "<a/>");
    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteAttributes(writer, reader, FALSE);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);
    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteAttributes(writer, reader, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "");
    IStream_Release(stream);

    /* Position on element with attributes. */
    reader_set_input(reader, "<a attr1=\'b\' attr2=\'c\' attr3=\'d\' />");
    stream = writer_set_output(writer);
    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteAttributes(writer, reader, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Element, "Unexpected node type %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<w attr1=\"b\" attr2=\"c\" attr3=\"d\"");
    IStream_Release(stream);

    /* Position on second attribute. */
    hr = IXmlReader_MoveToAttributeByName(reader, L"attr2", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlWriter_WriteAttributes(writer, reader, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Attribute, "Unexpected node type %d.\n", node_type);
    hr = IXmlReader_GetLocalName(reader, &name, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(name, L"attr3"), "Unexpected node %s.\n", debugstr_w(name));
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<w attr2=\"c\" attr3=\"d\"");
    IStream_Release(stream);

    IXmlWriter_Release(writer);
    IXmlReader_Release(reader);
}

static void test_WriteNode(void)
{
    static const struct
    {
        const char *input;
        const char *output;
        XmlNodeType node_type;
    }
    write_node_tests[] =
    {
        { "<r><!-- comment --></r>", "<w><!-- comment -->", XmlNodeType_Comment },
        { "<r>text</r>", "<w>text", XmlNodeType_Text },
        { "<r>  </r>", "<w>  ", XmlNodeType_Whitespace },
        { "<r><![CDATA[ cdata ]]></r>", "<w><![CDATA[ cdata ]]>", XmlNodeType_CDATA },
        { "<r><?pi  pidata  ?></r>", "<w><?pi pidata  ?>", XmlNodeType_ProcessingInstruction },
        { "<r><e1><e2 attr1=\'a\'/></e1></r>", "<w><e1><e2 attr1=\"a\" /></e1>", XmlNodeType_Element },
        { "<r><e1/></r>", "<w><e1 />", XmlNodeType_Element },
        { "<r></r>", "<w></w>", XmlNodeType_EndElement },
    };
    XmlNodeType node_type;
    IXmlWriter *writer;
    IXmlReader *reader;
    const WCHAR *name;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteNode(writer, NULL, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(write_node_tests); ++i)
    {
        winetest_push_context("Test %s", debugstr_a(write_node_tests[i].input));

        stream = writer_set_output(writer);
        reader_set_input(reader, write_node_tests[i].input);

        /* Skip top level element. */
        hr = IXmlReader_Read(reader, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXmlReader_Read(reader, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(node_type == write_node_tests[i].node_type, "Unexpected node type %d.\n", node_type);

        /* Always write a root node to give a valid context for following nodes. */
        hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXmlWriter_WriteNode(writer, reader, FALSE);
        ok(SUCCEEDED(hr), "Failed to write a node, hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            hr = IXmlReader_GetNodeType(reader, &node_type);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(node_type == XmlNodeType_EndElement, "Unexpected node type on return %d.\n", node_type);
            hr = IXmlReader_GetLocalName(reader, &name, NULL);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!wcscmp(name, L"r"), "Unexpected node name %s.\n", debugstr_w(name));
        }

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

        CHECK_OUTPUT(stream, write_node_tests[i].output);

        IStream_Release(stream);

        winetest_pop_context();
    }

    /* Current node is an attribute. */
    reader_set_input(reader, "<a attr=\'b\' ></a>");
    hr = IXmlReader_Read(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Element, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNode(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_EndElement, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "");
    IStream_Release(stream);

    /* Xml declaration node. */
    reader_set_input(reader, "<?xml version=\"1.0\" ?><a/>");
    hr = IXmlReader_Read(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_XmlDeclaration, "Unexpected node type on return %d.\n", node_type);

    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNode(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Element, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    IStream_Release(stream);

    /* With standalone attribute. */
    reader_set_input(reader, "<?xml version=\"1.0\" standalone=\'yes\'?><a/>");
    hr = IXmlReader_Read(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_XmlDeclaration, "Unexpected node type on return %d.\n", node_type);

    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNode(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Element, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    IStream_Release(stream);

    /* Initial state. */
    reader_set_input(reader, "<?xml version=\"1.0\" ?><a><b/></a>");
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_None, "Unexpected node type on return %d.\n", node_type);
    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNode(writer, reader, FALSE);
    ok(hr == S_FALSE, "Failed to write a node, hr %#lx.\n", hr);
    node_type = XmlNodeType_Element;
    hr = IXmlReader_GetNodeType(reader, &node_type);
    todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_None, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><a><b /></a>");
    IStream_Release(stream);

    IXmlReader_Release(reader);
    IXmlWriter_Release(writer);
}

static void test_WriteNodeShallow(void)
{
    static const struct
    {
        const char *input;
        const char *output;
        XmlNodeType node_type;
    }
    write_node_tests[] =
    {
        { "<r><!-- comment --></r>", "<w><!-- comment -->", XmlNodeType_Comment },
        { "<r>text</r>", "<w>text", XmlNodeType_Text },
        { "<r>  </r>", "<w>  ", XmlNodeType_Whitespace },
        { "<r><![CDATA[ cdata ]]></r>", "<w><![CDATA[ cdata ]]>", XmlNodeType_CDATA },
        { "<r><?pi  pidata  ?></r>", "<w><?pi pidata  ?>", XmlNodeType_ProcessingInstruction },
        { "<r><e1><e2 attr1=\'a\'/></e1></r>", "<w><e1", XmlNodeType_Element },
        { "<r><e1/></r>", "<w><e1 />", XmlNodeType_Element },
        { "<r><e1 attr1=\'a\'/></r>", "<w><e1 attr1=\"a\" />", XmlNodeType_Element },
        { "<r><e1 attr1=\'a\'></e1></r>", "<w><e1 attr1=\"a\"", XmlNodeType_Element },
        { "<r></r>", "<w></w>", XmlNodeType_EndElement },
    };
    XmlNodeType node_type;
    IXmlWriter *writer;
    IXmlReader *reader;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void **)&writer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXmlWriter_WriteNodeShallow(writer, NULL, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(write_node_tests); ++i)
    {
        winetest_push_context("Test %s", debugstr_a(write_node_tests[i].input));

        stream = writer_set_output(writer);
        reader_set_input(reader, write_node_tests[i].input);

        /* Skip top level element. */
        hr = IXmlReader_Read(reader, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXmlReader_Read(reader, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(node_type == write_node_tests[i].node_type, "Unexpected node type %d.\n", node_type);

        /* Always write a root node to give a valid context for following nodes. */
        hr = IXmlWriter_WriteStartElement(writer, NULL, L"w", NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
        ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);

        hr = IXmlReader_GetNodeType(reader, &node_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(node_type == write_node_tests[i].node_type, "Unexpected node type on return %d.\n", node_type);

        hr = IXmlWriter_Flush(writer);
        ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);

        CHECK_OUTPUT(stream, write_node_tests[i].output);

        IStream_Release(stream);

        winetest_pop_context();
    }

    /* Current node is an attribute. */
    reader_set_input(reader, "<a attr=\'b\' ></a>");
    hr = IXmlReader_Read(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Element, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Attribute, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "");
    IStream_Release(stream);

    /* Xml declaration node. */
    reader_set_input(reader, "<?xml version=\"1.0\" ?><a/>");
    hr = IXmlReader_Read(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_XmlDeclaration, "Unexpected node type on return %d.\n", node_type);

    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Attribute, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    IStream_Release(stream);

    /* With standalone attribute. */
    reader_set_input(reader, "<?xml version=\"1.0\" standalone=\'yes\'?><a/>");
    hr = IXmlReader_Read(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_XmlDeclaration, "Unexpected node type on return %d.\n", node_type);

    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_Attribute, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
    IStream_Release(stream);

    /* Initial state. */
    reader_set_input(reader, "<?xml version=\"1.0\" ?><a><b/></a>");
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_None, "Unexpected node type on return %d.\n", node_type);
    stream = writer_set_output(writer);
    hr = IXmlWriter_WriteNodeShallow(writer, reader, FALSE);
    ok(hr == S_OK, "Failed to write a node, hr %#lx.\n", hr);
    node_type = XmlNodeType_Element;
    hr = IXmlReader_GetNodeType(reader, &node_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node_type == XmlNodeType_None, "Unexpected node type on return %d.\n", node_type);
    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "Failed to flush, hr %#lx.\n", hr);
    CHECK_OUTPUT(stream, "");
    IStream_Release(stream);

    IXmlReader_Release(reader);
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
    test_WriteChars();
    test_WriteRawChars();
    test_WriteSurrogateCharEntity();
    test_WriteString();
    test_WriteDocType();
    test_WriteWhitespace();
    test_WriteProcessingInstruction();
    test_WriteAttributes();
    test_WriteNode();
    test_WriteNodeShallow();
}
