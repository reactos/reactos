/*
 * PROJECT:     ReactOS header
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Ghost window
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#ifndef REACTOS_GHOST_WND_INCLUDED
#define REACTOS_GHOST_WND_INCLUDED

#define GHOSTCLASSNAME L"Ghost"
#define GHOST_PROP L"GhostProp"

typedef struct GHOST_DATA
{
    HWND hwndTarget;
    HBITMAP hbm32bpp;
    DWORD style;
    DWORD exstyle;
    RECT rcWindow;
} GHOST_DATA;

/*-------------------------------------------------------------------------*/
/* Ghost window messages (GWM_*): */

// GWM_UNGHOST message:
//   wParam: BOOL bDestroyTarget: Whether the target is to be destroyed.
//   lParam: Ignored.
#define GWM_UNGHOST             (WM_USER + 100)

/*-------------------------------------------------------------------------*/
/* Ghost thread messages (GTM_*): */

// GTM_CREATE_GHOST message:
//   wParam: HWND hwndTarget: The target window.
//   lParam: VOID Ignored: Ignored.
#define GTM_CREATE_GHOST        (WM_USER + 1)

// GTM_GHOST_DESTROYED message:
//   wParam: HWND hwndTarget: The target window.
//   lParam: HWND hwndGhost: The ghost window.
#define GTM_GHOST_DESTROYED     (WM_USER + 2)

#endif
