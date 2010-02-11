#pragma once

// #pragma warning(disable:4146)

#define DBG 1

#define _NTSYSTEM_
#define _NTOSKRNL_
#define _NTDLLBUILD_
#define NTKERNELAPI

#define NO_RTL_INLINES
#define USE_COMPILER_EXCEPTIONS
#define _CRT_SECURE_NO_WARNINGS
#define _LIB
#define _SEH_ENABLE_TRACE
#define _USE_32BIT_TIME_T

/* We're a core NT DLL, we don't import syscalls */
#define WIN32_NO_STATUS
#define _INC_SWPRINTF_INL_
#undef __MSVCRT__

// #define _WIN32_WINNT 0x502
// #define _WIN32_WINDOWS 0x502
// #define _WIN32_IE 0x600
#define _SETUPAPI_VER 0x502

#include <reactos_cfg.h>
// #include <msc.h>

/* PSDK/NDK Headers */
#include <windows.h>
#include <ntndk.h>
#include <arch/ketypes.h>

/* C Headers */
#include <stdlib.h>
#include <stdio.h>

/* Internal RTL header */
#include "rtlp.h"

/* PSEH Support */
#include <pseh/pseh2.h>

#include <intrin.h>

#include <wchar.h>

_INTRINSIC(_BitScanReverse)
_INTRINSIC(_BitScanForward)

#include <dbg.h>
