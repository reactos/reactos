/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * MinGW-w64 libunwind K32EnumProcessModules support for NT 5.x builds.
 */

#include <windef.h>
#include <winbase.h>
#include <psapi.h>

BOOL
WINAPI
K32EnumProcessModules(HANDLE Process,
                      HMODULE *Modules,
                      DWORD BufferSize,
                      LPDWORD BytesNeeded)
{
    return EnumProcessModules(Process, Modules, BufferSize, BytesNeeded);
}
