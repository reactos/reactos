/*
 * XML test
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2007-2008 Alistair Leslie-Hughes
 * Copyright 2010-2011 Adam Martinson for CodeWeavers
 * Copyright 2010-2013 Nikolay Sivov for CodeWeavers
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

#include <stdio.h>
#include <assert.h>

#include "windows.h"

#include "msxml.h"
#include "msxml2.h"
#include "msxml2did.h"
#include "ole2.h"
#include "dispex.h"
#include "objsafe.h"
#include "mshtml.h"
#include "docobj.h"
#include "xmlparser.h"
#include "initguid.h"
#include "asptlb.h"
#include "shlwapi.h"

#include "wine/test.h"

#define XML_E_UNEXPECTED_ATTRIBUTE 0xC00CE56C

/* undef the #define in msxml2 so that we can access all versions */
#undef CLSID_DOMDocument

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(IID_transformdest_unknown,0xf5078f3a,0xc551,0x11d3,0x89,0xb9,0x00,0x00,0xf8,0x1f,0xe2,0x21);

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static int g_unexpectedcall, g_expectedcall;

struct msxmlsupported_data_t
{
    const GUID *clsid;
    const char *name;
    const IID  *ifaces[3];
    BOOL        supported[3];
};

static struct msxmlsupported_data_t domdoc_support_data[] =
{
    { &CLSID_DOMDocument,   "DOMDocument",   {&IID_IXMLDOMDocument, &IID_IXMLDOMDocument2} },
    { &CLSID_DOMDocument2,  "DOMDocument2",  {&IID_IXMLDOMDocument, &IID_IXMLDOMDocument2} },
    { &CLSID_DOMDocument30, "DOMDocument30", {&IID_IXMLDOMDocument, &IID_IXMLDOMDocument2} },
    { &CLSID_DOMDocument40, "DOMDocument40", {&IID_IXMLDOMDocument, &IID_IXMLDOMDocument2} },
    { &CLSID_DOMDocument60, "DOMDocument60", {&IID_IXMLDOMDocument, &IID_IXMLDOMDocument2, &IID_IXMLDOMDocument3} },
    { &CLSID_FreeThreadedDOMDocument, "FreeThreadedDOMDocument", {&IID_IXMLDOMDocument, &IID_IXMLDOMDocument2} },
    { &CLSID_XMLSchemaCache, "XMLSchemaCache", {&IID_IXMLDOMSchemaCollection} },
    { &CLSID_XSLTemplate,    "XSLTemplate", {&IID_IXSLTemplate} },
    { &CLSID_MXNamespaceManager40, "MXNamespaceManager40", {&IID_IMXNamespaceManager} },
    { NULL }
};

static const char *debugstr_msxml_guid(REFIID riid)
{
    if(!riid)
        return "(null)";

    if (IsEqualIID(&IID_IXMLDOMDocument, riid))
        return "IXMLDOMDocument";
    else if (IsEqualIID(&IID_IXMLDOMDocument2, riid))
        return "IXMLDOMDocument2";
    else if (IsEqualIID(&IID_IXMLDOMDocument3, riid))
        return "IXMLDOMDocument3";
    else if (IsEqualIID(&IID_IXMLDOMSchemaCollection, riid))
        return "IXMLDOMSchemaCollection";
    else if (IsEqualIID(&IID_IXSLTemplate, riid))
        return "IXSLTemplate";
    else if (IsEqualIID(&IID_IMXNamespaceManager, riid))
        return "IMXNamespaceManager";
    else
        return wine_dbgstr_guid(riid);
}

static void get_class_support_data(struct msxmlsupported_data_t *table)
{
    while (table->clsid)
    {
        IUnknown *unk;
        HRESULT hr;
        int i;

        for (i = 0; i < ARRAY_SIZE(table->ifaces) && table->ifaces[i] != NULL; i++)
        {
            hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, table->ifaces[i], (void**)&unk);
            if (hr == S_OK) IUnknown_Release(unk);

            table->supported[i] = hr == S_OK;
            if (hr != S_OK) win_skip("class %s, iface %s not supported\n", table->name, debugstr_msxml_guid(table->ifaces[i]));
        }

        table++;
    }
}

static BOOL is_clsid_supported(const GUID *clsid, REFIID riid)
{
    const struct msxmlsupported_data_t *table = domdoc_support_data;
    while (table->clsid)
    {
        if (table->clsid == clsid)
        {
            int i;

            for (i = 0; i < ARRAY_SIZE(table->ifaces) && table->ifaces[i] != NULL; i++)
                if (table->ifaces[i] == riid) return table->supported[i];
        }

        table++;
    }
    return FALSE;
}

typedef struct
{
    IDispatch IDispatch_iface;
    LONG ref;
} dispevent;

static inline dispevent *impl_from_IDispatch( IDispatch *iface )
{
    return CONTAINING_RECORD(iface, dispevent, IDispatch_iface);
}

static HRESULT WINAPI dispevent_QueryInterface(IDispatch *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = iface;
    }
    else
        return E_NOINTERFACE;

    IDispatch_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI dispevent_AddRef(IDispatch *iface)
{
    dispevent *This = impl_from_IDispatch( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI dispevent_Release(IDispatch *iface)
{
    dispevent *This = impl_from_IDispatch( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI dispevent_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    g_unexpectedcall++;
    *pctinfo = 0;
    return S_OK;
}

static HRESULT WINAPI dispevent_GetTypeInfo(IDispatch *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    g_unexpectedcall++;
    return S_OK;
}

static HRESULT WINAPI dispevent_GetIDsOfNames(IDispatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    g_unexpectedcall++;
    return S_OK;
}

static HRESULT WINAPI dispevent_Invoke(IDispatch *iface, DISPID member, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result,
        EXCEPINFO *excepInfo, UINT *argErr)
{
    ok(member == 0, "expected 0 member, got %ld\n", member);
    ok(lcid == LOCALE_SYSTEM_DEFAULT, "expected LOCALE_SYSTEM_DEFAULT, got lcid %lx\n", lcid);
    ok(flags == DISPATCH_METHOD, "expected DISPATCH_METHOD, got %d\n", flags);

    ok(params->cArgs == 0, "got %d\n", params->cArgs);
    ok(params->cNamedArgs == 0, "got %d\n", params->cNamedArgs);
    ok(params->rgvarg == NULL, "got %p\n", params->rgvarg);
    ok(params->rgdispidNamedArgs == NULL, "got %p\n", params->rgdispidNamedArgs);

    ok(result == NULL, "got %p\n", result);
    ok(excepInfo == NULL, "got %p\n", excepInfo);
    ok(argErr == NULL, "got %p\n", argErr);

    g_expectedcall++;
    return E_FAIL;
}

static const IDispatchVtbl dispeventVtbl =
{
    dispevent_QueryInterface,
    dispevent_AddRef,
    dispevent_Release,
    dispevent_GetTypeInfoCount,
    dispevent_GetTypeInfo,
    dispevent_GetIDsOfNames,
    dispevent_Invoke
};

static IDispatch* create_dispevent(void)
{
    dispevent *event = malloc(sizeof(*event));

    event->IDispatch_iface.lpVtbl = &dispeventVtbl;
    event->ref = 1;

    return &event->IDispatch_iface;
}

/* IStream */
static HRESULT WINAPI istream_QueryInterface(IStream *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IStream) ||
        IsEqualGUID(riid, &IID_IUnknown))
        *ppvObject = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI istream_AddRef(IStream *iface)
{
    return 2;
}

static ULONG WINAPI istream_Release(IStream *iface)
{
    return 1;
}

static HRESULT WINAPI istream_Read(IStream *iface, void *ptr, ULONG len, ULONG *pread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Write(IStream *iface, const void *ptr, ULONG len, ULONG *written)
{
    *written = len/2;
    return S_OK;
}

static HRESULT WINAPI istream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *new_pos)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_CopyTo(IStream *iface, IStream *stream, ULARGE_INTEGER len,
        ULARGE_INTEGER *pread, ULARGE_INTEGER *written)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Commit(IStream *iface, DWORD flags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Revert(IStream *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_LockRegion(IStream *iface, ULARGE_INTEGER offset,
        ULARGE_INTEGER len, DWORD locktype)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset,
        ULARGE_INTEGER len, DWORD locktype)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Stat(IStream *iface, STATSTG *pstatstg, DWORD flag)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI istream_Clone(IStream *iface, IStream **stream)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IStreamVtbl StreamVtbl = {
    istream_QueryInterface,
    istream_AddRef,
    istream_Release,
    istream_Read,
    istream_Write,
    istream_Seek,
    istream_SetSize,
    istream_CopyTo,
    istream_Commit,
    istream_Revert,
    istream_LockRegion,
    istream_UnlockRegion,
    istream_Stat,
    istream_Clone
};

static IStream savestream = { &StreamVtbl };

static HRESULT WINAPI response_QI(IResponse *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(&IID_IResponse, riid) ||
            IsEqualIID(&IID_IDispatch, riid) ||
            IsEqualIID(&IID_IUnknown, riid))
    {
        *obj = iface;
        return S_OK;
    }

    if (!IsEqualIID(&IID_IStream, riid) && !IsEqualIID(&IID_ISequentialStream, riid))
        ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI response_AddRef(IResponse *iface)
{
    return 2;
}

static ULONG WINAPI response_Release(IResponse *iface)
{
    return 1;
}

static HRESULT WINAPI response_GetTypeInfoCount(IResponse *iface, UINT *count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_GetTypeInfo(IResponse *iface, UINT ti, LCID lcid, ITypeInfo **tinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_GetIDsOfNames(IResponse *iface, REFIID riid, LPOLESTR *names,
        UINT cnames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_Invoke(IResponse *iface, DISPID dispid, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *ei, UINT *argerr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_Buffer(IResponse *iface, VARIANT_BOOL *fIsBuffering)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_Buffer(IResponse *iface, VARIANT_BOOL fIsBuffering)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_ContentType(IResponse *iface, BSTR *pbstrContentTypeRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_ContentType(IResponse *iface, BSTR bstrContentType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_Expires(IResponse *iface, VARIANT *pvarExpiresMinutesRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_Expires(IResponse *iface, LONG lExpiresMinutes)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_ExpiresAbsolute(IResponse *iface, VARIANT *pvarExpiresRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_ExpiresAbsolute(IResponse *iface, DATE dtExpires)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_Cookies(IResponse *iface, IRequestDictionary **ppCookies)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_Status(IResponse *iface, BSTR *pbstrStatusRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_Status(IResponse *iface, BSTR bstrStatus)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_Add(IResponse *iface, BSTR bstrHeaderValue, BSTR bstrHeaderName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_AddHeader(IResponse *iface, BSTR bstrHeaderName, BSTR bstrHeaderValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_AppendToLog(IResponse *iface, BSTR bstrLogEntry)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_BinaryWrite(IResponse *iface, VARIANT input)
{
    HRESULT hr;
    LONG bound;
    UINT dim;

    ok(V_VT(&input) == (VT_ARRAY | VT_UI1), "got wrong input type %x\n", V_VT(&input));

    dim = SafeArrayGetDim(V_ARRAY(&input));
    ok(dim == 1, "got wrong array dimensions %u\n", dim);

    bound = 1;
    hr = SafeArrayGetLBound(V_ARRAY(&input), 1, &bound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(bound == 0, "wrong array low bound %ld\n", bound);

    bound = 0;
    hr = SafeArrayGetUBound(V_ARRAY(&input), 1, &bound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(bound > 0, "wrong array high bound %ld\n", bound);

    return E_NOTIMPL;
}

static HRESULT WINAPI response_Clear(IResponse *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_End(IResponse *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_Flush(IResponse *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_Redirect(IResponse *iface, BSTR bstrURL)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_Write(IResponse *iface, VARIANT varText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_WriteBlock(IResponse *iface, short iBlockNumber)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_IsClientConnected(IResponse *iface, VARIANT_BOOL *pfIsClientConnected)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_CharSet(IResponse *iface, BSTR *pbstrCharSetRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_CharSet(IResponse *iface, BSTR bstrCharSet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_Pics(IResponse *iface, BSTR bstrHeaderValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_CacheControl(IResponse *iface, BSTR *pbstrCacheControl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_CacheControl(IResponse *iface, BSTR bstrCacheControl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_CodePage(IResponse *iface, LONG *plvar)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_CodePage(IResponse *iface, LONG codepage)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_get_LCID(IResponse *iface, LONG *lcid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI response_put_LCID(IResponse *iface, LONG lcid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IResponseVtbl testresponsevtbl =
{
    response_QI,
    response_AddRef,
    response_Release,
    response_GetTypeInfoCount,
    response_GetTypeInfo,
    response_GetIDsOfNames,
    response_Invoke,
    response_get_Buffer,
    response_put_Buffer,
    response_get_ContentType,
    response_put_ContentType,
    response_get_Expires,
    response_put_Expires,
    response_get_ExpiresAbsolute,
    response_put_ExpiresAbsolute,
    response_get_Cookies,
    response_get_Status,
    response_put_Status,
    response_Add,
    response_AddHeader,
    response_AppendToLog,
    response_BinaryWrite,
    response_Clear,
    response_End,
    response_Flush,
    response_Redirect,
    response_Write,
    response_WriteBlock,
    response_IsClientConnected,
    response_get_CharSet,
    response_put_CharSet,
    response_Pics,
    response_get_CacheControl,
    response_put_CacheControl,
    response_get_CodePage,
    response_put_CodePage,
    response_get_LCID,
    response_put_LCID,
};

static IResponse testresponse = { &testresponsevtbl };

#define EXPECT_CHILDREN(node) _expect_children((IXMLDOMNode*)node, __LINE__)
static void _expect_children(IXMLDOMNode *node, int line)
{
    VARIANT_BOOL b;
    HRESULT hr;

    b = VARIANT_FALSE;
    hr = IXMLDOMNode_hasChildNodes(node, &b);
    ok_(__FILE__,line)(hr == S_OK, "hasChildNodes() failed, %#lx.\n", hr);
    ok_(__FILE__,line)(b == VARIANT_TRUE, "no children, %d\n", b);
}

#define EXPECT_NO_CHILDREN(node) _expect_no_children((IXMLDOMNode*)node, __LINE__)
static void _expect_no_children(IXMLDOMNode *node, int line)
{
    VARIANT_BOOL b;
    HRESULT hr;

    b = VARIANT_TRUE;
    hr = IXMLDOMNode_hasChildNodes(node, &b);
    ok_(__FILE__,line)(hr == S_FALSE, "hasChildNodes() failed, %#lx.\n", hr);
    ok_(__FILE__,line)(b == VARIANT_FALSE, "no children, %d\n", b);
}

#define EXPECT_REF(node,ref) _expect_ref((IUnknown*)node, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

#define EXPECT_LIST_LEN(list,len) _expect_list_len(list, len, __LINE__)
static void _expect_list_len(IXMLDOMNodeList *list, LONG len, int line)
{
    LONG length;
    HRESULT hr;

    length = 0;
    hr = IXMLDOMNodeList_get_length(list, &length);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__,line)(length == len, "got %ld, expected %ld\n", length, len);
}

#define EXPECT_PARSE_ERROR(doc, hr_exp, hr_todo) _expect_parse_error(doc, hr_exp, hr_todo, __LINE__)
static void _expect_parse_error(IXMLDOMDocument *doc, HRESULT hr_exp, BOOL hr_todo, int line)
{
    IXMLDOMParseError *error;
    HRESULT hr;
    LONG code;

    error = NULL;
    hr = IXMLDOMDocument_get_parseError(doc, &error);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__,line)(!!error, "got NULL parseError\n");

    code = 0xdeadbeef;
    hr = IXMLDOMParseError_get_errorCode(error, &code);
    if (FAILED(hr_exp))
    {
        ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_(__FILE__,line)(FAILED(code), "expected failure HRESULT\n");
        todo_wine_if(hr_todo)
        ok_(__FILE__,line)(hr_exp == code, "expected %#lx, unexpected hr %#lx.\n", hr_exp, code);
    }
    else
    {
        ok_(__FILE__,line)(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok_(__FILE__,line)(SUCCEEDED(code), "expected successful HRESULT\n");
    }

    IXMLDOMParseError_Release(error);
}

static const WCHAR szComplete1[] = L"<?xml version='1.0'?>\n<open></open>\n";
static const char complete4A[] =
    "<?xml version=\'1.0\'?>\n"
    "<lc dl=\'str1\'>\n"
        "<bs vr=\'str2\' sz=\'1234\'>"
            "fn1.txt\n"
        "</bs>\n"
        "<pr id=\'str3\' vr=\'1.2.3\' pn=\'wine 20050804\'>\n"
            "fn2.txt\n"
        "</pr>\n"
        "<empty></empty>\n"
        "<fo>\n"
            "<ba>\n"
                "f1\n"
            "</ba>\n"
        "</fo>\n"
    "</lc>\n";

static const WCHAR szComplete5[] =
    L"<S:search xmlns:D=\"DAV:\" xmlns:C=\"urn:schemas-microsoft-com:office:clipgallery\" xmlns:S=\"urn:schemas-microsoft-com:office:clipgallery:search\">"
     "<S:scope><S:deep>/</S:deep></S:scope><S:contentfreetext><C:textorproperty/>computer</S:contentfreetext></S:search>";

static const char complete7[] = {
    "<?xml version=\"1.0\"?>\n\t"
    "<root>\n"
    "\t<a/>\n"
    "\t<b/>\n"
    "\t<c/>\n"
    "</root>"
};

#define DECL_GBK \
"<?xml version=\"1.0\" encoding=\"gbk\"?>"

static const char gbkxml[] =
DECL_GBK
"<open></open>";

#define DECL_ISO8859_1 \
"<?xml version=\"1.0\" encoding=\"ISO8859-1\"?>"

static const char iso8859_1_xml[] =
DECL_ISO8859_1
"<open></open>";

#define DECL_WIN_936 \
"<?xml version=\"1.0\" encoding=\"Windows-936\"?>"

static const char win936xml[] =
DECL_WIN_936
"<open></open>";

#define DECL_WIN_1252 \
"<?xml version=\"1.0\" encoding=\"Windows-1252\"?>"

static const char win1252xml[] =
DECL_WIN_1252
"<open></open>";

static const char win1252decl[] =
DECL_WIN_1252
;

static const char nocontent[] = "no xml content here";

static const char szExampleXML[] =
"<?xml version='1.0' encoding='utf-8'?>\n"
"<root xmlns:foo='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' a=\"attr a\" foo:b=\"attr b\" >\n"
"    <elem>\n"
"        <a>A1 field</a>\n"
"        <b>B1 field</b>\n"
"        <c>C1 field</c>\n"
"        <d>D1 field</d>\n"
"        <description xmlns:foo='http://www.winehq.org' xmlns:bar='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"            <html xmlns='http://www.w3.org/1999/xhtml'>\n"
"                <![CDATA[]]> This is<strong> a</strong>  <i>description</i><dot>. </dot><bar:x/>\n"
"            </html>\n"
"            <html xml:space='preserve' xmlns='http://www.w3.org/1999/xhtml'>\n"
"                This is <strong>a</strong> <i>description</i> with preserved whitespace. <bar:x/>\n"
"            </html>\n"
"        </description>\n"
"    </elem>\n"
"\n"
"    <elem a='a'>\n"
"        <a>A2 field</a>\n"
"        <b>B2 field</b>\n"
"        <c type=\"old\">C2 field</c>\n"
"        <d>D2 field</d>\n"
"    </elem>\n"
"\n"
"    <elem xmlns='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'>\n"
"        <a>A3 field</a>\n"
"        <b>B3 field</b>\n"
"        <c>C3 field</c>\n"
"    </elem>\n"
"\n"
"    <elem>\n"
"        <a>A4 field</a>\n"
"        <b>B4 field</b>\n"
"        <foo:c>C4 field</foo:c>\n"
"        <d>D4 field</d>\n"
"    </elem>\n"
"</root>\n";

static const char charrefsxml[] =
"<?xml version='1.0'?>"
"<a>"
"<b1> Text &#65; end </b1>"
"<b2>&#65;&#66; &#67; </b2>"
"</a>";

static const CHAR szNodeTypesXML[] =
"<?xml version='1.0'?>"
"<!-- comment node 0 -->"
"<root id='0' depth='0'>"
"   <!-- comment node 1 -->"
"   text node 0"
"   <x id='1' depth='1'>"
"       <?foo value='PI for x'?>"
"       <!-- comment node 2 -->"
"       text node 1"
"       <a id='3' depth='2'/>"
"       <b id='4' depth='2'/>"
"       <c id='5' depth='2'/>"
"   </x>"
"   <y id='2' depth='1'>"
"       <?bar value='PI for y'?>"
"       <!-- comment node 3 -->"
"       text node 2"
"       <a id='6' depth='2'/>"
"       <b id='7' depth='2'/>"
"       <c id='8' depth='2'/>"
"   </y>"
"</root>";

static const CHAR szTransformXML[] =
"<?xml version=\"1.0\"?>\n"
"<greeting>\n"
"Hello World\n"
"</greeting>";

static  const CHAR szTransformSSXML[] =
"<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
"   <xsl:output method=\"html\"/>\n"
"   <xsl:template match=\"/\">\n"
"       <xsl:apply-templates select=\"greeting\"/>\n"
"   </xsl:template>\n"
"   <xsl:template match=\"greeting\">\n"
"       <html>\n"
"           <body>\n"
"               <h1>\n"
"                   <xsl:value-of select=\".\"/>\n"
"               </h1>\n"
"           </body>\n"
"       </html>\n"
"   </xsl:template>\n"
"</xsl:stylesheet>";

static  const CHAR szTransformOutput[] =
"<html><body><h1>"
"Hello World"
"</h1></body></html>";

static const CHAR szTypeValueXML[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<root xmlns:dt=\"urn:schemas-microsoft-com:datatypes\">\n"
"   <string>Wine</string>\n"
"   <string2 dt:dt=\"string\">String </string2>\n"
"   <string3> Wine Wine </string3>\n"
"   <string4>\nWine Wine\t</string4>\n"
"   <string5>   </string5>\n"
"   <nested><s1> s1 </s1><s2>\ns2 s2\t</s2></nested>\n"
"   <number dt:dt=\"number\">12.44</number>\n"
"   <number2 dt:dt=\"NUMbEr\">-3.71e3</number2>\n"
"   <int dt:dt=\"int\">-13</int>\n"
"   <fixed dt:dt=\"fixed.14.4\">7322.9371</fixed>\n"
"   <bool dt:dt=\"boolean\">1</bool>\n"
"   <datetime dt:dt=\"datetime\">2009-11-18T03:21:33.12</datetime>\n"
"   <datetimetz dt:dt=\"datetime.tz\">2003-07-11T11:13:57+03:00</datetimetz>\n"
"   <date dt:dt=\"date\">3721-11-01</date>\n"
"   <time dt:dt=\"time\">13:57:12.31321</time>\n"
"   <timetz dt:dt=\"time.tz\">23:21:01.13+03:21</timetz>\n"
"   <i1 dt:dt=\"i1\">-13</i1>\n"
"   <i2 dt:dt=\"i2\">31915</i2>\n"
"   <i4 dt:dt=\"i4\">-312232</i4>\n"
"   <ui1 dt:dt=\"ui1\">123</ui1>\n"
"   <ui2 dt:dt=\"ui2\">48282</ui2>\n"
"   <ui4 dt:dt=\"ui4\">949281</ui4>\n"
"   <r4 dt:dt=\"r4\">213124.0</r4>\n"
"   <r8 dt:dt=\"r8\">0.412</r8>\n"
"   <float dt:dt=\"float\">41221.421</float>\n"
"   <uuid dt:dt=\"uuid\">333C7BC4-460F-11D0-BC04-0080C7055a83</uuid>\n"
"   <binhex dt:dt=\"bin.hex\">fffca012003c</binhex>\n"
"   <binbase64 dt:dt=\"bin.base64\">YmFzZTY0IHRlc3Q=</binbase64>\n"
"   <binbase64_1 dt:dt=\"bin.base64\">\nYmFzZTY0\nIHRlc3Q=\n</binbase64_1>\n"
"   <binbase64_2 dt:dt=\"bin.base64\">\nYmF\r\t z  ZTY0\nIHRlc3Q=\n</binbase64_2>\n"
"</root>";

static const CHAR szBasicTransformSSXMLPart1[] =
"<?xml version=\"1.0\"?>"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" >"
"<xsl:output method=\"html\"/>\n"
"<xsl:template match=\"/\">"
"<HTML><BODY><TABLE>"
"        <xsl:apply-templates select='document(\"";

static const CHAR szBasicTransformSSXMLPart2[] =
"\")/bottle/wine'>"
"           <xsl:sort select=\"cost\"/><xsl:sort select=\"name\"/>"
"        </xsl:apply-templates>"
"</TABLE></BODY></HTML>"
"</xsl:template>"
"<xsl:template match=\"bottle\">"
"   <TR><xsl:apply-templates select=\"name\" /><xsl:apply-templates select=\"cost\" /></TR>"
"</xsl:template>"
"<xsl:template match=\"name\">"
"   <TD><xsl:apply-templates /></TD>"
"</xsl:template>"
"<xsl:template match=\"cost\">"
"   <TD><xsl:apply-templates /></TD>"
"</xsl:template>"
"</xsl:stylesheet>";

static const CHAR szBasicTransformXML[] =
"<?xml version=\"1.0\"?><bottle><wine><name>Wine</name><cost>$25.00</cost></wine></bottle>";

static const CHAR szBasicTransformOutput[] =
"<HTML><BODY><TABLE><TD>Wine</TD><TD>$25.00</TD></TABLE></BODY></HTML>";

#define SZ_EMAIL_DTD \
"<!DOCTYPE email ["\
"   <!ELEMENT email         (recipients,from,reply-to?,subject,body,attachment*)>"\
"       <!ATTLIST email attachments IDREFS #REQUIRED>"\
"       <!ATTLIST email sent (yes|no) \"no\">"\
"   <!ELEMENT recipients    (to+,cc*)>"\
"   <!ELEMENT to            (#PCDATA)>"\
"       <!ATTLIST to name CDATA #IMPLIED>"\
"   <!ELEMENT cc            (#PCDATA)>"\
"       <!ATTLIST cc name CDATA #IMPLIED>"\
"   <!ELEMENT from          (#PCDATA)>"\
"       <!ATTLIST from name CDATA #IMPLIED>"\
"   <!ELEMENT reply-to      (#PCDATA)>"\
"       <!ATTLIST reply-to name CDATA #IMPLIED>"\
"   <!ELEMENT subject       ANY>"\
"   <!ELEMENT body          ANY>"\
"       <!ATTLIST body enc CDATA #FIXED \"UTF-8\">"\
"   <!ELEMENT attachment    (#PCDATA)>"\
"       <!ATTLIST attachment id ID #REQUIRED>"\
"]>"

static const CHAR szEmailXML[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 87)</subject>"
"   <body>"
"       It no longer causes spontaneous combustion..."
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_0D[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 88)</subject>"
"   <body>"
"       <undecl />"
"       XML_ELEMENT_UNDECLARED 0xC00CE00D"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_0E[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 89)</subject>"
"   <body>"
"       XML_ELEMENT_ID_NOT_FOUND 0xC00CE00E"
"   </body>"
"   <attachment id=\"patch\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_11[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 90)</subject>"
"   <body>"
"       XML_EMPTY_NOT_ALLOWED 0xC00CE011"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_13[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<msg attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 91)</subject>"
"   <body>"
"       XML_ROOT_NAME_MISMATCH 0xC00CE013"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</msg>";

static const CHAR szEmailXML_14[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <to>wine-patches@winehq.org</to>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 92)</subject>"
"   <body>"
"       XML_INVALID_CONTENT 0xC00CE014"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_15[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\" ip=\"127.0.0.1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 93)</subject>"
"   <body>"
"       XML_ATTRIBUTE_NOT_DEFINED 0xC00CE015"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_16[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 94)</subject>"
"   <body enc=\"ASCII\">"
"       XML_ATTRIBUTE_FIXED 0xC00CE016"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_17[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\" sent=\"true\">"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 95)</subject>"
"   <body>"
"       XML_ATTRIBUTE_VALUE 0xC00CE017"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_18[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email attachments=\"patch1\">"
"   oops"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 96)</subject>"
"   <body>"
"       XML_ILLEGAL_TEXT 0xC00CE018"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const CHAR szEmailXML_20[] =
"<?xml version=\"1.0\"?>"
SZ_EMAIL_DTD
"<email>"
"   <recipients>"
"       <to>wine-patches@winehq.org</to>"
"   </recipients>"
"   <from name=\"Anonymous\">user@localhost</from>"
"   <subject>msxml3/tests: DTD validation (try 97)</subject>"
"   <body>"
"       XML_REQUIRED_ATTRIBUTE_MISSING 0xC00CE020"
"   </body>"
"   <attachment id=\"patch1\">0001-msxml3-tests-DTD-validation.patch</attachment>"
"</email>";

static const char xpath_simple_list[] =
"<?xml version=\"1.0\"?>"
"<root>"
"   <a attr1=\"1\" attr2=\"2\" />"
"   <b/>"
"   <c/>"
"   <d/>"
"</root>";

static const char default_ns_doc[] = {
    "<?xml version=\"1.0\"?>"
    "<a xmlns:ns=\"nshref\" xml:lang=\"ru\" ns:b=\"b attr\" xml:c=\"c attr\" "
    "    d=\"d attr\" />"
};

static const char attributes_map[] = {
    "<?xml version=\"1.0\"?>"
    "<a attr1=\"value1\" attr2=\"value2\" attr3=\"value3\" attr4=\"value4\" />"
};

static const WCHAR nonexistent_fileW[] = L"c:\\Nonexistent.xml";

static const WCHAR szStrangeChars[] = L"&x \x2103";

#define expect_bstr_eq_and_free(bstr, expect) { \
    BSTR bstrExp = alloc_str_from_narrow(expect); \
    ok(lstrcmpW(bstr, bstrExp) == 0, "String differs\n"); \
    SysFreeString(bstr); \
    SysFreeString(bstrExp); \
}

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %x, expected %x\n", r, expect); \
}

#define double_eq(x, y) ok((x)-(y)<=1e-14*(x) && (x)-(y)>=-1e-14*(x), "expected %.16g, got %.16g\n", x, y)

static void* _create_object(const GUID *clsid, const char *name, const IID *iid, int line)
{
    void *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, &obj);
    ok(hr == S_OK, "failed to create %s instance: hr %#lx.\n", name, hr);

    return obj;
}

#define _create(cls) cls, #cls

#define create_document(iid) _create_object(&_create(CLSID_DOMDocument2), iid, __LINE__)
#define create_document_version(v, iid) _create_object(&_create(CLSID_DOMDocument ## v), iid, __LINE__)
#define create_cache(iid) _create_object(&_create(CLSID_XMLSchemaCache), iid, __LINE__)
#define create_xsltemplate(iid) _create_object(&_create(CLSID_XSLTemplate), iid, __LINE__)

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR _bstr_(const char *str)
{
    assert(alloced_bstrs_count < ARRAY_SIZE(alloced_bstrs));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

static VARIANT _variantbstr_(const char *str)
{
    VARIANT v;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_(str);
    return v;
}

static BOOL compareIgnoreReturns(BSTR sLeft, BSTR sRight)
{
    for (;;)
    {
        while (*sLeft == '\r' || *sLeft == '\n') sLeft++;
        while (*sRight == '\r' || *sRight == '\n') sRight++;
        if (*sLeft != *sRight) return FALSE;
        if (!*sLeft) return TRUE;
        sLeft++;
        sRight++;
    }
}

static void get_str_for_type(DOMNodeType type, char *buf)
{
    switch (type)
    {
        case NODE_ATTRIBUTE:
            strcpy(buf, "A");
            break;
        case NODE_ELEMENT:
            strcpy(buf, "E");
            break;
        case NODE_DOCUMENT:
            strcpy(buf, "D");
            break;
        case NODE_TEXT:
            strcpy(buf, "T");
            break;
        case NODE_COMMENT:
            strcpy(buf, "C");
            break;
        case NODE_PROCESSING_INSTRUCTION:
            strcpy(buf, "P");
            break;
        default:
            wsprintfA(buf, "[%d]", type);
    }
}

static int get_node_position(IXMLDOMNode *node)
{
    HRESULT r;
    int pos = 0;

    IXMLDOMNode_AddRef(node);
    do
    {
        IXMLDOMNode *new_node;

        pos++;
        r = IXMLDOMNode_get_previousSibling(node, &new_node);
        ok(SUCCEEDED(r), "get_previousSibling failed\n");
        IXMLDOMNode_Release(node);
        node = new_node;
    } while (r == S_OK);
    return pos;
}

static void node_to_string(IXMLDOMNode *node, char *buf)
{
    HRESULT hr = S_OK;
    DOMNodeType type;

    if (node == NULL)
    {
        lstrcpyA(buf, "(null)");
        return;
    }

    IXMLDOMNode_AddRef(node);
    while (hr == S_OK)
    {
        IXMLDOMNode *new_node;

        hr = IXMLDOMNode_get_nodeType(node, &type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        get_str_for_type(type, buf);
        buf+=strlen(buf);

        if (type == NODE_ATTRIBUTE)
        {
            BSTR bstr;
            hr = IXMLDOMNode_get_nodeName(node, &bstr);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            *(buf++) = '\'';
            wsprintfA(buf, "%ws", bstr);
            buf += strlen(buf);
            *(buf++) = '\'';
            SysFreeString(bstr);

            hr = IXMLDOMNode_selectSingleNode(node, _bstr_(".."), &new_node);
        }
        else
        {
            hr = IXMLDOMNode_get_parentNode(node, &new_node);
            sprintf(buf, "%d", get_node_position(node));
            buf += strlen(buf);
        }

        ok(SUCCEEDED(hr), "Unexpected hr %#lx.\n", hr);
        IXMLDOMNode_Release(node);
        node = new_node;
        if (hr == S_OK)
            *(buf++) = '.';
    }

    *buf = 0;
}

static char *list_to_string(IXMLDOMNodeList *list)
{
    static char buf[4096];
    char *pos = buf;
    LONG len = 0;
    HRESULT hr;
    int i;

    if (list == NULL)
    {
        strcpy(buf, "(null)");
        return buf;
    }
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < len; i++)
    {
        IXMLDOMNode *node;
        if (i > 0)
            *(pos++) = ' ';
        hr = IXMLDOMNodeList_nextNode(list, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        node_to_string(node, pos);
        pos += strlen(pos);
        IXMLDOMNode_Release(node);
    }
    *pos = 0;
    return buf;
}

#define expect_node(node, expstr) { char str[4096]; node_to_string(node, str); ok(strcmp(str, expstr)==0, "Invalid node: %s, expected %s\n", str, expstr); }
#define expect_list_and_release(list, expstr) { char *str = list_to_string(list); ok(strcmp(str, expstr)==0, "Invalid node list: %s, expected %s\n", str, expstr); if (list) IXMLDOMNodeList_Release(list); }

struct docload_ret_t {
    VARIANT_BOOL b;
    HRESULT hr;
};

struct leading_spaces_t {
    const CLSID *clsid;
    const char *name;
    struct docload_ret_t ret[2]; /* 0 - ::load(), 1 - ::loadXML() */
};

static const struct leading_spaces_t leading_spaces_classdata[] = {
    { &CLSID_DOMDocument,   "CLSID_DOMDocument",   {{VARIANT_FALSE, S_FALSE }, {VARIANT_TRUE,  S_OK } }},
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2",  {{VARIANT_FALSE, S_FALSE }, {VARIANT_FALSE, S_FALSE } }},
    { &CLSID_DOMDocument26, "CLSID_DOMDocument26", {{VARIANT_FALSE, S_FALSE }, {VARIANT_TRUE,  S_OK } }},
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", {{VARIANT_FALSE, S_FALSE }, {VARIANT_FALSE, S_FALSE } }},
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", {{VARIANT_FALSE, S_FALSE }, {VARIANT_FALSE, S_FALSE } }},
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", {{VARIANT_FALSE, S_FALSE }, {VARIANT_FALSE, S_FALSE } }},
    { NULL }
};

static const char* leading_spaces_xmldata[] = {
    "\n<?xml version=\"1.0\" encoding=\"UTF-16\" ?><root/>",
    " <?xml version=\"1.0\"?><root/>",
    "\n<?xml version=\"1.0\"?><root/>",
    "\t<?xml version=\"1.0\"?><root/>",
    "\r\n<?xml version=\"1.0\"?><root/>",
    "\r<?xml version=\"1.0\"?><root/>",
    "\r\r\r\r\t\t \n\n <?xml version=\"1.0\"?><root/>",
    0
};

static void test_domdoc( void )
{
    IXMLDOMDocument *doc;
    IXMLDOMParseError *error;
    IXMLDOMElement *element = NULL;
    IXMLDOMNode *node;
    IXMLDOMText *nodetext = NULL;
    IXMLDOMComment *node_comment = NULL;
    IXMLDOMAttribute *node_attr = NULL;
    IXMLDOMNode *nodeChild = NULL;
    IXMLDOMProcessingInstruction *nodePI = NULL;
    const struct leading_spaces_t *class_ptr;
    const char **data_ptr;
    VARIANT_BOOL b;
    VARIANT var;
    BSTR str;
    LONG code, ref;
    LONG nLength = 0;
    WCHAR buff[100];
    char path[MAX_PATH];
    HRESULT hr;
    int index;

    GetTempPathA(MAX_PATH, path);
    strcat(path, "leading_spaces.xml");

    /* Load document with leading spaces
     *
     * Test all CLSIDs with all test data XML strings
     */
    class_ptr = leading_spaces_classdata;
    index = 0;
    while (class_ptr->clsid)
    {
        HRESULT hr;
        int i;

        if (is_clsid_supported(class_ptr->clsid, &IID_IXMLDOMDocument))
        {
            hr = CoCreateInstance(class_ptr->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
        }
        else
        {
            class_ptr++;
            index++;
            continue;
        }

        data_ptr = leading_spaces_xmldata;
        i = 0;
        while (*data_ptr) {
            BSTR data = _bstr_(*data_ptr);
            DWORD written;
            HANDLE file;

            file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
            ok(file != INVALID_HANDLE_VALUE, "can't create file %s: %lu\n", path, GetLastError());

            WriteFile(file, data, lstrlenW(data)*sizeof(WCHAR), &written, NULL);
            CloseHandle(file);

            b = 0xc;
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = _bstr_(path);
            hr = IXMLDOMDocument_load(doc, var, &b);
            ok(hr == class_ptr->ret[0].hr, "Unexpected hr %#lx.\n", hr);
            ok(b == class_ptr->ret[0].b, "%d:%d, got %d, expected %d\n", index, i, b, class_ptr->ret[0].b);

            DeleteFileA(path);

            b = 0xc;
            hr = IXMLDOMDocument_loadXML(doc, data, &b);
            ok(hr == class_ptr->ret[1].hr, "Unexpected hr %#lx.\n", hr);
            ok(b == class_ptr->ret[1].b, "%d:%d, got %d, expected %d\n", index, i, b, class_ptr->ret[1].b);

            data_ptr++;
            i++;
        }

        class_ptr++;
        index++;
        free_bstrs();
    }

    doc = create_document(&IID_IXMLDOMDocument);
    if (!doc) return;

    check_interface(doc, &IID_IXMLDOMDocument, TRUE);
    check_interface(doc, &IID_IPersistStreamInit, TRUE);
    check_interface(doc, &IID_IObjectWithSite, TRUE);
    check_interface(doc, &IID_IObjectSafety, TRUE);
    check_interface(doc, &IID_IConnectionPointContainer, TRUE);
    check_interface(doc, &IID_IDispatch, TRUE);
    check_interface(doc, &IID_IDispatchEx, TRUE);

if (0)
{
    /* crashes on native */
    IXMLDOMDocument_loadXML( doc, (BSTR)0x1, NULL );
}

    /* try some stupid things */
    hr = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, NULL, &b );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_FALSE, "failed to load XML string\n");

    /* try to load a document from a nonexistent file */
    b = VARIANT_TRUE;
    str = SysAllocString( nonexistent_fileW );
    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = str;

    hr = IXMLDOMDocument_load( doc, var, &b);
    ok(hr == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    str = (void *)0xdeadbeef;
    hr = IXMLDOMDocument_get_url(doc, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(str == NULL, "got %p\n", str);

    /* try load an empty document */
    b = VARIANT_TRUE;
    str = SysAllocString( L"" );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    hr = IXMLDOMDocument_get_async( doc, &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "Wrong default value\n");

    /* check that there's no document element */
    element = NULL;
    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_FALSE, "should be no document element\n");

    /* try finding a node */
    node = NULL;
    str = SysAllocString( L"str1" );
    hr = IXMLDOMDocument_selectSingleNode( doc, str, &node );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    SysFreeString( str );

    b = VARIANT_TRUE;
    str = SysAllocString( L"<?xml version='1.0'?>\n" );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* check that there's no document element */
    element = (IXMLDOMElement*)1;
    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_FALSE, "should be no document element\n");
    ok( element == NULL, "Element should be NULL\n");

    /* test for BSTR handling, pass broken BSTR */
    memcpy(&buff[2], szComplete1, sizeof(szComplete1));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML( doc, &buff[2], &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    /* loadXML ignores the encoding attribute and always expects Unicode */
    b = VARIANT_FALSE;
    str = SysAllocString( L"<?xml version='1.0' encoding='Windows-1252'?>\n<open></open>\n" );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try a BSTR containing a Windows-1252 document */
    b = VARIANT_TRUE;
    str = SysAllocStringByteLen( win1252xml, strlen(win1252xml) );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* try a BSTR containing a ISO8859-1 document */
    b = VARIANT_TRUE;
    str = SysAllocStringByteLen( iso8859_1_xml, strlen(iso8859_1_xml) );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_FALSE, "loadXML succeeded\n");
    ok( b == VARIANT_FALSE, "succeeded in loading XML string\n");
    SysFreeString( str );

    /* try to load something valid */
    b = VARIANT_FALSE;
    str = SysAllocString( szComplete1 );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* check if nodename is correct */
    hr = IXMLDOMDocument_get_nodeName( doc, NULL );
    ok ( hr == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

    str = (void *)0xdeadbeef;
    hr = IXMLDOMDocument_get_baseName( doc, &str );
    ok ( hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok (str == NULL, "got %p\n", str);

    /* content doesn't matter here */
    str = NULL;
    hr = IXMLDOMDocument_get_nodeName( doc, &str );
    ok ( hr == S_OK, "get_nodeName wrong code\n");
    ok ( str != NULL, "str is null\n");
    ok( !lstrcmpW( str, L"#document" ), "incorrect nodeName\n");
    SysFreeString( str );

    /* test put_text */
    hr = IXMLDOMDocument_put_text( doc, _bstr_("Should fail") );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* check that there's a document element */
    element = NULL;
    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "should be a document element\n");
    if( element )
    {
        IObjectIdentity *ident;

        hr = IXMLDOMElement_QueryInterface( element, &IID_IObjectIdentity, (void**)&ident );
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

        IXMLDOMElement_Release( element );
        element = NULL;
    }

    /* as soon as we call loadXML again, the document element will disappear */
    b = 2;
    hr = IXMLDOMDocument_loadXML( doc, NULL, NULL );
    ok(hr == S_FALSE, "loadXML failed\n");
    ok( b == 2, "variant modified\n");
    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_FALSE, "should be no document element\n");

    /* try to load something else simple and valid */
    b = VARIANT_FALSE;
    str = SysAllocString( L"<?xml version='1.0'?>\n<a></a>\n" );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    /* try something a little more complicated */
    b = FALSE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_parseError( doc, &error );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMParseError_get_errorCode( error, &code );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( code == 0, "code %ld\n", code );
    IXMLDOMParseError_Release( error );

    /* test createTextNode */
    hr = IXMLDOMDocument_createTextNode(doc, _bstr_(""), &nodetext);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMText_Release(nodetext);

    str = SysAllocString( L"open" );
    hr = IXMLDOMDocument_createTextNode(doc, str, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createTextNode(doc, str, &nodetext);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString( str );
    if(nodetext)
    {
        hr = IXMLDOMText_QueryInterface(nodetext, &IID_IXMLDOMElement, (void**)&element);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

        /* Text Last Child Checks */
        hr = IXMLDOMText_get_lastChild(nodetext, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        nodeChild = (IXMLDOMNode*)0x1;
        hr = IXMLDOMText_get_lastChild(nodetext, &nodeChild);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(nodeChild == NULL, "nodeChild not NULL\n");

        /* test length property */
        hr = IXMLDOMText_get_length(nodetext, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_length(nodetext, &nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nLength == 4, "expected 4 got %ld\n", nLength);

        /* put data Tests */
        hr = IXMLDOMText_put_data(nodetext, _bstr_("This &is a ; test <>\\"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* get data Tests */
        hr = IXMLDOMText_get_data(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This &is a ; test <>\\"), "incorrect put_data string\n");
        SysFreeString(str);

        /* Confirm XML text is good */
        hr = IXMLDOMText_get_xml(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This &amp;is a ; test &lt;&gt;\\"), "incorrect xml string\n");
        SysFreeString(str);

        /* Confirm we get the put_data Text back */
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This &is a ; test <>\\"), "incorrect xml string\n");
        SysFreeString(str);

        /* test substringData */
        hr = IXMLDOMText_substringData(nodetext, 0, 4, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        /* test substringData - Invalid offset */
        str = (void *)0xdeadbeef;
        hr = IXMLDOMText_substringData(nodetext, -1, 4, &str);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid offset */
        str = (void *)0xdeadbeef;
        hr = IXMLDOMText_substringData(nodetext, 30, 0, &str);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid size */
        str = (void *)0xdeadbeef;
        hr = IXMLDOMText_substringData(nodetext, 0, -1, &str);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Invalid size */
        str = (void *)0xdeadbeef;
        hr = IXMLDOMText_substringData(nodetext, 2, 0, &str);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok( str == NULL, "incorrect string\n");

        /* test substringData - Start of string */
        hr = IXMLDOMText_substringData(nodetext, 0, 4, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This"), "incorrect substringData string\n");
        SysFreeString(str);

        /* test substringData - Middle of string */
        hr = IXMLDOMText_substringData(nodetext, 13, 4, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"test"), "incorrect substringData string\n");
        SysFreeString(str);

        /* test substringData - End of string */
        hr = IXMLDOMText_substringData(nodetext, 20, 4, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"\\"), "incorrect substringData string\n");
        SysFreeString(str);

        /* test appendData */
        hr = IXMLDOMText_appendData(nodetext, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_appendData(nodetext, _bstr_(""));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_appendData(nodetext, _bstr_("Append"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This &is a ; test <>\\Append"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* test insertData */
        str = SysAllocStringLen(NULL, 0);
        hr = IXMLDOMText_insertData(nodetext, -1, str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, -1, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 1000, str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 1000, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 0, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 0, str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        SysFreeString(str);

        hr = IXMLDOMText_insertData(nodetext, -1, _bstr_("Inserting"));
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 1000, _bstr_("Inserting"));
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 0, _bstr_("Begin "));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 17, _bstr_("Middle"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 39, _bstr_(" End"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"Begin This &is a Middle; test <>\\Append End"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* delete data */
        /* invalid arguments */
        hr = IXMLDOMText_deleteData(nodetext, -1, 1);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_deleteData(nodetext, 0, 0);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_deleteData(nodetext, 0, -1);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_length(nodetext, &nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nLength == 43, "expected 43 got %ld\n", nLength);

        hr = IXMLDOMText_deleteData(nodetext, nLength, 1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_deleteData(nodetext, nLength+1, 1);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        /* delete from start */
        hr = IXMLDOMText_deleteData(nodetext, 0, 5);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_length(nodetext, &nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nLength == 38, "expected 38 got %ld\n", nLength);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This &is a Middle; test <>\\Append End"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* delete from end */
        hr = IXMLDOMText_deleteData(nodetext, 35, 3);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_length(nodetext, &nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nLength == 35, "expected 35 got %ld\n", nLength);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"This &is a Middle; test <>\\Append"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* delete from inside */
        hr = IXMLDOMText_deleteData(nodetext, 1, 33);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_length(nodetext, &nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nLength == 2, "expected 2 got %ld\n", nLength);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L""), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* delete whole data ... */
        hr = IXMLDOMText_get_length(nodetext, &nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_deleteData(nodetext, 0, nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* ... and try again with empty string */
        hr = IXMLDOMText_deleteData(nodetext, 0, nLength);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* test put_data */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"str1");
        hr = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        VariantClear(&var);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok( !lstrcmpW( str, L"str1" ), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str) );
        SysFreeString(str);

        /* test put_data */
        V_VT(&var) = VT_I4;
        V_I4(&var) = 99;
        hr = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        VariantClear(&var);

        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"99"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* ::replaceData() */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"str1");
        hr = IXMLDOMText_put_nodeValue(nodetext, var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        VariantClear(&var);

        hr = IXMLDOMText_replaceData(nodetext, 6, 0, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"str1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMText_replaceData(nodetext, 0, 0, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"str1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* NULL pointer means delete */
        hr = IXMLDOMText_replaceData(nodetext, 0, 1, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"tr1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* empty string means delete */
        hr = IXMLDOMText_replaceData(nodetext, 0, 1, _bstr_(""));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"r1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* zero count means insert */
        hr = IXMLDOMText_replaceData(nodetext, 0, 0, _bstr_("a"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"ar1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMText_replaceData(nodetext, 0, 2, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_insertData(nodetext, 0, _bstr_("m"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"m1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* nonempty string, count greater than its length */
        hr = IXMLDOMText_replaceData(nodetext, 0, 2, _bstr_("a1.2"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"a1.2"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* nonempty string, count less than its length */
        hr = IXMLDOMText_replaceData(nodetext, 0, 1, _bstr_("wine"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMText_get_text(nodetext, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"wine1.2"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMText_Release( nodetext );
    }

    /* test Create Comment */
    hr = IXMLDOMDocument_createComment(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    node_comment = (IXMLDOMComment*)0x1;

    /* empty comment */
    hr = IXMLDOMDocument_createComment(doc, _bstr_(""), &node_comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = NULL;
    hr = IXMLDOMComment_get_data(node_comment, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty string data\n");
    IXMLDOMComment_Release(node_comment);
    SysFreeString(str);

    hr = IXMLDOMDocument_createComment(doc, NULL, &node_comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = NULL;
    hr = IXMLDOMComment_get_data(node_comment, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && (SysStringLen(str) == 0), "expected empty string data\n");
    IXMLDOMComment_Release(node_comment);
    SysFreeString(str);

    str = SysAllocString(L"A Comment");
    hr = IXMLDOMDocument_createComment(doc, str, &node_comment);
    SysFreeString(str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if(node_comment)
    {
        /* Last Child Checks */
        hr = IXMLDOMComment_get_lastChild(node_comment, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        nodeChild = (IXMLDOMNode*)0x1;
        hr = IXMLDOMComment_get_lastChild(node_comment, &nodeChild);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(nodeChild == NULL, "pLastChild not NULL\n");

        /* baseName */
        str = (void *)0xdeadbeef;
        hr = IXMLDOMComment_get_baseName(node_comment, &str);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(str == NULL, "Expected NULL\n");

        IXMLDOMComment_Release( node_comment );
    }

    /* test Create Attribute */
    str = SysAllocString(L"Attr");
    hr = IXMLDOMDocument_createAttribute(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createAttribute(doc, str, &node_attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMAttribute_Release( node_attr);
    SysFreeString(str);

    /* test Processing Instruction */
    str = SysAllocStringLen(NULL, 0);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, str, str, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, NULL, str, &nodePI);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, str, str, &nodePI);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\""), &nodePI);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if(nodePI)
    {
        /* Last Child Checks */
        hr = IXMLDOMProcessingInstruction_get_lastChild(nodePI, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        nodeChild = (IXMLDOMNode*)0x1;
        hr = IXMLDOMProcessingInstruction_get_lastChild(nodePI, &nodeChild);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(nodeChild == NULL, "nodeChild not NULL\n");

        /* test nodeName */
        hr = IXMLDOMProcessingInstruction_get_nodeName(nodePI, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"xml"), "incorrect nodeName string\n");
        SysFreeString(str);

        /* test baseName */
        str = NULL;
        hr = IXMLDOMProcessingInstruction_get_baseName(nodePI, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"xml"), "incorrect nodeName string\n");
        SysFreeString(str);

        /* test Target */
        hr = IXMLDOMProcessingInstruction_get_target(nodePI, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"xml"), "incorrect target string\n");
        SysFreeString(str);

        /* test get_data */
        hr = IXMLDOMProcessingInstruction_get_data(nodePI, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"version=\"1.0\""), "incorrect data string\n");
        SysFreeString(str);

        /* test put_data */
        hr = IXMLDOMProcessingInstruction_put_data(nodePI, _bstr_("version=\"1.0\" encoding=\"UTF-8\""));
        ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

        /* test put_data */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"open");  /* Doesn't matter what the string is, cannot set an xml node. */
        hr = IXMLDOMProcessingInstruction_put_nodeValue(nodePI, var);
        ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
        VariantClear(&var);

        /* test get nodeName */
        hr = IXMLDOMProcessingInstruction_get_nodeName(nodePI, &str);
        ok(!lstrcmpW(str, L"xml"), "incorrect nodeName string\n");
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        SysFreeString(str);

        IXMLDOMProcessingInstruction_Release(nodePI);
    }

    ref = IXMLDOMDocument_Release( doc );
    ok( ref == 0, "got %ld\n", ref);

    free_bstrs();
}

static void test_persiststream(void)
{
    IPersistStreamInit *streaminit;
    IPersistStream *stream;
    IXMLDOMDocument *doc;
    ULARGE_INTEGER size;
    IStream *istream;
    HRESULT hr;
    CLSID clsid;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&streaminit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IPersistStreamInit_InitNew(streaminit);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IPersistStreamInit_GetSizeMax(streaminit, &size);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IPersistStream, (void **)&stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((IUnknown *)stream == (IUnknown *)streaminit, "got %p, %p\n", stream, streaminit);

    check_interface(stream, &IID_IPersist, FALSE);
    check_interface(doc, &IID_IPersist, FALSE);

    hr = IPersistStreamInit_GetClassID(streaminit, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    memset(&clsid, 0, sizeof(clsid));
    hr = IPersistStreamInit_GetClassID(streaminit, &clsid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&clsid, &CLSID_DOMDocument2), "wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    IPersistStream_Release(stream);

    /* test Load */
    istream = SHCreateMemStream((const BYTE*)complete4A, strlen(complete4A));
    hr = IPersistStreamInit_Load(streaminit, istream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IStream_Release(istream);
    EXPECT_PARSE_ERROR(doc, S_OK, FALSE);

    istream = SHCreateMemStream((const BYTE*)"", 0);
    hr = IPersistStreamInit_Load(streaminit, istream);
    todo_wine ok(hr == XML_E_MISSINGROOT, "Unexpected hr %#lx.\n", hr);
    ok(FAILED(hr), "got success\n");
    IStream_Release(istream);
    EXPECT_PARSE_ERROR(doc, XML_E_MISSINGROOT, TRUE);

    IPersistStreamInit_Release(streaminit);
    IXMLDOMDocument_Release(doc);
}

static void test_domnode( void )
{
    IXMLDOMDocument *doc, *owner = NULL;
    IXMLDOMElement *element = NULL;
    IXMLDOMNamedNodeMap *map = NULL;
    IXMLDOMNode *node = NULL, *next = NULL;
    IXMLDOMNodeList *list = NULL;
    IXMLDOMAttribute *attr = NULL;
    DOMNodeType type = NODE_INVALID;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;
    VARIANT var;
    LONG count;

    doc = create_document(&IID_IXMLDOMDocument);

    b = FALSE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    EXPECT_CHILDREN(doc);

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    VariantInit(&var);
    ok( V_VT(&var) == VT_EMPTY, "variant init failed\n");

    hr = IXMLDOMDocument_get_nodeValue( doc, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_nodeValue( doc, &var );
    ok(hr == S_FALSE, "get_nodeValue returned wrong code\n");
    ok( V_VT(&var) == VT_NULL, "variant wasn't empty\n");
    ok( V_BSTR(&var) == NULL, "variant value wasn't null\n");

    if (element)
    {
        IXMLDOMNamedNodeMap *attributes;

        owner = NULL;
        hr = IXMLDOMElement_get_ownerDocument( element, &owner );
        ok(hr == S_OK, "get_ownerDocument return code\n");
        ok( owner != doc, "get_ownerDocument return\n");
        IXMLDOMDocument_Release(owner);

        type = NODE_INVALID;
        hr = IXMLDOMElement_get_nodeType( element, &type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok( type == NODE_ELEMENT, "node not an element\n");

        str = NULL;
        hr = IXMLDOMElement_get_baseName( element, &str );
        ok(hr == S_OK, "get_baseName returned wrong code\n");
        ok( !lstrcmpW(str, L"lc"), "Unexpected basename.\n");
        SysFreeString(str);

        /* check if nodename is correct */
        hr = IXMLDOMElement_get_nodeName( element, NULL );
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        /* content doesn't matter here */
        str = NULL;
        hr = IXMLDOMElement_get_nodeName( element, &str );
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, L"lc" ), "Unexpected nodeName\n");
        SysFreeString( str );

        str = SysAllocString( nonexistent_fileW );
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        hr = IXMLDOMElement_getAttribute( element, str, &var );
        ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
        ok( V_VT(&var) == VT_NULL || V_VT(&var) == VT_EMPTY, "vt = %x\n", V_VT(&var));
        VariantClear(&var);
        SysFreeString(str);

        str = SysAllocString( L"dl" );
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0x1234;
        hr = IXMLDOMElement_getAttribute( element, str, &var );
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
        ok( !lstrcmpW(V_BSTR(&var), L"str1"), "wrong attr value\n");
        VariantClear( &var );

        hr = IXMLDOMElement_getAttribute( element, NULL, &var );
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMElement_getAttribute( element, str, NULL );
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        attr = NULL;
        hr = IXMLDOMElement_getAttributeNode( element, str, &attr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok( attr != NULL, "getAttributeNode returned NULL\n" );
        if (attr)
        {
            hr = IXMLDOMAttribute_get_parentNode( attr, NULL );
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            /* attribute doesn't have a parent in msxml interpretation */
            node = (IXMLDOMNode*)0xdeadbeef;
            hr = IXMLDOMAttribute_get_parentNode( attr, &node );
            ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
            ok( node == NULL, "Expected NULL, got %p\n", node );

            IXMLDOMAttribute_Release(attr);
        }

        SysFreeString( str );

        hr = IXMLDOMElement_get_attributes( element, &map );
        ok(hr == S_OK, "get_attributes returned wrong code\n");
        ok( map != NULL, "should be attributes\n");

        EXPECT_CHILDREN(element);

        hr = IXMLDOMElement_get_childNodes( element, &list );
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMNodeList_nextNode( list, &node ); /* <bs> */
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IXMLDOMNode_Release( node );
        hr = IXMLDOMNodeList_nextNode( list, &node ); /* <pr> */
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IXMLDOMNode_Release( node );
        hr = IXMLDOMNodeList_nextNode( list, &node ); /* <empty> */
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMNode_get_attributes( node, &attributes );
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        next = (IXMLDOMNode*)0xdeadbeef;
        hr = IXMLDOMNamedNodeMap_nextNode( attributes, &next );
        ok(hr == S_FALSE, "Unexpected %#lx.\n", hr);
        ok( next == NULL, "Expected NULL, ret %p\n", next );
        IXMLDOMNamedNodeMap_Release( attributes );
        IXMLDOMNode_Release( node );
        node = NULL;
        next = NULL;
        IXMLDOMNodeList_Release( list );
        list = NULL;
    }
    else
        ok( FALSE, "no element\n");

    if (map)
    {
        str = SysAllocString( L"dl" );
        hr = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok(hr == S_OK, "getNamedItem returned wrong code\n");
        ok( node != NULL, "should be attributes\n");
        IXMLDOMNode_Release(node);
        SysFreeString( str );

        str = SysAllocString( L"dl" );
        hr = IXMLDOMNamedNodeMap_getNamedItem( map, str, NULL );
        ok(hr == E_INVALIDARG, "getNamedItem should return E_INVALIDARG\n");
        SysFreeString( str );

        /* something that isn't in complete4A */
        str = SysAllocString( L"open" );
        node = (IXMLDOMNode *) 1;
        hr = IXMLDOMNamedNodeMap_getNamedItem( map, str, &node );
        ok(hr == S_FALSE, "getNamedItem found a node that wasn't there\n");
        ok( node == NULL, "getNamedItem should have returned NULL\n");
        SysFreeString( str );

	/* test indexed access of attributes */
        hr = IXMLDOMNamedNodeMap_get_length( map, NULL );
        ok ( hr == E_INVALIDARG, "get_length should return E_INVALIDARG\n");

        hr = IXMLDOMNamedNodeMap_get_length( map, &count );
        ok ( hr == S_OK, "get_length wrong code\n");
        ok ( count == 1, "get_length != 1\n");

        node = NULL;
        hr = IXMLDOMNamedNodeMap_get_item( map, -1, &node);
        ok ( hr == S_FALSE, "get_item (-1) wrong code\n");
        ok ( node == NULL, "there is no node\n");

        node = NULL;
        hr = IXMLDOMNamedNodeMap_get_item( map, 1, &node);
        ok ( hr == S_FALSE, "get_item (1) wrong code\n");
        ok ( node == NULL, "there is no attribute\n");

        node = NULL;
        hr = IXMLDOMNamedNodeMap_get_item( map, 0, &node);
        ok ( hr == S_OK, "get_item (0) wrong code\n");
        ok ( node != NULL, "should be attribute\n");

        hr = IXMLDOMNode_get_nodeName( node, NULL );
        ok ( hr == E_INVALIDARG, "get_nodeName (NULL) wrong code\n");

        /* content doesn't matter here */
        str = NULL;
        hr = IXMLDOMNode_get_nodeName( node, &str );
        ok ( hr == S_OK, "get_nodeName wrong code\n");
        ok ( str != NULL, "str is null\n");
        ok( !lstrcmpW( str, L"dl" ), "incorrect node name\n");
        SysFreeString( str );
        IXMLDOMNode_Release( node );

        /* test sequential access of attributes */
        node = NULL;
        hr = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( hr == S_OK, "nextNode (first time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");
        IXMLDOMNode_Release( node );

        hr = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( hr != S_OK, "nextNode (second time) wrong code\n");
        ok ( node == NULL, "nextNode, there is no attribute\n");

        hr = IXMLDOMNamedNodeMap_reset( map );
        ok ( hr == S_OK, "reset should return S_OK\n");

        hr = IXMLDOMNamedNodeMap_nextNode( map, &node );
        ok ( hr == S_OK, "nextNode (third time) wrong code\n");
        ok ( node != NULL, "nextNode, should be attribute\n");
    }
    else
        ok( FALSE, "no map\n");

    if (node)
    {
        type = NODE_INVALID;
        hr = IXMLDOMNode_get_nodeType( node, &type);
        ok(hr == S_OK, "get_nodeType returned wrong code\n");
        ok( type == NODE_ATTRIBUTE, "node not an attribute\n");

        str = NULL;
        hr = IXMLDOMNode_get_baseName( node, NULL );
        ok(hr == E_INVALIDARG, "get_baseName returned wrong code\n");

        str = NULL;
        hr = IXMLDOMNode_get_baseName( node, &str );
        ok(hr == S_OK, "get_baseName returned wrong code\n");
        ok( !lstrcmpW(str, L"dl"), "Unexpected basename.\n");
        SysFreeString( str );

        hr = IXMLDOMNode_get_childNodes( node, NULL );
        ok(hr == E_INVALIDARG, "get_childNodes returned wrong code\n");

        hr = IXMLDOMNode_get_childNodes( node, &list );
        ok(hr == S_OK, "get_childNodes returned wrong code\n");

        if (list)
        {
            hr = IXMLDOMNodeList_nextNode( list, &next );
            ok(hr == S_OK, "nextNode returned wrong code\n");
        }
        else
            ok( FALSE, "no childlist\n");

        if (next)
        {
            EXPECT_NO_CHILDREN(next);

            type = NODE_INVALID;
            hr = IXMLDOMNode_get_nodeType( next, &type);
            ok(hr == S_OK, "get_nodeType returned wrong code\n");
            ok( type == NODE_TEXT, "node not text\n");

            str = (void *)0xdeadbeef;
            hr = IXMLDOMNode_get_baseName( next, &str );
            ok(hr == S_FALSE, "get_baseName returned wrong code\n");
            ok( str == NULL, "basename was wrong\n");
            SysFreeString(str);
        }
        else
            ok( FALSE, "no next\n");

        if (next)
            IXMLDOMNode_Release( next );
        next = NULL;
        if (list)
            IXMLDOMNodeList_Release( list );
        list = NULL;
        if (node)
            IXMLDOMNode_Release( node );
    }
    else
        ok( FALSE, "no node\n");
    node = NULL;

    if (map)
        IXMLDOMNamedNodeMap_Release( map );

    /* now traverse the tree from the root element */
    if (element)
    {
        hr = IXMLDOMElement_get_childNodes( element, &list );
        ok(hr == S_OK, "get_childNodes returned wrong code\n");

        /* using get_item for child list doesn't advance the position */
        hr = IXMLDOMNodeList_get_item(list, 1, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        expect_node(node, "E2.E2.D1");
        IXMLDOMNode_Release(node);
        hr = IXMLDOMNodeList_nextNode(list, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        expect_node(node, "E1.E2.D1");
        IXMLDOMNode_Release(node);
        hr = IXMLDOMNodeList_reset(list);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IXMLDOMNodeList_AddRef(list);
        expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1");
        hr = IXMLDOMNodeList_reset(list);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        node = (void*)0xdeadbeef;
        str = SysAllocString(L"dl");
        hr = IXMLDOMElement_selectSingleNode( element, str, &node );
        SysFreeString(str);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok( node == NULL, "node %p\n", node );

        node = (void*)0xdeadbeef;
        hr = IXMLDOMElement_selectSingleNode( element, _bstr_("In Valid"), &node );
        ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
        ok( node == NULL, "node %p\n", node );

        str = SysAllocString(L"bs");
        hr = IXMLDOMElement_selectSingleNode( element, str, &node );
        SysFreeString(str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IXMLDOMNode_Release( node );
    }
    else
        ok( FALSE, "no element\n");

    if (list)
    {
        hr = IXMLDOMNodeList_get_item(list, 0, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNodeList_get_length(list, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNodeList_get_length( list, &count );
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok( count == 4, "get_length got %ld\n", count );

        hr = IXMLDOMNodeList_nextNode(list, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNodeList_nextNode( list, &node );
        ok(hr == S_OK, "nextNode returned wrong code\n");
    }
    else
        ok( FALSE, "no list\n");

    if (node)
    {
        type = NODE_INVALID;
        hr = IXMLDOMNode_get_nodeType( node, &type);
        ok(hr == S_OK, "get_nodeType returned wrong code\n");
        ok( type == NODE_ELEMENT, "node not text\n");

        hr = IXMLDOMNode_hasChildNodes( node, NULL );
        ok(hr == E_INVALIDARG, "hasChildNodes bad return\n");

        EXPECT_CHILDREN(node);

        str = NULL;
        hr = IXMLDOMNode_get_baseName( node, &str );
        ok(hr == S_OK, "get_baseName returned wrong code\n");
        ok( !lstrcmpW(str, L"bs"), "Unexpected basename.\n");
        SysFreeString(str);
    }
    else
        ok( FALSE, "no node\n");

    if (node)
        IXMLDOMNode_Release( node );
    if (list)
        IXMLDOMNodeList_Release( list );
    if (element)
        IXMLDOMElement_Release( element );

    b = FALSE;
    str = SysAllocString( szComplete5 );
    hr = IXMLDOMDocument_loadXML( doc, str, &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    SysFreeString( str );

    EXPECT_CHILDREN(doc);

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "should be a document element\n");
    ok( element != NULL, "should be an element\n");

    if (element)
    {
        BSTR tag = NULL;

        /* check if the tag is correct */
        hr = IXMLDOMElement_get_tagName( element, &tag );
        ok(hr == S_OK, "couldn't get tag name\n");
        ok( tag != NULL, "tag was null\n");
        ok( !lstrcmpW( tag, L"S:search" ), "incorrect tag name\n");
        SysFreeString( tag );

        IXMLDOMElement_Release( element );
    }
    ok(IXMLDOMDocument_Release( doc ) == 0, "document is not destroyed\n");

    free_bstrs();
}

typedef struct {
    DOMNodeType type;
    REFIID iid;
} refcount_test_t;

static const refcount_test_t refcount_test[] = {
    { NODE_ELEMENT,                &IID_IXMLDOMElement },
    { NODE_ATTRIBUTE,              &IID_IXMLDOMAttribute },
    { NODE_TEXT,                   &IID_IXMLDOMText },
    { NODE_CDATA_SECTION,          &IID_IXMLDOMCDATASection },
    { NODE_ENTITY_REFERENCE,       &IID_IXMLDOMEntityReference },
    { NODE_PROCESSING_INSTRUCTION, &IID_IXMLDOMProcessingInstruction },
    { NODE_COMMENT,                &IID_IXMLDOMComment },
    { NODE_DOCUMENT_FRAGMENT,      &IID_IXMLDOMDocumentFragment },
    { NODE_INVALID,                &IID_NULL }
};

static void test_refs(void)
{
    IXMLDOMImplementation *impl, *impl2;
    IXMLDOMElement *element, *elem2;
    IXMLDOMNodeList *node_list = NULL;
    IXMLDOMNode *node, *node2, *node3;
    const refcount_test_t *ptr;
    IXMLDOMDocument *doc;
    IUnknown *unk, *unk2;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG ref;

    doc = create_document(&IID_IXMLDOMDocument);

    ptr = refcount_test;
    while (ptr->type != NODE_INVALID)
    {
        IUnknown *node_typed, *node_typed2;
        IDispatchEx *dispex, *dispex2;
        IDispatch *disp, *disp2;
        VARIANT type;

        V_VT(&type) = VT_I1;
        V_I1(&type) = ptr->type;

        EXPECT_REF(doc, 1);
        hr = IXMLDOMDocument_createNode(doc, type, _bstr_("name"), NULL, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(doc, 1);
        EXPECT_REF(node, 1);

        /* try IDispatch and IUnknown from IXMLDOMNode */
        hr = IXMLDOMNode_QueryInterface(node, &IID_IUnknown, (void**)&unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(unk, 2);
todo_wine {
        EXPECT_REF(node, 1);
        ok(unk != (IUnknown*)node, "%d: got %p and %p\n", ptr->type, unk, node);
}
        EXPECT_REF(unk, 2);
        hr = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(unk != (IUnknown*)disp, "%d: got %p and %p\n", ptr->type, unk, disp);
        EXPECT_REF(unk, 3);
        todo_wine EXPECT_REF(disp, 1);

        EXPECT_REF(unk, 3);
        hr = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(disp != disp2, "%d: got %p and %p\n", ptr->type, disp, disp2);
        EXPECT_REF(unk, 4);
        todo_wine EXPECT_REF(disp2, 1);

        IDispatch_Release(disp);
        IDispatch_Release(disp2);

        /* get IXMLDOMNode from this IUnknown */
        EXPECT_REF(unk, 2);
        hr = IUnknown_QueryInterface(unk, &IID_IXMLDOMNode, (void**)&node2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(unk != (IUnknown*)node2, "%d: got %p and %p\n", ptr->type, unk, node2);
        EXPECT_REF(unk, 3);
        todo_wine EXPECT_REF(node2, 1);

        EXPECT_REF(unk, 3);
        hr = IUnknown_QueryInterface(unk, &IID_IXMLDOMNode, (void**)&node3);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(node2 != node3, "%d: got %p and %p\n", ptr->type, node2, node3);
        EXPECT_REF(unk, 4);
        todo_wine EXPECT_REF(node3, 1);

        IXMLDOMNode_Release(node2);
        IXMLDOMNode_Release(node3);

        /* try IDispatchEx from IUnknown */
        EXPECT_REF(unk, 2);
        hr = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(unk != (IUnknown*)dispex, "%d: got %p and %p\n", ptr->type, unk, dispex);
        EXPECT_REF(unk, 3);
        todo_wine EXPECT_REF(dispex, 1);

        EXPECT_REF(unk, 3);
        hr = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        todo_wine ok(dispex != dispex2, "%d: got %p and %p\n", ptr->type, dispex, dispex2);
        EXPECT_REF(unk, 4);
        todo_wine EXPECT_REF(dispex2, 1);

        IDispatchEx_Release(dispex);
        IDispatchEx_Release(dispex2);

        /* try corresponding IXMLDOM* */
        EXPECT_REF(unk, 2);
        hr = IUnknown_QueryInterface(unk, ptr->iid, (void**)&node_typed);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(unk, 3);
        hr = IUnknown_QueryInterface(unk, ptr->iid, (void**)&node_typed2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(unk, 4);
        todo_wine ok(node_typed != node_typed2, "%d: got %p and %p\n", ptr->type, node_typed, node_typed2);
        IUnknown_Release(node_typed);
        IUnknown_Release(node_typed2);

        /* try invalid IXMLDOM* */
        hr = IUnknown_QueryInterface(unk, (ptr+1)->iid, (void**)&node_typed);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

        IUnknown_Release(unk);

        EXPECT_REF(node, 1);
        hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMNode, (void**)&node2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(node, 2);
        ok(node == node2, "%d: got %p and %p\n", ptr->type, node, node2);

        EXPECT_REF(node, 2);
        hr = IXMLDOMNode_QueryInterface(node, ptr->iid, (void**)&node_typed);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(node, 3);
todo_wine {
        EXPECT_REF(node_typed, 2);
        ok((IUnknown*)node != node_typed, "%d: got %p and %p\n", ptr->type, node, node_typed);
}
        IUnknown_Release(node_typed);

        IXMLDOMNode_Release(node2);
        IXMLDOMNode_Release(node);

        ptr++;
    }

    EXPECT_REF(doc, 1);
    ref = IXMLDOMDocument_Release(doc);
    ok( ref == 0, "ref %ld\n", ref);

    /* check IUnknown after releasing DOM iface */
    doc = create_document(&IID_IXMLDOMDocument);
    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
todo_wine {
    EXPECT_REF(unk, 3);
    EXPECT_REF(doc, 1);
}
    IXMLDOMDocument_Release(doc);
    EXPECT_REF(unk, 1);
    IUnknown_Release(unk);

    doc = create_document(&IID_IXMLDOMDocument);

    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
todo_wine {
    EXPECT_REF(unk, 3);
    EXPECT_REF(doc, 1);
}
    IUnknown_Release(unk);

    /* IXMLDOMImplementation */
    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_get_implementation(doc, &impl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(doc, 1);
    EXPECT_REF(impl, 1);
    hr = IXMLDOMDocument_get_implementation(doc, &impl2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(doc, 1);
    EXPECT_REF(impl2, 1);
    ok(impl != impl2, "got %p, %p\n", impl, impl2);
    IXMLDOMImplementation_Release(impl);
    IXMLDOMImplementation_Release(impl2);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    EXPECT_REF(doc, 1);
    IXMLDOMDocument_AddRef( doc );
    EXPECT_REF(doc, 2);
    IXMLDOMDocument_AddRef( doc );
    EXPECT_REF(doc, 3);

    IXMLDOMDocument_Release( doc );
    IXMLDOMDocument_Release( doc );

    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
todo_wine {
    EXPECT_REF(unk, 3);
    EXPECT_REF(doc, 1);
}
    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
todo_wine {
    EXPECT_REF(doc, 1);
    EXPECT_REF(element, 2);
}
    hr = IXMLDOMDocument_get_documentElement(doc, &elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

todo_wine {
    EXPECT_REF(doc, 1);
    EXPECT_REF(element, 2);
    EXPECT_REF(elem2, 2);
}
    IXMLDOMElement_AddRef(element);
    todo_wine EXPECT_REF(element, 3);
    IXMLDOMElement_Release(element);

    /* get IUnknown from a node doesn't touch node instance refcount */
    hr = IXMLDOMElement_QueryInterface(element, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(element, 2);
todo_wine {
    EXPECT_REF(unk, 4);
    EXPECT_REF(elem2, 2);
}
    hr = IXMLDOMElement_QueryInterface(elem2, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
todo_wine {
    EXPECT_REF(unk, 5);
    EXPECT_REF(unk2, 5);
}
    EXPECT_REF(element, 2);
    EXPECT_REF(elem2, 2);

    todo_wine ok(unk == unk2, "got %p and %p\n", unk, unk2);
    IUnknown_Release(unk);

    /* IUnknown refcount is not affected by node refcount */
    todo_wine EXPECT_REF(unk2, 4);
    IXMLDOMElement_AddRef(elem2);
    todo_wine EXPECT_REF(unk2, 4);
    IXMLDOMElement_Release(elem2);

    IXMLDOMElement_Release(elem2);
    todo_wine EXPECT_REF(unk2, 3);

    IUnknown_Release(unk2);

    hr = IXMLDOMElement_get_childNodes( element, &node_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    todo_wine EXPECT_REF(element, 2);
    EXPECT_REF(node_list, 1);

    hr = IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 1);
    EXPECT_REF(node, 1);

    hr = IXMLDOMNodeList_get_item( node_list, 0, &node2 );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 1);
    EXPECT_REF(node2, 1);

    ref = IXMLDOMNode_Release( node );
    ok( ref == 0, "ref %ld\n", ref );
    ref = IXMLDOMNode_Release( node2 );
    ok( ref == 0, "ref %ld\n", ref );

    ref = IXMLDOMNodeList_Release( node_list );
    ok( ref == 0, "ref %ld\n", ref );

    ok( node != node2, "node %p node2 %p\n", node, node2 );

    ref = IXMLDOMDocument_Release( doc );
    todo_wine ok( ref == 0, "ref %ld\n", ref );

    todo_wine EXPECT_REF(element, 2);

    /* IUnknown must be unique however we obtain it */
    hr = IXMLDOMElement_QueryInterface(element, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(element, 2);
    hr = IXMLDOMElement_QueryInterface(element, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(element, 2);
    hr = IXMLDOMNode_QueryInterface(node, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(element, 2);
    ok(unk == unk2, "unk %p unk2 %p\n", unk, unk2);
    todo_wine ok(element != (void*)node, "node %p element %p\n", node, element);

    IUnknown_Release( unk2 );
    IUnknown_Release( unk );
    IXMLDOMNode_Release( node );
    todo_wine EXPECT_REF(element, 2);

    IXMLDOMElement_Release( element );

    free_bstrs();
}

static void test_create(void)
{
    VARIANT var;
    BSTR str, name;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMComment *comment;
    IXMLDOMText *text;
    IXMLDOMCDATASection *cdata;
    IXMLDOMNode *root, *node, *child;
    IXMLDOMNamedNodeMap *attr_map;
    IUnknown *unk;
    HRESULT hr;
    LONG ref;
    LONG num;

    doc = create_document(&IID_IXMLDOMDocument);

    EXPECT_REF(doc, 1);

    /* types not supported for creation */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_DOCUMENT;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_DOCUMENT_TYPE;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_NOTATION;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    /* NODE_COMMENT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_COMMENT;
    node = NULL;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( node != NULL, "\n");

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMComment_get_data(comment, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMComment_get_data(comment, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMComment_get_data(comment, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMComment_Release(comment);
    SysFreeString(str);

    /* NODE_TEXT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_TEXT;
    node = NULL;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( node != NULL, "\n");

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMText_get_data(text, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMText_get_data(text, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMText_get_data(text, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMText_Release(text);
    SysFreeString(str);

    /* NODE_CDATA_SECTION */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_CDATA_SECTION;
    node = NULL;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( node != NULL, "\n");

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMCDATASection_get_data(cdata, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMCDATASection_get_data(cdata, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_("blah"), NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    str = NULL;
    hr = IXMLDOMCDATASection_get_data(cdata, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( str && SysStringLen(str) == 0, "expected empty comment, %p\n", str);
    IXMLDOMCDATASection_Release(cdata);
    SysFreeString(str);

    /* NODE_ATTRIBUTE */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( L"lc" );
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) IXMLDOMNode_Release(node);
    SysFreeString(str);

    /* a name is required for attribute, try a BSTR with first null wchar */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( L"str1" );
    str[0] = 0;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);
    SysFreeString(str);

    /* NODE_PROCESSING_INSTRUCTION */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_PROCESSING_INSTRUCTION;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_("pi"), NULL, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* NODE_ENTITY_REFERENCE */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY_REFERENCE;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ENTITY_REFERENCE;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    /* NODE_ELEMENT */
    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, NULL, NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    node = (IXMLDOMNode*)0x1;
    hr = IXMLDOMDocument_createNode( doc, var, _bstr_(""), NULL, &node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0x1, "expected same ptr, got %p\n", node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    str = SysAllocString( L"lc" );
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if( SUCCEEDED(hr) ) IXMLDOMNode_Release( node );

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_R4;
    V_R4(&var) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if( SUCCEEDED(hr) ) IXMLDOMNode_Release( node );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( L"1" );
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if( SUCCEEDED(hr) ) IXMLDOMNode_Release( node );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( L"1Garbage" );
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    if( SUCCEEDED(hr) ) IXMLDOMNode_Release( node );
    VariantClear(&var);

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_appendChild( doc, node, &root );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( node == root, "%p %p\n", node, root );
    EXPECT_REF(doc, 1);

    EXPECT_REF(node, 2);

    ref = IXMLDOMNode_Release( node );
    ok(ref == 1, "ref %ld\n", ref);
    SysFreeString( str );

    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ELEMENT;
    str = SysAllocString( L"bs" );
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString( str );

    EXPECT_REF(node, 1);

    hr = IXMLDOMNode_QueryInterface( node, &IID_IUnknown, (void**)&unk );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(unk, 2);

    V_VT(&var) = VT_EMPTY;
    child = NULL;
    hr = IXMLDOMNode_insertBefore( root, (IXMLDOMNode*)unk, var, &child );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( unk == (IUnknown*)child, "%p %p\n", unk, child );

    todo_wine EXPECT_REF(unk, 4);

    IXMLDOMNode_Release( child );
    IUnknown_Release( unk );

    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    hr = IXMLDOMNode_insertBefore( root, node, var, &child );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( node == child, "%p %p\n", node, child );
    IXMLDOMNode_Release( child );

    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = (IDispatch*)node;
    hr = IXMLDOMNode_insertBefore( root, node, var, NULL );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release( node );

    hr = IXMLDOMNode_QueryInterface( root, &IID_IXMLDOMElement, (void**)&element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_attributes( element, &attr_map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( num == 0, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( L"str1" );
    name = SysAllocString( L"dl" );
    hr = IXMLDOMElement_setAttribute( element, name, var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_get_attributes( element, &attr_map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( num == 1, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( L"str2" );
    hr = IXMLDOMElement_setAttribute( element, name, var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_get_attributes( element, &attr_map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( num == 1, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    hr = IXMLDOMElement_getAttribute( element, name, &var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( !lstrcmpW(V_BSTR(&var), L"str2"), "wrong attr value\n");
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString( L"str1" );
    name = SysAllocString( L"lc" );
    hr = IXMLDOMElement_setAttribute( element, name, var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_get_attributes( element, &attr_map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNamedNodeMap_get_length( attr_map, &num );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( num == 2, "num %ld\n", num );
    IXMLDOMNamedNodeMap_Release( attr_map );
    VariantClear(&var);
    SysFreeString(name);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    name = SysAllocString( L"bs" );
    hr = IXMLDOMElement_setAttribute( element, name, var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);
    hr = IXMLDOMElement_getAttribute( element, name, &var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_BSTR, "variant type %x\n", V_VT(&var));
    VariantClear(&var);
    SysFreeString(name);

    /* Create an Attribute */
    V_VT(&var) = VT_I4;
    V_I4(&var) = NODE_ATTRIBUTE;
    str = SysAllocString( L"Attr" );
    hr = IXMLDOMDocument_createNode( doc, var, str, NULL, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( node != NULL, "node was null\n");
    SysFreeString(str);

    IXMLDOMElement_Release( element );
    IXMLDOMNode_Release( root );
    IXMLDOMDocument_Release( doc );
}

struct queryresult_t {
    const char *query;
    const char *result;
    int len;
};

static const struct queryresult_t elementsbytagname[] = {
    { "",    "P1.D1 E2.D1 E1.E2.D1 T1.E1.E2.D1 E2.E2.D1 T1.E2.E2.D1 E3.E2.D1 E4.E2.D1 E1.E4.E2.D1 T1.E1.E4.E2.D1", 10 },
    { "*",   "E2.D1 E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1 E1.E4.E2.D1", 6 },
    { "bs",  "E1.E2.D1", 1 },
    { "dl",  "", 0 },
    { "str1","", 0 },
    { NULL }
};

static void test_getElementsByTagName(void)
{
    const struct queryresult_t *ptr = elementsbytagname;
    IXMLDOMNodeList *node_list;
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    WCHAR buff[100];
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    /* null arguments cases */
    hr = IXMLDOMDocument_getElementsByTagName(doc, NULL, &node_list);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_getElementsByTagName(doc, _bstr_("*"), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    while (ptr->query)
    {
        hr = IXMLDOMDocument_getElementsByTagName(doc, _bstr_(ptr->query), &node_list);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMNodeList_get_length(node_list, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len == ptr->len, "%s: got len %ld, expected %d\n", ptr->query, len, ptr->len);
        expect_list_and_release(node_list, ptr->result);

        free_bstrs();
        ptr++;
    }

    /* broken query BSTR */
    memcpy(&buff[2], L"*", 2 * sizeof(WCHAR));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    hr = IXMLDOMDocument_getElementsByTagName(doc, &buff[2], &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_get_length( node_list, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 6, "len %ld\n", len );
    IXMLDOMNodeList_Release( node_list );

    /* test for element */
    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = SysAllocString( L"*" );

    /* null arguments cases */
    hr = IXMLDOMElement_getElementsByTagName(elem, NULL, &node_list);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_getElementsByTagName(elem, str, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_getElementsByTagName(elem, str, &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_get_length( node_list, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 5, "len %ld\n", len );
    expect_list_and_release(node_list, "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1 E1.E4.E2.D1");
    SysFreeString( str );

    /* broken query BSTR */
    memcpy(&buff[2], L"*", 2 * sizeof(WCHAR));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    hr = IXMLDOMElement_getElementsByTagName(elem, &buff[2], &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_get_length( node_list, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 5, "len %ld\n", len );
    IXMLDOMNodeList_Release( node_list );

    IXMLDOMElement_Release(elem);

    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_get_text(void)
{
    BSTR str;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2, *node3;
    IXMLDOMNode *nodeRoot;
    IXMLDOMNodeList *node_list;
    IXMLDOMNamedNodeMap *node_map;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    str = SysAllocString( L"bs" );
    hr = IXMLDOMDocument_getElementsByTagName( doc, str, &node_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    /* Test to get all child node text. */
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void**)&nodeRoot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        hr = IXMLDOMNode_get_text( nodeRoot, &str );
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        expect_bstr_eq_and_free(str, "fn1.txt\n \nfn2.txt\n \nf1");

        IXMLDOMNode_Release(nodeRoot);
    }

    hr = IXMLDOMNodeList_get_length( node_list, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_length( node_list, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 1, "expect 1 got %ld\n", len );

    hr = IXMLDOMNodeList_get_item( node_list, 0, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_nextNode( node_list, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item( node_list, 0, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNodeList_Release( node_list );

    /* Invalid output parameter*/
    hr = IXMLDOMNode_get_text( node, NULL );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_text( node, &str );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( !lstrcmpW(str, L"fn1.txt"), "wrong string\n" );
    SysFreeString(str);

    hr = IXMLDOMNode_get_attributes( node, &node_map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = SysAllocString( L"vr" );
    hr = IXMLDOMNamedNodeMap_getNamedItem( node_map, str, &node2 );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    hr = IXMLDOMNode_get_text( node2, &str );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( !lstrcmpW(str, L"str2"), "Unexpected string.\n" );
    SysFreeString(str);

    hr = IXMLDOMNode_get_firstChild( node2, &node3 );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_text( node3, &str );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( !lstrcmpW(str, L"str2"), "Unexpected string.\n" );
    SysFreeString(str);


    IXMLDOMNode_Release( node3 );
    IXMLDOMNode_Release( node2 );
    IXMLDOMNamedNodeMap_Release( node_map );
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_get_childNodes(void)
{
    IXMLDOMNodeList *node_list, *node_list2;
    IEnumVARIANT *enum1, *enum2, *enum3;
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2;
    IXMLDOMElement *element;
    IUnknown *unk1, *unk2;
    ULONG fetched;
    VARIANT v[2];
    HRESULT hr;
    BSTR str;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes( element, &node_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_length( node_list, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 4, "len %ld\n", len);

    /* refcount tests for IEnumVARIANT support */
    EXPECT_REF(node_list, 1);
    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IEnumVARIANT, (void**)&enum1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 1);
    EXPECT_REF(enum1, 2);

    EXPECT_REF(node_list, 1);
    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IEnumVARIANT, (void**)&enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 1);
    ok(enum2 == enum1, "got %p, %p\n", enum2, enum1);
    IEnumVARIANT_Release(enum2);

    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IUnknown, (void**)&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 3);
    EXPECT_REF(enum1, 2);
    ok(unk1 == unk2, "got %p, %p\n", unk1, unk2);
    IUnknown_Release(unk1);
    IUnknown_Release(unk2);

    EXPECT_REF(node_list, 1);
    hr = IXMLDOMNodeList__newEnum(node_list, (IUnknown**)&enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 2);
    EXPECT_REF(enum2, 1);
    ok(enum2 != enum1, "got %p, %p\n", enum2, enum1);

    /* enumerator created with _newEnum() doesn't share IUnknown* with main object */
    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IUnknown, (void**)&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumVARIANT_QueryInterface(enum2, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node_list, 3);
    EXPECT_REF(enum2, 2);
    ok(unk1 != unk2, "got %p, %p\n", unk1, unk2);
    IUnknown_Release(unk1);
    IUnknown_Release(unk2);

    hr = IXMLDOMNodeList__newEnum(node_list, (IUnknown**)&enum3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum2 != enum3, "got %p, %p\n", enum2, enum3);
    IEnumVARIANT_Release(enum3);
    IEnumVARIANT_Release(enum2);

    /* iteration tests */
    hr = IXMLDOMNodeList_get_item(node_list, 0, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"bs"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMNodeList_nextNode(node_list, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"bs"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);

    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum1, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"bs"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    VariantClear(&v[0]);

    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum1, 2, v, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[1]));
    ok(fetched == 2, "got %ld, expected 2\n", fetched);
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"pr"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[1]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"empty"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    VariantClear(&v[1]);
    VariantClear(&v[0]);

    V_VT(&v[1]) = VT_NULL;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum1, 2, v, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_EMPTY, "got var type %d\n", V_VT(&v[1]));
    ok(fetched == 1, "got %ld, expected 1\n", fetched);
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"fo"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    VariantClear(&v[0]);

    hr = IXMLDOMNodeList_nextNode(node_list, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"pr"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);

    IEnumVARIANT_Release(enum1);

    hr = IXMLDOMNodeList_get_item( node_list, 2, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_childNodes( node, &node_list2 );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_length( node_list2, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 0, "len %ld\n", len);

    hr = IXMLDOMNodeList_get_item( node_list2, 0, &node2);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    IXMLDOMNodeList_Release( node_list2 );
    IXMLDOMNode_Release( node );
    IXMLDOMNodeList_Release( node_list );
    IXMLDOMElement_Release( element );

    /* test for children of <?xml ..?> node */
    hr = IXMLDOMDocument_get_firstChild(doc, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"xml"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* it returns empty but valid node list */
    node_list = (void*)0xdeadbeef;
    hr = IXMLDOMNode_get_childNodes(node, &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMNodeList_get_length(node_list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %ld\n", len);

    IXMLDOMNodeList_Release( node_list );
    IXMLDOMNode_Release(node);

    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

static void test_get_firstChild(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_firstChild( doc, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_nodeName( node, &str );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(!lstrcmpW(str, L"xml"), "expected \"xml\" node name, got %s\n", wine_dbgstr_w(str));

    SysFreeString(str);
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_get_lastChild(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *child;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_lastChild( doc, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_nodeName( node, &str );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(!lstrcmpW(str, L"lc"), "Unexpected node name.\n");
    SysFreeString(str);

    hr = IXMLDOMNode_get_lastChild( node, &child );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_nodeName( child, &str );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(!lstrcmpW(str, L"fo"), "Unexpected node name.\n");
    SysFreeString(str);

    IXMLDOMNode_Release( child );
    IXMLDOMNode_Release( node );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_removeChild(void)
{
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element, *lc_element;
    IXMLDOMNode *fo_node, *ba_node, *removed_node, *temp_node, *lc_node;
    IXMLDOMNodeList *root_list, *fo_list;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(element, 2);

    hr = IXMLDOMElement_get_childNodes( element, &root_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(root_list, 1);

    hr = IXMLDOMNodeList_get_item( root_list, 3, &fo_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(fo_node, 1);

    hr = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(fo_list, 1);

    hr = IXMLDOMNodeList_get_item( fo_list, 0, &ba_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(ba_node, 1);

    /* invalid parameter: NULL ptr */
    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_removeChild( element, NULL, &removed_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* ba_node is a descendant of element, but not a direct child. */
    removed_node = (void*)0xdeadbeef;
    EXPECT_REF(ba_node, 1);
    EXPECT_CHILDREN(fo_node);
    hr = IXMLDOMElement_removeChild( element, ba_node, &removed_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( removed_node == NULL, "%p\n", removed_node );
    EXPECT_REF(ba_node, 1);
    EXPECT_CHILDREN(fo_node);

    EXPECT_REF(ba_node, 1);
    EXPECT_REF(fo_node, 1);
    hr = IXMLDOMElement_removeChild( element, fo_node, &removed_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( fo_node == removed_node, "node %p node2 %p\n", fo_node, removed_node );
    EXPECT_REF(fo_node, 2);
    EXPECT_REF(ba_node, 1);

    /* try removing already removed child */
    temp_node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_removeChild( element, fo_node, &temp_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( temp_node == NULL, "%p\n", temp_node );
    IXMLDOMNode_Release( fo_node );

    /* the removed node has no parent anymore */
    hr = IXMLDOMNode_get_parentNode( removed_node, &temp_node );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( temp_node == NULL, "%p\n", temp_node );

    IXMLDOMNode_Release( removed_node );
    IXMLDOMNode_Release( ba_node );
    IXMLDOMNodeList_Release( fo_list );

    hr = IXMLDOMNodeList_get_item( root_list, 0, &lc_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface( lc_node, &IID_IXMLDOMElement, (void**)&lc_element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* MS quirk: passing wrong interface pointer works, too */
    hr = IXMLDOMElement_removeChild( element, (IXMLDOMNode*)lc_element, NULL );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release( lc_element );

    temp_node = (void*)0xdeadbeef;
    hr = IXMLDOMNode_get_parentNode( lc_node, &temp_node );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( temp_node == NULL, "%p\n", temp_node );

    IXMLDOMNode_Release( lc_node );
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_replaceChild(void)
{
    VARIANT_BOOL b;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element, *ba_element;
    IXMLDOMNode *fo_node, *ba_node, *lc_node, *removed_node, *temp_node;
    IXMLDOMNodeList *root_list, *fo_list;
    IUnknown * unk1, *unk2;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes( element, &root_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item( root_list, 0, &lc_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item( root_list, 3, &fo_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item( fo_list, 0, &ba_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMNodeList_Release( fo_list );

    /* invalid parameter: NULL ptr for element to remove */
    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_replaceChild( element, ba_node, NULL, &removed_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* invalid parameter: NULL for replacement element. (Sic!) */
    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_replaceChild( element, NULL, fo_node, &removed_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( removed_node == (void*)0xdeadbeef, "%p\n", removed_node );

    /* invalid parameter: OldNode is not a child */
    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_replaceChild( element, lc_node, ba_node, &removed_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( removed_node == NULL, "%p\n", removed_node );
    IXMLDOMNode_Release( lc_node );

    /* invalid parameter: would create loop */
    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMNode_replaceChild( fo_node, fo_node, ba_node, &removed_node );
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( removed_node == NULL, "%p\n", removed_node );

    hr = IXMLDOMElement_replaceChild( element, ba_node, fo_node, NULL );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item( root_list, 3, &temp_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* ba_node and temp_node refer to the same node, yet they
       are different interface pointers */
    ok( ba_node != temp_node, "ba_node %p temp_node %p\n", ba_node, temp_node);
    hr = IXMLDOMNode_QueryInterface( temp_node, &IID_IUnknown, (void**)&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_QueryInterface( ba_node, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok( unk1 == unk2, "unk1 %p unk2 %p\n", unk1, unk2);

    IUnknown_Release( unk1 );
    IUnknown_Release( unk2 );

    /* ba_node should have been removed from below fo_node */
    hr = IXMLDOMNode_get_childNodes( fo_node, &fo_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* MS quirk: replaceChild also accepts elements instead of nodes */
    hr = IXMLDOMNode_QueryInterface( ba_node, &IID_IXMLDOMElement, (void**)&ba_element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(ba_element, 2);

    removed_node = NULL;
    hr = IXMLDOMElement_replaceChild( element, ba_node, (IXMLDOMNode*)ba_element, &removed_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( removed_node != NULL, "got %p\n", removed_node);
    EXPECT_REF(ba_element, 3);
    IXMLDOMElement_Release( ba_element );

    hr = IXMLDOMNodeList_get_length( fo_list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "len %ld\n", len);

    IXMLDOMNodeList_Release( fo_list );

    IXMLDOMNode_Release(ba_node);
    IXMLDOMNode_Release(fo_node);
    IXMLDOMNode_Release(temp_node);
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );

    free_bstrs();
}

static void test_removeNamedItem(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *node, *removed_node, *removed_node2;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap *map;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement( doc, &element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes( element, &root_list );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item( root_list, 1, &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_attributes( node, &map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNamedNodeMap_get_length( map, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 3, "length %ld\n", len);

    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, NULL, &removed_node );
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(removed_node == (void*)0xdeadbeef, "got %p\n", removed_node);

    removed_node = (void*)0xdeadbeef;
    str = SysAllocString(L"vr");
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, str, &removed_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    removed_node2 = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, str, &removed_node2 );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(removed_node2 == NULL, "got %p\n", removed_node2 );

    hr = IXMLDOMNamedNodeMap_get_length( map, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 2, "length %ld\n", len);

    hr = IXMLDOMNamedNodeMap_setNamedItem( map, removed_node, NULL );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(removed_node);

    hr = IXMLDOMNamedNodeMap_get_length( map, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 3, "length %ld\n", len);

    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, str, NULL );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNamedNodeMap_get_length( map, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 2, "length %ld\n", len);

    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, str, NULL );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    SysFreeString(str);

    IXMLDOMNamedNodeMap_Release( map );
    IXMLDOMNode_Release( node );
    IXMLDOMNodeList_Release( root_list );
    IXMLDOMElement_Release( element );

    /* test with namespaces */
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(default_ns_doc), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectSingleNode( doc, _bstr_("a"), &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface( node, &IID_IXMLDOMElement, (void**)&element );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release( node );

    hr = IXMLDOMElement_get_attributes( element, &map );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    removed_node = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, _bstr_("foo:bar"), &removed_node );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok ( removed_node == NULL, "got %p\n", removed_node );

    removed_node = NULL;
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, _bstr_("d"), &removed_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release( removed_node );

    removed_node = NULL;
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, _bstr_("ns:b"), &removed_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release( removed_node );

    removed_node = NULL;
    hr = IXMLDOMNamedNodeMap_removeNamedItem( map, _bstr_("xml:lang"), &removed_node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release( removed_node );

    len = -1;
    hr = IXMLDOMNamedNodeMap_get_length( map, &len );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 2, "length %ld\n", len );

    IXMLDOMNamedNodeMap_Release( map );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

#define test_IObjectSafety_set(p, r, r2, s, m, e, e2) _test_IObjectSafety_set(__LINE__,p, r, r2, s, m, e, e2)
static void _test_IObjectSafety_set(unsigned line, IObjectSafety *safety, HRESULT result,
                                    HRESULT result2, DWORD set, DWORD mask, DWORD expected,
                                    DWORD expected2)
{
    DWORD enabled, supported;
    HRESULT hr;

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL, set, mask);
    if (result == result2)
        ok_(__FILE__,line)(hr == result, "SetInterfaceSafetyOptions: expected %#lx, returned %#lx\n", result, hr );
    else
        ok_(__FILE__,line)(broken(hr == result) || hr == result2,
           "SetInterfaceSafetyOptions: expected %#lx, got %#lx\n", result2, hr );

    supported = enabled = 0xCAFECAFE;
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    if (expected == expected2)
        ok_(__FILE__,line)(enabled == expected, "Expected %#lx, got %#lx\n", expected, enabled);
    else
        ok_(__FILE__,line)(broken(enabled == expected) || enabled == expected2,
           "Expected %#lx, got %#lx\n", expected2, enabled);

    /* reset the safety options */

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
            INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER,
            0);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok_(__FILE__,line)(enabled == 0, "Expected 0, got %#lx\n", enabled);
}

#define test_IObjectSafety_common(s) _test_IObjectSafety_common(__LINE__,s)
static void _test_IObjectSafety_common(unsigned line, IObjectSafety *safety)
{
    DWORD enabled = 0, supported = 0;
    HRESULT hr;

    /* get */
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, NULL, &enabled);
    ok_(__FILE__,line)(hr == E_POINTER, "Unexpected hr %#lx.\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, NULL);
    ok_(__FILE__,line)(hr == E_POINTER, "Unexpected hr %#lx.\n", hr );

    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok_(__FILE__,line)(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %#lx\n", supported);
    ok_(__FILE__,line)(enabled == 0, "Expected 0, got %#lx\n", enabled);

    /* set -- individual flags */

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER,
        0, INTERFACE_USES_SECURITY_MANAGER /* msxml3 SP8+ */);

    /* set INTERFACE_USES_DISPEX  */

    test_IObjectSafety_set(safety, S_OK, E_FAIL /* msxml3 SP8+ */,
        INTERFACE_USES_DISPEX, INTERFACE_USES_DISPEX,
        0, 0);

    test_IObjectSafety_set(safety, S_OK, E_FAIL /* msxml3 SP8+ */,
        INTERFACE_USES_DISPEX, 0,
        0, 0);

    test_IObjectSafety_set(safety, S_OK, S_OK /* msxml3 SP8+ */,
        0, INTERFACE_USES_DISPEX,
        0, 0);

    /* set option masking */

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACESAFE_FOR_UNTRUSTED_DATA);

    test_IObjectSafety_set(safety, S_OK, S_OK,
        INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA,
        INTERFACE_USES_SECURITY_MANAGER,
        0,
        0);

    /* set -- inheriting previous settings */

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
                                                         INTERFACESAFE_FOR_UNTRUSTED_CALLER,
                                                         INTERFACESAFE_FOR_UNTRUSTED_CALLER);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok_(__FILE__,line)(enabled == INTERFACESAFE_FOR_UNTRUSTED_CALLER, "Expected INTERFACESAFE_FOR_UNTRUSTED_CALLER got %#lx\n", enabled);
    ok_(__FILE__,line)(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %#lx.\n", supported);

    hr = IObjectSafety_SetInterfaceSafetyOptions(safety, NULL,
                                                         INTERFACESAFE_FOR_UNTRUSTED_DATA,
                                                         INTERFACESAFE_FOR_UNTRUSTED_DATA);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    hr = IObjectSafety_GetInterfaceSafetyOptions(safety, NULL, &supported, &enabled);
    ok_(__FILE__,line)(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok_(__FILE__,line)(broken(enabled == INTERFACESAFE_FOR_UNTRUSTED_DATA) ||
                       enabled == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA),
                       "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA) got %#lx\n", enabled);
    ok_(__FILE__,line)(broken(supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) ||
       supported == (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER) /* msxml3 SP8+ */,
        "Expected (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER), "
             "got %#lx\n", supported);
}

static void test_IXMLDOMDocument2(void)
{
    IXMLDOMDocument2 *doc2, *dtddoc2;
    IXMLDOMDocument *doc;
    IXMLDOMParseError* err;
    IDispatchEx *dispex;
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;
    LONG res;

    if (!is_clsid_supported(&CLSID_DOMDocument2, &IID_IXMLDOMDocument2)) return;

    doc = create_document(&IID_IXMLDOMDocument);
    dtddoc2 = create_document(&IID_IXMLDOMDocument2);

    hr = IXMLDOMDocument_QueryInterface( doc, &IID_IXMLDOMDocument2, (void**)&doc2 );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( doc == (IXMLDOMDocument*)doc2, "interfaces differ\n");

    hr = IXMLDOMDocument2_get_readyState(doc2, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_get_readyState(doc2, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(res == READYSTATE_COMPLETE, "expected READYSTATE_COMPLETE (4), got %ld\n", res);

    err = NULL;
    hr = IXMLDOMDocument2_validate(doc2, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_validate(doc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected a pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_E_NOTWF */
        ok(res == E_XML_NOTWF, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML( doc2, _bstr_(complete4A), &b );
    ok(hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_readyState(doc, &res);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(res == READYSTATE_COMPLETE, "expected READYSTATE_COMPLETE (4), got %ld\n", res);

    err = NULL;
    hr = IXMLDOMDocument2_validate(doc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected a pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_E_NODTD */
        ok(res == E_XML_NODTD, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument_QueryInterface( doc, &IID_IDispatchEx, (void**)&dispex );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if(hr == S_OK)
        IDispatchEx_Release(dispex);

    /* we will check if the variant got cleared */
    IXMLDOMDocument2_AddRef(doc2);
    EXPECT_REF(doc2, 3); /* doc, doc2, AddRef*/

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown *)doc2;

    /* invalid calls */
    hr = IXMLDOMDocument2_getProperty(doc2, _bstr_("askldhfaklsdf"), &var);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    expect_eq(V_VT(&var), VT_UNKNOWN, int, "%x");
    hr = IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* valid call */
    hr = IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");
    V_VT(&var) = VT_R4;

    /* the variant didn't get cleared*/
    expect_eq(IXMLDOMDocument2_Release(doc2), 2, int, "%d");

    /* setProperty tests */
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("askldhfaklsdf"), var);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), var);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("alskjdh faklsjd hfk"));
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XPath"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"");
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionNamespaces"), var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 0;
    hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionNamespaces"), var);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* contrary to what MSDN claims you can switch back from XPath to XSLPattern */
    hr = IXMLDOMDocument2_getProperty(doc2, _bstr_("SelectionLanguage"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    expect_bstr_eq_and_free(V_BSTR(&var), "XSLPattern");

    IXMLDOMDocument2_Release( doc2 );
    IXMLDOMDocument_Release( doc );

    /* DTD validation */
    hr = IXMLDOMDocument2_put_validateOnParse(dtddoc2, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(res == 0, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_0D), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ELEMENT_UNDECLARED */
        todo_wine ok(res == 0xC00CE00D, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_0E), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ELEMENT_ID_NOT_FOUND */
        todo_wine ok(res == 0xC00CE00E, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_11), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_EMPTY_NOT_ALLOWED */
        todo_wine ok(res == 0xC00CE011, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_13), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ROOT_NAME_MISMATCH */
        todo_wine ok(res == 0xC00CE013, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_14), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_INVALID_CONTENT */
        todo_wine ok(res == 0xC00CE014, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_15), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ATTRIBUTE_NOT_DEFINED */
        todo_wine ok(res == 0xC00CE015, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_16), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ATTRIBUTE_FIXED */
        todo_wine ok(res == 0xC00CE016, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_17), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ATTRIBUTE_VALUE */
        todo_wine ok(res == 0xC00CE017, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_18), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_ILLEGAL_TEXT */
        todo_wine ok(res == 0xC00CE018, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    hr = IXMLDOMDocument2_loadXML(dtddoc2, _bstr_(szEmailXML_20), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");
    err = NULL;
    hr = IXMLDOMDocument2_validate(dtddoc2, &err);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(err != NULL, "expected pointer\n");
    if (err)
    {
        res = 0;
        hr = IXMLDOMParseError_get_errorCode(err, &res);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        /* XML_REQUIRED_ATTRIBUTE_MISSING */
        todo_wine ok(res == 0xC00CE020, "Unexpected code %#lx.\n", res);
        IXMLDOMParseError_Release(err);
    }

    IXMLDOMDocument2_Release( dtddoc2 );
    free_bstrs();
}

#define helper_ole_check_ver(expr) { \
    HRESULT hr = expr; \
    ok_(__FILE__, line)(hr == S_OK, "-> %i (%s): " #expr " returned %#lx.\n", __LINE__, ver, hr); \
}

#define helper_expect_list_and_release(list, expstr) { \
    char *str = list_to_string(list); \
    ok_(__FILE__, line)(strcmp(str, expstr)==0, "=> %i (%s): Invalid node list: %s, expected %s\n", __LINE__, ver, str, expstr); \
    if (list) IXMLDOMNodeList_Release(list); \
}

#define helper_expect_bstr_and_release(bstr, str) { \
    ok_(__FILE__, line)(lstrcmpW(bstr, _bstr_(str)) == 0, \
       "=> %i (%s): got %s\n", __LINE__, ver, wine_dbgstr_w(bstr)); \
    SysFreeString(bstr); \
}

#define check_ws_ignored(ver, doc, str) _check_ws_ignored(__LINE__, ver, doc, str)
static inline void _check_ws_ignored(int line, const char *ver, IXMLDOMDocument2* doc, char const* str)
{
    IXMLDOMNode *node1, *node2;
    IXMLDOMNodeList *list;
    BSTR bstr;

    helper_ole_check_ver(IXMLDOMDocument2_selectNodes(doc, _bstr_("//*[local-name()='html']"), &list));
    helper_ole_check_ver(IXMLDOMNodeList_get_item(list, 0, &node1));
    helper_ole_check_ver(IXMLDOMNodeList_get_item(list, 1, &node2));
    helper_ole_check_ver(IXMLDOMNodeList_reset(list));
    helper_expect_list_and_release(list, "E1.E5.E1.E2.D1 E2.E5.E1.E2.D1");

    helper_ole_check_ver(IXMLDOMNode_get_childNodes(node1, &list));
    helper_expect_list_and_release(list,
            "[4]1.E1.E5.E1.E2.D1 T2.E1.E5.E1.E2.D1 E3.E1.E5.E1.E2.D1 "
            "E4.E1.E5.E1.E2.D1 E5.E1.E5.E1.E2.D1 E6.E1.E5.E1.E2.D1");
    helper_ole_check_ver(IXMLDOMNode_get_text(node1, &bstr));
    if (str)
    {
        helper_expect_bstr_and_release(bstr, str);
    }
    else
    {
        helper_expect_bstr_and_release(bstr, " This is a description.");
    }
    IXMLDOMNode_Release(node1);

    helper_ole_check_ver(IXMLDOMNode_get_childNodes(node2, &list));
    helper_expect_list_and_release(list,
            "T1.E2.E5.E1.E2.D1 E2.E2.E5.E1.E2.D1 T3.E2.E5.E1.E2.D1 "
            "E4.E2.E5.E1.E2.D1 T5.E2.E5.E1.E2.D1 E6.E2.E5.E1.E2.D1 T7.E2.E5.E1.E2.D1");
    helper_ole_check_ver(IXMLDOMNode_get_text(node2, &bstr));
    helper_expect_bstr_and_release(bstr,
            "\n                This is a description with preserved whitespace. \n            ");
    IXMLDOMNode_Release(node2);
}

#define check_ws_preserved(ver, doc, str) _check_ws_preserved(__LINE__, ver, doc, str)
static inline void _check_ws_preserved(int line, const char *ver, IXMLDOMDocument2* doc, char const* str)
{
    IXMLDOMNode *node1, *node2;
    IXMLDOMNodeList *list;
    BSTR bstr;

    helper_ole_check_ver(IXMLDOMDocument2_selectNodes(doc, _bstr_("//*[local-name()='html']"), &list));
    helper_ole_check_ver(IXMLDOMNodeList_get_item(list, 0, &node1));
    helper_ole_check_ver(IXMLDOMNodeList_get_item(list, 1, &node2));
    helper_ole_check_ver(IXMLDOMNodeList_reset(list));
    helper_expect_list_and_release(list, "E2.E10.E2.E2.D1 E4.E10.E2.E2.D1");

    helper_ole_check_ver(IXMLDOMNode_get_childNodes(node1, &list));
    helper_expect_list_and_release(list,
            "T1.E2.E10.E2.E2.D1 [4]2.E2.E10.E2.E2.D1 T3.E2.E10.E2.E2.D1 "
            "E4.E2.E10.E2.E2.D1 T5.E2.E10.E2.E2.D1 E6.E2.E10.E2.E2.D1 "
            "E7.E2.E10.E2.E2.D1 E8.E2.E10.E2.E2.D1 T9.E2.E10.E2.E2.D1");
    helper_ole_check_ver(IXMLDOMNode_get_text(node1, &bstr));
    if (str)
    {
        helper_expect_bstr_and_release(bstr, str);
    }
    else
    {
        helper_expect_bstr_and_release(bstr, "\n                 This is a  description. \n            ");
    }
    IXMLDOMNode_Release(node1);

    helper_ole_check_ver(IXMLDOMNode_get_childNodes(node2, &list));
    helper_expect_list_and_release(list,
            "T1.E4.E10.E2.E2.D1 E2.E4.E10.E2.E2.D1 T3.E4.E10.E2.E2.D1 "
            "E4.E4.E10.E2.E2.D1 T5.E4.E10.E2.E2.D1 E6.E4.E10.E2.E2.D1 T7.E4.E10.E2.E2.D1");
    helper_ole_check_ver(IXMLDOMNode_get_text(node2, &bstr));
    helper_expect_bstr_and_release(bstr,
            "\n                This is a description with preserved whitespace. \n            ");
    IXMLDOMNode_Release(node2);
}

static void test_preserve_charref(IXMLDOMDocument2 *doc, VARIANT_BOOL preserve)
{
    IXMLDOMNodeList *list;
    IXMLDOMElement *root;
    IXMLDOMNode *node;
    const WCHAR *text;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR s;

    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, preserve);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(charrefsxml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_get_documentElement(doc, &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes(root, &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(root);

    text = preserve == VARIANT_TRUE ? L" Text A end " : L"Text A end";
    hr = IXMLDOMNodeList_get_item(list, 0, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_text(node, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, text), "0x%x, got %s\n", preserve, wine_dbgstr_w(s));
    SysFreeString(s);
    IXMLDOMNode_Release(node);

    text = preserve == VARIANT_TRUE ? L"AB C " : L"AB C";
    hr = IXMLDOMNodeList_get_item(list, 1, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_text(node, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, text), "0x%x, got %s\n", preserve, wine_dbgstr_w(s));
    SysFreeString(s);
    IXMLDOMNode_Release(node);

    IXMLDOMNodeList_Release(list);
}

struct whitespace_t {
    const CLSID *clsid;
    const char *name;
};

static const struct whitespace_t whitespace_test_data[] = {
    { &CLSID_DOMDocument,   "CLSID_DOMDocument"   },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2"  },
    { &CLSID_DOMDocument26, "CLSID_DOMDocument26" },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30" },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40" },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60" },
    { 0 }
};

static void test_whitespace(void)
{
    const struct whitespace_t *class_ptr = whitespace_test_data;

    while (class_ptr->clsid)
    {
        IXMLDOMDocument2 *doc1, *doc2, *doc3, *doc4;
        IXMLDOMNodeList *list;
        IXMLDOMElement *root;
        VARIANT_BOOL b;
        HRESULT hr;
        LONG len;

        if (!is_clsid_supported(class_ptr->clsid, &IID_IXMLDOMDocument2))
        {
            class_ptr++;
            continue;
        }

        hr = CoCreateInstance(class_ptr->clsid, NULL, CLSCTX_INPROC_SERVER,
                &IID_IXMLDOMDocument2, (void**)&doc1);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(class_ptr->clsid, NULL, CLSCTX_INPROC_SERVER,
                &IID_IXMLDOMDocument2, (void**)&doc2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc2, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc1, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "expected false\n");
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc2, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "expected true\n");

        hr = IXMLDOMDocument2_loadXML(doc1, _bstr_(szExampleXML), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "failed to load XML string\n");
        hr = IXMLDOMDocument2_loadXML(doc2, _bstr_(szExampleXML), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "failed to load XML string\n");

        /* switch to XPath */
        hr = IXMLDOMDocument2_setProperty(doc1, _bstr_("SelectionLanguage"), _variantbstr_("XPath"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMDocument2_setProperty(doc2, _bstr_("SelectionLanguage"), _variantbstr_("XPath"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        check_ws_ignored(class_ptr->name, doc1, NULL);
        check_ws_preserved(class_ptr->name, doc2, NULL);

        /* new instances copy the property */
        hr = IXMLDOMDocument2_QueryInterface(doc1, &IID_IXMLDOMDocument2, (void**) &doc3);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMDocument2_QueryInterface(doc2, &IID_IXMLDOMDocument2, (void**) &doc4);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc3, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "expected false\n");
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc4, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "expected true\n");

        check_ws_ignored(class_ptr->name, doc3, NULL);
        check_ws_preserved(class_ptr->name, doc4, NULL);

        /* setting after loading xml affects trimming of leading/trailing ws only */
        hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc1, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc2, VARIANT_FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* the trailing "\n            " isn't there, because it was ws-only node */
        check_ws_ignored(class_ptr->name, doc1, " This is a description. ");
        check_ws_preserved(class_ptr->name, doc2, " This is a description.");

        /* it takes effect on reload */
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc1, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "expected true\n");
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc2, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "expected false\n");

        hr = IXMLDOMDocument2_loadXML(doc1, _bstr_(szExampleXML), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "failed to load XML string\n");
        hr = IXMLDOMDocument2_loadXML(doc2, _bstr_(szExampleXML), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "failed to load XML string\n");

        check_ws_preserved(class_ptr->name, doc1, NULL);
        check_ws_ignored(class_ptr->name, doc2, NULL);

        /* other instances follow suit */
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc3, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "expected true\n");
        hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc4, &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_FALSE, "expected false\n");

        check_ws_preserved(class_ptr->name, doc3, NULL);
        check_ws_ignored(class_ptr->name, doc4, NULL);

        IXMLDOMDocument2_Release(doc2);
        IXMLDOMDocument2_Release(doc3);
        IXMLDOMDocument2_Release(doc4);

        /* text with char references */
        test_preserve_charref(doc1, VARIANT_TRUE);
        test_preserve_charref(doc1, VARIANT_FALSE);

        /* formatting whitespaces */
        hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc1, VARIANT_FALSE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument2_loadXML(doc1, _bstr_(complete7), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "for %x\n", b);

        hr = IXMLDOMDocument2_get_documentElement(doc1, &root);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IXMLDOMElement_get_childNodes(root, &list);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        len = 0;
        hr = IXMLDOMNodeList_get_length(list, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len == 3, "got %ld\n", len);
        IXMLDOMNodeList_Release(list);
        IXMLDOMElement_Release(root);

        IXMLDOMDocument2_Release(doc1);

        free_bstrs();

        class_ptr++;
    }
}

typedef struct {
    const GUID *clsid;
    const char *name;
    const char *ns;
    HRESULT hr;
} selection_ns_t;

/* supposed to be tested with szExampleXML */
static const selection_ns_t selection_ns_data[] = {
    { &CLSID_DOMDocument,   "CLSID_DOMDocument",   "\txmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument,   "CLSID_DOMDocument",   "\n\rxmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument,   "CLSID_DOMDocument",   " xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument,   "CLSID_DOMDocument",   "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' ", S_OK },

    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2",  "\txmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2",  "\n\rxmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2",  " xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2",  "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' ", S_OK },

    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", "\txmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", "\n\rxmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", " xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' ", S_OK },

    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", "\txmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", "\n\rxmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", " xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' ", S_OK },

    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", "\txmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", "\n\rxmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", " xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'", S_OK },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' ", S_OK },

    { NULL }
};

typedef struct {
    const char *query;
    const char *list;
} xpath_test_t;

static const xpath_test_t xpath_test[] = {
    { "*/a", "E1.E1.E2.D1 E1.E2.E2.D1 E1.E4.E2.D1" },
    { "*/b", "E2.E1.E2.D1 E2.E2.E2.D1 E2.E4.E2.D1" },
    { "*/c", "E3.E1.E2.D1 E3.E2.E2.D1" },
    { "*/d", "E4.E1.E2.D1 E4.E2.E2.D1 E4.E4.E2.D1" },
    { "//a", "E1.E1.E2.D1 E1.E2.E2.D1 E1.E4.E2.D1" },
    { "//b", "E2.E1.E2.D1 E2.E2.E2.D1 E2.E4.E2.D1" },
    { "//c", "E3.E1.E2.D1 E3.E2.E2.D1" },
    { "//d", "E4.E1.E2.D1 E4.E2.E2.D1 E4.E4.E2.D1" },
    { "//c[@type]", "E3.E2.E2.D1" },
    { "//c[@type]/ancestor::node()[1]", "E2.E2.D1" },
    { "//c[@type]/ancestor-or-self::node()[1]", "E3.E2.E2.D1" },
    { "//c[@type]/attribute::node()[1]", "A'type'.E3.E2.E2.D1" },
    { "//c[@type]/child::node()[1]", "T1.E3.E2.E2.D1"  },
    { "//c[@type]/descendant::node()[1]", "T1.E3.E2.E2.D1" },
    { "//c[@type]/descendant-or-self::node()[1]", "E3.E2.E2.D1" },
    { "//c[@type]/following::node()[1]", "E4.E2.E2.D1" },
    { "//c[@type]/following-sibling::node()[1]", "E4.E2.E2.D1" },
    { "//c[@type]/parent::node()[1]", "E2.E2.D1" },
    { "//c[@type]/preceding::node()[1]", "T1.E2.E2.E2.D1" },
    { "//c[@type]/self::node()[1]", "E3.E2.E2.D1" },
    { "child::*", "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "child::node()", "E1.E2.D1 E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "child::text()", "" },
    { "child::*/..", "E2.D1" },
    { "child::*//@*/..", "E2.E5.E1.E2.D1 E2.E2.D1 E3.E2.E2.D1" },
    { "self::node()", "E2.D1" },
    { "ancestor::node()", "D1" },
    { "elem[c][last()]/a", "E1.E2.E2.D1"},
    { "ancestor-or-self::node()[1]", "E2.D1" },
    { "((//a)[1])[last()]", "E1.E1.E2.D1" },
    { "//elem[@*]", "E2.E2.D1" },
    { NULL }
};

static void test_XPath(void)
{
    const selection_ns_t *ptr = selection_ns_data;
    const xpath_test_t *xptest = xpath_test;
    VARIANT var;
    VARIANT_BOOL b;
    IXMLDOMDocument2 *doc;
    IXMLDOMDocument *doc2;
    IXMLDOMNode *rootNode;
    IXMLDOMNode *elem1Node;
    IXMLDOMNode *node;
    IXMLDOMNodeList *list;
    IXMLDOMElement *elem;
    IXMLDOMAttribute *attr;
    DOMNodeType type;
    HRESULT hr;
    LONG len;
    BSTR str;

    if (!is_clsid_supported(&CLSID_DOMDocument2, &IID_IXMLDOMDocument2)) return;
    doc = create_document(&IID_IXMLDOMDocument2);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XPath */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* some simple queries*/
    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(doc, 1);
    EXPECT_LIST_LEN(list, 1);

    EXPECT_REF(list, 1);
    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(list, 1);
    EXPECT_REF(rootNode, 1);

    hr = IXMLDOMNodeList_reset(list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E2.D1");

    /* perform xpath tests */
    for ( ; xptest->query ; xptest++ )
    {
        char *str;

        hr = IXMLDOMNode_selectNodes(rootNode, _bstr_(xptest->query), &list);
        ok(hr == S_OK, "query evaluation failed for query=%s\n", xptest->query);

        if (hr != S_OK)
            continue;

        str = list_to_string(list);

        ok(!strcmp(str, xptest->list), "query=%s, invalid node list: \"%s\", expected \"%s\"\n",
            xptest->query, str, xptest->list);

        if (list)
            IXMLDOMNodeList_Release(list);
    }

if (0)
{
    /* namespace:: axis test is disabled until namespace definitions
       are supported as attribute nodes, currently it's another node type */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("/root/namespace::*"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    len = -1;
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 2, "got %ld\n", len);

    hr = IXMLDOMNodeList_nextNode(list, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    type = NODE_INVALID;
    hr = IXMLDOMNode_get_nodeType(node, &type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(type == NODE_ATTRIBUTE, "got %d\n", type);
    IXMLDOMNode_Release(node);

    IXMLDOMNodeList_Release(list);
}

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E3.E1.E2.D1 E3.E2.E2.D1");

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//c[@type]"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E3.E2.E2.D1");

    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("elem"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* using get_item for query results advances the position */
    hr = IXMLDOMNodeList_get_item(list, 1, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_node(node, "E2.E2.D1");
    IXMLDOMNode_Release(node);
    hr = IXMLDOMNodeList_nextNode(list, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_node(node, "E4.E2.D1");
    IXMLDOMNode_Release(node);
    hr = IXMLDOMNodeList_reset(list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E4.E2.D1");

    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("."), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E2.D1");

    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("elem[3]/preceding-sibling::*"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_get_item(list, 0, &elem1Node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_reset(list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E1.E2.D1 E2.E2.D1 E3.E2.D1");

    /* select an attribute */
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_(".//@type"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "A'type'.E3.E2.E2.D1");

    /* would evaluate to a number */
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("count(*)"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    /* would evaluate to a boolean */
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("position()>0"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    /* would evaluate to a string */
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("name()"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* no results */
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_("c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "");
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("elem//c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "");
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//elem[4]"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "");
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//elem[0]"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "");

    /* foo undeclared in document node */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//foo:c"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    /* undeclared in <root> node */
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_(".//foo:c"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    /* undeclared in <elem> node */
    hr = IXMLDOMNode_selectNodes(elem1Node, _bstr_("//foo:c"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    /* but this trick can be used */
    hr = IXMLDOMNode_selectNodes(elem1Node, _bstr_("//*[name()='foo:c']"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E3.E4.E2.D1");

    /* it has to be declared in SelectionNamespaces */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* now the namespace can be used */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//test:c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    hr = IXMLDOMNode_selectNodes(rootNode, _bstr_(".//test:c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    hr = IXMLDOMNode_selectNodes(elem1Node, _bstr_("//test:c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");
    hr = IXMLDOMNode_selectNodes(elem1Node, _bstr_(".//test:x"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_list_and_release(list, "E6.E1.E5.E1.E2.D1 E6.E2.E5.E1.E2.D1");

    /* SelectionNamespaces syntax error - the namespaces doesn't work anymore but the value is stored */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###"));
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//foo:c"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_eq(V_VT(&var), VT_BSTR, int, "%x");
    if (V_VT(&var) == VT_BSTR)
        expect_bstr_eq_and_free(V_BSTR(&var), "xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' xmlns:foo=###");

    /* extra attributes - same thing*/
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
        _variantbstr_("xmlns:test='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29' param='test'"));
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//foo:c"), &list);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    IXMLDOMNode_Release(rootNode);
    IXMLDOMNode_Release(elem1Node);

    /* alter document with already built list */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMDocument2_get_lastChild(doc, &rootNode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(rootNode, 1);
    EXPECT_REF(doc, 1);

    hr = IXMLDOMDocument2_removeChild(doc, rootNode, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(rootNode);

    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(rootNode, 1);

    IXMLDOMNodeList_Release(list);

    hr = IXMLDOMNode_get_nodeName(rootNode, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"root"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(rootNode);

    /* alter node from list and get it another time */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(rootNode, &IID_IXMLDOMElement, (void**)&elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 1;
    hr = IXMLDOMElement_setAttribute(elem, _bstr_("attrtest"), var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(elem);
    IXMLDOMNode_Release(rootNode);

    /* now check attribute to be present */
    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(rootNode, &IID_IXMLDOMElement, (void**)&elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("attrtest"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMAttribute_Release(attr);

    IXMLDOMElement_Release(elem);
    IXMLDOMNode_Release(rootNode);

    /* and now check for attribute in original document */
    hr = IXMLDOMDocument2_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("attrtest"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMAttribute_Release(attr);

    IXMLDOMElement_Release(elem);

    /* attach node from list to another document */
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_LIST_LEN(list, 1);

    hr = IXMLDOMNodeList_get_item(list, 0, &rootNode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(rootNode, 1);

    hr = IXMLDOMDocument_appendChild(doc2, rootNode, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(rootNode, 1);
    EXPECT_REF(doc2, 1);
    EXPECT_REF(list, 1);

    EXPECT_LIST_LEN(list, 1);

    IXMLDOMNode_Release(rootNode);
    IXMLDOMNodeList_Release(list);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument2_Release(doc);

    while (ptr->clsid)
    {
        if (is_clsid_supported(ptr->clsid, &IID_IXMLDOMDocument2))
        {
            hr = CoCreateInstance(ptr->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void**)&doc);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }
        else
        {
            ptr++;
            continue;
        }

        hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(b == VARIANT_TRUE, "failed to load, %s\n", ptr->name);

        hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_(ptr->ns);

        hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), var);
        ok(hr == ptr->hr, "Unexpected hr %#lx., for %s, %s\n", hr, ptr->name, ptr->ns);

        V_VT(&var) = VT_EMPTY;
        hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&var) == VT_BSTR, "got wrong property type %d\n", V_VT(&var));
        ok(!lstrcmpW(V_BSTR(&var), _bstr_(ptr->ns)), "got wrong value %s\n", wine_dbgstr_w(V_BSTR(&var)));
        VariantClear(&var);

        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("root//test:c"), &list);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");

        IXMLDOMDocument2_Release(doc);
        ptr++;
        free_bstrs();
    }

    free_bstrs();
}

static void test_cloneNode(void )
{
    IXMLDOMDocument2 *doc, *doc_clone;
    IXMLDOMDocument *doc2;
    VARIANT_BOOL b;
    IXMLDOMNodeList *pList;
    IXMLDOMNamedNodeMap *mapAttr;
    LONG length, length1;
    LONG attr_cnt, attr_cnt1;
    IXMLDOMNode *node, *attr;
    IXMLDOMNode *node_clone;
    IXMLDOMNode *node_first;
    VARIANT v;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument2);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(complete4A), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionLanguage"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(V_BSTR(&v), L"XSLPattern"), "got prop value %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_BSTR(&v) = _bstr_("XPath");
    V_VT(&v) = VT_BSTR;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* clone document node */
    hr = IXMLDOMDocument2_cloneNode(doc, VARIANT_TRUE, &node);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok( node != NULL, "node %p\n", node );

    hr = IXMLDOMNode_get_childNodes(node, &pList);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    length = 0;
    hr = IXMLDOMNodeList_get_length(pList, &length);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(length == 2, "got %ld\n", length);
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMDocument2, (void**)&doc_clone);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* cloned document inherits properties */
    hr = IXMLDOMDocument2_getProperty(doc_clone, _bstr_("SelectionLanguage"), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(V_BSTR(&v), L"XPath"), "got prop value %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    IXMLDOMDocument2_Release(doc_clone);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument2_selectSingleNode(doc, _bstr_("lc/pr"), &node);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok( node != NULL, "node %p\n", node );

    /* Check invalid parameter */
    hr = IXMLDOMNode_cloneNode(node, VARIANT_TRUE, NULL);
    ok( hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    /* All Children */
    hr = IXMLDOMNode_cloneNode(node, VARIANT_TRUE, &node_clone);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok( node_clone != NULL, "node %p\n", node );

    hr = IXMLDOMNode_get_firstChild(node_clone, &node_first);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    hr = IXMLDOMNode_get_ownerDocument(node_clone, &doc2);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    IXMLDOMDocument_Release(doc2);
    IXMLDOMNode_Release(node_first);

    hr = IXMLDOMNode_get_childNodes(node, &pList);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    length = 0;
    hr = IXMLDOMNodeList_get_length(pList, &length);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(length == 1, "got %ld\n", length);
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_get_attributes(node, &mapAttr);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    attr_cnt = 0;
    hr = IXMLDOMNamedNodeMap_get_length(mapAttr, &attr_cnt);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(attr_cnt == 3, "got %ld\n", attr_cnt);
    IXMLDOMNamedNodeMap_Release(mapAttr);

    hr = IXMLDOMNode_get_childNodes(node_clone, &pList);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    length1 = 0;
    hr = IXMLDOMNodeList_get_length(pList, &length1);
    ok(length1 == 1, "got %ld\n", length1);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_get_attributes(node_clone, &mapAttr);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    attr_cnt1 = 0;
    hr = IXMLDOMNamedNodeMap_get_length(mapAttr, &attr_cnt1);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(attr_cnt1 == 3, "got %ld\n", attr_cnt1);
    /* now really get some attributes from cloned element */
    attr = NULL;
    hr = IXMLDOMNamedNodeMap_getNamedItem(mapAttr, _bstr_("id"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(attr);
    IXMLDOMNamedNodeMap_Release(mapAttr);

    ok(length == length1, "wrong Child count (%ld, %ld)\n", length, length1);
    ok(attr_cnt == attr_cnt1, "wrong Attribute count (%ld, %ld)\n", attr_cnt, attr_cnt1);
    IXMLDOMNode_Release(node_clone);

    /* No Children */
    hr = IXMLDOMNode_cloneNode(node, VARIANT_FALSE, &node_clone);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok( node_clone != NULL, "node %p\n", node );

    hr = IXMLDOMNode_get_firstChild(node_clone, &node_first);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMNode_get_childNodes(node_clone, &pList);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    hr = IXMLDOMNodeList_get_length(pList, &length1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok( length1 == 0, "Length should be 0 (%ld)\n", length1);
    IXMLDOMNodeList_Release(pList);

    hr = IXMLDOMNode_get_attributes(node_clone, &mapAttr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    hr = IXMLDOMNamedNodeMap_get_length(mapAttr, &attr_cnt1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(attr_cnt1 == 3, "Attribute count should be 3 (%ld)\n", attr_cnt1);
    IXMLDOMNamedNodeMap_Release(mapAttr);

    ok(length != length1, "wrong Child count (%ld, %ld)\n", length, length1);
    ok(attr_cnt == attr_cnt1, "wrong Attribute count (%ld, %ld)\n", attr_cnt, attr_cnt1);
    IXMLDOMNode_Release(node_clone);

    IXMLDOMNode_Release(node);
    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static void test_xmlTypes(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *pRoot;
    HRESULT hr;
    IXMLDOMComment *pComment;
    IXMLDOMElement *pElement;
    IXMLDOMAttribute *pAttribute;
    IXMLDOMNamedNodeMap *pAttribs;
    IXMLDOMCDATASection *pCDataSec;
    IXMLDOMImplementation *pIXMLDOMImplementation = NULL;
    IXMLDOMDocumentFragment *pDocFrag = NULL;
    IXMLDOMEntityReference *pEntityRef = NULL;
    BSTR str;
    IXMLDOMNode *pNextChild;
    VARIANT v;
    LONG len = 0;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_nextSibling(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    pNextChild = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_nextSibling(doc, &pNextChild);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
    ok(pNextChild == NULL, "pDocChild not NULL\n");

    /* test previous Sibling */
    hr = IXMLDOMDocument_get_previousSibling(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    pNextChild = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_previousSibling(doc, &pNextChild);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
    ok(pNextChild == NULL, "pNextChild not NULL\n");

    /* test get_dataType */
    V_VT(&v) = VT_EMPTY;
    hr = IXMLDOMDocument_get_dataType(doc, &v);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
    ok( V_VT(&v) == VT_NULL, "incorrect dataType type\n");
    VariantClear(&v);

    /* test implementation */
    hr = IXMLDOMDocument_get_implementation(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMDocument_get_implementation(doc, &pIXMLDOMImplementation);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    if(hr == S_OK)
    {
        VARIANT_BOOL hasFeature = VARIANT_TRUE;
        BSTR sEmpty = SysAllocStringLen(NULL, 0);

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, NULL, sEmpty, &hasFeature);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, sEmpty, sEmpty, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), sEmpty, &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, sEmpty, sEmpty, &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), NULL, &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), sEmpty, &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("DOM"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("XML"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("MS-DOM"), _bstr_("1.0"), &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_TRUE, "hasFeature returned true\n");

        hr = IXMLDOMImplementation_hasFeature(pIXMLDOMImplementation, _bstr_("SSS"), NULL, &hasFeature);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(hasFeature == VARIANT_FALSE, "hasFeature returned false\n");

        SysFreeString(sEmpty);
        IXMLDOMImplementation_Release(pIXMLDOMImplementation);
    }

    pRoot = (IXMLDOMElement*)0x1;
    hr = IXMLDOMDocument_createElement(doc, NULL, &pRoot);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
    ok(pRoot == (void*)0x1, "Expect same ptr, got %p\n", pRoot);

    pRoot = (IXMLDOMElement*)0x1;
    hr = IXMLDOMDocument_createElement(doc, _bstr_(""), &pRoot);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
    ok(pRoot == (void*)0x1, "Expect same ptr, got %p\n", pRoot);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &pRoot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    if(hr == S_OK)
    {
        hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)pRoot, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        if(hr == S_OK)
        {
            /* Comment */
            str = SysAllocString(L"A Comment");
            hr = IXMLDOMDocument_createComment(doc, str, &pComment);
            SysFreeString(str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pComment, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_nodeName(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"#comment" ), "incorrect comment node Name\n");
                SysFreeString(str);

                hr = IXMLDOMComment_get_xml(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"<!--A Comment-->" ), "incorrect comment xml\n");
                SysFreeString(str);

                /* put data Tests */
                hr = IXMLDOMComment_put_data(pComment, _bstr_("This &is a ; test <>\\"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                /* get data Tests */
                hr = IXMLDOMComment_get_data(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This &is a ; test <>\\"), "incorrect get_data string\n");
                SysFreeString(str);

                /* Confirm XML text is good */
                hr = IXMLDOMComment_get_xml(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"<!--This &is a ; test <>\\-->"), "incorrect xml string\n");
                SysFreeString(str);

                /* Confirm we get the put_data Text back */
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This &is a ; test <>\\"), "incorrect xml string\n");
                SysFreeString(str);

                /* test length property */
                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 21, "expected 21 got %ld\n", len);

                /* test substringData */
                hr = IXMLDOMComment_substringData(pComment, 0, 4, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                /* test substringData - Invalid offset */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMComment_substringData(pComment, -1, 4, &str);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid offset */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMComment_substringData(pComment, 30, 0, &str);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMComment_substringData(pComment, 0, -1, &str);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMComment_substringData(pComment, 2, 0, &str);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Start of string */
                hr = IXMLDOMComment_substringData(pComment, 0, 4, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This"), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - Middle of string */
                hr = IXMLDOMComment_substringData(pComment, 13, 4, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"test"), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - End of string */
                hr = IXMLDOMComment_substringData(pComment, 20, 4, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"\\"), "incorrect substringData string\n");
                SysFreeString(str);

                /* test appendData */
                hr = IXMLDOMComment_appendData(pComment, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_appendData(pComment, _bstr_(""));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_appendData(pComment, _bstr_("Append"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This &is a ; test <>\\Append"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* test insertData */
                str = SysAllocStringLen(NULL, 0);
                hr = IXMLDOMComment_insertData(pComment, -1, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, -1, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                SysFreeString(str);

                hr = IXMLDOMComment_insertData(pComment, -1, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 1000, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, _bstr_("Begin "));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 17, _bstr_("Middle"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 39, _bstr_(" End"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"Begin This &is a Middle; test <>\\Append End"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete data */
                /* invalid arguments */
                hr = IXMLDOMComment_deleteData(pComment, -1, 1);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, 0);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, -1);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 43, "expected 43 got %ld\n", len);

                hr = IXMLDOMComment_deleteData(pComment, len, 1);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, len+1, 1);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                /* delete from start */
                hr = IXMLDOMComment_deleteData(pComment, 0, 5);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 38, "expected 38 got %ld\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L" This &is a Middle; test <>\\Append End"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete from end */
                hr = IXMLDOMComment_deleteData(pComment, 35, 3);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 35, "expected 35 got %ld\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L" This &is a Middle; test <>\\Append "), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete from inside */
                hr = IXMLDOMComment_deleteData(pComment, 1, 33);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 2, "expected 2 got %ld\n", len);

                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"  "), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete whole data ... */
                hr = IXMLDOMComment_get_length(pComment, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_deleteData(pComment, 0, len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                /* ... and try again with empty string */
                hr = IXMLDOMComment_deleteData(pComment, 0, len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                /* ::replaceData() */
                V_VT(&v) = VT_BSTR;
                V_BSTR(&v) = SysAllocString(L"str1");
                hr = IXMLDOMComment_put_nodeValue(pComment, v);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                VariantClear(&v);

                hr = IXMLDOMComment_replaceData(pComment, 6, 0, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"str1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                hr = IXMLDOMComment_replaceData(pComment, 0, 0, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"str1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* NULL pointer means delete */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"tr1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* empty string means delete */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, _bstr_(""));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"r1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* zero count means insert */
                hr = IXMLDOMComment_replaceData(pComment, 0, 0, _bstr_("a"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"ar1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                hr = IXMLDOMComment_replaceData(pComment, 0, 2, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMComment_insertData(pComment, 0, _bstr_("m"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"m1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* nonempty string, count greater than its length */
                hr = IXMLDOMComment_replaceData(pComment, 0, 2, _bstr_("a1.2"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"a1.2"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* nonempty string, count less than its length */
                hr = IXMLDOMComment_replaceData(pComment, 0, 1, _bstr_("wine"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMComment_get_text(pComment, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"wine1.2"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                IXMLDOMComment_Release(pComment);
            }

            /* Element */
            str = SysAllocString(L"EleTest");
            hr = IXMLDOMDocument_createElement(doc, str, &pElement);
            SysFreeString(str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pElement, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMElement_get_nodeName(pElement, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"EleTest" ), "incorrect element node Name\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"<EleTest/>" ), "incorrect element xml\n");
                SysFreeString(str);

                /* Attribute */
                pAttribute = (IXMLDOMAttribute*)0x1;
                hr = IXMLDOMDocument_createAttribute(doc, NULL, &pAttribute);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                ok(pAttribute == (void*)0x1, "Expect same ptr, got %p\n", pAttribute);

                pAttribute = (IXMLDOMAttribute*)0x1;
                hr = IXMLDOMDocument_createAttribute(doc, _bstr_(""), &pAttribute);
                ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
                ok(pAttribute == (void*)0x1, "Expect same ptr, got %p\n", pAttribute);

                str = SysAllocString(L"Attr");
                hr = IXMLDOMDocument_createAttribute(doc, str, &pAttribute);
                SysFreeString(str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                if(hr == S_OK)
                {
                    IXMLDOMNode *pNewChild = (IXMLDOMNode *)0x1;

                    hr = IXMLDOMAttribute_get_nextSibling(pAttribute, NULL);
                    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                    pNextChild = (IXMLDOMNode *)0x1;
                    hr = IXMLDOMAttribute_get_nextSibling(pAttribute, &pNextChild);
                    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                    ok(pNextChild == NULL, "pNextChild not NULL\n");

                    /* test Previous Sibling*/
                    hr = IXMLDOMAttribute_get_previousSibling(pAttribute, NULL);
                    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                    pNextChild = (IXMLDOMNode *)0x1;
                    hr = IXMLDOMAttribute_get_previousSibling(pAttribute, &pNextChild);
                    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                    ok(pNextChild == NULL, "pNextChild not NULL\n");

                    hr = IXMLDOMElement_appendChild(pElement, (IXMLDOMNode*)pAttribute, &pNewChild);
                    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
                    ok(pNewChild == NULL, "pNewChild not NULL\n");

                    hr = IXMLDOMElement_get_attributes(pElement, &pAttribs);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                    if ( hr == S_OK )
                    {
                        hr = IXMLDOMNamedNodeMap_setNamedItem(pAttribs, (IXMLDOMNode*)pAttribute, NULL );
                        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                        IXMLDOMNamedNodeMap_Release(pAttribs);
                    }

                    hr = IXMLDOMAttribute_get_nodeName(pAttribute, &str);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                    ok( !lstrcmpW( str, L"Attr" ), "incorrect attribute node Name\n");
                    SysFreeString(str);

                    /* test nodeName */
                    hr = IXMLDOMAttribute_get_nodeName(pAttribute, &str);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                    ok( !lstrcmpW( str, L"Attr" ), "incorrect nodeName string\n");
                    SysFreeString(str);

                    /* test name property */
                    hr = IXMLDOMAttribute_get_name(pAttribute, &str);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                    ok( !lstrcmpW( str, L"Attr" ), "incorrect name string\n");
                    SysFreeString(str);

                    hr = IXMLDOMAttribute_get_xml(pAttribute, &str);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                    ok( !lstrcmpW( str, L"Attr=\"\"" ), "incorrect attribute xml\n");
                    SysFreeString(str);

                    IXMLDOMAttribute_Release(pAttribute);

                    /* Check Element again with the Add Attribute*/
                    hr = IXMLDOMElement_get_xml(pElement, &str);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                    ok( !lstrcmpW( str, L"<EleTest Attr=\"\"/>" ), "incorrect element xml\n");
                    SysFreeString(str);
                }

                hr = IXMLDOMElement_put_text(pElement, _bstr_("TestingNode"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"<EleTest Attr=\"\">TestingNode</EleTest>" ), "incorrect element xml\n");
                SysFreeString(str);

                /* Test for reversible escaping */
                str = SysAllocString( szStrangeChars );
                hr = IXMLDOMElement_put_text(pElement, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                SysFreeString( str );

                hr = IXMLDOMElement_get_xml(pElement, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"<EleTest Attr=\"\">&amp;x \x2103</EleTest>" ), "incorrect element xml\n");
                SysFreeString(str);

                hr = IXMLDOMElement_get_text(pElement, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, szStrangeChars ), "incorrect element text\n");
                SysFreeString(str);

                IXMLDOMElement_Release(pElement);
            }

            /* CData Section */
            str = SysAllocString(L"[1]*2=3; &gee that is not right!");
            hr = IXMLDOMDocument_createCDATASection(doc, str, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

            hr = IXMLDOMDocument_createCDATASection(doc, str, &pCDataSec);
            SysFreeString(str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMNode *pNextChild = (IXMLDOMNode *)0x1;
                VARIANT var;

                VariantInit(&var);

                hr = IXMLDOMCDATASection_QueryInterface(pCDataSec, &IID_IXMLDOMElement, (void**)&pElement);
                ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pCDataSec, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_nodeName(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"#cdata-section" ), "incorrect cdata node Name\n");
                SysFreeString(str);

                hr = IXMLDOMCDATASection_get_xml(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"<![CDATA[[1]*2=3; &gee that is not right!]]>" ), "incorrect cdata xml\n");
                SysFreeString(str);

                /* test lastChild */
                pNextChild = (IXMLDOMNode*)0x1;
                hr = IXMLDOMCDATASection_get_lastChild(pCDataSec, &pNextChild);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok(pNextChild == NULL, "pNextChild not NULL\n");

                /* put data Tests */
                hr = IXMLDOMCDATASection_put_data(pCDataSec, _bstr_("This &is a ; test <>\\"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                /* Confirm XML text is good */
                hr = IXMLDOMCDATASection_get_xml(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"<![CDATA[This &is a ; test <>\\]]>"), "incorrect xml string\n");
                SysFreeString(str);

                /* Confirm we get the put_data Text back */
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This &is a ; test <>\\"), "incorrect text string\n");
                SysFreeString(str);

                /* test length property */
                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 21, "expected 21 got %ld\n", len);

                /* test get data */
                hr = IXMLDOMCDATASection_get_data(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This &is a ; test <>\\"), "incorrect text string\n");
                SysFreeString(str);

                /* test substringData */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, 4, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                /* test substringData - Invalid offset */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, -1, 4, &str);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid offset */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 30, 0, &str);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, -1, &str);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Invalid size */
                str = (void *)0xdeadbeef;
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 2, 0, &str);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok( str == NULL, "incorrect string\n");

                /* test substringData - Start of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 0, 4, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This"), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - Middle of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 13, 4, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"test"), "incorrect substringData string\n");
                SysFreeString(str);

                /* test substringData - End of string */
                hr = IXMLDOMCDATASection_substringData(pCDataSec, 20, 4, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"\\"), "incorrect substringData string\n");
                SysFreeString(str);

                /* test appendData */
                hr = IXMLDOMCDATASection_appendData(pCDataSec, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_appendData(pCDataSec, _bstr_(""));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_appendData(pCDataSec, _bstr_("Append"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"This &is a ; test <>\\Append"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* test insertData */
                str = SysAllocStringLen(NULL, 0);
                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                SysFreeString(str);

                hr = IXMLDOMCDATASection_insertData(pCDataSec, -1, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 1000, _bstr_("Inserting"));
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, _bstr_("Begin "));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 17, _bstr_("Middle"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 39, _bstr_(" End"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"Begin This &is a Middle; test <>\\Append End"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete data */
                /* invalid arguments */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, -1, 1);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, 0);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, -1);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 43, "expected 43 got %ld\n", len);

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, len, 1);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, len+1, 1);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                /* delete from start */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, 5);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 38, "expected 38 got %ld\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L" This &is a Middle; test <>\\Append End"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete from end */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 35, 3);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 35, "expected 35 got %ld\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L" This &is a Middle; test <>\\Append "), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete from inside */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 1, 33);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(len == 2, "expected 2 got %ld\n", len);

                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"  "), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* delete whole data ... */
                hr = IXMLDOMCDATASection_get_length(pCDataSec, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                /* ... and try again with empty string */
                hr = IXMLDOMCDATASection_deleteData(pCDataSec, 0, len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                /* ::replaceData() */
                V_VT(&v) = VT_BSTR;
                V_BSTR(&v) = SysAllocString(L"str1");
                hr = IXMLDOMCDATASection_put_nodeValue(pCDataSec, v);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                VariantClear(&v);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 6, 0, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"str1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 0, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"str1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* NULL pointer means delete */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"tr1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* empty string means delete */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, _bstr_(""));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"r1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* zero count means insert */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 0, _bstr_("a"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"ar1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 2, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMCDATASection_insertData(pCDataSec, 0, _bstr_("m"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"m1"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* nonempty string, count greater than its length */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 2, _bstr_("a1.2"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"a1.2"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                /* nonempty string, count less than its length */
                hr = IXMLDOMCDATASection_replaceData(pCDataSec, 0, 1, _bstr_("wine"));
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                hr = IXMLDOMCDATASection_get_text(pCDataSec, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(!lstrcmpW(str, L"wine1.2"), "incorrect get_text string, got '%s'\n", wine_dbgstr_w(str));
                SysFreeString(str);

                IXMLDOMCDATASection_Release(pCDataSec);
            }

            /* Document Fragments */
            hr = IXMLDOMDocument_createDocumentFragment(doc, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

            hr = IXMLDOMDocument_createDocumentFragment(doc, &pDocFrag);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            if(hr == S_OK)
            {
                IXMLDOMNode *node;

                hr = IXMLDOMDocumentFragment_get_parentNode(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_parentNode(pDocFrag, &node);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok(node == NULL, "expected NULL, got %p\n", node);

                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pDocFrag, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                hr = IXMLDOMDocumentFragment_get_nodeName(pDocFrag, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"#document-fragment" ), "incorrect docfragment node Name\n");
                SysFreeString(str);

                /* test next Sibling*/
                hr = IXMLDOMDocumentFragment_get_nextSibling(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_nextSibling(pDocFrag, &node);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok(node == NULL, "next sibling not NULL\n");

                /* test Previous Sibling*/
                hr = IXMLDOMDocumentFragment_get_previousSibling(pDocFrag, NULL);
                ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

                node = (IXMLDOMNode *)0x1;
                hr = IXMLDOMDocumentFragment_get_previousSibling(pDocFrag, &node);
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
                ok(node == NULL, "previous sibling not NULL\n");

                IXMLDOMDocumentFragment_Release(pDocFrag);
            }

            /* Entity References */
            hr = IXMLDOMDocument_createEntityReference(doc, NULL, &pEntityRef);
            ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
            hr = IXMLDOMDocument_createEntityReference(doc, _bstr_(""), &pEntityRef);
            ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );

            str = SysAllocString(L"entityref");
            hr = IXMLDOMDocument_createEntityReference(doc, str, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

            hr = IXMLDOMDocument_createEntityReference(doc, str, &pEntityRef);
            SysFreeString(str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            if(hr == S_OK)
            {
                hr = IXMLDOMElement_appendChild(pRoot, (IXMLDOMNode*)pEntityRef, NULL);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                /* test get_xml*/
                hr = IXMLDOMEntityReference_get_xml(pEntityRef, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok( !lstrcmpW( str, L"&entityref;" ), "incorrect xml string\n");
                SysFreeString(str);

                IXMLDOMEntityReference_Release(pEntityRef);
            }

            IXMLDOMElement_Release( pRoot );
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

typedef struct {
    const char *name;
    const char *type;
    HRESULT hr;
} put_datatype_t;

/* Type test for elements only. Name passed into put_dataType is case-insensitive.
   So many of the names have been changed to reflect this. */
static put_datatype_t put_datatype_data[] = {
    { "test_inval",      "abcdefg",     E_FAIL },
    { "test_bool",       "Boolean",     S_OK },
    { "test_string",     "String",      S_OK },
    { "test_number",     "number",      S_OK },
    { "test_int",        "InT",         S_OK },
    { "test_fixed",      "fixed.14.4",  S_OK },
    { "test_datetime",   "DateTime",    S_OK },
    { "test_datetimetz", "DateTime.tz", S_OK },
    { "test_date",       "Date",        S_OK },
    { "test_time",       "Time",        S_OK },
    { "test_timetz",     "Time.tz",     S_OK },
    { "test_I1",         "I1",          S_OK },
    { "test_I2",         "I2",          S_OK },
    { "test_I4",         "I4",          S_OK },
    { "test_UI1",        "UI1",         S_OK },
    { "test_UI2",        "UI2",         S_OK },
    { "test_UI4",        "UI4",         S_OK },
    { "test_r4",         "r4",          S_OK },
    { "test_r8",         "r8",          S_OK },
    { "test_float",      "float",       S_OK },
    { "test_uuid",       "UuId",        S_OK },
    { "test_binhex",     "bin.hex",     S_OK },
    { "test_binbase64",  "bin.base64",  S_OK },
    { NULL }
};

typedef struct {
    DOMNodeType type;
    HRESULT hr;
} put_datatype_notype_t;

static put_datatype_notype_t put_dt_notype[] = {
    { NODE_PROCESSING_INSTRUCTION, E_FAIL },
    { NODE_DOCUMENT_FRAGMENT,      E_FAIL },
    { NODE_ENTITY_REFERENCE,       E_FAIL },
    { NODE_CDATA_SECTION,          E_FAIL },
    { NODE_COMMENT,                E_FAIL },
    { NODE_INVALID }
};

static void test_put_dataType( void )
{
    const put_datatype_notype_t *ptr2 = put_dt_notype;
    const put_datatype_t *ptr = put_datatype_data;
    IXMLDOMElement *root, *element;
    BSTR nameW, type1W, type2W;
    IXMLDOMDocument *doc;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)root, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_put_dataType(root, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    while (ptr->name)
    {
        hr = IXMLDOMDocument_createElement(doc, _bstr_(ptr->name), &element);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_appendChild(root, (IXMLDOMNode*)element, NULL);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IXMLDOMElement_put_dataType(element, _bstr_(ptr->type));
            ok(hr == ptr->hr, "failed for %s:%s, %#lx.\n", ptr->name, ptr->type, ptr->hr);

            IXMLDOMElement_Release(element);
        }
        ptr++;
    }

    /* check changing types */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing_Change"), &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_appendChild(root, (IXMLDOMNode*)element, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_put_dataType(element, _bstr_("DateTime.tz"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_put_dataType(element, _bstr_("string"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMElement_Release(element);

    /* try to set type for node without a type */
    nameW  = _bstr_("testname");
    type1W = _bstr_("string");
    type2W = _bstr_("number");
    while (ptr2->type != NODE_INVALID)
    {
        IXMLDOMNode *node;
        VARIANT type;

        V_VT(&type) = VT_I2;
        V_I2(&type) = ptr2->type;

        hr = IXMLDOMDocument_createNode(doc, type, nameW, NULL, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_appendChild(root, node, NULL);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IXMLDOMNode_put_dataType(node, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = IXMLDOMNode_put_dataType(node, type1W);
            ok(hr == ptr2->hr, "failed for type %d, %#lx\n", ptr2->type, ptr->hr);
            hr = IXMLDOMNode_put_dataType(node, type2W);
            ok(hr == ptr2->hr, "failed for type %d, %#lx\n", ptr2->type, ptr->hr);

            IXMLDOMNode_Release(node);
        }
        ptr2++;
    }

    IXMLDOMElement_Release(root);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_save(void)
{
    static const char cheA[] = "<Testing che=\"\xd0\xa7\"/>";
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMAttribute *attr;
    IXMLDOMElement *root;
    BSTR sOrig, sNew, filename;
    char buffer[100];
    IStream *stream;
    HGLOBAL global;
    VARIANT_BOOL b;
    DWORD read = 0;
    VARIANT v, dest;
    HANDLE hfile;
    HRESULT hr;
    char *ptr;

    doc = create_document(&IID_IXMLDOMDocument);
    doc2 = create_document(&IID_IXMLDOMDocument);

    /* save to IXMLDOMDocument */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)root, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)doc2;

    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_xml(doc, &sOrig);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_xml(doc2, &sNew);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok( !lstrcmpW( sOrig, sNew ), "New document is not the same as original\n");

    SysFreeString(sOrig);
    SysFreeString(sNew);

    IXMLDOMElement_Release(root);
    IXMLDOMDocument_Release(doc2);

    /* save to path */
    V_VT(&dest) = VT_BSTR;
    V_BSTR(&dest) = _bstr_("test.xml");

    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hfile = CreateFileA("test.xml", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "Could not open file: %lu\n", GetLastError());
    if(hfile == INVALID_HANDLE_VALUE) return;

    ReadFile(hfile, buffer, sizeof(buffer), &read, NULL);
    ok(read != 0, "could not read file\n");
    ok(buffer[0] != '<' || buffer[1] != '?', "File contains processing instruction\n");

    CloseHandle(hfile);
    DeleteFileA("test.xml");

    /* save to path VT_BSTR | VT_BYREF */
    filename = _bstr_("test.xml");
    V_VT(&dest) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&dest) = &filename;

    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hfile = CreateFileA("test.xml", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "Could not open file: %lu\n", GetLastError());
    if(hfile == INVALID_HANDLE_VALUE) return;

    if (hfile != INVALID_HANDLE_VALUE)
    {
       ReadFile(hfile, buffer, sizeof(buffer), &read, NULL);
       ok(read != 0, "could not read file\n");
       ok(buffer[0] != '<' || buffer[1] != '?', "File contains processing instruction\n");

       CloseHandle(hfile);
       DeleteFileA("test.xml");
    }

    /* save to stream */
    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)&savestream;

    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* loaded data contains xml declaration */
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(win1252xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &global);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ptr = GlobalLock(global);
    ok(!memcmp(ptr, win1252decl, strlen(win1252decl)), "got wrong xml declaration\n");
    GlobalUnlock(global);
    IStream_Release(stream);

    /* loaded data without xml declaration */
    hr = IXMLDOMDocument_loadXML(doc, _bstr_("<a/>"), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &global);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ptr = GlobalLock(global);
    ok(ptr[0] == '<' && ptr[1] != '?', "got wrong start tag %c%c\n", ptr[0], ptr[1]);
    GlobalUnlock(global);
    IStream_Release(stream);

    IXMLDOMDocument_Release(doc);

    /* test default encoding with non-english characters */
    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)root, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("che"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(L"\x0427");
    hr = IXMLDOMAttribute_put_value(attr, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&v);

    hr = IXMLDOMElement_setAttributeNode(root, attr, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_BSTR;
    V_BSTR(&dest) = _bstr_("test.xml");

    hr = IXMLDOMDocument_save(doc, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hfile = CreateFileA("test.xml", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "Could not open file: %lu\n", GetLastError());
    if (hfile != INVALID_HANDLE_VALUE)
    {
       ReadFile(hfile, buffer, sizeof(buffer), &read, NULL);
       ok(read != 0, "could not read file\n");
       ok(!memcmp(buffer, cheA, sizeof(cheA)-1), "got: %s\n", buffer);

       CloseHandle(hfile);
       DeleteFileA("test.xml");
    }

    IXMLDOMAttribute_Release(attr);
    IXMLDOMElement_Release(root);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_testTransforms(void)
{
    IXMLDOMDocument *doc, *docSS;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    docSS = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTransformXML), &bSucc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMDocument_loadXML(docSS, _bstr_(szTransformSSXML), &bSucc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMDocument_QueryInterface(docSS, &IID_IXMLDOMNode, (void**)&pNode );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    if(hr == S_OK)
    {
        BSTR bOut;

        hr = IXMLDOMDocument_transformNode(doc, pNode, &bOut);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        if(hr == S_OK)
        {
            ok( compareIgnoreReturns( bOut, _bstr_(szTransformOutput)), "got output %s\n", wine_dbgstr_w(bOut));
            SysFreeString(bOut);
        }

        IXMLDOMNode_Release(pNode);
    }

    IXMLDOMDocument_Release(docSS);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

struct namespaces_change_t {
    const CLSID *clsid;
    const char *name;
};

static const struct namespaces_change_t namespaces_change_test_data[] = {
    { &CLSID_DOMDocument,   "CLSID_DOMDocument"   },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2"  },
    { &CLSID_DOMDocument26, "CLSID_DOMDocument26" },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30" },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40" },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60" },
    { 0 }
};

static void test_namespaces_change(void)
{
    const struct namespaces_change_t *class_ptr = namespaces_change_test_data;

    while (class_ptr->clsid)
    {
        IXMLDOMDocument *doc = NULL;
        IXMLDOMElement *elem = NULL;
        IXMLDOMNode *node = NULL;

        VARIANT var;
        HRESULT hr;
        BSTR str;

        if (!is_clsid_supported(class_ptr->clsid, &IID_IXMLDOMDocument))
        {
            class_ptr++;
            continue;
        }

        hr = CoCreateInstance(class_ptr->clsid, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IXMLDOMDocument, (void**)&doc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&var) = VT_I2;
        V_I2(&var) = NODE_ELEMENT;

        hr = IXMLDOMDocument_createNode(doc, var, _bstr_("ns:elem"), _bstr_("ns/uri"), &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_appendChild(doc, node, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_get_documentElement(doc, &elem);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* try same prefix, different uri */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("ns/uri2");

        hr = IXMLDOMElement_setAttribute(elem, _bstr_("xmlns:ns"), var);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        /* try same prefix and uri */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = _bstr_("ns/uri");

        hr = IXMLDOMElement_setAttribute(elem, _bstr_("xmlns:ns"), var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMElement_get_xml(elem, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"<ns:elem xmlns:ns=\"ns/uri\"/>"), "got element %s for %s\n",
           wine_dbgstr_w(str), class_ptr->name);
        SysFreeString(str);

        IXMLDOMElement_Release(elem);
        IXMLDOMDocument_Release(doc);

        free_bstrs();

        class_ptr++;
    }
}

static void test_namespaces_basic(void)
{
    static const CHAR namespaces_xmlA[] =
        "<?xml version=\"1.0\"?>\n"
        "<XMI xmi.version=\"1.1\" xmlns:Model=\"http://omg.org/mof.Model/1.3\">"
        "  <XMI.content>"
        "    <Model:Package name=\"WinePackage\" Model:name2=\"name2 attr\" />"
        "  </XMI.content>"
        "</XMI>";

    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    IXMLDOMNode *node;

    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(namespaces_xmlA), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    str = (void *)0xdeadbeef;
    hr = IXMLDOMDocument_get_namespaceURI(doc, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(str == NULL, "got %p\n", str);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("//XMI.content"), &node );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if(hr == S_OK)
    {
        IXMLDOMAttribute *attr;
        IXMLDOMNode *node2;

        hr = IXMLDOMNode_get_firstChild(node, &node2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(node2 != NULL, "got %p\n", node2);

        /* Test get_prefix */
        hr = IXMLDOMNode_get_prefix(node2, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        /* NOTE: Need to test that arg2 gets cleared on Error. */

        hr = IXMLDOMNode_get_prefix(node2, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"Model"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_nodeName(node2, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"Model:Package"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        /* Test get_namespaceURI */
        hr = IXMLDOMNode_get_namespaceURI(node2, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        /* NOTE: Need to test that arg2 gets cleared on Error. */

        hr = IXMLDOMNode_get_namespaceURI(node2, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"http://omg.org/mof.Model/1.3"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_QueryInterface(node2, &IID_IXMLDOMElement, (void**)&elem);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("Model:name2"), &attr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMAttribute_get_nodeName(attr, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"Model:name2"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMAttribute_get_prefix(attr, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"Model"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMAttribute_Release(attr);
        IXMLDOMElement_Release(elem);

        IXMLDOMNode_Release(node2);
        IXMLDOMNode_Release(node);
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_FormattingXML(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *pElement;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    BSTR str;
    static const CHAR szLinefeedXML[] = "<?xml version=\"1.0\"?>\n<Root>\n\t<Sub val=\"A\" />\n</Root>";
    static const CHAR szLinefeedRootXML[] = "<Root>\r\n\t<Sub val=\"A\"/>\r\n</Root>";

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szLinefeedXML), &bSucc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");

    if(bSucc == VARIANT_TRUE)
    {
        hr = IXMLDOMDocument_get_documentElement(doc, &pElement);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        if(hr == S_OK)
        {
            hr = IXMLDOMElement_get_xml(pElement, &str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            ok( !lstrcmpW( str, _bstr_(szLinefeedRootXML) ), "incorrect element xml\n");
            SysFreeString(str);

            IXMLDOMElement_Release(pElement);
        }
    }

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

typedef struct _nodetypedvalue_t {
    const char *name;
    VARTYPE type;
    const char *value; /* value in string format */
    BOOL no_type;
} nodetypedvalue_t;

static const nodetypedvalue_t get_nodetypedvalue[] = {
    { "root/string",    VT_BSTR, "Wine", TRUE },
    { "root/string2",   VT_BSTR, "String " },
    { "root/string3",   VT_BSTR, " Wine Wine ", TRUE },
    { "root/string4",   VT_BSTR, "\nWine Wine\t", TRUE },
    { "root/string5",   VT_BSTR, "", TRUE },
    { "root/nested",    VT_BSTR, " s1 \ns2 s2\t", TRUE },
    { "root/number",    VT_BSTR, "12.44" },
    { "root/number2",   VT_BSTR, "-3.71e3" },
    { "root/int",       VT_I4,   "-13" },
    { "root/fixed",     VT_CY,   "7322.9371" },
    { "root/bool",      VT_BOOL, "-1" },
    { "root/datetime",  VT_DATE, "40135.14" },
    { "root/datetimetz",VT_DATE, "37813.59" },
    { "root/date",      VT_DATE, "665413" },
    { "root/time",      VT_DATE, "0.5813889" },
    { "root/timetz",    VT_DATE, "1.112512" },
    { "root/i1",        VT_I1,   "-13" },
    { "root/i2",        VT_I2,   "31915" },
    { "root/i4",        VT_I4,   "-312232" },
    { "root/ui1",       VT_UI1,  "123" },
    { "root/ui2",       VT_UI2,  "48282" },
    { "root/ui4",       VT_UI4,  "949281" },
    { "root/r4",        VT_R4,   "213124" },
    { "root/r8",        VT_R8,   "0.412" },
    { "root/float",     VT_R8,   "41221.421" },
    { "root/uuid",      VT_BSTR, "333C7BC4-460F-11D0-BC04-0080C7055a83" },
    { "root/binbase64", VT_ARRAY|VT_UI1, "base64 test" },
    { "root/binbase64_1", VT_ARRAY|VT_UI1, "base64 test" },
    { "root/binbase64_2", VT_ARRAY|VT_UI1, "base64 test" },
    { 0 }
};

static const char *strip_spaces(const char *str)
{
    static char buf[256];
    const char *p;

    while (isspace(*str))
        ++str;

    p = str + strlen(str);
    while (p != str && isspace(p[-1]))
        --p;

    memcpy(buf, str, p - str);
    buf[p - str] = 0;

    return buf;
}

static void test_nodeTypedValue(void)
{
    const nodetypedvalue_t *entry = get_nodetypedvalue;
    IXMLDOMDocumentType *doctype, *doctype2;
    IXMLDOMProcessingInstruction *pi;
    IXMLDOMDocumentFragment *frag;
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMCDATASection *cdata;
    IXMLDOMComment *comment;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    VARIANT value;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTypeValueXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_get_nodeValue(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = NULL;
    hr = IXMLDOMDocument_get_nodeValue(doc, &value);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
    ok(V_VT(&value) == VT_NULL, "expect VT_NULL got %d\n", V_VT(&value));

    hr = IXMLDOMDocument_get_nodeTypedValue(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMDocument_get_nodeTypedValue(doc, &value);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
    ok(V_VT(&value) == VT_NULL, "got %d\n", V_VT(&value));

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("root/string"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = NULL;
    hr = IXMLDOMNode_get_nodeValue(node, &value);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
    ok(V_VT(&value) == VT_NULL, "expect VT_NULL got %d\n", V_VT(&value));

    hr = IXMLDOMNode_get_nodeTypedValue(node, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr );

    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("root/binhex"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    {
        BYTE bytes[] = {0xff,0xfc,0xa0,0x12,0x00,0x3c};

        hr = IXMLDOMNode_get_nodeTypedValue(node, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == (VT_ARRAY|VT_UI1), "incorrect type\n");
        ok(V_ARRAY(&value)->rgsabound[0].cElements == 6, "incorrect array size\n");
        if(V_ARRAY(&value)->rgsabound[0].cElements == 6)
            ok(!memcmp(bytes, V_ARRAY(&value)->pvData, sizeof(bytes)), "incorrect value\n");
        VariantClear(&value);
        IXMLDOMNode_Release(node);
    }

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("foo"), _bstr_("value"), &pi);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    {
        V_VT(&value) = VT_NULL;
        V_BSTR(&value) = (void*)0xdeadbeef;
        hr = IXMLDOMProcessingInstruction_get_nodeTypedValue(pi, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
        ok(!lstrcmpW(V_BSTR(&value), L"value"), "got wrong value\n");
        IXMLDOMProcessingInstruction_Release(pi);
        VariantClear(&value);
    }

    hr = IXMLDOMDocument_createCDATASection(doc, _bstr_("[1]*2=3; &gee that's not right!"), &cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    {
        V_VT(&value) = VT_NULL;
        V_BSTR(&value) = (void*)0xdeadbeef;
        hr = IXMLDOMCDATASection_get_nodeTypedValue(cdata, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
        ok(!lstrcmpW(V_BSTR(&value), L"[1]*2=3; &gee that's not right!"), "got wrong value\n");
        IXMLDOMCDATASection_Release(cdata);
        VariantClear(&value);
    }

    hr = IXMLDOMDocument_createComment(doc, _bstr_("comment"), &comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    {
        V_VT(&value) = VT_NULL;
        V_BSTR(&value) = (void*)0xdeadbeef;
        hr = IXMLDOMComment_get_nodeTypedValue(comment, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == VT_BSTR, "got %d\n", V_VT(&value));
        ok(!lstrcmpW(V_BSTR(&value), L"comment"), "got wrong value\n");
        IXMLDOMComment_Release(comment);
        VariantClear(&value);
    }

    hr = IXMLDOMDocument_createDocumentFragment(doc, &frag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    {
        V_VT(&value) = VT_EMPTY;
        hr = IXMLDOMDocumentFragment_get_nodeTypedValue(frag, &value);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == VT_NULL, "got %d\n", V_VT(&value));
        IXMLDOMDocumentFragment_Release(frag);
    }

    doc2 = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc2, _bstr_(szEmailXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(b == VARIANT_TRUE, "got %d\n", b);

    EXPECT_REF(doc2, 1);

    hr = IXMLDOMDocument_get_doctype(doc2, &doctype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    EXPECT_REF(doc2, 1);
    todo_wine EXPECT_REF(doctype, 2);

    {
        V_VT(&value) = VT_EMPTY;
        hr = IXMLDOMDocumentType_get_nodeTypedValue(doctype, &value);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == VT_NULL, "got %d\n", V_VT(&value));
    }

    hr = IXMLDOMDocument_get_doctype(doc2, &doctype2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(doctype != doctype2, "got %p, was %p\n", doctype2, doctype);

    IXMLDOMDocumentType_Release(doctype2);
    IXMLDOMDocumentType_Release(doctype);

    IXMLDOMDocument_Release(doc2);

    while (entry->name)
    {
        hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_(entry->name), &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

        hr = IXMLDOMNode_get_nodeTypedValue(node, &value);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(V_VT(&value) == entry->type, "incorrect type, expected %d, got %d\n", entry->type, V_VT(&value));

        if (entry->type == (VT_ARRAY|VT_UI1))
        {
            ok(V_ARRAY(&value)->rgsabound[0].cElements == strlen(entry->value),
               "incorrect array size %ld\n", V_ARRAY(&value)->rgsabound[0].cElements);
        }

        if (entry->type != VT_BSTR)
        {
           if (entry->type == VT_DATE ||
               entry->type == VT_R8 ||
               entry->type == VT_CY)
           {
               if (entry->type == VT_DATE)
               {
                   hr = VariantChangeType(&value, &value, 0, VT_R4);
                   ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
               }
               hr = VariantChangeTypeEx(&value, &value,
                                        MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT),
                                        VARIANT_NOUSEROVERRIDE, VT_BSTR);
               ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
           }
           else
           {
               hr = VariantChangeType(&value, &value, 0, VT_BSTR);
               ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
           }

           /* for byte array from VT_ARRAY|VT_UI1 it's not a WCHAR buffer */
           if (entry->type == (VT_ARRAY|VT_UI1))
           {
               ok(!memcmp( V_BSTR(&value), entry->value, strlen(entry->value)),
                  "expected %s\n", entry->value);
           }
           else
               ok(lstrcmpW( V_BSTR(&value), _bstr_(entry->value)) == 0,
                  "expected %s, got %s\n", entry->value, wine_dbgstr_w(V_BSTR(&value)));
        }
        else
        {
           BSTR bstr, expected;

           expected = entry->no_type ? _bstr_(strip_spaces(entry->value)) : _bstr_(entry->value);
           ok(!wcscmp( V_BSTR(&value), expected ), "expected %s, got %s\n",
                debugstr_w(expected), wine_dbgstr_w(V_BSTR(&value)));
           if (entry->no_type)
           {
                VariantClear( &value );

                hr = IXMLDOMDocument_get_preserveWhiteSpace(doc, &b);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(b == VARIANT_FALSE, "got %d\n", b);

                hr = IXMLDOMNode_get_text(node, &bstr);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!wcscmp( bstr, expected ), "expected %s, got %s\n",
                        debugstr_w(expected), wine_dbgstr_w(bstr));
                SysFreeString(bstr);

                hr = IXMLDOMDocument_put_preserveWhiteSpace(doc, VARIANT_TRUE);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

                expected = _bstr_(entry->value);
                hr = IXMLDOMNode_get_text(node, &bstr);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!wcscmp( bstr, expected ), "expected %s, got %s\n",
                        debugstr_w(expected), wine_dbgstr_w(bstr));
                SysFreeString(bstr);

                hr = IXMLDOMNode_get_nodeTypedValue(node, &value);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                ok(V_VT(&value) == entry->type, "incorrect type, expected %d, got %d\n", entry->type, V_VT(&value));
                ok(!wcscmp( V_BSTR(&value), expected ), "expected %s, got %s\n",
                        debugstr_w(expected), wine_dbgstr_w(V_BSTR(&value)));

                hr = IXMLDOMDocument_put_preserveWhiteSpace(doc, VARIANT_FALSE);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
           }
        }

        VariantClear( &value );
        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_TransformWithLoadingLocalFile(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMDocument *xsl;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSucc;
    HRESULT hr;
    HANDLE file;
    DWORD dwWritten;
    char lpPathBuffer[MAX_PATH];
    int i;

    /* Create a Temp File. */
    GetTempPathA(MAX_PATH, lpPathBuffer);
    strcat(lpPathBuffer, "customers.xml" );

    file = CreateFileA(lpPathBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(file != INVALID_HANDLE_VALUE, "Could not create file: %lu\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, szBasicTransformXML, strlen(szBasicTransformXML), &dwWritten, NULL);
    CloseHandle(file);

    /* Correct path to not include an escape character. */
    for(i=0; i < strlen(lpPathBuffer); i++)
    {
        if(lpPathBuffer[i] == '\\')
            lpPathBuffer[i] = '/';
    }

    doc = create_document(&IID_IXMLDOMDocument);
    xsl = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTypeValueXML), &bSucc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
    if(bSucc == VARIANT_TRUE)
    {
        BSTR sXSL;
        BSTR sPart1 = _bstr_(szBasicTransformSSXMLPart1);
        BSTR sPart2 = _bstr_(szBasicTransformSSXMLPart2);
        BSTR sFileName = _bstr_(lpPathBuffer);
        int nLength = lstrlenW(sPart1) + lstrlenW(sPart2) + lstrlenW(sFileName) + 1;

        sXSL = SysAllocStringLen(NULL, nLength);
        lstrcpyW(sXSL, sPart1);
        lstrcatW(sXSL, sFileName);
        lstrcatW(sXSL, sPart2);

        hr = IXMLDOMDocument_loadXML(xsl, sXSL, &bSucc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
        ok(bSucc == VARIANT_TRUE, "Expected VARIANT_TRUE got VARIANT_FALSE\n");
        if(bSucc == VARIANT_TRUE)
        {
            BSTR sResult;

            hr = IXMLDOMDocument_QueryInterface(xsl, &IID_IXMLDOMNode, (void**)&pNode );
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
            if(hr == S_OK)
            {
                /* This will load the temp file via the XSL */
                hr = IXMLDOMDocument_transformNode(doc, pNode, &sResult);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
                if(hr == S_OK)
                {
                    ok( compareIgnoreReturns( sResult, _bstr_(szBasicTransformOutput)), "Stylesheet output not correct\n");
                    SysFreeString(sResult);
                }

                IXMLDOMNode_Release(pNode);
            }
        }

        SysFreeString(sXSL);
    }

    IXMLDOMDocument_Release(doc);
    IXMLDOMDocument_Release(xsl);

    DeleteFileA(lpPathBuffer);
    free_bstrs();
}

static void test_put_nodeValue(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMText *text;
    IXMLDOMEntityReference *entityref;
    IXMLDOMAttribute *attr;
    IXMLDOMNode *node;
    HRESULT hr;
    VARIANT data, type;

    doc = create_document(&IID_IXMLDOMDocument);

    /* test for unsupported types */
    /* NODE_DOCUMENT */
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_DOCUMENT_FRAGMENT */
    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_DOCUMENT_FRAGMENT;
    hr = IXMLDOMDocument_createNode(doc, type, _bstr_("test"), NULL, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_ELEMENT */
    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode(doc, type, _bstr_("test"), NULL, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
    IXMLDOMNode_Release(node);

    /* NODE_ENTITY_REFERENCE */
    hr = IXMLDOMDocument_createEntityReference(doc, _bstr_("ref"), &entityref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMEntityReference_put_nodeValue(entityref, data);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMEntityReference_QueryInterface(entityref, &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("one two three");
    hr = IXMLDOMNode_put_nodeValue(node, data);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr );
    IXMLDOMNode_Release(node);
    IXMLDOMEntityReference_Release(entityref);

    /* supported types */
    hr = IXMLDOMDocument_createTextNode(doc, _bstr_(""), &text);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("Jeeves & Wooster");
    hr = IXMLDOMText_put_nodeValue(text, data);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    IXMLDOMText_Release(text);

    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    V_VT(&data) = VT_BSTR;
    V_BSTR(&data) = _bstr_("Jeeves & Wooster");
    hr = IXMLDOMAttribute_put_nodeValue(attr, data);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    hr = IXMLDOMAttribute_get_nodeValue(attr, &data);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );
    ok(!lstrcmpW(V_BSTR(&data), L"Jeeves & Wooster"), "got %s\n", wine_dbgstr_w(V_BSTR(&data)));
    VariantClear(&data);
    IXMLDOMAttribute_Release(attr);

    free_bstrs();

    IXMLDOMDocument_Release(doc);
}

static void test_IObjectSafety(void)
{
    IXMLDOMDocument *doc;
    IObjectSafety *safety;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IObjectSafety, (void**)&safety);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    test_IObjectSafety_common(safety);

    IObjectSafety_Release(safety);
    IXMLDOMDocument_Release(doc);

    hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER,
        &IID_IObjectSafety, (void**)&safety);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    test_IObjectSafety_common(safety);

    IObjectSafety_Release(safety);

}

typedef struct _property_test_t {
    const GUID *guid;
    const char *clsid;
    const char *property;
    const char *value;
} property_test_t;

static const property_test_t properties_test_data[] = {
    { &CLSID_DOMDocument,  "CLSID_DOMDocument" , "SelectionLanguage", "XSLPattern" },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2" , "SelectionLanguage", "XSLPattern" },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", "SelectionLanguage", "XSLPattern" },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", "SelectionLanguage", "XPath" },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", "SelectionLanguage", "XPath" },
    { 0 }
};

static void test_default_properties(void)
{
    const property_test_t *entry = properties_test_data;

    while (entry->guid)
    {
        IXMLDOMDocument2 *doc;
        VARIANT var;
        HRESULT hr;

        if (!is_clsid_supported(entry->guid, &IID_IXMLDOMDocument2))
        {
            entry++;
            continue;
        }

        hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void**)&doc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument2_getProperty(doc, _bstr_(entry->property), &var);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(lstrcmpW(V_BSTR(&var), _bstr_(entry->value)) == 0, "expected %s, for %s\n",
           entry->value, entry->clsid);
        VariantClear(&var);

        IXMLDOMDocument2_Release(doc);

        entry++;
    }
}

typedef struct {
    const char *query;
    const char *list;
    BOOL todo;
} xslpattern_test_t;

static const xslpattern_test_t xslpattern_test[] = {
    { "root//elem[0]", "E1.E2.D1" },
    { "root//elem[index()=1]", "E2.E2.D1" },
    { "root//elem[index() $eq$ 1]", "E2.E2.D1" },
    { "root//elem[end()]", "E4.E2.D1" },
    { "root//elem[$not$ end()]", "E1.E2.D1 E2.E2.D1 E3.E2.D1" },
    { "root//elem[index() != 0]", "E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() $ne$ 0]", "E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() < 2]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() $lt$ 2]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() <= 1]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() $le$ 1]", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[index() > 1]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() $gt$ 1]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() >= 2]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[index() $ge$ 2]", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[a $ieq$ 'a2 field']", "E2.E2.D1" },
    { "root//elem[a $ine$ 'a2 field']", "E1.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[a $ilt$ 'a3 field']", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[a $ile$ 'a2 field']", "E1.E2.D1 E2.E2.D1" },
    { "root//elem[a $igt$ 'a2 field']", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[a $ige$ 'a3 field']", "E3.E2.D1 E4.E2.D1" },
    { "root//elem[$any$ *='B2 field']", "E2.E2.D1" },
    { "root//elem[$all$ *!='B2 field']", "E1.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root//elem[index()=0 or end()]", "E1.E2.D1 E4.E2.D1" },
    { "root//elem[index()=0 $or$ end()]", "E1.E2.D1 E4.E2.D1" },
    { "root//elem[index()=0 || end()]", "E1.E2.D1 E4.E2.D1" },
    { "root//elem[index()>0 and $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root//elem[index()>0 $and$ $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root//elem[index()>0 && $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root/elem[0]", "E1.E2.D1" },
    { "root/elem[index()=1]", "E2.E2.D1" },
    { "root/elem[index() $eq$ 1]", "E2.E2.D1" },
    { "root/elem[end()]", "E4.E2.D1" },
    { "root/elem[$not$ end()]", "E1.E2.D1 E2.E2.D1 E3.E2.D1" },
    { "root/elem[index() != 0]", "E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root/elem[index() $ne$ 0]", "E2.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root/elem[index() < 2]", "E1.E2.D1 E2.E2.D1" },
    { "root/elem[index() $lt$ 2]", "E1.E2.D1 E2.E2.D1" },
    { "root/elem[index() <= 1]", "E1.E2.D1 E2.E2.D1" },
    { "root/elem[index() $le$ 1]", "E1.E2.D1 E2.E2.D1" },
    { "root/elem[index() > 1]", "E3.E2.D1 E4.E2.D1" },
    { "root/elem[index() $gt$ 1]", "E3.E2.D1 E4.E2.D1" },
    { "root/elem[index() >= 2]", "E3.E2.D1 E4.E2.D1" },
    { "root/elem[index() $ge$ 2]", "E3.E2.D1 E4.E2.D1" },
    { "root/elem[a $ieq$ 'a2 field']", "E2.E2.D1" },
    { "root/elem[a $ine$ 'a2 field']", "E1.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root/elem[a $ilt$ 'a3 field']", "E1.E2.D1 E2.E2.D1" },
    { "root/elem[a $ile$ 'a2 field']", "E1.E2.D1 E2.E2.D1" },
    { "root/elem[a $igt$ 'a2 field']", "E3.E2.D1 E4.E2.D1" },
    { "root/elem[a $ige$ 'a3 field']", "E3.E2.D1 E4.E2.D1" },
    { "root/elem[$any$ *='B2 field']", "E2.E2.D1" },
    { "root/elem[$all$ *!='B2 field']", "E1.E2.D1 E3.E2.D1 E4.E2.D1" },
    { "root/elem[index()=0 or end()]", "E1.E2.D1 E4.E2.D1" },
    { "root/elem[index()=0 $or$ end()]", "E1.E2.D1 E4.E2.D1" },
    { "root/elem[index()=0 || end()]", "E1.E2.D1 E4.E2.D1" },
    { "root/elem[index()>0 and $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root/elem[index()>0 $and$ $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root/elem[index()>0 && $not$ end()]", "E2.E2.D1 E3.E2.D1" },
    { "root/elem[d]", "E1.E2.D1 E2.E2.D1 E4.E2.D1" },
    { "root/elem[@*]", "E2.E2.D1 E3.E2.D1", TRUE },
    { NULL }
};

static const xslpattern_test_t xslpattern_test_no_ns[] = {
    /* prefixes don't need to be registered, you may use them as they are in the doc */
    { "//bar:x", "E6.E1.E5.E1.E2.D1 E6.E2.E5.E1.E2.D1" },
    /* prefixes must be explicitly specified in the name */
    { "//foo:elem", "" },
    { "//foo:c", "E3.E4.E2.D1" },
    { NULL }
};

static const xslpattern_test_t xslpattern_test_func[] = {
    { "attribute()", "" },
    { "attribute('depth')", "" },
    { "root/attribute('depth')", "A'depth'.E3.D1" },
    { "//x/attribute()", "A'id'.E3.E3.D1 A'depth'.E3.E3.D1" },
    { "//x//attribute(id)", NULL },
    { "//x//attribute('id')", "A'id'.E3.E3.D1 A'id'.E4.E3.E3.D1 A'id'.E5.E3.E3.D1 A'id'.E6.E3.E3.D1" },
    { "comment()", "C2.D1" },
    { "//comment()", "C2.D1 C1.E3.D1 C2.E3.E3.D1 C2.E4.E3.D1" },
    { "element()", "E3.D1" },
    { "root/y/element()", "E4.E4.E3.D1 E5.E4.E3.D1 E6.E4.E3.D1" },
    { "//element(a)", NULL },
    { "//element('a')", "E4.E3.E3.D1 E4.E4.E3.D1" },
    { "node()", "P1.D1 C2.D1 E3.D1" },
    { "//x/node()", "P1.E3.E3.D1 C2.E3.E3.D1 T3.E3.E3.D1 E4.E3.E3.D1 E5.E3.E3.D1 E6.E3.E3.D1" },
    { "//x/node()[nodeType()=1]", "E4.E3.E3.D1 E5.E3.E3.D1 E6.E3.E3.D1" },
    { "//x/node()[nodeType()=3]", "T3.E3.E3.D1" },
    { "//x/node()[nodeType()=7]", "P1.E3.E3.D1" },
    { "//x/node()[nodeType()=8]", "C2.E3.E3.D1" },
    { "pi()", "P1.D1" },
    { "//y/pi()", "P1.E4.E3.D1" },
    { "root/textnode()", "T2.E3.D1" },
    { "root/element()/textnode()", "T3.E3.E3.D1 T3.E4.E3.D1" },
    { NULL }
};

static void test_XSLPattern(void)
{
    const xslpattern_test_t *ptr = xslpattern_test;
    IXMLDOMDocument2 *doc;
    IXMLDOMNodeList *list;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument2);

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szExampleXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* switch to XSLPattern */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* XPath doesn't select elements with non-null default namespace with unqualified selectors, XSLPattern does */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//elem/c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 0;
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* should select <elem><c> and <elem xmlns='...'><c> but not <elem><foo:c> */
    ok(len == 3, "expected 3 entries in list, got %ld\n", len);
    IXMLDOMNodeList_Release(list);

    while (ptr->query)
    {
        list = NULL;
        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_(ptr->query), &list);
        ok(hr == S_OK, "query=%s, failed with %#lx\n", ptr->query, hr);
        len = 0;
        hr = IXMLDOMNodeList_get_length(list, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len != 0, "query=%s, empty list\n", ptr->query);
        if (len) {
            if (ptr->todo) {
                char *str = list_to_string(list);
                todo_wine
                ok(!strcmp(str, ptr->list), "Invalid node list: %s, expected %s\n", str, ptr->list);
                IXMLDOMNodeList_Release(list);
            }
            else
                expect_list_and_release(list, ptr->list);
        }

        ptr++;
    }

    /* namespace handling */
    /* no registered namespaces */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_(""));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ptr = xslpattern_test_no_ns;
    while (ptr->query)
    {
        list = NULL;
        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_(ptr->query), &list);
        ok(hr == S_OK, "query=%s, failed with %#lx.\n", ptr->query, hr);

        if (*ptr->list)
        {
            len = 0;
            hr = IXMLDOMNodeList_get_length(list, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(len != 0, "query=%s, empty list\n", ptr->query);
        }
        else
        {
            len = 1;
            hr = IXMLDOMNodeList_get_length(list, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(len == 0, "query=%s, empty list\n", ptr->query);
        }
        if (len)
            expect_list_and_release(list, ptr->list);

        ptr++;
    }

    /* explicitly register prefix foo */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"),
            _variantbstr_("xmlns:foo='urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29'"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* now we get the same behavior as XPath */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//foo:c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    len = 0;
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len != 0, "expected filled list\n");
    if (len)
        expect_list_and_release(list, "E3.E3.E2.D1 E3.E4.E2.D1");

    /* set prefix foo to some nonexistent namespace */
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_("xmlns:foo='urn:nonexistent-foo'"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* the registered prefix takes precedence */
    hr = IXMLDOMDocument2_selectNodes(doc, _bstr_("//foo:c"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    len = 0;
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "expected empty list\n");
    IXMLDOMNodeList_Release(list);

    IXMLDOMDocument2_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument2);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(szNodeTypesXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    ptr = xslpattern_test_func;
    while (ptr->query)
    {
        list = NULL;
        hr = IXMLDOMDocument2_selectNodes(doc, _bstr_(ptr->query), &list);
        if (ptr->list)
        {
            ok(hr == S_OK, "query=%s, failed with %#lx.\n", ptr->query, hr);
            len = 0;
            hr = IXMLDOMNodeList_get_length(list, &len);
            ok(hr == S_OK, "Failed to get list length, hr %#lx.\n", hr);
            if (*ptr->list)
            {
                ok(len != 0, "query=%s, empty list\n", ptr->query);
                if (len)
                    expect_list_and_release(list, ptr->list);
            }
            else
                ok(len == 0, "query=%s, filled list\n", ptr->query);
        }
        else
            ok(hr == E_FAIL, "query=%s, failed with %#lx.\n", ptr->query, hr);

        ptr++;
    }

    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static void test_splitText(void)
{
    IXMLDOMCDATASection *cdata;
    IXMLDOMElement *root;
    IXMLDOMDocument *doc;
    IXMLDOMText *text, *text2;
    IXMLDOMNode *node;
    VARIANT var;
    VARIANT_BOOL success;
    LONG length;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_("<root></root>"), &success);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createCDATASection(doc, _bstr_("beautiful plumage"), &cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_EMPTY;
    hr = IXMLDOMElement_appendChild(root, (IXMLDOMNode*)cdata, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    length = 0;
    hr = IXMLDOMCDATASection_get_length(cdata, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length > 0, "got %ld\n", length);

    hr = IXMLDOMCDATASection_splitText(cdata, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    text = (void*)0xdeadbeef;
    /* negative offset */
    hr = IXMLDOMCDATASection_splitText(cdata, -1, &text);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(text == (void*)0xdeadbeef, "got %p\n", text);

    text = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMCDATASection_splitText(cdata, length + 1, &text);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(text == 0, "got %p\n", text);

    text = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMCDATASection_splitText(cdata, length, &text);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(text == 0, "got %p\n", text);

    /* no empty node created */
    node = (void*)0xdeadbeef;
    hr = IXMLDOMCDATASection_get_nextSibling(cdata, &node);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(node == 0, "got %p\n", text);

    hr = IXMLDOMCDATASection_splitText(cdata, 10, &text);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    length = 0;
    hr = IXMLDOMText_get_length(text, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 7, "got %ld\n", length);

    hr = IXMLDOMCDATASection_get_nextSibling(cdata, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    /* split new text node */
    hr = IXMLDOMText_get_length(text, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMText_get_nextSibling(text, &node);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(node == 0, "got %p\n", text);

    hr = IXMLDOMText_splitText(text, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    text2 = (void*)0xdeadbeef;
    /* negative offset */
    hr = IXMLDOMText_splitText(text, -1, &text2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(text2 == (void*)0xdeadbeef, "got %p\n", text2);

    text2 = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMText_splitText(text, length + 1, &text2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(text2 == 0, "got %p\n", text2);

    text2 = (void*)0xdeadbeef;
    /* offset outside data */
    hr = IXMLDOMText_splitText(text, length, &text2);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(text2 == 0, "got %p\n", text);

    text2 = 0;
    hr = IXMLDOMText_splitText(text, 4, &text2);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (text2) IXMLDOMText_Release(text2);

    node = 0;
    hr = IXMLDOMText_get_nextSibling(text, &node);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (node) IXMLDOMNode_Release(node);

    IXMLDOMText_Release(text);
    IXMLDOMElement_Release(root);
    IXMLDOMCDATASection_Release(cdata);
    free_bstrs();
}

typedef struct {
    const char *name;
    const char *uri;
    HRESULT hr;
} ns_item_t;

/* default_ns_doc used */
static const ns_item_t qualified_item_tests[] = {
    { "xml:lang", NULL, S_FALSE },
    { "xml:lang", "http://www.w3.org/XML/1998/namespace", S_FALSE },
    { "lang", "http://www.w3.org/XML/1998/namespace", S_OK },
    { "ns:b", NULL, S_FALSE },
    { "ns:b", "nshref", S_FALSE },
    { "b", "nshref", S_OK },
    { "d", NULL, S_OK },
    { NULL }
};

static const ns_item_t named_item_tests[] = {
    { "xml:lang", NULL, S_OK },
    { "lang", NULL, S_FALSE },
    { "ns:b", NULL, S_OK },
    { "b", NULL, S_FALSE },
    { "d", NULL, S_OK },
    { NULL }
};

static void test_getQualifiedItem(void)
{
    IXMLDOMNode *pr_node, *node;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMElement *element;
    const ns_item_t* ptr;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes(element, &root_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item(root_list, 1, &pr_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNodeList_Release(root_list);

    hr = IXMLDOMNode_get_attributes(pr_node, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(pr_node);

    len = 0;
    hr = IXMLDOMNamedNodeMap_get_length(map, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 3, "length %ld\n", len);

    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, NULL, NULL, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0xdeadbeef, "got %p\n", node);

    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, _bstr_("id"), NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, _bstr_("id"), NULL, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMNode_Release(node);
    IXMLDOMNamedNodeMap_Release(map);
    IXMLDOMElement_Release(element);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(default_ns_doc), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("a"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMElement_get_attributes(element, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ptr = qualified_item_tests;
    while (ptr->name)
    {
       node = (void*)0xdeadbeef;
       hr = IXMLDOMNamedNodeMap_getQualifiedItem(map, _bstr_(ptr->name), _bstr_(ptr->uri), &node);
       ok(hr == ptr->hr, "%s, %s: unexpected hr %#lx, expected %#lx.\n", ptr->name, ptr->uri, hr, ptr->hr);
       if (hr == S_OK)
           IXMLDOMNode_Release(node);
       else
           ok(node == NULL, "%s, %s: got %p\n", ptr->name, ptr->uri, node);
       ptr++;
    }

    ptr = named_item_tests;
    while (ptr->name)
    {
       node = (void*)0xdeadbeef;
       hr = IXMLDOMNamedNodeMap_getNamedItem(map, _bstr_(ptr->name), &node);
       ok(hr == ptr->hr, "%s: unexpected hr %#lx, expected %#lx.\n", ptr->name, hr, ptr->hr);
       if (hr == S_OK)
           IXMLDOMNode_Release(node);
       else
           ok(node == NULL, "%s: got %p\n", ptr->name, node);
       ptr++;
    }

    IXMLDOMNamedNodeMap_Release(map);

    IXMLDOMElement_Release(element);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_removeQualifiedItem(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *element;
    IXMLDOMNode *pr_node, *node;
    IXMLDOMNodeList *root_list;
    IXMLDOMNamedNodeMap *map;
    VARIANT_BOOL b;
    LONG len;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes(element, &root_list);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_item(root_list, 1, &pr_node);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNodeList_Release(root_list);

    hr = IXMLDOMNode_get_attributes(pr_node, &map);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(pr_node);

    hr = IXMLDOMNamedNodeMap_get_length(map, &len);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( len == 3, "length %ld\n", len);

    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, NULL, NULL, NULL);
    ok( hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, NULL, NULL, &node);
    ok( hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( node == (void*)0xdeadbeef, "got %p\n", node);

    /* out pointer is optional */
    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, _bstr_("id"), NULL, NULL);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* already removed */
    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, _bstr_("id"), NULL, NULL);
    ok( hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNamedNodeMap_removeQualifiedItem(map, _bstr_("vr"), NULL, &node);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    IXMLDOMNamedNodeMap_Release( map );
    IXMLDOMElement_Release( element );
    IXMLDOMDocument_Release( doc );
    free_bstrs();
}

#define check_default_props(doc) _check_default_props(__LINE__, doc)
static inline void _check_default_props(int line, IXMLDOMDocument2* doc)
{
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionLanguage"), &var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);

    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), L"") == 0, "expected empty string\n");
    VariantClear(&var);

    hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc, &b);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(b == VARIANT_FALSE, "expected FALSE\n");

    hr = IXMLDOMDocument2_get_schemas(doc, &var);
    ok_(__FILE__, line)(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);
}

#define check_set_props(doc) _check_set_props(__LINE__, doc)
static inline void _check_set_props(int line, IXMLDOMDocument2* doc)
{
    VARIANT_BOOL b;
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionLanguage"), &var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), L"XPath") == 0, "expected XPath\n");
    VariantClear(&var);

    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("SelectionNamespaces"), &var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(lstrcmpW(V_BSTR(&var), L"xmlns:wi=\'www.winehq.org\'") == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMDocument2_get_preserveWhiteSpace(doc, &b);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(b == VARIANT_TRUE, "expected TRUE\n");

    hr = IXMLDOMDocument2_get_schemas(doc, &var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(V_VT(&var) != VT_NULL, "expected pointer\n");
    VariantClear(&var);
}

#define set_props(doc, cache) _set_props(__LINE__, doc, cache)
static inline void _set_props(int line, IXMLDOMDocument2* doc, IXMLDOMSchemaCollection* cache)
{
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XPath"));
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_("xmlns:wi=\'www.winehq.org\'"));
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_TRUE);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = NULL;
    hr = IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&var));
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(V_DISPATCH(&var) != NULL, "expected pointer\n");
    hr = IXMLDOMDocument2_putref_schemas(doc, var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);
}

#define unset_props(doc) _unset_props(__LINE__, doc)
static inline void _unset_props(int line, IXMLDOMDocument2* doc)
{
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionLanguage"), _variantbstr_("XSLPattern"));
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("SelectionNamespaces"), _variantbstr_(""));
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument2_put_preserveWhiteSpace(doc, VARIANT_FALSE);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&var) = VT_NULL;
    hr = IXMLDOMDocument2_putref_schemas(doc, var);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&var);
}

static void test_get_ownerDocument(void)
{
    IXMLDOMDocument *doc1, *doc2, *doc3;
    IXMLDOMDocument2 *doc, *doc_owner;
    IXMLDOMNode *node;
    IXMLDOMSchemaCollection *cache;
    VARIANT_BOOL b;
    VARIANT var;
    IXMLDOMElement *element;
    IXMLDOMNodeList *node_list;
    IXMLDOMAttribute *attr;
    LONG i, len;
    HRESULT hr;
    const CHAR nodeXML[] =
    "<root id='0'>"
    "   <!-- comment node 0 -->"
    "   text node 0"
    "   <x attr='val'></x>"
    "   <?foo value='PI for x'?>"
    "   <![CDATA[ cdata ]]>"
    "</root>";

    if (!is_clsid_supported(&CLSID_DOMDocument2, &IID_IXMLDOMDocument2)) return;
    if (!is_clsid_supported(&CLSID_XMLSchemaCache, &IID_IXMLDOMSchemaCollection)) return;

    doc = create_document(&IID_IXMLDOMDocument2);
    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    VariantInit(&var);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(complete4A), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    check_default_props(doc);

    /* set properties and check that new instances use them */
    set_props(doc, cache);
    check_set_props(doc);

    hr = IXMLDOMDocument2_get_firstChild(doc, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_ownerDocument(node, &doc1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* new interface keeps props */
    hr = IXMLDOMDocument_QueryInterface(doc1, &IID_IXMLDOMDocument2, (void**)&doc_owner);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( doc_owner != doc, "got %p, doc %p\n", doc_owner, doc);
    check_set_props(doc_owner);
    IXMLDOMDocument2_Release(doc_owner);

    hr = IXMLDOMNode_get_ownerDocument(node, &doc2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    ok(doc1 != doc2, "got %p, expected %p. original %p\n", doc2, doc1, doc);

    /* reload */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(complete4A), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    /* properties retained even after reload */
    check_set_props(doc);

    hr = IXMLDOMDocument2_get_firstChild(doc, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_ownerDocument(node, &doc3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument_QueryInterface(doc3, &IID_IXMLDOMDocument2, (void**)&doc_owner);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(doc3 != doc1 && doc3 != doc2 && doc_owner != doc, "got %p, (%p, %p, %p)\n", doc3, doc, doc1, doc2);
    check_set_props(doc_owner);

    /* changing properties for one instance changes them for all */
    unset_props(doc_owner);
    check_default_props(doc_owner);
    check_default_props(doc);

    /* NULL check */
    hr = IXMLDOMDocument_loadXML(doc1, _bstr_(nodeXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc1, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes(element, &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_length(node_list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for(i = 0; i < len; i++) {
        hr = IXMLDOMNodeList_get_item(node_list, i, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNode_get_ownerDocument(node, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        IXMLDOMNode_Release(node);
    }
    IXMLDOMElement_Release(element);

    /* Test Attribute Node */
    hr = IXMLDOMNodeList_get_item(node_list, 2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMElement_getAttributeNode(element, _bstr_("attr"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attr != NULL, "attr == NULL\n");
    IXMLDOMElement_Release(element);
    hr = IXMLDOMAttribute_get_ownerDocument(attr, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IXMLDOMAttribute_Release(attr);
    IXMLDOMNodeList_Release(node_list);

    IXMLDOMSchemaCollection_Release(cache);
    IXMLDOMDocument_Release(doc1);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc3);
    IXMLDOMDocument2_Release(doc);
    IXMLDOMDocument2_Release(doc_owner);
    free_bstrs();
}

static void test_setAttributeNode(void)
{
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMElement *elem, *elem2;
    IXMLDOMAttribute *attr, *attr2, *ret_attr;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;
    BSTR str;
    ULONG ref1, ref2;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem2);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( elem2 != elem, "got same instance\n");

    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, NULL, &ret_attr);
    ok( hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok( ret_attr == (void*)0xdeadbeef, "got %p\n", ret_attr);

    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ref1 = IXMLDOMElement_AddRef(elem);
    IXMLDOMElement_Release(elem);

    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( ret_attr == NULL, "got %p\n", ret_attr);

    /* no reference added */
    ref2 = IXMLDOMElement_AddRef(elem);
    IXMLDOMElement_Release(elem);
    ok(ref2 == ref1, "got %ld, expected %ld\n", ref2, ref1);

    EXPECT_CHILDREN(elem);
    EXPECT_CHILDREN(elem2);

    IXMLDOMElement_Release(elem2);

    attr2 = NULL;
    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("attr"), &attr2);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( attr2 != attr, "got same instance %p\n", attr2);
    IXMLDOMAttribute_Release(attr2);

    /* try to add it another time */
    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok( hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( ret_attr == (void*)0xdeadbeef, "got %p\n", ret_attr);

    IXMLDOMElement_Release(elem);

    /* initially used element is released, attribute still 'has' a container */
    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok( hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok( ret_attr == (void*)0xdeadbeef, "got %p\n", ret_attr);
    IXMLDOMElement_Release(elem);

    /* add attribute already attached to another document */
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML( doc2, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_get_documentElement(doc2, &elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_setAttributeNode(elem, attr, NULL);
    ok( hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(elem);

    IXMLDOMAttribute_Release(attr);

    /* create element, add attribute, see if it's copied or linked */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("test"), &elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    attr = NULL;
    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attr != NULL, "got %p\n", attr);

    ref1 = IXMLDOMAttribute_AddRef(attr);
    IXMLDOMAttribute_Release(attr);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("attrvalue1");
    hr = IXMLDOMAttribute_put_nodeValue(attr, v);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IXMLDOMAttribute_get_xml(attr, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lstrcmpW(str, L"attr=\"attrvalue1\"") == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    ret_attr = (void*)0xdeadbeef;
    hr = IXMLDOMElement_setAttributeNode(elem, attr, &ret_attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret_attr == NULL, "got %p\n", ret_attr);

    /* attribute reference increased */
    ref2 = IXMLDOMAttribute_AddRef(attr);
    IXMLDOMAttribute_Release(attr);
    ok(ref1 == ref2, "got %ld, expected %ld\n", ref2, ref1);

    hr = IXMLDOMElement_get_xml(elem, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lstrcmpW(str, L"<test attr=\"attrvalue1\"/>") == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("attrvalue2");
    hr = IXMLDOMAttribute_put_nodeValue(attr, v);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_xml(elem, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(lstrcmpW(str, L"<test attr=\"attrvalue2\"/>") == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMAttribute_Release(attr);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_createNode(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    IXMLDOMNode *node;
    VARIANT v, var;
    BSTR prefix, str;
    HRESULT hr;
    ULONG ref;

    doc = create_document(&IID_IXMLDOMDocument);

    EXPECT_REF(doc, 1);

    /* reference count tests */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* initial reference is 2 */
todo_wine {
    EXPECT_REF(elem, 2);
    ref = IXMLDOMElement_Release(elem);
    ok(ref == 1, "got %ld\n", ref);
    /* it's released already, attempt to release now will crash it */
}

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(elem, 2);
    IXMLDOMDocument_Release(doc);
    todo_wine EXPECT_REF(elem, 2);
    IXMLDOMElement_Release(elem);

    doc = create_document(&IID_IXMLDOMDocument);

    /* NODE_ELEMENT nodes */
    /* 1. specified namespace */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;

    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("ns1:test"), _bstr_("http://winehq.org"), &node);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    prefix = NULL;
    hr = IXMLDOMNode_get_prefix(node, &prefix);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lstrcmpW(prefix, L"ns1") == 0, "wrong prefix\n");
    SysFreeString(prefix);
    IXMLDOMNode_Release(node);

    /* 2. default namespace */
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("test"), _bstr_("http://winehq.org/default"), &node);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    prefix = (void*)0xdeadbeef;
    hr = IXMLDOMNode_get_prefix(node, &prefix);
    ok( hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(prefix == 0, "expected empty prefix, got %p\n", prefix);
    /* check dump */
    hr = IXMLDOMNode_get_xml(node, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lstrcmpW(str, L"<test xmlns=\"http://winehq.org/default\"/>") == 0,
        "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_BSTR;
    hr = IXMLDOMElement_getAttribute(elem, _bstr_("xmlns"), &var);
    ok( hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_NULL, "got %d\n", V_VT(&var));

    str = NULL;
    hr = IXMLDOMElement_get_namespaceURI(elem, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lstrcmpW(str, L"http://winehq.org/default") == 0, "expected default namespace\n");
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMNode_Release(node);

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static const char get_prefix_doc[] =
    "<?xml version=\"1.0\" ?>"
    "<a xmlns:ns1=\"ns1 href\" />";

static void test_get_prefix(void)
{
    IXMLDOMDocumentFragment *fragment;
    IXMLDOMCDATASection *cdata;
    IXMLDOMElement *element;
    IXMLDOMComment *comment;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    /* nodes that can't support prefix */
    /* 1. document */
    str = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_prefix(doc, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(str == NULL, "got %p\n", str);

    hr = IXMLDOMDocument_get_prefix(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* 2. cdata */
    hr = IXMLDOMDocument_createCDATASection(doc, NULL, &cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMCDATASection_get_prefix(cdata, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMCDATASection_get_prefix(cdata, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IXMLDOMCDATASection_Release(cdata);

    /* 3. comment */
    hr = IXMLDOMDocument_createComment(doc, NULL, &comment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMComment_get_prefix(comment, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMComment_get_prefix(comment, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IXMLDOMComment_Release(comment);

    /* 4. fragment */
    hr = IXMLDOMDocument_createDocumentFragment(doc, &fragment);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMDocumentFragment_get_prefix(fragment, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( str == 0, "got %p\n", str);

    hr = IXMLDOMDocumentFragment_get_prefix(fragment, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IXMLDOMDocumentFragment_Release(fragment);

    /* no prefix */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_prefix(element, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_prefix(element, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(str == 0, "got %p\n", str);

    IXMLDOMElement_Release(element);

    /* with prefix */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("a:elem"), &element);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_prefix(element, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lstrcmpW(str, L"a") == 0, "expected prefix \"a\"\n");
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_namespaceURI(element, &str);
    ok( hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( str == 0, "got %p\n", str);

    IXMLDOMElement_Release(element);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(get_prefix_doc), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_prefix(element, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(str == NULL, "got %p\n", str);

    str = (void*)0xdeadbeef;
    hr = IXMLDOMElement_get_namespaceURI(element, &str);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_selectSingleNode(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMNodeList *list;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    HRESULT hr;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_selectSingleNode(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_loadXML( doc, _bstr_(complete4A), &b );
    ok( hr == S_OK, "loadXML failed\n");
    ok( b == VARIANT_TRUE, "failed to load XML string\n");

    hr = IXMLDOMDocument_selectSingleNode(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("lc"), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("lc"), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("lc"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("lc"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNodeList_Release(list);

    list = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_selectNodes(doc, NULL, &list);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(list == (void*)0xdeadbeef, "got %p\n", list);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("nonexistent"), &node);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(node == 0, "got %p\n", node);

    list = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("nonexistent"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    len = 1;
    hr = IXMLDOMNodeList_get_length(list, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %ld\n", len);
    IXMLDOMNodeList_Release(list);

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_events(void)
{
    IConnectionPointContainer *conn;
    IConnectionPoint *point;
    IXMLDOMDocument *doc;
    HRESULT hr;
    VARIANT v;
    IDispatch *event;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IConnectionPointContainer, (void**)&conn);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IConnectionPointContainer_FindConnectionPoint(conn, &IID_IDispatch, &point);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IConnectionPoint_Release(point);
    hr = IConnectionPointContainer_FindConnectionPoint(conn, &IID_IPropertyNotifySink, &point);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IConnectionPoint_Release(point);
    hr = IConnectionPointContainer_FindConnectionPoint(conn, &DIID_XMLDOMDocumentEvents, &point);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IConnectionPoint_Release(point);

    IConnectionPointContainer_Release(conn);

    /* ready state callback */
    VariantInit(&v);
    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);

    event = create_dispevent();
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)event;

    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(event, 2);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = event;

    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(event, 2);

    /* VT_NULL doesn't reset event handler */
    V_VT(&v) = VT_NULL;
    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(event, 2);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;

    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(event, 1);

    V_VT(&v) = VT_UNKNOWN;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMDocument_put_onreadystatechange(doc, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDispatch_Release(event);

    IXMLDOMDocument_Release(doc);
}

static void test_createProcessingInstruction(void)
{
    static const WCHAR xml1[] = L"<?xml version=\"1.0\"?>\r\n<test/>\r\n";
    static const char xml2[] = "<?xml version=\"1.0\" encoding=\"windows-1252\"?>\r\n<test/>\r\n";
    static const char xml2_wine[] = "<?xml version=\"1.0\" encoding=\"windows-1252\"?>\n<test/>\n";
    static const char xml3[] = "<?xml version=\"1.0\" standalone=\"yes\"?>\r\n<test/>\r\n";
    static const char xml3_wine[] = "<?xml version=\"1.0\" standalone=\"yes\"?>\n<test/>\n";
    IXMLDOMProcessingInstruction *pi;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *item;
    IXMLDOMNamedNodeMap *node_map;
    IXMLDOMElement *element;
    WCHAR buff[10];
    BSTR xml, bstr;
    VARIANT var;
    HRESULT hr;
    IStream *stream;
    LARGE_INTEGER off;
    VARIANT_BOOL b;
    HGLOBAL global;
    char *p;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\" encoding=\"windows-1252\" dummy=\"value\""), &pi);
    ok(hr == XML_E_UNEXPECTED_ATTRIBUTE, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, NULL, _bstr_("version=\"1.0\" encoding=\"UTF-8\""), &pi);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), NULL, &pi);
    ok(hr == XML_E_XMLDECLSYNTAX, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\" encoding=UTF-8"), &pi);
    ok(hr == XML_E_MISSINGQUOTE, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\" encoding='UTF-8\""), &pi);
    ok(hr == XML_E_BADCHARINSTRING, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\" encoding=\"UTF-8"), &pi);
    ok(hr == XML_E_BADCHARINSTRING, "Unexpected hr %#lx.\n", hr);
    pi = NULL;
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\" encoding='UTF-8'"), &pi);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMProcessingInstruction_QueryInterface(pi, &IID_IXMLDOMNode, (void **)&node);
    node_map = NULL;
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_attributes(node, &node_map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNamedNodeMap_Release(node_map);
    IXMLDOMNode_Release(node);
    IXMLDOMProcessingInstruction_Release(pi);

    /* test for BSTR handling, pass broken BSTR */
    memcpy(&buff[2], L"test", 5 * sizeof(WCHAR));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("test"), &buff[2], &pi);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMProcessingInstruction_Release(pi);
    IXMLDOMDocument_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"), _bstr_("version=\"1.0\" encoding=\"windows-1252\""), &pi);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMProcessingInstruction_QueryInterface(pi, &IID_IXMLDOMNode, (void **)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_appendChild(doc, node, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_attributes(node, &node_map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    item = (void *)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_getNamedItem(node_map, _bstr_("xml"), &item);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(!item, "got %p\n", item);

    item = NULL;
    hr = IXMLDOMNamedNodeMap_getNamedItem(node_map, _bstr_("encoding"), &item);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(item != NULL, "got NULL\n");

    hr = IXMLDOMNode_get_nodeName(item, &bstr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(bstr, L"encoding"), "got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    VariantInit(&var);
    hr = IXMLDOMNode_get_nodeValue(item, &var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "got %u\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"windows-1252"), "got %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    IXMLDOMNamedNodeMap_Release(node_map);
    IXMLDOMNode_Release(node);
    IXMLDOMProcessingInstruction_Release(pi);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("test"), &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode *)element, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(element);

    hr = IXMLDOMDocument_get_xml(doc, &xml);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!wcscmp(xml, xml1), "got %s\n", wine_dbgstr_w(xml));
    SysFreeString(xml);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)stream;
    hr = IXMLDOMDocument_save(doc, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &global);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    p = GlobalLock(global);
    ok(!memcmp(p, xml2, sizeof(xml2) - 1) || !memcmp(p, xml2_wine, sizeof(xml2_wine) - 1), "got %s\n", wine_dbgstr_a(p));
    GlobalUnlock(global);

    /* Verify the result after load+save */
    off.QuadPart = 0;
    hr = IStream_Seek(stream, off, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_load(doc, var, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    off.QuadPart = 0;
    hr = IStream_Seek(stream, off, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_save(doc, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &global);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    p = GlobalLock(global);
    ok(!memcmp(p, xml2, sizeof(xml2) - 1) || !memcmp(p, xml2_wine, sizeof(xml2_wine) - 1), "got %s\n", wine_dbgstr_a(p));
    GlobalUnlock(global);

    IStream_Release(stream);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_("<?xml version=\"1.0\" standalone=\"yes\"?>\r\n<test/>\r\n"), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)stream;
    hr = IXMLDOMDocument_save(doc, var);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &global);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    p = GlobalLock(global);
    ok(!memcmp(p, xml3, sizeof(xml3) - 1) || !memcmp(p, xml3_wine, sizeof(xml3_wine) - 1), "got %s\n", wine_dbgstr_a(p));
    GlobalUnlock(global);

    IStream_Release(stream);
    IXMLDOMDocument_Release(doc);
}

static void test_put_nodeTypedValue(void)
{
    static const BYTE binhexdata[16] =
        {0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf};
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    VARIANT type, value;
    LONG ubound, lbound;
    IXMLDOMNode *node;
    SAFEARRAY *array;
    HRESULT hr;
    BYTE *ptr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Element"), &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&type) = VT_EMPTY;
    hr = IXMLDOMElement_get_dataType(elem, &type);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&type) == VT_NULL, "got %d, expected VT_NULL\n", V_VT(&type));

    /* set typed value for untyped node */
    V_VT(&type) = VT_I1;
    V_I1(&type) = 1;
    hr = IXMLDOMElement_put_nodeTypedValue(elem, type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&type) = VT_EMPTY;
    hr = IXMLDOMElement_get_dataType(elem, &type);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&type) == VT_NULL, "got %d, expected VT_NULL\n", V_VT(&type));

    /* no type info stored */
    V_VT(&type) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&type) == VT_BSTR, "got %d, expected VT_BSTR\n", V_VT(&type));
    ok(memcmp(V_BSTR(&type), L"1", 2*sizeof(WCHAR)) == 0,
       "got %s, expected \"1\"\n", wine_dbgstr_w(V_BSTR(&type)));
    VariantClear(&type);

    /* int */
    hr = IXMLDOMElement_put_dataType(elem, _bstr_("int"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = _bstr_("1");
    hr = IXMLDOMElement_put_nodeTypedValue(elem, value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&value) == VT_I4, "got %d\n", V_VT(&value));
    ok(V_I4(&value) == 1, "got %ld\n", V_I4(&value));

    V_VT(&value) = VT_I2;
    V_I2(&value) = -1;
    hr = IXMLDOMElement_put_nodeTypedValue(elem, value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&value) == VT_I4, "got %d\n", V_VT(&value));
    ok(V_I4(&value) == -1, "got %ld\n", V_I4(&value));

    hr = IXMLDOMElement_get_firstChild(elem, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMElement_removeChild(elem, node, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)elem, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* bin.base64 */
    hr = IXMLDOMElement_put_dataType(elem, _bstr_("bin.base64"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = _bstr_("ABCD");
    hr = IXMLDOMElement_put_nodeTypedValue(elem, value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&value) == (VT_UI1|VT_ARRAY), "got %d\n", V_VT(&value));
    ok(SafeArrayGetDim(V_ARRAY(&value)) == 1, "got wrong dimension\n");
    ubound = 0;
    hr = SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ubound == 2, "got %ld\n", ubound);
    lbound = 0;
    hr = SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lbound == 0, "got %ld\n", lbound);
    hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ptr[0] == 0, "got %x\n", ptr[0]);
    ok(ptr[1] == 0x10, "got %x\n", ptr[1]);
    ok(ptr[2] == 0x83, "got %x\n", ptr[2]);
    SafeArrayUnaccessData(V_ARRAY(&value));
    VariantClear(&value);

    /* when set as VT_BSTR it's stored as is */
    hr = IXMLDOMElement_get_firstChild(elem, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_text(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"ABCD"), "%s\n", wine_dbgstr_w(str));
    IXMLDOMNode_Release(node);
    SysFreeString(str);

    array = SafeArrayCreateVector(VT_UI1, 0, 7);
    hr = SafeArrayAccessData(array, (void*)&ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memcpy(ptr, "dGVzdA=", strlen("dGVzdA="));
    SafeArrayUnaccessData(array);

    V_VT(&value) = VT_UI1|VT_ARRAY;
    V_ARRAY(&value) = array;
    hr = IXMLDOMElement_put_nodeTypedValue(elem, value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&value) == (VT_UI1|VT_ARRAY), "got %d\n", V_VT(&value));
    ok(SafeArrayGetDim(V_ARRAY(&value)) == 1, "got wrong dimension\n");
    ubound = 0;
    hr = SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ubound == 6, "got %ld\n", ubound);
    lbound = 0;
    hr = SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lbound == 0, "got %ld\n", lbound);
    hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(ptr, "dGVzdA=", strlen("dGVzdA=")), "got wrong data, %s\n", ptr);
    SafeArrayUnaccessData(V_ARRAY(&value));
    VariantClear(&value);

    /* if set with VT_UI1|VT_ARRAY it's encoded */
    hr = IXMLDOMElement_get_firstChild(elem, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_text(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"ZEdWemRBPQ=="), "%s\n", wine_dbgstr_w(str));
    IXMLDOMNode_Release(node);
    SafeArrayDestroyData(array);
    SysFreeString(str);

    /* bin.hex */
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = _bstr_("");
    hr = IXMLDOMElement_put_nodeTypedValue(elem, value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_put_dataType(elem, _bstr_("bin.hex"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    array = SafeArrayCreateVector(VT_UI1, 0, 16);
    hr = SafeArrayAccessData(array, (void*)&ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memcpy(ptr, binhexdata, sizeof(binhexdata));
    SafeArrayUnaccessData(array);

    V_VT(&value) = VT_UI1|VT_ARRAY;
    V_ARRAY(&value) = array;
    hr = IXMLDOMElement_put_nodeTypedValue(elem, value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&value) = VT_EMPTY;
    hr = IXMLDOMElement_get_nodeTypedValue(elem, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&value) == (VT_UI1|VT_ARRAY), "got %d\n", V_VT(&value));
    ok(SafeArrayGetDim(V_ARRAY(&value)) == 1, "got wrong dimension\n");
    ubound = 0;
    hr = SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ubound == 15, "got %ld\n", ubound);
    lbound = 0;
    hr = SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(lbound == 0, "got %ld\n", lbound);
    hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(ptr, binhexdata, sizeof(binhexdata)), "got wrong data\n");
    SafeArrayUnaccessData(V_ARRAY(&value));
    VariantClear(&value);

    /* if set with VT_UI1|VT_ARRAY it's encoded */
    hr = IXMLDOMElement_get_firstChild(elem, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_text(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"000102030405060708090a0b0c0d0e0f"), "%s\n", wine_dbgstr_w(str));
    IXMLDOMNode_Release(node);
    SafeArrayDestroyData(array);
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_get_xml(void)
{
    static const char xmlA[] = "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n<a>test</a>\r\n";
    static const char attrA[] = "attr=\"&quot;a &amp; b&quot;\"";
    static const char attr2A[] = "\"a & b\"";
    static const char attr3A[] = "attr=\"&amp;quot;a\"";
    static const char attr4A[] = "&quot;a";
    static const char fooA[] = "<foo/>";
    IXMLDOMProcessingInstruction *pi;
    IXMLDOMNode *first;
    IXMLDOMElement *elem = NULL;
    IXMLDOMAttribute *attr;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    VARIANT v;
    BSTR xml;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_("<a>test</a>"), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_createProcessingInstruction(doc, _bstr_("xml"),
                             _bstr_("version=\"1.0\" encoding=\"UTF-16\""), &pi);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_firstChild(doc, &first);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_UNKNOWN(&v) = (IUnknown*)first;
    V_VT(&v) = VT_UNKNOWN;

    hr = IXMLDOMDocument_insertBefore(doc, (IXMLDOMNode*)pi, v, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMProcessingInstruction_Release(pi);
    IXMLDOMNode_Release(first);

    hr = IXMLDOMDocument_get_xml(doc, &xml);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(memcmp(xml, _bstr_(xmlA), sizeof(xmlA)*sizeof(WCHAR)) == 0,
        "got %s, expected %s\n", wine_dbgstr_w(xml), xmlA);
    SysFreeString(xml);

    IXMLDOMDocument_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("foo"), &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_putref_documentElement(doc, elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_xml(doc, &xml);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(memcmp(xml, _bstr_(fooA), (sizeof(fooA)-1)*sizeof(WCHAR)) == 0,
        "got %s, expected %s\n", wine_dbgstr_w(xml), fooA);
    SysFreeString(xml);

    IXMLDOMElement_Release(elem);

    /* attribute node */
    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("attr"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("\"a & b\"");
    hr = IXMLDOMAttribute_put_value(attr, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    xml = NULL;
    hr = IXMLDOMAttribute_get_xml(attr, &xml);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(xml, _bstr_(attrA), (sizeof(attrA)-1)*sizeof(WCHAR)), "got %s\n", wine_dbgstr_w(xml));
    SysFreeString(xml);

    VariantInit(&v);
    hr = IXMLDOMAttribute_get_value(attr, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "got type %d\n", V_VT(&v));
    ok(!memcmp(V_BSTR(&v), _bstr_(attr2A), (sizeof(attr2A)-1)*sizeof(WCHAR)),
        "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("&quot;a");
    hr = IXMLDOMAttribute_put_value(attr, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    xml = NULL;
    hr = IXMLDOMAttribute_get_xml(attr, &xml);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(xml, _bstr_(attr3A), (sizeof(attr3A)-1)*sizeof(WCHAR)), "got %s\n", wine_dbgstr_w(xml));
    SysFreeString(xml);

    VariantInit(&v);
    hr = IXMLDOMAttribute_get_value(attr, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "got type %d\n", V_VT(&v));
    ok(!memcmp(V_BSTR(&v), _bstr_(attr4A), (sizeof(attr4A)-1)*sizeof(WCHAR)),
        "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    IXMLDOMAttribute_Release(attr);

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_xsltemplate(void)
{
    IXMLDOMDocument *doc, *doc2, *doc3;
    IXSLTemplate *template;
    IXSLProcessor *processor;
    IStream *stream;
    VARIANT_BOOL b;
    HRESULT hr;
    ULONG ref1, ref2;
    VARIANT v;
    BSTR str;

    if (!is_clsid_supported(&CLSID_XSLTemplate, &IID_IXSLTemplate)) return;
    template = create_xsltemplate(&IID_IXSLTemplate);

    check_interface(template, &IID_IXSLTemplate, TRUE);
    check_interface(template, &IID_IDispatch, TRUE);
    check_interface(template, &IID_IDispatchEx, TRUE);

    /* works as reset */
    hr = IXSLTemplate_putref_stylesheet(template, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    doc = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_("<a>test</a>"), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    /* putref with non-xsl document */
    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    todo_wine ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(szTransformSSXML), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    /* not a freethreaded document */
    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    todo_wine ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    IXMLDOMDocument_Release(doc);

    if (!is_clsid_supported(&CLSID_FreeThreadedDOMDocument, &IID_IXMLDOMDocument))
    {
        IXSLTemplate_Release(template);
        return;
    }

    hr = CoCreateInstance(&CLSID_FreeThreadedDOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_loadXML( doc, _bstr_(szTransformSSXML), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    /* freethreaded document */
    ref1 = IXMLDOMDocument_AddRef(doc);
    IXMLDOMDocument_Release(doc);
    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref2 = IXMLDOMDocument_AddRef(doc);
    IXMLDOMDocument_Release(doc);
    ok(ref2 > ref1, "got %ld\n", ref2);

    /* processor */
    hr = IXSLTemplate_createProcessor(template, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(template, 1);
    hr = IXSLTemplate_createProcessor(template, &processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(template, 2);

    /* input no set yet */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = NULL;
    hr = IXSLProcessor_get_input(processor, &v);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_EMPTY, "got %d\n", V_VT(&v));
}

    hr = IXSLProcessor_get_output(processor, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* reset before it was set */
    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = NULL;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_UNKNOWN;
    V_DISPATCH(&v) = NULL;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)stream;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* it seems processor grabs 2 references */
    todo_wine EXPECT_REF(stream, 3);

    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_get_output(processor, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "got type %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == (IUnknown*)stream, "got %p\n", V_UNKNOWN(&v));

    todo_wine EXPECT_REF(stream, 4);
    VariantClear(&v);

    hr = IXSLProcessor_transform(processor, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* reset and check stream refcount */
    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(stream, 1);

    IStream_Release(stream);

    /* no output interface set, check output */
    doc2 = create_document(&IID_IXMLDOMDocument);

    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML( doc2, _bstr_("<a>test</a>"), &b );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( b == VARIANT_TRUE, "got %d\n", b);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)doc2;
    hr = IXSLProcessor_put_input(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXSLProcessor_transform(processor, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_get_output(processor, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "got type %d\n", V_VT(&v));
    ok(*V_BSTR(&v) == 0, "got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* transform to document */
    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc2, _bstr_(szTransformXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)doc2;
    hr = IXSLProcessor_put_input(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    doc3 = create_document(&IID_IXMLDOMDocument);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)doc3;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_xml(doc3, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!*str, "Expected empty document\n");
    SysFreeString(str);

    hr = IXSLProcessor_transform(processor, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_get_output(processor, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "got type %d\n", V_VT(&v));
    VariantClear(&v);

    hr = IXMLDOMDocument_get_xml(doc3, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!*str, "Expected document\n");
    SysFreeString(str);

    /* transform to IResponse */
    V_VT(&v) = VT_EMPTY;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)&testresponse;
    hr = IXSLProcessor_put_output(processor, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_FALSE;
    hr = IXSLProcessor_transform(processor, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %x\n", b);

    IXSLProcessor_Release(processor);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc3);

    /* drop reference */
    hr = IXSLTemplate_putref_stylesheet(template, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ref2 = IXMLDOMDocument_AddRef(doc);
    IXMLDOMDocument_Release(doc);
    ok(ref2 == ref1, "got %ld\n", ref2);

    IXMLDOMDocument_Release(doc);
    IXSLTemplate_Release(template);
    free_bstrs();
}

static void test_insertBefore(void)
{
    IXMLDOMDocument *doc, *doc2, *doc3;
    IXMLDOMAttribute *attr;
    IXMLDOMElement *elem1, *elem2, *elem3, *elem4, *elem5;
    IXMLDOMNode *node, *newnode, *cdata;
    HRESULT hr;
    VARIANT v;
    BSTR p;

    doc = create_document(&IID_IXMLDOMDocument);
    doc3 = create_document(&IID_IXMLDOMDocument);

    /* NULL to document */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_insertBefore(doc, NULL, v, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)0xdeadbeef, "got %p\n", node);

    /* document to document */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_insertBefore(doc, (IXMLDOMNode*)doc3, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    /* document to itself */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_insertBefore(doc, (IXMLDOMNode*)doc, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    /* insertBefore behaviour for attribute node */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ATTRIBUTE;

    attr = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("attr"), NULL, (IXMLDOMNode**)&attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attr != NULL, "got %p\n", attr);

    /* attribute to document */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_insertBefore(doc3, (IXMLDOMNode*)attr, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    /* cdata to document */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_CDATA_SECTION;

    cdata = NULL;
    hr = IXMLDOMDocument_createNode(doc3, v, _bstr_("cdata"), NULL, &cdata);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cdata != NULL, "got %p\n", cdata);

    EXPECT_NO_CHILDREN(cdata);

    /* attribute to cdata */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMNode_insertBefore(cdata, (IXMLDOMNode*)attr, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    /* document to cdata */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMNode_insertBefore(cdata, (IXMLDOMNode*)doc, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_insertBefore(doc3, cdata, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    IXMLDOMNode_Release(cdata);
    IXMLDOMDocument_Release(doc3);

    /* attribute to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ATTRIBUTE;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("attr2"), NULL, &newnode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)attr;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    todo_wine ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* cdata to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_CDATA_SECTION;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* comment to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_COMMENT;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* element to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);

    /* pi to attribute */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_PROCESSING_INSTRUCTION;
    newnode = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("cdata"), NULL, &newnode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(newnode != NULL, "got %p\n", newnode);

    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMAttribute_insertBefore(attr, newnode, v, &node);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);
    IXMLDOMNode_Release(newnode);
    IXMLDOMAttribute_Release(attr);

    /* insertBefore for elements */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem2"), &elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem3"), &elem3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem3"), &elem3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem4"), &elem4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_NO_CHILDREN(elem1);
    EXPECT_NO_CHILDREN(elem2);
    EXPECT_NO_CHILDREN(elem3);

    todo_wine EXPECT_REF(elem2, 2);

    /* NULL to element */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_insertBefore(elem1, NULL, v, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)0xdeadbeef, "got %p\n", node);

    /* document to element */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)doc, v, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem4, v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)elem4, "got %p\n", node);

    EXPECT_CHILDREN(elem1);
    hr = IXMLDOMElement_removeChild(elem1, (IXMLDOMNode*)elem4, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(elem4);

    EXPECT_NO_CHILDREN(elem1);

    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)elem2, "got %p\n", node);

    EXPECT_CHILDREN(elem1);
    todo_wine EXPECT_REF(elem2, 3);
    IXMLDOMNode_Release(node);

    /* again for already linked node */
    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)elem2, "got %p\n", node);

    EXPECT_CHILDREN(elem1);

    /* increments each time */
    todo_wine EXPECT_REF(elem2, 3);
    IXMLDOMNode_Release(node);

    /* try to add to another element */
    V_VT(&v) = VT_NULL;
    node = (void*)0xdeadbeef;
    hr = IXMLDOMElement_insertBefore(elem3, (IXMLDOMNode*)elem2, v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)elem2, "got %p\n", node);

    EXPECT_CHILDREN(elem3);
    EXPECT_NO_CHILDREN(elem1);

    IXMLDOMNode_Release(node);

    /* cross document case - try to add as child to a node created with other doc */
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc2, _bstr_("elem4"), &elem4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(elem4, 2);

    /* same name, another instance */
    hr = IXMLDOMDocument_createElement(doc2, _bstr_("elem4"), &elem5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(elem5, 2);

    todo_wine EXPECT_REF(elem3, 2);
    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem3, (IXMLDOMNode*)elem4, v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)elem4, "got %p\n", node);
    todo_wine EXPECT_REF(elem4, 3);
    todo_wine EXPECT_REF(elem3, 2);
    IXMLDOMNode_Release(node);

    V_VT(&v) = VT_NULL;
    node = NULL;
    hr = IXMLDOMElement_insertBefore(elem3, (IXMLDOMNode*)elem5, v, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (void*)elem5, "got %p\n", node);
    todo_wine EXPECT_REF(elem4, 2);
    todo_wine EXPECT_REF(elem5, 3);
    IXMLDOMNode_Release(node);

    IXMLDOMDocument_Release(doc2);

    IXMLDOMElement_Release(elem1);
    IXMLDOMElement_Release(elem2);
    IXMLDOMElement_Release(elem3);
    IXMLDOMElement_Release(elem4);
    IXMLDOMElement_Release(elem5);

    /* elements with same default namespace */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem1 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem1"), _bstr_("http://winehq.org/default"), (IXMLDOMNode**)&elem1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(elem1 != NULL, "got %p\n", elem1);

    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem2 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem2"), _bstr_("http://winehq.org/default"), (IXMLDOMNode**)&elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(elem2 != NULL, "got %p\n", elem2);

    /* check contents so far */
    p = NULL;
    hr = IXMLDOMElement_get_xml(elem1, &p);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(p, L"<elem1 xmlns=\"http://winehq.org/default\"/>"), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(p, L"<elem2 xmlns=\"http://winehq.org/default\"/>"), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    V_VT(&v) = VT_NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* get_xml depends on context, for top node it omits child namespace attribute,
       but at child level it's still returned */
    p = NULL;
    hr = IXMLDOMElement_get_xml(elem1, &p);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(!lstrcmpW(p, L"<elem1 xmlns=\"http://winehq.org/default\"><elem2/></elem1>"),
        "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(p, L"<elem2 xmlns=\"http://winehq.org/default\"/>"), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    IXMLDOMElement_Release(elem1);
    IXMLDOMElement_Release(elem2);

    /* child without default namespace added to node with default namespace */
    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem1 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem1"), _bstr_("http://winehq.org/default"), (IXMLDOMNode**)&elem1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(elem1 != NULL, "got %p\n", elem1);

    V_VT(&v) = VT_I4;
    V_I4(&v) = NODE_ELEMENT;
    elem2 = NULL;
    hr = IXMLDOMDocument_createNode(doc, v, _bstr_("elem2"), NULL, (IXMLDOMNode**)&elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(elem2 != NULL, "got %p\n", elem2);

    EXPECT_REF(elem2, 1);
    V_VT(&v) = VT_NULL;
    hr = IXMLDOMElement_insertBefore(elem1, (IXMLDOMNode*)elem2, v, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(elem2, 1);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(p, L"<elem2/>"), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    hr = IXMLDOMElement_removeChild(elem1, (IXMLDOMNode*)elem2, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    p = NULL;
    hr = IXMLDOMElement_get_xml(elem2, &p);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(p, L"<elem2/>"), "got %s\n", wine_dbgstr_w(p));
    SysFreeString(p);

    IXMLDOMElement_Release(elem1);
    IXMLDOMElement_Release(elem2);
    IXMLDOMDocument_Release(doc);
}

static void test_appendChild(void)
{
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMElement *elem, *elem2;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("elem"), &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(doc2, _bstr_("elem2"), &elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(doc, 1);
    todo_wine EXPECT_REF(elem, 2);
    EXPECT_REF(doc2, 1);
    todo_wine EXPECT_REF(elem2, 2);
    EXPECT_NO_CHILDREN(doc);
    EXPECT_NO_CHILDREN(doc2);

    hr = IXMLDOMDocument_appendChild(doc2, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_appendChild(elem, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* append from another document */
    hr = IXMLDOMDocument_appendChild(doc2, (IXMLDOMNode*)elem, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(doc, 1);
    todo_wine EXPECT_REF(elem, 2);
    EXPECT_REF(doc2, 1);
    todo_wine EXPECT_REF(elem2, 2);
    EXPECT_NO_CHILDREN(doc);
    EXPECT_CHILDREN(doc2);

    IXMLDOMElement_Release(elem);
    IXMLDOMElement_Release(elem2);
    IXMLDOMDocument_Release(doc);
    IXMLDOMDocument_Release(doc2);
}

static void test_get_doctype(void)
{
    IXMLDOMDocumentType *doctype;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR s;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_doctype(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    doctype = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_doctype(doc, &doctype);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(doctype == NULL, "got %p\n", doctype);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szEmailXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML string\n");

    doctype = NULL;
    hr = IXMLDOMDocument_get_doctype(doc, &doctype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(doctype != NULL, "got %p\n", doctype);

    hr = IXMLDOMDocumentType_get_name(doctype, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocumentType_get_name(doctype, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"email", s), "got name %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    hr = IXMLDOMDocumentType_get_nodeName(doctype, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"email", s), "got name %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    IXMLDOMDocumentType_Release(doctype);
    IXMLDOMDocument_Release(doc);
}

static void test_get_tagName(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem, *elem2;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("element"), &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_tagName(elem, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IXMLDOMElement_get_tagName(elem, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"element"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("s:element"), &elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IXMLDOMElement_get_tagName(elem2, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"s:element"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMElement_Release(elem2);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct {
    DOMNodeType type;
    const char *name;
    VARTYPE vt;
    HRESULT hr;
} node_type_t;

static const node_type_t get_datatype[] = {
    { NODE_ELEMENT,                "element",   VT_NULL, S_FALSE },
    { NODE_ATTRIBUTE,              "attr",      VT_NULL, S_FALSE },
    { NODE_TEXT,                   "text",      VT_NULL, S_FALSE },
    { NODE_CDATA_SECTION ,         "cdata",     VT_NULL, S_FALSE },
    { NODE_ENTITY_REFERENCE,       "entityref", VT_NULL, S_FALSE },
    { NODE_PROCESSING_INSTRUCTION, "pi",        VT_NULL, S_FALSE },
    { NODE_COMMENT,                "comment",   VT_NULL, S_FALSE },
    { NODE_DOCUMENT_FRAGMENT,      "docfrag",   VT_NULL, S_FALSE },
    { 0 }
};

static void test_get_dataType(void)
{
    const node_type_t *entry = get_datatype;
    IXMLDOMDocument *doc;

    doc = create_document(&IID_IXMLDOMDocument);

    while (entry->type)
    {
        IXMLDOMNode *node = NULL;
        VARIANT var, type;
        HRESULT hr;

        V_VT(&var) = VT_I4;
        V_I4(&var) = entry->type;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_(entry->name), NULL, &node);
        ok(hr == S_OK, "failed to create node, type %d\n", entry->type);

        hr = IXMLDOMNode_get_dataType(node, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        VariantInit(&type);
        hr = IXMLDOMNode_get_dataType(node, &type);
        ok(hr == entry->hr, "Unexpected hr %#lx, expected %#lx. node type %d\n",
            hr, entry->hr, entry->type);
        ok(V_VT(&type) == entry->vt, "got %d, expected %d. node type %d\n",
            V_VT(&type), entry->vt, entry->type);
        VariantClear(&type);

        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct _get_node_typestring_t {
    DOMNodeType type;
    const char *string;
} get_node_typestring_t;

static const get_node_typestring_t get_node_typestring[] = {
    { NODE_ELEMENT,                "element"               },
    { NODE_ATTRIBUTE,              "attribute"             },
    { NODE_TEXT,                   "text"                  },
    { NODE_CDATA_SECTION ,         "cdatasection"          },
    { NODE_ENTITY_REFERENCE,       "entityreference"       },
    { NODE_PROCESSING_INSTRUCTION, "processinginstruction" },
    { NODE_COMMENT,                "comment"               },
    { NODE_DOCUMENT_FRAGMENT,      "documentfragment"      },
    { 0 }
};

static void test_get_nodeTypeString(void)
{
    const get_node_typestring_t *entry = get_node_typestring;
    IXMLDOMDocument *doc;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_nodeTypeString(doc, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"document"), "got string %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    while (entry->type)
    {
        IXMLDOMNode *node = NULL;
        VARIANT var;

        V_VT(&var) = VT_I4;
        V_I4(&var) = entry->type;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_("node"), NULL, &node);
        ok(hr == S_OK, "failed to create node, type %d\n", entry->type);

        hr = IXMLDOMNode_get_nodeTypeString(node, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNode_get_nodeTypeString(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, _bstr_(entry->string)), "got string %s, expected %s. node type %d\n",
            wine_dbgstr_w(str), entry->string, entry->type);
        SysFreeString(str);
        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct _get_attributes_t {
    DOMNodeType type;
    HRESULT hr;
} get_attributes_t;

static const get_attributes_t get_attributes[] = {
    { NODE_ATTRIBUTE,              S_FALSE },
    { NODE_TEXT,                   S_FALSE },
    { NODE_CDATA_SECTION ,         S_FALSE },
    { NODE_ENTITY_REFERENCE,       S_FALSE },
    { NODE_PROCESSING_INSTRUCTION, S_FALSE },
    { NODE_COMMENT,                S_FALSE },
    { NODE_DOCUMENT_FRAGMENT,      S_FALSE },
    { 0 }
};

static void test_get_attributes(void)
{
    const WCHAR *namespaces[] =
    {
        L"xmlns:oslc_am",
        L"xmlns:rdf",
        L"xmlns:dcterms",
        L"xmlns:foaf"
    };
    const get_attributes_t *entry = get_attributes;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMDocument *doc, *doc2;
    IXMLDOMNode *node, *node2;
    IXMLDOMElement *elem;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;
    BSTR str;
    LONG length, i;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(complete4A), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_attributes(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    map = (void*)0xdeadbeef;
    hr = IXMLDOMDocument_get_attributes(doc, &map);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(map == NULL, "got %p\n", map);

    /* first child is <?xml ?> */
    hr = IXMLDOMDocument_get_firstChild(doc, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_attributes(node, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    node2 = (void*)0xdeadbeef;
    hr = IXMLDOMNamedNodeMap_getNamedItem(map, _bstr_("attr"), &node2);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(node2 == NULL, "got %p\n", node2);

    length = -1;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(length == 1, "got %ld\n", length);

    if (hr == S_OK && length == 1)
    {
        IXMLDOMAttribute *attr;
        DOMNodeType type;
        VARIANT v;

        node2 = NULL;
        hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(node != NULL, "got %p\n", node2);

        hr = IXMLDOMNode_get_nodeName(node2, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"version"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 1, "got %ld\n", length);

        type = -1;
        hr = IXMLDOMNode_get_nodeType(node2, &type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(type == NODE_ATTRIBUTE, "got %d\n", type);

        hr = IXMLDOMNode_get_xml(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"<?xml version=\"1.0\"?>"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_text(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"version=\"1.0\""), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNamedNodeMap_removeNamedItem(map, _bstr_("version"), NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 0, "got %ld\n", length);

        hr = IXMLDOMNode_get_xml(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"<?xml version=\"1.0\"?>"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_text(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L""), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMNamedNodeMap_Release(map);

        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 0, "got %ld\n", length);

        hr = IXMLDOMDocument_createAttribute(doc, _bstr_("encoding"), &attr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = _bstr_("UTF-8");
        hr = IXMLDOMAttribute_put_nodeValue(attr, v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        EXPECT_REF(attr, 2);
        hr = IXMLDOMNamedNodeMap_setNamedItem(map, (IXMLDOMNode*)attr, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        EXPECT_REF(attr, 2);

        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        length = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 1, "got %ld\n", length);

        hr = IXMLDOMNode_get_xml(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"<?xml version=\"1.0\"?>"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        hr = IXMLDOMNode_get_text(node, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, L"encoding=\"UTF-8\""), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMNamedNodeMap_Release(map);
        IXMLDOMNode_Release(node2);
    }

    IXMLDOMNode_Release(node);

    /* last child is element */
    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_get_lastChild(doc, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(doc, 1);

    EXPECT_REF(node, 1);
    hr = IXMLDOMNode_get_attributes(node, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node, 1);
    EXPECT_REF(doc, 1);

    EXPECT_REF(map, 1);
    hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node, 1);
    EXPECT_REF(node2, 1);
    EXPECT_REF(map, 1);
    EXPECT_REF(doc, 1);
    IXMLDOMNode_Release(node2);

    /* release node before map release, map still works */
    IXMLDOMNode_Release(node);

    length = 0;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 1, "got %ld\n", length);

    node2 = NULL;
    hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(node2, 1);
    IXMLDOMNode_Release(node2);

    IXMLDOMNamedNodeMap_Release(map);

    /* append created element a different document, map still works */
    hr = IXMLDOMDocument_createElement(doc, _bstr_("test"), &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 1;
    hr = IXMLDOMElement_setAttribute(elem, _bstr_("testattr"), v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_attributes(elem, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    length = 0;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 1, "got %ld\n", length);

    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_appendChild(doc2, (IXMLDOMNode*)elem, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == (IXMLDOMNode*)elem, "node != elem\n");
    IXMLDOMNode_Release(node);
    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc2);

    length = 0;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 1, "got %ld\n", length);

    IXMLDOMNamedNodeMap_Release(map);

    while (entry->type)
    {
        VARIANT var;

        node = NULL;

        V_VT(&var) = VT_I4;
        V_I4(&var) = entry->type;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_("node"), NULL, &node);
        ok(hr == S_OK, "failed to create node, type %d\n", entry->type);

        hr = IXMLDOMNode_get_attributes(node, NULL);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        map = (void*)0xdeadbeef;
        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == entry->hr, "Unexpected hr %#lx, expected %#lx. node type %d\n",
            hr, entry->hr, entry->type);
        ok(map == NULL, "got %p\n", map);

        IXMLDOMNode_Release(node);

        entry++;
    }

    IXMLDOMDocument_Release(doc);

    str = SysAllocString(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                L"<rdf:RDF xmlns:oslc_am=\"http://open-services.net/ns/am#\""
                L"         xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\""
                L"         xmlns:dcterms=\"http://purl.org/dc/terms/\""
                L"         xmlns:foaf=\"http://xmlns.com/foaf/0.1/\" >"
                L"</rdf:RDF>");

    doc = create_document(&IID_IXMLDOMDocument2);

    hr = IXMLDOMDocument_loadXML(doc, str, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_attributes(elem, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    length = -1;
    hr = IXMLDOMNamedNodeMap_get_length(map, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 4, "length %#lx.\n", length);

    for(i=0; i < length; i++)
    {
        hr = IXMLDOMNamedNodeMap_get_item(map, i, &node2);
        ok( hr == S_OK, "Unexpected hr %#lx (%ld).\n", hr, i);

        hr = IXMLDOMNode_get_nodeName(node2, &str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(str, namespaces[i]), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IXMLDOMNode_Release(node2);
    }

    IXMLDOMNamedNodeMap_Release(map);
    IXMLDOMElement_Release(elem);

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void test_selection(void)
{
    IXMLDOMSelection *selection, *selection2;
    IEnumVARIANT *enum1, *enum2, *enum3;
    IXMLDOMNodeList *list;
    IUnknown *unk1, *unk2;
    IXMLDOMDocument *doc;
    IDispatchEx *dispex;
    IXMLDOMNode *node;
    IDispatch *disp;
    VARIANT_BOOL b;
    VARIANT v[3];
    HRESULT hr;
    DISPID did;
    BSTR name;
    ULONG ret;
    LONG len;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void**)&selection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMSelection_Release(selection);

    /* collection disp id */
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE, "got %ld\n", did);
    len = 0;
    hr = IXMLDOMSelection_get_length(selection, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 1, "got %ld\n", len);
    hr = IDispatchEx_GetDispID(dispex, _bstr_("10"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE+10, "got %ld\n", did);
    IDispatchEx_Release(dispex);

    /* IEnumVARIANT tests */
    enum1 = NULL;
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IEnumVARIANT, (void**)&enum1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum1 != NULL, "got %p\n", enum1);
    EXPECT_REF(enum1, 2);

    EXPECT_REF(selection, 1);
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IUnknown, (void**)&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(selection, 2);
    EXPECT_REF(enum1, 2);

    /* enumerator and selection object return same IUnknown* */
    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(selection, 3);
    EXPECT_REF(enum1, 2);
    ok(unk2 == unk1, "got %p, %p\n", unk1, unk2);
    IUnknown_Release(unk2);

    EXPECT_REF(selection, 2);
    IEnumVARIANT_AddRef(enum1);
    EXPECT_REF(selection, 2);
    IEnumVARIANT_Release(enum1);

    enum3 = NULL;
    hr = IXMLDOMSelection_QueryInterface(selection, &IID_IEnumVARIANT, (void**)&enum3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum3 != NULL, "got %p\n", enum3);
    ok(enum1 == enum3, "got %p and %p\n", enum1, enum3);
    EXPECT_REF(enum1, 3);
    IEnumVARIANT_Release(enum3);

    EXPECT_REF(selection, 2);
    EXPECT_REF(enum1, 2);

    enum2 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum2 != NULL, "got %p\n", enum2);

    EXPECT_REF(selection, 3);
    EXPECT_REF(enum1, 2);
    EXPECT_REF(enum2, 1);

    ok(enum1 != enum2, "got %p, %p\n", enum1, enum2);

    hr = IEnumVARIANT_QueryInterface(enum2, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(selection, 3);
    EXPECT_REF(enum2, 2);
    ok(unk2 != unk1, "got %p, %p\n", unk1, unk2);
    IUnknown_Release(unk2);
    IUnknown_Release(unk1);

    selection2 = NULL;
    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IXMLDOMSelection, (void**)&selection2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(selection2 == selection, "got %p and %p\n", selection, selection2);
    EXPECT_REF(selection, 3);
    EXPECT_REF(enum1, 2);

    IXMLDOMSelection_Release(selection2);

    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IDispatch, (void**)&disp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(selection, 3);
    IDispatch_Release(disp);

    hr = IEnumVARIANT_QueryInterface(enum1, &IID_IEnumVARIANT, (void**)&enum3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum3 == enum1, "got %p and %p\n", enum3, enum1);
    EXPECT_REF(selection, 2);
    EXPECT_REF(enum1, 3);

    IEnumVARIANT_Release(enum1);
    IEnumVARIANT_Release(enum2);
    IEnumVARIANT_Release(enum3);

    enum1 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum1 != NULL, "got %p\n", enum1);
    EXPECT_REF(enum1, 1);
    EXPECT_REF(selection, 2);

    enum2 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum2 != NULL, "got %p\n", enum2);
    EXPECT_REF(enum2, 1);
    EXPECT_REF(selection, 3);

    ok(enum1 != enum2, "got %p, %p\n", enum1, enum2);

    IEnumVARIANT_AddRef(enum1);
    EXPECT_REF(selection, 3);
    EXPECT_REF(enum1, 2);
    EXPECT_REF(enum2, 1);
    IEnumVARIANT_Release(enum1);

    IEnumVARIANT_Release(enum1);
    IEnumVARIANT_Release(enum2);

    EXPECT_REF(selection, 1);

    IXMLDOMNodeList_Release(list);

    hr = IXMLDOMDocument_get_childNodes(doc, &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void**)&selection);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    IXMLDOMNodeList_Release(list);

    /* test if IEnumVARIANT touches selection context */
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(xpath_simple_list), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root/*"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_QueryInterface(list, &IID_IXMLDOMSelection, (void**)&selection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 0;
    hr = IXMLDOMSelection_get_length(selection, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 4, "got %ld\n", len);

    enum1 = NULL;
    hr = IXMLDOMSelection_get__newEnum(selection, (IUnknown**)&enum1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* no-op if zero count */
    V_VT(&v[0]) = VT_I2;
    hr = IEnumVARIANT_Next(enum1, 0, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_I2, "got var type %d\n", V_VT(&v[0]));

    /* positive count, null array pointer */
    hr = IEnumVARIANT_Next(enum1, 1, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IEnumVARIANT_Next(enum1, 1, NULL, &ret);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(ret == 0, "got %ld\n", ret);

    V_VT(&v[0]) = VT_I2;
    hr = IEnumVARIANT_Next(enum1, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));

    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"a"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);
    VariantClear(&v[0]);

    /* list cursor is updated */
    hr = IXMLDOMSelection_nextNode(selection, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"c"), "got node name %s\n", wine_dbgstr_w(name));
    IXMLDOMNode_Release(node);
    SysFreeString(name);

    V_VT(&v[0]) = VT_I2;
    hr = IEnumVARIANT_Next(enum1, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"b"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);
    VariantClear(&v[0]);

    hr = IXMLDOMSelection_nextNode(selection, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"d"), "got node name %s\n", wine_dbgstr_w(name));
    IXMLDOMNode_Release(node);
    SysFreeString(name);

    hr = IEnumVARIANT_Reset(enum1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* getting multiple elements */
    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum1, 2, v, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ret == 2, "got %ld, expected 2\n", ret);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[1]));

    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"a"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);

    hr = IDispatch_QueryInterface(V_DISPATCH(&v[1]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(!lstrcmpW(name, L"b"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);

    VariantClear(&v[1]);
    VariantClear(&v[0]);

    /* IEnumVARIANT_Next makes the IXMLDOMSelection cursor advance one step more */
    hr = IXMLDOMSelection_nextNode(selection, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"d"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);

    /* The IEnumVARIANT cursor is still at position '2', */
    /* therefore attempting to fetch 4 elements yields 'c' and 'd' */
    V_VT(&v[2]) = VT_NULL;
    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum1, 4, v, &ret);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(ret == 2, "got %ld, expected 2\n", ret);

    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"c"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);

    ok(V_VT(&v[1]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[1]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[1]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &name);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(name, L"d"), "got node name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
    IXMLDOMNode_Release(node);

    ok(V_VT(&v[2]) == VT_EMPTY, "got var type %d, expected %d (VT_EMPTY)\n", V_VT(&v[2]), VT_EMPTY);

    VariantClear(&v[1]);
    VariantClear(&v[0]);

    IEnumVARIANT_Release(enum1);

    IXMLDOMSelection_Release(selection);
    IXMLDOMNodeList_Release(list);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static void write_to_file(const char *name, const char *data)
{
    DWORD written;
    HANDLE hfile;
    BOOL ret;

    hfile = CreateFileA(name, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create test file: %s\n", name);

    ret = WriteFile(hfile, data, strlen(data), &written, NULL);
    ok(ret, "WriteFile failed: %s, %ld\n", name, GetLastError());

    CloseHandle(hfile);
}

#define TEST_DOC_LOAD_FROM_PATH(doc,path,expected_hr, expected_ret) \
    _test_doc_load_from_path(doc, path, expected_hr, expected_ret, __LINE__)
static void _test_doc_load_from_path(IXMLDOMDocument *doc, const char *path,
        HRESULT expected_hr, VARIANT_BOOL expected_ret, int line)
{
    IXMLDOMDocument *doc2;
    IXMLDOMElement *elem;
    BSTR url, url2;
    VARIANT_BOOL b;
    VARIANT src;
    HRESULT hr;

    url = _bstr_(path);

    V_VT(&src) = VT_BSTR;
    V_BSTR(&src) = url;
    hr = IXMLDOMDocument_load(doc, src, &b);
    ok_(__FILE__, line)(hr == expected_hr, "Failed to load document, %#lx.\n", hr);
    ok_(__FILE__, line)(b == expected_ret, "got %d\n", b);

    V_VT(&src) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&src) = &url;
    hr = IXMLDOMDocument_load(doc, src, &b);
    ok_(__FILE__, line)(hr == expected_hr, "Failed to load document, %#lx.\n", hr);
    ok_(__FILE__, line)(b == expected_ret, "got %d\n", b);

    if (expected_hr != S_OK)
        return;

    url = NULL;
    hr = IXMLDOMDocument_get_url(doc, &url);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get document url, hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Create another instance for the same document, check url */
    hr = IXMLDOMElement_get_ownerDocument(elem, &doc2);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get owner document, hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_url(doc2, &url2);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get document url, hr %#lx.\n", hr);
    ok_(__FILE__, line)(!lstrcmpW(url, url2), "Unexpected url %s.\n", wine_dbgstr_w(url2));

    IXMLDOMDocument_Release(doc2);
    IXMLDOMElement_Release(elem);
    SysFreeString(url2);
    SysFreeString(url);
}

static void url_forward_slash(char *url)
{
    char *p = url;

    while (*p)
    {
        if (*p == '\\')
            *p = '/';
        p++;
    }
}

static void test_load(void)
{
    char path[MAX_PATH], path2[MAX_PATH];
    IXMLDOMNodeList *list;
    IXMLDOMDocument *doc;
    BSTR bstr1, bstr2;
    IStream *stream;
    VARIANT_BOOL b;
    VARIANT src;
    HRESULT hr;
    void* ptr;
    int n;
    struct encoding_test
    {
        const char *xml;
        HRESULT expected_hr;
        VARIANT_BOOL expected_ret;
    } encoding_tests[] =
    {
        { gbkxml,        S_OK,    VARIANT_TRUE  },
        { iso8859_1_xml, S_OK,    VARIANT_TRUE  },
        { win1252xml,    S_OK,    VARIANT_TRUE  },
        { win936xml,     S_FALSE, VARIANT_FALSE },
    };


    GetTempPathA(MAX_PATH, path);
    strcat(path, "winetest.xml");

    /* prepare a file */
    write_to_file(path, win1252xml);

    doc = create_document(&IID_IXMLDOMDocument);

    /* null pointer as input */
    V_VT(&src) = VT_UNKNOWN;
    V_UNKNOWN(&src) = NULL;
    hr = IXMLDOMDocument_load(doc, src, &b);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    /* "file://" url */
    strcpy(path2, "file://");
    strcat(path2, path);
    TEST_DOC_LOAD_FROM_PATH(doc, path2, S_OK, VARIANT_TRUE);

    /* file:// url, forward slashes */
    url_forward_slash(path2);
    TEST_DOC_LOAD_FROM_PATH(doc, path2, S_OK, VARIANT_TRUE);

    /* "file:/" url */
    strcpy(path2, "file:/");
    strcat(path2, path);
    TEST_DOC_LOAD_FROM_PATH(doc, path, S_OK, VARIANT_TRUE);

    /* file:/ with forward slashes. */
    url_forward_slash(path2);
    TEST_DOC_LOAD_FROM_PATH(doc, path2, S_OK, VARIANT_TRUE);

    /* Regular local path. */
    TEST_DOC_LOAD_FROM_PATH(doc, path, S_OK, VARIANT_TRUE);

    /* load from a path: VT_BSTR|VT_BYREF, null ptr */
    V_VT(&src) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&src) = NULL;
    hr = IXMLDOMDocument_load(doc, src, &b);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    bstr1 = NULL;
    hr = IXMLDOMDocument_get_url(doc, &bstr1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(bstr1);

    /* Regular local path with some URL encoded characters. */
    strcpy(path2, path);
    n = strlen(path2);
    strcpy(&path2[n-1], "%6C");  /* C:\path\to\winetest.xm%6C */
    TEST_DOC_LOAD_FROM_PATH(doc, path2, S_OK, VARIANT_TRUE);

    /* Both spaces and %20s work. */
    GetTempPathA(MAX_PATH, path2);
    strcat(path2, "wine test.xml");
    write_to_file(path2, win1252xml);
    TEST_DOC_LOAD_FROM_PATH(doc, path2, S_OK, VARIANT_TRUE);
    GetTempPathA(MAX_PATH, path2);
    strcat(path2, "wine%20test.xml");
    TEST_DOC_LOAD_FROM_PATH(doc, path2, S_OK, VARIANT_TRUE);
    DeleteFileA(path2);

    DeleteFileA(path);

    /* load from existing path, no xml content */
    write_to_file(path, nocontent);

    V_VT(&src) = VT_BSTR;
    V_BSTR(&src) = _bstr_(path);
    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_load(doc, src, &b);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    bstr1 = (void *)0xdeadbeef;
    hr = IXMLDOMDocument_get_url(doc, &bstr1);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(bstr1 == NULL, "got %p\n", bstr1);

    DeleteFileA(path);
    IXMLDOMDocument_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("//*"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    bstr1 = _bstr_(list_to_string(list));

    hr = IXMLDOMNodeList_reset(list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMDocument_Release(doc);

    doc = create_document(&IID_IXMLDOMDocument);

    VariantInit(&src);
    V_ARRAY(&src) = SafeArrayCreateVector(VT_UI1, 0, lstrlenA(szExampleXML));
    V_VT(&src) = VT_ARRAY|VT_UI1;
    ok(V_ARRAY(&src) != NULL, "SafeArrayCreateVector() returned NULL\n");
    ptr = NULL;
    hr = SafeArrayAccessData(V_ARRAY(&src), &ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "SafeArrayAccessData() returned NULL\n");

    memcpy(ptr, szExampleXML, lstrlenA(szExampleXML));
    hr = SafeArrayUnlock(V_ARRAY(&src));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_load(doc, src, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("//*"), &list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    bstr2 = _bstr_(list_to_string(list));

    hr = IXMLDOMNodeList_reset(list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(lstrcmpW(bstr1, bstr2) == 0, "strings not equal: %s : %s\n",
       wine_dbgstr_w(bstr1), wine_dbgstr_w(bstr2));

    IXMLDOMDocument_Release(doc);
    IXMLDOMNodeList_Release(list);
    VariantClear(&src);

    /* UTF-16 isn't accepted */
    doc = create_document(&IID_IXMLDOMDocument);

    V_ARRAY(&src) = SafeArrayCreateVector(VT_UI1, 0, lstrlenW(szComplete1) * sizeof(WCHAR));
    V_VT(&src) = VT_ARRAY|VT_UI1;
    ok(V_ARRAY(&src) != NULL, "SafeArrayCreateVector() returned NULL\n");
    ptr = NULL;
    hr = SafeArrayAccessData(V_ARRAY(&src), &ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "SafeArrayAccessData() returned NULL\n");

    memcpy(ptr, szComplete1, lstrlenW(szComplete1) * sizeof(WCHAR));
    hr = SafeArrayUnlock(V_ARRAY(&src));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_load(doc, src, &b);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    VariantClear(&src);

    /* it doesn't like it as a VT_ARRAY|VT_UI2 either */
    V_ARRAY(&src) = SafeArrayCreateVector(VT_UI2, 0, lstrlenW(szComplete1));
    V_VT(&src) = VT_ARRAY|VT_UI2;
    ok(V_ARRAY(&src) != NULL, "SafeArrayCreateVector() returned NULL\n");
    ptr = NULL;
    hr = SafeArrayAccessData(V_ARRAY(&src), &ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "SafeArrayAccessData() returned NULL\n");

    memcpy(ptr, szComplete1, lstrlenW(szComplete1) * sizeof(WCHAR));
    hr = SafeArrayUnlock(V_ARRAY(&src));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_load(doc, src, &b);
    todo_wine ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);

    VariantClear(&src);

    /* test istream with empty content */
    stream = SHCreateMemStream((const BYTE*)nocontent, strlen(nocontent));
    V_VT(&src) = VT_UNKNOWN;
    V_UNKNOWN(&src) = (IUnknown*)stream;
    b = VARIANT_TRUE;
    hr = IXMLDOMDocument_load(doc, src, &b);
    todo_wine ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "got %d\n", b);
    EXPECT_PARSE_ERROR(doc, XML_E_INVALIDATROOTLEVEL, TRUE);
    VariantClear(&src);

    /* test istream with valid xml */
    stream = SHCreateMemStream((const BYTE*)complete4A, strlen(complete4A));
    V_VT(&src) = VT_UNKNOWN;
    V_UNKNOWN(&src) = (IUnknown*)stream;
    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_load(doc, src, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);
    EXPECT_PARSE_ERROR(doc, S_OK, FALSE);
    VariantClear(&src);

    IXMLDOMDocument_Release(doc);

    /* Encoding tests. */
    for (n = 0; n < ARRAY_SIZE(encoding_tests); n++)
    {
        GetTempPathA(MAX_PATH, path);
        strcat(path, "codepage_test.xml");
        write_to_file(path, encoding_tests[n].xml);
        doc = create_document(&IID_IXMLDOMDocument);
        TEST_DOC_LOAD_FROM_PATH(doc, path, encoding_tests[n].expected_hr, encoding_tests[n].expected_ret);
        DeleteFileA(path);
        IXMLDOMDocument_Release(doc);
    }

    free_bstrs();
}

static void test_domobj_dispex(IUnknown *obj)
{
    DISPID dispid = DISPID_XMLDOM_NODELIST_RESET;
    IDispatchEx *dispex;
    IUnknown *unk;
    DWORD props;
    UINT ticnt;
    HRESULT hr;
    BSTR name;

    hr = IUnknown_QueryInterface(obj, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    ticnt = 0;
    hr = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ticnt == 1, "ticnt=%u\n", ticnt);

    name = SysAllocString(L"*");
    hr = IDispatchEx_DeleteMemberByName(dispex, name, fdexNameCaseSensitive);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    hr = IDispatchEx_DeleteMemberByDispID(dispex, dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    props = 0;
    hr = IDispatchEx_GetMemberProperties(dispex, dispid, grfdexPropCanAll, &props);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(props == 0, "expected 0 got %ld\n", props);

    hr = IDispatchEx_GetMemberName(dispex, dispid, &name);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) SysFreeString(name);

    hr = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, DISPID_XMLDOM_NODELIST_RESET, &dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    unk = (IUnknown*)0xdeadbeef;
    hr = IDispatchEx_GetNameSpaceParent(dispex, &unk);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown*)0xdeadbeef, "got %p\n", unk);

    name = SysAllocString(L"testprop");
    hr = IDispatchEx_GetDispID(dispex, name, fdexNameEnsure, &dispid);
    ok(hr == DISP_E_UNKNOWNNAME, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    IDispatchEx_Release(dispex);
}

static void test_mxnamespacemanager(void)
{
    static const char xmluriA[] = "http://www.w3.org/XML/1998/namespace";
    IMXNamespacePrefixes *prefixes;
    IVBMXNamespaceManager *mgr2;
    IMXNamespaceManager *nsmgr;
    IUnknown *unk1, *unk2;
    WCHAR buffW[250];
    IUnknown *unk;
    HRESULT hr;
    INT len;

    hr = CoCreateInstance(&CLSID_MXNamespaceManager40, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXNamespaceManager, (void**)&nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(nsmgr, &IID_IDispatch, TRUE);
    check_interface(nsmgr, &IID_IDispatchEx, TRUE);
    check_interface(nsmgr, &IID_IMXNamespaceManager, TRUE);
    check_interface(nsmgr, &IID_IVBMXNamespaceManager, TRUE);

    hr = IMXNamespaceManager_QueryInterface(nsmgr, &IID_IVBMXNamespaceManager, (void**)&mgr2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(nsmgr, 2);
    EXPECT_REF(mgr2, 2);
    prefixes = NULL;
    hr = IVBMXNamespaceManager_getDeclaredPrefixes(mgr2, &prefixes);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        IDispatchEx *dispex;
        VARIANT arg, ret;
        DISPPARAMS dispparams;

        ok(prefixes != NULL, "got %p\n", prefixes);
        EXPECT_REF(nsmgr, 2);
        EXPECT_REF(mgr2, 2);
        EXPECT_REF(prefixes, 1);

        IVBMXNamespaceManager_QueryInterface(mgr2, &IID_IUnknown, (void**)&unk1);
        IMXNamespacePrefixes_QueryInterface(prefixes, &IID_IUnknown, (void**)&unk2);

        EXPECT_REF(mgr2, 3);
        EXPECT_REF(prefixes, 2);

        IUnknown_Release(unk1);
        IUnknown_Release(unk2);

        hr = IMXNamespacePrefixes_QueryInterface(prefixes, &IID_IDispatchEx, (void**)&dispex);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&arg) = VT_I4;
        V_I4(&arg) = 0;
        dispparams.cArgs = 1;
        dispparams.cNamedArgs = 0;
        dispparams.rgdispidNamedArgs = NULL;
        dispparams.rgvarg = &arg;

        V_VT(&ret) = VT_EMPTY;
        V_DISPATCH(&ret) = (void*)0x1;
        hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&ret) == VT_BSTR, "got %d\n", V_VT(&ret));
        ok(V_BSTR(&ret) != NULL, "got %p\n", V_BSTR(&ret));
        VariantClear(&ret);

        IDispatchEx_Release(dispex);
        IMXNamespacePrefixes_Release(prefixes);
    }
    IVBMXNamespaceManager_Release(mgr2);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* prefix already added */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri"));
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns0"), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* "xml" and "xmlns" are not allowed here */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("xml"), _bstr_("uri1"));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("xmlns"), _bstr_("uri1"));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
todo_wine {
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, -1, NULL, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
}
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, NULL, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 3, "got %d\n", len);

    len = -1;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    ok(hr == E_XML_BUFFERTOOSMALL, "Unexpected hr %#lx.\n", hr);
    ok(len == -1, "got %d\n", len);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);

    len = 10;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 3, "got %d\n", len);
    ok(!lstrcmpW(buffW, L"xml"), "got prefix %s\n", wine_dbgstr_w(buffW));

    /* getURI */
    hr = IMXNamespaceManager_getURI(nsmgr, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IMXNamespaceManager_getURI(nsmgr, NULL, NULL, NULL, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(len == -1, "got %d\n", len);

    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, NULL, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* length of "xml" uri is constant */
    ok(len == strlen(xmluriA), "got %d\n", len);

    len = 100;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == strlen(xmluriA), "got %d\n", len);
    ok(!lstrcmpW(buffW, _bstr_(xmluriA)), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = strlen(xmluriA)-1;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml"), NULL, buffW, &len);
    ok(hr == E_XML_BUFFERTOOSMALL, "Unexpected hr %#lx.\n", hr);
    ok(len == strlen(xmluriA)-1, "got %d\n", len);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);

    /* prefix xml1 not defined */
    len = -1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml1"), NULL, NULL, &len);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_("xml1"), NULL, buffW, &len);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0, "got %x\n", buffW[0]);
    ok(len == 0, "got %d\n", len);

    /* IDispatchEx tests */
    hr = IMXNamespaceManager_QueryInterface(nsmgr, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);

    IMXNamespaceManager_Release(nsmgr);

    /* ::getPrefix() */
    hr = CoCreateInstance(&CLSID_MXNamespaceManager40, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXNamespaceManager, (void**)&nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_getPrefix(nsmgr, NULL, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, NULL, 0, NULL, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(len == -1, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns0 uri"), 0, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 0;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns0 uri"), 0, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 0, "got %d\n", len);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns1"), _bstr_("ns1 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("http://www.w3.org/XML/1998/namespace"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"xml"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    /* with null buffer it's possible to get required length */
    len = 100;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("http://www.w3.org/XML/1998/namespace"), 0, NULL, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 3, "got %d\n", len);

    len = 0;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("http://www.w3.org/XML/1998/namespace"), 0, NULL, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 3, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 1, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 2, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_(""), 0, buffW, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_(""), 1, buffW, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, NULL, 0, buffW, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns0 uri"), 1, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_(""), 1, buffW, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    /* declare another one, indices are shifted */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns2"), _bstr_("ns2 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns2 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns2"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns2 uri"), 1, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_(""), 1, buffW, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    IMXNamespaceManager_Release(nsmgr);

    /* push/pop tests */
    hr = CoCreateInstance(&CLSID_MXNamespaceManager40, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXNamespaceManager, (void**)&nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* pop with empty stack */
    hr = IMXNamespaceManager_popContext(nsmgr);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns1"), _bstr_("ns1 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    hr = IMXNamespaceManager_pushContext(nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns2"), _bstr_("ns2 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns2 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns2"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    hr = IMXNamespaceManager_pushContext(nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns3"), _bstr_("ns3 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns2 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns2"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    hr = IMXNamespaceManager_popContext(nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_popContext(nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns2 uri"), 0, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(buffW[0] == 0x1, "got %x\n", buffW[0]);
    ok(len == 100, "got %d\n", len);

    len = 100;
    buffW[0] = 0x1;
    hr = IMXNamespaceManager_getPrefix(nsmgr, _bstr_("ns1 uri"), 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got %s\n", wine_dbgstr_w(buffW));
    ok(len == 3, "got %d\n", len);

    IMXNamespaceManager_Release(nsmgr);

    free_bstrs();
}

static void test_mxnamespacemanager_override(void)
{
    IMXNamespaceManager *nsmgr;
    WCHAR buffW[250];
    VARIANT_BOOL b;
    HRESULT hr;
    INT len;

    hr = CoCreateInstance(&CLSID_MXNamespaceManager40, NULL, CLSCTX_INPROC_SERVER,
        &IID_IMXNamespaceManager, (void**)&nsmgr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"xml"), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 1, buffW, &len);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_getAllowOverride(nsmgr, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_FALSE;
    hr = IMXNamespaceManager_getAllowOverride(nsmgr, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IMXNamespaceManager_putAllowOverride(nsmgr, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_(""), NULL, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns0 uri"), "got uri %s\n", wine_dbgstr_w(buffW));

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns0"), _bstr_("ns0 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 0, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"xml"), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 1, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns0"), "got prefix %s\n", wine_dbgstr_w(buffW));

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 2, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L""), "got prefix %s\n", wine_dbgstr_w(buffW));

    /* new prefix placed at index 1 always */
    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_("ns1"), _bstr_("ns1 uri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 1, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns1"), "got prefix %s\n", wine_dbgstr_w(buffW));

    hr = IMXNamespaceManager_declarePrefix(nsmgr, _bstr_(""), NULL);
    todo_wine ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri"));
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_putAllowOverride(nsmgr, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXNamespaceManager_declarePrefix(nsmgr, NULL, _bstr_("ns0 uri override"));
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getURI(nsmgr, _bstr_(""), NULL, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L"ns0 uri override"), "got uri %s\n", wine_dbgstr_w(buffW));

    len = ARRAY_SIZE(buffW);
    buffW[0] = 0;
    hr = IMXNamespaceManager_getDeclaredPrefix(nsmgr, 3, buffW, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(buffW, L""), "got prefix %s\n", wine_dbgstr_w(buffW));

    IMXNamespaceManager_Release(nsmgr);

    free_bstrs();
}

static const DOMNodeType nodetypes_test[] =
{
    NODE_ELEMENT,
    NODE_ATTRIBUTE,
    NODE_TEXT,
    NODE_CDATA_SECTION,
    NODE_ENTITY_REFERENCE,
    NODE_PROCESSING_INSTRUCTION,
    NODE_COMMENT,
    NODE_DOCUMENT_FRAGMENT,
    NODE_INVALID
};

static void test_dispex(void)
{
    const DOMNodeType *type = nodetypes_test;
    IXMLDOMImplementation *impl;
    IXMLDOMNodeList *node_list;
    IXMLDOMParseError *error;
    IXMLDOMNamedNodeMap *map;
    IXSLProcessor *processor;
    IXSLTemplate *template;
    IXMLDOMDocument *doc;
    IXMLHTTPRequest *req;
    IXMLDOMElement *elem;
    IDispatchEx *dispex;
    DISPPARAMS dispparams;
    IXMLDOMNode *node;
    VARIANT arg, ret;
    VARIANT_BOOL b;
    IUnknown *unk;
    HRESULT hr;
    DISPID did;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);

    for(; *type != NODE_INVALID; type++)
    {
        IXMLDOMNode *node;
        VARIANT v;

        V_VT(&v) = VT_I2;
        V_I2(&v) = *type;

        hr = IXMLDOMDocument_createNode(doc, v, _bstr_("name"), NULL, &node);
        ok(hr == S_OK, "failed to create node type %d\n", *type);

        IXMLDOMNode_QueryInterface(node, &IID_IUnknown, (void**)&unk);

        test_domobj_dispex(unk);
        IUnknown_Release(unk);
        IXMLDOMNode_Release(node);
    }

    /* IXMLDOMNodeList */
    hr = IXMLDOMDocument_getElementsByTagName(doc, _bstr_("*"), &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    IXMLDOMNodeList_Release(node_list);

    /* IXMLDOMNodeList for children list */
    hr = IXMLDOMDocument_get_childNodes(doc, &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);

    /* collection dispex test, empty collection */
    hr = IXMLDOMNodeList_QueryInterface(node_list, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE, "Unexpected value %ld.\n", did);
    hr = IDispatchEx_GetDispID(dispex, _bstr_("1"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE+1, "Unexpected value %ld.\n", did);
    IDispatchEx_Release(dispex);

    did = -1;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("item"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_VALUE, "got %ld\n", did);

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
    todo_wine
    ok(broken(V_DISPATCH(&ret) == (void*)0x1) || (V_DISPATCH(&ret) == NULL), "got %p\n", V_DISPATCH(&ret));

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 2;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
    todo_wine
    ok(broken(V_DISPATCH(&ret) == (void*)0x1) || (V_DISPATCH(&ret) == NULL), "got %p\n", V_DISPATCH(&ret));

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %d\n", V_VT(&ret));
    ok(V_DISPATCH(&ret) == NULL, "got %p\n", V_DISPATCH(&ret));

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %d\n", V_VT(&ret));
    ok(V_DISPATCH(&ret) == NULL, "got %p\n", V_DISPATCH(&ret));

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %d\n", V_VT(&ret));
    ok(V_DISPATCH(&ret) == NULL, "got %p\n", V_DISPATCH(&ret));

    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

    V_VT(&ret) = VT_EMPTY;
    V_I4(&ret) = 1;
    hr = IDispatchEx_Invoke(dispex, DISPID_DOM_NODELIST_LENGTH, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_I4, "got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == 0, "got %ld\n", V_I4(&ret));

    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

    V_VT(&ret) = VT_EMPTY;
    V_I4(&ret) = 1;
    hr = IDispatchEx_Invoke(dispex, DISPID_DOM_NODELIST_LENGTH, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
    todo_wine
    ok(broken(V_I4(&ret) == 1) || (V_I4(&ret) == 0), "got %ld\n", V_I4(&ret));

    IXMLDOMNodeList_Release(node_list);

    /* IXMLDOMParseError */
    hr = IXMLDOMDocument_get_parseError(doc, &error);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMParseError_QueryInterface(error, &IID_IUnknown, (void**)&unk);
    test_domobj_dispex(unk);

    hr = IXMLDOMParseError_QueryInterface(error, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
    todo_wine
    ok(broken(V_DISPATCH(&ret) == (void*)0x1) || (V_DISPATCH(&ret) == NULL), "got %p\n", V_DISPATCH(&ret));

    IDispatchEx_Release(dispex);

    IUnknown_Release(unk);
    IXMLDOMParseError_Release(error);

    /* IXMLDOMNamedNodeMap */
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(xpath_simple_list), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root/a"), &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_get_item(node_list, 0, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNodeList_Release(node_list);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);
    hr = IXMLDOMElement_get_attributes(elem, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IUnknown, (void**)&unk);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    /* collection dispex test */
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE, "Unexpected value %ld.\n", did);
    IDispatchEx_Release(dispex);
    IXMLDOMNamedNodeMap_Release(map);

    hr = IXMLDOMDocument_selectNodes(doc, _bstr_("root/b"), &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNodeList_get_item(node_list, 0, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNodeList_Release(node_list);
    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);
    hr = IXMLDOMElement_get_attributes(elem, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* collection dispex test, empty collection */
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    did = 0;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("0"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE, "Unexpected value %ld\n", did);
    hr = IDispatchEx_GetDispID(dispex, _bstr_("1"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_DOM_COLLECTION_BASE+1, "Unexpected value %ld.\n", did);
    IXMLDOMNamedNodeMap_Release(map);

    did = -1;
    hr = IDispatchEx_GetDispID(dispex, _bstr_("item"), 0, &did);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(did == DISPID_VALUE, "got %ld\n", did);

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
todo_wine {
    ok(hr == DISP_E_BADPARAMCOUNT, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
}
    ok(broken(V_DISPATCH(&ret) == (void*)0x1) || (V_DISPATCH(&ret) == NULL), "got %p\n", V_DISPATCH(&ret));

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 2;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
todo_wine {
    ok(hr == DISP_E_BADPARAMCOUNT, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
}
    ok(broken(V_DISPATCH(&ret) == (void*)0x1) || (V_DISPATCH(&ret) == NULL), "got %p\n", V_DISPATCH(&ret));

    V_VT(&arg) = VT_I4;
    V_I4(&arg) = 0;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = &arg;

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %d\n", V_VT(&ret));
    ok(V_DISPATCH(&ret) == NULL, "got %p\n", V_DISPATCH(&ret));

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %d\n", V_VT(&ret));
    ok(V_DISPATCH(&ret) == NULL, "got %p\n", V_DISPATCH(&ret));

    V_VT(&ret) = VT_EMPTY;
    V_DISPATCH(&ret) = (void*)0x1;
    hr = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %d\n", V_VT(&ret));
    ok(V_DISPATCH(&ret) == NULL, "got %p\n", V_DISPATCH(&ret));

    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

    V_VT(&ret) = VT_EMPTY;
    V_I4(&ret) = 1;
    hr = IDispatchEx_Invoke(dispex, DISPID_DOM_NODELIST_LENGTH, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &ret, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&ret) == VT_I4, "got %d\n", V_VT(&ret));
    ok(V_I4(&ret) == 0, "got %ld\n", V_I4(&ret));

    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

    V_VT(&ret) = VT_EMPTY;
    V_I4(&ret) = 1;
    hr = IDispatchEx_Invoke(dispex, DISPID_DOM_NODELIST_LENGTH, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &ret, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(V_VT(&ret) == VT_EMPTY, "got %d\n", V_VT(&ret));
    ok(broken(V_I4(&ret) == 1) || (V_I4(&ret) == 0), "got %ld\n", V_I4(&ret));

    IDispatchEx_Release(dispex);
    IXMLDOMElement_Release(elem);

    /* IXMLDOMImplementation */
    hr = IXMLDOMDocument_get_implementation(doc, &impl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMImplementation_QueryInterface(impl, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDispatchEx_Release(dispex);
    IXMLDOMImplementation_Release(impl);

    IXMLDOMDocument_Release(doc);

    /* IXMLHTTPRequest */
    hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER,
        &IID_IXMLHttpRequest, (void**)&req);
    if (hr == S_OK)
    {
        hr = IXMLHTTPRequest_QueryInterface(req, &IID_IDispatchEx, (void**)&dispex);
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
        IXMLHTTPRequest_Release(req);
    }

    /* IXSLTemplate */
    template = create_xsltemplate(&IID_IXSLTemplate);
    hr = IXSLTemplate_QueryInterface(template, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_QueryInterface(dispex, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    IDispatchEx_Release(dispex);

    /* IXSLProcessor */
    hr = CoCreateInstance(&CLSID_FreeThreadedDOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTransformSSXML), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXSLTemplate_putref_stylesheet(template, (IXMLDOMNode*)doc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMDocument_Release(doc);

    hr = IXSLTemplate_createProcessor(template, &processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXSLProcessor_QueryInterface(processor, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_QueryInterface(dispex, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_domobj_dispex(unk);
    IUnknown_Release(unk);
    IDispatchEx_Release(dispex);

    IXSLProcessor_Release(processor);
    IXSLTemplate_Release(template);

    if (is_clsid_supported(&CLSID_DOMDocument60, &IID_IXMLDOMDocument))
    {
        doc = create_document_version(60, &IID_IXMLDOMDocument);
        test_domobj_dispex((IUnknown*)doc);
        IXMLDOMDocument_Release(doc);
    }

    free_bstrs();
}

static void test_parseerror(void)
{
    IXMLDOMParseError2 *error2;
    IXMLDOMParseError *error;
    IXMLDOMDocument *doc;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_parseError(doc, &error);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMParseError_get_line(error, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMParseError_get_srcText(error, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMParseError_get_linepos(error, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IXMLDOMParseError_Release(error);
    IXMLDOMDocument_Release(doc);

    if (!is_clsid_supported(&CLSID_DOMDocument60, &IID_IXMLDOMDocument)) return;
    doc = create_document_version(60, &IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_parseError(doc, &error);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMParseError_QueryInterface(error, &IID_IXMLDOMParseError2, (void**)&error2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMParseError2_Release(error2);
    IXMLDOMParseError_Release(error);
    IXMLDOMDocument_Release(doc);
}

static void test_getAttributeNode(void)
{
    IXMLDOMAttribute *attr;
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    VARIANT_BOOL v;
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = SysAllocString(nonexistent_fileW);
    hr = IXMLDOMElement_getAttributeNode(elem, str, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    attr = (IXMLDOMAttribute*)0xdeadbeef;
    hr = IXMLDOMElement_getAttributeNode(elem, str, &attr);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(attr == NULL, "got %p\n", attr);
    SysFreeString(str);

    str = SysAllocString(L"nonExistingAttribute");
    hr = IXMLDOMElement_getAttributeNode(elem, str, NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    attr = (IXMLDOMAttribute*)0xdeadbeef;
    hr = IXMLDOMElement_getAttributeNode(elem, str, &attr);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(attr == NULL, "got %p\n", attr);
    SysFreeString(str);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("foo:b"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMAttribute_Release(attr);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("b"), &attr);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_getAttributeNode(elem, _bstr_("a"), &attr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMAttribute_Release(attr);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_getAttribute(void)
{
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    VARIANT_BOOL v;
    VARIANT var;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szExampleXML), &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    VariantInit(&var);
    hr = IXMLDOMElement_getAttribute( elem, _bstr_("xmlns:foo"), &var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"urn:uuid:86B2F87F-ACB6-45cd-8B77-9BDB92A01A29"), "wrong attr value: %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMElement_getAttribute( elem, _bstr_("a"), &var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"attr a"), "wrong attr value: %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMElement_getAttribute( elem, _bstr_("foo:b"), &var );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_BSTR, "vt = %x\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"attr b"), "wrong attr value: %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);

    hr = IXMLDOMElement_getAttribute( elem, _bstr_("b"), &var );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_NULL, "vt = %x\n", V_VT(&var));
    VariantClear(&var);

    hr = IXMLDOMElement_getAttribute( elem, _bstr_("non-existent"), &var );
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok( V_VT(&var) == VT_NULL, "vt = %x\n", V_VT(&var));
    VariantClear(&var);

    IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

typedef struct {
    DOMNodeType type;
    const char *name;
    REFIID iids[3];
} supporterror_t;

static const supporterror_t supporterror_test[] = {
    { NODE_ELEMENT,                "element",   { &IID_IXMLDOMNode, &IID_IXMLDOMElement } },
    { NODE_ATTRIBUTE,              "attribute", { &IID_IXMLDOMNode, &IID_IXMLDOMAttribute } },
    { NODE_CDATA_SECTION,          "cdata",     { &IID_IXMLDOMNode, &IID_IXMLDOMCDATASection } },
    { NODE_ENTITY_REFERENCE,       "entityref", { &IID_IXMLDOMNode, &IID_IXMLDOMEntityReference } },
    { NODE_PROCESSING_INSTRUCTION, "pi",        { &IID_IXMLDOMNode, &IID_IXMLDOMProcessingInstruction } },
    { NODE_COMMENT,                "comment",   { &IID_IXMLDOMNode, &IID_IXMLDOMComment } },
    { NODE_DOCUMENT_FRAGMENT,      "fragment",  { &IID_IXMLDOMNode, &IID_IXMLDOMDocumentFragment } },
    { NODE_INVALID }
};

static void test_supporterrorinfo(void)
{
    static REFIID iids[5] = { &IID_IXMLDOMNode, &IID_IXMLDOMDocument,
                              &IID_IXMLDOMDocument2, &IID_IXMLDOMDocument3 };
    const supporterror_t *ptr = supporterror_test;
    ISupportErrorInfo *errorinfo, *info2;
    IXMLDOMSchemaCollection *schemacache;
    IXMLDOMNamedNodeMap *map, *map2;
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    VARIANT_BOOL b;
    IUnknown *unk;
    REFIID *iid;
    void *dummy;
    HRESULT hr;

    if (!is_clsid_supported(&CLSID_DOMDocument60, &IID_IXMLDOMDocument3)) return;
    doc = create_document_version(60, &IID_IXMLDOMDocument3);

    EXPECT_REF(doc, 1);
    hr = IXMLDOMDocument_QueryInterface(doc, &IID_ISupportErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(doc, 1);
    ISupportErrorInfo_AddRef(errorinfo);
    EXPECT_REF(errorinfo, 2);
    EXPECT_REF(doc, 1);
    ISupportErrorInfo_Release(errorinfo);

    hr = IXMLDOMDocument_QueryInterface(doc, &IID_ISupportErrorInfo, (void**)&info2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(errorinfo != info2, "got %p, %p\n", info2, errorinfo);

    /* error interface can't be queried back for DOM interface */
    hr = ISupportErrorInfo_QueryInterface(info2, &IID_IXMLDOMDocument, &dummy);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);
    hr = ISupportErrorInfo_QueryInterface(info2, &IID_IXMLDOMNode, &dummy);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    ISupportErrorInfo_Release(info2);

    iid = iids;
    while (*iid)
    {
        hr = IXMLDOMDocument_QueryInterface(doc, *iid, (void**)&unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            hr = ISupportErrorInfo_InterfaceSupportsErrorInfo(errorinfo, *iid);
            ok(hr == S_OK, "Unexpected hr %#lx. for %s\n", hr, wine_dbgstr_guid(*iid));
            IUnknown_Release(unk);
        }

        iid++;
    }

    ISupportErrorInfo_Release(errorinfo);

    while (ptr->type != NODE_INVALID)
    {
        IXMLDOMNode *node;
        VARIANT type;

        V_VT(&type) = VT_I1;
        V_I1(&type) = ptr->type;

        hr = IXMLDOMDocument_createNode(doc, type, _bstr_(ptr->name), NULL, &node);
        ok(hr == S_OK, "%d: Unexpected hr %#lx.\n", ptr->type, hr);

        EXPECT_REF(node, 1);
        hr = IXMLDOMNode_QueryInterface(node, &IID_ISupportErrorInfo, (void**)&errorinfo);
        ok(hr == S_OK, "%d: Unexpected hr %#lx.\n", ptr->type, hr);
        EXPECT_REF(node, 1);

        hr = ISupportErrorInfo_QueryInterface(errorinfo, &IID_IXMLDOMNode, &dummy);
        ok(hr == E_NOINTERFACE, "%d: Unexpected hr %#lx.\n", ptr->type, hr);

        iid = ptr->iids;

        while (*iid)
        {
            hr = IXMLDOMNode_QueryInterface(node, *iid, (void**)&unk);
            if (hr == S_OK)
            {
                hr = ISupportErrorInfo_InterfaceSupportsErrorInfo(errorinfo, *iid);
                ok(hr == S_OK, "%d: unexpected hr %#lx for %s\n", ptr->type, hr, wine_dbgstr_guid(*iid));
                IUnknown_Release(unk);
            }

            iid++;
        }

        ISupportErrorInfo_Release(errorinfo);
        IXMLDOMNode_Release(node);
        ptr++;
    }

    /* IXMLDOMNamedNodeMap */
    b = VARIANT_FALSE;
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(complete4A), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_attributes(elem, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(map, 1);
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_ISupportErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(map, 2);

    hr = ISupportErrorInfo_InterfaceSupportsErrorInfo(errorinfo, &IID_IXMLDOMNamedNodeMap);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISupportErrorInfo_QueryInterface(errorinfo, &IID_IXMLDOMNamedNodeMap, (void**)&map2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(map == map2, "got %p\n", map2);
    IXMLDOMNamedNodeMap_Release(map2);

    EXPECT_REF(errorinfo, 2);
    hr = ISupportErrorInfo_QueryInterface(errorinfo, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(errorinfo, 3);
    EXPECT_REF(map, 3);
    IUnknown_Release(unk);

    ISupportErrorInfo_Release(errorinfo);
    IXMLDOMNamedNodeMap_Release(map);
    IXMLDOMElement_Release(elem);

    IXMLDOMDocument_Release(doc);

    /* IXMLDOMSchemaCollection */
    hr = CoCreateInstance(&CLSID_XMLSchemaCache, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMSchemaCollection, (void**)&schemacache);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection_QueryInterface(schemacache, &IID_ISupportErrorInfo, (void**)&errorinfo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISupportErrorInfo_InterfaceSupportsErrorInfo(errorinfo, &IID_IXMLDOMSchemaCollection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ISupportErrorInfo_Release(errorinfo);
    IXMLDOMSchemaCollection_Release(schemacache);

    free_bstrs();
}

typedef struct {
    DOMNodeType type;
    const char *name;
    const char *put_content;
    HRESULT put_hr;
    VARTYPE get_vt;
    HRESULT get_hr;
} node_value_t;

static const node_value_t nodevalue_test[] = {
    { NODE_ELEMENT,                "element",   "",             E_FAIL, VT_NULL, S_FALSE },
    { NODE_ATTRIBUTE,              "attr",      "value",        S_OK,   VT_BSTR, S_OK },
    { NODE_TEXT,                   "text",      "textdata",     S_OK,   VT_BSTR, S_OK },
    { NODE_CDATA_SECTION ,         "cdata",     "cdata data",   S_OK,   VT_BSTR, S_OK },
    { NODE_ENTITY_REFERENCE,       "entityref", "ref",          E_FAIL, VT_NULL, S_FALSE },
    { NODE_PROCESSING_INSTRUCTION, "pi",        "instr",        S_OK,   VT_BSTR, S_OK },
    { NODE_COMMENT,                "comment",   "comment data", S_OK,   VT_BSTR, S_OK },
    { NODE_DOCUMENT_FRAGMENT,      "docfrag",   "",             E_FAIL, VT_NULL, S_FALSE },
    { NODE_INVALID }
};

static void test_nodeValue(void)
{
    const node_value_t *ptr = nodevalue_test;
    IXMLDOMDocument *doc;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    while (ptr->type != NODE_INVALID)
    {
        IXMLDOMNode *node;
        VARIANT v;

        V_VT(&v) = VT_I2;
        V_I2(&v) = ptr->type;

        hr = IXMLDOMDocument_createNode(doc, v, _bstr_(ptr->name), NULL, &node);
        ok(hr == S_OK, "failed to create node type %d\n", ptr->type);

        hr = IXMLDOMNode_get_nodeValue(node, NULL);
        ok(hr == E_INVALIDARG, "%d: unexpected hr %#lx\n", ptr->type, hr);

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = _bstr_(ptr->put_content);
        hr = IXMLDOMNode_put_nodeValue(node, v);
        ok(hr == ptr->put_hr, "%d: unexpected hr %#lx\n", ptr->type, hr);

        V_VT(&v) = VT_EMPTY;
        hr = IXMLDOMNode_get_nodeValue(node, &v);
        ok(hr == ptr->get_hr, "%d: unexpected hr %#lx, expected %#lx.\n", ptr->type, hr, ptr->get_hr);
        ok(V_VT(&v) == ptr->get_vt, "%d: got %d, expected %d\n", ptr->type, V_VT(&v), ptr->get_vt);
        if (hr == S_OK)
            ok(!lstrcmpW(V_BSTR(&v), _bstr_(ptr->put_content)), "%d: got %s\n", ptr->type,
                wine_dbgstr_w(V_BSTR(&v)));
        VariantClear(&v);

        IXMLDOMNode_Release(node);

        ptr++;
    }

    IXMLDOMDocument_Release(doc);
}

static void test_xmlns_attribute(void)
{
    BSTR str;
    IXMLDOMDocument *doc;
    IXMLDOMElement *root;
    IXMLDOMAttribute *pAttribute;
    IXMLDOMElement *elem;
    HRESULT hr;
    VARIANT v;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)root, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createAttribute(doc, _bstr_("xmlns:dt"), &pAttribute);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = _bstr_("urn:schemas-microsoft-com:datatypes");
    hr = IXMLDOMAttribute_put_nodeValue(pAttribute, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMElement_setAttributeNode(root, pAttribute, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMNode_put_dataType((IXMLDOMNode*)root, _bstr_("bin.base64"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr );

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IXMLDOMElement_get_xml(elem, &str);
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(lstrcmpW(str, L"<Testing xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"bin.base64\"/>") == 0,
    "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMElement_Release(elem);
    IXMLDOMAttribute_Release( pAttribute);

    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static const char namespacesA[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
"   <ns1:elem1 xmlns:ns1=\"http://blah.org\" b='1' >"
"     <ns2:elem2 xmlns:ns2=\"http://blah.org\"/>"
"     <ns1:elem3/>"
"     <ns1:elem4/>"
"     <elem5 xmlns=\"http://blahblah.org\"/>"
"     <ns1:elem6>true</ns1:elem6>"
"   </ns1:elem1>";

static const char xsd_schema1_uri[] = "x-schema:test1.xsd";
static const char xsd_schema1_xml[] =
"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test1.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";

static void test_get_namespaces(void)
{
    IXMLDOMSchemaCollection *collection, *collection2;
    IXMLDOMDocument2 *doc, *doc2;
    IEnumVARIANT *enumv, *enum2;
    IUnknown *unk1, *unk2;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    ULONG fetched;
    VARIANT v[2];
    HRESULT hr;
    LONG len;
    BSTR s;

    if (!is_clsid_supported(&CLSID_DOMDocument2, &IID_IXMLDOMDocument2)) return;
    doc = create_document(&IID_IXMLDOMDocument2);

    /* null pointer */
    hr = IXMLDOMDocument2_get_namespaces(doc, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* no document loaded */
    collection = (void*)0xdeadbeef;
    hr = IXMLDOMDocument2_get_namespaces(doc, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr != S_OK)
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }
    EXPECT_REF(collection, 2);

    collection2 = (void*)0xdeadbeef;
    hr = IXMLDOMDocument2_get_namespaces(doc, &collection2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(collection == collection2, "got %p\n", collection2);
    EXPECT_REF(collection, 3);
    IXMLDOMSchemaCollection_Release(collection);

    len = -1;
    hr = IXMLDOMSchemaCollection_get_length(collection, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %ld\n", len);
    IXMLDOMSchemaCollection_Release(collection);

    /* now with document */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(namespacesA), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_get_namespaces(doc, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMSchemaCollection_get_length(collection, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 2, "got %ld\n", len);

    /* try to lookup some uris */
    node = (void*)0xdeadbeef;
    hr = IXMLDOMSchemaCollection_get(collection, _bstr_("http://blah.org"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    node = (void*)0xdeadbeef;
    hr = IXMLDOMSchemaCollection_get(collection, _bstr_("http://blah1.org"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == NULL, "got %p\n", node);

    /* load schema and try to add it */
    doc2 = create_document(&IID_IXMLDOMDocument2);
    hr = IXMLDOMDocument2_loadXML(doc2, _bstr_(xsd_schema1_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v[0]) = VT_DISPATCH;
    V_DISPATCH(&v[0]) = (IDispatch*)doc2;
    hr = IXMLDOMSchemaCollection_add(collection, _bstr_(xsd_schema1_uri), v[0]);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection_get_namespaceURI(collection, 0, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, L"http://blah.org"), "got %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    hr = IXMLDOMSchemaCollection_get_namespaceURI(collection, 1, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, L"http://blahblah.org"), "got %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    s = (void*)0xdeadbeef;
    hr = IXMLDOMSchemaCollection_get_namespaceURI(collection, 2, &s);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(s == (void*)0xdeadbeef, "got %p\n", s);

    /* enumerate */
    enumv = (void*)0xdeadbeef;
    EXPECT_REF(collection, 2);
    hr = IXMLDOMSchemaCollection_get__newEnum(collection, (IUnknown**)&enumv);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(collection, 3);
    ok(enumv != NULL, "got %p\n", enumv);

    hr = IXMLDOMSchemaCollection_QueryInterface(collection, &IID_IUnknown, (void**)&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumVARIANT_QueryInterface(enumv, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk1 != unk2, "got %p, %p\n", unk1, unk2);
    IUnknown_Release(unk1);
    IUnknown_Release(unk2);

    hr = IXMLDOMSchemaCollection_QueryInterface(collection, &IID_IEnumVARIANT, (void**)&enum2);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enumv, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_BSTR, "got %d\n", V_VT(&v[0]));
    ok(!lstrcmpW(V_BSTR(&v[0]), L"http://blah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    VariantClear(&v[0]);

    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enumv, 1, &v[0], &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 1, "got %ld, expected 1\n", fetched);
    ok(V_VT(&v[0]) == VT_BSTR, "got %d\n", V_VT(&v[0]));
    ok(!lstrcmpW(V_BSTR(&v[0]), L"http://blahblah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    VariantClear(&v[0]);

    V_VT(&v[0]) = VT_NULL;
    hr = IEnumVARIANT_Next(enumv, 1, &v[0], &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 0, "got %ld, expected 0\n", fetched);
    ok(V_VT(&v[0]) == VT_EMPTY, "got %d\n", V_VT(&v[0]));

    hr = IEnumVARIANT_Reset(enumv);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enumv, 2, v, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 2, "got %ld, expected 2\n", fetched);
    ok(V_VT(&v[0]) == VT_BSTR, "got %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_BSTR, "got %d\n", V_VT(&v[1]));
    ok(!lstrcmpW(V_BSTR(&v[0]), L"http://blah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    ok(!lstrcmpW(V_BSTR(&v[1]), L"http://blahblah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[1])));
    VariantClear(&v[1]);
    VariantClear(&v[0]);

    V_VT(&v[0]) = VT_NULL;
    hr = IEnumVARIANT_Next(enumv, 1, v, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 0, "got %ld, expected 0\n", fetched);
    ok(V_VT(&v[0]) == VT_EMPTY, "got %d\n", V_VT(&v[0]));

    IEnumVARIANT_Release(enumv);
    IXMLDOMSchemaCollection_Release(collection);
    IXMLDOMDocument2_Release(doc);

    /* now with CLSID_DOMDocument60 */
    if (!is_clsid_supported(&CLSID_DOMDocument60, &IID_IXMLDOMDocument2)) return;
    doc = create_document_version(60, &IID_IXMLDOMDocument2);

    /* null pointer */
    hr = IXMLDOMDocument2_get_namespaces(doc, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* no document loaded */
    collection = (void*)0xdeadbeef;
    hr = IXMLDOMDocument2_get_namespaces(doc, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr != S_OK)
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }
    EXPECT_REF(collection, 2);

    collection2 = (void*)0xdeadbeef;
    hr = IXMLDOMDocument2_get_namespaces(doc, &collection2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(collection == collection2, "got %p\n", collection2);
    EXPECT_REF(collection, 3);
    IXMLDOMSchemaCollection_Release(collection);

    len = -1;
    hr = IXMLDOMSchemaCollection_get_length(collection, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %ld\n", len);
    IXMLDOMSchemaCollection_Release(collection);

    /* now with document */
    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(namespacesA), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument2_get_namespaces(doc, &collection);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMSchemaCollection_get_length(collection, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 2, "got %ld\n", len);

    /* try to lookup some uris */
    node = (void*)0xdeadbeef;
    hr = IXMLDOMSchemaCollection_get(collection, _bstr_("http://blah.org"), &node);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(broken(node == (void*)0xdeadbeef) || (node == NULL), "got %p\n", node);

    /* load schema and try to add it */
    doc2 = create_document(&IID_IXMLDOMDocument2);
    hr = IXMLDOMDocument2_loadXML(doc2, _bstr_(xsd_schema1_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v[0]) = VT_DISPATCH;
    V_DISPATCH(&v[0]) = (IDispatch*)doc2;
    hr = IXMLDOMSchemaCollection_add(collection, _bstr_(xsd_schema1_uri), v[0]);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    IXMLDOMDocument2_Release(doc2);

    hr = IXMLDOMSchemaCollection_get_namespaceURI(collection, 0, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, L"http://blah.org"), "got %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    hr = IXMLDOMSchemaCollection_get_namespaceURI(collection, 1, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, L"http://blahblah.org"), "got %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    s = (void*)0xdeadbeef;
    hr = IXMLDOMSchemaCollection_get_namespaceURI(collection, 2, &s);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(broken(s == (void*)0xdeadbeef) || (s == NULL), "got %p\n", s);

    /* enumerate */
    enumv = (void*)0xdeadbeef;
    hr = IXMLDOMSchemaCollection_get__newEnum(collection, (IUnknown**)&enumv);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enumv != NULL, "got %p\n", enumv);

    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enumv, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_BSTR, "got %d\n", V_VT(&v[0]));
    ok(!lstrcmpW(V_BSTR(&v[0]), L"http://blah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    VariantClear(&v[0]);

    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enumv, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_BSTR, "got %d\n", V_VT(&v[0]));
    ok(!lstrcmpW(V_BSTR(&v[0]), L"http://blahblah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    VariantClear(&v[0]);

    V_VT(&v[0]) = VT_NULL;
    hr = IEnumVARIANT_Next(enumv, 1, &v[0], NULL);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_EMPTY, "got %d\n", V_VT(&v[0]));

    hr = IEnumVARIANT_Reset(enumv);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enumv, 2, v, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 2, "got %ld, expected 2\n", fetched);
    ok(V_VT(&v[0]) == VT_BSTR, "got %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_BSTR, "got %d\n", V_VT(&v[1]));
    ok(!lstrcmpW(V_BSTR(&v[0]), L"http://blah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    ok(!lstrcmpW(V_BSTR(&v[1]), L"http://blahblah.org"), "got %s\n", wine_dbgstr_w(V_BSTR(&v[0])));
    VariantClear(&v[1]);
    VariantClear(&v[0]);

    V_VT(&v[0]) = VT_NULL;
    hr = IEnumVARIANT_Next(enumv, 1, v, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 0, "got %ld, expected 0\n", fetched);
    ok(V_VT(&v[0]) == VT_EMPTY, "got %d\n", V_VT(&v[0]));

    IEnumVARIANT_Release(enumv);
    IXMLDOMSchemaCollection_Release(collection);
    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

static const DOMNodeType put_data_types[] = {
    NODE_TEXT,
    NODE_CDATA_SECTION,
    NODE_PROCESSING_INSTRUCTION,
    NODE_COMMENT,
    NODE_INVALID
};

static void test_put_data(void)
{
    WCHAR buff[100], *data;
    IXMLDOMDocument *doc;
    const DOMNodeType *type;
    IXMLDOMText *text;
    IXMLDOMNode *node;
    VARIANT v;
    BSTR get_data;
    HRESULT hr;

    doc = create_document(&IID_IXMLDOMDocument);

    memcpy(&buff[2], L"test node data", 15 * sizeof(WCHAR));
    /* just a big length */
    *(DWORD*)buff = 0xf0f0;
    data = &buff[2];

    type = put_data_types;
    while (*type != NODE_INVALID)
    {
       V_VT(&v) = VT_I2;
       V_I2(&v) = *type;

       hr = IXMLDOMDocument_createNode(doc, v, _bstr_("name"), NULL, &node);
       ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

       /* put_data() is interface-specific */
       switch (*type)
       {
           case NODE_TEXT:
           {
              hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
              hr = IXMLDOMText_put_data(text, data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              hr = IXMLDOMText_get_data(text, &get_data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              IXMLDOMText_Release(text);
              break;
           }
           case NODE_CDATA_SECTION:
           {
              IXMLDOMCDATASection *cdata;

              hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)&cdata);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
              hr = IXMLDOMCDATASection_put_data(cdata, data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              hr = IXMLDOMCDATASection_get_data(cdata, &get_data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              IXMLDOMCDATASection_Release(cdata);
              break;
           }
           case NODE_PROCESSING_INSTRUCTION:
           {
              IXMLDOMProcessingInstruction *pi;

              hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMProcessingInstruction, (void**)&pi);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
              hr = IXMLDOMProcessingInstruction_put_data(pi, data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              hr = IXMLDOMProcessingInstruction_get_data(pi, &get_data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              IXMLDOMProcessingInstruction_Release(pi);
              break;
           }
           case NODE_COMMENT:
           {
              IXMLDOMComment *comment;

              hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)&comment);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
              hr = IXMLDOMComment_put_data(comment, data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              hr = IXMLDOMComment_get_data(comment, &get_data);
              ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

              IXMLDOMComment_Release(comment);
              break;
           }
           default:
              get_data = NULL;
              break;
       }

       /* compare */
       ok(!lstrcmpW(data, get_data), "%d: got wrong data %s, expected %s\n", *type, wine_dbgstr_w(get_data),
           wine_dbgstr_w(data));
       SysFreeString(get_data);

       IXMLDOMNode_Release(node);
       type++;
    }

    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_newline_normalization(void)
{
    const struct msxmlsupported_data_t *table = domdoc_support_data;
    IXMLDOMDocument *doc;
    IXMLDOMText *text;
    IXMLDOMNode *node;
    VARIANT v;
    VARIANT_BOOL b;
    BSTR s;
    HRESULT hr;
    LONG length;

    V_VT(&v) = VT_I2;
    V_I2(&v) = NODE_TEXT;

    while (table->clsid)
    {
        if (!is_clsid_supported(table->clsid, &IID_IXMLDOMDocument))
        {
            table++;
            continue;
        }

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_createNode(doc, v, _bstr_("name"), NULL, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)&text);

        /* \r\n is normalized to \n and back to \r\n */

        hr = IXMLDOMText_put_data(text, _bstr_("\r\n"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_data(text, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"\n"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        hr = IXMLDOMText_get_length(text, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 1, "got %ld, expected 1\n", length);

        hr = IXMLDOMText_get_xml(text, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"\r\n"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        /* \r\r\n is normalized to \n\n and back to \r\n\r\n */

        hr = IXMLDOMText_put_data(text, _bstr_("\r\r\n"));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMText_get_data(text, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"\n\n"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        hr = IXMLDOMText_get_length(text, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 2, "got %ld, expected 2\n", length);

        hr = IXMLDOMText_get_xml(text, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"\r\n\r\n"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        /* the same normalizations are applied when loading a document as a whole */

        hr = IXMLDOMDocument_loadXML(doc, _bstr_("<?xml version=\"1.0\"?><root>foo\n\r\n\r\r\nbar</root>"), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_get_text(doc, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"foo\n\n\n\nbar"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        hr = IXMLDOMDocument_get_xml(doc, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"<?xml version=\"1.0\"?>\r\n<root>foo\r\n\r\n\r\n\r\nbar</root>\r\n"),
           "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        /* even if xml:space="preserve" */

        hr = IXMLDOMDocument_loadXML(doc, _bstr_("<?xml version=\"1.0\"?>"
                                                 "<root xml:space=\"preserve\">foo\n\r\n\r\r\nbar</root>"), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_get_text(doc, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"foo\n\n\n\nbar"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        hr = IXMLDOMDocument_get_xml(doc, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, _bstr_("<?xml version=\"1.0\"?>\r\n"
                               "<root xml:space=\"preserve\">foo\r\n\r\n\r\n\r\nbar</root>\r\n")),
           "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        /* or preserveWhiteSpace is set */

        hr = IXMLDOMDocument_put_preserveWhiteSpace(doc, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_loadXML(doc, _bstr_("<?xml version=\"1.0\"?><root>foo\n\r\n\r\r\nbar</root>"), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument_get_text(doc, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!lstrcmpW(s, L"foo\n\n\n\nbar"), "got %s\n", wine_dbgstr_w(s));
        SysFreeString(s);

        hr = IXMLDOMDocument_get_xml(doc, &s);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (IsEqualGUID(table->clsid, &CLSID_DOMDocument60))
        {
            /* DOMDocument60 does the newline normalization but does not insert line breaks around the root node */
            todo_wine
            ok(!lstrcmpW(s, L"<?xml version=\"1.0\"?><root>foo\r\n\r\n\r\n\r\nbar</root>"),
               "got %s\n", wine_dbgstr_w(s));
        }
        else
        {
            ok(!lstrcmpW(s, L"<?xml version=\"1.0\"?>\r\n<root>foo\r\n\r\n\r\n\r\nbar</root>\r\n"),
               "got %s\n", wine_dbgstr_w(s));
        }
        SysFreeString(s);

        IXMLDOMText_Release(text);
        IXMLDOMNode_Release(node);
        IXMLDOMDocument_Release(doc);
        free_bstrs();
        table++;
    }
}

static void test_putref_schemas(void)
{
    IXMLDOMSchemaCollection *cache;
    IXMLDOMDocument2 *doc;
    VARIANT schema;
    HRESULT hr;

    if (!is_clsid_supported(&CLSID_DOMDocument2, &IID_IXMLDOMDocument2)) return;
    if (!is_clsid_supported(&CLSID_XMLSchemaCache, &IID_IXMLDOMSchemaCollection)) return;

    doc = create_document(&IID_IXMLDOMDocument2);
    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    /* set to NULL iface when no schema is set */
    V_VT(&schema) = VT_DISPATCH;
    V_DISPATCH(&schema) = NULL;
    hr = IXMLDOMDocument2_putref_schemas(doc, schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&schema) = VT_UNKNOWN;
    V_UNKNOWN(&schema) = NULL;
    hr = IXMLDOMDocument2_putref_schemas(doc, schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* set as VT_DISPATCH, reset with it */
    V_VT(&schema) = VT_DISPATCH;
    V_DISPATCH(&schema) = (IDispatch*)cache;
    hr = IXMLDOMDocument2_putref_schemas(doc, schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_DISPATCH(&schema) = NULL;
    hr = IXMLDOMDocument2_get_schemas(doc, &schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&schema) == (IDispatch*)cache, "got %p\n", V_DISPATCH(&schema));

    V_VT(&schema) = VT_DISPATCH;
    V_DISPATCH(&schema) = NULL;
    hr = IXMLDOMDocument2_putref_schemas(doc, schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_DISPATCH(&schema) = (IDispatch*)0xdeadbeef;
    V_VT(&schema) = VT_I2;
    hr = IXMLDOMDocument2_get_schemas(doc, &schema);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&schema) == NULL, "got %p\n", V_DISPATCH(&schema));
    ok(V_VT(&schema) == VT_NULL, "got %d\n", V_VT(&schema));

    /* set as VT_UNKNOWN, reset with it */
    V_VT(&schema) = VT_UNKNOWN;
    V_UNKNOWN(&schema) = (IUnknown*)cache;
    hr = IXMLDOMDocument2_putref_schemas(doc, schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_DISPATCH(&schema) = NULL;
    hr = IXMLDOMDocument2_get_schemas(doc, &schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&schema) == (IDispatch*)cache, "got %p\n", V_DISPATCH(&schema));

    V_VT(&schema) = VT_UNKNOWN;
    V_UNKNOWN(&schema) = NULL;
    hr = IXMLDOMDocument2_putref_schemas(doc, schema);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_DISPATCH(&schema) = (IDispatch*)0xdeadbeef;
    V_VT(&schema) = VT_I2;
    hr = IXMLDOMDocument2_get_schemas(doc, &schema);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&schema) == NULL, "got %p\n", V_DISPATCH(&schema));
    ok(V_VT(&schema) == VT_NULL, "got %d\n", V_VT(&schema));

    IXMLDOMSchemaCollection_Release(cache);
    IXMLDOMDocument2_Release(doc);
}

static void test_namedmap_newenum(void)
{
    IEnumVARIANT *enum1, *enum2, *enum3;
    IXMLDOMNamedNodeMap *map;
    IUnknown *unk1, *unk2;
    IXMLDOMDocument *doc;
    IXMLDOMElement *elem;
    IXMLDOMNode *node;
    VARIANT_BOOL b;
    ULONG fetched;
    VARIANT v[3];
    HRESULT hr;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(attributes_map), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(doc, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_attributes(elem, &map);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(elem);

    enum1 = NULL;
    EXPECT_REF(map, 1);
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IEnumVARIANT, (void**)&enum1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum1 != NULL, "got %p\n", enum1);
    EXPECT_REF(map, 1);
    EXPECT_REF(enum1, 2);

    enum2 = NULL;
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IEnumVARIANT, (void**)&enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum2 == enum1, "got %p\n", enum2);

    IEnumVARIANT_Release(enum2);

    EXPECT_REF(map, 1);
    hr = IXMLDOMNamedNodeMap__newEnum(map, (IUnknown**)&enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(map, 2);
    EXPECT_REF(enum2, 1);
    ok(enum2 != enum1, "got %p, %p\n", enum2, enum1);

    IEnumVARIANT_Release(enum1);

    /* enumerator created with _newEnum() doesn't share IUnknown* with main object */
    hr = IXMLDOMNamedNodeMap_QueryInterface(map, &IID_IUnknown, (void**)&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IEnumVARIANT_QueryInterface(enum2, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(map, 3);
    EXPECT_REF(enum2, 2);
    ok(unk1 != unk2, "got %p, %p\n", unk1, unk2);
    IUnknown_Release(unk1);
    IUnknown_Release(unk2);

    hr = IXMLDOMNamedNodeMap__newEnum(map, (IUnknown**)&enum3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(enum2 != enum3, "got %p, %p\n", enum2, enum3);
    IEnumVARIANT_Release(enum3);

    /* iteration tests */
    hr = IXMLDOMNamedNodeMap_get_item(map, 0, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr1"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);

    hr = IXMLDOMNamedNodeMap_nextNode(map, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr1"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);

    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum2, 1, &v[0], NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr1"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    VariantClear(&v[0]);

    hr = IXMLDOMNamedNodeMap_nextNode(map, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr2"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);

    hr = IEnumVARIANT_Reset(enum2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum2, 2, v, &fetched);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 2, "got %ld, expected 2\n", fetched);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[1]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr1"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[1]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr2"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    VariantClear(&v[1]);
    VariantClear(&v[0]);

    V_VT(&v[2]) = VT_NULL;
    V_VT(&v[1]) = VT_EMPTY;
    V_VT(&v[0]) = VT_EMPTY;
    hr = IEnumVARIANT_Next(enum2, 3, v, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(fetched == 2, "got %ld, expected 2\n", fetched);
    ok(V_VT(&v[0]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[0]));
    ok(V_VT(&v[1]) == VT_DISPATCH, "got var type %d\n", V_VT(&v[1]));
    ok(V_VT(&v[2]) == VT_EMPTY, "got var type %d\n", V_VT(&v[2]));
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[0]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr3"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    hr = IDispatch_QueryInterface(V_DISPATCH(&v[1]), &IID_IXMLDOMNode, (void**)&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMNode_get_nodeName(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"attr4"), "got node name %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IXMLDOMNode_Release(node);
    VariantClear(&v[1]);
    VariantClear(&v[0]);
    IEnumVARIANT_Release(enum2);

    IXMLDOMNamedNodeMap_Release(map);
    IXMLDOMDocument_Release(doc);
}

static const char xsltext_xsl[] =
"<?xml version=\"1.0\"?>"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" >"
"<xsl:output method=\"html\" encoding=\"us-ascii\"/>"
"<xsl:template match=\"/\">"
"    <xsl:choose>"
"        <xsl:when test=\"testkey\">"
"            <xsl:text>testdata</xsl:text>"
"        </xsl:when>"
"    </xsl:choose>"
"</xsl:template>"
"</xsl:stylesheet>";

static const char omitxmldecl_xsl[] =
"<?xml version=\"1.0\"?>"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" >"
"<xsl:output method=\"xml\" omit-xml-declaration=\"yes\"/>"
"<xsl:template match=\"/\">"
"    <xsl:for-each select=\"/a/item\">"
"        <xsl:element name=\"node\">"
"            <xsl:value-of select=\"@name\"/>"
"        </xsl:element>"
"    </xsl:for-each>"
"</xsl:template>"
"</xsl:stylesheet>";

static const char omitxmldecl_doc[] =
"<?xml version=\"1.0\"?>"
"<a>"
"    <item name=\"item1\"/>"
"    <item name=\"item2\"/>"
"</a>";

static void test_xsltext(void)
{
    IXMLDOMDocument *doc, *doc2;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR ret;

    doc = create_document(&IID_IXMLDOMDocument);
    doc2 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(xsltext_xsl), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_loadXML(doc2, _bstr_("<testkey/>"), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_transformNode(doc2, (IXMLDOMNode*)doc, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(ret, L"testdata"), "transform result %s\n", wine_dbgstr_w(ret));
    SysFreeString(ret);

    /* omit-xml-declaration */
    hr = IXMLDOMDocument_loadXML(doc, _bstr_(omitxmldecl_xsl), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_loadXML(doc2, _bstr_(omitxmldecl_doc), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_transformNode(doc2, (IXMLDOMNode*)doc, &ret);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(ret, L"<node>item1</node><node>item2</node>"), "transform result %s\n", wine_dbgstr_w(ret));
    SysFreeString(ret);

    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

struct attrtest_t {
    const char *name;
    const char *uri;
    const char *prefix;
    const char *href;
};

static struct attrtest_t attrtests[] = {
    { "xmlns", "http://www.w3.org/2000/xmlns/", "xmlns", "xmlns" },
    { "xmlns", "nondefaulturi", "xmlns", "xmlns" },
    { "c", "http://www.w3.org/2000/xmlns/", NULL, "http://www.w3.org/2000/xmlns/" },
    { "c", "nsref1", NULL, "nsref1" },
    { "ns:c", "nsref1", "ns", "nsref1" },
    { "xmlns:c", "http://www.w3.org/2000/xmlns/", "xmlns", "" },
    { "xmlns:c", "nondefaulturi", "xmlns", "" },
    { 0 }
};

/* see dlls/msxml[46]/tests/domdoc.c */
static void test_create_attribute(void)
{
    struct attrtest_t *ptr = attrtests;
    IXMLDOMElement *el;
    IXMLDOMDocument *doc;
    IXMLDOMNode *node, *node2;
    VARIANT var;
    HRESULT hr;
    int i = 0;
    BSTR str;

    doc = create_document(&IID_IXMLDOMDocument);

    while (ptr->name)
    {
        V_VT(&var) = VT_I1;
        V_I1(&var) = NODE_ATTRIBUTE;
        hr = IXMLDOMDocument_createNode(doc, var, _bstr_(ptr->name), _bstr_(ptr->uri), &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        str = NULL;
        hr = IXMLDOMNode_get_prefix(node, &str);
        if (ptr->prefix)
        {
            ok(hr == S_OK, "%d: unexpected hr %#lx\n", i, hr);
            ok(!lstrcmpW(str, _bstr_(ptr->prefix)), "%d: got prefix %s, expected %s\n", i, wine_dbgstr_w(str), ptr->prefix);
        }
        else
        {
            ok(hr == S_FALSE, "%d: unexpected hr %#lx\n", i, hr);
            ok(str == NULL, "%d: got prefix %s\n", i, wine_dbgstr_w(str));
        }
        SysFreeString(str);

        str = NULL;
        hr = IXMLDOMNode_get_namespaceURI(node, &str);
        ok(hr == S_OK, "%d: unexpected hr %#lx\n", i, hr);
        ok(!lstrcmpW(str, _bstr_(ptr->href)), "%d: got uri %s, expected %s\n", i, wine_dbgstr_w(str), ptr->href);
        SysFreeString(str);

        IXMLDOMNode_Release(node);
        free_bstrs();

        i++;
        ptr++;
    }

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    hr = IXMLDOMDocument_createNode(doc, var, _bstr_("e"), NULL, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node2, &IID_IXMLDOMElement, (void**)&el);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node2);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    hr = IXMLDOMDocument_createNode(doc, var, _bstr_("xmlns:a"), _bstr_("http://www.w3.org/2000/xmlns/"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_setAttributeNode(el, (IXMLDOMAttribute*)node, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* for some reason default namespace uri is not reported */
    hr = IXMLDOMNode_get_namespaceURI(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L""), "got uri %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMNode_Release(node);
    IXMLDOMElement_Release(el);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_url(void)
{
    IXMLDOMDocument *doc;
    HRESULT hr;
    BSTR s;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_get_url(doc, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    s = (void *)0xdeadbeef;
    hr = IXMLDOMDocument_get_url(doc, &s);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(s == NULL, "got %s\n", wine_dbgstr_w(s));

    IXMLDOMDocument_Release(doc);
}

static void test_merging_text(void)
{
    IXMLDOMText *nodetext;
    IXMLDOMText *newtext;
    IXMLDOMElement *root;
    IXMLDOMDocument *doc;
    IXMLDOMNode *first;
    HRESULT hr;
    VARIANT v;
    BSTR str;
    int i;

    doc = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_createElement(doc, _bstr_("Testing"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_appendChild(doc, (IXMLDOMNode*)root, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* test xmlAddChild */
    for (i = 0; i < 10; i++)
    {
        str = SysAllocString(L"str1");
        hr = IXMLDOMDocument_createTextNode(doc, str, &nodetext);
        SysFreeString(str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        newtext = NULL;
        hr = IXMLDOMElement_appendChild(root, (IXMLDOMNode*)nodetext, (IXMLDOMNode**)&newtext);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nodetext == newtext, "expected %p, got %p\n", nodetext, newtext);

        IXMLDOMText_Release(newtext);
        IXMLDOMText_Release(nodetext);
    }

    /* test xmlAddPrevSibling */
    hr = IXMLDOMElement_get_firstChild(root, &first);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)first;
    for (i = 0; i < 10; i++)
    {
        str = SysAllocString(L"str2");
        hr = IXMLDOMDocument_createTextNode(doc, str, &nodetext);
        SysFreeString(str);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        newtext = NULL;
        hr = IXMLDOMElement_insertBefore(root, (IXMLDOMNode*)nodetext, v, (IXMLDOMNode**)&newtext);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(nodetext == newtext, "expected %p, got %p\n", nodetext, newtext);

        IXMLDOMText_Release(newtext);
        IXMLDOMText_Release(nodetext);
    }

    IXMLDOMNode_Release(first);
    IXMLDOMElement_Release(root);
    IXMLDOMDocument_Release(doc);

    free_bstrs();
}

static HRESULT WINAPI transformdest_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    BOOL known_iid = IsEqualIID(riid, &IID_IHTMLObjectElement) ||
        IsEqualIID(riid, &IID_transformdest_unknown) ||
        IsEqualIID(riid, &IID_IServiceProvider) ||
        IsEqualIID(riid, &IID_IStream) ||
        IsEqualIID(riid, &IID_ISequentialStream) ||
        IsEqualIID(riid, &IID_IResponse);

    todo_wine_if(IsEqualIID(riid, &IID_IXMLDOMDocument))
    ok(known_iid, "Unexpected riid %s\n", wine_dbgstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI transformdest_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI transformdest_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl transformdestvtbl =
{
    transformdest_QueryInterface,
    transformdest_AddRef,
    transformdest_Release,
};

static void test_transformNodeToObject(void)
{
    IUnknown transformdest = { &transformdestvtbl };
    IXMLDOMDocument *doc, *doc2, *doc3;
    ISequentialStream *sstream;
    LARGE_INTEGER off;
    WCHAR buffer[256];
    IStream *istream;
    VARIANT_BOOL b;
    ULONG nread;
    HRESULT hr;
    VARIANT v;

    doc = create_document(&IID_IXMLDOMDocument);
    doc2 = create_document(&IID_IXMLDOMDocument);
    doc3 = create_document(&IID_IXMLDOMDocument);

    hr = IXMLDOMDocument_loadXML(doc, _bstr_(szTransformXML), &b);
    ok(hr == S_OK, "Failed to load document, hr %#lx.\n", hr);
    hr = IXMLDOMDocument_loadXML(doc2, _bstr_(szTransformSSXML), &b);
    ok(hr == S_OK, "Failed to load document, hr %#lx.\n", hr);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = &transformdest;
    hr = IXMLDOMDocument_transformNodeToObject(doc, (IXMLDOMNode *)doc2, v);
    ok(hr == E_INVALIDARG, "Failed to transform node, hr %#lx.\n", hr);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = NULL;
    hr = IXMLDOMDocument_transformNodeToObject(doc, (IXMLDOMNode *)doc2, v);
    ok(hr == E_INVALIDARG, "Failed to transform node, hr %#lx.\n", hr);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMDocument_transformNodeToObject(doc, (IXMLDOMNode *)doc2, v);
    ok(hr == E_INVALIDARG, "Failed to transform node, hr %#lx.\n", hr);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch *)doc3;
    hr = IXMLDOMDocument_transformNodeToObject(doc, (IXMLDOMNode *)doc2, v);
    ok(hr == S_OK, "Failed to transform node, hr %#lx.\n", hr);

    /* IStream */
    istream = SHCreateMemStream(NULL, 0);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)istream;
    hr = IXMLDOMDocument_transformNodeToObject(doc, (IXMLDOMNode *)doc2, v);
    ok(hr == S_OK, "Failed to transform node, hr %#lx.\n", hr);

    off.QuadPart = 0;
    hr = IStream_Seek(istream, off, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to seek, hr %#lx.\n", hr);

    nread = 0;
    memset(buffer, 0xcc, sizeof(buffer));
    hr = IStream_Read(istream, buffer, sizeof(buffer), &nread);
    ok(hr == S_OK, "Failed to read, hr %#lx.\n", hr);
    buffer[nread/2] = 0;
    ok(buffer[0] == 0xfeff, "got %x\n", buffer[0]);
    ok(compareIgnoreReturns(&buffer[1], _bstr_(szTransformOutput)), "got output %s\n", wine_dbgstr_w(buffer));
    IStream_Release(istream);

    /* ISequentialStream */
    istream = SHCreateMemStream(NULL, 0);
    sstream = NULL;
    hr = IStream_QueryInterface(istream, &IID_ISequentialStream, (void**)&sstream);
    ok(hr == S_OK, "Failed to QI, hr %#lx.\n", hr);

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)sstream;
    hr = IXMLDOMDocument_transformNodeToObject(doc, (IXMLDOMNode *)doc2, v);
    ok(hr == S_OK, "Failed to transform node, hr %#lx.\n", hr);

    off.QuadPart = 0;
    hr = IStream_Seek(istream, off, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to seek, hr %#lx.\n", hr);

    nread = 0;
    memset(buffer, 0xcc, sizeof(buffer));
    hr = ISequentialStream_Read(sstream, buffer, sizeof(buffer), &nread);
    ok(hr == S_OK, "Failed to read, hr %#lx.\n", hr);
    buffer[nread/2] = 0;
    ok(buffer[0] == 0xfeff, "got %x\n", buffer[0]);
    ok(compareIgnoreReturns(&buffer[1], _bstr_(szTransformOutput)), "got output %s\n", wine_dbgstr_w(buffer));
    ISequentialStream_Release(sstream);
    IStream_Release(istream);

    IXMLDOMDocument_Release(doc3);
    IXMLDOMDocument_Release(doc2);
    IXMLDOMDocument_Release(doc);
    free_bstrs();
}

static void test_normalize_attribute_values(void)
{
    IXMLDOMDocument2 *doc;
    VARIANT var;
    HRESULT hr;

    if (!is_clsid_supported(&CLSID_DOMDocument60, &IID_IXMLDOMDocument2))
    {
        win_skip("NormalizeAttributeValues is not supported.\n");
        return;
    }

    doc = create_document_version(60, &IID_IXMLDOMDocument2);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 10;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("NormalizeAttributeValues"), &var);
todo_wine {
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BOOL, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_BOOL(&var) == VARIANT_FALSE, "Unexpected property value.\n");
}
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = VARIANT_TRUE;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("NormalizeAttributeValues"), var);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);

    V_VT(&var) = VT_I2;
    V_I2(&var) = 10;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("NormalizeAttributeValues"), &var);
todo_wine {
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_BOOL, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_BOOL(&var) == VARIANT_TRUE, "Unexpected property value.\n");
}
    IXMLDOMDocument2_Release(doc);
}

static void test_max_element_depth_values(void)
{
    IXMLDOMDocument2 *doc;
    VARIANT var;
    HRESULT hr;

    doc = create_document_version(60, &IID_IXMLDOMDocument2);

    /* The default max element depth value should be 256. */
    V_VT(&var) = VT_UI4;
    V_UI4(&var) = 0xdeadbeef;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("MaxElementDepth"), &var);
todo_wine {
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_I4(&var) == 256, "Unexpected property value.\n");
}

    /* Changes to the depth value should be observable when subsequently retrieved. */
    V_VT(&var) = VT_I4;
    V_I4(&var) = 32;
    hr = IXMLDOMDocument2_setProperty(doc, _bstr_("MaxElementDepth"), var);
    ok(hr == S_OK, "Failed to set property, hr %#lx.\n", hr);

    V_VT(&var) = VT_UI4;
    V_UI4(&var) = 0xdeadbeef;
    hr = IXMLDOMDocument2_getProperty(doc, _bstr_("MaxElementDepth"), &var);
todo_wine {
    ok(hr == S_OK, "Failed to get property value, hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_I4, "Unexpected property value type, vt %d.\n", V_VT(&var));
    ok(V_I4(&var) == 32, "Unexpected property value.\n");
}

    IXMLDOMDocument2_Release(doc);
}

typedef struct _namespace_as_attribute_t {
    const GUID *guid;
    const char *clsid;
} namespace_as_attribute_t;

static const namespace_as_attribute_t namespace_as_attribute_test_data[] = {
    { &CLSID_DOMDocument,   "CLSID_DOMDocument"   },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2"  },
    { &CLSID_DOMDocument26, "CLSID_DOMDocument26" },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30" },
    { 0 }
};

/* see dlls/msxml[46]/tests/domdoc.c */
static void test_namespaces_as_attributes(void)
{
    const namespace_as_attribute_t *entry = namespace_as_attribute_test_data;
    struct test {
        const char *xml;
        int explen;
        const char *names[3];
        const char *prefixes[3];
        const char *basenames[3];
        const char *uris[3];
        const char *texts[3];
        const char *xmls[3];
    };
    static const struct test tests[] = {
        {
            "<a ns:b=\"b attr\" d=\"d attr\" xmlns:ns=\"nshref\" />", 3,
            { "ns:b",   "d",     "xmlns:ns" },  /* nodeName */
            { "ns",     NULL,     "xmlns" },    /* prefix */
            { "b",      "d",      "ns" },       /* baseName */
            { "nshref", NULL,     "" },         /* namespaceURI */
            { "b attr", "d attr", "nshref" },   /* text */
            { "ns:b=\"b attr\"", "d=\"d attr\"", "xmlns:ns=\"nshref\"" }, /* xml */
        },
        /* property only */
        {
            "<a d=\"d attr\" />", 1,
            { "d" },        /* nodeName */
            { NULL },       /* prefix */
            { "d" },        /* baseName */
            { NULL },       /* namespaceURI */
            { "d attr" },   /* text */
            { "d=\"d attr\"" }, /* xml */
        },
        /* namespace only */
        {
            "<a xmlns:ns=\"nshref\" />", 1,
            { "xmlns:ns" }, /* nodeName */
            { "xmlns" },    /* prefix */
            { "ns" },       /* baseName */
            { "" },         /* namespaceURI */
            { "nshref" },   /* text */
            { "xmlns:ns=\"nshref\"" }, /* xml */
        },
        /* default namespace */
        {
            "<a xmlns=\"nshref\" />", 1,
            { "xmlns" },            /* nodeName */
            { "xmlns" },            /* prefix */
            { "" },                 /* baseName */
            { "" },                 /* namespaceURI */
            { "nshref" },           /* text */
            { "xmlns=\"nshref\"" }, /* xml */
        },
        /* no properties or namespaces */
        {
            "<a />", 0,
        },

        { NULL }
    };
    const struct test *test;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMNode *node, *item;
    IXMLDOMDocument *doc;
    VARIANT_BOOL b;
    LONG len, i;
    HRESULT hr;
    BSTR str;

    while (entry->guid)
    {
        if (!is_clsid_supported(entry->guid, &IID_IXMLDOMDocument2))
        {
            entry++;
            continue;
        }

        test = tests;
        while (test->xml) {
            hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
            ok(hr == S_OK, "Unexpected hr %#lx for %s.\n", hr, wine_dbgstr_guid(entry->guid));

            hr = IXMLDOMDocument_loadXML(doc, _bstr_(test->xml), &b);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            node = NULL;
            hr = IXMLDOMDocument_selectSingleNode(doc, _bstr_("a"), &node);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            hr = IXMLDOMNode_get_attributes(node, &map);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            len = -1;
            hr = IXMLDOMNamedNodeMap_get_length(map, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(len == test->explen, "got %ld\n", len);

            item = NULL;
            hr = IXMLDOMNamedNodeMap_get_item(map, test->explen+1, &item);
            ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
            ok(!item, "Item should be NULL\n");

            for (i = 0; i < len; i++)
            {
                item = NULL;
                hr = IXMLDOMNamedNodeMap_get_item(map, i, &item);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

                str = NULL;
                hr = IXMLDOMNode_get_nodeName(item, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(str, _bstr_(test->names[i])), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);

                str = NULL;
                hr = IXMLDOMNode_get_prefix(item, &str);
                if (test->prefixes[i])
                {
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                    ok(!lstrcmpW(str, _bstr_(test->prefixes[i])), "got %s\n", wine_dbgstr_w(str));
                    SysFreeString(str);
                }
                else
                    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

                str = NULL;
                hr = IXMLDOMNode_get_baseName(item, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(str, _bstr_(test->basenames[i])), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);

                str = NULL;
                hr = IXMLDOMNode_get_namespaceURI(item, &str);
                if (test->uris[i])
                {
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                    if (test->prefixes[i] && !strcmp(test->prefixes[i], "xmlns"))
                        ok(!SysStringLen(str), "got %s\n", wine_dbgstr_w(str));
                    else
                        ok(!lstrcmpW(str, _bstr_(test->uris[i])), "got %s\n", wine_dbgstr_w(str));
                    SysFreeString(str);
                }
                else
                    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

                str = NULL;
                hr = IXMLDOMNode_get_text(item, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(str, _bstr_(test->texts[i])), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);

                str = NULL;
                hr = IXMLDOMNode_get_xml(item, &str);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(str, _bstr_(test->xmls[i])), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);

                IXMLDOMNode_Release(item);
            }

            IXMLDOMNamedNodeMap_Release(map);
            IXMLDOMNode_Release(node);
            IXMLDOMDocument_Release(doc);

            test++;
        }

        entry++;
    }
    free_bstrs();
}

static const IID *qi_list[32];
static int qi_count;

static BOOL qi_list_contains(REFIID iid)
{
    int i;

    for (i = 0; i < qi_count; i++)
    {
        if (IsEqualGUID(qi_list[i], iid))
            return TRUE;
    }

    return FALSE;
}

static BOOL qi_list_contains_service(REFIID service, REFIID iid)
{
    int i;

    for (i = 0; i < qi_count; i++)
    {
        if (IsEqualGUID(qi_list[i], service) && IsEqualGUID(qi_list[i + 1], iid))
            return TRUE;
    }

    return FALSE;
}

static HRESULT WINAPI sp_QueryInterface(IServiceProvider *iface, REFIID iid, void **ppv)
{
    if (qi_count < ARRAY_SIZE(qi_list))
        qi_list[qi_count++] = iid;
    else
        ok(0, "qi_list overflow: %d\n", qi_count);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IServiceProvider))
    {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sp_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI sp_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI sp_QueryService(IServiceProvider *iface, REFGUID service,
                                      REFIID iid, void **ppv)
{
    if (IsEqualGUID(service, &SID_SContainerDispatch) ||
        IsEqualGUID(service, &SID_SInternetHostSecurityManager))
    {
        if (qi_count + 1 < ARRAY_SIZE(qi_list))
        {
            qi_list[qi_count++] = service;
            qi_list[qi_count++] = iid;
        }
        else
            ok(0, "qi_list overflow: %d\n", qi_count);
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl sp_vtbl =
{
    sp_QueryInterface,
    sp_AddRef,
    sp_Release,
    sp_QueryService
};

static IServiceProvider sp = { &sp_vtbl };

static void test_load_with_site(void)
{
    char path[MAX_PATH];
    IXMLDOMDocument2 *doc;
    IObjectWithSite *site;
    VARIANT var;
    VARIANT_BOOL b;
    HRESULT hr;

    GetTempPathA(MAX_PATH, path);
    strcat(path, "winetest.xml");
    write_to_file(path, win1252xml);

    doc = create_document(&IID_IXMLDOMDocument2);

    hr = IXMLDOMDocument2_QueryInterface(doc, &IID_IObjectWithSite, (void **)&site);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    qi_count = 0;
    hr = IObjectWithSite_SetSite(site, (IUnknown *)&sp);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(qi_count != 0, "got %d QI calls\n", qi_count);
    todo_wine
    ok(qi_list_contains(&IID_IXMLDOMDocument), "QI(IID_IXMLDOMDocument) was not called\n");
    ok(qi_list_contains(&IID_IHTMLDocument2), "QI(IID_IHTMLDocument2) was not called\n");
    ok(qi_list_contains(&IID_IServiceProvider), "QI(IID_IServiceProvider) was not called\n");
    todo_wine
    ok(qi_list_contains(&IID_IOleClientSite), "QI(IID_IOleClientSite) was not called\n");
    ok(qi_list_contains_service(&SID_SContainerDispatch, &IID_IHTMLDocument2),
       "QI(SID_SContainerDispatch, IID_IHTMLDocument2) was not called\n");
    todo_wine
    ok(qi_list_contains_service(&SID_SInternetHostSecurityManager, &IID_IXMLDOMDocument),
       "QI(SID_SInternetHostSecurityManager, IID_IXMLDOMDocument) was not called\n");

    qi_count = 0;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = _bstr_(path);
    hr = IXMLDOMDocument2_load(doc, var, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);
    ok(qi_count == 0, "got %d QI calls\n", qi_count);

    IXMLDOMDocument2_Release(doc);

    DeleteFileA(path);
    free_bstrs();
}

typedef struct _validate_on_parse_test_t
{
    const GUID *guid;
    const char *clsid;
    HRESULT hr;
} validate_on_parse_test_t;

static const validate_on_parse_test_t validate_on_parse_tests[] =
{
    { &CLSID_DOMDocument,   "CLSID_DOMDocument" ,  E_FAIL },
    { &CLSID_DOMDocument2,  "CLSID_DOMDocument2",  E_FAIL },
    { &CLSID_DOMDocument26, "CLSID_DOMDocument26", E_FAIL },
    { &CLSID_DOMDocument30, "CLSID_DOMDocument30", E_FAIL },
    { &CLSID_DOMDocument40, "CLSID_DOMDocument40", S_OK },
    { &CLSID_DOMDocument60, "CLSID_DOMDocument60", S_OK },
    { 0 }
};

static void test_validate_on_parse_values(void)
{
    const validate_on_parse_test_t *entry = validate_on_parse_tests;
    int i = 0;

    while (entry->guid)
    {
        IXMLDOMDocument2 *doc, *doc_clone;
        IXMLDOMNode *node_clone;
        VARIANT var, var_clone;
        VARIANT_BOOL b;
        HRESULT hr;

        i++;
        if (!is_clsid_supported(entry->guid, &IID_IXMLDOMDocument2))
        {
            entry++;
            continue;
        }

        hr = CoCreateInstance(entry->guid, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
        ok(hr == S_OK, "%d: got %#lx.\n", i, hr);

        V_VT(&var) = VT_I2;
        V_I2(&var) = 0;
        hr = IXMLDOMDocument2_getProperty(doc, _bstr_("ValidateOnParse"), &var);
        ok(hr == entry->hr || broken(hr == E_FAIL) /* win8 */, "%d: got hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(V_VT(&var) == VT_BOOL, "%d: got property value type %d.\n", i, V_VT(&var));
            ok(V_BOOL(&var) == VARIANT_TRUE, "%d: got property value %d.\n", i, V_BOOL(&var));
        }
        else
        {
            ok(V_VT(&var) == VT_I2, "%d: got property value type %d.\n", i, V_VT(&var));
            ok(V_I2(&var) == 0, "%d: got property value %d.\n", i, V_I2(&var));
            if (IsEqualCLSID(entry->guid, &CLSID_DOMDocument40))
            {
                win_skip("Skip tests for CLSID_DOMDocument40 on Windows 8.\n");
                IXMLDOMDocument2_Release(doc);
                entry++;
                continue;
            }
        }

        b = VARIANT_FALSE;
        hr = IXMLDOMDocument2_get_validateOnParse(doc, &b);
        ok(hr == S_OK, "%d: got hr %#lx.\n", i, hr);
        ok(b == VARIANT_TRUE, "%d: got property value %d.\n", i, b);

        V_VT(&var) = VT_I2;
        V_I2(&var) = 0;
        hr = IXMLDOMDocument2_setProperty(doc, _bstr_("ValidateOnParse"), var);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        V_VT(&var) = VT_I2;
        V_I2(&var) = 10;
        hr = IXMLDOMDocument2_getProperty(doc, _bstr_("ValidateOnParse"), &var);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(V_VT(&var) == VT_BOOL, "%d: got property value type %d.\n", i, V_VT(&var));
            ok(V_BOOL(&var) == VARIANT_FALSE, "%d: got property value %d.\n", i, V_BOOL(&var));
        }

        V_VT(&var) = VT_BOOL;
        V_BOOL(&var) = VARIANT_TRUE;
        hr = IXMLDOMDocument2_setProperty(doc, _bstr_("ValidateOnParse"), var);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        b = VARIANT_FALSE;
        hr = IXMLDOMDocument2_get_validateOnParse(doc, &b);
        ok(hr == S_OK, "%d: got hr %#lx.\n", i, hr);
        ok(b == VARIANT_TRUE, "%d: got property value %d.\n", i, b);

        V_VT(&var) = VT_I2;
        V_I2(&var) = 0;
        hr = IXMLDOMDocument2_getProperty(doc, _bstr_("ValidateOnParse"), &var);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(V_VT(&var) == VT_BOOL, "%d: got property value type %d.\n", i, V_VT(&var));
            ok(V_BOOL(&var) == VARIANT_TRUE, "%d: got property value %d.\n", i, V_BOOL(&var));
        }

        b = VARIANT_FALSE;
        hr = IXMLDOMDocument2_put_validateOnParse(doc, b);
        ok(hr == S_OK, "%d: got hr %#lx.\n", i, hr);
        V_VT(&var) = VT_I2;
        V_I2(&var) = 10;
        hr = IXMLDOMDocument2_getProperty(doc, _bstr_("ValidateOnParse"), &var);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(V_VT(&var) == VT_BOOL, "%d: got property value type %d.\n", i, V_VT(&var));
            ok(V_BOOL(&var) == VARIANT_FALSE, "%d: got property value %d.\n", i, V_BOOL(&var));
        }

        /* Cloned document inherits the property. */
        hr = IXMLDOMDocument2_cloneNode(doc, VARIANT_TRUE, &node_clone);
        ok(hr == S_OK, "%d: got hr %#lx.\n", i, hr);
        ok(node_clone != NULL, "%d: got node %p.\n", i, node_clone);
        hr = IXMLDOMNode_QueryInterface(node_clone, &IID_IXMLDOMDocument2, (void **)&doc_clone);
        ok(hr == S_OK, "%d: got hr %#lx.\n", i, hr);
        V_VT(&var) = VT_I2;
        V_I2(&var) = 10;
        hr = IXMLDOMDocument2_getProperty(doc, _bstr_("ValidateOnParse"), &var);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(V_VT(&var) == VT_BOOL, "%d: got property value type %d.\n", i, V_VT(&var));
            ok(V_BOOL(&var) == VARIANT_FALSE, "%d: got property value %d.\n", i, V_BOOL(&var));
        }
        V_VT(&var_clone) = VT_I2;
        V_I2(&var_clone) = 10;
        hr = IXMLDOMDocument2_getProperty(doc_clone, _bstr_("ValidateOnParse"), &var_clone);
        ok(hr == entry->hr, "%d: got hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            ok(V_VT(&var_clone) == V_VT(&var), "%d: got property value type %d.\n", i, V_VT(&var_clone));
            ok(V_BOOL(&var_clone) == V_BOOL(&var), "%d: got property value %d.\n", i, V_BOOL(&var_clone));
        }
        b = VARIANT_TRUE;
        hr = IXMLDOMDocument2_get_validateOnParse(doc_clone, &b);
        ok(hr == S_OK, "%d: got hr %#lx.\n", i, hr);
        ok(b == VARIANT_FALSE, "%d: got property value %d.\n", i, b);
        IXMLDOMNode_Release(node_clone);
        IXMLDOMDocument2_Release(doc_clone);

        IXMLDOMDocument2_Release(doc);
        entry++;
    }
}

static void test_indent(void)
{
    HRESULT hr;
    VARIANT_BOOL b = VARIANT_FALSE;
    BSTR data, str;
    const WCHAR *data_expected;
    IXMLDOMDocument *doc;
    IXMLDOMElement *element = NULL;

    str = SysAllocString(L"<?xml version='1.0' encoding='Windows-1252'?>\n"
                          "<root>\n"
                              "<a>\n"
                                  "<b/>\n"
                              "</a>\n"
                          "</root>\n");
    hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc);
    ok(hr == S_OK, "Unable to create instance hr %#lx.\n", hr);
    hr = IXMLDOMDocument_loadXML(doc, str, &b);
    ok(hr == S_OK, "Unable to load XML hr %#lx.\n", hr);
    hr = IXMLDOMDocument_get_documentElement(doc, &element);
    ok(hr == S_OK, "Unable to get element hr %#lx.\n", hr);
    hr = IXMLDOMElement_get_xml(element, &data);
    ok(hr == S_OK, "Unable to get XML hr %#lx.\n", hr);

    data_expected = L"<root>\r\n"
                         "\t<a>\r\n"
                             "\t\t<b/>\r\n"
                         "\t</a>\r\n"
                     "</root>";
    ok(!lstrcmpW(data, data_expected), "incorrect element string, got '%s'\n", wine_dbgstr_w(data));

    SysFreeString(str);
}

static DWORD WINAPI new_thread(void *arg)
{
    HRESULT hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");
    if (hr != S_OK) return 1;

    test_indent();

    CoUninitialize();
    return 0;
}

START_TEST(domdoc)
{
    HRESULT hr;
    HANDLE thread;

    hr = CoInitialize( NULL );
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK) return;

    get_class_support_data(domdoc_support_data);
    if (!is_clsid_supported(&CLSID_DOMDocument2, &IID_IXMLDOMDocument))
    {
        win_skip("DOMDocument2 is not supported. Skipping all tests.\n");
        CoUninitialize();
        return;
    }

    test_createProcessingInstruction();
    test_load_with_site();
    test_domdoc();
    test_persiststream();
    test_domnode();
    test_refs();
    test_create();
    test_getElementsByTagName();
    test_get_text();
    test_get_childNodes();
    test_get_firstChild();
    test_get_lastChild();
    test_removeChild();
    test_replaceChild();
    test_removeNamedItem();
    test_IXMLDOMDocument2();
    test_whitespace();
    test_XPath();
    test_XSLPattern();
    test_cloneNode();
    test_xmlTypes();
    test_save();
    test_testTransforms();
    test_namespaces_basic();
    test_namespaces_change();
    test_FormattingXML();
    test_nodeTypedValue();
    test_TransformWithLoadingLocalFile();
    test_put_nodeValue();
    test_IObjectSafety();
    test_splitText();
    test_getQualifiedItem();
    test_removeQualifiedItem();
    test_get_ownerDocument();
    test_setAttributeNode();
    test_put_dataType();
    test_createNode();
    test_create_attribute();
    test_get_prefix();
    test_default_properties();
    test_selectSingleNode();
    test_events();
    test_put_nodeTypedValue();
    test_get_xml();
    test_insertBefore();
    test_appendChild();
    test_get_doctype();
    test_get_tagName();
    test_get_dataType();
    test_get_nodeTypeString();
    test_get_attributes();
    test_selection();
    test_load();
    test_dispex();
    test_parseerror();
    test_getAttributeNode();
    test_getAttribute();
    test_supporterrorinfo();
    test_nodeValue();
    test_get_namespaces();
    test_put_data();
    test_newline_normalization();
    test_putref_schemas();
    test_namedmap_newenum();
    test_xmlns_attribute();
    test_url();
    test_merging_text();
    test_transformNodeToObject();
    test_normalize_attribute_values();
    test_namespaces_as_attributes();
    test_validate_on_parse_values();
    test_xsltemplate();
    test_xsltext();
    test_max_element_depth_values();

    if (is_clsid_supported(&CLSID_MXNamespaceManager40, &IID_IMXNamespaceManager))
    {
        test_mxnamespacemanager();
        test_mxnamespacemanager_override();
    }

    /* We need to test test_indent in a seperate thread. This is to prevent regressions in multi-threaded
    applications where the default indentation is set (e.g. by setting xmlTreeIndentString) in the first
    thread but not for new threads, leading to the wrong indentation in subsequent threads. */
    thread = CreateThread(NULL, 0, new_thread, NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    CoUninitialize();
}
