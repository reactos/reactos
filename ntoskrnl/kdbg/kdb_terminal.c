/*
 * PROJECT:     ReactOS KDBG Kernel Debugger Terminal Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Terminal Management for the Kernel Debugger
 * COPYRIGHT:   Copyright 2005 Gregor Anich <blight@blight.eu.org>
 *              Copyright 2022-2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

// #include <ntoskrnl.h>
#include "kdb_terminal.h"

#define KdbpGetCharKeyboard(ScanCode) KdbpTryGetCharKeyboard((ScanCode), 0)
CHAR
KdbpTryGetCharKeyboard(PULONG ScanCode, ULONG Retry);

#define KdbpGetCharSerial()  KdbpTryGetCharSerial(0)
CHAR
KdbpTryGetCharSerial(ULONG Retry);


/* FUNCTIONS *****************************************************************/

BOOLEAN
KdpInitTerminal(
    _Inout_ PKD_TERMINAL KdTerm)
{
    ULONG Length;

    /* Enable line-wrap */
    KdpDprintf("\x1b[?7h");

    /*
     * Query terminal type.
     * Historically it was done with CTRL-E ('\x05'), however nowadays
     * terminals respond to it with an empty (or a user-configurable)
     * string. Instead, use the VT52-compatible 'ESC Z' sequence or the
     * VT100-compatible 'ESC[c' one.
     */
    KdpDprintf("\x1b[c");
    KeStallExecutionProcessor(100000);

    Length = 0;
    for (;;)
    {
        /* Verify we get an answer, but don't care about it */
        if (KdbpTryGetCharSerial(5000) == -1)
            break;
        ++Length;
    }

    /* Determine whether the controlling terminal is a serial terminal:
     * serial output is enabled *and* KDSERIAL is set (i.e. user input
     * through serial). */
    KdTerm->Serial =
#if 0
    // Old logic where KDSERIAL also enables serial output.
    (KdbDebugState & KD_DEBUG_KDSERIAL) ||
    (KdpDebugMode.Serial && !KdpDebugMode.Screen);
#else
    // New logic where KDSERIAL does not necessarily enable serial output.
    KdpDebugMode.Serial &&
    ((KdbDebugState & KD_DEBUG_KDSERIAL) || !KdpDebugMode.Screen);
#endif

    /* Terminal is connected (TRUE) or not connected (FALSE) */
    KdTerm->Connected = (Length > 0);
    return KdTerm->Connected; // NOTE: Old code kind of returned "the last status".
}

BOOLEAN
KdpGetTerminalSize(
    _Inout_ PKD_TERMINAL KdTerm,
    _Out_ PLONG Rows,
    _Out_ PLONG Cols)
{
    static CHAR Buffer[128];
    CHAR c;
    LONG NumberOfRows = -1; // Or initialize to *Rows ??
    LONG NumberOfCols = -1; // Or initialize to *Cols ??

    /* Retrieve the size of the controlling terminal only when it is serial */
    if (KdTerm->Serial && KdTerm->Connected && KdTerm->ReportsSize)
    {
        /* Try to query number of rows from terminal. A reply looks like "\x1b[8;24;80t" */
        KdTerm->ReportsSize = FALSE;
        KdpDprintf("\x1b[18t");
        KeStallExecutionProcessor(100000);

        c = KdbpTryGetCharSerial(5000);
        if (c == KEY_ESC)
        {
            c = KdbpTryGetCharSerial(5000);
            if (c == '[')
            {
                ULONG Length = 0;
                for (;;)
                {
                    c = KdbpTryGetCharSerial(5000);
                    if (c == -1)
                        break;

                    Buffer[Length++] = c;
                    if (isalpha(c) || Length >= (sizeof(Buffer) - 1))
                        break;
                }
                Buffer[Length] = '\0';

                if (Buffer[0] == '8' && Buffer[1] == ';')
                {
                    SIZE_T i;
                    for (i = 2; (i < Length) && (Buffer[i] != ';'); i++);

                    if (Buffer[i] == ';')
                    {
                        Buffer[i++] = '\0';

                        /* Number of rows is now at Buffer + 2 and number of cols at Buffer + i */
                        NumberOfRows = strtoul(Buffer + 2, NULL, 0);
                        NumberOfCols = strtoul(Buffer + i, NULL, 0);
                        KdTerm->ReportsSize = TRUE;
                    }
                }
            }
            /* Clear further characters */
            while ((c = KdbpTryGetCharSerial(5000)) != -1);
        }
    }

    if (NumberOfRows <= 0)
    {
        /* Set number of rows to the default */
        if (KdpDebugMode.Screen && !KdTerm->Serial)
            NumberOfRows = (SCREEN_HEIGHT / (13 /*BOOTCHAR_HEIGHT*/ + 1));
        else
            NumberOfRows = 24;
    }
    if (NumberOfCols <= 0)
    {
        /* Set number of cols to the default */
        if (KdpDebugMode.Screen && !KdTerm->Serial)
            NumberOfCols = (SCREEN_WIDTH / 8 /*BOOTCHAR_WIDTH*/);
        else
            NumberOfCols = 80;
    }

    *Rows = NumberOfRows;
    *Cols = NumberOfCols;
    return KdTerm->ReportsSize;
}

VOID
KdpFlushTerminalInput(VOID)
{
    if (KdbDebugState & KD_DEBUG_KDSERIAL)
    {
        while (KdbpTryGetCharSerial(1) != -1);
    }
    else
    {
        ULONG ScanCode;
        while (KdbpTryGetCharKeyboard(&ScanCode, 1) != -1);
    }
}

/**
 * @brief
 * "Simply" reads one character from the terminal. This function will
 * not return a scan code if reading is done from a serial terminal.
 **/
CHAR
KdpSimpleReadTerminal(
    _Out_ PULONG ScanCode)
{
    CHAR c;

    *ScanCode = 0;

    if (KdbDebugState & KD_DEBUG_KDSERIAL)
        c = KdbpGetCharSerial();
    else
        c = KdbpGetCharKeyboard(ScanCode);

    if (c == '\r')
    {
        /* Try to read '\n' which might follow '\r' - if \n is not received here
         * it will be interpreted as "return" when the next command should be read.
         */
        if (KdbDebugState & KD_DEBUG_KDSERIAL)
            c = KdbpTryGetCharSerial(5);
        else
            c = KdbpTryGetCharKeyboard(ScanCode, 5);
    }

    return c;
}

/**
 * @brief
 * Reads one character from the terminal. This function will return
 * a scan code even when reading is done from a serial terminal.
 **/
CHAR
KdpReadTerminal(
    _Out_ PULONG ScanCode,
    _Inout_ PCHAR pNextKey)
{
    CHAR NextKey = *pNextKey;
    CHAR Key;

    // do {

    if (KdbDebugState & KD_DEBUG_KDSERIAL)
    {
        Key = (NextKey == '\0') ? KdbpGetCharSerial() : NextKey;
        NextKey = '\0';
        *ScanCode = 0;
        if (Key == KEY_ESC) /* ESC */
        {
            Key = KdbpGetCharSerial();
            if (Key == '[')
            {
                Key = KdbpGetCharSerial();
                switch (Key)
                {
                    case 'A':
                        *ScanCode = KEY_SCAN_UP;
                        break;
                    case 'B':
                        *ScanCode = KEY_SCAN_DOWN;
                        break;
                    case 'C':
                        break;
                    case 'D':
                        break;
                }
            }
        }
    }
    else
    {
        *ScanCode = 0;
        Key = (NextKey == '\0') ? KdbpGetCharKeyboard(ScanCode) : NextKey;
        NextKey = '\0';
    }

    /* Check for return */
    if (Key == '\r')
    {
        /*
         * We might need to discard the next '\n' which most clients
         * should send after \r. Wait a bit to make sure we receive it.
         */
        KeStallExecutionProcessor(100000);

        if (KdbDebugState & KD_DEBUG_KDSERIAL)
            NextKey = KdbpTryGetCharSerial(5);
        else
            NextKey = KdbpTryGetCharKeyboard(ScanCode, 5);

        if (NextKey == '\n' || NextKey == -1) /* \n or no response at all */
            NextKey = '\0';
    }

    // } while (!*NextKey);

    *pNextKey = NextKey;
    return Key;
}

/* EOF */
