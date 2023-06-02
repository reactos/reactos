/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Console message IDs
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifndef WM_USER
    #define WM_USER 0x400
#endif
#ifndef WM_APP
    #define WM_APP 0x8000
#endif

/* Console messages */
#define PM_RESIZE_TERMINAL    (WM_APP  + 3)
#define PM_CONSOLE_BEEP       (WM_USER + 3)
#define PM_CONSOLE_SET_TITLE  (WM_USER + 5)
#define PM_MINIMIZE           (WM_USER + 8)
#define PM_SET_HKL            (WM_USER + 15)
