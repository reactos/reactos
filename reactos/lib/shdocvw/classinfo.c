/*
 * Implementation of IProvideClassInfo interfaces for IE Web Browser control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "shdocvw.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IProvideClassInfo interface
 *
 * FIXME: Should we just pass in the IProvideClassInfo2 methods rather
 *        reimplementing them here?
 */

static HRESULT WINAPI WBPCI_QueryInterface(LPPROVIDECLASSINFO iface,
                                           REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;

    return E_NOINTERFACE;
}

static ULONG WINAPI WBPCI_AddRef(LPPROVIDECLASSINFO iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBPCI_Release(LPPROVIDECLASSINFO iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

/* Return an ITypeInfo interface to retrieve type library info about
 * this control.
 */
static HRESULT WINAPI WBPCI_GetClassInfo(LPPROVIDECLASSINFO iface, LPTYPEINFO *ppTI)
{
    FIXME("stub: LPTYPEINFO = %p\n", *ppTI);
    return S_OK;
}

/**********************************************************************
 * IProvideClassInfo virtual function table for IE Web Browser component
 */

static IProvideClassInfoVtbl WBPCI_Vtbl =
{
    WBPCI_QueryInterface,
    WBPCI_AddRef,
    WBPCI_Release,
    WBPCI_GetClassInfo
};

IProvideClassInfoImpl SHDOCVW_ProvideClassInfo = { &WBPCI_Vtbl};


/**********************************************************************
 * Implement the IProvideClassInfo2 interface (inherits from
 * IProvideClassInfo).
 */

static HRESULT WINAPI WBPCI2_QueryInterface(LPPROVIDECLASSINFO2 iface,
                                            REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPCI2_AddRef(LPPROVIDECLASSINFO2 iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBPCI2_Release(LPPROVIDECLASSINFO2 iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

/* Return an ITypeInfo interface to retrieve type library info about
 * this control.
 */
static HRESULT WINAPI WBPCI2_GetClassInfo(LPPROVIDECLASSINFO2 iface, LPTYPEINFO *ppTI)
{
    FIXME("stub: LPTYPEINFO = %p\n", *ppTI);
    return S_OK;
}

/* Get the IID for generic default event callbacks.  This IID will
 * in theory be used to later query for an IConnectionPoint to connect
 * an event sink (callback implementation in the OLE control site)
 * to this control.
*/
static HRESULT WINAPI WBPCI2_GetGUID(LPPROVIDECLASSINFO2 iface,
                                     DWORD dwGuidKind, GUID *pGUID)
{
    FIXME("stub: dwGuidKind = %ld, pGUID = %s\n", dwGuidKind, debugstr_guid(pGUID));

    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        FIXME ("Requested unsupported GUID type: %ld\n", dwGuidKind);
        return E_FAIL;  /* Is there a better return type here? */
    }

    /* FIXME: Returning IPropertyNotifySink interface, but should really
     * return a more generic event set (???) dispinterface.
     * However, this hack, allows a control site to return with success
     * (MFC's COleControlSite falls back to older IProvideClassInfo interface
     * if GetGUID() fails to return a non-NULL GUID).
     */
    memcpy(pGUID, &IID_IPropertyNotifySink, sizeof(GUID));
    FIXME("Wrongly returning IPropertyNotifySink interface %s\n",
          debugstr_guid(pGUID));

    return S_OK;
}

/**********************************************************************
 * IProvideClassInfo virtual function table for IE Web Browser component
 */

static IProvideClassInfo2Vtbl WBPCI2_Vtbl =
{
    WBPCI2_QueryInterface,
    WBPCI2_AddRef,
    WBPCI2_Release,
    WBPCI2_GetClassInfo,
    WBPCI2_GetGUID
};

IProvideClassInfo2Impl SHDOCVW_ProvideClassInfo2 = { &WBPCI2_Vtbl};
