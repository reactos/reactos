/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/guiterm.h
 * PURPOSE:         GUI Terminal Front-End
 * PROGRAMMERS:     Gé van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#define CONGUI_MIN_WIDTH      10
#define CONGUI_MIN_HEIGHT     10
#define CONGUI_UPDATE_TIME    0
#define CONGUI_UPDATE_TIMER   1

#define CURSOR_BLINK_TIME 500

NTSTATUS FASTCALL GuiInitConsole(PCONSOLE Console,
                                 /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
                                 PCONSOLE_INFO ConsoleInfo,
                                 DWORD ProcessId,
                                 LPCWSTR IconPath,
                                 INT IconIndex);

/* EOF */
