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

/* Public Win32K Headers */
#include <ntuser.h>

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
#define CONIO_INPUT_BUFFER_MAGIC    0x00000001  // -->  Input-type handles
#define CONIO_SCREEN_BUFFER_MAGIC   0x00000002  // --> Output-type handles

/* Common things to input/output/console objects */
typedef struct Object_tt
{
    ULONG Type;
    struct _CONSOLE *Console;
    LONG AccessRead, AccessWrite;
    LONG ExclusiveRead, ExclusiveWrite;
    LONG HandleCount;
} Object_t;


typedef struct _CONSOLE_IO_HANDLE
{
    Object_t *Object;   /* The object on which the handle points to */
    DWORD Access;
    BOOL Inheritable;
    DWORD ShareMode;
} CONSOLE_IO_HANDLE, *PCONSOLE_IO_HANDLE;


#define ConsoleGetPerProcessData(Process)   \
    ((PCONSOLE_PROCESS_DATA)((Process)->ServerData[CONSRV_SERVERDLL_INDEX]))

typedef struct _CONSOLE_PROCESS_DATA
{
    LIST_ENTRY ConsoleLink;
    PCSR_PROCESS Process;   // Process owning this structure.
    HANDLE ConsoleEvent;
    /* PCONSOLE */ struct _CONSOLE* Console;
    /* PCONSOLE */ struct _CONSOLE* ParentConsole;

    BOOL ConsoleApp;    // TRUE if it is a CUI app, FALSE otherwise.

    RTL_CRITICAL_SECTION HandleTableLock;
    ULONG HandleTableSize;
    PCONSOLE_IO_HANDLE HandleTable; // Length-varying table

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
CSR_API(SrvAttachConsole);
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

NTSTATUS FASTCALL ConSrvInitHandlesTable(IN OUT PCONSOLE_PROCESS_DATA ProcessData,
                                         OUT PHANDLE pInputHandle,
                                         OUT PHANDLE pOutputHandle,
                                         OUT PHANDLE pErrorHandle);
NTSTATUS FASTCALL ConSrvInheritHandlesTable(IN PCONSOLE_PROCESS_DATA SourceProcessData,
                                            IN PCONSOLE_PROCESS_DATA TargetProcessData);
VOID FASTCALL ConSrvFreeHandlesTable(PCONSOLE_PROCESS_DATA ProcessData);
NTSTATUS FASTCALL ConSrvInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     PHANDLE Handle,
                                     Object_t *Object,
                                     DWORD Access,
                                     BOOL Inheritable,
                                     DWORD ShareMode);
NTSTATUS FASTCALL ConSrvRemoveObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     HANDLE Handle);
NTSTATUS FASTCALL ConSrvGetObject(PCONSOLE_PROCESS_DATA ProcessData,
                                  HANDLE Handle,
                                  Object_t** Object,
                                  PCONSOLE_IO_HANDLE* Entry OPTIONAL,
                                  DWORD Access,
                                  BOOL LockConsole,
                                  ULONG Type);
VOID FASTCALL ConSrvReleaseObject(Object_t *Object,
                                 BOOL IsConsoleLocked);
NTSTATUS FASTCALL ConSrvAllocateConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                        PHANDLE pInputHandle,
                                        PHANDLE pOutputHandle,
                                        PHANDLE pErrorHandle,
                                        int ShowCmd,
                                        PCSR_PROCESS CsrProcess);
VOID FASTCALL ConSrvRemoveConsole(PCONSOLE_PROCESS_DATA ProcessData);
NTSTATUS FASTCALL ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                   struct _CONSOLE** Console,
                                   BOOL LockConsole);
VOID FASTCALL ConSrvReleaseConsole(struct _CONSOLE* Console,
                                   BOOL IsConsoleLocked);

NTSTATUS NTAPI ConSrvNewProcess(PCSR_PROCESS SourceProcess,
                                PCSR_PROCESS TargetProcess);
NTSTATUS NTAPI ConSrvConnect(IN PCSR_PROCESS CsrProcess,
                             IN OUT PVOID ConnectionInfo,
                             IN OUT PULONG ConnectionInfoLength);
VOID NTAPI ConSrvDisconnect(PCSR_PROCESS Process);

/* lineinput.c */
CSR_API(SrvGetConsoleCommandHistoryLength);
CSR_API(SrvGetConsoleCommandHistory);
CSR_API(SrvExpungeConsoleCommandHistory);
CSR_API(SrvSetConsoleNumberOfCommands);
CSR_API(SrvGetConsoleHistory);
CSR_API(SrvSetConsoleHistory);

#endif // __CONSRV_H__

/* EOF */
