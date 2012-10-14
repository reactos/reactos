/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#ifndef RTL_H
#define RTL_H

/* We're a core NT DLL, we don't import syscalls */
#define WIN32_NO_STATUS
#define _INC_SWPRINTF_INL_
#undef __MSVCRT__

/* C Headers */
#include <stdlib.h>
#include <stdio.h>

/* PSDK/NDK Headers */
#include <windows.h>
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kdfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/ldrfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>
#include <ndk/umfuncs.h>

/* Internal RTL header */
#include "rtlp.h"

/* PSEH Support */
#include <pseh/pseh2.h>

#include <intrin.h>

/* Use intrinsics for x86 and x64 */
#if defined(_M_IX86) || defined(_M_AMD64)
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#endif

#endif /* RTL_H */

/* EOF */
