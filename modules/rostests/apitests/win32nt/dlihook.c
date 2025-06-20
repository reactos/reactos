/*
 * PROJECT:     ReactOS delayimport Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Delay-import hook for win32u.dll
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <windows.h>
#include <delayimp.h>
#include <stdio.h>
#include <versionhelpers.h>

static
FARPROC
WINAPI
DliNotifyHook(
    unsigned code,
    PDelayLoadInfo pdli)
{
    if (code != dliNotePreLoadLibrary)
    {
        return NULL;
    }

    if (strcmp(pdli->szDll, "win32u.dll") != 0)
    {
        return NULL;
    }

    if (IsReactOS())
    {
        return NULL;
    }

    switch (_winver)
    {
        case _WIN32_WINNT_WINXP:
            pdli->szDll = "win32u_xpsp2.dll";
            break;

        case _WIN32_WINNT_WS03:
            pdli->szDll = "win32u_2k3sp2.dll";
            break;

        case _WIN32_WINNT_VISTA:
            pdli->szDll = "win32u_vista.dll";
            break;

        default:
            break;
    }

    printf("_winver = 0x%x, _osver = 0x%x, loading %s\n", _winver, _osver, pdli->szDll);

    return NULL;
}

PfnDliHook __pfnDliNotifyHook2 = DliNotifyHook;

static
FARPROC
WINAPI
DliFailureHook(
    unsigned code,
    PDelayLoadInfo pdli)
{
    if (code == dliFailLoadLib)
    {
        fprintf(stderr, "Delayimport: Failed to load %s\n", pdli->szDll);
    }
    else if (code == dliFailGetProc)
    {
        fprintf(stderr,
                "Delayimport: Failed to import %s from %s\n",
                pdli->dlp.szProcName,
                pdli->szDll);
    }
    else
    {
        fprintf(stderr, "Delayimport: Unknown error code %d (%s, 0x%lx)\n", code, pdli->szDll, pdli->dwLastError);
    }

    return NULL;
}

PfnDliHook __pfnDliFailureHook2 = DliFailureHook;
