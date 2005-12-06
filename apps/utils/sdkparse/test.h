/* $Id$
*/
/*
 * test.h
 *
 * This file is a combination of a couple different headers
 * from ReactOS's include/ folder, and a little bit of custom
 * hacking as well for the purpose of testing sdkparse.
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

void  *_lfind(const void* match, const void* start,unsigned int* array_size, unsigned int elem_size,int (*cf)(const void*,const void*));

static inline struct _TEB * NtCurrentTeb(void)
{
 struct _TEB * pTeb;

 /* FIXME: instead of hardcoded offsets, use offsetof() - if possible */
 __asm__ __volatile__
 (
  "movl %%fs:0x18, %0\n" /* fs:18h == Teb->Tib.Self */
  : "=r" (pTeb) /* can't have two memory operands */
  : /* no inputs */
 );

 return pTeb;
}

typedef struct tagFOO
{
	INT lonibble : 4;
	INT hinibble : 4;
} FOO, *PFOO, FAR *LPFOO;

/* INCLUDES */
#define NTOS_MODE_USER
#include <ntos.h>

/* OBJECTS */
typedef struct
{
	LPSTR LeftVolumeName;
	LPSTR RightVolumeName;
	ULONG DefaultVolume;
	ULONG Type;
	ULONG DeviceType;
	char Key[4];
	LPSTR PrototypeName;
	PVOID DeferredRoutine;
	PVOID ExclusionRoutine;
	PVOID DispatchRoutine;
	PVOID DevCapsRoutine;
	PVOID HwSetVolume;
	ULONG IoMethod;
}SOUND_DEVICE_INIT;

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
 IN PSYSTEM_MODULE_INFORMATION_ENTRY CurrentModule,
 IN OUT PVOID CallbackContext
);

typedef NTSTATUS NTAPI (*PPROCMOD_ENUM_ROUTINE)
(
 IN HANDLE ProcessHandle,
 IN PLDR_DATA_TABLE_ENTRY CurrentModule,
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
 OUT PSYSTEM_MODULE_INFORMATION * SystemModules
);

NTSTATUS
NTAPI
PsaWalkSystemModules
(
 IN PSYSTEM_MODULE_INFORMATION SystemModules,
 IN PSYSMOD_ENUM_ROUTINE Callback,
 IN OUT PVOID CallbackContext
);

PSYSTEM_MODULE_INFORMATION_ENTRY
FASTCALL
PsaWalkFirstSystemModule
(
 IN PSYSTEM_MODULE_INFORMATION SystemModules
);

PSYSTEM_MODULE_INFORMATION_ENTRY
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
