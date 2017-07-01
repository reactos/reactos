/*
 * Copyright 2017 Mark Jansen
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

#ifndef SHIMENG_H
#define SHIMENG_H

/* Structure that allows dynamic growing */
typedef struct _ARRAY
{
    PVOID Data;
    DWORD Size;
    DWORD MaxSize;
} ARRAY, *PARRAY;


/* Shims do not know it, but actually they use the type HOOKAPIEX.
   We use the 'Reserved' entry for our own purposes. */

typedef struct tagHOOKAPIEX
{
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    SINGLE_LIST_ENTRY ModuleLink;
    SINGLE_LIST_ENTRY ApiLink;
} HOOKAPIEX, *PHOOKAPIEX;

C_ASSERT(sizeof(HOOKAPIEX) == sizeof(HOOKAPI));
C_ASSERT(offsetof(HOOKAPIEX, ModuleLink) == offsetof(HOOKAPI, Reserved));


typedef struct _HOOKAPIPOINTERS
{
    PHOOKAPIEX pHookApi;
    DWORD dwHookCount;
} HOOKAPIPOINTERS, *PHOOKAPIPOINTERS;

typedef struct _SHIMMODULE
{
    UNICODE_STRING Name;
    PVOID BaseAddress;

    PHOOKAPIEX (WINAPI* pGetHookAPIs)(LPCSTR szCommandLine, LPCWSTR wszShimName, PDWORD pdwHookCount);
    BOOL (WINAPI* pNotifyShims)(DWORD fdwReason, PVOID ptr);

    ARRAY HookApis;     /* HOOKAPIPOINTERS */
} SHIMMODULE, *PSHIMMODULE;

typedef struct _HOOKMODULEINFO
{
    UNICODE_STRING Name;
    PVOID BaseAddress;

    SINGLE_LIST_ENTRY ModuleLink;   /* Normal link, all entries from this module */
    SINGLE_LIST_ENTRY ApiLink;      /* Multiple hooks on one api, unsupported for now */
} HOOKMODULEINFO, *PHOOKMODULEINFO;


#if SDBAPI_DEBUG_ALLOC

LPVOID SdbpAlloc(SIZE_T size, int line, const char* file);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, SIZE_T oldSize, int line, const char* file);
VOID SdbpFree(LPVOID mem, int line, const char* file);

#define SeiAlloc(size) SdbpAlloc(size, __LINE__, __FILE__)
#define SeiReAlloc(mem, size, oldSize) SdbpReAlloc(mem, size, oldSize, __LINE__, __FILE__)
#define SeiFree(mem) SdbpFree(mem, __LINE__, __FILE__)

#else

LPVOID SdbpAlloc(SIZE_T size);
LPVOID SdbpReAlloc(LPVOID mem, SIZE_T size, SIZE_T oldSize);
VOID SdbpFree(LPVOID mem);

#define SeiAlloc(size) SdbpAlloc(size)
#define SeiReAlloc(mem, size, oldSize) SdbpReAlloc(mem, size, oldSize)
#define SeiFree(mem) SdbpFree(mem)

#endif

#endif // SHIMENG_H
