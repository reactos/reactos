/*
 * PROJECT:     ReactOS Shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     SHAppBarMessage implementation
 * COPYRIGHT:   Copyright 2008 Vincent Povirk for CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <undocshell.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(appbar);

/* For internal AppBar messaging (private) */
typedef struct _APPBAR_COMMAND
{
    APPBARDATA data;
    DWORD dwMessage;
    DWORD dwProcessId;
    HANDLE hOutput;
} APPBAR_COMMAND, PAPPBAR_COMMAND;

static LPVOID
AppBar_CopyIn(
    _In_ LPCVOID pvSrc,
    _In_ SIZE_T dwSize,
    _In_ DWORD dwProcessId)
{
    HANDLE hMem = SHAllocShared(NULL, dwSize, dwProcessId);
    if (!hMem)
        return NULL;

    LPVOID pvDest = SHLockShared(hMem, dwProcessId);
    if (!pvDest)
    {
        SHFreeShared(hMem, dwProcessId);
        return NULL;
    }

    CopyMemory(pvDest, pvSrc, dwSize);
    SHUnlockShared(pvDest);
    return hMem;
}

static BOOL
AppBar_CopyOut(
    _In_ HANDLE hOutput,
    _Out_ LPVOID pvDest,
    _In_ SIZE_T cbDest,
    _In_ DWORD dwProcessId)
{
    LPVOID pvSrc = SHLockShared(hOutput, dwProcessId);
    if (pvSrc)
    {
        CopyMemory(pvDest, pvSrc, cbDest);
        SHUnlockShared(pvSrc);
    }

    SHFreeShared(hOutput, dwProcessId);
    return pvSrc != NULL;
}

/*************************************************************************
 * SHAppBarMessage            [SHELL32.@]
 */
UINT_PTR
WINAPI
SHAppBarMessage(
    _In_ DWORD dwMessage,
    _Inout_ PAPPBARDATA pData)
{
    TRACE("dwMessage=%d, pData={cb=%d, hwnd=%p}\n", dwMessage, pData->cbSize, pData->hWnd);

    HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (!hTrayWnd || pData->cbSize > sizeof(APPBARDATA))
    {
        WARN("%p, %d\n", hTrayWnd, pData->cbSize);
        return FALSE;
    }

    APPBAR_COMMAND cmd;
    cmd.data = *pData;
    cmd.data.cbSize = 0xBEEFCAFE; // For security check
    cmd.dwMessage = dwMessage;
    cmd.dwProcessId = GetCurrentProcessId();
    cmd.hOutput = NULL;

    /* Make output data if necessary */
    switch (dwMessage)
    {
        case ABM_QUERYPOS:
        case ABM_SETPOS:
        case ABM_GETTASKBARPOS:
            cmd.hOutput = AppBar_CopyIn(&cmd.data, sizeof(cmd.data), cmd.dwProcessId);
            if (!cmd.hOutput)
            {
                ERR("AppBar_CopyIn: %d\n", dwMessage);
                return FALSE;
            }
            break;
        default:
            break;
    }

    /* Send WM_COPYDATA message */
    COPYDATASTRUCT copyData = { TABDMC_APPBAR, sizeof(cmd), &cmd };
    UINT_PTR ret = SendMessageW(hTrayWnd, WM_COPYDATA, (WPARAM)pData->hWnd, (LPARAM)&copyData);

    /* Apply output data */
    if (cmd.hOutput)
    {
        if (!AppBar_CopyOut(cmd.hOutput, &cmd.data, sizeof(cmd.data), cmd.dwProcessId))
        {
            ERR("AppBar_CopyOut: %d\n", dwMessage);
            return FALSE;
        }

        pData->hWnd             = cmd.data.hWnd;
        pData->uCallbackMessage = cmd.data.uCallbackMessage;
        pData->uEdge            = cmd.data.uEdge;
        pData->rc               = cmd.data.rc;
        pData->lParam           = cmd.data.lParam;
    }

    return ret;
}
