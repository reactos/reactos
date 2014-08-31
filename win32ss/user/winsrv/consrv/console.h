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
                  OUT struct _CONSRV_CONSOLE** /* PCONSRV_CONSOLE* */ NewConsole,
                  IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                  IN ULONG ConsoleLeaderProcessId);
VOID NTAPI ConSrvDeleteConsole(struct _CONSRV_CONSOLE* /* PCONSRV_CONSOLE */ Console);

NTSTATUS
ConSrvGetConsole(IN PCONSOLE_PROCESS_DATA ProcessData,
                 OUT struct _CONSRV_CONSOLE** /* PCONSRV_CONSOLE* */ Console,
                 IN BOOLEAN LockConsole);
VOID
ConSrvReleaseConsole(IN struct _CONSRV_CONSOLE* /* PCONSRV_CONSOLE */ Console,
                     IN BOOLEAN WasConsoleLocked);


BOOLEAN NTAPI
ConSrvValidateConsole(OUT struct _CONSRV_CONSOLE** /* PCONSRV_CONSOLE* */ Console,
                      IN HANDLE ConsoleHandle,
                      IN CONSOLE_STATE ExpectedState,
                      IN BOOLEAN LockConsole);
