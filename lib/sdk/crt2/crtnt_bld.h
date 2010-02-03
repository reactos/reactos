#pragma once

#define DBG 1

#define _NTSYSTEM_
#define _NTOSKRNL_
#define _NTDLLBUILD_
#define _LIB
#define _LIBCNT_

#define WIN32_NO_STATUS
#define NO_RTL_INLINES
#define USE_COMPILER_EXCEPTIONS
#define _CRT_SECURE_NO_WARNINGS
#define _SEH_ENABLE_TRACE
#define _USE_32BIT_TIME_T

#define _SETUPAPI_VER 0x502

#define __MINGW_IMPORT
#define _CRTIMP

#include <reactos_cfg.h>
#include <cpu.h>

// #include <msc.h>

// win
#include <windows.h>

// ndk
#include <ntndk.h>

// crt
// #include <errno.h>

#define TRACE DPRINT
