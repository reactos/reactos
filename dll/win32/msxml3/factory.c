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
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml.h"
#include "xmldom.h"
#include "msxml2.h"

/* undef the #define in msxml2 so that we can access the v.2 version
   independent CLSID as well as the v.3 one. */
#undef CLSID_DOMDocument

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef HRESULT (*fnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);

/******************************************************************************
 * MSXML ClassFactory
 */
typedef struct _xmlcf
{
    const struct IClassFactoryVtbl *lpVtbl;
    fnCreateInstance pfnCreateInstance;
} xmlcf;

static inline xmlcf *impl_from_IClassFactory( IClassFactory *iface )
{
    return (xmlcf *)((char*)iface - FIELD_OFFSET(xmlcf, lpVtbl));
}

static HRESULT WINAPI xmlcf_QueryInterface(
    IClassFactory *iface,
    REFIID riid,
    LPVOID *ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI xmlcf_AddRef(
    IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI xmlcf_Release(
    IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI xmlcf_CreateInstance(
    IClassFactory *iface,
    LPUNKNOWN pOuter,
    REFIID riid,
    LPVOID *ppobj )
{
    xmlcf *This = impl_from_IClassFactory( iface );
    HRESULT r;
    IUnknown *punk;
    
    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pfnCreateInstance( pOuter, (LPVOID*) &punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );
    return r;
}

static HRESULT WINAPI xmlcf_LockServer(
    IClassFactory *iface,
    BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n",iface,dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl xmlcf_vtbl =
{
    xmlcf_QueryInterface,
    xmlcf_AddRef,
    xmlcf_Release,
    xmlcf_CreateInstance,
    xmlcf_LockServer
};

static xmlcf domdoccf = { &xmlcf_vtbl, DOMDocument_create };
static xmlcf schemacf = { &xmlcf_vtbl, SchemaCache_create };
static xmlcf xmldoccf = { &xmlcf_vtbl, XMLDocument_create };
static xmlcf saxreadcf = { &xmlcf_vtbl, SAXXMLReader_create };

/******************************************************************
 *		DllGetClassObject (MSXML3.@)
 */
HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv );

    if( IsEqualCLSID( rclsid, &CLSID_DOMDocument ) ||   /* Version indep. v 2.x */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument2 ) ||  /* Version indep. v 3.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument30 ) )  /* Version dep.   v 3.0 */
    {
        cf = (IClassFactory*) &domdoccf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache30 ) )
    {
        cf = (IClassFactory*) &schemacf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLDocument ) )
    {
        cf = (IClassFactory*) &xmldoccf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_DOMFreeThreadedDocument ) ||   /* Version indep. v 2.x */
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument ) )
    {
        cf = (IClassFactory*) &domdoccf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_SAXXMLReader) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader30 ))
    {
        cf = (IClassFactory*) &saxreadcf.lpVtbl;
    }

    if ( !cf )
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, ppv );
}
