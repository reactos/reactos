/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Close windows after tests
 * COPYRIGHT:   Copyright 2024-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

typedef struct WINDOW_LIST
{
    SIZE_T m_chWnds;
    HWND *m_phWnds;
} WINDOW_LIST, *PWINDOW_LIST;

static inline VOID FreeWindowList(PWINDOW_LIST pList)
{
    free(pList->m_phWnds);
    pList->m_phWnds = NULL;
    pList->m_chWnds = 0;
}

static inline BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    PWINDOW_LIST pList = (PWINDOW_LIST)lParam;
    SIZE_T cb = (pList->m_chWnds + 1) * sizeof(HWND);
    HWND *phWnds = (HWND *)realloc(pList->m_phWnds, cb);
    if (!phWnds)
        return FALSE;
    phWnds[pList->m_chWnds++] = hwnd;
    pList->m_phWnds = phWnds;
    return TRUE;
}

static inline VOID GetWindowList(PWINDOW_LIST pList)
{
    pList->m_phWnds = NULL;
    pList->m_chWnds = 0;
    EnumWindows(EnumWindowsProc, (LPARAM)pList);
}

static inline HWND FindInWindowList(const WINDOW_LIST &list, HWND hWnd)
{
    for (SIZE_T i = 0; i < list.m_chWnds; ++i)
    {
        if (list.m_phWnds[i] == hWnd)
            return hWnd;
    }
    return NULL;
}

static inline VOID CloseNewWindows(PWINDOW_LIST List1, PWINDOW_LIST List2)
{
    for (SIZE_T i = 0; i < List2->m_chWnds; ++i)
    {
        HWND hWnd = List2->m_phWnds[i];
        if (!IsWindowVisible(hWnd) || FindInWindowList(*List1, hWnd))
            continue;

        if (!PostMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0))
        {
            DWORD_PTR result;
            if (!SendMessageTimeoutW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0, SMTO_ABORTIFHUNG, 3000, &result))
            {
                SwitchToThisWindow(hWnd, TRUE);
                Sleep(500);

                // Alt+F4
                INPUT inputs[4];
                ZeroMemory(&inputs, sizeof(inputs));
                inputs[0].type = inputs[1].type = inputs[2].type = inputs[3].type = INPUT_KEYBOARD;
                inputs[0].ki.wVk = inputs[3].ki.wVk = VK_LMENU;
                inputs[1].ki.wVk = inputs[2].ki.wVk = VK_F4;
                inputs[2].ki.dwFlags = inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(_countof(inputs), inputs, sizeof(INPUT));
                Sleep(1000);
            }
        }
    }
}

static inline HWND FindNewWindow(PWINDOW_LIST List1, PWINDOW_LIST List2)
{
    for (SIZE_T i2 = 0; i2 < List2->m_chWnds; ++i2)
    {
        HWND hWnd = List2->m_phWnds[i2];
        if (!IsWindowEnabled(hWnd) || !IsWindowVisible(hWnd))
            continue;

        BOOL bFoundInList1 = FALSE;
        for (SIZE_T i1 = 0; i1 < List1->m_chWnds; ++i1)
        {
            if (hWnd == List1->m_phWnds[i1])
            {
                bFoundInList1 = TRUE;
                break;
            }
        }

        if (!bFoundInList1)
            return hWnd;
    }
    return NULL;
}

static inline BOOL CALLBACK CountWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    *(INT *)lParam += 1;
    return TRUE;
}

static inline INT GetWindowCount(VOID)
{
    INT nCount = 0;
    EnumWindows(CountWindowsProc, (LPARAM)&nCount);
    return nCount;
}
