/*
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "storage32.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

#define _IPropertySetStorage_Offset ((int)(&(((StorageImpl*)0)->base.pssVtbl)))
#define _ICOM_THIS_From_IPropertySetStorage(class, name) \
    class* This = (class*)(((char*)name)-_IPropertySetStorage_Offset)

static IPropertyStorageVtbl IPropertyStorage_Vtbl;

/***********************************************************************
 * Implementation of IPropertyStorage
 */
typedef struct tagPropertyStorage_impl
{
    IPropertyStorageVtbl *vtbl;
    DWORD ref;
    IStream *stm;
} PropertyStorage_impl;

/************************************************************************
 * IPropertyStorage_fnQueryInterface (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnQueryInterface(
    IPropertyStorage *iface,
    REFIID riid,
    void** ppvObject)
{
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;

    if ( (This==0) || (ppvObject==0) )
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IPropertyStorage, riid))
    {
        IPropertyStorage_AddRef(iface);
        *ppvObject = (IPropertyStorage*)iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

/************************************************************************
 * IPropertyStorage_fnAddRef (IPropertyStorage)
 */
static ULONG WINAPI IPropertyStorage_fnAddRef(
    IPropertyStorage *iface)
{
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * IPropertyStorage_fnRelease (IPropertyStorage)
 */
static ULONG WINAPI IPropertyStorage_fnRelease(
    IPropertyStorage *iface)
{
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        IStream_Release(This->stm);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/************************************************************************
 * IPropertyStorage_fnReadMultiple (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnReadMultiple(
    IPropertyStorage* iface,
    ULONG cpspec,
    const PROPSPEC rgpspec[],
    PROPVARIANT rgpropvar[])
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnWriteMultiple (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnWriteMultiple(
    IPropertyStorage* iface,
    ULONG cpspec,
    const PROPSPEC rgpspec[],
    const PROPVARIANT rgpropvar[],
    PROPID propidNameFirst)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnDeleteMultiple (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnDeleteMultiple(
    IPropertyStorage* iface,
    ULONG cpspec,
    const PROPSPEC rgpspec[])
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnReadPropertyNames (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnReadPropertyNames(
    IPropertyStorage* iface,
    ULONG cpropid,
    const PROPID rgpropid[],
    LPOLESTR rglpwstrName[])
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnWritePropertyNames (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnWritePropertyNames(
    IPropertyStorage* iface,
    ULONG cpropid,
    const PROPID rgpropid[],
    const LPOLESTR rglpwstrName[])
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnDeletePropertyNames (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnDeletePropertyNames(
    IPropertyStorage* iface,
    ULONG cpropid,
    const PROPID rgpropid[])
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnCommit (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnCommit(
    IPropertyStorage* iface,
    DWORD grfCommitFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnRevert (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnRevert(
    IPropertyStorage* iface)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnEnum (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnEnum(
    IPropertyStorage* iface,
    IEnumSTATPROPSTG** ppenum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnSetTimes (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnSetTimes(
    IPropertyStorage* iface,
    const FILETIME* pctime,
    const FILETIME* patime,
    const FILETIME* pmtime)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnSetClass (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnSetClass(
    IPropertyStorage* iface,
    REFCLSID clsid)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnStat (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnStat(
    IPropertyStorage* iface,
    STATPROPSETSTG* statpsstg)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/***********************************************************************
 * PropertyStorage_Contruct
 */
static HRESULT PropertyStorage_Contruct(IStream *stm, IPropertyStorage** pps)
{
    PropertyStorage_impl *ps;

    ps = HeapAlloc(GetProcessHeap(), 0, sizeof *ps);
    if (!ps)
        return E_OUTOFMEMORY;

    ps->vtbl = &IPropertyStorage_Vtbl;
    ps->ref = 1;
    ps->stm = stm;

    *pps = (IPropertyStorage*)ps;

    TRACE("PropertyStorage %p constructed\n", ps);

    return S_OK;
}


/***********************************************************************
 * Implementation of IPropertySetStorage
 */

static LPCWSTR format_id_to_name(REFFMTID rfmtid)
{
    static const WCHAR szSummaryInfo[] = { 5,'S','u','m','m','a','r','y',
        'I','n','f','o','r','m','a','t','i','o','n',0 };

    if (IsEqualGUID(&FMTID_SummaryInformation, rfmtid))
        return szSummaryInfo;
    ERR("Unknown format id %s\n", debugstr_guid(rfmtid));
    return NULL;
}

/************************************************************************
 * IPropertySetStorage_fnQueryInterface (IUnknown)
 *
 *  This method forwards to the common QueryInterface implementation
 */
static HRESULT WINAPI IPropertySetStorage_fnQueryInterface(
    IPropertySetStorage *ppstg,
    REFIID riid,
    void** ppvObject)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    return StorageBaseImpl_QueryInterface( (IStorage*)This, riid, ppvObject );
}

/************************************************************************
 * IPropertySetStorage_fnAddRef (IUnknown)
 *
 *  This method forwards to the common AddRef implementation
 */
static ULONG WINAPI IPropertySetStorage_fnAddRef(
    IPropertySetStorage *ppstg)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    return StorageBaseImpl_AddRef( (IStorage*)This );
}

/************************************************************************
 * IPropertySetStorage_fnRelease (IUnknown)
 *
 *  This method forwards to the common Release implementation
 */
static ULONG WINAPI IPropertySetStorage_fnRelease(
    IPropertySetStorage *ppstg)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    return StorageBaseImpl_Release( (IStorage*)This );
}

/************************************************************************
 * IPropertySetStorage_fnCreate (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnCreate(
    IPropertySetStorage *ppstg,
    REFFMTID rfmtid,
    const CLSID* pclsid,
    DWORD grfFlags,
    DWORD grfMode,
    IPropertyStorage** ppprstg)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    LPCWSTR name = NULL;
    IStream *stm = NULL;
    HRESULT r;

    TRACE("%p %s %08lx %p\n", This, debugstr_guid(rfmtid), grfMode, ppprstg);

    /* be picky */
    if (grfMode != (STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE))
        return STG_E_INVALIDFLAG;

    if (!rfmtid)
        return E_INVALIDARG;

    name = format_id_to_name(rfmtid);
    if (!name)
        return STG_E_FILENOTFOUND;

    r = IStorage_CreateStream( (IStorage*)This, name, grfMode, 0, 0, &stm );
    if (FAILED(r))
        return r;

    return PropertyStorage_Contruct(stm, ppprstg);
}

/************************************************************************
 * IPropertySetStorage_fnOpen (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnOpen(
    IPropertySetStorage *ppstg,
    REFFMTID rfmtid,
    DWORD grfMode,
    IPropertyStorage** ppprstg)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    IStream *stm = NULL;
    LPCWSTR name = NULL;
    HRESULT r;

    TRACE("%p %s %08lx %p\n", This, debugstr_guid(rfmtid), grfMode, ppprstg);

    /* be picky */
    if (grfMode != (STGM_READWRITE|STGM_SHARE_EXCLUSIVE) &&
        grfMode != (STGM_READ|STGM_SHARE_EXCLUSIVE))
        return STG_E_INVALIDFLAG;

    if (!rfmtid)
        return E_INVALIDARG;

    name = format_id_to_name(rfmtid);
    if (!name)
        return STG_E_FILENOTFOUND;

    r = IStorage_OpenStream((IStorage*) This, name, 0, grfMode, 0, &stm );
    if (FAILED(r))
        return r;

    return PropertyStorage_Contruct(stm, ppprstg);
}

/************************************************************************
 * IPropertySetStorage_fnDelete (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnDelete(
    IPropertySetStorage *ppstg,
    REFFMTID rfmtid)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    IStorage *stg = NULL;
    LPCWSTR name = NULL;

    TRACE("%p %s\n", This, debugstr_guid(rfmtid));

    if (!rfmtid)
        return E_INVALIDARG;

    name = format_id_to_name(rfmtid);
    if (!name)
        return STG_E_FILENOTFOUND;

    stg = (IStorage*) This;
    return IStorage_DestroyElement(stg, name);
}

/************************************************************************
 * IPropertySetStorage_fnEnum (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnEnum(
    IPropertySetStorage *ppstg,
    IEnumSTATPROPSETSTG** ppenum)
{
    _ICOM_THIS_From_IPropertySetStorage(StorageImpl, ppstg);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}


/***********************************************************************
 * vtables
 */
IPropertySetStorageVtbl IPropertySetStorage_Vtbl =
{
    IPropertySetStorage_fnQueryInterface,
    IPropertySetStorage_fnAddRef,
    IPropertySetStorage_fnRelease,
    IPropertySetStorage_fnCreate,
    IPropertySetStorage_fnOpen,
    IPropertySetStorage_fnDelete,
    IPropertySetStorage_fnEnum
};

static IPropertyStorageVtbl IPropertyStorage_Vtbl =
{
    IPropertyStorage_fnQueryInterface,
    IPropertyStorage_fnAddRef,
    IPropertyStorage_fnRelease,
    IPropertyStorage_fnReadMultiple,
    IPropertyStorage_fnWriteMultiple,
    IPropertyStorage_fnDeleteMultiple,
    IPropertyStorage_fnReadPropertyNames,
    IPropertyStorage_fnWritePropertyNames,
    IPropertyStorage_fnDeletePropertyNames,
    IPropertyStorage_fnCommit,
    IPropertyStorage_fnRevert,
    IPropertyStorage_fnEnum,
    IPropertyStorage_fnSetTimes,
    IPropertyStorage_fnSetClass,
    IPropertyStorage_fnStat,
};
