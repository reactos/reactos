/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/include/conio2.h
 * PURPOSE:         Internal Console I/O Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* Macros used to call functions in the FRONTEND_VTBL virtual table */

#define ConioDrawRegion(Console, Region) \
    (Console)->TermIFace.Vtbl->DrawRegion(&(Console)->TermIFace, (Region))
#define ConioWriteStream(Console, Block, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
    (Console)->TermIFace.Vtbl->WriteStream(&(Console)->TermIFace, (Block), (CurStartX), (CurStartY), \
                                           (ScrolledLines), (Buffer), (Length))
#define ConioSetCursorInfo(Console, Buff) \
    (Console)->TermIFace.Vtbl->SetCursorInfo(&(Console)->TermIFace, (Buff))
#define ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY) \
    (Console)->TermIFace.Vtbl->SetScreenInfo(&(Console)->TermIFace, (Buff), (OldCursorX), (OldCursorY))
#define ConioResizeTerminal(Console) \
    (Console)->TermIFace.Vtbl->ResizeTerminal(&(Console)->TermIFace)
#define ConioProcessKeyCallback(Console, Msg, KeyStateMenu, ShiftState, VirtualKeyCode, Down) \
    (Console)->TermIFace.Vtbl->ProcessKeyCallback(&(Console)->TermIFace, (Msg), (KeyStateMenu), (ShiftState), (VirtualKeyCode), (Down))
#define ConioRefreshInternalInfo(Console) \
    (Console)->TermIFace.Vtbl->RefreshInternalInfo(&(Console)->TermIFace)

#define ConioChangeTitle(Console) \
    (Console)->TermIFace.Vtbl->ChangeTitle(&(Console)->TermIFace)
#define ConioChangeIcon(Console, hWindowIcon) \
    (Console)->TermIFace.Vtbl->ChangeIcon(&(Console)->TermIFace, (hWindowIcon))
#define ConioGetConsoleWindowHandle(Console) \
    (Console)->TermIFace.Vtbl->GetConsoleWindowHandle(&(Console)->TermIFace)
#define ConioGetLargestConsoleWindowSize(Console, pSize) \
    (Console)->TermIFace.Vtbl->GetLargestConsoleWindowSize(&(Console)->TermIFace, (pSize))
#define ConioGetDisplayMode(Console) \
    (Console)->TermIFace.Vtbl->GetDisplayMode(&(Console)->TermIFace)
#define ConioSetDisplayMode(Console, NewMode) \
    (Console)->TermIFace.Vtbl->SetDisplayMode(&(Console)->TermIFace, (NewMode))
#define ConioShowMouseCursor(Console, Show) \
    (Console)->TermIFace.Vtbl->ShowMouseCursor(&(Console)->TermIFace, (Show))
#define ConioSetMouseCursor(Console, hCursor) \
    (Console)->TermIFace.Vtbl->SetMouseCursor(&(Console)->TermIFace, (hCursor))
#define ConioMenuControl(Console, CmdIdLow, CmdIdHigh) \
    (Console)->TermIFace.Vtbl->MenuControl(&(Console)->TermIFace, (CmdIdLow), (CmdIdHigh))
#define ConioSetMenuClose(Console, Enable) \
    (Console)->TermIFace.Vtbl->SetMenuClose(&(Console)->TermIFace, (Enable))

/* EOF */
