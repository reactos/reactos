/*
 * PROJECT:     ReactOS KDBG Kernel Debugger Terminal Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KD Terminal Management
 * COPYRIGHT:   Copyright 2005 Gregor Anich <blight@blight.eu.org>
 *              Copyright 2022-2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "kd.h"
#include "kdterminal.h"

#define KdbpGetCharKeyboard(ScanCode) KdbpTryGetCharKeyboard((ScanCode), 0)
CHAR
KdbpTryGetCharKeyboard(PULONG ScanCode, ULONG Retry);

#define KdbpGetCharSerial()  KdbpTryGetCharSerial(0)
CHAR
KdbpTryGetCharSerial(
    _In_ ULONG Retry);

VOID
KdbpSendCommandSerial(
    _In_ PCSTR Command);


/* GLOBALS *******************************************************************/

/* KD Controlling Terminal */
ULONG KdbDebugState = 0; /* KDBG Settings (NOECHO, KDSERIAL) */
SIZE KdTermSize = {0,0};
BOOLEAN KdTermConnected = FALSE;
BOOLEAN KdTermSerial = FALSE;
BOOLEAN KdTermReportsSize = TRUE;

static CHAR KdTermNextKey = ANSI_NULL; /* 1-character input queue buffer */


/* FUNCTIONS *****************************************************************/

/**
 * @brief   Initializes the controlling terminal.
 *
 * @return
 * TRUE if the controlling terminal is serial and detected
 * as being connected, or FALSE if not.
 **/
BOOLEAN
KdpInitTerminal(VOID)
{
    /* Determine whether the controlling terminal is a serial terminal:
     * serial output is enabled *and* KDSERIAL is set (i.e. user input
     * through serial). */
    KdTermSerial =
#if 0
    // Old logic where KDSERIAL also enables serial output.
    (KdbDebugState & KD_DEBUG_KDSERIAL) ||
    (KdpDebugMode.Serial && !KdpDebugMode.Screen);
#else
    // New logic where KDSERIAL does not necessarily enable serial output.
    KdpDebugMode.Serial &&
    ((KdbDebugState & KD_DEBUG_KDSERIAL) || !KdpDebugMode.Screen);
#endif

    /* Flush the input buffer */
    KdpFlushTerminalInput();

    if (KdTermSerial)
    {
        ULONG Length;

        /* Enable line-wrap */
        KdbpSendCommandSerial("\x1b[?7h");

        /*
         * Query terminal type.
         * Historically it was done with CTRL-E ('\x05'), however nowadays
         * terminals respond to it with an empty (or a user-configurable)
         * string. Instead, use the VT52-compatible 'ESC Z' sequence or the
         * VT100-compatible 'ESC[c' one.
         */
        KdbpSendCommandSerial("\x1b[c");
        KeStallExecutionProcessor(100000);

        Length = 0;
        for (;;)
        {
            /* Verify we get an answer, but don't care about it */
            if (KdbpTryGetCharSerial(5000) == -1)
                break;
            ++Length;
        }

        /* Terminal is connected (TRUE) or not connected (FALSE) */
        KdTermConnected = (Length > 0);
    }
    else
    {
        /* Terminal is not serial, assume it's *not* connected */
        KdTermConnected = FALSE;
    }
    return KdTermConnected;
}

BOOLEAN
KdpUpdateTerminalSize(
    _Out_ PSIZE TermSize)
{
    static CHAR Buffer[128];
    CHAR c;
    LONG NumberOfCols = -1; // Or initialize to TermSize->cx ??
    LONG NumberOfRows = -1; // Or initialize to TermSize->cy ??

    /* Retrieve the size of the controlling terminal only when it is serial */
    if (KdTermConnected && KdTermSerial && KdTermReportsSize)
    {
        /* Flush the input buffer */
        KdpFlushTerminalInput();

        /* Try to query the terminal size. A reply looks like "\x1b[8;24;80t" */
        KdTermReportsSize = FALSE;
        KdbpSendCommandSerial("\x1b[18t");
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
                Buffer[Length] = ANSI_NULL;

                if (Buffer[0] == '8' && Buffer[1] == ';')
                {
                    SIZE_T i;
                    for (i = 2; (i < Length) && (Buffer[i] != ';'); i++);

                    if (Buffer[i] == ';')
                    {
                        Buffer[i++] = ANSI_NULL;

                        /* Number of rows is now at Buffer + 2
                         * and number of columns at Buffer + i */
                        NumberOfRows = strtoul(Buffer + 2, NULL, 0);
                        NumberOfCols = strtoul(Buffer + i, NULL, 0);
                        KdTermReportsSize = TRUE;
                    }
                }
            }
            /* Clear further characters */
            while (KdbpTryGetCharSerial(5000) != -1);
        }
    }

    if (NumberOfCols <= 0)
    {
        /* Set the number of columns to the default */
        if (KdpDebugMode.Screen && !KdTermSerial)
            NumberOfCols = (SCREEN_WIDTH / 8 /*BOOTCHAR_WIDTH*/);
        else
            NumberOfCols = 80;
    }
    if (NumberOfRows <= 0)
    {
        /* Set the number of rows to the default */
        if (KdpDebugMode.Screen && !KdTermSerial)
            NumberOfRows = (SCREEN_HEIGHT / (13 /*BOOTCHAR_HEIGHT*/ + 1));
        else
            NumberOfRows = 24;
    }

    TermSize->cx = NumberOfCols;
    TermSize->cy = NumberOfRows;

    // KdIoPrintf("Cols/Rows: %dx%d\n", TermSize->cx, TermSize->cy);

    return KdTermReportsSize;
}

/**
 * @brief   Flushes terminal input (either serial or PS/2).
 **/
VOID
KdpFlushTerminalInput(VOID)
{
    KdTermNextKey = ANSI_NULL;
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
 * Reads one character from the terminal. This function returns
 * a scan code even when reading is done from a serial terminal.
 **/
CHAR
KdpReadTermKey(
    _Out_ PULONG ScanCode)
{
    CHAR Key;

    *ScanCode = 0;

    if (KdbDebugState & KD_DEBUG_KDSERIAL)
    {
        Key = (!KdTermNextKey ? KdbpGetCharSerial() : KdTermNextKey);
        KdTermNextKey = ANSI_NULL;
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
        Key = (!KdTermNextKey ? KdbpGetCharKeyboard(ScanCode) : KdTermNextKey);
        KdTermNextKey = ANSI_NULL;
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
            KdTermNextKey = KdbpTryGetCharSerial(5);
        else
            KdTermNextKey = KdbpTryGetCharKeyboard(ScanCode, 5);

        if (KdTermNextKey == '\n' || KdTermNextKey == -1) /* \n or no response at all */
            KdTermNextKey = ANSI_NULL;
    }

    return Key;
}

/* EOF */
