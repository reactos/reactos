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
    BOOL bDestroyTarget;
} GHOST_DATA;

// GWM_UNGHOST message:
//   wParam: BOOL bDestroyTarget.
//   lParam: VOID Unused.
#define GWM_UNGHOST     (WM_USER + 1)

#endif
