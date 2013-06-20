/*
 * Queue Manager (BITS) File Enumerator
 *
 * Copyright 2007, 2008 Google (Roy Shea, Dan Hipschman)
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

#include "qmgr.h"
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

typedef struct
{
    IEnumBackgroundCopyFiles IEnumBackgroundCopyFiles_iface;
    LONG ref;
    IBackgroundCopyFile **files;
    ULONG numFiles;
    ULONG indexFiles;
} EnumBackgroundCopyFilesImpl;

static inline EnumBackgroundCopyFilesImpl *impl_from_IEnumBackgroundCopyFiles(IEnumBackgroundCopyFiles *iface)
{
    return CONTAINING_RECORD(iface, EnumBackgroundCopyFilesImpl, IEnumBackgroundCopyFiles_iface);
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_QueryInterface(IEnumBackgroundCopyFiles *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumBackgroundCopyFiles))
    {
        *ppv = iface;
        IEnumBackgroundCopyFiles_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BITS_IEnumBackgroundCopyFiles_AddRef(IEnumBackgroundCopyFiles *iface)
{
    EnumBackgroundCopyFilesImpl *This = impl_from_IEnumBackgroundCopyFiles(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI BITS_IEnumBackgroundCopyFiles_Release(IEnumBackgroundCopyFiles *iface)
{
    EnumBackgroundCopyFilesImpl *This = impl_from_IEnumBackgroundCopyFiles(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    ULONG i;

    if (ref == 0)
    {
        for(i = 0; i < This->numFiles; i++)
            IBackgroundCopyFile_Release(This->files[i]);
        HeapFree(GetProcessHeap(), 0, This->files);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* Return reference to one or more files in the file enumerator */
static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Next(IEnumBackgroundCopyFiles *iface,
        ULONG celt, IBackgroundCopyFile **rgelt, ULONG *pceltFetched)
{
    EnumBackgroundCopyFilesImpl *This = impl_from_IEnumBackgroundCopyFiles(iface);
    ULONG fetched;
    ULONG i;
    IBackgroundCopyFile *file;

    /* Despite documented behavior, Windows (tested on XP) is not verifying
       that the caller set pceltFetched to zero.  No check here. */

    fetched = min(celt, This->numFiles - This->indexFiles);
    if (pceltFetched)
        *pceltFetched = fetched;
    else
    {
        /* We need to initialize this array if the caller doesn't request
           the length because length_is will default to celt.  */
        for (i = 0; i < celt; i++)
            rgelt[i] = NULL;

        /* pceltFetched can only be NULL if celt is 1 */
        if (celt != 1)
            return E_INVALIDARG;
    }

    /* Fill in the array of objects */
    for (i = 0; i < fetched; i++)
    {
        file = This->files[This->indexFiles++];
        IBackgroundCopyFile_AddRef(file);
        rgelt[i] = file;
    }

    return fetched == celt ? S_OK : S_FALSE;
}

/* Skip over one or more files in the file enumerator */
static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Skip(IEnumBackgroundCopyFiles *iface,
        ULONG celt)
{
    EnumBackgroundCopyFilesImpl *This = impl_from_IEnumBackgroundCopyFiles(iface);

    if (celt > This->numFiles - This->indexFiles)
    {
        This->indexFiles = This->numFiles;
        return S_FALSE;
    }

    This->indexFiles += celt;
    return S_OK;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Reset(IEnumBackgroundCopyFiles *iface)
{
    EnumBackgroundCopyFilesImpl *This = impl_from_IEnumBackgroundCopyFiles(iface);
    This->indexFiles = 0;
    return S_OK;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Clone(IEnumBackgroundCopyFiles *iface,
        IEnumBackgroundCopyFiles **ppenum)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_GetCount(IEnumBackgroundCopyFiles *iface,
        ULONG *puCount)
{
    EnumBackgroundCopyFilesImpl *This = impl_from_IEnumBackgroundCopyFiles(iface);
    *puCount = This->numFiles;
    return S_OK;
}

static const IEnumBackgroundCopyFilesVtbl BITS_IEnumBackgroundCopyFiles_Vtbl =
{
    BITS_IEnumBackgroundCopyFiles_QueryInterface,
    BITS_IEnumBackgroundCopyFiles_AddRef,
    BITS_IEnumBackgroundCopyFiles_Release,
    BITS_IEnumBackgroundCopyFiles_Next,
    BITS_IEnumBackgroundCopyFiles_Skip,
    BITS_IEnumBackgroundCopyFiles_Reset,
    BITS_IEnumBackgroundCopyFiles_Clone,
    BITS_IEnumBackgroundCopyFiles_GetCount
};

HRESULT EnumBackgroundCopyFilesConstructor(BackgroundCopyJobImpl *job, IEnumBackgroundCopyFiles **enum_files)
{
    EnumBackgroundCopyFilesImpl *This;
    BackgroundCopyFileImpl *file;
    ULONG i;

    TRACE("%p, %p)\n", job, enum_files);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->IEnumBackgroundCopyFiles_iface.lpVtbl = &BITS_IEnumBackgroundCopyFiles_Vtbl;
    This->ref = 1;

    /* Create array of files */
    This->indexFiles = 0;
    EnterCriticalSection(&job->cs);
    This->numFiles = list_count(&job->files);
    This->files = NULL;
    if (This->numFiles > 0)
    {
        This->files = HeapAlloc(GetProcessHeap(), 0,
                                This->numFiles * sizeof This->files[0]);
        if (!This->files)
        {
            LeaveCriticalSection(&job->cs);
            HeapFree(GetProcessHeap(), 0, This);
            return E_OUTOFMEMORY;
        }
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(file, &job->files, BackgroundCopyFileImpl, entryFromJob)
    {
        IBackgroundCopyFile_AddRef(&file->IBackgroundCopyFile_iface);
        This->files[i] = &file->IBackgroundCopyFile_iface;
        ++i;
    }
    LeaveCriticalSection(&job->cs);

    *enum_files = &This->IEnumBackgroundCopyFiles_iface;
    return S_OK;
}
