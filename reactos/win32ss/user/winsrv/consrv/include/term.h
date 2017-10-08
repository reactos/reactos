/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/include/term.h
 * PURPOSE:         Internal Frontend Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* Macros used to call functions in the TERMINAL_VTBL virtual table */

#define TermReadStream(Console, /**/ Unicode, /**/ Buffer, ReadControl, Parameter, NumCharsToRead, NumCharsRead) \
    (Console)->TermIFace.Vtbl->ReadStream(&(Console)->TermIFace, /**/ (Unicode), /**/ \
                                           (Buffer), (ReadControl), (Parameter), (NumCharsToRead), (NumCharsRead))

#define TermWriteStream(Console, ScreenBuffer, Buffer, Length, Attrib) \
    (Console)->TermIFace.Vtbl->WriteStream(&(Console)->TermIFace, (ScreenBuffer), (Buffer), \
                                           (Length), (Attrib))


#define TermDrawRegion(Console, Region) \
    (Console)->TermIFace.Vtbl->DrawRegion(&(Console)->TermIFace, (Region))
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
#define TermGetLargestConsoleWindowSize(Console, pSize) \
    (Console)->TermIFace.Vtbl->GetLargestConsoleWindowSize(&(Console)->TermIFace, (pSize))
#define TermSetPalette(Console, PaletteHandle, PaletteUsage) \
    (Console)->TermIFace.Vtbl->SetPalette(&(Console)->TermIFace, (PaletteHandle), (PaletteUsage))
#define TermShowMouseCursor(Console, Show) \
    (Console)->TermIFace.Vtbl->ShowMouseCursor(&(Console)->TermIFace, (Show))


/* Macros used to call functions in the FRONTEND_VTBL virtual table */

#define TermRefreshInternalInfo(Console) \
    (Console)->FrontEndIFace.Vtbl->RefreshInternalInfo(&(Console)->FrontEndIFace)
#define TermChangeTitle(Console) \
    (Console)->FrontEndIFace.Vtbl->ChangeTitle(&(Console)->FrontEndIFace)
#define TermChangeIcon(Console, IconHandle) \
    (Console)->FrontEndIFace.Vtbl->ChangeIcon(&(Console)->FrontEndIFace, (IconHandle))
#define TermGetConsoleWindowHandle(Console) \
    (Console)->FrontEndIFace.Vtbl->GetConsoleWindowHandle(&(Console)->FrontEndIFace)
#define TermGetSelectionInfo(Console, pSelectionInfo) \
    (Console)->FrontEndIFace.Vtbl->GetSelectionInfo(&(Console)->FrontEndIFace, (pSelectionInfo))
#define TermGetDisplayMode(Console) \
    (Console)->FrontEndIFace.Vtbl->GetDisplayMode(&(Console)->FrontEndIFace)
#define TermSetDisplayMode(Console, NewMode) \
    (Console)->FrontEndIFace.Vtbl->SetDisplayMode(&(Console)->FrontEndIFace, (NewMode))
#define TermSetMouseCursor(Console, CursorHandle) \
    (Console)->FrontEndIFace.Vtbl->SetMouseCursor(&(Console)->FrontEndIFace, (CursorHandle))
#define TermMenuControl(Console, CmdIdLow, CmdIdHigh) \
    (Console)->FrontEndIFace.Vtbl->MenuControl(&(Console)->FrontEndIFace, (CmdIdLow), (CmdIdHigh))
#define TermSetMenuClose(Console, Enable) \
    (Console)->FrontEndIFace.Vtbl->SetMenuClose(&(Console)->FrontEndIFace, (Enable))

/* EOF */
