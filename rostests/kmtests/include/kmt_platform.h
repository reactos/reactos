/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite platform declarations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTEST_PLATFORM_H_
#define _KMTEST_PLATFORM_H_

#if !defined _KMTEST_TEST_H_
#error include kmt_test.h instead of including kmt_platform.h!
#endif /* !defined _KMTEST_TEST_H_ */

#include <limits.h>
#include <malloc.h>
#include <stdarg.h>

#if defined KMT_KERNEL_MODE || defined KMT_STANDALONE_DRIVER
#include <ntddk.h>
#include <ntifs.h>
#include <ndk/exfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/sefuncs.h>
#include <ntstrsafe.h>

#elif defined KMT_USER_MODE
#define WIN32_NO_STATUS
#define UNICODE
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>
#include <ndk/cmfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/mmfuncs.h>
#include <strsafe.h>

#ifdef KMT_EMULATE_KERNEL
#define ok_irql(i)
#define KIRQL int
typedef const UCHAR CUCHAR, *PCUCHAR;
typedef ULONG LOGICAL, *PLOGICAL;

#undef KeRaiseIrql
#define KeRaiseIrql(new, old) *(old) = 123
#undef KeLowerIrql
#define KeLowerIrql(i) (void)(i)
#define ExAllocatePool(type, size)              HeapAlloc(GetProcessHeap(), 0, size)
#define ExAllocatePoolWithTag(type, size, tag)  HeapAlloc(GetProcessHeap(), 0, size)
#define ExFreePool(p)                           HeapFree(GetProcessHeap(), 0, p)
#define ExFreePoolWithTag(p, tag)               HeapFree(GetProcessHeap(), 0, p)
#define RtlCopyMemoryNonTemporal                RtlCopyMemory
#define RtlPrefetchMemoryNonTemporal(s, l)
#define ExRaiseStatus                           RtlRaiseStatus
#define KmtIsCheckedBuild                       FALSE
#endif /* defined KMT_EMULATE_KERNEL */

#endif /* defined KMT_USER_MODE */

#include <pseh/pseh2.h>

#endif /* !defined _KMTEST_PLATFORM_H_ */
