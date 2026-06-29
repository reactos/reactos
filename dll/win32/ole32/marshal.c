/*
 *	Marshalling library
 *
 * Copyright 2002 Marcus Meissner
 * Copyright 2004 Mike Hearn, for CodeWeavers
 * Copyright 2004 Rob Shearman, for CodeWeavers
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

#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"

#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HRESULT WINAPI InternalCoStdMarshalObject(REFIID riid, DWORD dest_context, void *dest_context_data, void **ppvObject);

static HRESULT WINAPI StdMarshalCF_QueryInterface(LPCLASSFACTORY iface,
                                                  REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI StdMarshalCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI StdMarshalCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI StdMarshalCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMarshal))
        return InternalCoStdMarshalObject(riid, 0, NULL, ppv);

    FIXME("(%s), not supported.\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI StdMarshalCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl StdMarshalCFVtbl =
{
    StdMarshalCF_QueryInterface,
    StdMarshalCF_AddRef,
    StdMarshalCF_Release,
    StdMarshalCF_CreateInstance,
    StdMarshalCF_LockServer
};
static const IClassFactoryVtbl *StdMarshalCF = &StdMarshalCFVtbl;

HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv)
{
    *ppv = &StdMarshalCF;
    return S_OK;
}
