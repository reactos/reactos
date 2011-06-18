/*
 * XMLLite IXmlReader tests
 *
 * Copyright 2010 (C) Nikolay Sivov
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "xmllite.h"
#include "wine/test.h"

DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

HRESULT (WINAPI *pCreateXmlReader)(REFIID riid, void **ppvObject, IMalloc *pMalloc);
HRESULT (WINAPI *pCreateXmlReaderInputWithEncodingName)(IUnknown *stream,
                                                        IMalloc *pMalloc,
                                                        LPCWSTR encoding,
                                                        BOOL hint,
                                                        LPCWSTR base_uri,
                                                        IXmlReaderInput **ppInput);
static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static const char xmldecl_full[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

static IStream *create_stream_on_data(const char *data, int size)
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

static void ok_pos_(IXmlReader *reader, int line, int pos, int line_broken,
                                           int pos_broken, int todo, int _line_)
{
    UINT l, p;
    HRESULT hr;
    int broken_state;

    hr = IXmlReader_GetLineNumber(reader, &l);
    ok_(__FILE__, _line_)(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IXmlReader_GetLinePosition(reader, &p);
    ok_(__FILE__, _line_)(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    if (line_broken == -1 && pos_broken == -1)
        broken_state = 0;
    else
        broken_state = broken((line_broken == -1 ? line : line_broken) == l &&
                              (pos_broken == -1 ? pos : pos_broken) == p);

    if (todo)
        todo_wine
        ok_(__FILE__, _line_)((l == line && pos == p) || broken_state,
                            "Expected (%d,%d), got (%d,%d)\n", line, pos, l, p);
    else
    {
        ok_(__FILE__, _line_)((l == line && pos == p) || broken_state,
                            "Expected (%d,%d), got (%d,%d)\n", line, pos, l, p);
    }
}
#define ok_pos(reader, l, p, l_brk, p_brk, todo) ok_pos_(reader, l, p, l_brk, p_brk, todo, __LINE__)

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

static void ok_iids_(const input_iids_t *iids, const IID **expected, const IID **exp_broken, int todo, int line)
{
    int i = 0, size = 0;

    while (expected[i++]) size++;

    if (todo) {
        todo_wine
            ok_(__FILE__, line)(iids->count == size, "Sequence size mismatch (%d), got (%d)\n", size, iids->count);
    }
    else
       ok_(__FILE__, line)(iids->count == size, "Sequence size mismatch (%d), got (%d)\n", size, iids->count);

    if (iids->count != size) return;

    for (i = 0; i < size; i++) {
        ok_(__FILE__, line)(IsEqualGUID(&iids->iids[i], expected[i]) ||
            (exp_broken ? broken(IsEqualGUID(&iids->iids[i], exp_broken[i])) : FALSE),
            "Wrong IID(%d), got (%s)\n", i, debugstr_guid(&iids->iids[i]));
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

static void test_read_state_(IXmlReader *reader, XmlReadState expected,
                                    XmlReadState exp_broken, int todo, int line)
{
    XmlReadState state;
    HRESULT hr;
    int broken_state;

    state = -1; /* invalid value */
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, (LONG_PTR*)&state);
    ok_(__FILE__, line)(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    if (exp_broken == -1)
        broken_state = 0;
    else
        broken_state = broken(exp_broken == state);

    if (todo)
    {
    todo_wine
        ok_(__FILE__, line)(state == expected || broken_state, "Expected (%s), got (%s)\n",
                                   state_to_str(expected), state_to_str(state));
    }
    else
        ok_(__FILE__, line)(state == expected || broken_state, "Expected (%s), got (%s)\n",
                                   state_to_str(expected), state_to_str(state));
}

#define test_read_state(reader, exp, brk, todo) test_read_state_(reader, exp, brk, todo, __LINE__)

typedef struct _testinput
{
    const IUnknownVtbl *lpVtbl;
    LONG ref;
} testinput;

static inline testinput *impl_from_IUnknown(IUnknown *iface)
{
    return (testinput *)((char*)iface - FIELD_OFFSET(testinput, lpVtbl));
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
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

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

    input = HeapAlloc(GetProcessHeap(), 0, sizeof (*input));
    if(!input) return E_OUTOFMEMORY;

    input->lpVtbl = &testinput_vtbl;
    input->ref = 1;

    *ppObj = &input->lpVtbl;

    return S_OK;
}

static BOOL init_pointers(void)
{
    /* don't free module here, it's to be unloaded on exit */
    HMODULE mod = LoadLibraryA("xmllite.dll");

    if (!mod)
    {
        win_skip("xmllite library not available\n");
        return FALSE;
    }

#define MAKEFUNC(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return FALSE;
    MAKEFUNC(CreateXmlReader);
    MAKEFUNC(CreateXmlReaderInputWithEncodingName);
#undef MAKEFUNC

    return TRUE;
}

static void test_reader_create(void)
{
    HRESULT hr;
    IXmlReader *reader;
    IUnknown *input;

    /* crashes native */
    if (0)
    {
        hr = pCreateXmlReader(&IID_IXmlReader, NULL, NULL);
        hr = pCreateXmlReader(NULL, (LPVOID*)&reader, NULL);
    }

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_read_state(reader, XmlReadState_Closed, -1, FALSE);

    /* Null input pointer, releases previous input */
    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_read_state(reader, XmlReadState_Initial, XmlReadState_Closed, FALSE);

    /* test input interface selection sequence */
    hr = testinput_createinstance((void**)&input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    hr = IXmlReader_SetInput(reader, input);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok_iids(&input_iids, setinput_full, setinput_full_old, FALSE);

    IUnknown_Release(input);

    IXmlReader_Release(reader);
}

static void test_readerinput(void)
{
    IXmlReaderInput *reader_input;
    IXmlReader *reader, *reader2;
    IUnknown *obj, *input;
    IStream *stream;
    HRESULT hr;
    LONG ref;

    hr = pCreateXmlReaderInputWithEncodingName(NULL, NULL, NULL, FALSE, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    hr = pCreateXmlReaderInputWithEncodingName(NULL, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IStream_AddRef(stream);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IStream_Release(stream);
    hr = pCreateXmlReaderInputWithEncodingName((IUnknown*)stream, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* IXmlReaderInput grabs a stream reference */
    ref = IStream_AddRef(stream);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IStream_Release(stream);

    /* try ::SetInput() with valid IXmlReaderInput */
    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);

    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_read_state(reader, XmlReadState_Initial, -1, FALSE);

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

    test_read_state(reader, XmlReadState_Initial, XmlReadState_Closed, FALSE);

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
    hr = testinput_createinstance((void**)&input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    input_iids.count = 0;
    ref = IUnknown_AddRef(input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(input);
    hr = pCreateXmlReaderInputWithEncodingName(input, NULL, NULL, FALSE, NULL, &reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok_iids(&input_iids, empty_seq, NULL, FALSE);
    /* IXmlReaderInput stores stream interface as IUnknown */
    ref = IUnknown_AddRef(input);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IUnknown_Release(input);

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
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

    test_read_state(reader, XmlReadState_Closed, -1, FALSE);

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
    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader2, NULL);
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
    IXmlReader *reader;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* invalid arguments */
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    IXmlReader_Release(reader);
}

static void test_read_xmldeclaration(void)
{
    IXmlReader *reader;
    IStream *stream;
    HRESULT hr;
    XmlNodeType type;
    UINT count = 0;

    hr = pCreateXmlReader(&IID_IXmlReader, (LPVOID*)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* position methods with Null args */
    hr = IXmlReader_GetLineNumber(reader, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXmlReader_GetLinePosition(reader, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    stream = create_stream_on_data(xmldecl_full, sizeof(xmldecl_full));

    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ok_pos(reader, 0, 0, -1, -1, FALSE);

    type = -1;
    hr = IXmlReader_Read(reader, &type);
todo_wine {
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XmlNodeType_XmlDeclaration,
                     "Expected XmlNodeType_XmlDeclaration, got %s\n", type_to_str(type));
}
    /* new version 1.2.x and 1.3.x properly update postition for <?xml ?> */
    ok_pos(reader, 1, 3, -1, 55, TRUE);

    /* check attributes */
    hr = IXmlReader_MoveToNextAttribute(reader);
    todo_wine ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok_pos(reader, 1, 7, -1, 55, TRUE);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    todo_wine ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok_pos(reader, 1, 7, -1, 55, TRUE);

    hr = IXmlReader_GetAttributeCount(reader, &count);
todo_wine {
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(count == 3, "Expected 3, got %d\n", count);
}
    hr = IXmlReader_GetDepth(reader, &count);
todo_wine {
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(count == 1, "Expected 1, got %d\n", count);
}

    IStream_Release(stream);
    IXmlReader_Release(reader);
}

START_TEST(reader)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    if (!init_pointers())
    {
       CoUninitialize();
       return;
    }

    test_reader_create();
    test_readerinput();
    test_reader_state();
    test_read_xmldeclaration();

    CoUninitialize();
}
