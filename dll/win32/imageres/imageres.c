/*
 * PROJECT:     imageres
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Image Resource
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define COBJMACROS
#define WIN32_NO_STATUS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "resource.h"

BOOL WINAPI
DllMainCRTStartup(HANDLE hinstDll, DWORD fdwReason, LPVOID fImpLoad)
{
    return TRUE;
}
