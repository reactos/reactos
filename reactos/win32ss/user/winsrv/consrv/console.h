/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/console.h
 * PURPOSE:         Console Initialization Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID NTAPI
ConSrvInitConsoleSupport(VOID);

NTSTATUS NTAPI
ConSrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT struct _CONSOLE** /* PCONSOLE* */ NewConsole,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN ULONG ConsoleLeaderProcessId);
VOID NTAPI ConSrvDeleteConsole(struct _CONSOLE* /* PCONSOLE */ Console);

NTSTATUS
ConSrvGetConsole(IN PCONSOLE_PROCESS_DATA ProcessData,
                 OUT struct _CONSOLE** /* PCONSOLE* */ Console,
                 IN BOOLEAN LockConsole);
VOID
ConSrvReleaseConsole(IN struct _CONSOLE* /* PCONSOLE */ Console,
                     IN BOOLEAN WasConsoleLocked);


BOOLEAN NTAPI
ConSrvValidateConsole(OUT struct _CONSOLE** /* PCONSOLE* */ Console,
                      IN HANDLE ConsoleHandle,
                      IN CONSOLE_STATE ExpectedState,
                      IN BOOLEAN LockConsole);
