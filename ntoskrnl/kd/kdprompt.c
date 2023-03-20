/*
 * PROJECT:     ReactOS KDBG Kernel Debugger Terminal Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Terminal line-editing (Prompt) interface
 * COPYRIGHT:   Copyright 2001-2004 David Welch <welch@cwcom.net>
 *              Copyright 2004-2005 Gregor Anich <blight@blight.eu.org>
 *              Copyright 2022-2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "kdterminal.h"

/* FUNCTIONS *****************************************************************/

/**
 * @brief   Reads a line of user input from the terminal.
 *
 * @param[out]  Buffer
 * Buffer where to store the input. Trailing newlines are removed.
 *
 * @param[in]   Size
 * Size of @p Buffer.
 *
 * @return
 * Returns the number of characters stored, not counting the NULL terminator.
 *
 * @note Accepts only \n newlines, \r is ignored.
 **/
SIZE_T
KdIoReadLine(
    _Out_ PCHAR Buffer,
    _In_ SIZE_T Size)
{
    PCHAR Orig = Buffer;
    ULONG ScanCode = 0;
    CHAR Key;
    static CHAR NextKey = ANSI_NULL;
    BOOLEAN EchoOn;
    LONG CmdHistIndex = -1; // Start at end of history.

    /* Bail out if the buffer is zero-sized */
    if (Size == 0)
        return 0;

    EchoOn = ((KdbDebugState & KD_DEBUG_KDNOECHO) == 0);

    for (;;)
    {
        ScanCode = 0;
        if (KdbDebugState & KD_DEBUG_KDSERIAL)
        {
            Key = (!NextKey ? KdbpGetCharSerial() : NextKey);
            NextKey = ANSI_NULL;
            if (Key == KEY_ESC) /* ESC */
            {
                Key = KdbpGetCharSerial();
                if (Key == '[')
                {
                    Key = KdbpGetCharSerial();

                    switch (Key)
                    {
                        case 'A':
                            ScanCode = KEY_SCAN_UP;
                            break;
                        case 'B':
                            ScanCode = KEY_SCAN_DOWN;
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
            Key = (!NextKey ? KdbpGetCharKeyboard(&ScanCode) : NextKey);
            NextKey = ANSI_NULL;
        }

        /* Check for return or newline */
        if ((Key == '\r') || (Key == '\n'))
        {
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
                    NextKey = KdbpTryGetCharKeyboard(&ScanCode, 5);

                if (NextKey == '\n' || NextKey == -1) /* \n or no response at all */
                    NextKey = ANSI_NULL;
            }

            *Buffer = ANSI_NULL;
            KdIoPuts("\n");
            return (SIZE_T)(Buffer - Orig);
        }
        else if (Key == KEY_BS || Key == KEY_DEL)
        {
            /* Erase the last character */
            if (Buffer > Orig)
            {
                Buffer--;
                *Buffer = ANSI_NULL;

                if (EchoOn)
                    KdIoPrintf("%c %c", KEY_BS, KEY_BS);
                else
                    KdIoPrintf(" %c", KEY_BS);
            }
        }
        else if (ScanCode == KEY_SCAN_UP || ScanCode == KEY_SCAN_DOWN)
        {
            PCSTR CmdHistory = KdbGetHistoryEntry(&CmdHistIndex,
                                                  (ScanCode == KEY_SCAN_DOWN));
            if (CmdHistory)
            {
                SIZE_T i;

                /* Erase the whole line */
                while (Buffer > Orig)
                {
                    Buffer--;
                    *Buffer = ANSI_NULL;

                    if (EchoOn)
                        KdIoPrintf("%c %c", KEY_BS, KEY_BS);
                    else
                        KdIoPrintf(" %c", KEY_BS);
                }

                /* Copy and display the history entry */
                i = min(strlen(CmdHistory), Size - 1);
                memcpy(Orig, CmdHistory, i);
                Orig[i] = ANSI_NULL;
                Buffer = Orig + i;
                KdIoPuts(Orig);
            }
        }
        else
        {
            /* Do not accept characters anymore if the buffer is full */
            if ((SIZE_T)(Buffer - Orig) >= (Size - 1))
                continue;

            if (EchoOn)
                KdIoPrintf("%c", Key);

            *Buffer = Key;
            Buffer++;
        }
    }
}

/* EOF */
