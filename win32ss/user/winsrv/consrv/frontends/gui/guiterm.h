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

extern inline
VOID
GetScreenBufferSizeUnits(IN PCONSOLE_SCREEN_BUFFER Buffer,
                         IN PGUI_CONSOLE_DATA GuiData,
                         OUT PUINT WidthUnit,
                         OUT PUINT HeightUnit)
{
    ASSERT(Buffer && GuiData && WidthUnit && HeightUnit);

    if (GetType(Buffer) == TEXTMODE_BUFFER)
    {
        *WidthUnit  = GuiData->CharWidth ;
        *HeightUnit = GuiData->CharHeight;
    }
    else /* if (GetType(Buffer) == GRAPHICS_BUFFER) */
    {
        *WidthUnit  = 1;
        *HeightUnit = 1;
    }
}

extern inline
VOID
SmallRectToRect(PGUI_CONSOLE_DATA GuiData, PRECT Rect, PSMALL_RECT SmallRect)
{
    PCONSOLE_SCREEN_BUFFER Buffer = GuiData->ActiveBuffer;
    UINT WidthUnit, HeightUnit;

    GetScreenBufferSizeUnits(Buffer, GuiData, &WidthUnit, &HeightUnit);

    Rect->left   = (SmallRect->Left       - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->top    = (SmallRect->Top        - Buffer->ViewOrigin.Y) * HeightUnit;
    Rect->right  = (SmallRect->Right  + 1 - Buffer->ViewOrigin.X) * WidthUnit ;
    Rect->bottom = (SmallRect->Bottom + 1 - Buffer->ViewOrigin.Y) * HeightUnit;
}

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
