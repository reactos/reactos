/*
 *  ReactOS Task Manager
 *
 *  priority.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

TCHAR                szTemp[256];
TCHAR                szTempA[256];

void ProcessPage_OnSetPriorityRealTime(void)
{
    LVITEM            lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE            hProcess;
    TCHAR            strErrorText[260];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadString(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szTemp, 256);
    LoadString(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);
    if (MessageBox(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS))
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnSetPriorityHigh(void)
{
    LVITEM            lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE            hProcess;
    TCHAR            strErrorText[260];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadString(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szTemp, 256);
    LoadString(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);
    if (MessageBox(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnSetPriorityAboveNormal(void)
{
    LVITEM            lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE            hProcess;
    TCHAR            strErrorText[260];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadString(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szTemp, 256);
    LoadString(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);
    if (MessageBox(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS))
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnSetPriorityNormal(void)
{
    LVITEM            lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE            hProcess;
    TCHAR            strErrorText[260];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadString(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szTemp, 256);
    LoadString(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);
    if (MessageBox(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS))
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnSetPriorityBelowNormal(void)
{
    LVITEM            lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE            hProcess;
    TCHAR            strErrorText[260];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadString(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szTemp, 256);
    LoadString(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);
    if (MessageBox(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS))
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnSetPriorityLow(void)
{
    LVITEM            lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE            hProcess;
    TCHAR            strErrorText[260];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        memset(&lvitem, 0, sizeof(LVITEM));

        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;

        (void)ListView_GetItem(hProcessPageListCtrl, &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    LoadString(hInst, IDS_MSG_WARNINGCHANGEPRIORITY, szTemp, 256);
    LoadString(hInst, IDS_MSG_TASKMGRWARNING, szTempA, 256);
    if (MessageBox(hMainWnd, szTemp, szTempA, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS))
    {
        GetLastErrorText(strErrorText, 260);
        LoadString(hInst, IDS_MSG_UNABLECHANGEPRIORITY, szTemp, 256);
        MessageBox(hMainWnd, strErrorText, szTemp, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}
