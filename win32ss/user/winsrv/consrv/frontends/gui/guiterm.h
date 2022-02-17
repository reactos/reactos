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


/* HELPER FUNCTIONS ***********************************************************/

FORCEINLINE
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

FORCEINLINE
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


/* FUNCTIONS ******************************************************************/

/* guiterm.c */

VOID
GuiConsoleMoveWindow(PGUI_CONSOLE_DATA GuiData);


/* conwnd.c */

BOOL
InitFonts(
    _Inout_ PGUI_CONSOLE_DATA GuiData,
    _In_reads_or_z_(LF_FACESIZE)
         PCWSTR FaceName,
    _In_ ULONG FontWeight,
    _In_ ULONG FontFamily,
    _In_ COORD FontSize,
    _In_opt_ UINT CodePage,
    _In_ BOOL UseDefaultFallback);

VOID
DeleteFonts(PGUI_CONSOLE_DATA GuiData);


/* fullscreen.c */

BOOL
EnterFullScreen(PGUI_CONSOLE_DATA GuiData);
VOID
LeaveFullScreen(PGUI_CONSOLE_DATA GuiData);
VOID
SwitchFullScreen(PGUI_CONSOLE_DATA GuiData, BOOL FullScreen);
VOID
GuiConsoleSwitchFullScreen(PGUI_CONSOLE_DATA GuiData);


/* graphics.c */

VOID
GuiCopyFromGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData);
VOID
GuiPasteToGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData);
VOID
GuiPaintGraphicsBuffer(PGRAPHICS_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer);


/* text.c */

VOID
PasteText(
    IN PCONSRV_CONSOLE Console,
    IN PWCHAR Buffer,
    IN SIZE_T cchSize);

VOID
GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData);
VOID
GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData);
VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer);

/* EOF */
