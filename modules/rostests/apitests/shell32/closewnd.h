/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Close windows after tests
 * COPYRIGHT:   Copyright 2024-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#define WaitForWindow(hWnd, Func, Seconds) \
    for (UINT waited = 0; !Func(hWnd) && waited < (Seconds) * 1000; waited += 250) Sleep(250);

static BOOL WaitForForegroundWindow(HWND hWnd, UINT wait = 500)
{
    for (UINT waited = 0, interval = 50; waited < wait; waited += interval)
    {
        if (GetForegroundWindow() == hWnd)
            return TRUE;
        if (IsWindowVisible(hWnd))
            Sleep(interval);
    }
    return FALSE;
}

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

static inline VOID GetWindowListForClose(PWINDOW_LIST pList)
{
    WINDOW_LIST list;
    pList->m_phWnds = NULL;
    pList->m_chWnds = 0;
    for (SIZE_T tries = 5, count; tries--;)
    {
        if (tries)
            FreeWindowList(pList);
        GetWindowList(pList);
        Sleep(250);
        GetWindowList(&list);
        count = list.m_chWnds;
        FreeWindowList(&list);
        if (count == pList->m_chWnds)
            break;
    }
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

static inline BOOL SendAltF4Input()
{
    INPUT inputs[4];
    ZeroMemory(&inputs, sizeof(inputs));
    inputs[0].type = inputs[1].type = inputs[2].type = inputs[3].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = inputs[3].ki.wVk = VK_LMENU;
    inputs[1].ki.wVk = inputs[2].ki.wVk = VK_F4;
    inputs[2].ki.dwFlags = inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(_countof(inputs), inputs, sizeof(INPUT)) == _countof(inputs);
}

static inline VOID CloseNewWindows(PWINDOW_LIST pExisting, PWINDOW_LIST pNew)
{
    for (SIZE_T i = 0; i < pNew->m_chWnds; ++i)
    {
        HWND hWnd = pNew->m_phWnds[i];
        if (!IsWindowVisible(hWnd) || FindInWindowList(*pExisting, hWnd))
            continue;

        SwitchToThisWindow(hWnd, TRUE);
        WaitForForegroundWindow(hWnd); // SetForegroundWindow may take some time
        DWORD_PTR result;
        if (!SendMessageTimeoutW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0, SMTO_ABORTIFHUNG, 3000, &result) &&
            !PostMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0))
        {
            if (WaitForForegroundWindow(hWnd)) // We can't fake keyboard input if the target is not foreground
            {
                SendAltF4Input();
                WaitForWindow(hWnd, IsWindowVisible, 1); // Closing a window may take some time
            }

            if (IsWindowVisible(hWnd))
            {
                CHAR szClass[64];
                GetClassNameA(hWnd, szClass, _countof(szClass));
                trace("Unable to close window %p (%s)\n", hWnd, szClass);
            }
        }
    }
}

#ifdef __cplusplus
static inline VOID CloseNewWindows(PWINDOW_LIST InitialList)
{
    WINDOW_LIST newwindows;
    GetWindowListForClose(&newwindows);
    CloseNewWindows(InitialList, &newwindows);
    FreeWindowList(&newwindows);
}
#endif

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

static inline BOOL CALLBACK CountVisibleWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    *(PINT)lParam += 1;
    return TRUE;
}

static inline INT GetWindowCount(VOID)
{
    INT nCount = 0;
    EnumWindows(CountVisibleWindowsProc, (LPARAM)&nCount);
    return nCount;
}
