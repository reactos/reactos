/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim Engine
 * FILE:            dll/appcompat/shims/shimlib/shimlib.h
 * PURPOSE:         ReactOS Shim Engine
 * PROGRAMMER:      Mark Jansen
 */

#pragma once

typedef struct tagHOOKAPI {
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PVOID Unk1;
    PVOID Unk2;
} HOOKAPI, *PHOOKAPI;

extern HINSTANCE g_hinstDll;

void ShimLib_Init(HINSTANCE);
void ShimLib_Deinit(void);
PVOID ShimLib_ShimMalloc(SIZE_T);
void ShimLib_ShimFree(PVOID);
PCSTR ShimLib_StringDuplicateA(PCSTR);
PHOOKAPI WINAPI ShimLib_GetHookAPIs(LPCSTR,LPCWSTR,PDWORD);


#define SHIM_REASON_ATTACH      1
#define SHIM_REASON_DETACH      2
#define SHIM_REASON_DLL_LOAD    3   /* Arg: PLDR_DATA_TABLE_ENTRY */
#define SHIM_REASON_DLL_UNLOAD  4   /* Arg: Module Handle */


typedef PVOID (__cdecl *_PVSHIM)(PCWSTR);

#if defined(_MSC_VER)
#define _SHMALLOC(x) __declspec(allocate(x))
#elif defined(__GNUC__)
#define _SHMALLOC(x) __attribute__ ((section (x) ))
#else
#error Your compiler is not supported.
#endif

