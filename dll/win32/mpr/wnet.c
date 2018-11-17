/*
 * MPR WNet functions
 *
 * Copyright 1999 Ulrich Weigand
 * Copyright 2004 Juan Lang
 * Copyright 2007 Maarten Lankhorst
 * Copyright 2016 Pierre Schweitzer
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
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winioctl.h"
#include "winnetwk.h"
#include "npapi.h"
#include "winreg.h"
#include "winuser.h"
#define WINE_MOUNTMGR_EXTENSIONS
#include "ddk/mountmgr.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "mprres.h"
#include "wnetpriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);

/* Data structures representing network service providers.  Assumes only one
 * thread creates them, and that they are constant for the life of the process
 * (and therefore doesn't synchronize access).
 * FIXME: only basic provider data and enumeration-related data are implemented
 * so far, need to implement the rest too.
 */
typedef struct _WNetProvider
{
    HMODULE           hLib;
    PWSTR             name;
    PF_NPGetCaps      getCaps;
    DWORD             dwSpecVersion;
    DWORD             dwNetType;
    DWORD             dwEnumScopes;
    PF_NPOpenEnum     openEnum;
    PF_NPEnumResource enumResource;
    PF_NPCloseEnum    closeEnum;
    PF_NPGetResourceInformation getResourceInformation;
    PF_NPAddConnection addConnection;
    PF_NPAddConnection3 addConnection3;
    PF_NPCancelConnection cancelConnection;
#ifdef __REACTOS__
    PF_NPGetConnection getConnection;
#endif
} WNetProvider, *PWNetProvider;

typedef struct _WNetProviderTable
{
    LPWSTR           entireNetwork;
    DWORD            numAllocated;
    DWORD            numProviders;
    WNetProvider     table[1];
} WNetProviderTable, *PWNetProviderTable;

#ifndef __REACTOS__
#define WNET_ENUMERATOR_TYPE_NULL      0
#define WNET_ENUMERATOR_TYPE_GLOBAL    1
#define WNET_ENUMERATOR_TYPE_PROVIDER  2
#define WNET_ENUMERATOR_TYPE_CONTEXT   3
#define WNET_ENUMERATOR_TYPE_CONNECTED 4
#else
#define WNET_ENUMERATOR_TYPE_GLOBAL     0
#define WNET_ENUMERATOR_TYPE_PROVIDER   1
#define WNET_ENUMERATOR_TYPE_CONTEXT    2
#define WNET_ENUMERATOR_TYPE_CONNECTED  3
#define WNET_ENUMERATOR_TYPE_REMEMBERED 4
#endif

#ifndef __REACTOS__
/* An WNet enumerator.  Note that the type doesn't correspond to the scope of
 * the enumeration; it represents one of the following types:
 * - a 'null' enumeration, one that contains no members
 * - a global enumeration, one that's executed across all providers
 * - a provider-specific enumeration, one that's only executed by a single
 *   provider
 * - a context enumeration.  I know this contradicts what I just said about
 *   there being no correspondence between the scope and the type, but it's
 *   necessary for the special case that a "Entire Network" entry needs to
 *   be enumerated in an enumeration of the context scope.  Thus an enumeration
 *   of the context scope results in a context type enumerator, which morphs
 *   into a global enumeration (so the enumeration continues across all
 *   providers).
 */
#else
/* An WNet enumerator.  Note that the type doesn't correspond to the scope of
 * the enumeration; it represents one of the following types:
 * - a global enumeration, one that's executed across all providers
 * - a provider-specific enumeration, one that's only executed by a single
 *   provider
 * - a context enumeration.  I know this contradicts what I just said about
 *   there being no correspondence between the scope and the type, but it's
 *   necessary for the special case that a "Entire Network" entry needs to
 *   be enumerated in an enumeration of the context scope.  Thus an enumeration
 *   of the context scope results in a context type enumerator, which morphs
 *   into a global enumeration (so the enumeration continues across all
 *   providers).
 * - a remembered enumeration, not related to providers themselves, but it
 *   is a registry enumeration for saved connections
 */
#endif
typedef struct _WNetEnumerator
{
    DWORD          enumType;
    DWORD          providerIndex;
    HANDLE         handle;
    BOOL           providerDone;
    DWORD          dwScope;
    DWORD          dwType;
    DWORD          dwUsage;
    union
    {
        NETRESOURCEW* net;
        HANDLE* handles;
#ifdef __REACTOS__
        struct
        {
            HKEY registry;
            DWORD index;
        } remembered;
#endif
    } specific;
} WNetEnumerator, *PWNetEnumerator;

#define BAD_PROVIDER_INDEX (DWORD)0xffffffff

/* Returns an index (into the global WNetProviderTable) of the provider with
 * the given name, or BAD_PROVIDER_INDEX if not found.
 */
static DWORD _findProviderIndexW(LPCWSTR lpProvider);

static PWNetProviderTable providerTable;

/*
 * Global provider table functions
 */

static void _tryLoadProvider(PCWSTR provider)
{
    static const WCHAR servicePrefix[] = { 'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'S','e','r','v','i','c','e','s','\\',0 };
    static const WCHAR serviceFmt[] = { '%','s','%','s','\\',
     'N','e','t','w','o','r','k','P','r','o','v','i','d','e','r',0 };
    WCHAR serviceName[MAX_PATH];
    HKEY hKey;

    TRACE("%s\n", debugstr_w(provider));
    snprintfW(serviceName, sizeof(serviceName) / sizeof(WCHAR), serviceFmt,
     servicePrefix, provider);
    serviceName[sizeof(serviceName) / sizeof(WCHAR) - 1] = '\0';
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, serviceName, 0, KEY_READ, &hKey) ==
     ERROR_SUCCESS)
    {
        static const WCHAR szProviderPath[] = { 'P','r','o','v','i','d','e','r',
         'P','a','t','h',0 };
        WCHAR providerPath[MAX_PATH];
        DWORD type, size = sizeof(providerPath);

        if (RegQueryValueExW(hKey, szProviderPath, NULL, &type,
         (LPBYTE)providerPath, &size) == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ))
        {
            static const WCHAR szProviderName[] = { 'N','a','m','e',0 };
            PWSTR name = NULL;

            if (type == REG_EXPAND_SZ)
            {
                WCHAR path[MAX_PATH];
                if (ExpandEnvironmentStringsW(providerPath, path, MAX_PATH)) lstrcpyW( providerPath, path );
            }

            size = 0;
            RegQueryValueExW(hKey, szProviderName, NULL, NULL, NULL, &size);
            if (size)
            {
                name = HeapAlloc(GetProcessHeap(), 0, size);
                if (RegQueryValueExW(hKey, szProviderName, NULL, &type,
                 (LPBYTE)name, &size) != ERROR_SUCCESS || type != REG_SZ)
                {
                    HeapFree(GetProcessHeap(), 0, name);
                    name = NULL;
                }
            }
            if (name)
            {
                HMODULE hLib = LoadLibraryW(providerPath);

                if (hLib)
                {
#define MPR_GETPROC(proc) ((PF_##proc)GetProcAddress(hLib, #proc))

                    PF_NPGetCaps getCaps = MPR_GETPROC(NPGetCaps);

                    TRACE("loaded lib %p\n", hLib);
                    if (getCaps)
                    {
                        DWORD connectCap;
                        PWNetProvider provider =
                         &providerTable->table[providerTable->numProviders];

                        provider->hLib = hLib;
                        provider->name = name;
                        TRACE("name is %s\n", debugstr_w(name));
                        provider->getCaps = getCaps;
                        provider->dwSpecVersion = getCaps(WNNC_SPEC_VERSION);
                        provider->dwNetType = getCaps(WNNC_NET_TYPE);
                        TRACE("net type is 0x%08x\n", provider->dwNetType);
                        provider->dwEnumScopes = getCaps(WNNC_ENUMERATION);
                        if (provider->dwEnumScopes)
                        {
                            TRACE("supports enumeration\n");
                            provider->openEnum = MPR_GETPROC(NPOpenEnum);
                            TRACE("NPOpenEnum %p\n", provider->openEnum);
                            provider->enumResource = MPR_GETPROC(NPEnumResource);
                            TRACE("NPEnumResource %p\n", provider->enumResource);
                            provider->closeEnum = MPR_GETPROC(NPCloseEnum);
                            TRACE("NPCloseEnum %p\n", provider->closeEnum);
                            provider->getResourceInformation = MPR_GETPROC(NPGetResourceInformation);
                            TRACE("NPGetResourceInformation %p\n", provider->getResourceInformation);
                            if (!provider->openEnum ||
                                !provider->enumResource ||
                                !provider->closeEnum)
                            {
                                provider->openEnum = NULL;
                                provider->enumResource = NULL;
                                provider->closeEnum = NULL;
                                provider->dwEnumScopes = 0;
                                WARN("Couldn't load enumeration functions\n");
                            }
                        }
                        connectCap = getCaps(WNNC_CONNECTION);
                        if (connectCap & WNNC_CON_ADDCONNECTION)
                            provider->addConnection = MPR_GETPROC(NPAddConnection);
                        if (connectCap & WNNC_CON_ADDCONNECTION3)
                            provider->addConnection3 = MPR_GETPROC(NPAddConnection3);
                        if (connectCap & WNNC_CON_CANCELCONNECTION)
                            provider->cancelConnection = MPR_GETPROC(NPCancelConnection);
#ifdef __REACTOS__
                        if (connectCap & WNNC_CON_GETCONNECTIONS)
                            provider->getConnection = MPR_GETPROC(NPGetConnection);
#endif
                        TRACE("NPAddConnection %p\n", provider->addConnection);
                        TRACE("NPAddConnection3 %p\n", provider->addConnection3);
                        TRACE("NPCancelConnection %p\n", provider->cancelConnection);
                        providerTable->numProviders++;
                    }
                    else
                    {
                        WARN("Provider %s didn't export NPGetCaps\n",
                         debugstr_w(provider));
                        HeapFree(GetProcessHeap(), 0, name);
                        FreeLibrary(hLib);
                    }

#undef MPR_GETPROC
                }
                else
                {
                    WARN("Couldn't load library %s for provider %s\n",
                     debugstr_w(providerPath), debugstr_w(provider));
                    HeapFree(GetProcessHeap(), 0, name);
                }
            }
            else
            {
                WARN("Couldn't get provider name for provider %s\n",
                 debugstr_w(provider));
            }
        }
        else
            WARN("Couldn't open value %s\n", debugstr_w(szProviderPath));
        RegCloseKey(hKey);
    }
    else
        WARN("Couldn't open service key for provider %s\n",
         debugstr_w(provider));
}

void wnetInit(HINSTANCE hInstDll)
{
    static const WCHAR providerOrderKey[] = { 'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\',
     'N','e','t','w','o','r','k','P','r','o','v','i','d','e','r','\\',
     'O','r','d','e','r',0 };
     static const WCHAR providerOrder[] = { 'P','r','o','v','i','d','e','r',
      'O','r','d','e','r',0 };
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, providerOrderKey, 0, KEY_READ, &hKey)
     == ERROR_SUCCESS)
    {
        DWORD size = 0;

        RegQueryValueExW(hKey, providerOrder, NULL, NULL, NULL, &size);
        if (size)
        {
            PWSTR providers = HeapAlloc(GetProcessHeap(), 0, size);

            if (providers)
            {
                DWORD type;

                if (RegQueryValueExW(hKey, providerOrder, NULL, &type,
                 (LPBYTE)providers, &size) == ERROR_SUCCESS && type == REG_SZ)
                {
                    PWSTR ptr;
                    DWORD numToAllocate;

                    TRACE("provider order is %s\n", debugstr_w(providers));
                    /* first count commas as a heuristic for how many to
                     * allocate space for */
                    for (ptr = providers, numToAllocate = 1; ptr; )
                    {
                        ptr = strchrW(ptr, ',');
                        if (ptr) {
                            numToAllocate++;
                            ptr++;
                        }
                    }
                    providerTable =
                     HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(WNetProviderTable)
                     + (numToAllocate - 1) * sizeof(WNetProvider));
                    if (providerTable)
                    {
                        PWSTR ptrPrev;
                        int entireNetworkLen;
                        LPCWSTR stringresource;

                        entireNetworkLen = LoadStringW(hInstDll,
                         IDS_ENTIRENETWORK, (LPWSTR)&stringresource, 0);
                        providerTable->entireNetwork = HeapAlloc(
                         GetProcessHeap(), 0, (entireNetworkLen + 1) *
                         sizeof(WCHAR));
                        if (providerTable->entireNetwork)
                        {
                            memcpy(providerTable->entireNetwork, stringresource, entireNetworkLen*sizeof(WCHAR));
                            providerTable->entireNetwork[entireNetworkLen] = 0;
                        }
                        providerTable->numAllocated = numToAllocate;
                        for (ptr = providers; ptr; )
                        {
                            ptrPrev = ptr;
                            ptr = strchrW(ptr, ',');
                            if (ptr)
                                *ptr++ = '\0';
                            _tryLoadProvider(ptrPrev);
                        }
                    }
                }
                HeapFree(GetProcessHeap(), 0, providers);
            }
        }
        RegCloseKey(hKey);
    }
}

void wnetFree(void)
{
    if (providerTable)
    {
        DWORD i;

        for (i = 0; i < providerTable->numProviders; i++)
        {
            HeapFree(GetProcessHeap(), 0, providerTable->table[i].name);
            FreeModule(providerTable->table[i].hLib);
        }
        HeapFree(GetProcessHeap(), 0, providerTable->entireNetwork);
        HeapFree(GetProcessHeap(), 0, providerTable);
        providerTable = NULL;
    }
}

static DWORD _findProviderIndexW(LPCWSTR lpProvider)
{
    DWORD ret = BAD_PROVIDER_INDEX;

    if (providerTable && providerTable->numProviders)
    {
        DWORD i;

        for (i = 0; i < providerTable->numProviders &&
         ret == BAD_PROVIDER_INDEX; i++)
            if (!strcmpW(lpProvider, providerTable->table[i].name))
                ret = i;
    }
    return ret;
}

/*
 * Browsing Functions
 */

static LPNETRESOURCEW _copyNetResourceForEnumW(LPNETRESOURCEW lpNet)
{
    LPNETRESOURCEW ret;

    if (lpNet)
    {
        ret = HeapAlloc(GetProcessHeap(), 0, sizeof(NETRESOURCEW));
        if (ret)
        {
            size_t len;

            *ret = *lpNet;
            ret->lpLocalName = ret->lpComment = ret->lpProvider = NULL;
            if (lpNet->lpRemoteName)
            {
                len = strlenW(lpNet->lpRemoteName) + 1;
                ret->lpRemoteName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                if (ret->lpRemoteName)
                    strcpyW(ret->lpRemoteName, lpNet->lpRemoteName);
            }
        }
    }
    else
        ret = NULL;
    return ret;
}

static void _freeEnumNetResource(LPNETRESOURCEW lpNet)
{
    if (lpNet)
    {
        HeapFree(GetProcessHeap(), 0, lpNet->lpRemoteName);
        HeapFree(GetProcessHeap(), 0, lpNet);
    }
}

#ifndef __REACTOS__
static PWNetEnumerator _createNullEnumerator(void)
{
    PWNetEnumerator ret = HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));

    if (ret)
        ret->enumType = WNET_ENUMERATOR_TYPE_NULL;
    return ret;
}
#endif

static PWNetEnumerator _createGlobalEnumeratorW(DWORD dwScope, DWORD dwType,
 DWORD dwUsage, LPNETRESOURCEW lpNet)
{
    PWNetEnumerator ret = HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));

    if (ret)
    {
        ret->enumType = WNET_ENUMERATOR_TYPE_GLOBAL;
        ret->dwScope = dwScope;
        ret->dwType  = dwType;
        ret->dwUsage = dwUsage;
        ret->specific.net = _copyNetResourceForEnumW(lpNet);
    }
    return ret;
}

static PWNetEnumerator _createProviderEnumerator(DWORD dwScope, DWORD dwType,
 DWORD dwUsage, DWORD index, HANDLE handle)
{
    PWNetEnumerator ret;

    if (!providerTable || index >= providerTable->numProviders)
        ret = NULL;
    else
    {
        ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));
        if (ret)
        {
            ret->enumType      = WNET_ENUMERATOR_TYPE_PROVIDER;
            ret->providerIndex = index;
            ret->dwScope       = dwScope;
            ret->dwType        = dwType;
            ret->dwUsage       = dwUsage;
            ret->handle        = handle;
        }
    }
    return ret;
}

static PWNetEnumerator _createContextEnumerator(DWORD dwScope, DWORD dwType,
 DWORD dwUsage)
{
    PWNetEnumerator ret = HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));

    if (ret)
    {
        ret->enumType = WNET_ENUMERATOR_TYPE_CONTEXT;
        ret->dwScope = dwScope;
        ret->dwType  = dwType;
        ret->dwUsage = dwUsage;
    }
    return ret;
}

static PWNetEnumerator _createConnectedEnumerator(DWORD dwScope, DWORD dwType,
 DWORD dwUsage)
{
    PWNetEnumerator ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));
    if (ret)
    {
        ret->enumType = WNET_ENUMERATOR_TYPE_CONNECTED;
        ret->dwScope = dwScope;
        ret->dwType  = dwType;
        ret->dwUsage = dwUsage;
        ret->specific.handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HANDLE) * providerTable->numProviders);
        if (!ret->specific.handles)
        {
            HeapFree(GetProcessHeap(), 0, ret);
            ret = NULL;
        }
    }
    return ret;
}

#ifdef __REACTOS__
static PWNetEnumerator _createRememberedEnumerator(DWORD dwScope, DWORD dwType,
 HKEY remembered)
{
    PWNetEnumerator ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));
    if (ret)
    {
        ret->enumType = WNET_ENUMERATOR_TYPE_REMEMBERED;
        ret->dwScope = dwScope;
        ret->dwType = dwType;
        ret->specific.remembered.registry = remembered;
    }
    return ret;
}
#endif

/* Thunks the array of wide-string LPNETRESOURCEs lpNetArrayIn into buffer
 * lpBuffer, with size *lpBufferSize.  lpNetArrayIn contains *lpcCount entries
 * to start.  On return, *lpcCount reflects the number thunked into lpBuffer.
 * Returns WN_SUCCESS on success (all of lpNetArrayIn thunked), WN_MORE_DATA
 * if not all members of the array could be thunked, and something else on
 * failure.
 */
static DWORD _thunkNetResourceArrayWToA(const NETRESOURCEW *lpNetArrayIn,
 const DWORD *lpcCount, LPVOID lpBuffer, const DWORD *lpBufferSize)
{
    DWORD i, numToThunk, totalBytes, ret;
    LPSTR strNext;

    if (!lpNetArrayIn)
        return WN_BAD_POINTER;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (*lpcCount == -1)
        return WN_BAD_VALUE;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;

    for (i = 0, numToThunk = 0, totalBytes = 0; i < *lpcCount; i++)
    {
        const NETRESOURCEW *lpNet = lpNetArrayIn + i;

        totalBytes += sizeof(NETRESOURCEA);
        if (lpNet->lpLocalName)
            totalBytes += WideCharToMultiByte(CP_ACP, 0, lpNet->lpLocalName,
             -1, NULL, 0, NULL, NULL);
        if (lpNet->lpRemoteName)
            totalBytes += WideCharToMultiByte(CP_ACP, 0, lpNet->lpRemoteName,
             -1, NULL, 0, NULL, NULL);
        if (lpNet->lpComment)
            totalBytes += WideCharToMultiByte(CP_ACP, 0, lpNet->lpComment,
             -1, NULL, 0, NULL, NULL);
        if (lpNet->lpProvider)
            totalBytes += WideCharToMultiByte(CP_ACP, 0, lpNet->lpProvider,
             -1, NULL, 0, NULL, NULL);
        if (totalBytes < *lpBufferSize)
            numToThunk = i + 1;
    }
    strNext = (LPSTR)((LPBYTE)lpBuffer + numToThunk * sizeof(NETRESOURCEA));
    for (i = 0; i < numToThunk; i++)
    {
        LPNETRESOURCEA lpNetOut = (LPNETRESOURCEA)lpBuffer + i;
        const NETRESOURCEW *lpNetIn = lpNetArrayIn + i;

        memcpy(lpNetOut, lpNetIn, sizeof(NETRESOURCEA));
        /* lie about string lengths, we already verified how many
         * we have space for above
         */
        if (lpNetIn->lpLocalName)
        {
            lpNetOut->lpLocalName = strNext;
            strNext += WideCharToMultiByte(CP_ACP, 0, lpNetIn->lpLocalName, -1,
             lpNetOut->lpLocalName, *lpBufferSize, NULL, NULL);
        }
        if (lpNetIn->lpRemoteName)
        {
            lpNetOut->lpRemoteName = strNext;
            strNext += WideCharToMultiByte(CP_ACP, 0, lpNetIn->lpRemoteName, -1,
             lpNetOut->lpRemoteName, *lpBufferSize, NULL, NULL);
        }
        if (lpNetIn->lpComment)
        {
            lpNetOut->lpComment = strNext;
            strNext += WideCharToMultiByte(CP_ACP, 0, lpNetIn->lpComment, -1,
             lpNetOut->lpComment, *lpBufferSize, NULL, NULL);
        }
        if (lpNetIn->lpProvider)
        {
            lpNetOut->lpProvider = strNext;
            strNext += WideCharToMultiByte(CP_ACP, 0, lpNetIn->lpProvider, -1,
             lpNetOut->lpProvider, *lpBufferSize, NULL, NULL);
        }
    }
    ret = numToThunk < *lpcCount ? WN_MORE_DATA : WN_SUCCESS;
    TRACE("numToThunk is %d, *lpcCount is %d, returning %d\n", numToThunk,
     *lpcCount, ret);
    return ret;
}

/* Thunks the array of multibyte-string LPNETRESOURCEs lpNetArrayIn into buffer
 * lpBuffer, with size *lpBufferSize.  lpNetArrayIn contains *lpcCount entries
 * to start.  On return, *lpcCount reflects the number thunked into lpBuffer.
 * Returns WN_SUCCESS on success (all of lpNetArrayIn thunked), WN_MORE_DATA
 * if not all members of the array could be thunked, and something else on
 * failure.
 */
static DWORD _thunkNetResourceArrayAToW(const NETRESOURCEA *lpNetArrayIn,
 const DWORD *lpcCount, LPVOID lpBuffer, const DWORD *lpBufferSize)
{
    DWORD i, numToThunk, totalBytes, ret;
    LPWSTR strNext;

    if (!lpNetArrayIn)
        return WN_BAD_POINTER;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (*lpcCount == -1)
        return WN_BAD_VALUE;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;

    for (i = 0, numToThunk = 0, totalBytes = 0; i < *lpcCount; i++)
    {
        const NETRESOURCEA *lpNet = lpNetArrayIn + i;

        totalBytes += sizeof(NETRESOURCEW);
        if (lpNet->lpLocalName)
            totalBytes += MultiByteToWideChar(CP_ACP, 0, lpNet->lpLocalName,
             -1, NULL, 0) * sizeof(WCHAR);
        if (lpNet->lpRemoteName)
            totalBytes += MultiByteToWideChar(CP_ACP, 0, lpNet->lpRemoteName,
             -1, NULL, 0) * sizeof(WCHAR);
        if (lpNet->lpComment)
            totalBytes += MultiByteToWideChar(CP_ACP, 0, lpNet->lpComment,
             -1, NULL, 0) * sizeof(WCHAR);
        if (lpNet->lpProvider)
            totalBytes += MultiByteToWideChar(CP_ACP, 0, lpNet->lpProvider,
             -1, NULL, 0) * sizeof(WCHAR);
        if (totalBytes < *lpBufferSize)
            numToThunk = i + 1;
    }
    strNext = (LPWSTR)((LPBYTE)lpBuffer + numToThunk * sizeof(NETRESOURCEW));
    for (i = 0; i < numToThunk; i++)
    {
        LPNETRESOURCEW lpNetOut = (LPNETRESOURCEW)lpBuffer + i;
        const NETRESOURCEA *lpNetIn = lpNetArrayIn + i;

        memcpy(lpNetOut, lpNetIn, sizeof(NETRESOURCEW));
        /* lie about string lengths, we already verified how many
         * we have space for above
         */
        if (lpNetIn->lpLocalName)
        {
            lpNetOut->lpLocalName = strNext;
            strNext += MultiByteToWideChar(CP_ACP, 0, lpNetIn->lpLocalName,
             -1, lpNetOut->lpLocalName, *lpBufferSize);
        }
        if (lpNetIn->lpRemoteName)
        {
            lpNetOut->lpRemoteName = strNext;
            strNext += MultiByteToWideChar(CP_ACP, 0, lpNetIn->lpRemoteName,
             -1, lpNetOut->lpRemoteName, *lpBufferSize);
        }
        if (lpNetIn->lpComment)
        {
            lpNetOut->lpComment = strNext;
            strNext += MultiByteToWideChar(CP_ACP, 0, lpNetIn->lpComment,
             -1, lpNetOut->lpComment, *lpBufferSize);
        }
        if (lpNetIn->lpProvider)
        {
            lpNetOut->lpProvider = strNext;
            strNext += MultiByteToWideChar(CP_ACP, 0, lpNetIn->lpProvider,
             -1, lpNetOut->lpProvider, *lpBufferSize);
        }
    }
    ret = numToThunk < *lpcCount ? WN_MORE_DATA : WN_SUCCESS;
    TRACE("numToThunk is %d, *lpcCount is %d, returning %d\n", numToThunk,
     *lpcCount, ret);
    return ret;
}

/*********************************************************************
 * WNetOpenEnumA [MPR.@]
 *
 * See comments for WNetOpenEnumW.
 */
DWORD WINAPI WNetOpenEnumA( DWORD dwScope, DWORD dwType, DWORD dwUsage,
                            LPNETRESOURCEA lpNet, LPHANDLE lphEnum )
{
    DWORD ret;

    TRACE( "(%08X, %08X, %08X, %p, %p)\n",
	    dwScope, dwType, dwUsage, lpNet, lphEnum );

    if (!lphEnum)
        ret = WN_BAD_POINTER;
    else if (!providerTable || providerTable->numProviders == 0)
    {
        *lphEnum = NULL;
        ret = WN_NO_NETWORK;
    }
    else
    {
        if (lpNet)
        {
            LPNETRESOURCEW lpNetWide = NULL;
            BYTE buf[1024];
            DWORD size = sizeof(buf), count = 1;
            BOOL allocated = FALSE;

            ret = _thunkNetResourceArrayAToW(lpNet, &count, buf, &size);
            if (ret == WN_MORE_DATA)
            {
                lpNetWide = HeapAlloc(GetProcessHeap(), 0,
                 size);
                if (lpNetWide)
                {
                    ret = _thunkNetResourceArrayAToW(lpNet, &count, lpNetWide,
                     &size);
                    allocated = TRUE;
                }
                else
                    ret = WN_OUT_OF_MEMORY;
            }
            else if (ret == WN_SUCCESS)
                lpNetWide = (LPNETRESOURCEW)buf;
            if (ret == WN_SUCCESS)
                ret = WNetOpenEnumW(dwScope, dwType, dwUsage, lpNetWide,
                 lphEnum);
            if (allocated)
                HeapFree(GetProcessHeap(), 0, lpNetWide);
        }
        else
            ret = WNetOpenEnumW(dwScope, dwType, dwUsage, NULL, lphEnum);
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*********************************************************************
 * WNetOpenEnumW [MPR.@]
 *
 * Network enumeration has way too many parameters, so I'm not positive I got
 * them right.  What I've got so far:
 *
 * - If the scope is RESOURCE_GLOBALNET, and no LPNETRESOURCE is passed,
 *   all the network providers should be enumerated.
 *
 * - If the scope is RESOURCE_GLOBALNET, and LPNETRESOURCE is passed, and
 *   and neither the LPNETRESOURCE's lpRemoteName nor the LPNETRESOURCE's
 *   lpProvider is set, all the network providers should be enumerated.
 *   (This means the enumeration is a list of network providers, not that the
 *   enumeration is passed on to the providers.)
 *
 * - If the scope is RESOURCE_GLOBALNET, and LPNETRESOURCE is passed, and the
 *   resource matches the "Entire Network" resource (no remote name, no
 *   provider, comment is the "Entire Network" string), a RESOURCE_GLOBALNET
 *   enumeration is done on every network provider.
 *
 * - If the scope is RESOURCE_GLOBALNET, and LPNETRESOURCE is passed, and
 *   the LPNETRESOURCE's lpProvider is set, enumeration will be passed through
 *   only to the given network provider.
 *
 * - If the scope is RESOURCE_GLOBALNET, and LPNETRESOURCE is passed, and
 *   no lpProvider is set, enumeration will be tried on every network provider,
 *   in the order in which they're loaded.
 *
 * - The LPNETRESOURCE should be disregarded for scopes besides
 *   RESOURCE_GLOBALNET.  MSDN states that lpNet must be NULL if dwScope is not
 *   RESOURCE_GLOBALNET, but Windows doesn't return an error if it isn't NULL.
 *
 * - If the scope is RESOURCE_CONTEXT, MS includes an "Entire Network" net
 *   resource in the enumerated list, as well as any machines in your
 *   workgroup.  The machines in your workgroup come from doing a
 *   RESOURCE_CONTEXT enumeration of every Network Provider.
 */
DWORD WINAPI WNetOpenEnumW( DWORD dwScope, DWORD dwType, DWORD dwUsage,
                            LPNETRESOURCEW lpNet, LPHANDLE lphEnum )
{
    DWORD ret;

    TRACE( "(%08X, %08X, %08X, %p, %p)\n",
          dwScope, dwType, dwUsage, lpNet, lphEnum );

    if (!lphEnum)
        ret = WN_BAD_POINTER;
    else if (!providerTable || providerTable->numProviders == 0)
    {
        *lphEnum = NULL;
        ret = WN_NO_NETWORK;
    }
    else
    {
        switch (dwScope)
        {
            case RESOURCE_GLOBALNET:
                if (lpNet)
                {
                    if (lpNet->lpProvider)
                    {
                        DWORD index = _findProviderIndexW(lpNet->lpProvider);

                        if (index != BAD_PROVIDER_INDEX)
                        {
                            if (providerTable->table[index].openEnum &&
                             providerTable->table[index].dwEnumScopes & WNNC_ENUM_GLOBAL)
                            {
                                HANDLE handle;
                                PWSTR RemoteName = lpNet->lpRemoteName;

                                if ((lpNet->dwUsage & RESOURCEUSAGE_CONTAINER) &&
                                    RemoteName && !strcmpW(RemoteName, lpNet->lpProvider))
                                    lpNet->lpRemoteName = NULL;

                                ret = providerTable->table[index].openEnum(
                                 dwScope, dwType, dwUsage, lpNet, &handle);
                                if (ret == WN_SUCCESS)
                                {
                                    *lphEnum = _createProviderEnumerator(
                                     dwScope, dwType, dwUsage, index, handle);
                                    ret = *lphEnum ? WN_SUCCESS :
                                     WN_OUT_OF_MEMORY;
                                }

                                lpNet->lpRemoteName = RemoteName;
                            }
                            else
                                ret = WN_NOT_SUPPORTED;
                        }
                        else
                            ret = WN_BAD_PROVIDER;
                    }
                    else if (lpNet->lpRemoteName)
                    {
                        *lphEnum = _createGlobalEnumeratorW(dwScope,
                         dwType, dwUsage, lpNet);
                        ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                    }
                    else
                    {
                        if (lpNet->lpComment && !strcmpW(lpNet->lpComment,
                         providerTable->entireNetwork))
                        {
                            /* comment matches the "Entire Network", enumerate
                             * global scope of every provider
                             */
                            *lphEnum = _createGlobalEnumeratorW(dwScope,
                             dwType, dwUsage, lpNet);
                        }
                        else
                        {
                            /* this is the same as not having passed lpNet */
                            *lphEnum = _createGlobalEnumeratorW(dwScope,
                             dwType, dwUsage, NULL);
                        }
                        ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    *lphEnum = _createGlobalEnumeratorW(dwScope, dwType,
                     dwUsage, lpNet);
                    ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                }
                break;
            case RESOURCE_CONTEXT:
                *lphEnum = _createContextEnumerator(dwScope, dwType, dwUsage);
                ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                break;
            case RESOURCE_CONNECTED:
                *lphEnum = _createConnectedEnumerator(dwScope, dwType, dwUsage);
                ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                break;
            case RESOURCE_REMEMBERED:
#ifndef __REACTOS__
                *lphEnum = _createNullEnumerator();
                ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
#else
                {
                    HKEY remembered, user_profile;

                    ret = WN_OUT_OF_MEMORY;
                    if (RegOpenCurrentUser(KEY_READ, &user_profile) == ERROR_SUCCESS)
                    {
                        WCHAR subkey[8] = {'N', 'e', 't', 'w', 'o', 'r', 'k', 0};

                        if (RegOpenKeyExW(user_profile, subkey, 0, KEY_READ, &remembered) == ERROR_SUCCESS)
                        {
                            *lphEnum = _createRememberedEnumerator(dwScope, dwType, remembered);
                            ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                        }

                        RegCloseKey(user_profile);
                    }
                }
#endif
                break;
            default:
                WARN("unknown scope 0x%08x\n", dwScope);
                ret = WN_BAD_VALUE;
        }
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*********************************************************************
 * WNetEnumResourceA [MPR.@]
 */
DWORD WINAPI WNetEnumResourceA( HANDLE hEnum, LPDWORD lpcCount,
                                LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE( "(%p, %p, %p, %p)\n", hEnum, lpcCount, lpBuffer, lpBufferSize );

    if (!hEnum)
        ret = WN_BAD_POINTER;
    else if (!lpcCount)
        ret = WN_BAD_POINTER;
    else if (!lpBuffer)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else if (*lpBufferSize < sizeof(NETRESOURCEA))
    {
        *lpBufferSize = sizeof(NETRESOURCEA);
        ret = WN_MORE_DATA;
    }
    else
    {
        DWORD localCount = *lpcCount, localSize = *lpBufferSize;
        LPVOID localBuffer = HeapAlloc(GetProcessHeap(), 0, localSize);

        if (localBuffer)
        {
            ret = WNetEnumResourceW(hEnum, &localCount, localBuffer,
             &localSize);
            if (ret == WN_SUCCESS || (ret == WN_MORE_DATA && localCount != -1))
            {
                /* FIXME: this isn't necessarily going to work in the case of
                 * WN_MORE_DATA, because our enumerator may have moved on to
                 * the next provider.  MSDN states that a large (16KB) buffer
                 * size is the appropriate usage of this function, so
                 * hopefully it won't be an issue.
                 */
                ret = _thunkNetResourceArrayWToA(localBuffer, &localCount,
                 lpBuffer, lpBufferSize);
                *lpcCount = localCount;
            }
            HeapFree(GetProcessHeap(), 0, localBuffer);
        }
        else
            ret = WN_OUT_OF_MEMORY;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

static DWORD _countProviderBytesW(PWNetProvider provider)
{
    DWORD ret;

    if (provider)
    {
        ret = sizeof(NETRESOURCEW);
        ret += 2 * (strlenW(provider->name) + 1) * sizeof(WCHAR);
    }
    else
        ret = 0;
    return ret;
}

static DWORD _enumerateProvidersW(PWNetEnumerator enumerator, LPDWORD lpcCount,
 LPVOID lpBuffer, const DWORD *lpBufferSize)
{
    DWORD ret;

    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_GLOBAL)
        return WN_BAD_VALUE;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;
    if (*lpBufferSize < sizeof(NETRESOURCEA))
        return WN_MORE_DATA;

    if (!providerTable || enumerator->providerIndex >= 
     providerTable->numProviders)
        ret = WN_NO_MORE_ENTRIES;
    else
    {
        DWORD bytes = 0, count = 0, countLimit, i;
        LPNETRESOURCEW resource;
        LPWSTR strNext;

        countLimit = *lpcCount == -1 ?
         providerTable->numProviders - enumerator->providerIndex : *lpcCount;
        while (count < countLimit && bytes < *lpBufferSize)
        {
            DWORD bytesNext = _countProviderBytesW(
             &providerTable->table[count + enumerator->providerIndex]);

            if (bytes + bytesNext < *lpBufferSize)
            {
                bytes += bytesNext;
                count++;
            }
        }
        strNext = (LPWSTR)((LPBYTE)lpBuffer + count * sizeof(NETRESOURCEW));
        for (i = 0, resource = lpBuffer; i < count; i++, resource++)
        {
            resource->dwScope = RESOURCE_GLOBALNET;
            resource->dwType = RESOURCETYPE_ANY;
            resource->dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
            resource->dwUsage = RESOURCEUSAGE_CONTAINER |
             RESOURCEUSAGE_RESERVED;
            resource->lpLocalName = NULL;
            resource->lpRemoteName = strNext;
            strcpyW(resource->lpRemoteName,
             providerTable->table[i + enumerator->providerIndex].name);
            strNext += strlenW(resource->lpRemoteName) + 1;
            resource->lpComment = NULL;
            resource->lpProvider = strNext;
            strcpyW(resource->lpProvider,
             providerTable->table[i + enumerator->providerIndex].name);
            strNext += strlenW(resource->lpProvider) + 1;
        }
        enumerator->providerIndex += count;
        *lpcCount = count;
        ret = count > 0 ? WN_SUCCESS : WN_MORE_DATA;
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

/* Advances the enumerator (assumed to be a global enumerator) to the next
 * provider that supports the enumeration scope passed to WNetOpenEnum.  Does
 * not open a handle with the next provider.
 * If the existing handle is NULL, may leave the enumerator unchanged, since
 * the current provider may support the desired scope.
 * If the existing handle is not NULL, closes it before moving on.
 * Returns WN_SUCCESS on success, WN_NO_MORE_ENTRIES if there is no available
 * provider, and another error on failure.
 */
static DWORD _globalEnumeratorAdvance(PWNetEnumerator enumerator)
{
    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_GLOBAL)
        return WN_BAD_VALUE;
    if (!providerTable || enumerator->providerIndex >=
     providerTable->numProviders)
        return WN_NO_MORE_ENTRIES;

    if (enumerator->providerDone)
    {
        DWORD dwEnum = 0;
        enumerator->providerDone = FALSE;
        if (enumerator->handle)
        {
            providerTable->table[enumerator->providerIndex].closeEnum(
             enumerator->handle);
            enumerator->handle = NULL;
            enumerator->providerIndex++;
        }
        if (enumerator->dwScope == RESOURCE_CONNECTED)
            dwEnum = WNNC_ENUM_LOCAL;
        else if (enumerator->dwScope == RESOURCE_GLOBALNET)
            dwEnum = WNNC_ENUM_GLOBAL;
        else if (enumerator->dwScope == RESOURCE_CONTEXT)
            dwEnum = WNNC_ENUM_CONTEXT;
        for (; enumerator->providerIndex < providerTable->numProviders &&
         !(providerTable->table[enumerator->providerIndex].dwEnumScopes
         & dwEnum); enumerator->providerIndex++)
            ;
    }
    return enumerator->providerIndex < providerTable->numProviders ?
     WN_SUCCESS : WN_NO_MORE_ENTRIES;
}

/* "Passes through" call to the next provider that supports the enumeration
 * type.
 * FIXME: if one call to a provider's enumerator succeeds while there's still
 * space in lpBuffer, I don't call to the next provider.  The caller may not
 * expect that it should call EnumResourceW again with a return value of
 * WN_SUCCESS (depending what *lpcCount was to begin with).  That means strings
 * may have to be moved around a bit, ick.
 */
static DWORD _enumerateGlobalPassthroughW(PWNetEnumerator enumerator,
 LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    DWORD ret;

    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_GLOBAL)
        return WN_BAD_VALUE;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;
    if (*lpBufferSize < sizeof(NETRESOURCEW))
        return WN_MORE_DATA;

    ret = _globalEnumeratorAdvance(enumerator);
    if (ret == WN_SUCCESS)
    {
        ret = providerTable->table[enumerator->providerIndex].
         openEnum(enumerator->dwScope, enumerator->dwType,
         enumerator->dwUsage, enumerator->specific.net,
         &enumerator->handle);
        if (ret == WN_SUCCESS)
        {
            ret = providerTable->table[enumerator->providerIndex].
             enumResource(enumerator->handle, lpcCount, lpBuffer,
             lpBufferSize);
            if (ret != WN_MORE_DATA)
                enumerator->providerDone = TRUE;
        }
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static DWORD _enumerateGlobalW(PWNetEnumerator enumerator, LPDWORD lpcCount,
 LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    DWORD ret;

    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_GLOBAL)
        return WN_BAD_VALUE;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;
    if (*lpBufferSize < sizeof(NETRESOURCEW))
        return WN_MORE_DATA;
    if (!providerTable)
        return WN_NO_NETWORK;

    switch (enumerator->dwScope)
    {
        case RESOURCE_GLOBALNET:
            if (enumerator->specific.net)
                ret = _enumerateGlobalPassthroughW(enumerator, lpcCount,
                 lpBuffer, lpBufferSize);
            else
                ret = _enumerateProvidersW(enumerator, lpcCount, lpBuffer,
                 lpBufferSize);
            break;
        case RESOURCE_CONTEXT:
            ret = _enumerateGlobalPassthroughW(enumerator, lpcCount, lpBuffer,
             lpBufferSize);
            break;
        default:
            WARN("unexpected scope 0x%08x\n", enumerator->dwScope);
            ret = WN_NO_MORE_ENTRIES;
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static DWORD _enumerateProviderW(PWNetEnumerator enumerator, LPDWORD lpcCount,
 LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_PROVIDER)
        return WN_BAD_VALUE;
    if (!enumerator->handle)
        return WN_BAD_VALUE;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;
    if (!providerTable)
        return WN_NO_NETWORK;
    if (enumerator->providerIndex >= providerTable->numProviders)
        return WN_NO_MORE_ENTRIES;
    if (!providerTable->table[enumerator->providerIndex].enumResource)
        return WN_BAD_VALUE;
    return providerTable->table[enumerator->providerIndex].enumResource(
     enumerator->handle, lpcCount, lpBuffer, lpBufferSize);
}

static DWORD _enumerateContextW(PWNetEnumerator enumerator, LPDWORD lpcCount,
 LPVOID lpBuffer, LPDWORD lpBufferSize)
{
    DWORD ret;
    size_t cchEntireNetworkLen, bytesNeeded;

    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_CONTEXT)
        return WN_BAD_VALUE;
    if (!lpcCount)
        return WN_BAD_POINTER;
    if (!lpBuffer)
        return WN_BAD_POINTER;
    if (!lpBufferSize)
        return WN_BAD_POINTER;
    if (!providerTable)
        return WN_NO_NETWORK;

    cchEntireNetworkLen = strlenW(providerTable->entireNetwork) + 1;
    bytesNeeded = sizeof(NETRESOURCEW) + cchEntireNetworkLen * sizeof(WCHAR);
    if (*lpBufferSize < bytesNeeded)
    {
        *lpBufferSize = bytesNeeded;
        ret = WN_MORE_DATA;
    }
    else
    {
        LPNETRESOURCEW lpNet = lpBuffer;

        lpNet->dwScope = RESOURCE_GLOBALNET;
        lpNet->dwType = enumerator->dwType;
        lpNet->dwDisplayType = RESOURCEDISPLAYTYPE_ROOT;
        lpNet->dwUsage = RESOURCEUSAGE_CONTAINER;
        lpNet->lpLocalName = NULL;
        lpNet->lpRemoteName = NULL;
        lpNet->lpProvider = NULL;
        /* odd, but correct: put comment at end of buffer, so it won't get
         * overwritten by subsequent calls to a provider's enumResource
         */
        lpNet->lpComment = (LPWSTR)((LPBYTE)lpBuffer + *lpBufferSize -
         (cchEntireNetworkLen * sizeof(WCHAR)));
        strcpyW(lpNet->lpComment, providerTable->entireNetwork);
        ret = WN_SUCCESS;
    }
    if (ret == WN_SUCCESS)
    {
        DWORD bufferSize = *lpBufferSize - bytesNeeded;

        /* "Entire Network" entry enumerated--morph this into a global
         * enumerator.  enumerator->lpNet continues to be NULL, since it has
         * no meaning when the scope isn't RESOURCE_GLOBALNET.
         */
        enumerator->enumType = WNET_ENUMERATOR_TYPE_GLOBAL;
        ret = _enumerateGlobalW(enumerator, lpcCount,
         (LPBYTE)lpBuffer + bytesNeeded, &bufferSize);
        if (ret == WN_SUCCESS)
        {
            /* reflect the fact that we already enumerated "Entire Network" */
            (*lpcCount)++;
            *lpBufferSize = bufferSize + bytesNeeded;
        }
        else
        {
            /* the provider enumeration failed, but we already succeeded in
             * enumerating "Entire Network"--leave type as global to allow a
             * retry, but indicate success with a count of one.
             */
            ret = WN_SUCCESS;
            *lpcCount = 1;
            *lpBufferSize = bytesNeeded;
        }
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static DWORD _copyStringToEnumW(const WCHAR *source, DWORD* left, void** end)
{
    DWORD len;
    WCHAR* local = *end;

    len = strlenW(source) + 1;
    len *= sizeof(WCHAR);
    if (*left < len)
        return WN_MORE_DATA;

    local -= (len / sizeof(WCHAR));
    memcpy(local, source, len);
    *left -= len;
    *end = local;

    return WN_SUCCESS;
}

static DWORD _enumerateConnectedW(PWNetEnumerator enumerator, DWORD* user_count,
                                  void* user_buffer, DWORD* user_size)
{
    DWORD ret, index, count, total_count, size, i, left;
    void* end;
    NETRESOURCEW* curr, * buffer;
    HANDLE* handles;

    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_CONNECTED)
        return WN_BAD_VALUE;
    if (!user_count || !user_buffer || !user_size)
        return WN_BAD_POINTER;
    if (!providerTable)
        return WN_NO_NETWORK;

    handles = enumerator->specific.handles;
    left = *user_size;
    size = *user_size;
    buffer = HeapAlloc(GetProcessHeap(), 0, *user_size);
    if (!buffer)
        return WN_NO_NETWORK;

    curr = user_buffer;
    end = (char *)user_buffer + size;
    count = *user_count;
    total_count = 0;

    ret = WN_NO_MORE_ENTRIES;
    for (index = 0; index < providerTable->numProviders; index++)
    {
        if (providerTable->table[index].dwEnumScopes)
        {
            if (handles[index] == 0)
            {
                ret = providerTable->table[index].openEnum(enumerator->dwScope,
                                                           enumerator->dwType,
                                                           enumerator->dwUsage,
                                                           NULL, &handles[index]);
                if (ret != WN_SUCCESS)
                    continue;
            }

            ret = providerTable->table[index].enumResource(handles[index],
                                                           &count, buffer,
                                                           &size);
            total_count += count;
            if (ret == WN_MORE_DATA)
                break;

            if (ret == WN_SUCCESS)
            {
                for (i = 0; i < count; ++i)
                {
                    if (left < sizeof(NETRESOURCEW))
                    {
                        ret = WN_MORE_DATA;
                        break;
                    }

                    memcpy(curr, &buffer[i], sizeof(NETRESOURCEW));
                    left -= sizeof(NETRESOURCEW);

                    ret = _copyStringToEnumW(buffer[i].lpLocalName, &left, &end);
                    if (ret == WN_MORE_DATA)
                        break;
                    curr->lpLocalName = end;

                    ret = _copyStringToEnumW(buffer[i].lpRemoteName, &left, &end);
                    if (ret == WN_MORE_DATA)
                        break;
                    curr->lpRemoteName = end;

                    ret = _copyStringToEnumW(buffer[i].lpProvider, &left, &end);
                    if (ret == WN_MORE_DATA)
                        break;
                    curr->lpProvider = end;

                    ++curr;
                }

                size = left;
            }

            if (*user_count != -1)
                count = *user_count - total_count;
            else
                count = *user_count;
        }
    }

    if (total_count == 0)
        ret = WN_NO_MORE_ENTRIES;

    *user_count = total_count;
    if (ret != WN_MORE_DATA && ret != WN_NO_MORE_ENTRIES)
        ret = WN_SUCCESS;

    HeapFree(GetProcessHeap(), 0, buffer);

    TRACE("Returning %d\n", ret);
    return ret;
}

#ifdef __REACTOS__
static const WCHAR connectionType[] = { 'C','o','n','n','e','c','t',
 'i','o','n','T','y','p','e',0 };
static const WCHAR providerName[] = { 'P','r','o','v','i','d','e','r',
 'N','a','m','e',0 };
static const WCHAR remotePath[] = { 'R','e','m','o','t','e','P',
 'a','t','h',0 };

static DWORD _enumeratorRememberedW(PWNetEnumerator enumerator, DWORD* user_count,
                                    void* user_buffer, DWORD* user_size)
{
    HKEY registry, connection;
    WCHAR buffer[255];
    DWORD index, ret, type, len, size, provider_size, remote_size, full_size, total_count, size_left = *user_size;
    NETRESOURCEW * net_buffer = user_buffer;
    WCHAR * str;

    if (!enumerator)
        return WN_BAD_POINTER;
    if (enumerator->enumType != WNET_ENUMERATOR_TYPE_REMEMBERED)
        return WN_BAD_VALUE;
    if (!user_count || !user_buffer || !user_size)
        return WN_BAD_POINTER;
    if (!providerTable)
        return WN_NO_NETWORK;

    /* We will do the work in a single loop, so here is some things:
     * we write netresource at the begin of the user buffer
     * we write strings at the end of the user buffer
     */
    total_count = 0;
    type = enumerator->dwType;
    registry = enumerator->specific.remembered.registry;
    str = (WCHAR *)((ULONG_PTR)user_buffer + *user_size - sizeof(WCHAR));
    for (index = enumerator->specific.remembered.index; ; ++index)
    {
        enumerator->specific.remembered.index = index;

        if (*user_count != -1 && total_count == *user_count)
        {
            ret = WN_SUCCESS;
            break;
        }

        if (size_left < sizeof(NETRESOURCEW))
        {
            ret = WN_MORE_DATA;
            break;
        }

        len = ARRAY_SIZE(buffer);
        ret = RegEnumKeyExW(registry, index, buffer, &len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS)
        {
            /* We're done, that's a success! */
            if (ret == ERROR_NO_MORE_ITEMS)
            {
                ret = WN_SUCCESS;
                break;
            }

            continue;
        }

        if (RegOpenKeyExW(registry, buffer, 0, KEY_READ, &connection) != ERROR_SUCCESS)
        {
            continue;
        }

        size = sizeof(DWORD);
        RegQueryValueExW(connection, connectionType, NULL, NULL, (BYTE *)&net_buffer->dwType, &size);
        if (type != RESOURCETYPE_ANY && net_buffer->dwType != type)
        {
            RegCloseKey(connection);
            continue;
        }

        net_buffer->dwScope = RESOURCE_REMEMBERED;
        net_buffer->dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
        net_buffer->dwUsage = RESOURCEUSAGE_CONNECTABLE;

        size_left -= sizeof(NETRESOURCEW);

        /* Compute the whole size */
        full_size = 0;
        if (RegQueryValueExW(connection, providerName, NULL, NULL, NULL, &provider_size) != ERROR_SUCCESS)
        {
            RegCloseKey(connection);
            continue;
        }
        full_size += provider_size;

        if (RegQueryValueExW(connection, remotePath, NULL, NULL, NULL, &remote_size) != ERROR_SUCCESS)
        {
            RegCloseKey(connection);
            continue;
        }
        full_size += remote_size;

        /* FIXME: this only supports drive letters */
        full_size += 3 * sizeof(WCHAR);
        if (full_size > size_left)
        {
            RegCloseKey(connection);
            ret = WN_MORE_DATA;
            break;
        }

        str -= 3;
        str[0] = buffer[0];
        str[1] = L':';
        str[2] = 0;
        net_buffer->lpLocalName = str;
        size_left -= 3 * sizeof(WCHAR);

        size = provider_size;
        str -= (provider_size / sizeof(WCHAR));
        ret = RegQueryValueExW(connection, providerName, NULL, NULL, (BYTE *)str, &size);
        if (ret == ERROR_MORE_DATA)
        {
            RegCloseKey(connection);
            ret = WN_MORE_DATA;
            break;
        }
        net_buffer->lpProvider = str;
        size_left -= size;

        size = remote_size;
        str -= (remote_size / sizeof(WCHAR));
        ret = RegQueryValueExW(connection, remotePath, NULL, NULL, (BYTE *)str, &size);
        if (ret == ERROR_MORE_DATA)
        {
            RegCloseKey(connection);
            ret = WN_MORE_DATA;
            break;
        }
        net_buffer->lpRemoteName = str;
        size_left -= size;

        RegCloseKey(connection);

        net_buffer->lpComment = NULL;

        ++total_count;
        ++net_buffer;
    }

    if (total_count == 0)
        ret = WN_NO_MORE_ENTRIES;

    *user_count = total_count;

    if (ret != WN_MORE_DATA && ret != WN_NO_MORE_ENTRIES)
        ret = WN_SUCCESS;

    if (ret == WN_MORE_DATA)
        *user_size = *user_size + full_size;

    return ret;
}
#endif

/*********************************************************************
 * WNetEnumResourceW [MPR.@]
 */
DWORD WINAPI WNetEnumResourceW( HANDLE hEnum, LPDWORD lpcCount,
                                LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE( "(%p, %p, %p, %p)\n", hEnum, lpcCount, lpBuffer, lpBufferSize );

    if (!hEnum)
        ret = WN_BAD_POINTER;
    else if (!lpcCount)
        ret = WN_BAD_POINTER;
    else if (!lpBuffer)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else if (*lpBufferSize < sizeof(NETRESOURCEW))
    {
        *lpBufferSize = sizeof(NETRESOURCEW);
        ret = WN_MORE_DATA;
    }
    else
    {
        PWNetEnumerator enumerator = (PWNetEnumerator)hEnum;

        switch (enumerator->enumType)
        {
#ifndef __REACTOS__
            case WNET_ENUMERATOR_TYPE_NULL:
                ret = WN_NO_MORE_ENTRIES;
                break;
#endif
            case WNET_ENUMERATOR_TYPE_GLOBAL:
                ret = _enumerateGlobalW(enumerator, lpcCount, lpBuffer,
                 lpBufferSize);
                break;
            case WNET_ENUMERATOR_TYPE_PROVIDER:
                ret = _enumerateProviderW(enumerator, lpcCount, lpBuffer,
                 lpBufferSize);
                break;
            case WNET_ENUMERATOR_TYPE_CONTEXT:
                ret = _enumerateContextW(enumerator, lpcCount, lpBuffer,
                 lpBufferSize);
                break;
            case WNET_ENUMERATOR_TYPE_CONNECTED:
                ret = _enumerateConnectedW(enumerator, lpcCount, lpBuffer,
                 lpBufferSize);
                break;
#ifdef __REACTOS__
            case WNET_ENUMERATOR_TYPE_REMEMBERED:
                ret = _enumeratorRememberedW(enumerator, lpcCount, lpBuffer,
                 lpBufferSize);
                break;
#endif
            default:
                WARN("bogus enumerator type!\n");
                ret = WN_NO_NETWORK;
        }
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*********************************************************************
 * WNetCloseEnum [MPR.@]
 */
DWORD WINAPI WNetCloseEnum( HANDLE hEnum )
{
    DWORD ret, index;
    HANDLE *handles;

    TRACE( "(%p)\n", hEnum );

    if (hEnum)
    {
        PWNetEnumerator enumerator = (PWNetEnumerator)hEnum;

        switch (enumerator->enumType)
        {
#ifndef __REACTOS__
            case WNET_ENUMERATOR_TYPE_NULL:
                ret = WN_SUCCESS;
                break;
#endif
            case WNET_ENUMERATOR_TYPE_GLOBAL:
                if (enumerator->specific.net)
                    _freeEnumNetResource(enumerator->specific.net);
                if (enumerator->handle)
                    providerTable->table[enumerator->providerIndex].
                     closeEnum(enumerator->handle);
                ret = WN_SUCCESS;
                break;
            case WNET_ENUMERATOR_TYPE_PROVIDER:
                if (enumerator->handle)
                    providerTable->table[enumerator->providerIndex].
                     closeEnum(enumerator->handle);
                ret = WN_SUCCESS;
                break;
            case WNET_ENUMERATOR_TYPE_CONNECTED:
                handles = enumerator->specific.handles;
                for (index = 0; index < providerTable->numProviders; index++)
                {
                    if (providerTable->table[index].dwEnumScopes && handles[index])
                        providerTable->table[index].closeEnum(handles[index]);
                }
                HeapFree(GetProcessHeap(), 0, handles);
                ret = WN_SUCCESS;
                break;
#ifdef __REACTOS__
            case WNET_ENUMERATOR_TYPE_REMEMBERED:
                RegCloseKey(enumerator->specific.remembered.registry);
                ret = WN_SUCCESS;
                break;
#endif
            default:
                WARN("bogus enumerator type!\n");
                ret = WN_BAD_HANDLE;
        }
        HeapFree(GetProcessHeap(), 0, hEnum);
    }
    else
        ret = WN_BAD_HANDLE;
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*********************************************************************
 * WNetGetResourceInformationA [MPR.@]
 *
 * See WNetGetResourceInformationW
 */
DWORD WINAPI WNetGetResourceInformationA( LPNETRESOURCEA lpNetResource,
                                          LPVOID lpBuffer, LPDWORD cbBuffer,
                                          LPSTR *lplpSystem )
{
    DWORD ret;

    TRACE( "(%p, %p, %p, %p)\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    if (!providerTable || providerTable->numProviders == 0)
        ret = WN_NO_NETWORK;
    else if (lpNetResource)
    {
        LPNETRESOURCEW lpNetResourceW = NULL;
        DWORD size = 1024, count = 1;
        DWORD len;

        lpNetResourceW = HeapAlloc(GetProcessHeap(), 0, size);
        ret = _thunkNetResourceArrayAToW(lpNetResource, &count, lpNetResourceW, &size);
        if (ret == WN_MORE_DATA)
        {
            HeapFree(GetProcessHeap(), 0, lpNetResourceW);
            lpNetResourceW = HeapAlloc(GetProcessHeap(), 0, size);
            if (lpNetResourceW)
                ret = _thunkNetResourceArrayAToW(lpNetResource,
                        &count, lpNetResourceW, &size);
            else
                ret = WN_OUT_OF_MEMORY;
        }
        if (ret == WN_SUCCESS)
        {
            LPWSTR lpSystemW = NULL;
            LPVOID lpBufferW;
            size = 1024;
            lpBufferW = HeapAlloc(GetProcessHeap(), 0, size);
            if (lpBufferW)
            {
                ret = WNetGetResourceInformationW(lpNetResourceW,
                        lpBufferW, &size, &lpSystemW);
                if (ret == WN_MORE_DATA)
                {
                    HeapFree(GetProcessHeap(), 0, lpBufferW);
                    lpBufferW = HeapAlloc(GetProcessHeap(), 0, size);
                    if (lpBufferW)
                        ret = WNetGetResourceInformationW(lpNetResourceW,
                            lpBufferW, &size, &lpSystemW);
                    else
                        ret = WN_OUT_OF_MEMORY;
                }
                if (ret == WN_SUCCESS)
                {
                    ret = _thunkNetResourceArrayWToA(lpBufferW,
                            &count, lpBuffer, cbBuffer);
                    HeapFree(GetProcessHeap(), 0, lpNetResourceW);
                    lpNetResourceW = lpBufferW;
                    size = sizeof(NETRESOURCEA);
                    size += WideCharToMultiByte(CP_ACP, 0, lpNetResourceW->lpRemoteName,
                            -1, NULL, 0, NULL, NULL);
                    size += WideCharToMultiByte(CP_ACP, 0, lpNetResourceW->lpProvider,
                            -1, NULL, 0, NULL, NULL);

                    len = WideCharToMultiByte(CP_ACP, 0, lpSystemW,
                                      -1, NULL, 0, NULL, NULL);
                    if ((len) && ( size + len < *cbBuffer))
                    {
                        *lplpSystem = (char*)lpBuffer + *cbBuffer - len;
                        WideCharToMultiByte(CP_ACP, 0, lpSystemW, -1,
                             *lplpSystem, len, NULL, NULL);
                         ret = WN_SUCCESS;
                    }
                    else
                        ret = WN_MORE_DATA;
                }
                else
                    ret = WN_OUT_OF_MEMORY;
                HeapFree(GetProcessHeap(), 0, lpBufferW);
            }
            else
                ret = WN_OUT_OF_MEMORY;
            HeapFree(GetProcessHeap(), 0, lpSystemW);
        }
        HeapFree(GetProcessHeap(), 0, lpNetResourceW);
    }
    else
        ret = WN_NO_NETWORK;

    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*********************************************************************
 * WNetGetResourceInformationW [MPR.@]
 *
 * WNetGetResourceInformationW function identifies the network provider
 * that owns the resource and gets information about the type of the resource.
 *
 * PARAMS:
 *  lpNetResource    [ I]    the pointer to NETRESOURCEW structure, that
 *                          defines a network resource.
 *  lpBuffer         [ O]   the pointer to buffer, containing result. It
 *                          contains NETRESOURCEW structure and strings to
 *                          which the members of the NETRESOURCEW structure
 *                          point.
 *  cbBuffer         [I/O] the pointer to DWORD number - size of buffer
 *                          in bytes.
 *  lplpSystem       [ O]   the pointer to string in the output buffer,
 *                          containing the part of the resource name without
 *                          names of the server and share.
 *
 * RETURNS:
 *  NO_ERROR if the function succeeds. System error code if the function fails.
 */

DWORD WINAPI WNetGetResourceInformationW( LPNETRESOURCEW lpNetResource,
                                          LPVOID lpBuffer, LPDWORD cbBuffer,
                                          LPWSTR *lplpSystem )
{
    DWORD ret = WN_NO_NETWORK;
    DWORD index;

    TRACE( "(%p, %p, %p, %p)\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem);

    if (!(lpBuffer))
        ret = WN_OUT_OF_MEMORY;
    else if (providerTable != NULL)
    {
        /* FIXME: For function value of a variable is indifferent, it does
         * search of all providers in a network.
         */
        for (index = 0; index < providerTable->numProviders; index++)
        {
            if(providerTable->table[index].getCaps(WNNC_DIALOG) &
                WNNC_DLG_GETRESOURCEINFORMATION)
            {
                if (providerTable->table[index].getResourceInformation)
                    ret = providerTable->table[index].getResourceInformation(
                        lpNetResource, lpBuffer, cbBuffer, lplpSystem);
                else
                    ret = WN_NO_NETWORK;
                if (ret == WN_SUCCESS)
                    break;
            }
        }
    }
    if (ret)
        SetLastError(ret);
    return ret;
}

/*********************************************************************
 * WNetGetResourceParentA [MPR.@]
 */
DWORD WINAPI WNetGetResourceParentA( LPNETRESOURCEA lpNetResource,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n",
           lpNetResource, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceParentW [MPR.@]
 */
DWORD WINAPI WNetGetResourceParentW( LPNETRESOURCEW lpNetResource,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n",
           lpNetResource, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}



/*
 * Connection Functions
 */

/*********************************************************************
 *  WNetAddConnectionA [MPR.@]
 */
DWORD WINAPI WNetAddConnectionA( LPCSTR lpRemoteName, LPCSTR lpPassword,
                                 LPCSTR lpLocalName )
{
    NETRESOURCEA resourcesA;

    memset(&resourcesA, 0, sizeof(resourcesA));
    resourcesA.lpRemoteName = (LPSTR)lpRemoteName;
    resourcesA.lpLocalName = (LPSTR)lpLocalName;
    return WNetUseConnectionA(NULL, &resourcesA, lpPassword, NULL, 0, NULL, 0, NULL);
}

/*********************************************************************
 *  WNetAddConnectionW [MPR.@]
 */
DWORD WINAPI WNetAddConnectionW( LPCWSTR lpRemoteName, LPCWSTR lpPassword,
                                 LPCWSTR lpLocalName )
{
    NETRESOURCEW resourcesW;

    memset(&resourcesW, 0, sizeof(resourcesW));
    resourcesW.lpRemoteName = (LPWSTR)lpRemoteName;
    resourcesW.lpLocalName = (LPWSTR)lpLocalName;
    return WNetUseConnectionW(NULL, &resourcesW, lpPassword, NULL, 0, NULL, 0, NULL);
}

/*********************************************************************
 *  WNetAddConnection2A [MPR.@]
 */
DWORD WINAPI WNetAddConnection2A( LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID,
                                  DWORD dwFlags )
{
    return WNetUseConnectionA(NULL, lpNetResource, lpPassword, lpUserID, dwFlags,
                              NULL, 0, NULL);
}

/*********************************************************************
 * WNetAddConnection2W [MPR.@]
 */
DWORD WINAPI WNetAddConnection2W( LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID,
                                  DWORD dwFlags )
{
    return WNetUseConnectionW(NULL, lpNetResource, lpPassword, lpUserID, dwFlags,
                              NULL, 0, NULL);
}

/*********************************************************************
 * WNetAddConnection3A [MPR.@]
 */
DWORD WINAPI WNetAddConnection3A( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID,
                                  DWORD dwFlags )
{
    return WNetUseConnectionA(hwndOwner, lpNetResource, lpPassword, lpUserID,
                              dwFlags, NULL, 0, NULL);
}

/*********************************************************************
 * WNetAddConnection3W [MPR.@]
 */
DWORD WINAPI WNetAddConnection3W( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID,
                                  DWORD dwFlags )
{
    return WNetUseConnectionW(hwndOwner, lpNetResource, lpPassword, lpUserID,
                              dwFlags, NULL, 0, NULL);
}

struct use_connection_context
{
    HWND hwndOwner;
    NETRESOURCEW *resource;
    NETRESOURCEA *resourceA; /* only set for WNetUseConnectionA */
    WCHAR *password;
    WCHAR *userid;
    DWORD flags;
    void *accessname;
    DWORD *buffer_size;
    DWORD *result;
    DWORD (*pre_set_accessname)(struct use_connection_context*, WCHAR *);
    void  (*set_accessname)(struct use_connection_context*, WCHAR *);
};

static DWORD use_connection_pre_set_accessnameW(struct use_connection_context *ctxt, WCHAR *local_name)
{
    if (ctxt->accessname && ctxt->buffer_size && *ctxt->buffer_size)
    {
        DWORD len;

        if (local_name)
            len = strlenW(local_name);
        else
            len = strlenW(ctxt->resource->lpRemoteName);

        if (++len > *ctxt->buffer_size)
        {
            *ctxt->buffer_size = len;
            return ERROR_MORE_DATA;
        }
    }
    else
        ctxt->accessname = NULL;

    return ERROR_SUCCESS;
}

static void use_connection_set_accessnameW(struct use_connection_context *ctxt, WCHAR *local_name)
{
    WCHAR *accessname = ctxt->accessname;
    if (local_name)
    {
        strcpyW(accessname, local_name);
        if (ctxt->result)
            *ctxt->result = CONNECT_LOCALDRIVE;
    }
    else
        strcpyW(accessname, ctxt->resource->lpRemoteName);
}

static DWORD wnet_use_provider( struct use_connection_context *ctxt, NETRESOURCEW * netres, WNetProvider *provider, BOOLEAN redirect )
{
    DWORD caps, ret;

    caps = provider->getCaps(WNNC_CONNECTION);
    if (!(caps & (WNNC_CON_ADDCONNECTION | WNNC_CON_ADDCONNECTION3)))
        return ERROR_BAD_PROVIDER;

    ret = WN_ACCESS_DENIED;
    do
    {
        if ((caps & WNNC_CON_ADDCONNECTION3) && provider->addConnection3)
            ret = provider->addConnection3(ctxt->hwndOwner, netres, ctxt->password, ctxt->userid, ctxt->flags);
        else if ((caps & WNNC_CON_ADDCONNECTION) && provider->addConnection)
            ret = provider->addConnection(netres, ctxt->password, ctxt->userid);

        if (ret == WN_ALREADY_CONNECTED && redirect)
            netres->lpLocalName[0] -= 1;
    } while (redirect && ret == WN_ALREADY_CONNECTED && netres->lpLocalName[0] >= 'C');

    if (ret == WN_SUCCESS && ctxt->accessname)
        ctxt->set_accessname(ctxt, netres->lpLocalName);

    return ret;
}

static DWORD wnet_use_connection( struct use_connection_context *ctxt )
{
    WNetProvider *provider;
    DWORD index, ret = WN_NO_NETWORK;
    BOOL redirect = FALSE;
    WCHAR letter[3] = {'Z', ':', 0};
    NETRESOURCEW netres;

    if (!providerTable || providerTable->numProviders == 0)
        return WN_NO_NETWORK;

    if (!ctxt->resource)
        return ERROR_INVALID_PARAMETER;
    netres = *ctxt->resource;

    if (!netres.lpLocalName && (ctxt->flags & CONNECT_REDIRECT))
    {
        if (netres.dwType != RESOURCETYPE_DISK && netres.dwType != RESOURCETYPE_PRINT)
            return ERROR_BAD_DEV_TYPE;

        if (netres.dwType == RESOURCETYPE_PRINT)
        {
            FIXME("Local device selection is not implemented for printers.\n");
            return WN_NO_NETWORK;
        }

        redirect = TRUE;
        netres.lpLocalName = letter;
    }

    if (ctxt->flags & CONNECT_INTERACTIVE)
        return ERROR_BAD_NET_NAME;

    if ((ret = ctxt->pre_set_accessname(ctxt, netres.lpLocalName)))
        return ret;

    if (netres.lpProvider)
    {
        index = _findProviderIndexW(netres.lpProvider);
        if (index == BAD_PROVIDER_INDEX)
            return ERROR_BAD_PROVIDER;

        provider = &providerTable->table[index];
        ret = wnet_use_provider(ctxt, &netres, provider, redirect);
    }
    else
    {
        for (index = 0; index < providerTable->numProviders; index++)
        {
            provider = &providerTable->table[index];
            ret = wnet_use_provider(ctxt, &netres, provider, redirect);
            if (ret == WN_SUCCESS || ret == WN_ALREADY_CONNECTED)
                break;
        }
    }

#ifdef __REACTOS__
    if (ret == WN_SUCCESS && ctxt->flags & CONNECT_UPDATE_PROFILE)
    {
        HKEY user_profile;

        if (netres.dwType == RESOURCETYPE_PRINT)
        {
            FIXME("Persistent connection are not supported for printers\n");
            return ret;
        }

        if (RegOpenCurrentUser(KEY_ALL_ACCESS, &user_profile) == ERROR_SUCCESS)
        {
            HKEY network;
            WCHAR subkey[10] = {'N', 'e', 't', 'w', 'o', 'r', 'k', '\\', netres.lpLocalName[0], 0};

            if (RegCreateKeyExW(user_profile, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &network, NULL) == ERROR_SUCCESS)
            {
                DWORD dword_arg = RESOURCETYPE_DISK;
                DWORD len = (strlenW(provider->name) + 1) * sizeof(WCHAR);

                RegSetValueExW(network, L"ConnectionType", 0, REG_DWORD, (const BYTE *)&dword_arg, sizeof(DWORD));
                RegSetValueExW(network, L"ProviderName", 0, REG_SZ, (const BYTE *)provider->name, len);
                dword_arg = provider->dwNetType;
                RegSetValueExW(network, L"ProviderType", 0, REG_DWORD, (const BYTE *)&dword_arg, sizeof(DWORD));
                len = (strlenW(netres.lpRemoteName) + 1) * sizeof(WCHAR);
                RegSetValueExW(network, L"RemotePath", 0, REG_SZ, (const BYTE *)netres.lpRemoteName, len);
                len = 0;
                RegSetValueExW(network, L"UserName", 0, REG_SZ, (const BYTE *)netres.lpRemoteName, len);
                RegCloseKey(network);
            }

            RegCloseKey(user_profile);
        }
    }
#endif

    return ret;
}

/*****************************************************************
 *  WNetUseConnectionW [MPR.@]
 */
DWORD WINAPI WNetUseConnectionW( HWND hwndOwner, NETRESOURCEW *resource, LPCWSTR password,
    LPCWSTR userid, DWORD flags, LPWSTR accessname, DWORD *buffer_size, DWORD *result )
{
    struct use_connection_context ctxt;

    TRACE( "(%p, %p, %p, %s, 0x%08X, %p, %p, %p)\n",
           hwndOwner, resource, password, debugstr_w(userid), flags,
           accessname, buffer_size, result );

    ctxt.hwndOwner = hwndOwner;
    ctxt.resource = resource;
    ctxt.resourceA = NULL;
    ctxt.password = (WCHAR*)password;
    ctxt.userid = (WCHAR*)userid;
    ctxt.flags = flags;
    ctxt.accessname = accessname;
    ctxt.buffer_size = buffer_size;
    ctxt.result = result;
    ctxt.pre_set_accessname = use_connection_pre_set_accessnameW;
    ctxt.set_accessname = use_connection_set_accessnameW;

    return wnet_use_connection(&ctxt);
}

static DWORD use_connection_pre_set_accessnameA(struct use_connection_context *ctxt, WCHAR *local_name)
{
    if (ctxt->accessname && ctxt->buffer_size && *ctxt->buffer_size)
    {
        DWORD len;

        if (local_name)
            len = WideCharToMultiByte(CP_ACP, 0, local_name, -1, NULL, 0, NULL, NULL) - 1;
        else
            len = strlen(ctxt->resourceA->lpRemoteName);

        if (++len > *ctxt->buffer_size)
        {
            *ctxt->buffer_size = len;
            return ERROR_MORE_DATA;
        }
    }
    else
        ctxt->accessname = NULL;

    return ERROR_SUCCESS;
}

static void use_connection_set_accessnameA(struct use_connection_context *ctxt, WCHAR *local_name)
{
    char *accessname = ctxt->accessname;
    if (local_name)
    {
        WideCharToMultiByte(CP_ACP, 0, local_name, -1, accessname, *ctxt->buffer_size, NULL, NULL);
        if (ctxt->result)
            *ctxt->result = CONNECT_LOCALDRIVE;
    }
    else
        strcpy(accessname, ctxt->resourceA->lpRemoteName);
}

static LPWSTR strdupAtoW( LPCSTR str )
{
    LPWSTR ret;
    INT len;

    if (!str) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

static void netresource_a_to_w( NETRESOURCEA *resourceA, NETRESOURCEW *resourceW )
{
    resourceW->dwScope = resourceA->dwScope;
    resourceW->dwType = resourceA->dwType;
    resourceW->dwDisplayType = resourceA->dwDisplayType;
    resourceW->dwUsage = resourceA->dwUsage;
    resourceW->lpLocalName = strdupAtoW(resourceA->lpLocalName);
    resourceW->lpRemoteName = strdupAtoW(resourceA->lpRemoteName);
    resourceW->lpComment = strdupAtoW(resourceA->lpComment);
    resourceW->lpProvider = strdupAtoW(resourceA->lpProvider);
}

static void free_netresourceW( NETRESOURCEW *resource )
{
    HeapFree(GetProcessHeap(), 0, resource->lpLocalName);
    HeapFree(GetProcessHeap(), 0, resource->lpRemoteName);
    HeapFree(GetProcessHeap(), 0, resource->lpComment);
    HeapFree(GetProcessHeap(), 0, resource->lpProvider);
}

/*****************************************************************
 *  WNetUseConnectionA [MPR.@]
 */
DWORD WINAPI WNetUseConnectionA( HWND hwndOwner, NETRESOURCEA *resource,
    LPCSTR password, LPCSTR userid, DWORD flags, LPSTR accessname,
    DWORD *buffer_size, DWORD *result )
{
    struct use_connection_context ctxt;
    NETRESOURCEW resourceW;
    DWORD ret;

    TRACE( "(%p, %p, %p, %s, 0x%08X, %p, %p, %p)\n", hwndOwner, resource, password, debugstr_a(userid), flags,
        accessname, buffer_size, result );

    netresource_a_to_w(resource, &resourceW);

    ctxt.hwndOwner = hwndOwner;
    ctxt.resource = &resourceW;
    ctxt.resourceA = resource;
    ctxt.password = strdupAtoW(password);
    ctxt.userid = strdupAtoW(userid);
    ctxt.flags = flags;
    ctxt.accessname = accessname;
    ctxt.buffer_size = buffer_size;
    ctxt.result = result;
    ctxt.pre_set_accessname = use_connection_pre_set_accessnameA;
    ctxt.set_accessname = use_connection_set_accessnameA;

    ret = wnet_use_connection(&ctxt);

    free_netresourceW(&resourceW);
    HeapFree(GetProcessHeap(), 0, ctxt.password);
    HeapFree(GetProcessHeap(), 0, ctxt.userid);

    return ret;
}

/*********************************************************************
 *  WNetCancelConnectionA [MPR.@]
 */
DWORD WINAPI WNetCancelConnectionA( LPCSTR lpName, BOOL fForce )
{
    return WNetCancelConnection2A(lpName, 0, fForce);
}

/*********************************************************************
 *  WNetCancelConnectionW [MPR.@]
 */
DWORD WINAPI WNetCancelConnectionW( LPCWSTR lpName, BOOL fForce )
{
    return WNetCancelConnection2W(lpName, 0, fForce);
}

/*********************************************************************
 *  WNetCancelConnection2A [MPR.@]
 */
DWORD WINAPI WNetCancelConnection2A( LPCSTR lpName, DWORD dwFlags, BOOL fForce )
{
    DWORD ret;
    WCHAR * name = strdupAtoW(lpName);
    if (!name)
        return ERROR_NOT_CONNECTED;

    ret = WNetCancelConnection2W(name, dwFlags, fForce);
    HeapFree(GetProcessHeap(), 0, name);

    return ret;
}

/*********************************************************************
 *  WNetCancelConnection2W [MPR.@]
 */
DWORD WINAPI WNetCancelConnection2W( LPCWSTR lpName, DWORD dwFlags, BOOL fForce )
{
    DWORD ret = WN_NO_NETWORK;
    DWORD index;

    if (providerTable != NULL)
    {
        for (index = 0; index < providerTable->numProviders; index++)
        {
            if(providerTable->table[index].getCaps(WNNC_CONNECTION) &
                WNNC_CON_CANCELCONNECTION)
            {
                if (providerTable->table[index].cancelConnection)
                    ret = providerTable->table[index].cancelConnection((LPWSTR)lpName, fForce);
                else
                    ret = WN_NO_NETWORK;
                if (ret == WN_SUCCESS || ret == WN_OPEN_FILES)
                    break;
            }
        }
    }
#ifdef __REACTOS__

    if (dwFlags & CONNECT_UPDATE_PROFILE)
    {
        HKEY user_profile;
        WCHAR *coma = strchrW(lpName, ':');

        if (coma && RegOpenCurrentUser(KEY_ALL_ACCESS, &user_profile) == ERROR_SUCCESS)
        {
            WCHAR  *subkey;
            DWORD len;

            len = (ULONG_PTR)coma - (ULONG_PTR)lpName + sizeof(L"Network\\");
            subkey = HeapAlloc(GetProcessHeap(), 0, len);
            if (subkey)
            {
                strcpyW(subkey, L"Network\\");
                memcpy(subkey + (sizeof(L"Network\\") / sizeof(WCHAR)) - 1, lpName, (ULONG_PTR)coma - (ULONG_PTR)lpName);
                subkey[len / sizeof(WCHAR) - 1] = 0;

                TRACE("Removing: %S\n", subkey);

                RegDeleteKeyW(user_profile, subkey);
                HeapFree(GetProcessHeap(), 0, subkey);
            }

            RegCloseKey(user_profile);
        }
    }

#endif
    return ret;
}

/*****************************************************************
 *  WNetRestoreConnectionA [MPR.@]
 */
DWORD WINAPI WNetRestoreConnectionA( HWND hwndOwner, LPCSTR lpszDevice )
{
    FIXME( "(%p, %s), stub\n", hwndOwner, debugstr_a(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetRestoreConnectionW [MPR.@]
 */
DWORD WINAPI WNetRestoreConnectionW( HWND hwndOwner, LPCWSTR lpszDevice )
{
    FIXME( "(%p, %s), stub\n", hwndOwner, debugstr_w(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetGetConnectionA [MPR.@]
 *
 * RETURNS
 * - WN_BAD_LOCALNAME     lpLocalName makes no sense
 * - WN_NOT_CONNECTED     drive is a local drive
 * - WN_MORE_DATA         buffer isn't big enough
 * - WN_SUCCESS           success (net path in buffer)
 *
 * FIXME: need to test return values under different errors
 */
DWORD WINAPI WNetGetConnectionA( LPCSTR lpLocalName,
                                 LPSTR lpRemoteName, LPDWORD lpBufferSize )
{
    DWORD ret;

    if (!lpLocalName)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else if (!lpRemoteName && *lpBufferSize)
        ret = WN_BAD_POINTER;
    else
    {
        int len = MultiByteToWideChar(CP_ACP, 0, lpLocalName, -1, NULL, 0);

        if (len)
        {
            PWSTR wideLocalName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

            if (wideLocalName)
            {
                WCHAR wideRemoteStatic[MAX_PATH];
                DWORD wideRemoteSize = sizeof(wideRemoteStatic) / sizeof(WCHAR);

                MultiByteToWideChar(CP_ACP, 0, lpLocalName, -1, wideLocalName, len);

                /* try once without memory allocation */
                ret = WNetGetConnectionW(wideLocalName, wideRemoteStatic,
                 &wideRemoteSize);
                if (ret == WN_SUCCESS)
                {
                    int len = WideCharToMultiByte(CP_ACP, 0, wideRemoteStatic,
                     -1, NULL, 0, NULL, NULL);

                    if (len <= *lpBufferSize)
                    {
                        WideCharToMultiByte(CP_ACP, 0, wideRemoteStatic, -1,
                         lpRemoteName, *lpBufferSize, NULL, NULL);
                        ret = WN_SUCCESS;
                    }
                    else
                    {
                        *lpBufferSize = len;
                        ret = WN_MORE_DATA;
                    }
                }
                else if (ret == WN_MORE_DATA)
                {
                    PWSTR wideRemote = HeapAlloc(GetProcessHeap(), 0,
                     wideRemoteSize * sizeof(WCHAR));

                    if (wideRemote)
                    {
                        ret = WNetGetConnectionW(wideLocalName, wideRemote,
                         &wideRemoteSize);
                        if (ret == WN_SUCCESS)
                        {
                            if (len <= *lpBufferSize)
                            {
                                WideCharToMultiByte(CP_ACP, 0, wideRemoteStatic,
                                 -1, lpRemoteName, *lpBufferSize, NULL, NULL);
                                ret = WN_SUCCESS;
                            }
                            else
                            {
                                *lpBufferSize = len;
                                ret = WN_MORE_DATA;
                            }
                        }
                        HeapFree(GetProcessHeap(), 0, wideRemote);
                    }
                    else
                        ret = WN_OUT_OF_MEMORY;
                }
                HeapFree(GetProcessHeap(), 0, wideLocalName);
            }
            else
                ret = WN_OUT_OF_MEMORY;
        }
        else
            ret = WN_BAD_LOCALNAME;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/* find the network connection for a given drive; helper for WNetGetConnection */
static DWORD get_drive_connection( WCHAR letter, LPWSTR remote, LPDWORD size )
{
#ifndef __REACTOS__
    char buffer[1024];
    struct mountmgr_unix_drive *data = (struct mountmgr_unix_drive *)buffer;
    HANDLE mgr;
    DWORD ret = WN_NOT_CONNECTED;
    DWORD bytes_returned;

    if ((mgr = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                            0, 0 )) == INVALID_HANDLE_VALUE)
    {
        ERR( "failed to open mount manager err %u\n", GetLastError() );
        return ret;
    }
    memset( data, 0, sizeof(*data) );
    data->letter = letter;
    if (DeviceIoControl( mgr, IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE, data, sizeof(*data),
                         data, sizeof(buffer), &bytes_returned, NULL ))
    {
        char *p, *mount_point = buffer + data->mount_point_offset;
        DWORD len;

        if (data->mount_point_offset && !strncmp( mount_point, "unc/", 4 ))
        {
            mount_point += 2;
            mount_point[0] = '\\';
            for (p = mount_point; *p; p++) if (*p == '/') *p = '\\';

            len = MultiByteToWideChar( CP_UNIXCP, 0, mount_point, -1, NULL, 0 );
            if (len > *size)
            {
                *size = len;
                ret = WN_MORE_DATA;
            }
            else
            {
                *size = MultiByteToWideChar( CP_UNIXCP, 0, mount_point, -1, remote, *size);
                ret = WN_SUCCESS;
            }
        }
    }
    CloseHandle( mgr );
    return ret;
#else
    DWORD ret = WN_NO_NETWORK;
    DWORD index;
    WCHAR local[3] = {letter, ':', 0};

    if (providerTable != NULL)
    {
        for (index = 0; index < providerTable->numProviders; index++)
        {
            if(providerTable->table[index].getCaps(WNNC_CONNECTION) &
                WNNC_CON_GETCONNECTIONS)
            {
                if (providerTable->table[index].getConnection)
                    ret = providerTable->table[index].getConnection(
                        local, remote, size);
                else
                    ret = WN_NO_NETWORK;
                if (ret == WN_SUCCESS || ret == WN_MORE_DATA)
                    break;
            }
        }
    }
    if (ret)
        SetLastError(ret);
    return ret;
#endif
}

/**************************************************************************
 * WNetGetConnectionW [MPR.@]
 *
 * FIXME: need to test return values under different errors
 */
DWORD WINAPI WNetGetConnectionW( LPCWSTR lpLocalName,
                                 LPWSTR lpRemoteName, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE("(%s, %p, %p)\n", debugstr_w(lpLocalName), lpRemoteName,
     lpBufferSize);

    if (!lpLocalName)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else if (!lpRemoteName && *lpBufferSize)
        ret = WN_BAD_POINTER;
    else if (!lpLocalName[0])
        ret = WN_BAD_LOCALNAME;
    else
    {
        if (lpLocalName[1] == ':')
        {
            switch(GetDriveTypeW(lpLocalName))
            {
            case DRIVE_REMOTE:
                ret = get_drive_connection( lpLocalName[0], lpRemoteName, lpBufferSize );
                break;
            case DRIVE_REMOVABLE:
            case DRIVE_FIXED:
            case DRIVE_CDROM:
                TRACE("file is local\n");
                ret = WN_NOT_CONNECTED;
                break;
            default:
                ret = WN_BAD_LOCALNAME;
            }
        }
        else
            ret = WN_BAD_LOCALNAME;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/**************************************************************************
 * WNetSetConnectionA [MPR.@]
 */
DWORD WINAPI WNetSetConnectionA( LPCSTR lpName, DWORD dwProperty,
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08X, %p): stub\n", debugstr_a(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetSetConnectionW [MPR.@]
 */
DWORD WINAPI WNetSetConnectionW( LPCWSTR lpName, DWORD dwProperty,
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08X, %p): stub\n", debugstr_w(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameA [MPR.@]
 */
DWORD WINAPI WNetGetUniversalNameA ( LPCSTR lpLocalPath, DWORD dwInfoLevel,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    DWORD err, size;

    FIXME( "(%s, 0x%08X, %p, %p): stub\n",
           debugstr_a(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    switch (dwInfoLevel)
    {
    case UNIVERSAL_NAME_INFO_LEVEL:
    {
        LPUNIVERSAL_NAME_INFOA info = lpBuffer;

        if (GetDriveTypeA(lpLocalPath) != DRIVE_REMOTE)
        {
            err = ERROR_NOT_CONNECTED;
            break;
        }

        size = sizeof(*info) + lstrlenA(lpLocalPath) + 1;
        if (*lpBufferSize < size)
        {
            err = WN_MORE_DATA;
            break;
        }
        info->lpUniversalName = (char *)info + sizeof(*info);
        lstrcpyA(info->lpUniversalName, lpLocalPath);
        err = WN_NO_ERROR;
        break;
    }
    case REMOTE_NAME_INFO_LEVEL:
        err = WN_NOT_CONNECTED;
        break;

    default:
        err = WN_BAD_VALUE;
        break;
    }

    SetLastError(err);
    return err;
}

/*****************************************************************
 * WNetGetUniversalNameW [MPR.@]
 */
DWORD WINAPI WNetGetUniversalNameW ( LPCWSTR lpLocalPath, DWORD dwInfoLevel,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    DWORD err, size;

    FIXME( "(%s, 0x%08X, %p, %p): stub\n",
           debugstr_w(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    switch (dwInfoLevel)
    {
    case UNIVERSAL_NAME_INFO_LEVEL:
    {
        LPUNIVERSAL_NAME_INFOW info = lpBuffer;

        if (GetDriveTypeW(lpLocalPath) != DRIVE_REMOTE)
        {
            err = ERROR_NOT_CONNECTED;
            break;
        }

        size = sizeof(*info) + (lstrlenW(lpLocalPath) + 1) * sizeof(WCHAR);
        if (*lpBufferSize < size)
        {
            err = WN_MORE_DATA;
            break;
        }
        info->lpUniversalName = (LPWSTR)((char *)info + sizeof(*info));
        lstrcpyW(info->lpUniversalName, lpLocalPath);
        err = WN_NO_ERROR;
        break;
    }
    case REMOTE_NAME_INFO_LEVEL:
        err = WN_NO_NETWORK;
        break;

    default:
        err = WN_BAD_VALUE;
        break;
    }

    if (err != WN_NO_ERROR) SetLastError(err);
    return err;
}

/*****************************************************************
 * WNetClearConnections [MPR.@]
 */
DWORD WINAPI WNetClearConnections ( HWND owner )
{
    HANDLE connected;
    PWSTR connection;
    DWORD ret, size, count;
    NETRESOURCEW * resources, * iter;

    ret = WNetOpenEnumW(RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL, &connected);
    if (ret != WN_SUCCESS)
    {
        if (ret != WN_NO_NETWORK)
        {
            return ret;
        }

        /* Means no provider, then, clearing is OK */
        return WN_SUCCESS;
    }

    size = 0x1000;
    resources = HeapAlloc(GetProcessHeap(), 0, size);
    if (!resources)
    {
        WNetCloseEnum(connected);
        return WN_OUT_OF_MEMORY;
    }

    for (;;)
    {
        size = 0x1000;
        count = -1;

        memset(resources, 0, size);
        ret = WNetEnumResourceW(connected, &count, resources, &size);
        if (ret == WN_SUCCESS || ret == WN_MORE_DATA)
        {
            for (iter = resources; count; count--, iter++)
            {
                if (iter->lpLocalName && iter->lpLocalName[0])
                    connection = iter->lpLocalName;
                else
                    connection = iter->lpRemoteName;

                WNetCancelConnection2W(connection, 0, TRUE);
            }
        }
        else
            break;
    }

    HeapFree(GetProcessHeap(), 0, resources);
    WNetCloseEnum(connected);

    return ret;
}


/*
 * Other Functions
 */

/**************************************************************************
 * WNetGetUserA [MPR.@]
 *
 * FIXME: we should not return ourselves, but the owner of the drive lpName
 */
DWORD WINAPI WNetGetUserA( LPCSTR lpName, LPSTR lpUserID, LPDWORD lpBufferSize )
{
    if (GetUserNameA( lpUserID, lpBufferSize )) return WN_SUCCESS;
    return GetLastError();
}

/*****************************************************************
 * WNetGetUserW [MPR.@]
 *
 * FIXME: we should not return ourselves, but the owner of the drive lpName
 */
DWORD WINAPI WNetGetUserW( LPCWSTR lpName, LPWSTR lpUserID, LPDWORD lpBufferSize )
{
    if (GetUserNameW( lpUserID, lpBufferSize )) return WN_SUCCESS;
    return GetLastError();
}

/*********************************************************************
 * WNetConnectionDialog [MPR.@]
 */
DWORD WINAPI WNetConnectionDialog( HWND hwnd, DWORD dwType )
{
    CONNECTDLGSTRUCTW conn_dlg;
    NETRESOURCEW net_res;

    ZeroMemory(&conn_dlg, sizeof(conn_dlg));
    ZeroMemory(&net_res, sizeof(net_res));

    conn_dlg.cbStructure = sizeof(conn_dlg);
    conn_dlg.lpConnRes = &net_res;
    conn_dlg.hwndOwner = hwnd;
    net_res.dwType = dwType;

    return WNetConnectionDialog1W(&conn_dlg);
}

/*********************************************************************
 * WNetConnectionDialog1A [MPR.@]
 */
DWORD WINAPI WNetConnectionDialog1A( LPCONNECTDLGSTRUCTA lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog1W [MPR.@]
 */
DWORD WINAPI WNetConnectionDialog1W( LPCONNECTDLGSTRUCTW lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog [MPR.@]
 */
DWORD WINAPI WNetDisconnectDialog( HWND hwnd, DWORD dwType )
{
    FIXME( "(%p, %08X): stub\n", hwnd, dwType );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog1A [MPR.@]
 */
DWORD WINAPI WNetDisconnectDialog1A( LPDISCDLGSTRUCTA lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog1W [MPR.@]
 */
DWORD WINAPI WNetDisconnectDialog1W( LPDISCDLGSTRUCTW lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetLastErrorA [MPR.@]
 */
DWORD WINAPI WNetGetLastErrorA( LPDWORD lpError,
                                LPSTR lpErrorBuf, DWORD nErrorBufSize,
                                LPSTR lpNameBuf, DWORD nNameBufSize )
{
    FIXME( "(%p, %p, %d, %p, %d): stub\n",
           lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetLastErrorW [MPR.@]
 */
DWORD WINAPI WNetGetLastErrorW( LPDWORD lpError,
                                LPWSTR lpErrorBuf, DWORD nErrorBufSize,
                         LPWSTR lpNameBuf, DWORD nNameBufSize )
{
    FIXME( "(%p, %p, %d, %p, %d): stub\n",
           lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetNetworkInformationA [MPR.@]
 */
DWORD WINAPI WNetGetNetworkInformationA( LPCSTR lpProvider,
                                         LPNETINFOSTRUCT lpNetInfoStruct )
{
    DWORD ret;

    TRACE( "(%s, %p)\n", debugstr_a(lpProvider), lpNetInfoStruct );

    if (!lpProvider)
        ret = WN_BAD_POINTER;
    else
    {
        int len;

        len = MultiByteToWideChar(CP_ACP, 0, lpProvider, -1, NULL, 0);
        if (len)
        {
            LPWSTR wideProvider = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

            if (wideProvider)
            {
                MultiByteToWideChar(CP_ACP, 0, lpProvider, -1, wideProvider,
                 len);
                ret = WNetGetNetworkInformationW(wideProvider, lpNetInfoStruct);
                HeapFree(GetProcessHeap(), 0, wideProvider);
            }
            else
                ret = WN_OUT_OF_MEMORY;
        }
        else
            ret = GetLastError();
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*********************************************************************
 * WNetGetNetworkInformationW [MPR.@]
 */
DWORD WINAPI WNetGetNetworkInformationW( LPCWSTR lpProvider,
                                         LPNETINFOSTRUCT lpNetInfoStruct )
{
    DWORD ret;

    TRACE( "(%s, %p)\n", debugstr_w(lpProvider), lpNetInfoStruct );

    if (!lpProvider)
        ret = WN_BAD_POINTER;
    else if (!lpNetInfoStruct)
        ret = WN_BAD_POINTER;
    else if (lpNetInfoStruct->cbStructure < sizeof(NETINFOSTRUCT))
        ret = WN_BAD_VALUE;
    else
    {
        if (providerTable && providerTable->numProviders)
        {
            DWORD providerIndex = _findProviderIndexW(lpProvider);

            if (providerIndex != BAD_PROVIDER_INDEX)
            {
                lpNetInfoStruct->cbStructure = sizeof(NETINFOSTRUCT);
                lpNetInfoStruct->dwProviderVersion =
                 providerTable->table[providerIndex].dwSpecVersion;
                lpNetInfoStruct->dwStatus = NO_ERROR;
                lpNetInfoStruct->dwCharacteristics = 0;
                lpNetInfoStruct->dwHandle = 0;
                lpNetInfoStruct->wNetType =
                 HIWORD(providerTable->table[providerIndex].dwNetType);
                lpNetInfoStruct->dwPrinters = -1;
                lpNetInfoStruct->dwDrives = -1;
                ret = WN_SUCCESS;
            }
            else
                ret = WN_BAD_PROVIDER;
        }
        else
            ret = WN_NO_NETWORK;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*****************************************************************
 *  WNetGetProviderNameA [MPR.@]
 */
DWORD WINAPI WNetGetProviderNameA( DWORD dwNetType,
                                   LPSTR lpProvider, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE("(0x%08x, %s, %p)\n", dwNetType, debugstr_a(lpProvider),
     lpBufferSize);

    if (!lpProvider)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else
    {
        if (providerTable)
        {
            DWORD i;

            ret = WN_NO_NETWORK;
            for (i = 0; i < providerTable->numProviders &&
             HIWORD(providerTable->table[i].dwNetType) != HIWORD(dwNetType);
             i++)
                ;
            if (i < providerTable->numProviders)
            {
                DWORD sizeNeeded = WideCharToMultiByte(CP_ACP, 0,
                 providerTable->table[i].name, -1, NULL, 0, NULL, NULL);

                if (*lpBufferSize < sizeNeeded)
                {
                    *lpBufferSize = sizeNeeded;
                    ret = WN_MORE_DATA;
                }
                else
                {
                    WideCharToMultiByte(CP_ACP, 0, providerTable->table[i].name,
                     -1, lpProvider, *lpBufferSize, NULL, NULL);
                    ret = WN_SUCCESS;
                    /* FIXME: is *lpBufferSize set to the number of characters
                     * copied? */
                }
            }
        }
        else
            ret = WN_NO_NETWORK;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*****************************************************************
 *  WNetGetProviderNameW [MPR.@]
 */
DWORD WINAPI WNetGetProviderNameW( DWORD dwNetType,
                                   LPWSTR lpProvider, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE("(0x%08x, %s, %p)\n", dwNetType, debugstr_w(lpProvider),
     lpBufferSize);

    if (!lpProvider)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else
    {
        if (providerTable)
        {
            DWORD i;

            ret = WN_NO_NETWORK;
            for (i = 0; i < providerTable->numProviders &&
             HIWORD(providerTable->table[i].dwNetType) != HIWORD(dwNetType);
             i++)
                ;
            if (i < providerTable->numProviders)
            {
                DWORD sizeNeeded = strlenW(providerTable->table[i].name) + 1;

                if (*lpBufferSize < sizeNeeded)
                {
                    *lpBufferSize = sizeNeeded;
                    ret = WN_MORE_DATA;
                }
                else
                {
                    strcpyW(lpProvider, providerTable->table[i].name);
                    ret = WN_SUCCESS;
                    /* FIXME: is *lpBufferSize set to the number of characters
                     * copied? */
                }
            }
        }
        else
            ret = WN_NO_NETWORK;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %d\n", ret);
    return ret;
}
