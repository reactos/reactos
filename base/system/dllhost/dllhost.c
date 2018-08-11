/*
 * PROJECT:     ReactOS DllHost
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     COM surrogate.
 * COPYRIGHT:   Copyright 2018 Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

INT
WINAPI
wWinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPWSTR lpCmdLine,
    INT nCmdShow)
{
    DPRINT1("dllhost: %S\n", lpCmdLine);

    return 0;
}

/* EOF */
