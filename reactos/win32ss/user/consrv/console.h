/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/console.h
 * PURPOSE:         Console Initialization Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID WINAPI ConSrvInitConsoleSupport(VOID);

NTSTATUS WINAPI ConSrvInitConsole(OUT struct _CONSOLE** /* PCONSOLE* */ NewConsole,
                                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                                  IN PCSR_PROCESS ConsoleLeaderProcess);
VOID WINAPI ConSrvDeleteConsole(struct _CONSOLE* /* PCONSOLE */ Console);
NTSTATUS FASTCALL ConSrvGetConsole(PCONSOLE_PROCESS_DATA ProcessData,
                                   struct _CONSOLE** /* PCONSOLE* */ Console,
                                   BOOL LockConsole);
VOID FASTCALL ConSrvReleaseConsole(struct _CONSOLE* /* PCONSOLE */ Console,
                                   BOOL WasConsoleLocked);

/* EOF */
