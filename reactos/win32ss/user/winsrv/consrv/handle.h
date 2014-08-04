/*
 * LICENSE:         GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/handle.h
 * PURPOSE:         Console I/O Handles functions
 * PROGRAMMERS:     David Welch
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

VOID ConSrvInitObject(IN OUT PCONSOLE_IO_OBJECT Object,
                               IN CONSOLE_IO_OBJECT_TYPE Type,
                               IN PCONSOLE Console);
NTSTATUS ConSrvInsertObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     PHANDLE Handle,
                                     PCONSOLE_IO_OBJECT Object,
                                     DWORD Access,
                                     BOOL Inheritable,
                                     DWORD ShareMode);
NTSTATUS ConSrvRemoveObject(PCONSOLE_PROCESS_DATA ProcessData,
                                     HANDLE Handle);
NTSTATUS ConSrvGetObject(PCONSOLE_PROCESS_DATA ProcessData,
                                  HANDLE Handle,
                                  PCONSOLE_IO_OBJECT* Object,
                                  PVOID* Entry OPTIONAL,
                                  DWORD Access,
                                  BOOL LockConsole,
                                  CONSOLE_IO_OBJECT_TYPE Type);
VOID ConSrvReleaseObject(PCONSOLE_IO_OBJECT Object,
                                  BOOL IsConsoleLocked);
