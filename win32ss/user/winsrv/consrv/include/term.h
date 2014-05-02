/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/term.h
 * PURPOSE:         Internal Frontend Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* Macros used to call functions in the FRONTEND_VTBL virtual table */

#define TermDrawRegion(Console, Region) \
    (Console)->FrontEndIFace.Vtbl->DrawRegion(&(Console)->FrontEndIFace, (Region))
#define TermWriteStream(Console, Block, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
    (Console)->FrontEndIFace.Vtbl->WriteStream(&(Console)->FrontEndIFace, (Block), (CurStartX), (CurStartY), \
                                           (ScrolledLines), (Buffer), (Length))
#define TermSetCursorInfo(Console, ScreenBuffer) \
    (Console)->FrontEndIFace.Vtbl->SetCursorInfo(&(Console)->FrontEndIFace, (ScreenBuffer))
#define TermSetScreenInfo(Console, ScreenBuffer, OldCursorX, OldCursorY) \
    (Console)->FrontEndIFace.Vtbl->SetScreenInfo(&(Console)->FrontEndIFace, (ScreenBuffer), (OldCursorX), (OldCursorY))
#define TermResizeTerminal(Console) \
    (Console)->FrontEndIFace.Vtbl->ResizeTerminal(&(Console)->FrontEndIFace)
#define TermSetActiveScreenBuffer(Console) \
    (Console)->FrontEndIFace.Vtbl->SetActiveScreenBuffer(&(Console)->FrontEndIFace)
#define TermReleaseScreenBuffer(Console, ScreenBuffer) \
    (Console)->FrontEndIFace.Vtbl->ReleaseScreenBuffer(&(Console)->FrontEndIFace, (ScreenBuffer))
#define TermProcessKeyCallback(Console, Msg, KeyStateMenu, ShiftState, VirtualKeyCode, Down) \
    (Console)->FrontEndIFace.Vtbl->ProcessKeyCallback(&(Console)->FrontEndIFace, (Msg), (KeyStateMenu), (ShiftState), (VirtualKeyCode), (Down))
#define TermRefreshInternalInfo(Console) \
    (Console)->FrontEndIFace.Vtbl->RefreshInternalInfo(&(Console)->FrontEndIFace)

#define TermChangeTitle(Console) \
    (Console)->FrontEndIFace.Vtbl->ChangeTitle(&(Console)->FrontEndIFace)
#define TermChangeIcon(Console, IconHandle) \
    (Console)->FrontEndIFace.Vtbl->ChangeIcon(&(Console)->FrontEndIFace, (IconHandle))
#define TermGetConsoleWindowHandle(Console) \
    (Console)->FrontEndIFace.Vtbl->GetConsoleWindowHandle(&(Console)->FrontEndIFace)
#define TermGetLargestConsoleWindowSize(Console, pSize) \
    (Console)->FrontEndIFace.Vtbl->GetLargestConsoleWindowSize(&(Console)->FrontEndIFace, (pSize))
#define TermGetSelectionInfo(Console, pSelectionInfo) \
    (Console)->FrontEndIFace.Vtbl->GetSelectionInfo(&(Console)->FrontEndIFace, (pSelectionInfo))
#define TermSetPalette(Console, PaletteHandle, PaletteUsage) \
    (Console)->FrontEndIFace.Vtbl->SetPalette(&(Console)->FrontEndIFace, (PaletteHandle), (PaletteUsage))
#define TermGetDisplayMode(Console) \
    (Console)->FrontEndIFace.Vtbl->GetDisplayMode(&(Console)->FrontEndIFace)
#define TermSetDisplayMode(Console, NewMode) \
    (Console)->FrontEndIFace.Vtbl->SetDisplayMode(&(Console)->FrontEndIFace, (NewMode))
#define TermShowMouseCursor(Console, Show) \
    (Console)->FrontEndIFace.Vtbl->ShowMouseCursor(&(Console)->FrontEndIFace, (Show))
#define TermSetMouseCursor(Console, CursorHandle) \
    (Console)->FrontEndIFace.Vtbl->SetMouseCursor(&(Console)->FrontEndIFace, (CursorHandle))
#define TermMenuControl(Console, CmdIdLow, CmdIdHigh) \
    (Console)->FrontEndIFace.Vtbl->MenuControl(&(Console)->FrontEndIFace, (CmdIdLow), (CmdIdHigh))
#define TermSetMenuClose(Console, Enable) \
    (Console)->FrontEndIFace.Vtbl->SetMenuClose(&(Console)->FrontEndIFace, (Enable))

/* EOF */
