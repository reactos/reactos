/*
 * PROJECT:     ReactOS Shim helper library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS Shim Engine common functions / structures
 * COPYRIGHT:   Copyright 2016-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


typedef struct tagHOOKAPI
{
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PVOID Reserved[2];
} HOOKAPI, *PHOOKAPI;


PVOID ShimLib_ShimMalloc(SIZE_T dwSize);
VOID ShimLib_ShimFree(PVOID pData);
PCSTR ShimLib_StringDuplicateA(PCSTR szString);
PCSTR ShimLib_StringNDuplicateA(PCSTR szString, SIZE_T stringLength);
BOOL ShimLib_StrAEqualsWNC(PCSTR szString, PCWSTR wszString);
HINSTANCE ShimLib_Instance(VOID);

/* Forward events to generic handlers */
VOID ShimLib_Init(HINSTANCE hInstance);
VOID ShimLib_Deinit(VOID);
PHOOKAPI WINAPI ShimLib_GetHookAPIs(LPCSTR szCommandLine,LPCWSTR wszShimName,PDWORD pdwHookCount);
BOOL WINAPI ShimLib_NotifyShims(DWORD fdwReason, PVOID ptr);


/* Shims should respond to SHIM_REASON_XXXX in the Notify routines.
   SHIM_NOTIFY_ codes are sent by apphelp, and translated to SHIM_REASON_ by the shimlib routines.
   The only exception being SHIM_NOTIFY_ATTACH, that is also set for one-time init.
   */

#define SHIM_REASON_INIT                    100
#define SHIM_REASON_DEINIT                  101
#define SHIM_REASON_DLL_LOAD                102   /* Arg: PLDR_DATA_TABLE_ENTRY */
#define SHIM_REASON_DLL_UNLOAD              103   /* Arg: PLDR_DATA_TABLE_ENTRY */

#define SHIM_NOTIFY_ATTACH                  1
#define SHIM_NOTIFY_DETACH                  2
#define SHIM_NOTIFY_DLL_LOAD                3   /* Arg: PLDR_DATA_TABLE_ENTRY */
#define SHIM_NOTIFY_DLL_UNLOAD              4   /* Arg: PLDR_DATA_TABLE_ENTRY */



typedef enum _SEI_LOG_LEVEL {
    SEI_MSG = 1,
    SEI_FAIL = 2,
    SEI_WARN = 3,
    SEI_INFO = 4,
} SEI_LOG_LEVEL;

BOOL WINAPIV SeiDbgPrint(SEI_LOG_LEVEL Level, PCSTR Function, PCSTR Format, ...);
extern ULONG g_ShimEngDebugLevel;

#if defined(IN_APPHELP)
/* Apphelp shimeng logging uses the function name */
#define SHIMENG_MSG(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_MSG, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIMENG_FAIL(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_FAIL, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIMENG_WARN(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_WARN, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIMENG_INFO(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_INFO, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#else
/* Shims use the shim name */
#define SHIM_MSG(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_MSG, SHIM_OBJ_NAME(g_szModuleName), fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_FAIL(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_FAIL, SHIM_OBJ_NAME(g_szModuleName), fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_WARN(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_WARN, SHIM_OBJ_NAME(g_szModuleName), fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_INFO(fmt, ...)  do { if (g_ShimEngDebugLevel) SeiDbgPrint(SEI_INFO, SHIM_OBJ_NAME(g_szModuleName), fmt, ##__VA_ARGS__ ); } while (0)
#endif

typedef PHOOKAPI (WINAPI* _PVGetHookAPIs)(DWORD, PCSTR, PDWORD);
typedef BOOL (WINAPI* _PVNotify)(DWORD, PVOID);

typedef struct tagSHIMREG
{
    _PVGetHookAPIs GetHookAPIs;
    _PVNotify Notify;
    PCSTR ShimName;
} SHIMREG, *PSHIMREG;


#if defined(_MSC_VER)
#define _SHMALLOC(x) __declspec(allocate(x))
#elif defined(__GNUC__)
#define _SHMALLOC(x) __attribute__ ((section (x) ))
#else
#error Your compiler is not supported.
#endif

#ifdef __cplusplus
} // extern "C"
#endif

