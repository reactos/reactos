/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_DBGK_H
#define __NTOSKRNL_INCLUDE_INTERNAL_DBGK_H

//
// DebugObject access rights.
// Note that DEBUG_OBJECT_ALL_ACCESS includes the specific rights 0x0F, but there are only two
// debug object specific access rights that are ever checked by the kernel. This appears to be a bug.
//
#define DEBUG_OBJECT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x0F)
#define DEBUG_OBJECT_WAIT_STATE_CHANGE 0x0001 /* Required to call NtWaitForDebugEvent & NtContinueDebugEvent */
#define DEBUG_OBJECT_ADD_REMOVE_PROCESS 0x0002 /* Required to call NtDebugActiveProcess & NtRemoveProcessDebug */

typedef enum _DEBUGOBJECTINFOCLASS {
       DebugObjectUnusedInformation,
       DebugObjectKillProcessOnExitInformation
} DEBUGOBJECTINFOCLASS, * PDEBUGOBJECTINFOCLASS;

typedef struct _DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION {
       ULONG KillProcessOnExit;                // Interpreted as a BOOLEAN, TRUE -> process is terminated on NtRemoveProcessDebug.
} DEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION, *
PDEBUG_OBJECT_KILL_PROCESS_ON_EXIT_INFORMATION;

//
// Debug Object
//
typedef struct _DBGK_DEBUG_OBJECT {
    KEVENT Event;
    FAST_MUTEX Mutex;
    LIST_ENTRY StateEventListEntry;
    union {
        ULONG Flags;
        struct {
            UCHAR DebuggerInactive  :1;
            UCHAR KillProcessOnExit :1;
        };
    };
} DBGK_DEBUG_OBJECT, *PDBGK_DEBUG_OBJECT;

//
// DbgUi types for LPC debug port messages (KM -> UM).
//
// These also apply to Nt*Debug APIs with NT 5.01, 5.02, and later.
//
typedef enum _DBG_STATE {
       DbgIdle,
       DbgReplyPending,
       DbgCreateThreadStateChange,
       DbgCreateProcessStateChange,
       DbgExitThreadStateChange,
       DbgExitProcessStateChange,
       DbgExceptionStateChange,
       DbgBreakpointStateChange,
       DbgSingleStepStateChange,
       DbgLoadDllStateChange,
       DbgUnloadDllStateChange
} DBG_STATE, *PDBG_STATE;

typedef struct _DBGKM_EXCEPTION {
       EXCEPTION_RECORD ExceptionRecord;
       ULONG FirstChance;
} DBGKM_EXCEPTION, *PDBGKM_EXCEPTION;

typedef struct _DBGKM_CREATE_THREAD {
       ULONG SubSystemKey;
       PVOID StartAddress;
} DBGKM_CREATE_THREAD, *PDBGKM_CREATE_THREAD;

typedef struct _DBGKM_CREATE_PROCESS {
       ULONG SubSystemKey;
       HANDLE FileHandle;
       PVOID BaseOfImage;
       ULONG DebugInfoFileOffset;
       ULONG DebugInfoSize;
       DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, *PDBGKM_CREATE_PROCESS;

typedef struct _DBGKM_EXIT_THREAD {
       NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, *PDBGKM_EXIT_THREAD;

typedef struct _DBGKM_EXIT_PROCESS {
       NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, *PDBGKM_EXIT_PROCESS;

typedef struct _DBGKM_LOAD_DLL {
       HANDLE FileHandle;
       PVOID BaseOfDll;
       ULONG DebugInfoFileOffset;
       ULONG DebugInfoSize;
} DBGKM_LOAD_DLL, *PDBGKM_LOAD_DLL;

typedef struct _DBGKM_UNLOAD_DLL {
       PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, *PDBGKM_UNLOAD_DLL;

typedef struct _DBGUI_WAIT_STATE_CHANGE {
       DBG_STATE NewState;
       CLIENT_ID AppClientId;
       union {
               struct {
                       HANDLE HandleToThread;
                       DBGKM_CREATE_THREAD NewThread;
               } CreateThread;
               struct {
                       HANDLE HandleToProcess;
                       HANDLE HandleToThread;
                       DBGKM_CREATE_PROCESS NewProcess;
               } CreateProcessInfo;
               DBGKM_EXIT_THREAD ExitThread;
               DBGKM_EXIT_PROCESS ExitProcess;
               DBGKM_EXCEPTION Exception;
               DBGKM_LOAD_DLL LoadDll;
               DBGKM_UNLOAD_DLL UnloadDll;
       } StateInfo;
} DBGUI_WAIT_STATE_CHANGE, * PDBGUI_WAIT_STATE_CHANGE;


VOID
STDCALL
DbgkCreateThread(PVOID StartAddress);

#endif

/* EOF */
