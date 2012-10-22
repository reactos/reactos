
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

#include "guiconsole.h"
#include "tuiconsole.h"

/* Shared header with console.dll */
#include "console.h"

extern HANDLE ConSrvHeap;
extern HANDLE BaseSrvSharedHeap;
extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;


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

/* alias.c */
CSR_API(SrvAddConsoleAlias);
CSR_API(SrvGetConsoleAlias);
CSR_API(SrvGetConsoleAliases);
CSR_API(SrvGetConsoleAliasesLength);
CSR_API(SrvGetConsoleAliasExes);
CSR_API(SrvGetConsoleAliasExesLength);

/* lineinput.c */
CSR_API(SrvGetConsoleCommandHistoryLength);
CSR_API(SrvGetConsoleCommandHistory);
CSR_API(SrvExpungeConsoleCommandHistory);
CSR_API(SrvSetConsoleNumberOfCommands);
CSR_API(SrvGetConsoleHistory);
CSR_API(SrvSetConsoleHistory);

#endif // __CONSRV_H__

/* EOF */
