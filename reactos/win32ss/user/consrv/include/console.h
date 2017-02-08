/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/include/console.h
 * PURPOSE:         Public Console Management Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

BOOL FASTCALL ConSrvValidateConsolePointer(PCONSOLE Console);
BOOL FASTCALL ConSrvValidateConsoleState(PCONSOLE Console,
                                         CONSOLE_STATE ExpectedState);
BOOL FASTCALL ConSrvValidateConsoleUnsafe(PCONSOLE Console,
                                          CONSOLE_STATE ExpectedState,
                                          BOOL LockConsole);
BOOL FASTCALL ConSrvValidateConsole(PCONSOLE Console,
                                    CONSOLE_STATE ExpectedState,
                                    BOOL LockConsole);

/* EOF */
