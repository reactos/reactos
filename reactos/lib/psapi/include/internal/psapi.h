/* $Id: psapi.h,v 1.3 2002/08/31 15:36:55 hyperion Exp $
*/
/*
 * internal/psapi.h
 *
 * Process Status Helper API, native interface
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __INTERNAL_PSAPI_H_INCLUDED__
#define __INTERNAL_PSAPI_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>
#include <ntdll/ldr.h>

/* OBJECTS */

/* TYPES */
typedef NTSTATUS STDCALL (*PPROC_ENUM_ROUTINE)
(
 IN PSYSTEM_PROCESS_INFORMATION CurrentProcess,
 IN OUT PVOID CallbackContext
);

typedef NTSTATUS STDCALL (*PSYSMOD_ENUM_ROUTINE)
(
 IN ULONG ModuleCount,
 IN PSYSTEM_MODULE_ENTRY CurrentModule,
 IN OUT PVOID CallbackContext
);

typedef NTSTATUS STDCALL (*PPROCMOD_ENUM_ROUTINE)
(
 IN HANDLE ProcessHandle,
 IN PLDR_MODULE CurrentModule,
 IN OUT PVOID CallbackContext
);

/* CONSTANTS */
#define FAILED_WITH_STATUS DEFINE_DBG_MSG("%s() failed, status 0x%08X")

/* PROTOTYPES */
NTSTATUS
STDCALL
PsaEnumerateProcesses
(
 IN PPROC_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

NTSTATUS
STDCALL
PsaEnumerateSystemModules
(
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

NTSTATUS
STDCALL
PsaEnumerateProcessModules
(
 IN HANDLE ProcessHandle,
 IN PPROCMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

/* MACROS */
#define DEFINE_DBG_MSG(__str__) "PSAPI: " __str__ "\n"

#endif /* __INTERNAL_PSAPI_H_INCLUDED__ */

/* EOF */

