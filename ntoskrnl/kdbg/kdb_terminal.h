/*
 * PROJECT:     ReactOS KDBG Kernel Debugger Terminal Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Terminal Management for the Kernel Debugger
 * COPYRIGHT:   Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#define KEY_BS          8
#define KEY_ESC         27
#define KEY_DEL         127

#define KEY_SCAN_UP     72
#define KEY_SCAN_DOWN   80

/* Scan codes of keyboard keys */
#define KEYSC_END       0x004f
#define KEYSC_PAGEUP    0x0049
#define KEYSC_PAGEDOWN  0x0051
#define KEYSC_HOME      0x0047
#define KEYSC_ARROWUP   0x0048  // == KEY_SCAN_UP


/* Characteristics of the controlling terminal */
typedef struct _KD_TERMINAL
{
    LONG NumberOfRows;
    LONG NumberOfCols;
    BOOLEAN Connected;
    union
    {
        UCHAR Flags;
        struct
        {
            UCHAR Serial : 1;
            UCHAR ReportsSize : 1;
        };
    };
} KD_TERMINAL, *PKD_TERMINAL;


BOOLEAN
KdpInitTerminal(
    _Inout_ PKD_TERMINAL KdTerm);

BOOLEAN
KdpGetTerminalSize(
    _Inout_ PKD_TERMINAL KdTerm,
    _Out_ PLONG Rows,
    _Out_ PLONG Cols);

VOID
KdpFlushTerminalInput(VOID);

CHAR
KdpSimpleReadTerminal(
    _Out_ PULONG ScanCode);

CHAR
KdpReadTerminal(
    _Out_ PULONG ScanCode,
    _Inout_ PCHAR pNextKey);

/* EOF */
