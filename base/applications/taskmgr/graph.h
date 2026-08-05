/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Performance Graph Meters
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2022 Hermès Bélusca-Maïto
 */

#pragma once

#define BRIGHT_GREEN    RGB(0, 255, 0)
#define MEDIUM_GREEN    RGB(0, 190, 0)
#define DARK_GREEN      RGB(0, 130, 0)
#define RED             RGB(255, 0, 0)

typedef struct _TM_GAUGE_CONTROL
{
    // HWND hParentWnd;
    // HWND hWnd;
    ULONG Current1; // Usage
    ULONG Current2;
    ULONG Maximum; // Limit
    BOOL  DrawSecondaryLevel;

    INT nlastBarsUsed;

    /*** TEMP! ***/
    BOOL bIsCPU; // TRUE: Gauge for CPU; FALSE: Gauge for Memory
} TM_GAUGE_CONTROL, *PTM_GAUGE_CONTROL;

/* Snapshot of instantaneous values */
extern ULONGLONG Meter_CommitChargeTotal;
extern ULONGLONG Meter_CommitChargeLimit;

INT_PTR CALLBACK
Graph_DrawUsageGraph(HWND hWnd, PTM_GAUGE_CONTROL gauge, WPARAM wParam, LPARAM lParam);
