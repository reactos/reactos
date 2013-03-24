/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/guiconsole.h
 * PURPOSE:         GUI front-end
 * PROGRAMMERS:
 */

#pragma once

// #include "guisettings.h"

#define CONGUI_MIN_WIDTH      10
#define CONGUI_MIN_HEIGHT     10
#define CONGUI_UPDATE_TIME    0
#define CONGUI_UPDATE_TIMER   1

NTSTATUS FASTCALL GuiInitConsole(PCONSOLE Console,
                                 /*IN*/ PCONSOLE_START_INFO ConsoleStartInfo,
                                 PCONSOLE_INFO ConsoleInfo,
                                 DWORD ProcessId,
                                 LPCWSTR IconPath,
                                 INT IconIndex);

/* EOF */
