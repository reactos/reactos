/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <windows.h>
#include <ntdll/rtl.h>
#include <napi/teb.h>
#include <ntos/minmax.h>
#include <string.h>
#include <napi/i386/segment.h>
#include <ntdll/ldr.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>
#include <rosrtl/thread.h>
#include <winerror.h>
#include <ntos.h>
#include <stdio.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define ROUNDUP(a,b)    ((((a)+(b)-1)/(b))*(b))
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#define LOWORD(l) ((WORD)(l))
#endif

/* EOF */
