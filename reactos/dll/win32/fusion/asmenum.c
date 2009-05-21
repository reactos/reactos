/*
 * IAssemblyEnum implementation
 *
 * Copyright 2008 James Hawkins
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "guiddef.h"
#include "fusion.h"
#include "corerror.h"
#include "fusionpriv.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

typedef struct _tagASMNAME
{
    struct list entry;
    IAssemblyName *name;
} ASMNAME;

typedef struct
{
    const IAssemblyEnumVtbl *lpIAssemblyEnumVtbl;

    struct list assemblies;
    struct list *iter;
    LONG ref;
} IAssemblyEnumImpl;

static HRESULT WINAPI IAssemblyEnumImpl_QueryInterface(IAssemblyEnum *iface,
                                                       REFIID riid, LPVOID *ppobj)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyEnum))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyEnumImpl_AddRef(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyEnumImpl_Release(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);
    struct list *item, *cursor;

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
    {
        LIST_FOR_EACH_SAFE(item, cursor, &This->assemblies)
        {
            ASMNAME *asmname = LIST_ENTRY(item, ASMNAME, entry);

            list_remove(&asmname->entry);
            IAssemblyName_Release(asmname->name);
            HeapFree(GetProcessHeap(), 0, asmname);
        }

        HeapFree(GetProcessHeap(), 0, This);
    }

    return refCount;
}

static HRESULT WINAPI IAssemblyEnumImpl_GetNextAssembly(IAssemblyEnum *iface,
                                                        LPVOID pvReserved,
                                                        IAssemblyName **ppName,
                                                        DWORD dwFlags)
{
    IAssemblyEnumImpl *asmenum = (IAssemblyEnumImpl *)iface;
    ASMNAME *asmname;

    TRACE("(%p, %p, %p, %d)\n", iface, pvReserved, ppName, dwFlags);

    if (!ppName)
        return E_INVALIDARG;

    asmname = LIST_ENTRY(asmenum->iter, ASMNAME, entry);
    if (!asmname)
        return S_FALSE;

    *ppName = asmname->name;
    IAssemblyName_AddRef(*ppName);

    asmenum->iter = list_next(&asmenum->assemblies, asmenum->iter);

    return S_OK;
}

static HRESULT WINAPI IAssemblyEnumImpl_Reset(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *asmenum = (IAssemblyEnumImpl *)iface;

    TRACE("(%p)\n", iface);

    asmenum->iter = list_head(&asmenum->assemblies);
    return S_OK;
}

static HRESULT WINAPI IAssemblyEnumImpl_Clone(IAssemblyEnum *iface,
                                               IAssemblyEnum **ppEnum)
{
    FIXME("(%p, %p) stub!\n", iface, ppEnum);
    return E_NOTIMPL;
}

static const IAssemblyEnumVtbl AssemblyEnumVtbl = {
    IAssemblyEnumImpl_QueryInterface,
    IAssemblyEnumImpl_AddRef,
    IAssemblyEnumImpl_Release,
    IAssemblyEnumImpl_GetNextAssembly,
    IAssemblyEnumImpl_Reset,
    IAssemblyEnumImpl_Clone
};

static void parse_name(IAssemblyName *name, int depth, LPWSTR path, LPWSTR buf)
{
    WCHAR disp[MAX_PATH];
    LPCWSTR verptr, pubkeyptr;
    HRESULT hr;
    DWORD size, major_size, minor_size, build_size, revision_size;
    WORD major, minor, build, revision;

    static const WCHAR star[] = {'*',0};
    static const WCHAR ss_fmt[] = {'%','s','\\','%','s',0};
    static const WCHAR verpubkey[] = {'%','s','\\','%','s','_','_','%','s',0};
    static const WCHAR ver_fmt[] = {'%','u','.','%','u','.','%','u','.','%','u',0};

    WCHAR version[24]; /* strlen("65535") * 4 + 3 + 1 */
    WCHAR token_str[TOKEN_LENGTH + 1];
    BYTE token[BYTES_PER_TOKEN];

    if (depth == 0)
    {
        size = MAX_PATH;
        *disp = '\0';
        hr = IAssemblyName_GetName(name, &size, disp);
        if (SUCCEEDED(hr))
            sprintfW(buf, ss_fmt, path, disp);
        else
            sprintfW(buf, ss_fmt, path, star);
    }
    else if (depth == 1)
    {
        major_size = sizeof(major);
        IAssemblyName_GetProperty(name, ASM_NAME_MAJOR_VERSION, &major, &major_size);

        minor_size = sizeof(minor);
        IAssemblyName_GetProperty(name, ASM_NAME_MINOR_VERSION, &minor, &minor_size);

        build_size = sizeof(build);
        IAssemblyName_GetProperty(name, ASM_NAME_BUILD_NUMBER, &build, &build_size);

        revision_size = sizeof(revision);
        IAssemblyName_GetProperty(name, ASM_NAME_REVISION_NUMBER, &revision, &revision_size);

        if (!major_size || !minor_size || !build_size || !revision_size) verptr = star;
        else
        {
            sprintfW(version, ver_fmt, major, minor, build, revision);
            verptr = version;
        }

        size = sizeof(token);
        IAssemblyName_GetProperty(name, ASM_NAME_PUBLIC_KEY_TOKEN, token, &size);

        if (!size) pubkeyptr = star;
        else
        {
            token_to_str(token, token_str);
            pubkeyptr = token_str;
        }

        sprintfW(buf, verpubkey, path, verptr, pubkeyptr);
    }
}

static int compare_assembly_names(ASMNAME *asmname1, ASMNAME *asmname2)
{
    int ret;
    WORD version1, version2;
    WCHAR name1[MAX_PATH], name2[MAX_PATH];
    WCHAR token_str1[TOKEN_LENGTH + 1], token_str2[TOKEN_LENGTH + 1];
    BYTE token1[BYTES_PER_TOKEN], token2[BYTES_PER_TOKEN];
    DWORD size, i;

    size = sizeof(name1);
    IAssemblyName_GetProperty(asmname1->name, ASM_NAME_NAME, &name1, &size);
    size = sizeof(name2);
    IAssemblyName_GetProperty(asmname2->name, ASM_NAME_NAME, &name2, &size);

    if ((ret = strcmpiW(name1, name2))) return ret;

    for (i = ASM_NAME_MAJOR_VERSION; i < ASM_NAME_CULTURE; i++)
    {
        size = sizeof(version1);
        IAssemblyName_GetProperty(asmname1->name, i, &version1, &size);
        size = sizeof(version2);
        IAssemblyName_GetProperty(asmname2->name, i, &version2, &size);

        if (version1 < version2) return -1;
        if (version1 > version2) return 1;
    }

    /* FIXME: compare cultures */

    size = sizeof(token1);
    IAssemblyName_GetProperty(asmname1->name, ASM_NAME_PUBLIC_KEY_TOKEN, token1, &size);
    size = sizeof(token2);
    IAssemblyName_GetProperty(asmname2->name, ASM_NAME_PUBLIC_KEY_TOKEN, token2, &size);

    token_to_str(token1, token_str1);
    token_to_str(token2, token_str2);

    if ((ret = strcmpiW(token_str1, token_str2))) return ret;

    return 0;
}

/* insert assembly in list preserving sort order */
static void insert_assembly(struct list *assemblies, ASMNAME *to_insert)
{
    struct list *item;

    LIST_FOR_EACH(item, assemblies)
    {
        ASMNAME *name = LIST_ENTRY(item, ASMNAME, entry);

        if (compare_assembly_names(name, to_insert) > 0)
        {
            list_add_before(&name->entry, &to_insert->entry);
            return;
        }
    }
    list_add_tail(assemblies, &to_insert->entry);
}

static HRESULT enum_gac_assemblies(struct list *assemblies, IAssemblyName *name,
                                   int depth, LPWSTR path)
{
    WIN32_FIND_DATAW ffd;
    WCHAR buf[MAX_PATH];
    WCHAR disp[MAX_PATH];
    ASMNAME *asmname;
    HANDLE hfind;
    LPWSTR ptr;
    HRESULT hr = S_OK;

    static WCHAR parent[MAX_PATH];

    static const WCHAR dot[] = {'.',0};
    static const WCHAR dotdot[] = {'.','.',0};
    static const WCHAR search_fmt[] = {'%','s','\\','*',0};
    static const WCHAR parent_fmt[] = {'%','s',',',' ',0};
    static const WCHAR dblunder[] = {'_','_',0};
    static const WCHAR fmt[] = {'V','e','r','s','i','o','n','=','%','s',',',' ',
        'C','u','l','t','u','r','e','=','n','e','u','t','r','a','l',',',' ',
        'P','u','b','l','i','c','K','e','y','T','o','k','e','n','=','%','s',0};
    static const WCHAR ss_fmt[] = {'%','s','\\','%','s',0};

    if (name)
        parse_name(name, depth, path, buf);
    else
        sprintfW(buf, search_fmt, path);

    hfind = FindFirstFileW(buf, &ffd);
    if (hfind == INVALID_HANDLE_VALUE)
        return S_OK;

    do
    {
        if (!lstrcmpW(ffd.cFileName, dot) || !lstrcmpW(ffd.cFileName, dotdot))
            continue;

        if (depth == 0)
        {
            if (name)
                ptr = strrchrW(buf, '\\') + 1;
            else
                ptr = ffd.cFileName;

            sprintfW(parent, parent_fmt, ptr);
        }
        else if (depth == 1)
        {
            ptr = strstrW(ffd.cFileName, dblunder);
            *ptr = '\0';
            ptr += 2;
            sprintfW(buf, fmt, ffd.cFileName, ptr);

            lstrcpyW(disp, parent);
            lstrcatW(disp, buf);

            asmname = HeapAlloc(GetProcessHeap(), 0, sizeof(ASMNAME));
            if (!asmname)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            hr = CreateAssemblyNameObject(&asmname->name, disp,
                                          CANOF_PARSE_DISPLAY_NAME, NULL);
            if (FAILED(hr))
            {
                HeapFree(GetProcessHeap(), 0, asmname);
                break;
            }

            insert_assembly(assemblies, asmname);
            continue;
        }

        sprintfW(buf, ss_fmt, path, ffd.cFileName);
        hr = enum_gac_assemblies(assemblies, name, depth + 1, buf);
        if (FAILED(hr))
            break;
    } while (FindNextFileW(hfind, &ffd) != 0);

    FindClose(hfind);
    return hr;
}

static HRESULT enumerate_gac(IAssemblyEnumImpl *asmenum, IAssemblyName *pName)
{
    WCHAR path[MAX_PATH];
    WCHAR buf[MAX_PATH];
    HRESULT hr;
    DWORD size;

    static WCHAR under32[] = {'_','3','2',0};
    static WCHAR msil[] = {'_','M','S','I','L',0};

    size = MAX_PATH;
    hr = GetCachePath(ASM_CACHE_GAC, buf, &size);
    if (FAILED(hr))
        return hr;

    lstrcpyW(path, buf);
    lstrcatW(path, under32);
    hr = enum_gac_assemblies(&asmenum->assemblies, pName, 0, path);
    if (FAILED(hr))
        return hr;

    lstrcpyW(path, buf);
    lstrcatW(path, msil);
    hr = enum_gac_assemblies(&asmenum->assemblies, pName, 0, path);
    if (FAILED(hr))
        return hr;

    hr = enum_gac_assemblies(&asmenum->assemblies, pName, 0, buf);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

/******************************************************************
 *  CreateAssemblyEnum   (FUSION.@)
 */
HRESULT WINAPI CreateAssemblyEnum(IAssemblyEnum **pEnum, IUnknown *pUnkReserved,
                                  IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved)
{
    IAssemblyEnumImpl *asmenum;
    HRESULT hr;

    TRACE("(%p, %p, %p, %08x, %p)\n", pEnum, pUnkReserved,
          pName, dwFlags, pvReserved);

    if (!pEnum)
        return E_INVALIDARG;

    if (dwFlags == 0 || dwFlags == ASM_CACHE_ROOT)
        return E_INVALIDARG;

    asmenum = HeapAlloc(GetProcessHeap(), 0, sizeof(IAssemblyEnumImpl));
    if (!asmenum)
        return E_OUTOFMEMORY;

    asmenum->lpIAssemblyEnumVtbl = &AssemblyEnumVtbl;
    asmenum->ref = 1;
    list_init(&asmenum->assemblies);

    if (dwFlags & ASM_CACHE_GAC)
    {
        hr = enumerate_gac(asmenum, pName);
        if (FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, asmenum);
            return hr;
        }
    }

    asmenum->iter = list_head(&asmenum->assemblies);
    *pEnum = (IAssemblyEnum *)asmenum;

    return S_OK;
}
