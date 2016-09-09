/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/console.h
 * PURPOSE:         Public Console Management Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID NTAPI
ConDrvInitConsoleSupport(VOID);

NTSTATUS NTAPI
ConDrvInitConsole(OUT PCONSOLE* NewConsole,
                  IN PCONSOLE_INFO ConsoleInfo);
NTSTATUS NTAPI
ConDrvAttachTerminal(IN PCONSOLE Console,
                     IN PTERMINAL Terminal);
NTSTATUS NTAPI
ConDrvDetachTerminal(IN PCONSOLE Console);
VOID NTAPI
ConDrvDeleteConsole(IN PCONSOLE Console);



BOOLEAN NTAPI
ConDrvValidateConsoleState(IN PCONSOLE Console,
                           IN CONSOLE_STATE ExpectedState);

BOOLEAN NTAPI
ConDrvValidateConsoleUnsafe(IN PCONSOLE Console,
                            IN CONSOLE_STATE ExpectedState,
                            IN BOOLEAN LockConsole);

/* EOF */
