/* $Id: epsapi.h,v 1.2 2003/04/13 03:57:41 hyperion Exp $
*/
/*
 * epsapi.h
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
typedef NTSTATUS NTAPI (*PPROC_ENUM_ROUTINE)
(
 IN PSYSTEM_PROCESSES CurrentProcess,
 IN OUT PVOID CallbackContext
);

typedef NTSTATUS NTAPI (*PTHREAD_ENUM_ROUTINE)
(
 IN PSYSTEM_THREADS CurrentThread,
 IN OUT PVOID CallbackContext
);

typedef NTSTATUS NTAPI (*PSYSMOD_ENUM_ROUTINE)
(
 IN PSYSTEM_MODULE_INFORMATION CurrentModule,
 IN OUT PVOID CallbackContext
);

typedef NTSTATUS NTAPI (*PPROCMOD_ENUM_ROUTINE)
(
 IN HANDLE ProcessHandle,
 IN PLDR_MODULE CurrentModule,
 IN OUT PVOID CallbackContext
);

/* CONSTANTS */
#define FAILED_WITH_STATUS DEFINE_DBG_MSG("%s() failed, status 0x%08X")

/* PROTOTYPES */
/* Processes and threads */
/* enumeration */
NTSTATUS
NTAPI
PsaEnumerateProcessesAndThreads
(
 IN PPROC_ENUM_ROUTINE ProcessCallback,
 IN OUT PVOID ProcessCallbackContext,
 IN PTHREAD_ENUM_ROUTINE ThreadCallback,
 IN OUT PVOID ThreadCallbackContext
);

NTSTATUS
NTAPI
PsaEnumerateProcesses
(
 IN PPROC_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

NTSTATUS
NTAPI
PsaEnumerateThreads
(
 IN PTHREAD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

/* capturing & walking */
NTSTATUS
NTAPI
PsaCaptureProcessesAndThreads
(
 OUT PSYSTEM_PROCESSES * ProcessesAndThreads
);

NTSTATUS
NTAPI
PsaWalkProcessesAndThreads
(
 IN PSYSTEM_PROCESSES ProcessesAndThreads,
 IN PPROC_ENUM_ROUTINE ProcessCallback,
 IN OUT PVOID ProcessCallbackContext,
 IN PTHREAD_ENUM_ROUTINE ThreadCallback,
 IN OUT PVOID ThreadCallbackContext
);

NTSTATUS
NTAPI
PsaWalkProcesses
(
 IN PSYSTEM_PROCESSES ProcessesAndThreads,
 IN PPROC_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

NTSTATUS
NTAPI
PsaWalkThreads
(
 IN PSYSTEM_PROCESSES ProcessesAndThreads,
 IN PTHREAD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

PSYSTEM_PROCESSES
FASTCALL
PsaWalkFirstProcess
(
 IN PSYSTEM_PROCESSES ProcessesAndThreads
);

PSYSTEM_PROCESSES
FASTCALL
PsaWalkNextProcess
(
 IN PSYSTEM_PROCESSES CurrentProcess
);

PSYSTEM_THREADS
FASTCALL
PsaWalkFirstThread
(
 IN PSYSTEM_PROCESSES CurrentProcess
);

PSYSTEM_THREADS
FASTCALL
PsaWalkNextThread
(
 IN PSYSTEM_THREADS CurrentThread
);

/* System modules */
/* enumeration */
NTSTATUS
NTAPI
PsaEnumerateSystemModules
(
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

/* capturing & walking */
NTSTATUS
NTAPI
PsaCaptureSystemModules
(
 OUT PSYSTEM_MODULES * SystemModules
);

NTSTATUS
NTAPI
PsaWalkSystemModules
(
 IN PSYSTEM_MODULES SystemModules,
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

PSYSTEM_MODULE_INFORMATION
FASTCALL
PsaWalkFirstSystemModule
(
 IN PSYSTEM_MODULES SystemModules
);

PSYSTEM_MODULE_INFORMATION
FASTCALL
PsaWalkNextSystemModule
(
 IN PSYSTEM_MODULE_INFORMATION CurrentSystemModule
);

/* Process modules */
NTSTATUS
NTAPI
PsaEnumerateProcessModules
(
 IN HANDLE ProcessHandle,
 IN PPROCMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

/* Miscellaneous */
VOID
NTAPI
PsaFreeCapture
(
 IN PVOID Capture
);

/* The user must define these functions. They are called by PSAPI to allocate 
   memory. This allows PSAPI to be called from any environment */
void *PsaiMalloc(SIZE_T size);
void *PsaiRealloc(void *ptr, SIZE_T size);
void PsaiFree(void *ptr);

/* MACROS */
#define DEFINE_DBG_MSG(__str__) "PSAPI: " __str__ "\n"

#endif /* __INTERNAL_PSAPI_H_INCLUDED__ */

/* EOF */
