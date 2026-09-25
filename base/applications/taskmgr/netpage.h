/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Networking Page
 * COPYRIGHT:   Copyright 2026 ReactOS Team
 */

#pragma once

extern HWND hNetworkPage;           /* Networking Property Page */
extern HWND hNetworkPageListCtrl;   /* Network adapters list */

INT_PTR CALLBACK NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void RefreshNetworkPage(void);
void NetworkPage_AppendViewMenu(HMENU hViewMenu);
void NetworkPage_OnViewHistoryOption(UINT idCmd);
