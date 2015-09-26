/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeLoader
 * FILE:            boot/freeldr/freeldr/include/ui/minitui.h
 * PURPOSE:         Mini Text UI interface header
 * PROGRAMMERS:     Hervé Poussineau
 */

#pragma once

///////////////////////////////////////////////////////////////////////////////////////
//
// Textual User Interface Functions
//
///////////////////////////////////////////////////////////////////////////////////////

VOID MiniTuiDrawBackdrop(VOID);
VOID MiniTuiDrawStatusText(PCSTR StatusText);
VOID MiniTuiDrawProgressBarCenter(ULONG Position, ULONG Range, PCHAR ProgressText);
VOID MiniTuiDrawProgressBar(ULONG Left, ULONG Top, ULONG Right, ULONG Bottom, ULONG Position, ULONG Range, PCHAR ProgressText);

///////////////////////////////////////////////////////////////////////////////////////
//
// Menu Functions
//
///////////////////////////////////////////////////////////////////////////////////////

VOID MiniTuiDrawMenu(PUI_MENU_INFO MenuInfo);

extern const UIVTBL MiniTuiVtbl;
