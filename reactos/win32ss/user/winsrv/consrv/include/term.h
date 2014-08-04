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
#define TermWriteStream(Console, Region, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
    (Console)->TermIFace.Vtbl->WriteStream(&(Console)->TermIFace, (Region), (CurStartX), (CurStartY), \
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
#define TermRefreshInternalInfo(Console) \
    (Console)->FrontEndIFace.Vtbl->RefreshInternalInfo(&(Console)->FrontEndIFace)

#define TermChangeTitle(Console) \
    (Console)->TermIFace.Vtbl->ChangeTitle(&(Console)->TermIFace)
#define TermChangeIcon(Console, IconHandle) \
    (Console)->FrontEndIFace.Vtbl->ChangeIcon(&(Console)->FrontEndIFace, (IconHandle))
#define TermGetConsoleWindowHandle(Console) \
    (Console)->FrontEndIFace.Vtbl->GetConsoleWindowHandle(&(Console)->FrontEndIFace)
#define TermGetLargestConsoleWindowSize(Console, pSize) \
    (Console)->TermIFace.Vtbl->GetLargestConsoleWindowSize(&(Console)->TermIFace, (pSize))
#define TermGetSelectionInfo(Console, pSelectionInfo) \
    (Console)->FrontEndIFace.Vtbl->GetSelectionInfo(&(Console)->FrontEndIFace, (pSelectionInfo))
#define TermSetPalette(Console, PaletteHandle, PaletteUsage) \
    (Console)->TermIFace.Vtbl->SetPalette(&(Console)->TermIFace, (PaletteHandle), (PaletteUsage))
#define TermGetDisplayMode(Console) \
    (Console)->FrontEndIFace.Vtbl->GetDisplayMode(&(Console)->FrontEndIFace)
#define TermSetDisplayMode(Console, NewMode) \
    (Console)->FrontEndIFace.Vtbl->SetDisplayMode(&(Console)->FrontEndIFace, (NewMode))
#define TermShowMouseCursor(Console, Show) \
    (Console)->TermIFace.Vtbl->ShowMouseCursor(&(Console)->TermIFace, (Show))
#define TermSetMouseCursor(Console, CursorHandle) \
    (Console)->FrontEndIFace.Vtbl->SetMouseCursor(&(Console)->FrontEndIFace, (CursorHandle))
#define TermMenuControl(Console, CmdIdLow, CmdIdHigh) \
    (Console)->FrontEndIFace.Vtbl->MenuControl(&(Console)->FrontEndIFace, (CmdIdLow), (CmdIdHigh))
#define TermSetMenuClose(Console, Enable) \
    (Console)->FrontEndIFace.Vtbl->SetMenuClose(&(Console)->FrontEndIFace, (Enable))

/* EOF */
