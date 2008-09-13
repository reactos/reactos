/*
 *	OLE32 proxy/stub handler
 *
 *  Copyright 2002  Marcus Meissner
 *  Copyright 2001  Ove Kåven, TransGaming Technologies
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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "rpc.h"
#include "rpcproxy.h"

#include "compobj_private.h"
#include "moniker.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static CStdPSFactoryBuffer PSFactoryBuffer;

CSTDSTUBBUFFERRELEASE(&PSFactoryBuffer)

extern const ExtendedProxyFileInfo dcom_ProxyFileInfo;
extern const ExtendedProxyFileInfo ole32_objidl_ProxyFileInfo;
extern const ExtendedProxyFileInfo ole32_oleidl_ProxyFileInfo;
extern const ExtendedProxyFileInfo ole32_unknwn_ProxyFileInfo;

static const ProxyFileInfo *OLE32_ProxyFileList[] = {
  &dcom_ProxyFileInfo,
  &ole32_objidl_ProxyFileInfo,
  &ole32_oleidl_ProxyFileInfo,
  &ole32_unknwn_ProxyFileInfo,
  NULL
};

/***********************************************************************
 *           DllGetClassObject [OLE32.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(rclsid,&CLSID_DfMarshal)&&(
		IsEqualIID(iid,&IID_IClassFactory) ||
		IsEqualIID(iid,&IID_IUnknown)
	)
    )
	return MARSHAL_GetStandardMarshalCF(ppv);
    if (IsEqualIID(rclsid,&CLSID_StdGlobalInterfaceTable) && (IsEqualIID(iid,&IID_IClassFactory) || IsEqualIID(iid,&IID_IUnknown)))
        return StdGlobalInterfaceTable_GetFactory(ppv);
    if (IsEqualCLSID(rclsid, &CLSID_FileMoniker))
        return FileMonikerCF_Create(iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_ItemMoniker))
        return ItemMonikerCF_Create(iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_AntiMoniker))
        return AntiMonikerCF_Create(iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_CompositeMoniker))
        return CompositeMonikerCF_Create(iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_ClassMoniker))
        return ClassMonikerCF_Create(iid, ppv);
    if (IsEqualCLSID(rclsid, &CLSID_PointerMoniker))
        return PointerMonikerCF_Create(iid, ppv);

    return NdrDllGetClassObject(rclsid, iid, ppv, OLE32_ProxyFileList,
                                &CLSID_PSFactoryBuffer, &PSFactoryBuffer);
}
