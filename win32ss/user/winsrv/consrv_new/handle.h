/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/handle.h
 * PURPOSE:         Console I/O Handles functions
 * PROGRAMMERS:     David Welch
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID FASTCALL ConSrvInitObject(IN OUT PCONSOLE_IO_OBJECT Object,
                               IN CONSOLE_IO_OBJECT_TYPE Type,
                               IN PCONSOLE Console);
NTSTATUS FASTCALL ConSrvInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     PHANDLE Handle,
                                     PCONSOLE_IO_OBJECT Object,
                                     DWORD Access,
                                     BOOL Inheritable,
                                     DWORD ShareMode);
NTSTATUS FASTCALL ConSrvRemoveObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     HANDLE Handle);
NTSTATUS FASTCALL ConSrvGetObject(PCONSOLE_PROCESS_DATA ProcessData,
                                  HANDLE Handle,
                                  PCONSOLE_IO_OBJECT* Object,
                                  PVOID* Entry OPTIONAL,
                                  DWORD Access,
                                  BOOL LockConsole,
                                  CONSOLE_IO_OBJECT_TYPE Type);
VOID FASTCALL ConSrvReleaseObject(PCONSOLE_IO_OBJECT Object,
                                  BOOL IsConsoleLocked);

/* EOF */
