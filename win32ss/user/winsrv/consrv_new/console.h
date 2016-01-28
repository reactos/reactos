/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/console.h
 * PURPOSE:         Console Initialization Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

// FIXME: Fix compilation
struct _CONSOLE;

NTSTATUS WINAPI
ConSrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT struct _CONSOLE** /* PCONSOLE* */ NewConsole,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN ULONG ConsoleLeaderProcessId);
VOID WINAPI ConSrvDeleteConsole(struct _CONSOLE* /* PCONSOLE */ Console);

NTSTATUS FASTCALL ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                   struct _CONSOLE** /* PCONSOLE* */ Console,
                                   BOOL LockConsole);
VOID FASTCALL ConSrvReleaseConsole(struct _CONSOLE* /* PCONSOLE */ Console,
                                   BOOL WasConsoleLocked);

/* EOF */
