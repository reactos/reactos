/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/guisettings.h
 * PURPOSE:         GUI front-end settings management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE: Also used by console.dll
 */

#pragma once

/* STRUCTURES *****************************************************************/

typedef struct _GUI_CONSOLE_INFO
{
    WCHAR FaceName[LF_FACESIZE];
    ULONG FontFamily;
    COORD FontSize;
    ULONG FontWeight;

    BOOL  FullScreen;       /* Whether the console is displayed in full-screen or windowed mode */
//  ULONG HardwareState;    /* _GDI_MANAGED, _DIRECT */

    WORD  ShowWindow;
    BOOL  AutoPosition;
    POINT WindowOrigin;
} GUI_CONSOLE_INFO, *PGUI_CONSOLE_INFO;

#ifndef CONSOLE_H__ // If we aren't included by console.dll

#include "conwnd.h"

/* FUNCTIONS ******************************************************************/

BOOL GuiConsoleReadUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo);
BOOL GuiConsoleWriteUserSettings(IN OUT PGUI_CONSOLE_INFO TermInfo);
VOID GuiConsoleGetDefaultSettings(IN OUT PGUI_CONSOLE_INFO TermInfo);

VOID GuiConsoleShowConsoleProperties(PGUI_CONSOLE_DATA GuiData,
                                     BOOL Defaults);
VOID GuiApplyUserSettings(PGUI_CONSOLE_DATA GuiData,
                          HANDLE hClientSection);

#endif

/* EOF */
