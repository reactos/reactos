/*
 * IXmlReader tests
 *
 * Copyright 2010, 2012-2013, 2016-2017 Nikolay Sivov
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "xmllite.h"
#include "wine/test.h"
#include "wine/heap.h"

DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

static WCHAR *a2w(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    WCHAR *ret = heap_alloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static void free_str(WCHAR *str)
{
    heap_free(str);
}

static int strcmp_wa(const WCHAR *str1, const char *stra)
{
    WCHAR *str2 = a2w(stra);
    int r = lstrcmpW(str1, str2);
    free_str(str2);
    return r;
}

static const char xmldecl_full[] = "\xef\xbb\xbf<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
static const char xmldecl_short[] = "<?xml version=\"1.0\"?><RegistrationInfo/>";

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
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stream != NULL, "Expected non-NULL stream\n");

    GlobalUnlock(hglobal);

    return stream;
}

static void test_reader_pos(IXmlReader *reader, UINT line, UINT pos, UINT line_broken,
        UINT pos_broken, int _line_)
{
    UINT l = ~0u, p = ~0u;
    BOOL broken_state;

    IXmlReader_GetLineNumber(reader, &l);
    IXmlReader_GetLinePosition(reader, &p);

    if (line_broken == ~0u && pos_broken == ~0u)
        broken_state = FALSE;
    else
        broken_state = broken((line_broken == ~0u ? line : line_broken) == l &&
                              (pos_broken == ~0u ? pos : pos_broken) == p);

    ok_(__FILE__, _line_)((l == line && pos == p) || broken_state,
            "Expected (%d,%d), got (%d,%d)\n", line, pos, l, p);
}
#define TEST_READER_POSITION(reader, line, pos) \
    test_reader_pos(reader, line, pos, ~0u, ~0u, __LINE__)
#define TEST_READER_POSITION2(reader, line, pos, line_broken, pos_broken) \
    test_reader_pos(reader, line, pos, line_broken, pos_broken, __LINE__)

typedef struct input_iids_t {
    IID iids[10];
    int count;
} input_iids_t;

static const IID *setinput_full[] = {
    &IID_IXmlReaderInput,
    &IID_IStream,
    &IID_ISequentialStream,
    NULL
};

/* this applies to early xmllite versions */
static const IID *setinput_full_old[] = {
    &IID_IXmlReaderInput,
    &IID_ISequentialStream,
    &IID_IStream,
    NULL
};

/* after ::SetInput(IXmlReaderInput*) */
static const IID *setinput_readerinput[] = {
    &IID_IStream,
    &IID_ISequentialStream,
    NULL
};

static const IID *empty_seq[] = {
    NULL
};

static input_iids_t input_iids;

static void ok_iids_(const input_iids_t *iids, const IID **expected, const IID **exp_broken, BOOL todo, int line)
{
    int i = 0, size = 0;

    while (expected[i++]) size++;

    todo_wine_if (todo)
        ok_(__FILE__, line)(iids->count == size, "Sequence size mismatch (%d), got (%d)\n", size, iids->count);

    if (iids->count != size) return;

    for (i = 0; i < size; i++) {
        ok_(__FILE__, line)(IsEqualGUID(&iids->iids[i], expected[i]) ||
            (exp_broken ? broken(IsEqualGUID(&iids->iids[i], exp_broken[i])) : FALSE),
            "Wrong IID(%d), got %s\n", i, wine_dbgstr_guid(&iids->iids[i]));
    }
}
#define ok_iids(got, exp, brk, todo) ok_iids_(got, exp, brk, todo, __LINE__)

static const char *state_to_str(XmlReadState state)
{
    static const char* state_names[] = {
        "XmlReadState_Initial",
        "XmlReadState_Interactive",
        "XmlReadState_Error",
        "XmlReadState_EndOfFile",
        "XmlReadState_Closed"
    };

    static const char unknown[] = "unknown";

    switch (state)
    {
    case XmlReadState_Initial:
    case XmlReadState_Interactive:
    case XmlReadState_Error:
    case XmlReadState_EndOfFile:
    case XmlReadState_Closed:
        return state_names[state];
    default:
        return unknown;
    }
}

static const char *type_to_str(XmlNodeType type)
{
    static const char* type_names[] = {
        "XmlNodeType_None",
        "XmlNodeType_Element",
        "XmlNodeType_Attribute",
        "XmlNodeType_Text",
        "XmlNodeType_CDATA",
        "", "",
        "XmlNodeType_ProcessingInstruction",
        "XmlNodeType_Comment",
        "",
        "XmlNodeType_DocumentType",
        "", "",
        "XmlNodeType_Whitespace",
        "",
        "XmlNodeType_EndElement",
        "",
        "XmlNodeType_XmlDeclaration"
    };

    static const char unknown[] = "unknown";

    switch (type)
    {
    case XmlNodeType_None:
    case XmlNodeType_Element:
    case XmlNodeType_Attribute:
    case XmlNodeType_Text:
    case XmlNodeType_CDATA:
    case XmlNodeType_ProcessingInstruction:
    case XmlNodeType_Comment:
    case XmlNodeType_DocumentType:
    case XmlNodeType_Whitespace:
    case XmlNodeType_EndElement:
    case XmlNodeType_XmlDeclaration:
        return type_names[type];
    default:
        return unknown;
    }
}

#define set_input_string(a,b) _set_input_string(__LINE__,a,b);
static void _set_input_string(unsigned line, IXmlReader *reader, const char *xml)
{
    IStream *stream;
    HRESULT hr;

    stream = create_stream_on_data(xml, strlen(xml));

    hr = IXmlReader_SetInput(reader, (IUnknown *)stream);
    ok_(__FILE__,line)(hr == S_OK, "got %08x\n", hr);

    IStream_Release(stream);
}

#define read_node(a,b) _read_node(__LINE__,a,b)
static void _read_node(unsigned line, IXmlReader *reader, XmlNodeType expected_type)
{
    XmlNodeType type;
    HRESULT hr;

    hr = IXmlReader_Read(reader, &type);
    if (expected_type == XmlNodeType_None)
        ok_(__FILE__,line)(hr == S_FALSE, "Read returned %08x, expected S_FALSE\n", hr);
    else
        ok_(__FILE__,line)(hr == S_OK, "Read returned %08x\n", hr);
    ok_(__FILE__,line)(type == expected_type, "read type %d, expected %d\n", type, expected_type);
}

#define next_attribute(a) _next_attribute(__LINE__,a)
static void _next_attribute(unsigned line, IXmlReader *reader)
{
    HRESULT hr;
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok_(__FILE__,line)(hr == S_OK, "MoveToNextAttribute returned %08x\n", hr);
}

#define move_to_element(a) _move_to_element(__LINE__,a)
static void _move_to_element(unsigned line, IXmlReader *reader)
{
    HRESULT hr;
    hr = IXmlReader_MoveToElement(reader);
    ok_(__FILE__,line)(hr == S_OK, "MoveToElement failed: %08x\n", hr);
}

static void test_read_state(IXmlReader *reader, XmlReadState expected,
    XmlReadState exp_broken, int line)
{
    BOOL broken_state;
    LONG_PTR state;

    state = -1; /* invalid state value */
    IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, &state);

    if (exp_broken == expected)
        broken_state = FALSE;
    else
        broken_state = broken(exp_broken == state);

    ok_(__FILE__, line)(state == expected || broken_state, "Expected (%s), got (%s)\n",
            state_to_str(expected), state_to_str(state));
}

#define TEST_READER_STATE(reader, state) test_read_state(reader, state, state, __LINE__)
#define TEST_READER_STATE2(reader, state, brk) test_read_state(reader, state, brk, __LINE__)

#define reader_value(a,b) _reader_value(__LINE__,a,b)
static const WCHAR *_reader_value(unsigned line, IXmlReader *reader, const char *expect)
{
    const WCHAR *str = (void*)0xdeadbeef;
    ULONG len = 0xdeadbeef;
    HRESULT hr;

    hr = IXmlReader_GetValue(reader, &str, &len);
    ok_(__FILE__,line)(hr == S_OK, "GetValue returned %08x\n", hr);
    ok_(__FILE__,line)(len == lstrlenW(str), "len = %u\n", len);
    ok_(__FILE__,line)(!strcmp_wa(str, expect), "value = %s\n", wine_dbgstr_w(str));
    return str;
}

#define reader_name(a,b) _reader_name(__LINE__,a,b)
static const WCHAR *_reader_name(unsigned line, IXmlReader *reader, const char *expect)
{
    const WCHAR *str = (void*)0xdeadbeef;
    ULONG len = 0xdeadbeef;
    HRESULT hr;

    hr = IXmlReader_GetLocalName(reader, &str, &len);
    ok_(__FILE__,line)(hr == S_OK, "GetLocalName returned %08x\n", hr);
    ok_(__FILE__,line)(len == lstrlenW(str), "len = %u\n", len);
    ok_(__FILE__,line)(!strcmp_wa(str, expect), "name = %s\n", wine_dbgstr_w(str));
    return str;
}

#define reader_prefix(a,b) _reader_prefix(__LINE__,a,b)
static const WCHAR *_reader_prefix(unsigned line, IXmlReader *reader, const char *expect)
{
    const WCHAR *str = (void*)0xdeadbeef;
    ULONG len = 0xdeadbeef;
    HRESULT hr;

    hr = IXmlReader_GetPrefix(reader, &str, &len);
    ok_(__FILE__,line)(hr == S_OK, "GetPrefix returned %08x\n", hr);
    ok_(__FILE__,line)(len == lstrlenW(str), "len = %u\n", len);
    ok_(__FILE__,line)(!strcmp_wa(str, expect), "prefix = %s\n", wine_dbgstr_w(str));
    return str;
}

#define reader_namespace(a,b) _reader_namespace(__LINE__,a,b)
static const WCHAR *_reader_namespace(unsigned line, IXmlReader *reader, const char *expect)
{
    const WCHAR *str = (void*)0xdeadbeef;
    ULONG len = 0xdeadbeef;
    HRESULT hr;

    hr = IXmlReader_GetNamespaceUri(reader, &str, &len);
    ok_(__FILE__,line)(hr == S_OK, "GetNamespaceUri returned %08x\n", hr);
    ok_(__FILE__,line)(len == lstrlenW(str), "len = %u\n", len);
    ok_(__FILE__,line)(!strcmp_wa(str, expect), "namespace = %s\n", wine_dbgstr_w(str));
    return str;
}

#define reader_qname(a,b) _reader_qname(a,b,__LINE__)
static const WCHAR *_reader_qname(IXmlReader *reader, const char *expect, unsigned line)
{
    const WCHAR *str = (void*)0xdeadbeef;
    ULONG len = 0xdeadbeef;
    HRESULT hr;

    hr = IXmlReader_GetQualifiedName(reader, &str, &len);
    ok_(__FILE__,line)(hr == S_OK, "GetQualifiedName returned %08x\n", hr);
    ok_(__FILE__,line)(len == lstrlenW(str), "len = %u\n", len);
    ok_(__FILE__,line)(!strcmp_wa(str, expect), "name = %s\n", wine_dbgstr_w(str));
    return str;
}

#define read_value_char(a,b) _read_value_char(a,b,__LINE__)
static void _read_value_char(IXmlReader *reader, WCHAR expected_char, unsigned line)
{
    WCHAR c = 0xffff;
    UINT count = 0;
    HRESULT hr;

    hr = IXmlReader_ReadValueChunk(reader, &c, 1, &count);
    ok_(__FILE__,line)(hr == S_OK, "got %08x\n", hr);
    ok_(__FILE__,line)(count == 1, "got %u\n", c);
    ok_(__FILE__,line)(c == expected_char, "got %x\n", c);
}

typedef struct _testinput
{
    IUnknown IUnknown_iface;
    LONG ref;
} testinput;

static inline testinput *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, testinput, IUnknown_iface);
}

static HRESULT WINAPI testinput_QueryInterface(IUnknown *iface, REFIID riid, void** ppvObj)
{
    if (IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    input_iids.iids[input_iids.count++] = *riid;

    *ppvObj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI testinput_AddRef(IUnknown *iface)
{
    testinput *This = impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI testinput_Release(IUnknown *iface)
{
    testinput *This = impl_from_IUnknown(iface);
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        heap_free(This);

    return ref;
}

static const struct IUnknownVtbl testinput_vtbl =
{
    testinput_QueryInterface,
    testinput_AddRef,
    testinput_Release
};

static HRESULT testinput_createinstance(void **ppObj)
{
    testinput *input;

    input = heap_alloc(sizeof(*input));
    if(!input) return E_OUTOFMEMORY;

    input->IUnknown_iface.lpVtbl = &testinput_vtbl;
    input->ref = 1;

    *ppObj = &input->IUnknown_iface;

    return S_OK;
}

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

static int stream_readcall;

static HRESULT WINAPI teststream_Read(ISequentialStream *iface, void *pv, ULONG cb, ULONG *pread)
{
    static const char xml[] = "<!-- comment -->";

    if (stream_readcall++)
    {
        *pread = 0;
        return E_PENDING;
    }

    *pread = sizeof(xml) / 2;
    memcpy(pv, xml, *pread);
    return S_OK;
}

static HRESULT WINAPI teststream_Write(ISequentialStream *iface, const void *pv, ULONG cb, ULONG *written)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ISequentialStreamVtbl teststreamvtbl =
{
    teststream_QueryInterface,
    teststream_AddRef,
    teststream_Release,
    teststream_Read,
    teststream_Write
};

static HRESULT WINAPI resolver_QI(IXmlResolver *iface, REFIID riid, void **obj)
{
    ok(0, "unexpected call, riid %s\n", wine_dbgstr_guid(riid));

    if (IsEqualIID(riid, &IID_IXmlResolver) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IXmlResolver_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI resolver_AddRef(IXmlResolver *iface)
{
    return 2;
}

static ULONG WINAPI resolver_Release(IXmlResolver *iface)
{
    return 1;
}

static HRESULT WINAPI resolver_ResolveUri(IXmlResolver *iface, const WCHAR *base_uri,
    const WCHAR *public_id, const WCHAR *system_id, IUnknown **input)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IXmlResolverVtbl resolvervtbl =
{
    resolver_QI,
    resolver_AddRef,
    resolver_Release,
    resolver_ResolveUri
};

static IXmlResolver testresolver = { &resolvervtbl };

static void test_reader_create(void)
{
    IXmlResolver *resolver;
    IUnknown *input, *unk;
    IXmlReader *reader;
#ifdef __REACTOS__
    LONG_PTR dtd;
#else
    DtdProcessing dtd;
#endif
    XmlNodeType nodetype;
    HRESULT hr;

    /* crashes native */
    if (0)
    {
        CreateXmlReader(&IID_IXmlReader, NULL, NULL);
        CreateXmlReader(NULL, (void**)&reader, NULL);
    }

    hr = CreateXmlReader(&IID_IStream, (void **)&unk, NULL);
    ok(hr == E_NOINTERFACE, "got %08x\n", hr);

    hr = CreateXmlReader(&IID_IUnknown, (void **)&unk, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IXmlReader, (void **)&reader);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(unk == (IUnknown *)reader, "unexpected interface\n");
    IXmlReader_Release(reader);
    IUnknown_Release(unk);

    hr = CreateXmlReader(&IID_IUnknown, (void **)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    IXmlReader_Release(reader);

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    TEST_READER_STATE(reader, XmlReadState_Closed);

    nodetype = XmlNodeType_Element;
    hr = IXmlReader_GetNodeType(reader, &nodetype);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "got %d\n", nodetype);

    /* crashes on XP, 2k3, works on newer versions */
    if (0)
    {
        hr = IXmlReader_GetNodeType(reader, NULL);
        ok(hr == E_INVALIDARG, "got %08x\n", hr);
    }

    resolver = (void*)0xdeadbeef;
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_XmlResolver, (LONG_PTR*)&resolver);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(resolver == NULL, "got %p\n", resolver);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_XmlResolver, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_XmlResolver, (LONG_PTR)&testresolver);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    resolver = NULL;
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_XmlResolver, (LONG_PTR*)&resolver);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(resolver == &testresolver, "got %p\n", resolver);
    IXmlResolver_Release(resolver);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_XmlResolver, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    dtd = 2;
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_DtdProcessing, (LONG_PTR*)&dtd);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
#ifdef __REACTOS__
    ok(dtd == DtdProcessing_Prohibit, "got %Id\n", dtd);
#else
    ok(dtd == DtdProcessing_Prohibit, "got %d\n", dtd);
#endif

    dtd = 2;
    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, dtd);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, -1);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    /* Null input pointer, releases previous input */
    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    TEST_READER_STATE2(reader, XmlReadState_Initial, XmlReadState_Closed);

    /* test input interface selection sequence */
    hr = testinput_createinstance((void**)&input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    if (hr == S_OK)
    {
        input_iids.count = 0;
        hr = IXmlReader_SetInput(reader, input);
        ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
        ok_iids(&input_iids, setinput_full, setinput_full_old, FALSE);
        IUnknown_Release(input);
    }
    IXmlReader_Release(reader);
}

static void test_readerinput(void)
{
    IXmlReaderInput *reader_input;
    IXmlReader *reader, *reader2;
    IUnknown *obj, *input;
    IStream *stream, *stream2;
    XmlNodeType nodetype;
    HRESULT hr;
    LONG ref;

    hr = CreateXmlReaderInputWithEncodingName(NULL, NULL, NULL, FALSE, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    hr = CreateXmlReaderInputWithEncodingName(NULL, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IStream_AddRef(stream);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IStream_Release(stream);
    hr = CreateXmlReaderInputWithEncodingName((IUnknown*)stream, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IUnknown_QueryInterface(reader_input, &IID_IStream, (void**)&stream2);
    ok(hr == E_NOINTERFACE, "Expected S_OK, got %08x\n", hr);

    hr = IUnknown_QueryInterface(reader_input, &IID_ISequentialStream, (void**)&stream2);
    ok(hr == E_NOINTERFACE, "Expected S_OK, got %08x\n", hr);

    /* IXmlReaderInput grabs a stream reference */
    ref = IStream_AddRef(stream);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IStream_Release(stream);

    /* try ::SetInput() with valid IXmlReaderInput */
    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);

    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    TEST_READER_STATE(reader, XmlReadState_Initial);

    nodetype = XmlNodeType_Element;
    hr = IXmlReader_GetNodeType(reader, &nodetype);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "got %d\n", nodetype);

    /* IXmlReader grabs a IXmlReaderInput reference */
    ref = IUnknown_AddRef(reader_input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(reader_input);

    ref = IStream_AddRef(stream);
    ok(ref == 4, "Expected 4, got %d\n", ref);
    IStream_Release(stream);

    /* reset input and check state */
    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    TEST_READER_STATE2(reader, XmlReadState_Initial, XmlReadState_Closed);

    IXmlReader_Release(reader);

    ref = IStream_AddRef(stream);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IStream_Release(stream);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);

    /* IID_IXmlReaderInput */
    /* it returns a kind of private undocumented vtable incompatible with IUnknown,
       so it's not a COM interface actually.
       Such query will be used only to check if input is really IXmlReaderInput */
    obj = (IUnknown*)0xdeadbeef;
    hr = IUnknown_QueryInterface(reader_input, &IID_IXmlReaderInput, (void**)&obj);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ref = IUnknown_AddRef(reader_input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(reader_input);

    IUnknown_Release(reader_input);
    IUnknown_Release(reader_input);
    IStream_Release(stream);

    /* test input interface selection sequence */
    input = NULL;
    hr = testinput_createinstance((void**)&input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    ref = IUnknown_AddRef(input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(input);
    hr = CreateXmlReaderInputWithEncodingName(input, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok_iids(&input_iids, empty_seq, NULL, FALSE);
    /* IXmlReaderInput stores stream interface as IUnknown */
    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);

    hr = CreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);
    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);
    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_readerinput, NULL, FALSE);

    TEST_READER_STATE(reader, XmlReadState_Closed);

    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 3 || broken(ref == 2) /* versions 1.0.x and 1.1.x - XP, Vista */,
          "Expected 3, got %d\n", ref);
    IUnknown_Release(reader_input);
    /* repeat another time, no check or caching here */
    input_iids.count = 0;
    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_readerinput, NULL, FALSE);

    /* another reader */
    hr = CreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader2, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* resolving from IXmlReaderInput to IStream/ISequentialStream is done at
       ::SetInput() level, each time it's called */
    input_iids.count = 0;
    hr = IXmlReader_SetInput(reader2, reader_input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_readerinput, NULL, FALSE);

    IXmlReader_Release(reader2);
    IXmlReader_Release(reader);

    IUnknown_Release(reader_input);
    IUnknown_Release(input);
}

static void test_reader_state(void)
{
    XmlNodeType nodetype;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* invalid arguments */
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    /* attempt to read on closed reader */
    TEST_READER_STATE(reader, XmlReadState_Closed);

if (0)
{
    /* newer versions crash here, probably because no input was set */
    hr = IXmlReader_Read(reader, &nodetype);
    ok(hr == S_FALSE, "got %08x\n", hr);
}
    set_input_string(reader, "xml");
    TEST_READER_STATE(reader, XmlReadState_Initial);

    nodetype = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &nodetype);
todo_wine
    ok(FAILED(hr), "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "Unexpected node type %d\n", nodetype);

todo_wine
    TEST_READER_STATE(reader, XmlReadState_Error);

    nodetype = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &nodetype);
todo_wine
    ok(FAILED(hr), "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "Unexpected node type %d\n", nodetype);

    IXmlReader_Release(reader);
}

static void test_reader_depth(IXmlReader *reader, UINT depth, UINT brk, int line)
{
    BOOL condition;
    UINT d = ~0u;

    IXmlReader_GetDepth(reader, &d);

    condition = d == depth;
    if (brk != ~0u)
        condition |= broken(d == brk);
    ok_(__FILE__, line)(condition, "Unexpected nesting depth %u, expected %u\n", d, depth);
}

#define TEST_DEPTH(reader, depth) test_reader_depth(reader, depth, ~0u, __LINE__)
#define TEST_DEPTH2(reader, depth, brk) test_reader_depth(reader, depth, brk, __LINE__)

static void test_read_xmldeclaration(void)
{
    static const struct
    {
        WCHAR name[12];
        WCHAR val[12];
    } name_val[] =
    {
        { {'v','e','r','s','i','o','n',0}, {'1','.','0',0} },
        { {'e','n','c','o','d','i','n','g',0}, {'U','T','F','-','8',0} },
        { {'s','t','a','n','d','a','l','o','n','e',0}, {'y','e','s',0} }
    };
    IXmlReader *reader;
    IStream *stream;
    HRESULT hr;
    XmlNodeType type;
    UINT count = 0, len, i;
    BOOL ret;
    const WCHAR *val;

    hr = CreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stream = create_stream_on_data(xmldecl_full, sizeof(xmldecl_full));

    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(count == 0, "got %d\n", count);

    /* try to move without attributes */
    hr = IXmlReader_MoveToElement(reader);
    ok(hr == S_FALSE, "got %08x\n", hr);

    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_FALSE, "got %08x\n", hr);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_FALSE, "got %08x\n", hr);

    TEST_READER_POSITION(reader, 0, 0);

    read_node(reader, XmlNodeType_XmlDeclaration);

    /* new version 1.2.x and 1.3.x properly update position for <?xml ?> */
    TEST_READER_POSITION2(reader, 1, 3, ~0u, 55);

    TEST_DEPTH(reader, 0);
    TEST_READER_STATE(reader, XmlReadState_Interactive);

    reader_value(reader, "");

    /* check attributes */
    next_attribute(reader);

    TEST_DEPTH(reader, 1);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    TEST_READER_POSITION2(reader, 1, 7, ~0u, 55);

    /* try to move from last attribute */
    next_attribute(reader);
    next_attribute(reader);
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_FALSE, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);
    TEST_READER_POSITION2(reader, 1, 7, ~0u, 55);

    hr = IXmlReader_GetAttributeCount(reader, NULL);
    ok(hr == E_INVALIDARG, "got %08x\n", hr);

    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(count == 3, "Expected 3, got %d\n", count);

    for (i = 0; i < count; i++)
    {
        len = 0;
        hr = IXmlReader_GetLocalName(reader, &val, &len);
        ok(hr == S_OK, "got %08x\n", hr);
        ok(len == lstrlenW(name_val[i].name), "expected %u, got %u\n", lstrlenW(name_val[i].name), len);
        ok(!lstrcmpW(name_val[i].name, val), "expected %s, got %s\n", wine_dbgstr_w(name_val[i].name), wine_dbgstr_w(val));

        len = 0;
        hr = IXmlReader_GetValue(reader, &val, &len);
        ok(hr == S_OK, "got %08x\n", hr);
        ok(len == lstrlenW(name_val[i].val), "expected %u, got %u\n", lstrlenW(name_val[i].val), len);
        ok(!lstrcmpW(name_val[i].val, val), "expected %s, got %s\n", wine_dbgstr_w(name_val[i].val), wine_dbgstr_w(val));

        hr = IXmlReader_MoveToNextAttribute(reader);
        ok(hr == ((i < count - 1) ? S_OK : S_FALSE), "got %08x\n", hr);
    }

    TEST_DEPTH(reader, 1);

    move_to_element(reader);
    TEST_READER_POSITION2(reader, 1, 3, ~0u, 55);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_XmlDeclaration, "got %d\n", type);

    type = XmlNodeType_XmlDeclaration;
    hr = IXmlReader_Read(reader, &type);
    /* newer versions return syntax error here cause document is incomplete,
       it makes more sense than invalid char error */
todo_wine {
    ok(hr == WC_E_SYNTAX || broken(hr == WC_E_XMLCHARACTER), "got 0x%08x\n", hr);
    ok(type == XmlNodeType_None, "got %d\n", type);
}
    IStream_Release(stream);

    /* test short variant */
    stream = create_stream_on_data(xmldecl_short, sizeof(xmldecl_short));

    hr = IXmlReader_SetInput(reader, (IUnknown *)stream);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);

    read_node(reader, XmlNodeType_XmlDeclaration);
    TEST_READER_POSITION2(reader, 1, 3, ~0u, 21);
    TEST_READER_STATE(reader, XmlReadState_Interactive);

    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(count == 1, "expected 1, got %d\n", count);

    ret = IXmlReader_IsEmptyElement(reader);
    ok(!ret, "element should not be empty\n");

    reader_value(reader, "");
    reader_name(reader, "xml");

    reader_qname(reader, "xml");

    /* check attributes */
    next_attribute(reader);

    type = -1;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);
    TEST_READER_POSITION2(reader, 1, 7, ~0u, 21);

    /* try to move from last attribute */
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_FALSE, "expected S_FALSE, got %08x\n", hr);

    read_node(reader, XmlNodeType_Element);
    TEST_READER_POSITION2(reader, 1, 23, ~0u, 40);
    TEST_READER_STATE(reader, XmlReadState_Interactive);

    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(count == 0, "expected 0, got %d\n", count);

    ret = IXmlReader_IsEmptyElement(reader);
    ok(ret, "element should be empty\n");

    reader_value(reader, "");
    reader_name(reader, "RegistrationInfo");

    type = -1;
    hr = IXmlReader_Read(reader, &type);
todo_wine
    ok(hr == WC_E_SYNTAX || hr == WC_E_XMLCHARACTER /* XP */, "expected WC_E_SYNTAX, got %08x\n", hr);
    ok(type == XmlNodeType_None, "expected XmlNodeType_None, got %s\n", type_to_str(type));
    TEST_READER_POSITION(reader, 1, 41);
todo_wine
    TEST_READER_STATE(reader, XmlReadState_Error);

    IStream_Release(stream);
    IXmlReader_Release(reader);
}

struct test_entry {
    const char *xml;
    const char *name;
    const char *value;
    HRESULT hr;
    HRESULT hr_broken; /* this is set to older version results */
    BOOL todo;
};

static struct test_entry comment_tests[] = {
    { "<!-- comment -->", "", " comment ", S_OK },
    { "<!-- - comment-->", "", " - comment", S_OK },
    { "<!-- -- comment-->", NULL, NULL, WC_E_COMMENT, WC_E_GREATERTHAN },
    { "<!-- -- comment--->", NULL, NULL, WC_E_COMMENT, WC_E_GREATERTHAN },
    { NULL }
};

static void test_read_comment(void)
{
    static const char *teststr = "<a>text<!-- comment --></a>";
    struct test_entry *test = comment_tests;
    static const XmlNodeType types[] =
    {
        XmlNodeType_Element,
        XmlNodeType_Text,
        XmlNodeType_Comment,
        XmlNodeType_EndElement,
    };
    unsigned int i = 0;
    IXmlReader *reader;
    XmlNodeType type;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, teststr);

    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        const WCHAR *value;

        ok(type == types[i], "%d: unexpected node type %d\n", i, type);

        if (type == XmlNodeType_Text || type == XmlNodeType_Comment)
        {
            hr = IXmlReader_GetValue(reader, &value, NULL);
            ok(hr == S_OK, "got %08x\n", hr);
            ok(*value != 0, "Expected node value\n");
        }
        i++;
    }

    while (test->xml)
    {
        set_input_string(reader, test->xml);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        if (test->hr_broken)
            ok(hr == test->hr || broken(hr == test->hr_broken), "got %08x for %s\n", hr, test->xml);
        else
            ok(hr == test->hr, "got %08x for %s\n", hr, test->xml);
        if (hr == S_OK)
        {
            const WCHAR *str;

            ok(type == XmlNodeType_Comment, "got %d for %s\n", type, test->xml);

            reader_name(reader, "");

            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            reader_qname(reader, "");

            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            /* value */
            reader_value(reader, test->value);
        }

        test++;
    }

    IXmlReader_Release(reader);
}

static struct test_entry pi_tests[] = {
    { "<?pi?>", "pi", "", S_OK },
    { "<?pi ?>", "pi", "", S_OK },
    { "<?pi  ?>", "pi", "", S_OK },
    { "<?pi pi data?>", "pi", "pi data", S_OK },
    { "<?pi pi data  ?>", "pi", "pi data  ", S_OK },
    { "<?pi    data  ?>", "pi", "data  ", S_OK },
    { "<?pi:pi?>", NULL, NULL, NC_E_NAMECOLON, WC_E_NAMECHARACTER },
    { "<?:pi ?>", NULL, NULL, WC_E_PI, WC_E_NAMECHARACTER },
    { "<?-pi ?>", NULL, NULL, WC_E_PI, WC_E_NAMECHARACTER },
    { "<?xml-stylesheet ?>", "xml-stylesheet", "", S_OK },
    { NULL }
};

static void test_read_pi(void)
{
    struct test_entry *test = pi_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;

        set_input_string(reader, test->xml);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        if (test->hr_broken)
            ok(hr == test->hr || broken(hr == test->hr_broken), "got %08x for %s\n", hr, test->xml);
        else
            ok(hr == test->hr, "got %08x for %s\n", hr, test->xml);
        if (hr == S_OK)
        {
            const WCHAR *str;
            WCHAR *str_exp;
            UINT len;

            ok(type == XmlNodeType_ProcessingInstruction, "got %d for %s\n", type, test->xml);

            reader_name(reader, test->name);

            len = 0;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            reader_value(reader, test->value);
        }

        test++;
    }

    IXmlReader_Release(reader);
}

struct nodes_test {
    const char *xml;
    struct {
        XmlNodeType type;
        const char *value;
    } nodes[20];
};

static const char misc_test_xml[] =
    "<!-- comment1 -->"
    "<!-- comment2 -->"
    "<?pi1 pi1body ?>"
    "<!-- comment3 -->"
    " \t \r \n"
    "<!-- comment4 -->"
    "<a>"
    "\r\n\t"
    "<b/>"
    "text"
    "<!-- comment -->"
    "text2"
    "<?pi pibody ?>"
    "\r\n"
    "</a>"
;

static struct nodes_test misc_test = {
    misc_test_xml,
    {
        {XmlNodeType_Comment, " comment1 "},
        {XmlNodeType_Comment, " comment2 "},
        {XmlNodeType_ProcessingInstruction, "pi1body "},
        {XmlNodeType_Comment, " comment3 "},
        {XmlNodeType_Whitespace, " \t \n \n"},
        {XmlNodeType_Comment, " comment4 "},
        {XmlNodeType_Element, ""},
        {XmlNodeType_Whitespace, "\n\t"},
        {XmlNodeType_Element, ""},
        {XmlNodeType_Text, "text"},
        {XmlNodeType_Comment, " comment "},
        {XmlNodeType_Text, "text2"},
        {XmlNodeType_ProcessingInstruction, "pibody "},
        {XmlNodeType_Whitespace, "\n"},
        {XmlNodeType_EndElement, ""},
        {XmlNodeType_None, ""}
    }
};

static void test_read_full(void)
{
    struct nodes_test *test = &misc_test;
    IXmlReader *reader;
    HRESULT hr;
    int i;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, test->xml);

    i = 0;
    do
    {
        read_node(reader, test->nodes[i].type);
        reader_value(reader, test->nodes[i].value);
    } while(test->nodes[i++].type != XmlNodeType_None);

    IXmlReader_Release(reader);
}

static const char test_public_dtd[] =
    "<!DOCTYPE testdtd PUBLIC \"pubid\" \"externalid uri\" >";

static void test_read_public_dtd(void)
{
    static const WCHAR dtdnameW[] = {'t','e','s','t','d','t','d',0};
    IXmlReader *reader;
    const WCHAR *str;
    XmlNodeType type;
    UINT len, count;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Parse);
    ok(hr == S_OK, "got 0x%8x\n", hr);

    set_input_string(reader, test_public_dtd);

    read_node(reader, XmlNodeType_DocumentType);

    count = 0;
    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(count == 2, "got %d\n", count);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    reader_name(reader, "PUBLIC");
    reader_value(reader, "pubid");

    next_attribute(reader);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    reader_name(reader, "SYSTEM");
    reader_value(reader, "externalid uri");

    move_to_element(reader);
    reader_name(reader, "testdtd");

    len = 0;
    str = NULL;
    hr = IXmlReader_GetQualifiedName(reader, &str, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine {
    ok(len == lstrlenW(dtdnameW), "got %u\n", len);
    ok(!lstrcmpW(str, dtdnameW), "got %s\n", wine_dbgstr_w(str));
}
    IXmlReader_Release(reader);
}

static const char test_system_dtd[] =
    "<!DOCTYPE testdtd SYSTEM \"externalid uri\" >"
    "<!-- comment -->";

static void test_read_system_dtd(void)
{
    static const WCHAR dtdnameW[] = {'t','e','s','t','d','t','d',0};
    IXmlReader *reader;
    const WCHAR *str;
    XmlNodeType type;
    UINT len, count;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Parse);
    ok(hr == S_OK, "got 0x%8x\n", hr);

    set_input_string(reader, test_system_dtd);

    read_node(reader, XmlNodeType_DocumentType);

    count = 0;
    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(count == 1, "got %d\n", count);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    reader_name(reader, "SYSTEM");
    reader_value(reader, "externalid uri");

    move_to_element(reader);
    reader_name(reader, "testdtd");

    len = 0;
    str = NULL;
    hr = IXmlReader_GetQualifiedName(reader, &str, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine {
    ok(len == lstrlenW(dtdnameW), "got %u\n", len);
    ok(!lstrcmpW(str, dtdnameW), "got %s\n", wine_dbgstr_w(str));
}

    read_node(reader, XmlNodeType_Comment);

    IXmlReader_Release(reader);
}

static struct test_entry element_tests[] = {
    { "<a/>", "a", "", S_OK },
    { "<a />", "a", "", S_OK },
    { "<a:b/>", "a:b", "", NC_E_UNDECLAREDPREFIX },
    { "<:a/>", NULL, NULL, NC_E_QNAMECHARACTER },
    { "< a/>", NULL, NULL, NC_E_QNAMECHARACTER },
    { "<a>", "a", "", S_OK },
    { "<a >", "a", "", S_OK },
    { "<a \r \t\n>", "a", "", S_OK },
    { "</a>", NULL, NULL, NC_E_QNAMECHARACTER },
    { "<a:b:c />", NULL, NULL, NC_E_QNAMECOLON },
    { "<:b:c />", NULL, NULL, NC_E_QNAMECHARACTER },
    { NULL }
};

static void test_read_element(void)
{
    struct test_entry *test = element_tests;
    static const char stag[] =
         "<a attr1=\"_a\">"
             "<b attr2=\"_b\">"
                 "text"
                 "<c attr3=\"_c\"/>"
                 "<d attr4=\"_d\"></d>"
             "</b>"
         "</a>";
    static const UINT depths[] = { 0, 1, 2, 2, 2, 3, 2, 1 };
    IXmlReader *reader;
    XmlNodeType type;
    unsigned int i;
    UINT depth;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        set_input_string(reader, test->xml);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        if (test->hr_broken)
            ok(hr == test->hr || broken(hr == test->hr_broken), "got %08x for %s\n", hr, test->xml);
        else
            todo_wine_if(test->hr == NC_E_UNDECLAREDPREFIX)
                ok(hr == test->hr, "got %08x for %s\n", hr, test->xml);
        if (hr == S_OK)
        {
            const WCHAR *str;
            WCHAR *str_exp;
            UINT len;

            ok(type == XmlNodeType_Element, "got %d for %s\n", type, test->xml);

            len = 0;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            reader_value(reader, "");
        }

        test++;
    }

    /* test reader depth increment */
    set_input_string(reader, stag);

    i = 0;
    while (IXmlReader_Read(reader, &type) == S_OK)
    {
        UINT count;

        ok(type == XmlNodeType_Element || type == XmlNodeType_EndElement ||
                type == XmlNodeType_Text, "Unexpected node type %d\n", type);

        depth = 123;
        hr = IXmlReader_GetDepth(reader, &depth);
        ok(hr == S_OK, "got %08x\n", hr);
        ok(depth == depths[i], "%u: got depth %u, expected %u\n", i, depth, depths[i]);

        if (type == XmlNodeType_Element || type == XmlNodeType_EndElement)
        {
            const WCHAR *prefix;

            prefix = NULL;
            hr = IXmlReader_GetPrefix(reader, &prefix, NULL);
            ok(hr == S_OK, "got %08x\n", hr);
            ok(prefix != NULL, "got %p\n", prefix);

            if (!*prefix)
            {
                const WCHAR *local, *qname;

                local = NULL;
                hr = IXmlReader_GetLocalName(reader, &local, NULL);
                ok(hr == S_OK, "got %08x\n", hr);
                ok(local != NULL, "got %p\n", local);

                qname = NULL;
                hr = IXmlReader_GetQualifiedName(reader, &qname, NULL);
                ok(hr == S_OK, "got %08x\n", hr);
                ok(qname != NULL, "got %p\n", qname);

                ok(local == qname, "expected same pointer\n");
            }
        }

        if (type == XmlNodeType_EndElement)
        {
            count = 1;
            hr = IXmlReader_GetAttributeCount(reader, &count);
            ok(hr == S_OK, "got %08x\n", hr);
            ok(count == 0, "got %u\n", count);
        }

        if (type == XmlNodeType_Element)
        {
            count = 0;
            hr = IXmlReader_GetAttributeCount(reader, &count);
            ok(hr == S_OK, "got %08x\n", hr);

            /* moving to attributes increases depth */
            if (count)
            {
                const WCHAR *value;

                reader_value(reader, "");

                hr = IXmlReader_MoveToFirstAttribute(reader);
                ok(hr == S_OK, "got %08x\n", hr);

                hr = IXmlReader_GetValue(reader, &value, NULL);
                ok(*value != 0, "Unexpected value %s\n", wine_dbgstr_w(value));

                depth = 123;
                hr = IXmlReader_GetDepth(reader, &depth);
                ok(hr == S_OK, "got %08x\n", hr);
                ok(depth == depths[i] + 1, "%u: got depth %u, expected %u\n", i, depth, depths[i] + 1);

                move_to_element(reader);
                reader_value(reader, "");

                depth = 123;
                hr = IXmlReader_GetDepth(reader, &depth);
                ok(hr == S_OK, "got %08x\n", hr);
                ok(depth == depths[i], "%u: got depth %u, expected %u\n", i, depth, depths[i]);
            }
        }

        i++;
    }

    /* start/end tag mismatch */
    set_input_string(reader, "<a></b>");

    read_node(reader, XmlNodeType_Element);

    type = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == WC_E_ELEMENTMATCH, "got %08x\n", hr);
    ok(type == XmlNodeType_None, "got %d\n", type);
    TEST_READER_STATE(reader, XmlReadState_Error);

    IXmlReader_Release(reader);
}

static ISequentialStream teststream = { &teststreamvtbl };

static void test_read_pending(void)
{
    IXmlReader *reader;
    const WCHAR *value;
    XmlNodeType type;
    HRESULT hr;
    int c;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got 0x%08x\n", hr);

    hr = IXmlReader_SetInput(reader, (IUnknown*)&teststream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* first read call returns incomplete node, second attempt fails with E_PENDING */
    stream_readcall = 0;
    type = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK || broken(hr == E_PENDING), "got 0x%08x\n", hr);
    /* newer versions are happy when it's enough data to detect node type,
       older versions keep reading until it fails to read more */
todo_wine
    ok(stream_readcall == 1 || broken(stream_readcall > 1), "got %d\n", stream_readcall);
    ok(type == XmlNodeType_Comment || broken(type == XmlNodeType_None), "got %d\n", type);

    /* newer versions' GetValue() makes an attempt to read more */
    c = stream_readcall;
    value = (void*)0xdeadbeef;
    hr = IXmlReader_GetValue(reader, &value, NULL);
    ok(hr == E_PENDING, "got 0x%08x\n", hr);
    ok(value == NULL || broken(value == (void*)0xdeadbeef) /* Win8 sets it to NULL */, "got %p\n", value);
    ok(c < stream_readcall || broken(c == stream_readcall), "got %d, expected %d\n", stream_readcall, c+1);

    IXmlReader_Release(reader);
}

static void test_readvaluechunk(void)
{
    IXmlReader *reader;
    XmlNodeType type;
    WCHAR buf[64];
    WCHAR b;
    HRESULT hr;
    UINT c;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, "<!-- comment1 --><!-- comment2 -->");

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Comment, "type = %u\n", type);

    read_value_char(reader, ' ');
    read_value_char(reader, 'c');

    /* portion read as chunk is skipped from resulting node value */
    reader_value(reader, "omment1 ");

    /* once value is returned/allocated it's not possible to read by chunk */
    c = 0;
    b = 0;
    hr = IXmlReader_ReadValueChunk(reader, &b, 1, &c);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(c == 0, "got %u\n", c);
    ok(b == 0, "got %x\n", b);

    c = 0xdeadbeef;
    hr = IXmlReader_ReadValueChunk(reader, buf, 0, &c);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(!c, "c = %u\n", c);

    reader_value(reader, "omment1 ");

    /* read comment2 */
    read_node(reader, XmlNodeType_Comment);

    c = 0xdeadbeef;
    hr = IXmlReader_ReadValueChunk(reader, buf, 0, &c);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(!c, "c = %u\n", c);

    c = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    hr = IXmlReader_ReadValueChunk(reader, buf, ARRAY_SIZE(buf), &c);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(c == 10, "got %u\n", c);
    ok(buf[c] == 0xcccc, "buffer overflow\n");
    buf[c] = 0;
    ok(!strcmp_wa(buf, " comment2 "), "buf = %s\n", wine_dbgstr_w(buf));

    c = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    hr = IXmlReader_ReadValueChunk(reader, buf, ARRAY_SIZE(buf), &c);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(!c, "got %u\n", c);

    /* portion read as chunk is skipped from resulting node value */
    reader_value(reader, "");

    /* once value is returned/allocated it's not possible to read by chunk */
    c = 0xdeadbeef;
    b = 0xffff;
    hr = IXmlReader_ReadValueChunk(reader, &b, 1, &c);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(c == 0, "got %u\n", c);
    ok(b == 0xffff, "got %x\n", b);

    reader_value(reader, "");

    IXmlReader_Release(reader);
}

static struct test_entry cdata_tests[] = {
    { "<a><![CDATA[ ]]data ]]></a>", "", " ]]data ", S_OK },
    { "<a><![CDATA[<![CDATA[ data ]]]]></a>", "", "<![CDATA[ data ]]", S_OK },
    { "<a><![CDATA[\n \r\n \n\n ]]></a>", "", "\n \n \n\n ", S_OK, S_OK },
    { "<a><![CDATA[\r \r\r\n \n\n ]]></a>", "", "\n \n\n \n\n ", S_OK, S_OK },
    { "<a><![CDATA[\r\r \n\r \r \n\n ]]></a>", "", "\n\n \n\n \n \n\n ", S_OK },
    { NULL }
};

static void test_read_cdata(void)
{
    struct test_entry *test = cdata_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;

        set_input_string(reader, test->xml);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);

        /* read one more to get to CDATA */
        if (type == XmlNodeType_Element)
        {
            type = XmlNodeType_None;
            hr = IXmlReader_Read(reader, &type);
        }

        if (test->hr_broken)
            ok(hr == test->hr || broken(hr == test->hr_broken), "got %08x for %s\n", hr, test->xml);
        else
            ok(hr == test->hr, "got %08x for %s\n", hr, test->xml);
        if (hr == S_OK)
        {
            const WCHAR *str;
            UINT len;

            ok(type == XmlNodeType_CDATA, "got %d for %s\n", type, test->xml);

            reader_name(reader, "");

            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == 0, "got %u\n", len);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            /* value */
            reader_value(reader, test->value);
        }

        test++;
    }

    IXmlReader_Release(reader);
}

static struct test_entry text_tests[] = {
    { "<a>simple text</a>", "", "simple text", S_OK },
    { "<a>text ]]> text</a>", "", "", WC_E_CDSECTEND },
    { "<a>\n \r\n \n\n text</a>", "", "\n \n \n\n text", S_OK, S_OK },
    { "<a>\r \r\r\n \n\n text</a>", "", "\n \n\n \n\n text", S_OK, S_OK },
    { NULL }
};

static void test_read_text(void)
{
    struct test_entry *test = text_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;

        set_input_string(reader, test->xml);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);

        /* read one more to get to text node */
        if (type == XmlNodeType_Element)
        {
            type = XmlNodeType_None;
            hr = IXmlReader_Read(reader, &type);
        }
        ok(hr == test->hr, "got %08x for %s\n", hr, test->xml);
        if (hr == S_OK)
        {
            const WCHAR *str;
            UINT len;

            ok(type == XmlNodeType_Text, "got %d for %s\n", type, test->xml);

            reader_name(reader, "");

            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == 0, "got %u\n", len);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));

            /* value */
            reader_value(reader, test->value);
        }

        test++;
    }

    IXmlReader_Release(reader);
}

struct test_entry_empty {
    const char *xml;
    BOOL empty;
};

static struct test_entry_empty empty_element_tests[] = {
    { "<a></a>", FALSE },
    { "<a/>", TRUE },
    { NULL }
};

static void test_isemptyelement(void)
{
    struct test_entry_empty *test = empty_element_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        BOOL ret;

        set_input_string(reader, test->xml);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(type == XmlNodeType_Element, "got %d\n", type);

        ret = IXmlReader_IsEmptyElement(reader);
        ok(ret == test->empty, "got %d, expected %d. xml=%s\n", ret, test->empty, test->xml);

        test++;
    }

    IXmlReader_Release(reader);
}

static struct test_entry attributes_tests[] = {
    { "<a attr1=\"attrvalue\"/>", "attr1", "attrvalue", S_OK },
    { "<a attr1=\"a\'\'ttrvalue\"/>", "attr1", "a\'\'ttrvalue", S_OK },
    { "<a attr1=\'a\"ttrvalue\'/>", "attr1", "a\"ttrvalue", S_OK },
    { "<a attr1=\' \'/>", "attr1", " ", S_OK },
    { "<a attr1=\" \"/>", "attr1", " ", S_OK },
    { "<a attr1=\"\r\n \r \n \t\n\r\"/>", "attr1", "         ", S_OK },
    { "<a attr1=\" val \"/>", "attr1", " val ", S_OK },
    { "<a attr1=\"\r\n\tval\n\"/>", "attr1", "  val ", S_OK },
    { "<a attr1=\"val&#32;\"/>", "attr1", "val ", S_OK },
    { "<a attr1=\"val&#x20;\"/>", "attr1", "val ", S_OK },
    { "<a attr1=\"&lt;&gt;&amp;&apos;&quot;\"/>", "attr1", "<>&\'\"", S_OK },
    { "<a attr1=\"&entname;\"/>", NULL, NULL, WC_E_UNDECLAREDENTITY },
    { "<a attr1=\"val&#xfffe;\"/>", NULL, NULL, WC_E_XMLCHARACTER },
    { "<a attr1=\"val &#a;\"/>", NULL, NULL, WC_E_DIGIT, WC_E_SEMICOLON },
    { "<a attr1=\"val &#12a;\"/>", NULL, NULL, WC_E_SEMICOLON },
    { "<a attr1=\"val &#x12g;\"/>", NULL, NULL, WC_E_SEMICOLON },
    { "<a attr1=\"val &#xg;\"/>", NULL, NULL, WC_E_HEXDIGIT, WC_E_SEMICOLON },
    { "<a attr1=attrvalue/>", NULL, NULL, WC_E_QUOTE },
    { "<a attr1=\"attr<value\"/>", NULL, NULL, WC_E_LESSTHAN },
    { "<a attr1=\"&entname\"/>", NULL, NULL, WC_E_SEMICOLON },
    { NULL }
};

static void test_read_attribute(void)
{
    struct test_entry *test = attributes_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;

        set_input_string(reader, test->xml);

        hr = IXmlReader_Read(reader, NULL);

        if (test->hr_broken)
            ok(hr == test->hr || broken(hr == test->hr_broken), "got %08x for %s\n", hr, test->xml);
        else
            ok(hr == test->hr, "got %08x for %s\n", hr, test->xml);
        if (hr == S_OK)
        {
            const WCHAR *str;
            WCHAR *str_exp;
            UINT len;

            type = XmlNodeType_None;
            hr = IXmlReader_GetNodeType(reader, &type);
            ok(hr == S_OK, "Failed to get node type, %#x\n", hr);

            ok(type == XmlNodeType_Element, "got %d for %s\n", type, test->xml);

            hr = IXmlReader_MoveToFirstAttribute(reader);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            reader_name(reader, test->name);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            reader_value(reader, test->value);
        }

        test++;
    }

    IXmlReader_Release(reader);
}

static void test_reader_properties(void)
{
    IXmlReader *reader;
    LONG_PTR value;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    value = 0;
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_MaxElementDepth, &value);
    ok(hr == S_OK, "GetProperty failed: %08x\n", hr);
    ok(value == 256, "Unexpected default max depth value %ld\n", value);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_MultiLanguage, 0);
    ok(hr == S_OK, "SetProperty failed: %08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_MaxElementDepth, 0);
    ok(hr == S_OK, "SetProperty failed: %08x\n", hr);

    value = 256;
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_MaxElementDepth, &value);
    ok(hr == S_OK, "GetProperty failed: %08x\n", hr);
    ok(value == 0, "Unexpected max depth value %ld\n", value);

    IXmlReader_Release(reader);
}

static void test_prefix(void)
{
    static const struct
    {
        const char *xml;
        const char *prefix1;
        const char *prefix2;
        const char *prefix3;
    } prefix_tests[] =
    {
        { "<b xmlns=\"defns\" xml:a=\"a ns\"/>", "", "", "xml" },
        { "<c:b xmlns:c=\"c ns\" xml:a=\"a ns\"/>", "c", "xmlns", "xml" },
    };
    IXmlReader *reader;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(prefix_tests); i++) {
        XmlNodeType type;

        set_input_string(reader, prefix_tests[i].xml);

        hr = IXmlReader_Read(reader, &type);
        ok(hr == S_OK, "Read() failed, %#x\n", hr);
        ok(type == XmlNodeType_Element, "Unexpected node type %d.\n", type);

        reader_prefix(reader, prefix_tests[i].prefix1);

        hr = IXmlReader_MoveToFirstAttribute(reader);
        ok(hr == S_OK, "MoveToFirstAttribute() failed, %#x.\n", hr);

        hr = IXmlReader_GetNodeType(reader, &type);
        ok(hr == S_OK, "GetNodeType() failed, %#x.\n", hr);
        ok(type == XmlNodeType_Attribute, "Unexpected node type %d.\n", type);

        reader_prefix(reader, prefix_tests[i].prefix2);

        next_attribute(reader);

        hr = IXmlReader_GetNodeType(reader, &type);
        ok(hr == S_OK, "GetNodeType() failed, %#x.\n", hr);
        ok(type == XmlNodeType_Attribute, "Unexpected node type %d.\n", type);

        reader_prefix(reader, prefix_tests[i].prefix3);

        /* back to the element, check prefix */
        move_to_element(reader);
        reader_prefix(reader, prefix_tests[i].prefix1);
    }

    IXmlReader_Release(reader);
}

static void test_namespaceuri(void)
{
    struct uri_test
    {
        const char *xml;
        const char *uri[5];
    } uri_tests[] =
    {
        { "<a xmlns=\"defns a\"><b xmlns=\"defns b\"><c xmlns=\"defns c\"/></b></a>",
                { "defns a", "defns b", "defns c", "defns b", "defns a" }},
        { "<r:a xmlns=\"defns a\" xmlns:r=\"ns r\"/>",
                { "ns r" }},
        { "<r:a xmlns=\"defns a\" xmlns:r=\"ns r\"><b/></r:a>",
                { "ns r", "defns a", "ns r" }},
        { "<a xmlns=\"defns a\" xmlns:r=\"ns r\"><r:b/></a>",
                { "defns a", "ns r", "defns a" }},
        { "<a><b><c/></b></a>",
                { "", "", "", "", "" }},
        { "<a>text</a>",
                { "", "", "" }},
        { "<a>\r\n</a>",
                { "", "", "" }},
        { "<a><![CDATA[data]]></a>",
                { "", "", "" }},
        { "<?xml version=\"1.0\" ?><a/>",
                { "", "" }},
        { "<a><?pi ?></a>",
                { "", "", "" }},
        { "<a><!-- comment --></a>",
                { "", "", "" }},
    };
    IXmlReader *reader;
    XmlNodeType type;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    for (i = 0; i < ARRAY_SIZE(uri_tests); i++) {
        unsigned int j = 0;

        set_input_string(reader, uri_tests[i].xml);

        type = ~0u;
        while (IXmlReader_Read(reader, &type) == S_OK) {
            const WCHAR *local, *qname;
            UINT length, length2;

            ok(type == XmlNodeType_Element ||
                    type == XmlNodeType_Text ||
                    type == XmlNodeType_CDATA ||
                    type == XmlNodeType_ProcessingInstruction ||
                    type == XmlNodeType_Comment ||
                    type == XmlNodeType_Whitespace ||
                    type == XmlNodeType_EndElement ||
                    type == XmlNodeType_XmlDeclaration, "Unexpected node type %d.\n", type);

            local = NULL;
            length = 0;
            hr = IXmlReader_GetLocalName(reader, &local, &length);
            ok(hr == S_OK, "S_OK, got %08x\n", hr);
            ok(local != NULL, "Unexpected NULL local name pointer\n");

            qname = NULL;
            length2 = 0;
            hr = IXmlReader_GetQualifiedName(reader, &qname, &length2);
            ok(hr == S_OK, "S_OK, got %08x\n", hr);
            ok(qname != NULL, "Unexpected NULL qualified name pointer\n");

            if (type == XmlNodeType_Element ||
                    type == XmlNodeType_EndElement ||
                    type == XmlNodeType_ProcessingInstruction ||
                    type == XmlNodeType_XmlDeclaration)
            {
                ok(*local != 0, "Unexpected empty local name\n");
                ok(length > 0, "Unexpected local name length\n");

                ok(*qname != 0, "Unexpected empty qualified name\n");
                ok(length2 > 0, "Unexpected qualified name length\n");
            }

            reader_namespace(reader, uri_tests[i].uri[j]);

            j++;
        }
        ok(type == XmlNodeType_None, "Unexpected node type %d\n", type);
    }

    IXmlReader_Release(reader);
}

static void test_read_charref(void)
{
    static const char testA[] = "<a b=\"c\">&#x1f3;&#x103;&gt;</a>";
    static const WCHAR chardataW[] = {0x01f3,0x0103,'>',0};
    const WCHAR *value;
    IXmlReader *reader;
    XmlNodeType type;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, testA);

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Element, "Unexpected node type %d\n", type);

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Text, "Unexpected node type %d\n", type);

    hr = IXmlReader_GetValue(reader, &value, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(!lstrcmpW(value, chardataW), "Text value : %s\n", wine_dbgstr_w(value));

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_EndElement, "Unexpected node type %d\n", type);

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(type == XmlNodeType_None, "Unexpected node type %d\n", type);

    IXmlReader_Release(reader);
}

static void test_encoding_detection(void)
{
    static const struct encoding_testW
    {
        WCHAR text[16];
    }
    encoding_testsW[] =
    {
        { { '<','?','p','i',' ','?','>',0 } },
        { { '<','!','-','-',' ','c','-','-','>',0 } },
        { { 0xfeff,'<','a','/','>',0 } },
        { { '<','a','/','>',0 } },
    };
    static const char *encoding_testsA[] =
    {
        "<?pi ?>",
        "<!-- comment -->",
        "\xef\xbb\xbf<a/>", /* UTF-8 BOM */
        "<a/>",
    };
    IXmlReader *reader;
    XmlNodeType type;
    IStream *stream;
    unsigned int i;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    /* there's no way to query detected encoding back, so just verify that document is browsable */

    for (i = 0; i < ARRAY_SIZE(encoding_testsA); i++)
    {
        set_input_string(reader, encoding_testsA[i]);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        ok(hr == S_OK, "got %08x\n", hr);
        ok(type != XmlNodeType_None, "Unexpected node type %d\n", type);
    }

    for (i = 0; i < ARRAY_SIZE(encoding_testsW); i++)
    {
        stream = create_stream_on_data(encoding_testsW[i].text, lstrlenW(encoding_testsW[i].text) * sizeof(WCHAR));

        hr = IXmlReader_SetInput(reader, (IUnknown *)stream);
        ok(hr == S_OK, "got %08x\n", hr);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        ok(hr == S_OK, "%u: got %08x\n", i, hr);
        ok(type != XmlNodeType_None, "%u: unexpected node type %d\n", i, type);

        IStream_Release(stream);
    }

    IXmlReader_Release(reader);
}

static void test_eof_state(IXmlReader *reader, BOOL eof)
{
    LONG_PTR state;
    HRESULT hr;

    ok(IXmlReader_IsEOF(reader) == eof, "Unexpected IsEOF() result\n");
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, &state);
    ok(hr == S_OK, "GetProperty() failed, %#x\n", hr);
    ok((state == XmlReadState_EndOfFile) == eof, "Unexpected EndOfFile state %ld\n", state);
}

static void test_endoffile(void)
{
    IXmlReader *reader;
    XmlNodeType type;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    test_eof_state(reader, FALSE);

    set_input_string(reader, "<a/>");

    test_eof_state(reader, FALSE);

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(type == XmlNodeType_Element, "Unexpected type %d\n", type);

    test_eof_state(reader, FALSE);

    type = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(type == XmlNodeType_None, "Unexpected type %d\n", type);

    test_eof_state(reader, TRUE);

    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    test_eof_state(reader, FALSE);

    IXmlReader_Release(reader);

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, "<a/>text");

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(type == XmlNodeType_Element, "Unexpected type %d\n", type);

    test_eof_state(reader, FALSE);

    type = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == WC_E_SYNTAX, "got %#x\n", hr);
    ok(type == XmlNodeType_None, "Unexpected type %d\n", type);

    test_eof_state(reader, FALSE);

    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    IXmlReader_Release(reader);
}

static void test_max_element_depth(void)
{
    static const char *xml =
        "<a>"
            "<b attrb=\"_b\">"
                "<c>"
                   "<d></d>"
                "</c>"
            "</b>"
        "</a>";
    XmlNodeType nodetype;
    unsigned int count;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, xml);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_MaxElementDepth, 2);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 0);

    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 0);

    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 1);
    TEST_READER_STATE(reader, XmlReadState_Interactive);

    hr = IXmlReader_Read(reader, NULL);
    ok(hr == SC_E_MAXELEMENTDEPTH, "got %08x\n", hr);

    TEST_DEPTH2(reader, 0, 2);
    TEST_READER_STATE(reader, XmlReadState_Error);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_MaxElementDepth, 10);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXmlReader_Read(reader, NULL);
    ok(hr == SC_E_MAXELEMENTDEPTH, "got %08x\n", hr);

    TEST_DEPTH2(reader, 0, 2);
    TEST_READER_STATE(reader, XmlReadState_Error);

    /* test if stepping into attributes enforces depth limit too */
    set_input_string(reader, xml);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_MaxElementDepth, 2);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 0);

    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 0);

    hr = IXmlReader_Read(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 1);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);

    TEST_DEPTH(reader, 2);
    TEST_READER_STATE(reader, XmlReadState_Interactive);

    nodetype = 123;
    hr = IXmlReader_Read(reader, &nodetype);
    ok(hr == SC_E_MAXELEMENTDEPTH, "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "got node type %d\n", nodetype);

    nodetype = 123;
    hr = IXmlReader_Read(reader, &nodetype);
    ok(hr == SC_E_MAXELEMENTDEPTH, "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "got node type %d\n", nodetype);

    TEST_DEPTH2(reader, 0, 2);
    TEST_READER_STATE(reader, XmlReadState_Error);

    /* set max depth to 0, this disables depth limit */
    set_input_string(reader, xml);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_MaxElementDepth, 0);
    ok(hr == S_OK, "got %08x\n", hr);

    count = 0;
    while (IXmlReader_Read(reader, NULL) == S_OK)
        count++;
    ok(count == 8, "Unexpected node number %u\n", count);
    TEST_READER_STATE(reader, XmlReadState_EndOfFile);

    IXmlReader_Release(reader);
}

static void test_reader_position(void)
{
    static const char *xml = "<c:a xmlns:c=\"nsdef c\" b=\"attr b\">\n</c:a>";
    IXmlReader *reader;
    XmlNodeType type;
    UINT position;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    TEST_READER_STATE(reader, XmlReadState_Closed);

    /* position methods with Null args */
    hr = IXmlReader_GetLineNumber(reader, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXmlReader_GetLinePosition(reader, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    position = 123;
    hr = IXmlReader_GetLinePosition(reader, &position);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(position == 0, "got %u\n", position);

    position = 123;
    hr = IXmlReader_GetLineNumber(reader, &position);
    ok(hr == S_FALSE, "got %#x\n", hr);
    ok(position == 0, "got %u\n", position);

    set_input_string(reader, xml);

    TEST_READER_STATE(reader, XmlReadState_Initial);
    TEST_READER_POSITION(reader, 0, 0);
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Element, "got type %d\n", type);
    TEST_READER_POSITION2(reader, 1, 2, ~0u, 34);

    next_attribute(reader);
    TEST_READER_POSITION2(reader, 1, 6, ~0u, 34);

    next_attribute(reader);
    TEST_READER_POSITION2(reader, 1, 24, ~0u, 34);

    move_to_element(reader);
    TEST_READER_POSITION2(reader, 1, 2, ~0u, 34);

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Whitespace, "got type %d\n", type);
    TEST_READER_POSITION2(reader, 1, 35, 2, 6);

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_EndElement, "got type %d\n", type);
    TEST_READER_POSITION2(reader, 2, 3, 2, 6);

    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    TEST_READER_STATE2(reader, XmlReadState_Initial, XmlReadState_Closed);
    TEST_READER_POSITION(reader, 0, 0);

    IXmlReader_Release(reader);
}

static void test_string_pointers(void)
{
    const WCHAR *ns, *nsq, *empty, *xmlns_ns, *xmlns_name, *name, *p, *q, *xml, *ptr, *value;
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    set_input_string(reader, "<elem xmlns=\"myns\">myns<elem2 /></elem>");

    read_node(reader, XmlNodeType_Element);
    empty = reader_value(reader, "");
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    name = reader_name(reader, "elem");
    ok(name == reader_qname(reader, "elem"), "name != qname\n");
    ns = reader_namespace(reader, "myns");

    next_attribute(reader);
    ptr = reader_value(reader, "myns");
    if (ns != ptr)
    {
        win_skip("attr value is different than namespace pointer, assuming old xmllite\n");
        IXmlReader_Release(reader);
        return;
    }
    ok(ns == ptr, "ns != value\n");
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    xmlns_ns = reader_namespace(reader, "http://www.w3.org/2000/xmlns/");
    xmlns_name = reader_name(reader, "xmlns");
    ok(xmlns_name == reader_qname(reader, "xmlns"), "xmlns_name != qname\n");

    read_node(reader, XmlNodeType_Text);
    ok(ns != reader_value(reader, "myns"), "ns == value\n");
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    ok(empty == reader_namespace(reader, ""), "empty != namespace\n");
    ok(empty == reader_name(reader, ""), "empty != name\n");
    ok(empty == reader_qname(reader, ""), "empty != qname\n");

    read_node(reader, XmlNodeType_Element);
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    ok(ns == reader_namespace(reader, "myns"), "empty != namespace\n");

    read_node(reader, XmlNodeType_EndElement);
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    ok(name == reader_name(reader, "elem"), "empty != name\n");
    ok(name == reader_qname(reader, "elem"), "empty != qname\n");
    ok(ns == reader_namespace(reader, "myns"), "empty != namespace\n");

    set_input_string(reader, "<elem xmlns:p=\"myns\" xmlns:q=\"mynsq\"><p:elem2 q:attr=\"\"></p:elem2></elem>");

    read_node(reader, XmlNodeType_Element);
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    name = reader_name(reader, "elem");
    ok(empty == reader_namespace(reader, ""), "empty != namespace\n");

    next_attribute(reader);
    ns = reader_value(reader, "myns");
    ok(xmlns_name == reader_prefix(reader, "xmlns"), "xmlns_name != prefix\n");
    p = reader_name(reader, "p");
    ok(xmlns_ns == reader_namespace(reader, "http://www.w3.org/2000/xmlns/"), "xmlns_ns != namespace\n");

    next_attribute(reader);
    nsq = reader_value(reader, "mynsq");
    ok(xmlns_name == reader_prefix(reader, "xmlns"), "xmlns_name != prefix\n");
    q = reader_name(reader, "q");
    ok(xmlns_ns == reader_namespace(reader, "http://www.w3.org/2000/xmlns/"), "xmlns_ns != namespace\n");

    read_node(reader, XmlNodeType_Element);
    ok(p == reader_prefix(reader, "p"), "p != prefix\n");
    ok(ns == reader_namespace(reader, "myns"), "empty != namespace\n");
    name = reader_qname(reader, "p:elem2");

    next_attribute(reader);
    ok(empty != reader_value(reader, ""), "empty == value\n");
    ok(q == reader_prefix(reader, "q"), "q != prefix\n");
    ok(nsq == reader_namespace(reader, "mynsq"), "nsq != namespace\n");

    read_node(reader, XmlNodeType_EndElement);
    ptr = reader_qname(reader, "p:elem2"); todo_wine ok(name != ptr, "q == qname\n");

    set_input_string(reader, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");

    read_node(reader, XmlNodeType_XmlDeclaration);
    ok(empty == reader_value(reader, ""), "empty != value\n");
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    xml = reader_name(reader, "xml");
    ptr = reader_qname(reader, "xml"); todo_wine ok(xml == ptr, "xml != qname\n");
    ok(empty == reader_namespace(reader, ""), "empty != namespace\n");

    next_attribute(reader);
    ok(empty == reader_prefix(reader, ""), "empty != prefix\n");
    ok(empty == reader_namespace(reader, ""), "empty != namespace\n");

    set_input_string(reader, "<elem xmlns:p=\"myns\"><p:elem2 attr=\"\" /></elem>");

    read_node(reader, XmlNodeType_Element);
    next_attribute(reader);
    read_value_char(reader, 'm');
    p = reader_value(reader, "yns");

    read_node(reader, XmlNodeType_Element);
    ns = reader_namespace(reader, "myns");
    ok(ns+1 == p, "ns+1 != p\n");

    set_input_string(reader, "<elem attr=\"value\"></elem>");

    read_node(reader, XmlNodeType_Element);
    next_attribute(reader);
    name = reader_name(reader, "attr");
    value = reader_value(reader, "value");

    move_to_element(reader);
    next_attribute(reader);
    ok(name == reader_name(reader, "attr"), "attr pointer changed\n");
    ok(value == reader_value(reader, "value"), "value pointer changed\n");

    IXmlReader_Release(reader);
}

static void test_attribute_by_name(void)
{
    static const char *xml = "<a><elem xmlns=\"myns\" a=\"value a\" b=\"value b\" xmlns:ns=\"ns uri\" "
        "ns:c=\"value c\" c=\"value c2\"/></a>";
    static const WCHAR xmlns_uriW[] = {'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g','/',
        '2','0','0','0','/','x','m','l','n','s','/',0};
    static const WCHAR nsuriW[] = {'n','s',' ','u','r','i',0};
    static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};
    static const WCHAR mynsW[] = {'m','y','n','s',0};
    static const WCHAR nsW[] = {'n','s',0};
    static const WCHAR emptyW[] = {0};
    static const WCHAR aW[] = {'a',0};
    static const WCHAR bW[] = {'b',0};
    static const WCHAR cW[] = {'c',0};
    IXmlReader *reader;
    HRESULT hr;

    hr = CreateXmlReader(&IID_IXmlReader, (void **)&reader, NULL);
    ok(hr == S_OK, "Failed to create reader, hr %#x.\n", hr);

    set_input_string(reader, xml);

    hr = IXmlReader_MoveToAttributeByName(reader, NULL, NULL);
    ok(hr == E_INVALIDARG || broken(hr == S_FALSE) /* WinXP */, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, emptyW, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    read_node(reader, XmlNodeType_Element);

    hr = IXmlReader_MoveToAttributeByName(reader, emptyW, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    read_node(reader, XmlNodeType_Element);

    hr = IXmlReader_MoveToAttributeByName(reader, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, NULL, xmlns_uriW);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, emptyW, xmlns_uriW);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, xmlnsW, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, xmlnsW, xmlns_uriW);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "myns");

    hr = IXmlReader_MoveToAttributeByName(reader, aW, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "value a");

    hr = IXmlReader_MoveToAttributeByName(reader, bW, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "value b");

    hr = IXmlReader_MoveToAttributeByName(reader, aW, mynsW);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, nsW, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#x.\n", hr);

    hr = IXmlReader_MoveToAttributeByName(reader, nsW, xmlns_uriW);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "ns uri");

    hr = IXmlReader_MoveToAttributeByName(reader, bW, emptyW);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "value b");

    hr = IXmlReader_MoveToAttributeByName(reader, cW, NULL);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "value c2");

    hr = IXmlReader_MoveToAttributeByName(reader, cW, nsuriW);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    reader_value(reader, "value c");

    IXmlReader_Release(reader);
}

START_TEST(reader)
{
    test_reader_create();
    test_readerinput();
    test_reader_state();
    test_read_attribute();
    test_read_cdata();
    test_read_comment();
    test_read_pi();
    test_read_system_dtd();
    test_read_public_dtd();
    test_read_element();
    test_isemptyelement();
    test_read_text();
    test_read_full();
    test_read_pending();
    test_readvaluechunk();
    test_read_xmldeclaration();
    test_reader_properties();
    test_prefix();
    test_namespaceuri();
    test_read_charref();
    test_encoding_detection();
    test_endoffile();
    test_max_element_depth();
    test_reader_position();
    test_string_pointers();
    test_attribute_by_name();
}
