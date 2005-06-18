/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/ntdll/inc/ntdll.h
 * PURPOSE:         Native Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <ddk/ntddk.h> /* FIXME: NDK */
#include <ddk/ntifs.h> /* FIXME: NDK */
#include <ntos/ldrtypes.h>  /* FIXME: NDK */

/* NTDLL Public Headers. FIXME: USE NDK */
#include <ntdll/csr.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <ntdll/ntdll.h>

/* ROSRTL Headers */
#include <rosrtl/string.h> /* FIXME: KILL ROSRTL */

/* Helper Macros FIXME: NDK */
#define ROUNDUP(a,b)    ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#define LOWORD(l) ((WORD)(l))
#endif

/* EOF */
