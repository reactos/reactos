/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Misc Stuff
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

UINT ViewModeToCmdId(LISTVIEW_MODE ListViewMode);
LISTVIEW_MODE CmdIdToViewMode(UINT CmdId);
LPWSTR ViewModeToString(LISTVIEW_MODE ListViewMode);
LISTVIEW_MODE StringToViewMode(LPWSTR ViewMode);
DWORD ViewModeToStyle(LISTVIEW_MODE ListViewMode);
