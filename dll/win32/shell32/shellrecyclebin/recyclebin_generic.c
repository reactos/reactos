/*
 * PROJECT:     Recycle bin management
 * LICENSE:     GPL v2 - See COPYING in the top level directory
 * FILE:        lib/recyclebin/recyclebin_generic.c
 * PURPOSE:     Deals with a system-wide recycle bin
 * PROGRAMMERS: Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "recyclebin_private.h"

struct RecycleBinGeneric
{
    ULONG ref;
    IRecycleBin recycleBinImpl;
};

static HRESULT STDMETHODCALLTYPE
RecycleBinGeneric_RecycleBin_QueryInterface(
    IRecycleBin *This,
    REFIID riid,
    void **ppvObject)
{
    struct RecycleBinGeneric *s = CONTAINING_RECORD(This, struct RecycleBinGeneric, recycleBinImpl);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppvObject = &s->recycleBinImpl;
    else if (IsEqualIID(riid, &IID_IRecycleBin))
        *ppvObject = &s->recycleBinImpl;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(This);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
RecycleBinGeneric_RecycleBin_AddRef(
    IRecycleBin *This)
{
    struct RecycleBinGeneric *s = CONTAINING_RECORD(This, struct RecycleBinGeneric, recycleBinImpl);
    ULONG refCount = InterlockedIncrement((PLONG)&s->ref);
    TRACE("(%p)\n", This);
    return refCount;
}

static VOID
RecycleBinGeneric_Destructor(
    struct RecycleBinGeneric *s)
{
    TRACE("(%p)\n", s);

    CoTaskMemFree(s);
}

static ULONG STDMETHODCALLTYPE
RecycleBinGeneric_RecycleBin_Release(
    IRecycleBin *This)
{
    struct RecycleBinGeneric *s = CONTAINING_RECORD(This, struct RecycleBinGeneric, recycleBinImpl);
    ULONG refCount;

    TRACE("(%p)\n", This);

    refCount = InterlockedDecrement((PLONG)&s->ref);

    if (refCount == 0)
        RecycleBinGeneric_Destructor(s);

    return refCount;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGeneric_RecycleBin_DeleteFile(
    IN IRecycleBin *This,
    IN LPCWSTR szFileName)
{
    IRecycleBin *prb;
    LPWSTR szFullName = NULL;
    DWORD dwBufferLength = 0;
    DWORD len;
    WCHAR szVolume[MAX_PATH];
    HRESULT hr;

    TRACE("(%p, %s)\n", This, debugstr_w(szFileName));

    /* Get full file name */
    while (TRUE)
    {
        len = GetFullPathNameW(szFileName, dwBufferLength, szFullName, NULL);
        if (len == 0)
        {
            if (szFullName)
                CoTaskMemFree(szFullName);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        else if (len < dwBufferLength)
            break;
        if (szFullName)
            CoTaskMemFree(szFullName);
        dwBufferLength = len;
        szFullName = CoTaskMemAlloc(dwBufferLength * sizeof(WCHAR));
        if (!szFullName)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    /* Get associated volume path */
#ifndef __REACTOS__
    if (!GetVolumePathNameW(szFullName, szVolume, MAX_PATH))
    {
        CoTaskMemFree(szFullName);
        return HRESULT_FROM_WIN32(GetLastError());
    }
#else
    swprintf(szVolume, L"%c:\\", szFullName[0]);
#endif

    /* Skip namespace (if any) */
    if (szVolume[0] == '\\'
     && szVolume[1] == '\\'
     && (szVolume[2] == '.' || szVolume[2] == '?')
     && szVolume[3] == '\\')
    {
        MoveMemory(szVolume, &szVolume[4], (MAX_PATH - 4) * sizeof(WCHAR));
    }

    hr = GetDefaultRecycleBin(szVolume, &prb);
    if (!SUCCEEDED(hr))
    {
        CoTaskMemFree(szFullName);
        return hr;
    }

    hr = IRecycleBin_DeleteFile(prb, szFullName);
    CoTaskMemFree(szFullName);
    IRecycleBin_Release(prb);
    return hr;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGeneric_RecycleBin_EmptyRecycleBin(
    IN IRecycleBin *This)
{
    WCHAR szVolumeName[MAX_PATH];
    DWORD dwLogicalDrives, i;
    IRecycleBin *prb;
    HRESULT hr;

    TRACE("(%p)\n", This);

    dwLogicalDrives = GetLogicalDrives();
    if (dwLogicalDrives == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    for (i = 0; i < 26; i++)
    {
        if (!(dwLogicalDrives & (1 << i)))
            continue;
        swprintf(szVolumeName, L"%c:\\", 'A' + i);
        if (GetDriveTypeW(szVolumeName) != DRIVE_FIXED)
            continue;

        hr = GetDefaultRecycleBin(szVolumeName, &prb);
        if (!SUCCEEDED(hr))
            return hr;

        hr = IRecycleBin_EmptyRecycleBin(prb);
        IRecycleBin_Release(prb);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
RecycleBinGeneric_RecycleBin_EnumObjects(
    IN IRecycleBin *This,
    OUT IRecycleBinEnumList **ppEnumList)
{
    TRACE("(%p, %p)\n", This, ppEnumList);
    return RecycleBinGenericEnum_Constructor(ppEnumList);
}

CONST_VTBL struct IRecycleBinVtbl RecycleBinGenericVtbl =
{
    RecycleBinGeneric_RecycleBin_QueryInterface,
    RecycleBinGeneric_RecycleBin_AddRef,
    RecycleBinGeneric_RecycleBin_Release,
    RecycleBinGeneric_RecycleBin_DeleteFile,
    RecycleBinGeneric_RecycleBin_EmptyRecycleBin,
    RecycleBinGeneric_RecycleBin_EnumObjects,
};

HRESULT RecycleBinGeneric_Constructor(OUT IUnknown **ppUnknown)
{
    /* This RecycleBin implementation was introduced to be able to manage all
     * drives at once, and instanciate the 'real' implementations when needed */
    struct RecycleBinGeneric *s;

    s = CoTaskMemAlloc(sizeof(struct RecycleBinGeneric));
    if (!s)
        return E_OUTOFMEMORY;
    s->ref = 1;
    s->recycleBinImpl.lpVtbl = &RecycleBinGenericVtbl;

    *ppUnknown = (IUnknown *)&s->recycleBinImpl;
    return S_OK;
}
