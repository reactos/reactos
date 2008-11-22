/*
 * Internet Messaging APIs
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winuser.h"
#include "ole2.h"
#include "mimeole.h"

#include "inetcomm_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcomm);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    static IMimeInternational *international;

    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        if (!InternetTransport_RegisterClass(hinstDLL))
            return FALSE;
        MimeInternational_Construct(&international);
        break;
    case DLL_PROCESS_DETACH:
        IMimeInternational_Release(international);
        InternetTransport_UnregisterClass(hinstDLL);
        break;
    default:
        break;
    }
    return TRUE;
}

/******************************************************************************
 * ClassFactory
 */
typedef struct
{
    const struct IClassFactoryVtbl *lpVtbl;
    HRESULT (*create_object)(IUnknown *, void **);
} cf;

static inline cf *impl_from_IClassFactory( IClassFactory *iface )
{
    return (cf *)((char*)iface - FIELD_OFFSET(cf, lpVtbl));
}

static HRESULT WINAPI cf_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *ppobj )
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

static ULONG WINAPI cf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI cf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI cf_CreateInstance( IClassFactory *iface, LPUNKNOWN pOuter,
                                         REFIID riid, LPVOID *ppobj )
{
    cf *This = impl_from_IClassFactory( iface );
    HRESULT r;
    IUnknown *punk;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    r = This->create_object( pOuter, (LPVOID*) &punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );

    return r;
}

static HRESULT WINAPI cf_LockServer( IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n",iface,dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl cf_vtbl =
{
    cf_QueryInterface,
    cf_AddRef,
    cf_Release,
    cf_CreateInstance,
    cf_LockServer
};

static cf mime_body_cf      = { &cf_vtbl, MimeBody_create };
static cf mime_allocator_cf = { &cf_vtbl, MimeAllocator_create };
static cf mime_message_cf   = { &cf_vtbl, MimeMessage_create };
static cf mime_security_cf  = { &cf_vtbl, MimeSecurity_create };
static cf virtual_stream_cf = { &cf_vtbl, VirtualStream_create };

/***********************************************************************
 *              DllGetClassObject (INETCOMM.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv );

    if (IsEqualCLSID(rclsid, &CLSID_ISMTPTransport))
        return SMTPTransportCF_Create(iid, ppv);

    if (IsEqualCLSID(rclsid, &CLSID_ISMTPTransport2))
        return SMTPTransportCF_Create(iid, ppv);

    if (IsEqualCLSID(rclsid, &CLSID_IIMAPTransport))
        return IMAPTransportCF_Create(iid, ppv);

    if (IsEqualCLSID(rclsid, &CLSID_IPOP3Transport))
        return POP3TransportCF_Create(iid, ppv);

    if ( IsEqualCLSID( rclsid, &CLSID_IMimeSecurity ))
    {
        cf = (IClassFactory*) &mime_security_cf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_IMimeMessage ))
    {
        cf = (IClassFactory*) &mime_message_cf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_IMimeBody ))
    {
        cf = (IClassFactory*) &mime_body_cf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_IMimeAllocator ))
    {
        cf = (IClassFactory*) &mime_allocator_cf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_IVirtualStream ))
    {
        cf = (IClassFactory*) &virtual_stream_cf.lpVtbl;
    }

    if ( !cf )
    {
        FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IClassFactory_QueryInterface( cf, iid, ppv );
}

/***********************************************************************
 *              DllCanUnloadNow (INETCOMM.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}
