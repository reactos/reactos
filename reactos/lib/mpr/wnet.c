/*
 * MPR WNet functions
 *
 * Copyright 1999 Ulrich Weigand
 * Copyright 2004 Juan Lang
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
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winnetwk.h"
#include "npapi.h"
#include "winreg.h"
#include "winuser.h"
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
} WNetProvider, *PWNetProvider;

typedef struct _WNetProviderTable
{
    LPWSTR           entireNetwork;
    DWORD            numAllocated;
    DWORD            numProviders;
    WNetProvider     table[1];
} WNetProviderTable, *PWNetProviderTable;

#define WNET_ENUMERATOR_TYPE_NULL     0
#define WNET_ENUMERATOR_TYPE_GLOBAL   1
#define WNET_ENUMERATOR_TYPE_PROVIDER 2
#define WNET_ENUMERATOR_TYPE_CONTEXT  3

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
typedef struct _WNetEnumerator
{
    DWORD          enumType;
    DWORD          providerIndex;
    HANDLE         handle;
    BOOL           providerDone;
    DWORD          dwScope;
    DWORD          dwType;
    DWORD          dwUsage;
    LPNETRESOURCEW lpNet;
} WNetEnumerator, *PWNetEnumerator;

#define BAD_PROVIDER_INDEX (DWORD)0xffffffff

/* Returns an index (into the global WNetProviderTable) of the provider with
 * the given name, or BAD_PROVIDER_INDEX if not found.
 */
static DWORD _findProviderIndexW(LPCWSTR lpProvider);

PWNetProviderTable providerTable;

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
         (LPBYTE)providerPath, &size) == ERROR_SUCCESS && type == REG_SZ)
        {
            static const WCHAR szProviderName[] = { 'N','a','m','e',0 };
            PWSTR name = NULL;
           
            size = 0;
            RegQueryValueExW(hKey, szProviderName, NULL, NULL, NULL, &size);
            if (size)
            {
                name = (PWSTR)HeapAlloc(GetProcessHeap(), 0, size);
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
                    PF_NPGetCaps getCaps = (PF_NPGetCaps)GetProcAddress(hLib,
                     "NPGetCaps");

                    TRACE("loaded lib %p\n", hLib);
                    if (getCaps)
                    {
                        PWNetProvider provider =
                         &providerTable->table[providerTable->numProviders];

                        provider->hLib = hLib;
                        provider->name = name;
                        TRACE("name is %s\n", debugstr_w(name));
                        provider->getCaps = getCaps;
                        provider->dwSpecVersion = getCaps(WNNC_SPEC_VERSION);
                        provider->dwNetType = getCaps(WNNC_NET_TYPE);
                        TRACE("net type is 0x%08lx\n", provider->dwNetType);
                        provider->dwEnumScopes = getCaps(WNNC_ENUMERATION);
                        if (provider->dwEnumScopes)
                        {
                            TRACE("supports enumeration\n");
                            provider->openEnum = (PF_NPOpenEnum)
                             GetProcAddress(hLib, "NPOpenEnum");
                            TRACE("openEnum is %p\n", provider->openEnum);
                            provider->enumResource = (PF_NPEnumResource)
                             GetProcAddress(hLib, "NPEnumResource");
                            TRACE("enumResource is %p\n",
                             provider->enumResource);
                            provider->closeEnum = (PF_NPCloseEnum)
                             GetProcAddress(hLib, "NPCloseEnum");
                            TRACE("closeEnum is %p\n", provider->closeEnum);
                            if (!provider->openEnum || !provider->enumResource
                             || !provider->closeEnum)
                            {
                                provider->openEnum = NULL;
                                provider->enumResource = NULL;
                                provider->closeEnum = NULL;
                                provider->dwEnumScopes = 0;
                                WARN("Couldn't load enumeration functions\n");
                            }
                        }
                        providerTable->numProviders++;
                    }
                    else
                    {
                        WARN("Provider %s didn't export NPGetCaps\n",
                         debugstr_w(provider));
                        if (name)
                            HeapFree(GetProcessHeap(), 0, name);
                        FreeLibrary(hLib);
                    }
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
            PWSTR providers = (PWSTR)HeapAlloc(GetProcessHeap(), 0, size);

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
                        if (ptr)
                            numToAllocate++;
                    }
                    providerTable = (PWNetProviderTable)
                     HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(WNetProviderTable)
                     + (numToAllocate - 1) * sizeof(WNetProvider));
                    if (providerTable)
                    {
                        PWSTR ptrPrev;
                        int entireNetworkLen;

                        entireNetworkLen = LoadStringW(hInstDll,
                         IDS_ENTIRENETWORK, NULL, 0);
                        providerTable->entireNetwork = (LPWSTR)HeapAlloc(
                         GetProcessHeap(), 0, (entireNetworkLen + 1) *
                         sizeof(WCHAR));
                        if (providerTable->entireNetwork)
                            LoadStringW(hInstDll, IDS_ENTIRENETWORK,
                             providerTable->entireNetwork,
                             entireNetworkLen + 1);
                        providerTable->numAllocated = numToAllocate;
                        for (ptr = providers; ptr; )
                        {
                            ptrPrev = ptr;
                            ptr = strchrW(ptr, ',');
                            if (ptr)
                                *ptr = '\0';
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
        if (providerTable->entireNetwork)
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
        ret = (LPNETRESOURCEW)HeapAlloc(GetProcessHeap(), 0,
         sizeof(NETRESOURCEW));
        if (ret)
        {
            size_t len;

            memcpy(ret, lpNet, sizeof(ret));
            ret->lpLocalName = ret->lpComment = ret->lpProvider = NULL;
            if (lpNet->lpRemoteName)
            {
                len = strlenW(lpNet->lpRemoteName) + 1;
                ret->lpRemoteName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                 len * sizeof(WCHAR));
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
        if (lpNet->lpRemoteName)
            HeapFree(GetProcessHeap(), 0, lpNet->lpRemoteName);
        HeapFree(GetProcessHeap(), 0, lpNet);
    }
}

static PWNetEnumerator _createNullEnumerator(void)
{
    PWNetEnumerator ret = (PWNetEnumerator)HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));

    if (ret)
        ret->enumType = WNET_ENUMERATOR_TYPE_NULL;
    return ret;
}

static PWNetEnumerator _createGlobalEnumeratorW(DWORD dwScope, DWORD dwType,
 DWORD dwUsage, LPNETRESOURCEW lpNet)
{
    PWNetEnumerator ret = (PWNetEnumerator)HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));

    if (ret)
    {
        ret->enumType = WNET_ENUMERATOR_TYPE_GLOBAL;
        ret->dwScope = dwScope;
        ret->dwType  = dwType;
        ret->dwUsage = dwUsage;
        ret->lpNet   = _copyNetResourceForEnumW(lpNet);
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
        ret = (PWNetEnumerator)HeapAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, sizeof(WNetEnumerator));
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
    PWNetEnumerator ret = (PWNetEnumerator)HeapAlloc(GetProcessHeap(),
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

/* Thunks the array of wide-string LPNETRESOURCEs lpNetArrayIn into buffer
 * lpBuffer, with size *lpBufferSize.  lpNetArrayIn contains *lpcCount entries
 * to start.  On return, *lpcCount reflects the number thunked into lpBuffer.
 * Returns WN_SUCCESS on success (all of lpNetArrayIn thunked), WN_MORE_DATA
 * if not all members of the array could be thunked, and something else on
 * failure.
 */
static DWORD _thunkNetResourceArrayWToA(const LPNETRESOURCEW lpNetArrayIn,
 LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
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
        LPNETRESOURCEW lpNet = lpNetArrayIn + i;

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
        LPNETRESOURCEW lpNetIn = lpNetArrayIn + i;

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
    TRACE("numToThunk is %ld, *lpcCount is %ld, returning %ld\n", numToThunk,
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
static DWORD _thunkNetResourceArrayAToW(const LPNETRESOURCEA lpNetArrayIn,
 LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
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
        LPNETRESOURCEA lpNet = lpNetArrayIn + i;

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
        LPNETRESOURCEA lpNetIn = lpNetArrayIn + i;

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
    TRACE("numToThunk is %ld, *lpcCount is %ld, returning %ld\n", numToThunk,
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

    TRACE( "(%08lX, %08lX, %08lX, %p, %p)\n",
	    dwScope, dwType, dwUsage, lpNet, lphEnum );

    if (!lphEnum)
        ret = WN_BAD_POINTER;
    else if (!providerTable || providerTable->numProviders == 0)
        ret = WN_NO_NETWORK;
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
                lpNetWide = (LPNETRESOURCEW)HeapAlloc(GetProcessHeap(), 0,
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
            if (allocated && lpNetWide)
                HeapFree(GetProcessHeap(), 0, lpNetWide);
        }
        else
            ret = WNetOpenEnumW(dwScope, dwType, dwUsage, NULL, lphEnum);
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %ld\n", ret);
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

    TRACE( "(%08lX, %08lX, %08lX, %p, %p)\n",
          dwScope, dwType, dwUsage, lpNet, lphEnum );

    if (!lphEnum)
        ret = WN_BAD_POINTER;
    else if (!providerTable || providerTable->numProviders == 0)
        ret = WN_NO_NETWORK;
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
                             providerTable->table[index].dwEnumScopes & dwScope)
                            {
                                HANDLE handle;

                                ret = providerTable->table[index].openEnum(
                                 dwScope, dwType, dwUsage, lpNet, &handle);
                                if (ret == WN_SUCCESS)
                                {
                                    *lphEnum =
                                     (HANDLE)_createProviderEnumerator(
                                     dwScope, dwType, dwUsage, index, handle);
                                    ret = *lphEnum ? WN_SUCCESS :
                                     WN_OUT_OF_MEMORY;
                                }
                            }
                            else
                                ret = WN_NOT_SUPPORTED;
                        }
                        else
                            ret = WN_BAD_PROVIDER;
                    }
                    else if (lpNet->lpRemoteName)
                    {
                        *lphEnum = (HANDLE)_createGlobalEnumeratorW(dwScope,
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
                            *lphEnum = (HANDLE)_createGlobalEnumeratorW(dwScope,
                             dwType, dwUsage, lpNet);
                        }
                        else
                        {
                            /* this is the same as not having passed lpNet */
                            *lphEnum = (HANDLE)_createGlobalEnumeratorW(dwScope,
                             dwType, dwUsage, NULL);
                        }
                        ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    *lphEnum = (HANDLE)_createGlobalEnumeratorW(dwScope, dwType,
                     dwUsage, lpNet);
                    ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                }
                break;
            case RESOURCE_CONTEXT:
                *lphEnum = (HANDLE)_createContextEnumerator(dwScope, dwType,
                 dwUsage);
                ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                break;
            case RESOURCE_REMEMBERED:
            case RESOURCE_CONNECTED:
                *lphEnum = (HANDLE)_createNullEnumerator();
                ret = *lphEnum ? WN_SUCCESS : WN_OUT_OF_MEMORY;
                break;
            default:
                WARN("unknown scope 0x%08lx\n", dwScope);
                ret = WN_BAD_VALUE;
        }
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %ld\n", ret);
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
    if (!lpBuffer)
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
                ret = _thunkNetResourceArrayWToA((LPNETRESOURCEW)localBuffer,
                 &localCount, lpBuffer, lpBufferSize);
                *lpcCount = localCount;
            }
            HeapFree(GetProcessHeap(), 0, localBuffer);
        }
        else
            ret = WN_OUT_OF_MEMORY;
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %ld\n", ret);
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
        for (i = 0, resource = (LPNETRESOURCEW)lpBuffer; i < count;
         i++, resource++)
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
    TRACE("Returning %ld\n", ret);
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
        enumerator->providerDone = FALSE;
        if (enumerator->handle)
        {
            providerTable->table[enumerator->providerIndex].closeEnum(
             enumerator->handle);
            enumerator->handle = NULL;
            enumerator->providerIndex++;
        }
        for (; enumerator->providerIndex < providerTable->numProviders &&
         !(enumerator->dwScope & providerTable->table
         [enumerator->providerIndex].dwEnumScopes);
         enumerator->providerIndex++)
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
         enumerator->dwUsage, enumerator->lpNet,
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
    TRACE("Returning %ld\n", ret);
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
            if (enumerator->lpNet)
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
            WARN("unexpected scope 0x%08lx\n", enumerator->dwScope);
            ret = WN_NO_MORE_ENTRIES;
    }
    TRACE("Returning %ld\n", ret);
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
        LPNETRESOURCEW lpNet = (LPNETRESOURCEW)lpBuffer;

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
            lpcCount++;
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
    TRACE("Returning %ld\n", ret);
    return ret;
}

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
            case WNET_ENUMERATOR_TYPE_NULL:
                ret = WN_NO_MORE_ENTRIES;
                break;
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
            default:
                WARN("bogus enumerator type!\n");
                ret = WN_NO_NETWORK;
        }
    }
    if (ret)
        SetLastError(ret);
    TRACE("Returning %ld\n", ret);
    return ret;
}

/*********************************************************************
 * WNetCloseEnum [MPR.@]
 */
DWORD WINAPI WNetCloseEnum( HANDLE hEnum )
{
    DWORD ret;

    TRACE( "(%p)\n", hEnum );

    if (hEnum)
    {
        PWNetEnumerator enumerator = (PWNetEnumerator)hEnum;

        switch (enumerator->enumType)
        {
            case WNET_ENUMERATOR_TYPE_NULL:
                ret = WN_SUCCESS;
                break;
            case WNET_ENUMERATOR_TYPE_GLOBAL:
                if (enumerator->lpNet)
                    _freeEnumNetResource(enumerator->lpNet);
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
    TRACE("Returning %ld\n", ret);
    return ret;
}

/*********************************************************************
 * WNetGetResourceInformationA [MPR.@]
 */
DWORD WINAPI WNetGetResourceInformationA( LPNETRESOURCEA lpNetResource,
                                          LPVOID lpBuffer, LPDWORD cbBuffer,
                                          LPSTR *lplpSystem )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceInformationW [MPR.@]
 */
DWORD WINAPI WNetGetResourceInformationW( LPNETRESOURCEW lpNetResource,
                                          LPVOID lpBuffer, LPDWORD cbBuffer,
                                          LPWSTR *lplpSystem )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
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
    FIXME( "(%s, %p, %s): stub\n",
           debugstr_a(lpRemoteName), lpPassword, debugstr_a(lpLocalName) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetAddConnectionW [MPR.@]
 */
DWORD WINAPI WNetAddConnectionW( LPCWSTR lpRemoteName, LPCWSTR lpPassword,
                                 LPCWSTR lpLocalName )
{
    FIXME( "(%s, %p, %s): stub\n",
           debugstr_w(lpRemoteName), lpPassword, debugstr_w(lpLocalName) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetAddConnection2A [MPR.@]
 */
DWORD WINAPI WNetAddConnection2A( LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %s, 0x%08lX): stub\n",
           lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection2W [MPR.@]
 */
DWORD WINAPI WNetAddConnection2W( LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %s, 0x%08lX): stub\n",
           lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection3A [MPR.@]
 */
DWORD WINAPI WNetAddConnection3A( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection3W [MPR.@]
 */
DWORD WINAPI WNetAddConnection3W( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetUseConnectionA [MPR.@]
 */
DWORD WINAPI WNetUseConnectionA( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                 LPCSTR lpPassword, LPCSTR lpUserID, DWORD dwFlags,
                                 LPSTR lpAccessName, LPDWORD lpBufferSize,
                                 LPDWORD lpResult )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX, %s, %p, %p), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags,
           debugstr_a(lpAccessName), lpBufferSize, lpResult );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetUseConnectionW [MPR.@]
 */
DWORD WINAPI WNetUseConnectionW( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                 LPCWSTR lpPassword, LPCWSTR lpUserID, DWORD dwFlags,
                                 LPWSTR lpAccessName, LPDWORD lpBufferSize,
                                 LPDWORD lpResult )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX, %s, %p, %p), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags,
           debugstr_w(lpAccessName), lpBufferSize, lpResult );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetCancelConnectionA [MPR.@]
 */
DWORD WINAPI WNetCancelConnectionA( LPCSTR lpName, BOOL fForce )
{
    FIXME( "(%s, %d), stub\n", debugstr_a(lpName), fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnectionW [MPR.@]
 */
DWORD WINAPI WNetCancelConnectionW( LPCWSTR lpName, BOOL fForce )
{
    FIXME( "(%s, %d), stub\n", debugstr_w(lpName), fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnection2A [MPR.@]
 */
DWORD WINAPI WNetCancelConnection2A( LPCSTR lpName, DWORD dwFlags, BOOL fForce )
{
    FIXME( "(%s, %08lX, %d), stub\n", debugstr_a(lpName), dwFlags, fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnection2W [MPR.@]
 */
DWORD WINAPI WNetCancelConnection2W( LPCWSTR lpName, DWORD dwFlags, BOOL fForce )
{
    FIXME( "(%s, %08lX, %d), stub\n", debugstr_w(lpName), dwFlags, fForce );

    return WN_SUCCESS;
}

/*****************************************************************
 *  WNetRestoreConnectionA [MPR.@]
 */
DWORD WINAPI WNetRestoreConnectionA( HWND hwndOwner, LPSTR lpszDevice )
{
    FIXME( "(%p, %s), stub\n", hwndOwner, debugstr_a(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetRestoreConnectionW [MPR.@]
 */
DWORD WINAPI WNetRestoreConnectionW( HWND hwndOwner, LPWSTR lpszDevice )
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
    else if (!lpRemoteName)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
        ret = WN_BAD_POINTER;
    else
    {
        int len = MultiByteToWideChar(CP_ACP, 0, lpLocalName, -1, NULL, 0);

        if (len)
        {
            PWSTR wideLocalName = (PWSTR)HeapAlloc(GetProcessHeap(), 0, len);

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
                    PWSTR wideRemote = (PWSTR)HeapAlloc(GetProcessHeap(), 0,
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
    TRACE("Returning %ld\n", ret);
    return ret;
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
    else if (!lpRemoteName)
        ret = WN_BAD_POINTER;
    else if (!lpBufferSize)
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
            {
                WCHAR remote[MAX_PATH];
                if (!QueryDosDeviceW( lpLocalName, remote, MAX_PATH )) remote[0] = 0;
                if (strlenW(remote) + 1 > *lpBufferSize)
                {
                    *lpBufferSize = strlenW(remote) + 1;
                    ret = WN_MORE_DATA;
                }
                else
                {
                    strcpyW( lpRemoteName, remote );
                    *lpBufferSize = strlenW(lpRemoteName) + 1;
                    ret = WN_SUCCESS;
                }
                break;
            }
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
    TRACE("Returning %ld\n", ret);
    return ret;
}

/**************************************************************************
 * WNetSetConnectionA [MPR.@]
 */
DWORD WINAPI WNetSetConnectionA( LPCSTR lpName, DWORD dwProperty,
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08lX, %p): stub\n", debugstr_a(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetSetConnectionW [MPR.@]
 */
DWORD WINAPI WNetSetConnectionW( LPCWSTR lpName, DWORD dwProperty,
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08lX, %p): stub\n", debugstr_w(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameA [MPR.@]
 */
DWORD WINAPI WNetGetUniversalNameA ( LPCSTR lpLocalPath, DWORD dwInfoLevel,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%s, 0x%08lX, %p, %p): stub\n",
           debugstr_a(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameW [MPR.@]
 */
DWORD WINAPI WNetGetUniversalNameW ( LPCWSTR lpLocalPath, DWORD dwInfoLevel,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%s, 0x%08lX, %p, %p): stub\n",
           debugstr_w(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
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
    FIXME( "(%p, %08lX): stub\n", hwnd, dwType );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
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
    FIXME( "(%p, %08lX): stub\n", hwnd, dwType );

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
    FIXME( "(%p, %p, %ld, %p, %ld): stub\n",
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
    FIXME( "(%p, %p, %ld, %p, %ld): stub\n",
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
            LPWSTR wideProvider = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
             len * sizeof(WCHAR));

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
    TRACE("Returning %ld\n", ret);
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
                lpNetInfoStruct->dwHandle = (ULONG_PTR)NULL;
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
    TRACE("Returning %ld\n", ret);
    return ret;
}

/*****************************************************************
 *  WNetGetProviderNameA [MPR.@]
 */
DWORD WINAPI WNetGetProviderNameA( DWORD dwNetType,
                                   LPSTR lpProvider, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE("(0x%08lx, %s, %p)\n", dwNetType, debugstr_a(lpProvider),
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
    TRACE("Returning %ld\n", ret);
    return ret;
}

/*****************************************************************
 *  WNetGetProviderNameW [MPR.@]
 */
DWORD WINAPI WNetGetProviderNameW( DWORD dwNetType,
                                   LPWSTR lpProvider, LPDWORD lpBufferSize )
{
    DWORD ret;

    TRACE("(0x%08lx, %s, %p)\n", dwNetType, debugstr_w(lpProvider),
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
    TRACE("Returning %ld\n", ret);
    return ret;
}
