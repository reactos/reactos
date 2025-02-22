/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Close windows after tests
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include "closewnd.h"

void FreeWindowList(PWINDOW_LIST pList)
{
    free(pList->m_phWnds);
    pList->m_phWnds = NULL;
    pList->m_chWnds = 0;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
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

void GetWindowList(PWINDOW_LIST pList)
{
    pList->m_phWnds = NULL;
    pList->m_chWnds = 0;
    EnumWindows(EnumWindowsProc, (LPARAM)pList);
}

void GetWindowListForClose(PWINDOW_LIST pList)
{
    for (UINT tries = 5; tries--;)
    {
        if (tries)
            FreeWindowList(pList);
        GetWindowList(pList);
        Sleep(500);
        WINDOW_LIST list;
        GetWindowList(&list);
        SIZE_T count = list.m_chWnds;
        FreeWindowList(&list);
        if (count == pList->m_chWnds)
            break;
    }
}

static HWND FindInWindowList(const WINDOW_LIST &list, HWND hWnd)
{
    for (SIZE_T i = 0; i < list.m_chWnds; ++i)
    {
        if (list.m_phWnds[i] == hWnd)
            return hWnd;
    }
    return NULL;
}

HWND FindNewWindow(PWINDOW_LIST List1, PWINDOW_LIST List2)
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

static void WaitForForegroundWindow(HWND hWnd, UINT wait = 250)
{
    for (UINT waited = 0, interval = 50; waited < wait; waited += interval)
    {
        if (GetForegroundWindow() == hWnd || !IsWindowVisible(hWnd))
            return;
        Sleep(interval);
    }
}

void CloseNewWindows(PWINDOW_LIST List1, PWINDOW_LIST List2)
{
    for (SIZE_T i = 0; i < List2->m_chWnds; ++i)
    {
        HWND hWnd = List2->m_phWnds[i];
        if (!IsWindow(hWnd) || FindInWindowList(*List1, hWnd))
            continue;

        SwitchToThisWindow(hWnd, TRUE);
        WaitForForegroundWindow(hWnd);

        if (!PostMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0))
        {
            DWORD_PTR result;
            SendMessageTimeoutW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0, 0, 3000, &result);
        }
    }
}
