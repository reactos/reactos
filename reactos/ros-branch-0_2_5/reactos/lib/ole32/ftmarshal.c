/*
 *	free threaded marshaller
 *
 *  Copyright 2002  Juergen Schmied
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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct _FTMarshalImpl {
	IUnknownVtbl *lpVtbl;
	DWORD ref;
	IMarshalVtbl *lpvtblFTM;

	IUnknown *pUnkOuter;
} FTMarshalImpl;

#define _IFTMUnknown_(This)(IUnknown*)&(This->lpVtbl)
#define _IFTMarshal_(This) (IMarshal*)&(This->lpvtblFTM)

#define _IFTMarshall_Offset ((int)(&(((FTMarshalImpl*)0)->lpvtblFTM)))
#define _ICOM_THIS_From_IFTMarshal(class, name) class* This = (class*)(((char*)name)-_IFTMarshall_Offset);

/* inner IUnknown to handle aggregation */
HRESULT WINAPI IiFTMUnknown_fnQueryInterface (IUnknown * iface, REFIID riid, LPVOID * ppv)
{

    FTMarshalImpl *This = (FTMarshalImpl *)iface;

    TRACE ("\n");
    *ppv = NULL;

    if (IsEqualIID (&IID_IUnknown, riid))
	*ppv = _IFTMUnknown_ (This);
    else if (IsEqualIID (&IID_IMarshal, riid))
	*ppv = _IFTMarshal_ (This);
    else {
	FIXME ("No interface for %s.\n", debugstr_guid (riid));
	return E_NOINTERFACE;
    }
    IUnknown_AddRef ((IUnknown *) * ppv);
    return S_OK;
}

ULONG WINAPI IiFTMUnknown_fnAddRef (IUnknown * iface)
{

    FTMarshalImpl *This = (FTMarshalImpl *)iface;

    TRACE ("\n");
    return InterlockedIncrement (&This->ref);
}

ULONG WINAPI IiFTMUnknown_fnRelease (IUnknown * iface)
{

    FTMarshalImpl *This = (FTMarshalImpl *)iface;

    TRACE ("\n");
    if (InterlockedDecrement (&This->ref))
	return This->ref;
    HeapFree (GetProcessHeap (), 0, This);
    return 0;
}

static IUnknownVtbl iunkvt =
{
	IiFTMUnknown_fnQueryInterface,
	IiFTMUnknown_fnAddRef,
	IiFTMUnknown_fnRelease
};

HRESULT WINAPI FTMarshalImpl_QueryInterface (LPMARSHAL iface, REFIID riid, LPVOID * ppv)
{

    _ICOM_THIS_From_IFTMarshal (FTMarshalImpl, iface);

    TRACE ("(%p)->(\n\tIID:\t%s,%p)\n", This, debugstr_guid (riid), ppv);
    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppv);
}

ULONG WINAPI FTMarshalImpl_AddRef (LPMARSHAL iface)
{

    _ICOM_THIS_From_IFTMarshal (FTMarshalImpl, iface);

    TRACE ("\n");
    return IUnknown_AddRef (This->pUnkOuter);
}

ULONG WINAPI FTMarshalImpl_Release (LPMARSHAL iface)
{

    _ICOM_THIS_From_IFTMarshal (FTMarshalImpl, iface);

    TRACE ("\n");
    return IUnknown_Release (This->pUnkOuter);
}

HRESULT WINAPI FTMarshalImpl_GetUnmarshalClass (LPMARSHAL iface, REFIID riid, void *pv, DWORD dwDestContext,
						void *pvDestContext, DWORD mshlflags, CLSID * pCid)
{
    FIXME ("(), stub!\n");
    return S_OK;
}

HRESULT WINAPI FTMarshalImpl_GetMarshalSizeMax (LPMARSHAL iface, REFIID riid, void *pv, DWORD dwDestContext,
						void *pvDestContext, DWORD mshlflags, DWORD * pSize)
{

    IMarshal *pMarshal = NULL;
    HRESULT hres;

    _ICOM_THIS_From_IFTMarshal (FTMarshalImpl, iface);

    FIXME ("(), stub!\n");

    /* if the marshalling happens inside the same process the interface pointer is
       copied between the apartments */
    if (dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_CROSSCTX) {
	*pSize = sizeof (This);
	return S_OK;
    }

    /* use the standard marshaller to handle all other cases */
    CoGetStandardMarshal (riid, pv, dwDestContext, pvDestContext, mshlflags, &pMarshal);
    hres = IMarshal_GetMarshalSizeMax (pMarshal, riid, pv, dwDestContext, pvDestContext, mshlflags, pSize);
    IMarshal_Release (pMarshal);
    return hres;

    return S_OK;
}

HRESULT WINAPI FTMarshalImpl_MarshalInterface (LPMARSHAL iface, IStream * pStm, REFIID riid, void *pv,
					       DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{

    IMarshal *pMarshal = NULL;
    HRESULT hres;

    _ICOM_THIS_From_IFTMarshal (FTMarshalImpl, iface);

    FIXME ("(), stub!\n");

    /* if the marshalling happens inside the same process the interface pointer is
       copied between the apartments */
    if (dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_CROSSCTX) {
	return IStream_Write (pStm, This, sizeof (This), 0);
    }

    /* use the standard marshaler to handle all other cases */
    CoGetStandardMarshal (riid, pv, dwDestContext, pvDestContext, mshlflags, &pMarshal);
    hres = IMarshal_MarshalInterface (pMarshal, pStm, riid, pv, dwDestContext, pvDestContext, mshlflags);
    IMarshal_Release (pMarshal);
    return hres;
}

HRESULT WINAPI FTMarshalImpl_UnmarshalInterface (LPMARSHAL iface, IStream * pStm, REFIID riid, void **ppv)
{
    FIXME ("(), stub!\n");
    return S_OK;
}

HRESULT WINAPI FTMarshalImpl_ReleaseMarshalData (LPMARSHAL iface, IStream * pStm)
{
    FIXME ("(), stub!\n");
    return S_OK;
}

HRESULT WINAPI FTMarshalImpl_DisconnectObject (LPMARSHAL iface, DWORD dwReserved)
{
    FIXME ("(), stub!\n");
    return S_OK;
}

IMarshalVtbl ftmvtbl =
{
	FTMarshalImpl_QueryInterface,
	FTMarshalImpl_AddRef,
	FTMarshalImpl_Release,
	FTMarshalImpl_GetUnmarshalClass,
	FTMarshalImpl_GetMarshalSizeMax,
	FTMarshalImpl_MarshalInterface,
	FTMarshalImpl_UnmarshalInterface,
	FTMarshalImpl_ReleaseMarshalData,
	FTMarshalImpl_DisconnectObject
};

/***********************************************************************
 *          CoCreateFreeThreadedMarshaler [OLE32.@]
 *
 */
HRESULT WINAPI CoCreateFreeThreadedMarshaler (LPUNKNOWN punkOuter, LPUNKNOWN * ppunkMarshal)
{

    FTMarshalImpl *ftm;

    TRACE ("(%p %p)\n", punkOuter, ppunkMarshal);

    ftm = (FTMarshalImpl *) HeapAlloc (GetProcessHeap (), 0, sizeof (FTMarshalImpl));
    if (!ftm)
	return E_OUTOFMEMORY;

    ftm->lpVtbl = &iunkvt;
    ftm->lpvtblFTM = &ftmvtbl;
    ftm->ref = 1;
    ftm->pUnkOuter = punkOuter;

    *ppunkMarshal = _IFTMUnknown_ (ftm);
    return S_OK;
}
