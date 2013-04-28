/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/conoutput.h
 * PURPOSE:         Console Output functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#define ConSrvGetScreenBuffer(ProcessData, Handle, Ptr, Access, LockConsole)    \
    ConSrvGetObject((ProcessData), (Handle), (PCONSOLE_IO_OBJECT*)(Ptr), NULL,  \
                    (Access), (LockConsole), SCREEN_BUFFER)
#define ConSrvGetScreenBufferAndHandleEntry(ProcessData, Handle, Ptr, Entry, Access, LockConsole)   \
    ConSrvGetObject((ProcessData), (Handle), (PCONSOLE_IO_OBJECT*)(Ptr), (Entry),                   \
                    (Access), (LockConsole), SCREEN_BUFFER)
#define ConSrvReleaseScreenBuffer(Buff, IsConsoleLocked)    \
    ConSrvReleaseObject(&(Buff)->Header, (IsConsoleLocked))

NTSTATUS FASTCALL ConSrvCreateScreenBuffer(IN OUT PCONSOLE Console,
                                           OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                                           IN COORD ScreenBufferSize,
                                           IN USHORT ScreenAttrib,
                                           IN USHORT PopupAttrib,
                                           IN ULONG DisplayMode,
                                           IN BOOLEAN IsCursorVisible,
                                           IN ULONG CursorSize);
VOID WINAPI ConioDeleteScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer);

/* EOF */
