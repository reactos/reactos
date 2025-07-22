/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Close windows after tests
 * COPYRIGHT:   Copyright 2024-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

#define WaitForWindow(hWnd, Func, Seconds) \
    for (UINT waited = 0; !Func(hWnd) && waited < (Seconds) * 1000; waited += 250) Sleep(250);

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

static inline VOID CloseNewWindows(PWINDOW_LIST pExisting, PWINDOW_LIST pNew)
{
    for (SIZE_T i = 0; i < pNew->m_chWnds; ++i)
    {
        HWND hWnd = pNew->m_phWnds[i];
        if (!IsWindowVisible(hWnd) || FindInWindowList(*pExisting, hWnd))
            continue;

        if (!SendMessageTimeoutW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 3000, NULL))
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
        /* If this window is still open, you'll need TerminateProcess(). */
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
