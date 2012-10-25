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
#include <win/conmsg.h>
// #include <win/base.h>

//#include "guiconsole.h"
//#include "tuiconsole.h"

#include "resource.h"

/* Shared header with console.dll */
#include "console.h"

extern HANDLE ConSrvHeap;
// extern HANDLE BaseSrvSharedHeap;
// extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;

typedef struct Object_tt
{
    LONG Type;
    struct tagCSRSS_CONSOLE *Console;
    LONG AccessRead, AccessWrite;
    LONG ExclusiveRead, ExclusiveWrite;
    LONG HandleCount;
} Object_t;


/* alias.c */
CSR_API(SrvAddConsoleAlias);
CSR_API(SrvGetConsoleAlias);
CSR_API(SrvGetConsoleAliases);
CSR_API(SrvGetConsoleAliasesLength);
CSR_API(SrvGetConsoleAliasExes);
CSR_API(SrvGetConsoleAliasExesLength);

/* coninput.c */
CSR_API(SrvReadConsole);
CSR_API(CsrReadInputEvent);
CSR_API(SrvFlushConsoleInputBuffer);
CSR_API(SrvGetConsoleNumberOfInputEvents);
CSR_API(SrvGetConsoleInput);
CSR_API(SrvWriteConsoleInput);

/* conoutput.c */
CSR_API(SrvWriteConsole);
CSR_API(SrvGetConsoleScreenBufferInfo);
CSR_API(SrvSetConsoleCursor);
CSR_API(CsrWriteConsoleOutputChar);
CSR_API(CsrFillOutputChar);
CSR_API(CsrWriteConsoleOutputAttrib);
CSR_API(CsrFillOutputAttrib);
CSR_API(SrvGetConsoleCursorInfo);
CSR_API(SrvSetConsoleCursorInfo);
CSR_API(CsrSetTextAttrib);
CSR_API(SrvCreateConsoleScreenBuffer);
CSR_API(SrvSetConsoleActiveScreenBuffer);
CSR_API(SrvWriteConsoleOutput);
CSR_API(SrvScrollConsoleScreenBuffer);
CSR_API(CsrReadConsoleOutputChar);
CSR_API(CsrReadConsoleOutputAttrib);
CSR_API(SrvReadConsoleOutput);
CSR_API(SrvSetConsoleScreenBufferSize);

/* console.c */
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
CSR_API(CsrGetConsoleOutputCodePage);
CSR_API(CsrSetConsoleOutputCodePage);
CSR_API(SrvGetConsoleProcessList);
CSR_API(SrvGenerateConsoleCtrlEvent);
CSR_API(SrvGetConsoleSelectionInfo);

/* handle.c */
CSR_API(CsrGetHandle);
CSR_API(SrvCloseHandle);
CSR_API(SrvVerifyConsoleIoHandle);
CSR_API(SrvDuplicateHandle);
CSR_API(CsrGetInputWaitHandle);

NTSTATUS FASTCALL Win32CsrInsertObject(PCSR_PROCESS ProcessData,
                                       PHANDLE Handle,
                                       Object_t *Object,
                                       DWORD Access,
                                       BOOL Inheritable,
                                       DWORD ShareMode);
NTSTATUS FASTCALL Win32CsrLockObject(PCSR_PROCESS ProcessData,
                                     HANDLE Handle,
                                     Object_t **Object,
                                     DWORD Access,
                                     long Type);
VOID FASTCALL Win32CsrUnlockObject(Object_t *Object);
NTSTATUS FASTCALL Win32CsrReleaseObject(PCSR_PROCESS ProcessData,
                                        HANDLE Object);
VOID WINAPI Win32CsrReleaseConsole(PCSR_PROCESS ProcessData);
NTSTATUS WINAPI Win32CsrDuplicateHandleTable(PCSR_PROCESS SourceProcessData,
                                             PCSR_PROCESS TargetProcessData);

/* lineinput.c */
CSR_API(SrvGetConsoleCommandHistoryLength);
CSR_API(SrvGetConsoleCommandHistory);
CSR_API(SrvExpungeConsoleCommandHistory);
CSR_API(SrvSetConsoleNumberOfCommands);
CSR_API(SrvGetConsoleHistory);
CSR_API(SrvSetConsoleHistory);

/* server.c */
BOOL FASTCALL Win32CsrValidateBuffer(PCSR_PROCESS ProcessData,
                                     PVOID Buffer,
                                     SIZE_T NumElements,
                                     SIZE_T ElementSize);

#endif // __CONSRV_H__

/* EOF */
