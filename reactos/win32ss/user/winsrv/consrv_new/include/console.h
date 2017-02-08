/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/include/console.h
 * PURPOSE:         Public Console Management Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID NTAPI
ConDrvInitConsoleSupport(VOID);

NTSTATUS NTAPI
ConDrvInitConsole(OUT PHANDLE NewConsoleHandle,
                  OUT PCONSOLE* NewConsole,
                  IN PCONSOLE_INFO ConsoleInfo,
                  IN ULONG ConsoleLeaderProcessId);
NTSTATUS NTAPI
ConDrvRegisterFrontEnd(IN PCONSOLE Console,
                       IN PFRONTEND FrontEnd);
NTSTATUS NTAPI
ConDrvDeregisterFrontEnd(IN PCONSOLE Console);
VOID NTAPI
ConDrvDeleteConsole(IN PCONSOLE Console);



BOOLEAN NTAPI
ConDrvValidateConsoleState(IN PCONSOLE Console,
                           IN CONSOLE_STATE ExpectedState);

BOOLEAN NTAPI
ConDrvValidateConsoleUnsafe(IN PCONSOLE Console,
                            IN CONSOLE_STATE ExpectedState,
                            IN BOOLEAN LockConsole);

BOOLEAN NTAPI
ConDrvValidateConsole(OUT PCONSOLE* Console,
                      IN HANDLE ConsoleHandle,
                      IN CONSOLE_STATE ExpectedState,
                      IN BOOLEAN LockConsole);



NTSTATUS NTAPI
ConDrvGetConsole(OUT PCONSOLE* Console,
                 IN HANDLE ConsoleHandle,
                 IN BOOLEAN LockConsole);
VOID NTAPI
ConDrvReleaseConsole(IN PCONSOLE Console,
                     IN BOOLEAN WasConsoleLocked);

/* EOF */
