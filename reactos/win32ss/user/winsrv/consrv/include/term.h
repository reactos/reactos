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
    (Console)->TermIFace.Vtbl->DrawRegion(&(Console)->TermIFace, (Region))
#define TermWriteStream(Console, Block, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
    (Console)->TermIFace.Vtbl->WriteStream(&(Console)->TermIFace, (Block), (CurStartX), (CurStartY), \
                                           (ScrolledLines), (Buffer), (Length))
#define TermSetCursorInfo(Console, ScreenBuffer) \
    (Console)->TermIFace.Vtbl->SetCursorInfo(&(Console)->TermIFace, (ScreenBuffer))
#define TermSetScreenInfo(Console, ScreenBuffer, OldCursorX, OldCursorY) \
    (Console)->TermIFace.Vtbl->SetScreenInfo(&(Console)->TermIFace, (ScreenBuffer), (OldCursorX), (OldCursorY))
#define TermResizeTerminal(Console) \
    (Console)->TermIFace.Vtbl->ResizeTerminal(&(Console)->TermIFace)
#define TermSetActiveScreenBuffer(Console) \
    (Console)->TermIFace.Vtbl->SetActiveScreenBuffer(&(Console)->TermIFace)
#define TermReleaseScreenBuffer(Console, ScreenBuffer) \
    (Console)->TermIFace.Vtbl->ReleaseScreenBuffer(&(Console)->TermIFace, (ScreenBuffer))
#define TermProcessKeyCallback(Console, Msg, KeyStateMenu, ShiftState, VirtualKeyCode, Down) \
    (Console)->TermIFace.Vtbl->ProcessKeyCallback(&(Console)->TermIFace, (Msg), (KeyStateMenu), (ShiftState), (VirtualKeyCode), (Down))
#define TermRefreshInternalInfo(Console) \
    (Console)->TermIFace.Vtbl->RefreshInternalInfo(&(Console)->TermIFace)

#define TermChangeTitle(Console) \
    (Console)->TermIFace.Vtbl->ChangeTitle(&(Console)->TermIFace)
#define TermChangeIcon(Console, hWindowIcon) \
    (Console)->TermIFace.Vtbl->ChangeIcon(&(Console)->TermIFace, (hWindowIcon))
#define TermGetConsoleWindowHandle(Console) \
    (Console)->TermIFace.Vtbl->GetConsoleWindowHandle(&(Console)->TermIFace)
#define TermGetLargestConsoleWindowSize(Console, pSize) \
    (Console)->TermIFace.Vtbl->GetLargestConsoleWindowSize(&(Console)->TermIFace, (pSize))
#define TermGetDisplayMode(Console) \
    (Console)->TermIFace.Vtbl->GetDisplayMode(&(Console)->TermIFace)
#define TermSetDisplayMode(Console, NewMode) \
    (Console)->TermIFace.Vtbl->SetDisplayMode(&(Console)->TermIFace, (NewMode))
#define TermShowMouseCursor(Console, Show) \
    (Console)->TermIFace.Vtbl->ShowMouseCursor(&(Console)->TermIFace, (Show))
#define TermSetMouseCursor(Console, hCursor) \
    (Console)->TermIFace.Vtbl->SetMouseCursor(&(Console)->TermIFace, (hCursor))
#define TermMenuControl(Console, CmdIdLow, CmdIdHigh) \
    (Console)->TermIFace.Vtbl->MenuControl(&(Console)->TermIFace, (CmdIdLow), (CmdIdHigh))
#define TermSetMenuClose(Console, Enable) \
    (Console)->TermIFace.Vtbl->SetMenuClose(&(Console)->TermIFace, (Enable))

/* EOF */
