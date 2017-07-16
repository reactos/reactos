/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim Engine
 * FILE:            dll/appcompat/shims/shimlib/shimlib.h
 * PURPOSE:         ReactOS Shim Engine
 * PROGRAMMER:      Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

typedef struct tagHOOKAPI
{
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PVOID Reserved[2];
} HOOKAPI, *PHOOKAPI;


PVOID ShimLib_ShimMalloc(SIZE_T);
void ShimLib_ShimFree(PVOID);
PCSTR ShimLib_StringDuplicateA(PCSTR);
BOOL ShimLib_StrAEqualsW(PCSTR, PCWSTR);


/* Forward events to generic handlers */
void ShimLib_Init(HINSTANCE);
void ShimLib_Deinit(void);
PHOOKAPI WINAPI ShimLib_GetHookAPIs(LPCSTR,LPCWSTR,PDWORD);
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

