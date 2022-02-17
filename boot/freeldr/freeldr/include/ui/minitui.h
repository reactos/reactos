/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/include/ui/minitui.h
 * PURPOSE:         Mini Text UI interface header
 * PROGRAMMERS:     Hervé Poussineau
 */

#pragma once

/* Textual User Interface Functions ******************************************/

VOID MiniTuiDrawBackdrop(VOID);
VOID MiniTuiDrawStatusText(PCSTR StatusText);

/* Draws the progress bar showing nPos percent filled */
VOID
MiniTuiDrawProgressBarCenter(
    _In_ ULONG Position,
    _In_ ULONG Range,
    _Inout_z_ PSTR ProgressText);

/* Draws the progress bar showing nPos percent filled */
VOID
MiniTuiDrawProgressBar(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ ULONG Position,
    _In_ ULONG Range,
    _Inout_z_ PSTR ProgressText);

/* Menu Functions ************************************************************/

VOID
MiniTuiDrawMenu(
    _In_ PUI_MENU_INFO MenuInfo);

extern const UIVTBL MiniTuiVtbl;
