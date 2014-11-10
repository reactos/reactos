/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/tui/tuiterm.h
 * PURPOSE:         TUI Terminal Front-End
 * PROGRAMMERS:     David Welch
 *                  Gé van Geldorp
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

NTSTATUS FASTCALL TuiInitConsole(PCONSOLE Console,
                                 /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
                                 PCONSOLE_INFO ConsoleInfo,
                                 DWORD ProcessId);

/* EOF */
