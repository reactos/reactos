/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * PURPOSE:         Public Console Management Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS NTAPI
ConDrvInitConsole(
    IN OUT PCONSOLE Console,
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
