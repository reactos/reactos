/*
 * IXmlReader tests
 *
 * Copyright 2010, 2012-2013 Nikolay Sivov
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

#define COBJMACROS
#define CONST_VTABLE

//#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <initguid.h>
#include <ole2.h>
#include <xmllite.h>
#include <wine/test.h>

DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

static HRESULT (WINAPI *pCreateXmlReader)(REFIID riid, void **ppvObject, IMalloc *pMalloc);
static HRESULT (WINAPI *pCreateXmlReaderInputWithEncodingName)(IUnknown *stream,
                                                        IMalloc *pMalloc,
                                                        LPCWSTR encoding,
                                                        BOOL hint,
                                                        LPCWSTR base_uri,
                                                        IXmlReaderInput **ppInput);

static WCHAR *a2w(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    WCHAR *ret = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static void free_str(WCHAR *str)
{
    HeapFree(GetProcessHeap(), 0, str);
}

static const char xmldecl_full[] = "\xef\xbb\xbf<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
static const char xmldecl_short[] = "<?xml version=\"1.0\"?><RegistrationInfo/>";

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
                                        int pos_broken, BOOL todo, int _line_)
{
    UINT l, p;
    HRESULT hr;
    BOOL broken_state;

    hr = IXmlReader_GetLineNumber(reader, &l);
    ok_(__FILE__, _line_)(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IXmlReader_GetLinePosition(reader, &p);
    ok_(__FILE__, _line_)(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    if (line_broken == -1 && pos_broken == -1)
        broken_state = FALSE;
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

static void ok_iids_(const input_iids_t *iids, const IID **expected, const IID **exp_broken, BOOL todo, int line)
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

static void test_read_state_(IXmlReader *reader, XmlReadState expected,
                             XmlReadState exp_broken, BOOL todo, int line)
{
    LONG_PTR state;
    HRESULT hr;
    BOOL broken_state;

    state = -1; /* invalid value */
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, &state);
    ok_(__FILE__, line)(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    if (exp_broken == -1)
        broken_state = FALSE;
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
    DtdProcessing dtd;
    XmlNodeType nodetype;

    /* crashes native */
    if (0)
    {
        pCreateXmlReader(&IID_IXmlReader, NULL, NULL);
        pCreateXmlReader(NULL, (void**)&reader, NULL);
    }

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_read_state(reader, XmlReadState_Closed, -1, FALSE);

    nodetype = XmlNodeType_Element;
    hr = IXmlReader_GetNodeType(reader, &nodetype);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(nodetype == XmlNodeType_None, "got %d\n", nodetype);

    dtd = 2;
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_DtdProcessing, (LONG_PTR*)&dtd);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(dtd == DtdProcessing_Prohibit, "got %d\n", dtd);

    dtd = 2;
    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, dtd);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, -1);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    /* Null input pointer, releases previous input */
    hr = IXmlReader_SetInput(reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_read_state(reader, XmlReadState_Initial, XmlReadState_Closed, FALSE);

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

    hr = IUnknown_QueryInterface(reader_input, &IID_IStream, (void**)&stream2);
    ok(hr == E_NOINTERFACE, "Expected S_OK, got %08x\n", hr);

    hr = IUnknown_QueryInterface(reader_input, &IID_ISequentialStream, (void**)&stream2);
    ok(hr == E_NOINTERFACE, "Expected S_OK, got %08x\n", hr);

    /* IXmlReaderInput grabs a stream reference */
    ref = IStream_AddRef(stream);
    ok(ref == 3, "Expected 3, got %d\n", ref);
    IStream_Release(stream);

    /* try ::SetInput() with valid IXmlReaderInput */
    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    ref = IUnknown_AddRef(reader_input);
    ok(ref == 2, "Expected 2, got %d\n", ref);
    IUnknown_Release(reader_input);

    hr = IXmlReader_SetInput(reader, reader_input);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_read_state(reader, XmlReadState_Initial, -1, FALSE);

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
    input = NULL;
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
    XmlNodeType nodetype;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* invalid arguments */
    hr = IXmlReader_GetProperty(reader, XmlReaderProperty_ReadState, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    /* attempt to read on closed reader */
    test_read_state(reader, XmlReadState_Closed, -1, FALSE);
if (0)
{
    /* newer versions crash here, probably cause no input was set */
    hr = IXmlReader_Read(reader, &nodetype);
    ok(hr == S_FALSE, "got %08x\n", hr);
}
    IXmlReader_Release(reader);
}

static void test_read_xmldeclaration(void)
{
    static const WCHAR xmlW[] = {'x','m','l',0};
    static const WCHAR RegistrationInfoW[] = {'R','e','g','i','s','t','r','a','t','i','o','n','I','n','f','o',0};
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

    ok_pos(reader, 0, 0, -1, -1, FALSE);

    type = -1;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XmlNodeType_XmlDeclaration,
                     "Expected XmlNodeType_XmlDeclaration, got %s\n", type_to_str(type));
    /* new version 1.2.x and 1.3.x properly update position for <?xml ?> */
    ok_pos(reader, 1, 3, -1, 55, TRUE);
    test_read_state(reader, XmlReadState_Interactive, -1, FALSE);

    hr = IXmlReader_GetValue(reader, &val, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(*val == 0, "got %s\n", wine_dbgstr_w(val));

    /* check attributes */
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    ok_pos(reader, 1, 7, -1, 55, TRUE);

    /* try to move from last attribute */
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_FALSE, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);

    hr = IXmlReader_MoveToFirstAttribute(reader);
    ok(hr == S_OK, "got %08x\n", hr);
    ok_pos(reader, 1, 7, -1, 55, TRUE);

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

    hr = IXmlReader_GetDepth(reader, &count);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(count == 1, "Expected 1, got %d\n", count);

    hr = IXmlReader_MoveToElement(reader);
    ok(hr == S_OK, "got %08x\n", hr);

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

    type = -1;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(type == XmlNodeType_XmlDeclaration, "expected XmlDeclaration, got %s\n", type_to_str(type));
    ok_pos(reader, 1, 3, 1, 21, TRUE);
    test_read_state(reader, XmlReadState_Interactive, -1, TRUE);

    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(count == 1, "expected 1, got %d\n", count);

    ret = IXmlReader_IsEmptyElement(reader);
    ok(!ret, "element should not be empty\n");

    hr = IXmlReader_GetValue(reader, &val, NULL);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(*val == 0, "got %s\n", wine_dbgstr_w(val));

    hr = IXmlReader_GetLocalName(reader, &val, NULL);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
todo_wine
    ok(!lstrcmpW(val, xmlW), "got %s\n", wine_dbgstr_w(val));

    /* check attributes */
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);

    type = -1;
    hr = IXmlReader_GetNodeType(reader, &type);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(type == XmlNodeType_Attribute, "got %d\n", type);
    ok_pos(reader, 1, 7, 1, 21, TRUE);

    /* try to move from last attribute */
    hr = IXmlReader_MoveToNextAttribute(reader);
    ok(hr == S_FALSE, "expected S_FALSE, got %08x\n", hr);

    type = -1;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(type == XmlNodeType_Element, "expected Element, got %s\n", type_to_str(type));
    ok_pos(reader, 1, 23, 1, 40, TRUE);
    test_read_state(reader, XmlReadState_Interactive, -1, TRUE);

    hr = IXmlReader_GetAttributeCount(reader, &count);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(count == 0, "expected 0, got %d\n", count);

    ret = IXmlReader_IsEmptyElement(reader);
    ok(ret, "element should be empty\n");

    hr = IXmlReader_GetValue(reader, &val, NULL);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
todo_wine
    ok(*val == 0, "got %s\n", wine_dbgstr_w(val));

    hr = IXmlReader_GetLocalName(reader, &val, NULL);
    ok(hr == S_OK, "expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(val, RegistrationInfoW), "got %s\n", wine_dbgstr_w(val));

    type = -1;
    hr = IXmlReader_Read(reader, &type);
todo_wine
    ok(hr == WC_E_SYNTAX || hr == WC_E_XMLCHARACTER /* XP */, "expected WC_E_SYNTAX, got %08x\n", hr);
todo_wine
    ok(type == XmlNodeType_None, "expected None, got %s\n", type_to_str(type));
    ok_pos(reader, 1, 41, -1, -1, TRUE);
    test_read_state(reader, XmlReadState_Error, -1, TRUE);

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
    struct test_entry *test = comment_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        IStream *stream;

        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

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

            ok(type == XmlNodeType_Comment, "got %d for %s\n", type, test->xml);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            len = 1;
            str = NULL;
            hr = IXmlReader_GetValue(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->value), "got %u\n", len);
            str_exp = a2w(test->value);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);
        }

        IStream_Release(stream);
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

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        IStream *stream;

        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

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

            len = 0;
            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            len = 0;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            len = !strlen(test->value);
            str = NULL;
            hr = IXmlReader_GetValue(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->value), "got %u\n", len);
            str_exp = a2w(test->value);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);
        }

        IStream_Release(stream);
        test++;
    }

    IXmlReader_Release(reader);
}

struct nodes_test {
    const char *xml;
    XmlNodeType types[20];
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
        XmlNodeType_Comment,
        XmlNodeType_Comment,
        XmlNodeType_ProcessingInstruction,
        XmlNodeType_Comment,
        XmlNodeType_Whitespace,
        XmlNodeType_Comment,
        XmlNodeType_Element,
        XmlNodeType_Whitespace,
        XmlNodeType_Element,
        XmlNodeType_Text,
        XmlNodeType_Comment,
        XmlNodeType_Text,
        XmlNodeType_ProcessingInstruction,
        XmlNodeType_Whitespace,
        XmlNodeType_EndElement,
        XmlNodeType_None
    }
};

static void test_read_full(void)
{
    struct nodes_test *test = &misc_test;
    IXmlReader *reader;
    XmlNodeType type;
    IStream *stream;
    HRESULT hr;
    int i;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "got %08x\n", hr);

    i = 0;
    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    while (hr == S_OK)
    {
        ok(test->types[i] != XmlNodeType_None, "%d: unexpected end of test data\n", i);
        if (test->types[i] == XmlNodeType_None) break;
        ok(type == test->types[i], "%d: got wrong type %d, expected %d\n", i, type, test->types[i]);
        if (type == XmlNodeType_Whitespace)
        {
            const WCHAR *ptr;
            UINT len = 0;

            hr = IXmlReader_GetValue(reader, &ptr, &len);
            ok(hr == S_OK, "%d: GetValue failed 0x%08x\n", i, hr);
            ok(len > 0, "%d: wrong value length %d\n", i, len);
        }
        hr = IXmlReader_Read(reader, &type);
        i++;
    }
    ok(test->types[i] == XmlNodeType_None, "incomplete sequence, got %d\n", test->types[i]);

    IStream_Release(stream);
    IXmlReader_Release(reader);
}

static const char test_dtd[] =
    "<!DOCTYPE testdtd SYSTEM \"externalid uri\" >"
    "<!-- comment -->";

static void test_read_dtd(void)
{
    static const WCHAR sysvalW[] = {'e','x','t','e','r','n','a','l','i','d',' ','u','r','i',0};
    static const WCHAR dtdnameW[] = {'t','e','s','t','d','t','d',0};
    static const WCHAR sysW[] = {'S','Y','S','T','E','M',0};
    IXmlReader *reader;
    const WCHAR *str;
    XmlNodeType type;
    IStream *stream;
    UINT len, count;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    hr = IXmlReader_SetProperty(reader, XmlReaderProperty_DtdProcessing, DtdProcessing_Parse);
    ok(hr == S_OK, "got 0x%8x\n", hr);

    stream = create_stream_on_data(test_dtd, sizeof(test_dtd));
    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got 0x%8x\n", hr);
    ok(type == XmlNodeType_DocumentType, "got type %d\n", type);

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

    len = 0;
    str = NULL;
    hr = IXmlReader_GetLocalName(reader, &str, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(len == lstrlenW(sysW), "got %u\n", len);
    ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));

    len = 0;
    str = NULL;
    hr = IXmlReader_GetValue(reader, &str, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(len == lstrlenW(sysvalW), "got %u\n", len);
    ok(!lstrcmpW(str, sysvalW), "got %s\n", wine_dbgstr_w(str));

    hr = IXmlReader_MoveToElement(reader);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    len = 0;
    str = NULL;
    hr = IXmlReader_GetLocalName(reader, &str, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine {
    ok(len == lstrlenW(dtdnameW), "got %u\n", len);
    ok(!lstrcmpW(str, dtdnameW), "got %s\n", wine_dbgstr_w(str));
}
    len = 0;
    str = NULL;
    hr = IXmlReader_GetQualifiedName(reader, &str, &len);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(len == lstrlenW(dtdnameW), "got %u\n", len);
    ok(!lstrcmpW(str, dtdnameW), "got %s\n", wine_dbgstr_w(str));

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got 0x%8x\n", hr);
    ok(type == XmlNodeType_Comment, "got type %d\n", type);

    IStream_Release(stream);
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
    { NULL }
};

static void test_read_element(void)
{
    struct test_entry *test = element_tests;
    static const char stag[] = "<a><b></b></a>";
    static const char mismatch[] = "<a></b>";
    IXmlReader *reader;
    XmlNodeType type;
    IStream *stream;
    UINT depth;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

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
            len = 1;
            str = NULL;
            hr = IXmlReader_GetValue(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == 0, "got %u\n", len);
            ok(*str == 0, "got %s\n", wine_dbgstr_w(str));
        }

        IStream_Release(stream);
        test++;
    }

    /* test reader depth increment */
    stream = create_stream_on_data(stag, sizeof(stag));
    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "got %08x\n", hr);

    depth = 1;
    hr = IXmlReader_GetDepth(reader, &depth);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(depth == 0, "got %d\n", depth);

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Element, "got %d\n", type);

    depth = 1;
    hr = IXmlReader_GetDepth(reader, &depth);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(depth == 0, "got %d\n", depth);

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Element, "got %d\n", type);

    depth = 0;
    hr = IXmlReader_GetDepth(reader, &depth);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(depth == 1, "got %d\n", depth);

    /* read end tag for inner element */
    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_EndElement, "got %d\n", type);

    depth = 0;
    hr = IXmlReader_GetDepth(reader, &depth);
    ok(hr == S_OK, "got %08x\n", hr);
todo_wine
    ok(depth == 2, "got %d\n", depth);

    /* read end tag for container element */
    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_EndElement, "got %d\n", type);

    depth = 0;
    hr = IXmlReader_GetDepth(reader, &depth);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(depth == 1, "got %d\n", depth);

    IStream_Release(stream);

    /* start/end tag mismatch */
    stream = create_stream_on_data(mismatch, sizeof(mismatch));
    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "got %08x\n", hr);

    type = XmlNodeType_None;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(type == XmlNodeType_Element, "got %d\n", type);

    type = XmlNodeType_Element;
    hr = IXmlReader_Read(reader, &type);
    ok(hr == WC_E_ELEMENTMATCH, "got %08x\n", hr);
todo_wine
    ok(type == XmlNodeType_None, "got %d\n", type);

    IStream_Release(stream);

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

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
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
    static const char testA[] = "<!-- comment1 -->";
    IXmlReader *reader;
    XmlNodeType type;
    IStream *stream;
    const WCHAR *value;
    WCHAR b;
    HRESULT hr;
    UINT c;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    stream = create_stream_on_data(testA, sizeof(testA));
    hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IXmlReader_Read(reader, &type);
    ok(hr == S_OK, "got %08x\n", hr);

    c = 0;
    b = 0;
    hr = IXmlReader_ReadValueChunk(reader, &b, 1, &c);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(c == 1, "got %u\n", c);
    ok(b == ' ', "got %x\n", b);

    /* portion read as chunk is skipped from resulting node value */
    value = NULL;
    hr = IXmlReader_GetValue(reader, &value, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(value[0] == 'c', "got %s\n", wine_dbgstr_w(value));

    /* once value is returned/allocated it's not possible to read by chunk */
    c = 0;
    b = 0;
    hr = IXmlReader_ReadValueChunk(reader, &b, 1, &c);
    ok(hr == S_FALSE, "got %08x\n", hr);
    ok(c == 0, "got %u\n", c);
    ok(b == 0, "got %x\n", b);

    value = NULL;
    hr = IXmlReader_GetValue(reader, &value, NULL);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(value[0] == 'c', "got %s\n", wine_dbgstr_w(value));

    IXmlReader_Release(reader);
    IStream_Release(stream);
}

static struct test_entry cdata_tests[] = {
    { "<a><![CDATA[ ]]data ]]></a>", "", " ]]data ", S_OK },
    { "<a><![CDATA[<![CDATA[ data ]]]]></a>", "", "<![CDATA[ data ]]", S_OK },
    { "<a><![CDATA[\n \r\n \n\n ]]></a>", "", "\n \n \n\n ", S_OK, S_OK, TRUE },
    { "<a><![CDATA[\r \r\r\n \n\n ]]></a>", "", "\n \n\n \n\n ", S_OK, S_OK, TRUE },
    { "<a><![CDATA[\r\r \n\r \r \n\n ]]></a>", "", "\n\n \n\n \n \n\n ", S_OK },
    { NULL }
};

static void test_read_cdata(void)
{
    struct test_entry *test = cdata_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        IStream *stream;

        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

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
            WCHAR *str_exp;
            UINT len;

            ok(type == XmlNodeType_CDATA, "got %d for %s\n", type, test->xml);

            str_exp = a2w(test->name);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));

            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));

            free_str(str_exp);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            len = 1;
            str = NULL;
            hr = IXmlReader_GetValue(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            str_exp = a2w(test->value);
            if (test->todo)
            {
            todo_wine {
                ok(len == strlen(test->value), "got %u\n", len);
                ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            }
            }
            else
            {
                ok(len == strlen(test->value), "got %u\n", len);
                ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            }
            free_str(str_exp);
        }

        IStream_Release(stream);
        test++;
    }

    IXmlReader_Release(reader);
}

static struct test_entry text_tests[] = {
    { "<a>simple text</a>", "", "simple text", S_OK },
    { "<a>text ]]> text</a>", "", "", WC_E_CDSECTEND },
    { NULL }
};

static void test_read_text(void)
{
    struct test_entry *test = text_tests;
    IXmlReader *reader;
    HRESULT hr;

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        IStream *stream;

        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

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
            WCHAR *str_exp;
            UINT len;

            ok(type == XmlNodeType_Text, "got %d for %s\n", type, test->xml);

            str_exp = a2w(test->name);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));

            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, NULL);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));

            free_str(str_exp);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            /* value */
            len = 1;
            str = NULL;
            hr = IXmlReader_GetValue(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            str_exp = a2w(test->value);
            if (test->todo)
            {
            todo_wine {
                ok(len == strlen(test->value), "got %u\n", len);
                ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            }
            }
            else
            {
                ok(len == strlen(test->value), "got %u\n", len);
                ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            }
            free_str(str_exp);
        }

        IStream_Release(stream);
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

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        IStream *stream;
        BOOL ret;

        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

        type = XmlNodeType_None;
        hr = IXmlReader_Read(reader, &type);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(type == XmlNodeType_Element, "got %d\n", type);

        ret = IXmlReader_IsEmptyElement(reader);
        ok(ret == test->empty, "got %d, expected %d. xml=%s\n", ret, test->empty, test->xml);

        IStream_Release(stream);
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

    hr = pCreateXmlReader(&IID_IXmlReader, (void**)&reader, NULL);
    ok(hr == S_OK, "S_OK, got %08x\n", hr);

    while (test->xml)
    {
        XmlNodeType type;
        IStream *stream;

        stream = create_stream_on_data(test->xml, strlen(test->xml)+1);
        hr = IXmlReader_SetInput(reader, (IUnknown*)stream);
        ok(hr == S_OK, "got %08x\n", hr);

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

            ok(type == XmlNodeType_Element, "got %d for %s\n", type, test->xml);

            hr = IXmlReader_MoveToFirstAttribute(reader);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetLocalName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);

            len = 1;
            str = NULL;
            hr = IXmlReader_GetQualifiedName(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
        todo_wine {
            ok(len == strlen(test->name), "got %u\n", len);
            str_exp = a2w(test->name);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);
        }
            /* value */
            len = 1;
            str = NULL;
            hr = IXmlReader_GetValue(reader, &str, &len);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            ok(len == strlen(test->value), "got %u\n", len);
            str_exp = a2w(test->value);
            ok(!lstrcmpW(str, str_exp), "got %s\n", wine_dbgstr_w(str));
            free_str(str_exp);
        }

        IStream_Release(stream);
        test++;
    }

    IXmlReader_Release(reader);
}

START_TEST(reader)
{
    if (!init_pointers())
       return;

    test_reader_create();
    test_readerinput();
    test_reader_state();
    test_read_attribute();
    test_read_cdata();
    test_read_comment();
    test_read_pi();
    test_read_dtd();
    test_read_element();
    test_isemptyelement();
    test_read_text();
    test_read_full();
    test_read_pending();
    test_readvaluechunk();
    test_read_xmldeclaration();
}
