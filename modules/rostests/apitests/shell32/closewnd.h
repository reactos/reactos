/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Close windows after tests
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

typedef struct WINDOW_LIST
{
    SIZE_T m_chWnds;
    HWND *m_phWnds;
} WINDOW_LIST, *PWINDOW_LIST;

void GetWindowList(PWINDOW_LIST pList);
void GetWindowListForClose(PWINDOW_LIST pList);
HWND FindNewWindow(PWINDOW_LIST List1, PWINDOW_LIST List2);
void CloseNewWindows(PWINDOW_LIST List1, PWINDOW_LIST List2);
void FreeWindowList(PWINDOW_LIST pList);
