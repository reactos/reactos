/* $Id$
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

#ifndef __EPSAPI_H_INCLUDED__
#define __EPSAPI_H_INCLUDED__

/* TYPES */
typedef NTSTATUS (NTAPI *PPROC_ENUM_ROUTINE)(IN PSYSTEM_PROCESS_INFORMATION CurrentProcess,
                                             IN OUT PVOID CallbackContext);

typedef NTSTATUS (NTAPI *PTHREAD_ENUM_ROUTINE)(IN PSYSTEM_THREAD_INFORMATION CurrentThread,
                                               IN OUT PVOID CallbackContext);

typedef NTSTATUS (NTAPI *PSYSMOD_ENUM_ROUTINE)(IN PRTL_PROCESS_MODULE_INFORMATION CurrentModule,
                                               IN OUT PVOID CallbackContext);

typedef NTSTATUS (NTAPI *PPROCMOD_ENUM_ROUTINE)(IN HANDLE ProcessHandle,
                                                IN PLDR_DATA_TABLE_ENTRY CurrentModule,
                                                IN OUT PVOID CallbackContext);

/* CONSTANTS */
#define FAILED_WITH_STATUS DEFINE_DBG_MSG("%s() failed, status 0x%08X")

/* PROTOTYPES */
/* Processes and threads */
/* enumeration */
NTSTATUS NTAPI
PsaEnumerateProcessesAndThreads(IN PPROC_ENUM_ROUTINE ProcessCallback,
                                IN OUT PVOID ProcessCallbackContext,
                                IN PTHREAD_ENUM_ROUTINE ThreadCallback,
                                IN OUT PVOID ThreadCallbackContext);

NTSTATUS NTAPI
PsaEnumerateProcesses(IN PPROC_ENUM_ROUTINE Callback,
                      IN OUT PVOID CallbackContext);

NTSTATUS NTAPI
PsaEnumerateThreads(IN PTHREAD_ENUM_ROUTINE Callback,
                    IN OUT PVOID CallbackContext);

/* capturing & walking */
NTSTATUS NTAPI
PsaCaptureProcessesAndThreads(OUT PSYSTEM_PROCESS_INFORMATION * ProcessesAndThreads);

NTSTATUS NTAPI
PsaWalkProcessesAndThreads(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads,
                           IN PPROC_ENUM_ROUTINE ProcessCallback,
                           IN OUT PVOID ProcessCallbackContext,
                           IN PTHREAD_ENUM_ROUTINE ThreadCallback,
                           IN OUT PVOID ThreadCallbackContext);

NTSTATUS NTAPI
PsaWalkProcesses(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads,
                 IN PPROC_ENUM_ROUTINE Callback,
                 IN OUT PVOID CallbackContext);

NTSTATUS NTAPI
PsaWalkThreads(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads,
               IN PTHREAD_ENUM_ROUTINE Callback,
               IN OUT PVOID CallbackContext);

PSYSTEM_PROCESS_INFORMATION FASTCALL
PsaWalkFirstProcess(IN PSYSTEM_PROCESS_INFORMATION ProcessesAndThreads);

PSYSTEM_PROCESS_INFORMATION FASTCALL
PsaWalkNextProcess(IN PSYSTEM_PROCESS_INFORMATION CurrentProcess);

PSYSTEM_THREAD_INFORMATION FASTCALL
PsaWalkFirstThread(IN PSYSTEM_PROCESS_INFORMATION CurrentProcess);

PSYSTEM_THREAD_INFORMATION FASTCALL
PsaWalkNextThread(IN PSYSTEM_THREAD_INFORMATION CurrentThread);

/* System modules */
/* enumeration */
NTSTATUS NTAPI
PsaEnumerateSystemModules(IN PSYSMOD_ENUM_ROUTINE Callback,
                          IN OUT PVOID CallbackContext);

/* capturing & walking */
NTSTATUS NTAPI
PsaCaptureSystemModules(OUT PRTL_PROCESS_MODULES * SystemModules);

NTSTATUS NTAPI
PsaWalkSystemModules(IN PRTL_PROCESS_MODULES SystemModules,
                     IN PSYSMOD_ENUM_ROUTINE Callback,
                     IN OUT PVOID CallbackContext);

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkFirstSystemModule(IN PRTL_PROCESS_MODULES SystemModules);

PRTL_PROCESS_MODULE_INFORMATION FASTCALL
PsaWalkNextSystemModule(IN PRTL_PROCESS_MODULES CurrentSystemModule);

/* Process modules */
NTSTATUS NTAPI
PsaEnumerateProcessModules(IN HANDLE ProcessHandle,
                           IN PPROCMOD_ENUM_ROUTINE Callback,
                           IN OUT PVOID CallbackContext);

/* Miscellaneous */
VOID NTAPI
PsaFreeCapture(IN PVOID Capture);

/* The user must define these functions. They are called by PSAPI to allocate
   memory. This allows PSAPI to be called from any environment */
void *PsaiMalloc(SIZE_T size);
void *PsaiRealloc(void *ptr, SIZE_T size);
void PsaiFree(void *ptr);

/* MACROS */
#define DEFINE_DBG_MSG(__str__) "PSAPI: " __str__ "\n"

#endif /* __EPSAPI_H_INCLUDED__ */
