/*
 * Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
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

/* ReactOS specific */

/* Structure that allows dynamic growing.
   Be aware, the data may move! */
typedef struct _ARRAY
{
    PVOID Data__;
    DWORD Size__;
    DWORD MaxSize__;
    DWORD ItemSize__;
} ARRAY, *PARRAY;

typedef struct _SHIMINFO *PSHIMINFO;
typedef struct _SHIMMODULE *PSHIMMODULE;

/* Shims know this structure as HOOKAPI, with 2 reserved members (the last 2). */
typedef struct tagHOOKAPIEX
{
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PSHIMINFO pShimInfo;
    PVOID Unused;
} HOOKAPIEX, *PHOOKAPIEX;

C_ASSERT(sizeof(HOOKAPIEX) == sizeof(HOOKAPI));
C_ASSERT(offsetof(HOOKAPIEX, pShimInfo) == offsetof(HOOKAPI, Reserved));


typedef struct _SHIMINFO
{
    PHOOKAPIEX pHookApi;
    DWORD dwHookCount;
    PSHIMMODULE pShimModule;
} SHIMINFO, *PSHIMINFO;

typedef struct _SHIMMODULE
{
    UNICODE_STRING Name;
    PVOID BaseAddress;

    PHOOKAPIEX (WINAPI* pGetHookAPIs)(LPCSTR szCommandLine, LPCWSTR wszShimName, PDWORD pdwHookCount);
    BOOL (WINAPI* pNotifyShims)(DWORD fdwReason, PVOID ptr);

    ARRAY EnabledShims;     /* PSHIMINFO */
} SHIMMODULE, *PSHIMMODULE;

typedef struct _HOOKMODULEINFO
{
    UNICODE_STRING Name;
    PVOID BaseAddress;

    ARRAY HookApis;         /* PHOOKAPIEX */

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
