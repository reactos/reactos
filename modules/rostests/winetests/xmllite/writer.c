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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define CONST_VTABLE
#define COBJMACROS

#include <stdarg.h>
//#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <objbase.h>
#include <ole2.h>
#include <xmllite.h>
#include <wine/test.h>

#include <initguid.h>
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
    char *ptr;

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get the stream handle, hr %#x.\n", hr);

    content_size = GlobalSize(hglobal);
    ok_(__FILE__, line)(size <= content_size, "Unexpected test output size.\n");
    ptr = GlobalLock(hglobal);
    if (size <= content_size)
        ok_(__FILE__, line)(!memcmp(expected, ptr, size), "Unexpected output content.\n");

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

    /* FIXME: add WriteDocType */

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

    /* TODO: WriteDocType */

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
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n','=','"','1','.','0','"',0};
    static const WCHAR utf16W[] = {'u','t','f','-','1','6',0};
    static const WCHAR xmlW[] = {'x','m','l',0};
    IXmlWriterOutput *output;
    unsigned char *ptr;
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

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ptr = GlobalLock(hglobal);
    ok(ptr[0] == 0xff && ptr[1] == 0xfe, "got %x,%x\n", ptr[0], ptr[1]);
    GlobalUnlock(hglobal);

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

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ptr = GlobalLock(hglobal);
    ok(ptr[0] == 0xff && ptr[1] == 0xfe, "got %x,%x\n", ptr[0], ptr[1]);
    GlobalUnlock(hglobal);

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

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ptr = GlobalLock(hglobal);
    ok(ptr[0] == 0xff && ptr[1] == 0xfe, "got %x,%x\n", ptr[0], ptr[1]);
    GlobalUnlock(hglobal);

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

    ptr = GlobalLock(hglobal);
    ok(ptr[0] == 0xff && ptr[1] == 0xfe && ptr[2] == '<', "Unexpected output: %#x,%#x,%#x\n",
            ptr[0], ptr[1], ptr[2]);
    GlobalUnlock(hglobal);

    IUnknown_Release(output);
    IStream_Release(stream);

    IXmlWriter_Release(writer);
}

static void test_writestartelement(void)
{
    static const WCHAR valueW[] = {'v','a','l','u','e',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, aW, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, NULL, aW);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
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

    hr = IXmlWriter_WriteElementString(writer, NULL, bW, NULL, valueW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, bW, NULL, valueW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteElementString(writer, NULL, bW, NULL, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a><b>value</b><b />");

    IStream_Release(stream);
    IXmlWriter_Release(writer);
}

static void test_writeendelement(void)
{
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
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

static void test_WriteAttributeString(void)
{
    static const WCHAR prefixW[] = {'p','r','e','f','i','x',0};
    static const WCHAR localW[] = {'l','o','c','a','l',0};
    static const WCHAR uriW[] = {'u','r','i',0};
    static const WCHAR uri2W[] = {'u','r','i','2',0};
    static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stream = writer_set_output(writer);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, aW, NULL, bW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<a a=\"b\" />");
    IStream_Release(stream);

    /* with namespaces */
    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartDocument(writer, XmlStandalone_Omit);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteStartElement(writer, NULL, aW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, aW, NULL, NULL, bW);
todo_wine
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, prefixW, localW, uriW, bW);
todo_wine
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, aW, NULL, bW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, xmlnsW, uri2W, NULL);
todo_wine
    ok(hr == WR_E_XMLNSPREFIXDECLARATION, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, NULL, xmlnsW, NULL, uri2W);
todo_wine
    ok(hr == WR_E_NSPREFIXDECLARED, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteAttributeString(writer, prefixW, localW, NULL, bW);
todo_wine
    ok(hr == WR_E_DUPLICATEATTRIBUTE, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteEndDocument(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT_TODO(stream,
        "<a prefix:local=\"b\" a=\"b\" xmlns:prefix=\"uri\" />");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
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
    static const WCHAR markupW[] = {'<','&','"','>','=',0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    static const WCHAR emptyW[] = {0};
    IXmlWriter *writer;
    IStream *stream;
    HRESULT hr;

    hr = CreateXmlWriter(&IID_IXmlWriter, (void**)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    writer_set_property(writer, XmlWriterProperty_OmitXmlDeclaration);

    hr = IXmlWriter_WriteString(writer, aW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteString(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteString(writer, emptyW);
    ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteString(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteString(writer, emptyW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteString(writer, aW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* WriteString automatically escapes markup characters */
    hr = IXmlWriter_WriteString(writer, markupW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b>a&lt;&amp;\"&gt;=");
    IStream_Release(stream);

    stream = writer_set_output(writer);

    hr = IXmlWriter_WriteStartElement(writer, NULL, bW, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_WriteString(writer, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b");

    hr = IXmlWriter_WriteString(writer, emptyW);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlWriter_Flush(writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    CHECK_OUTPUT(stream,
        "<b>");

    IXmlWriter_Release(writer);
    IStream_Release(stream);
}

START_TEST(writer)
{
    test_writer_create();
    test_writer_state();
    test_writeroutput();
    test_writestartdocument();
    test_writestartelement();
    test_writeendelement();
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
}
