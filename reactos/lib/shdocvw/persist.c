/*
 * Implementation of IPersist interfaces for IE Web Browser control
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IPersistStorage interface
 */

static HRESULT WINAPI WBPS_QueryInterface(LPPERSISTSTORAGE iface,
                                          REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPS_AddRef(LPPERSISTSTORAGE iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBPS_Release(LPPERSISTSTORAGE iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT WINAPI WBPS_GetClassID(LPPERSISTSTORAGE iface, CLSID *pClassID)
{
    FIXME("stub: CLSID = %s\n", debugstr_guid(pClassID));
    return S_OK;
}

static HRESULT WINAPI WBPS_IsDirty(LPPERSISTSTORAGE iface)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI WBPS_InitNew(LPPERSISTSTORAGE iface, LPSTORAGE pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return S_OK;
}

static HRESULT WINAPI WBPS_Load(LPPERSISTSTORAGE iface, LPSTORAGE pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return S_OK;
}

static HRESULT WINAPI WBPS_Save(LPPERSISTSTORAGE iface, LPSTORAGE pStg,
                                BOOL fSameAsLoad)
{
    FIXME("stub: LPSTORAGE = %p, fSameAsLoad = %d\n", pStg, fSameAsLoad);
    return S_OK;
}

static HRESULT WINAPI WBPS_SaveCompleted(LPPERSISTSTORAGE iface, LPSTORAGE pStgNew)
{
    FIXME("stub: LPSTORAGE = %p\n", pStgNew);
    return S_OK;
}

/**********************************************************************
 * IPersistStorage virtual function table for IE Web Browser component
 */

static const IPersistStorageVtbl WBPS_Vtbl =
{
    WBPS_QueryInterface,
    WBPS_AddRef,
    WBPS_Release,
    WBPS_GetClassID,
    WBPS_IsDirty,
    WBPS_InitNew,
    WBPS_Load,
    WBPS_Save,
    WBPS_SaveCompleted
};

IPersistStorageImpl SHDOCVW_PersistStorage = {&WBPS_Vtbl};


/**********************************************************************
 * Implement the IPersistStreamInit interface
 */

static HRESULT WINAPI WBPSI_QueryInterface(LPPERSISTSTREAMINIT iface,
                                           REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPSI_AddRef(LPPERSISTSTREAMINIT iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBPSI_Release(LPPERSISTSTREAMINIT iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT WINAPI WBPSI_GetClassID(LPPERSISTSTREAMINIT iface, CLSID *pClassID)
{
    FIXME("stub: CLSID = %s\n", debugstr_guid(pClassID));
    return S_OK;
}

static HRESULT WINAPI WBPSI_IsDirty(LPPERSISTSTREAMINIT iface)
{
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI WBPSI_Load(LPPERSISTSTREAMINIT iface, LPSTREAM pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return S_OK;
}

static HRESULT WINAPI WBPSI_Save(LPPERSISTSTREAMINIT iface, LPSTREAM pStg,
                                BOOL fSameAsLoad)
{
    FIXME("stub: LPSTORAGE = %p, fSameAsLoad = %d\n", pStg, fSameAsLoad);
    return S_OK;
}

static HRESULT WINAPI WBPSI_GetSizeMax(LPPERSISTSTREAMINIT iface,
                                       ULARGE_INTEGER *pcbSize)
{
    FIXME("stub: ULARGE_INTEGER = %p\n", pcbSize);
    return S_OK;
}

static HRESULT WINAPI WBPSI_InitNew(LPPERSISTSTREAMINIT iface)
{
    FIXME("stub\n");
    return S_OK;
}

/**********************************************************************
 * IPersistStreamInit virtual function table for IE Web Browser component
 */

static const IPersistStreamInitVtbl WBPSI_Vtbl =
{
    WBPSI_QueryInterface,
    WBPSI_AddRef,
    WBPSI_Release,
    WBPSI_GetClassID,
    WBPSI_IsDirty,
    WBPSI_Load,
    WBPSI_Save,
    WBPSI_GetSizeMax,
    WBPSI_InitNew
};

IPersistStreamInitImpl SHDOCVW_PersistStreamInit = {&WBPSI_Vtbl};
