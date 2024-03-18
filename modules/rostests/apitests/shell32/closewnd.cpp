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

#define TRIALS_COUNT 8

void CloseNewWindows(PWINDOW_LIST List1, PWINDOW_LIST List2)
{
    INT cDiff = List2->m_chWnds - List1->m_chWnds;
    for (INT j = 0; j < cDiff; ++j)
    {
        HWND hWnd = FindNewWindow(List1, List2);
        if (!hWnd)
            break;

        for (INT i = 0; i < TRIALS_COUNT; ++i)
        {
            if (!IsWindow(hWnd))
                break;

            SwitchToThisWindow(hWnd, TRUE);

            // Alt+F4
            keybd_event(VK_MENU, 0x38, 0, 0);
            keybd_event(VK_F4, 0x3E, 0, 0);
            keybd_event(VK_F4, 0x3E, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_MENU, 0x38, KEYEVENTF_KEYUP, 0);
            Sleep(100);
        }
    }
}
