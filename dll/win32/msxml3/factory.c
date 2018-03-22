/*
 *    MSXML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2005 Mike McCormack
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

#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml.h"
#include "msxml2.h"
#include "xmlparser.h"

/* undef the #define in msxml2 so that we can access the v.2 version
   independent CLSID as well as the v.3 one. */
#undef CLSID_DOMDocument

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef HRESULT (*ClassFactoryCreateInstanceFunc)(void**);
typedef HRESULT (*DOMFactoryCreateInstanceFunc)(MSXML_VERSION, void**);

struct clsid_version_t
{
    const GUID *clsid;
    MSXML_VERSION version;
};

static const struct clsid_version_t clsid_versions_table[] =
{
    { &CLSID_DOMDocument,   MSXML_DEFAULT },
    { &CLSID_DOMDocument2,  MSXML2  },
    { &CLSID_DOMDocument26, MSXML26 },
    { &CLSID_DOMDocument30, MSXML3  },
    { &CLSID_DOMDocument40, MSXML4  },
    { &CLSID_DOMDocument60, MSXML6  },

    { &CLSID_DOMFreeThreadedDocument,   MSXML_DEFAULT },
    { &CLSID_FreeThreadedDOMDocument,   MSXML_DEFAULT },
    { &CLSID_FreeThreadedDOMDocument26, MSXML26 },
    { &CLSID_FreeThreadedDOMDocument30, MSXML3  },
    { &CLSID_FreeThreadedDOMDocument40, MSXML4  },
    { &CLSID_FreeThreadedDOMDocument60, MSXML6  },

    { &CLSID_XMLSchemaCache,   MSXML_DEFAULT },
    { &CLSID_XMLSchemaCache26, MSXML26 },
    { &CLSID_XMLSchemaCache30, MSXML3  },
    { &CLSID_XMLSchemaCache40, MSXML4  },
    { &CLSID_XMLSchemaCache60, MSXML6  },

    { &CLSID_MXXMLWriter,   MSXML_DEFAULT },
    { &CLSID_MXXMLWriter30, MSXML3 },
    { &CLSID_MXXMLWriter40, MSXML4 },
    { &CLSID_MXXMLWriter60, MSXML6 },

    { &CLSID_SAXXMLReader,   MSXML_DEFAULT },
    { &CLSID_SAXXMLReader30, MSXML3 },
    { &CLSID_SAXXMLReader40, MSXML4 },
    { &CLSID_SAXXMLReader60, MSXML6 },

    { &CLSID_SAXAttributes,   MSXML_DEFAULT },
    { &CLSID_SAXAttributes30, MSXML3 },
    { &CLSID_SAXAttributes40, MSXML4 },
    { &CLSID_SAXAttributes60, MSXML6 },

    { &CLSID_XMLView, MSXML_DEFAULT }
};

static MSXML_VERSION get_msxml_version(const GUID *clsid)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(clsid_versions_table); i++)
        if (IsEqualGUID(clsid, clsid_versions_table[i].clsid))
            return clsid_versions_table[i].version;

    ERR("unknown clsid=%s\n", debugstr_guid(clsid));
    return MSXML_DEFAULT;
}

/******************************************************************************
 * MSXML ClassFactory
 */
typedef struct
{
    IClassFactory IClassFactory_iface;
    ClassFactoryCreateInstanceFunc pCreateInstance;
} ClassFactory;

typedef struct
{
    IClassFactory IClassFactory_iface;
    LONG ref;
    DOMFactoryCreateInstanceFunc pCreateInstance;
    MSXML_VERSION version;
} DOMFactory;

static inline ClassFactory *ClassFactory_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactory, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(
    IClassFactory *iface,
    REFIID riid,
    void **ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(
    IClassFactory *iface,
    IUnknown *pOuter,
    REFIID riid,
    void **ppobj )
{
    ClassFactory *This = ClassFactory_from_IClassFactory(iface);
    IUnknown *punk;
    HRESULT r;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pCreateInstance( (void**) &punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );
    return r;
}

static HRESULT WINAPI ClassFactory_LockServer(
    IClassFactory *iface,
    BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n",iface,dolock);
    return S_OK;
}

static inline DOMFactory *DOMFactory_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, DOMFactory, IClassFactory_iface);
}

static ULONG WINAPI DOMClassFactory_AddRef(IClassFactory *iface )
{
    DOMFactory *This = DOMFactory_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI DOMClassFactory_Release(IClassFactory *iface )
{
    DOMFactory *This = DOMFactory_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref = %u\n", This, ref);
    if(!ref) {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI DOMClassFactory_CreateInstance(
    IClassFactory *iface,
    IUnknown *pOuter,
    REFIID riid,
    void **ppobj )
{
    DOMFactory *This = DOMFactory_from_IClassFactory(iface);
    IUnknown *punk;
    HRESULT r;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pCreateInstance( This->version, (void**) &punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );
    return r;
}

static const struct IClassFactoryVtbl ClassFactoryVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static const struct IClassFactoryVtbl DOMClassFactoryVtbl =
{
    ClassFactory_QueryInterface,
    DOMClassFactory_AddRef,
    DOMClassFactory_Release,
    DOMClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT DOMClassFactory_Create(const GUID *clsid, REFIID riid, void **ppv, DOMFactoryCreateInstanceFunc fnCreateInstance)
{
    DOMFactory *ret = heap_alloc(sizeof(DOMFactory));
    HRESULT hres;

    ret->IClassFactory_iface.lpVtbl = &DOMClassFactoryVtbl;
    ret->ref = 0;
    ret->version = get_msxml_version(clsid);
    ret->pCreateInstance = fnCreateInstance;

    hres = IClassFactory_QueryInterface(&ret->IClassFactory_iface, riid, ppv);
    if(FAILED(hres)) {
        heap_free(ret);
        *ppv = NULL;
    }
    return hres;
}

static ClassFactory xmldoccf = { { &ClassFactoryVtbl }, XMLDocument_create };
static ClassFactory httpreqcf = { { &ClassFactoryVtbl }, XMLHTTPRequest_create };
static ClassFactory serverhttp = { { &ClassFactoryVtbl }, ServerXMLHTTP_create };
static ClassFactory xsltemplatecf = { { &ClassFactoryVtbl }, XSLTemplate_create };
static ClassFactory mxnsmanagercf = { {&ClassFactoryVtbl }, MXNamespaceManager_create };
static ClassFactory xmlparsercf = { { &ClassFactoryVtbl }, XMLParser_create };
static ClassFactory xmlviewcf = { { &ClassFactoryVtbl }, XMLView_create };

/******************************************************************
 *		DllGetClassObject (MSXML3.@)
 */
HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, void **ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv );

    if( IsEqualCLSID( rclsid, &CLSID_DOMDocument )  ||  /* Version indep. v 2.x */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument2 ) ||  /* Version indep. v 3.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument26 )||  /* Version dep.   v 2.6 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument30 )||  /* Version dep.   v 3.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument40 )||  /* Version dep.   v 4.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument60 ))   /* Version dep.   v 6.0 */
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, DOMDocument_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache )   ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache26 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache30 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache40 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache60 ))
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, SchemaCache_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLDocument ) )
    {
        cf = &xmldoccf.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_DOMFreeThreadedDocument )   ||   /* Version indep. v 2.x */
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument )   ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument26 ) ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument30 ) ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument40 ) ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument60 ))
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, DOMDocument_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_SAXXMLReader) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader30 ) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader40 ) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader60 ))
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, SAXXMLReader_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLHTTPRequest ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP26 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP30 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP40 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP60 ))
    {
        cf = &httpreqcf.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_ServerXMLHTTP ) ||
             IsEqualCLSID( rclsid, &CLSID_ServerXMLHTTP30 ) ||
             IsEqualCLSID( rclsid, &CLSID_ServerXMLHTTP40 ) ||
             IsEqualCLSID( rclsid, &CLSID_ServerXMLHTTP60 ))
    {
        cf = &serverhttp.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XSLTemplate )   ||
             IsEqualCLSID( rclsid, &CLSID_XSLTemplate26 ) ||
             IsEqualCLSID( rclsid, &CLSID_XSLTemplate30 ) ||
             IsEqualCLSID( rclsid, &CLSID_XSLTemplate40 ) ||
             IsEqualCLSID( rclsid, &CLSID_XSLTemplate60 ))
    {
        cf = &xsltemplatecf.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_MXXMLWriter )   ||
             IsEqualCLSID( rclsid, &CLSID_MXXMLWriter30 ) ||
             IsEqualCLSID( rclsid, &CLSID_MXXMLWriter40 ) ||
             IsEqualCLSID( rclsid, &CLSID_MXXMLWriter60 ) )
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, MXWriter_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_SAXAttributes) ||
             IsEqualCLSID( rclsid, &CLSID_SAXAttributes30 ) ||
             IsEqualCLSID( rclsid, &CLSID_SAXAttributes40 ) ||
             IsEqualCLSID( rclsid, &CLSID_SAXAttributes60 ))
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, SAXAttributes_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_MXNamespaceManager ) ||
             IsEqualCLSID( rclsid, &CLSID_MXNamespaceManager40 ) ||
             IsEqualCLSID( rclsid, &CLSID_MXNamespaceManager60 ) )
    {
        cf = &mxnsmanagercf.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLParser )  ||
             IsEqualCLSID( rclsid, &CLSID_XMLParser26 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLParser30 )  )
    {
        cf = &xmlparsercf.IClassFactory_iface;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLView ) )
    {
        cf = &xmlviewcf.IClassFactory_iface;
    }

    if ( !cf )
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, riid, ppv );
}
