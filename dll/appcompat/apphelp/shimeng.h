/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim engine structures
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef SHIMENG_H
#define SHIMENG_H

/* This header is ReactOS specific */

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

typedef struct tagHOOKAPIEX *PHOOKAPIEX;

/* Shims know this structure as HOOKAPI, with 2 reserved members (the last 2). */
typedef struct tagHOOKAPIEX
{
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PSHIMINFO pShimInfo;
    PHOOKAPIEX ApiLink;
} HOOKAPIEX;

C_ASSERT(sizeof(HOOKAPIEX) == sizeof(HOOKAPI));
C_ASSERT(offsetof(HOOKAPIEX, pShimInfo) == offsetof(HOOKAPI, Reserved));

typedef struct _INEXCLUDE
{
    UNICODE_STRING Module;
    BOOL Include;
} INEXCLUDE, *PINEXCLUDE;

typedef struct _SHIMINFO
{
    PCWSTR ShimName;
    PHOOKAPIEX pHookApi;
    DWORD dwHookCount;
    PSHIMMODULE pShimModule;
    ARRAY InExclude;        /* INEXCLUDE */
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

typedef struct _FLAGINFO
{
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    ULONG ProcessParameters_Flags;
} FLAGINFO, *PFLAGINFO;


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
