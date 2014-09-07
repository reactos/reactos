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

#include "guisettings.h"
#include "conwnd.h"

VOID
GuiConsoleMoveWindow(PGUI_CONSOLE_DATA GuiData);

VOID
SwitchFullScreen(PGUI_CONSOLE_DATA GuiData, BOOL FullScreen);

BOOL
InitFonts(PGUI_CONSOLE_DATA GuiData,
          LPWSTR FaceName, // Points to a WCHAR array of LF_FACESIZE elements.
          ULONG  FontFamily,
          COORD  FontSize,
          ULONG  FontWeight);
VOID
DeleteFonts(PGUI_CONSOLE_DATA GuiData);
