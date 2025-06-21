/*
 * PROJECT:     ReactOS Keyboard Layout Switcher
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IME menu handling
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#include <immdev.h>

#define ID_STARTIMEMENU 1000

struct tagIMEMENUNODE;

typedef struct tagIMEMENUITEM
{
    IMEMENUITEMINFO m_Info;
    UINT m_nRealID;
    struct tagIMEMENUNODE *m_pSubMenu;
} IMEMENUITEM, *PIMEMENUITEM;

typedef struct tagIMEMENUNODE
{
    struct tagIMEMENUNODE *m_pNext;
    INT m_nItems;
    IMEMENUITEM m_Items[ANYSIZE_ARRAY];
} IMEMENUNODE, *PIMEMENUNODE;

PIMEMENUNODE
CreateImeMenu(
    _In_ HIMC hIMC,
    _Inout_opt_ PIMEMENUITEMINFO lpImeParentMenu,
    _In_ BOOL bRightMenu);

HMENU MenuFromImeMenu(_In_ const IMEMENUNODE *pMenu);
INT GetRealImeMenuID(_In_ const IMEMENUNODE *pMenu, _In_ INT nFakeID);
VOID CleanupImeMenus(VOID);
