/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/consrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __CONSRV_H__
#define __CONSRV_H__

#pragma once

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* CONSOLE Headers */
#include <win/console.h>
#include <win/conmsg.h>

#include "resource.h"

/* Shared header with console.dll */
#include "console.h"


extern HINSTANCE ConSrvDllInstance;
extern HANDLE ConSrvHeap;
// extern HANDLE BaseSrvSharedHeap;
// extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;

/* Object type magic numbers */
#define CONIO_CONSOLE_MAGIC         0x00000001  // -->  Input-type handles
#define CONIO_SCREEN_BUFFER_MAGIC   0x00000002  // --> Output-type handles

/* Common things to input/output/console objects */
typedef struct Object_tt
{
    LONG Type;
    struct tagCSRSS_CONSOLE *Console;
    LONG AccessRead, AccessWrite;
    LONG ExclusiveRead, ExclusiveWrite;
    LONG HandleCount;
} Object_t;


typedef struct _CSRSS_HANDLE
{
    Object_t *Object;   /* The object on which the handle points to */
    DWORD Access;
    BOOL Inheritable;
    DWORD ShareMode;
} CSRSS_HANDLE, *PCSRSS_HANDLE;


#define ConsoleGetPerProcessData(pcsrprocess)   \
    ((PCONSOLE_PROCESS_DATA)((pcsrprocess)->ServerData[CONSRV_SERVERDLL_INDEX]))

typedef struct _CONSOLE_PROCESS_DATA
{
    LIST_ENTRY ConsoleLink;
    PCSR_PROCESS Process;   // Process owning this structure.
    HANDLE ConsoleEvent;
    /* PCSRSS_CONSOLE */ struct tagCSRSS_CONSOLE* Console;
    /* PCSRSS_CONSOLE */ struct tagCSRSS_CONSOLE* ParentConsole;

    // BOOL bInheritHandles;
    BOOL ConsoleApp;    // TRUE if it is a CUI app, FALSE otherwise.

    RTL_CRITICAL_SECTION HandleTableLock;
    ULONG HandleTableSize;
    PCSRSS_HANDLE HandleTable; // Length-varying table
    LPTHREAD_START_ROUTINE CtrlDispatcher;
} CONSOLE_PROCESS_DATA, *PCONSOLE_PROCESS_DATA;


/* alias.c */
CSR_API(SrvAddConsoleAlias);
CSR_API(SrvGetConsoleAlias);
CSR_API(SrvGetConsoleAliases);
CSR_API(SrvGetConsoleAliasesLength);
CSR_API(SrvGetConsoleAliasExes);
CSR_API(SrvGetConsoleAliasExesLength);

/* coninput.c */
CSR_API(SrvGetConsoleInput);
CSR_API(SrvWriteConsoleInput);
CSR_API(SrvReadConsole);
CSR_API(SrvFlushConsoleInputBuffer);
CSR_API(SrvGetConsoleNumberOfInputEvents);

/* conoutput.c */
CSR_API(SrvReadConsoleOutput);
CSR_API(SrvWriteConsoleOutput);
CSR_API(SrvReadConsoleOutputString);
CSR_API(SrvWriteConsoleOutputString);
CSR_API(SrvFillConsoleOutput);
CSR_API(SrvWriteConsole);
CSR_API(SrvSetConsoleCursorPosition);
CSR_API(SrvGetConsoleCursorInfo);
CSR_API(SrvSetConsoleCursorInfo);
CSR_API(SrvSetConsoleTextAttribute);
CSR_API(SrvCreateConsoleScreenBuffer);
CSR_API(SrvGetConsoleScreenBufferInfo);
CSR_API(SrvSetConsoleActiveScreenBuffer);
CSR_API(SrvScrollConsoleScreenBuffer);
CSR_API(SrvSetConsoleScreenBufferSize);

/* console.c */
CSR_API(SrvOpenConsole);
CSR_API(SrvAllocConsole);
CSR_API(SrvFreeConsole);
CSR_API(SrvSetConsoleMode);
CSR_API(SrvGetConsoleMode);
CSR_API(SrvSetConsoleTitle);
CSR_API(SrvGetConsoleTitle);
CSR_API(SrvGetConsoleHardwareState);
CSR_API(SrvSetConsoleHardwareState);
CSR_API(SrvGetConsoleWindow);
CSR_API(SrvSetConsoleIcon);
CSR_API(SrvGetConsoleCP);
CSR_API(SrvSetConsoleCP);
CSR_API(SrvGetConsoleProcessList);
CSR_API(SrvGenerateConsoleCtrlEvent);
CSR_API(SrvGetConsoleSelectionInfo);

/* handle.c */
CSR_API(SrvCloseHandle);
CSR_API(SrvVerifyConsoleIoHandle);
CSR_API(SrvDuplicateHandle);
/// CSR_API(CsrGetInputWaitHandle);

NTSTATUS FASTCALL Win32CsrInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object,
                                       DWORD Access,
                                       BOOL Inheritable,
                                       DWORD ShareMode);
NTSTATUS FASTCALL Win32CsrLockObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     HANDLE Handle,
                                     Object_t **Object,
                                     DWORD Access,
                                     LONG Type);
VOID FASTCALL Win32CsrUnlockObject(Object_t *Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCONSOLE_PROCESS_DATA ProcessData,
                                        HANDLE Handle);

NTSTATUS NTAPI ConsoleNewProcess(PCSR_PROCESS SourceProcess,
                                 PCSR_PROCESS TargetProcess);
NTSTATUS NTAPI ConsoleConnect(IN PCSR_PROCESS CsrProcess,
                              IN OUT PVOID ConnectionInfo,
                              IN OUT PULONG ConnectionInfoLength);
VOID NTAPI ConsoleDisconnect(PCSR_PROCESS Process);
VOID NTAPI Win32CsrReleaseConsole(PCSR_PROCESS Process);

/* lineinput.c */
CSR_API(SrvGetConsoleCommandHistoryLength);
CSR_API(SrvGetConsoleCommandHistory);
CSR_API(SrvExpungeConsoleCommandHistory);
CSR_API(SrvSetConsoleNumberOfCommands);
CSR_API(SrvGetConsoleHistory);
CSR_API(SrvSetConsoleHistory);

#endif // __CONSRV_H__

/* EOF */
