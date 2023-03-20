/*
 * PROJECT:     ReactOS KDBG Kernel Debugger
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Debugger prompt interface
 * COPYRIGHT:   Copyright 2001-2004 David Welch <welch@cwcom.net>
 *              Copyright 2004-2005 Gregor Anich <blight@blight.eu.org>
 *              Copyright 2022-2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#include "kdb_terminal.h"
/***/#include "kdb_terminal.c"/***/ // FIXME: Properly add to compilation!

/* GLOBALS *******************************************************************/

/* Command history string ring buffer */
static CHAR KdbCommandHistoryBuffer[2048];
/* Command history ring buffer */
static PCHAR KdbCommandHistory[sizeof(KdbCommandHistoryBuffer) / 8] = { NULL };
static LONG KdbCommandHistoryBufferIndex = 0;
static LONG KdbCommandHistoryIndex = 0;
BOOLEAN KdbRepeatLastCommand = FALSE;

/* FUNCTIONS *****************************************************************/

/**
 * @brief   Appends a command to the command history.
 *
 * @param[in]   Command
 * Pointer to the command to append to the history.
 **/
static VOID
KdbpCommandHistoryAppend(
    _In_ PCSTR Command)
{
    SIZE_T Length1 = strlen(Command) + 1;
    SIZE_T Length2 = 0;
    INT i;
    PCHAR Buffer;

    ASSERT(Length1 <= RTL_NUMBER_OF(KdbCommandHistoryBuffer));

    if (Length1 <= 1 ||
        (KdbCommandHistory[KdbCommandHistoryIndex] &&
         strcmp(KdbCommandHistory[KdbCommandHistoryIndex], Command) == 0))
    {
        return;
    }

    /* Calculate Length1 and Length2 */
    Buffer = KdbCommandHistoryBuffer + KdbCommandHistoryBufferIndex;
    KdbCommandHistoryBufferIndex += Length1;
    if (KdbCommandHistoryBufferIndex >= (LONG)RTL_NUMBER_OF(KdbCommandHistoryBuffer))
    {
        KdbCommandHistoryBufferIndex -= RTL_NUMBER_OF(KdbCommandHistoryBuffer);
        Length2 = KdbCommandHistoryBufferIndex;
        Length1 -= Length2;
    }

    /* Remove previous commands until there is enough space to append the new command */
    for (i = KdbCommandHistoryIndex; KdbCommandHistory[i];)
    {
        if ((Length2 > 0 &&
            (KdbCommandHistory[i] >= Buffer ||
             KdbCommandHistory[i] < (KdbCommandHistoryBuffer + KdbCommandHistoryBufferIndex))) ||
            (Length2 <= 0 &&
             (KdbCommandHistory[i] >= Buffer &&
              KdbCommandHistory[i] < (KdbCommandHistoryBuffer + KdbCommandHistoryBufferIndex))))
        {
            KdbCommandHistory[i] = NULL;
        }

        i--;
        if (i < 0)
            i = RTL_NUMBER_OF(KdbCommandHistory) - 1;

        if (i == KdbCommandHistoryIndex)
            break;
    }

    /* Make sure the new command history entry is free */
    KdbCommandHistoryIndex++;
    KdbCommandHistoryIndex %= RTL_NUMBER_OF(KdbCommandHistory);
    if (KdbCommandHistory[KdbCommandHistoryIndex])
    {
        KdbCommandHistory[KdbCommandHistoryIndex] = NULL;
    }

    /* Append command */
    KdbCommandHistory[KdbCommandHistoryIndex] = Buffer;
    ASSERT((KdbCommandHistory[KdbCommandHistoryIndex] + Length1) <= KdbCommandHistoryBuffer + RTL_NUMBER_OF(KdbCommandHistoryBuffer));
    memcpy(KdbCommandHistory[KdbCommandHistoryIndex], Command, Length1);
    if (Length2 > 0)
    {
        memcpy(KdbCommandHistoryBuffer, Command + Length1, Length2);
    }
}

/**
 * @brief   Reads a line of user input from the terminal.
 *
 * @param[out]  Buffer
 * Buffer where to store the input. Trailing newlines are removed.
 *
 * @param[in]   Size
 * Size of \a Buffer.
 *
 * @return
 * Returns the number of characters stored, not counting the NULL terminator.
 *
 * @note Accepts only \n newlines, \r is ignored.
 **/
SIZE_T
KdbpReadCommand(
    _Out_ PCHAR Buffer,
    _In_ SIZE_T Size)
{
    PCHAR Orig = Buffer;
    ULONG ScanCode = 0;
    CHAR Key;
    BOOLEAN EchoOn;
    static CHAR NextKey = ANSI_NULL;
    INT CmdHistIndex = -1;
    INT_PTR i;

    /* Bail out if the buffer is zero-sized */
    if (Size == 0)
        return 0;

    EchoOn = ((KdbDebugState & KD_DEBUG_KDNOECHO) == 0);

    for (;;)
    {
        Key = KdpReadTerminal(&ScanCode, &NextKey);

        /* Check for return or newline */
        if ((Key == '\r') || (Key == '\n'))
        {
            *Buffer = ANSI_NULL;
            KdpDprintf("\n");
            return (SIZE_T)(Buffer - Orig);
        }
        else if (Key == KEY_BS || Key == KEY_DEL)
        {
            if (Buffer > Orig)
            {
                Buffer--;
                *Buffer = ANSI_NULL;

                if (EchoOn)
                    KdpDprintf("%c %c", KEY_BS, KEY_BS);
                else
                    KdpDprintf(" %c", KEY_BS);
            }
        }
        else if (ScanCode == KEY_SCAN_UP)
        {
            BOOLEAN Print = TRUE;

            if (CmdHistIndex < 0)
            {
                CmdHistIndex = KdbCommandHistoryIndex;
            }
            else
            {
                i = CmdHistIndex - 1;

                if (i < 0)
                    CmdHistIndex = RTL_NUMBER_OF(KdbCommandHistory) - 1;

                if (KdbCommandHistory[i] && i != KdbCommandHistoryIndex)
                    CmdHistIndex = i;
                else
                    Print = FALSE;
            }

            if (Print && KdbCommandHistory[CmdHistIndex])
            {
                while (Buffer > Orig)
                {
                    Buffer--;
                    *Buffer = ANSI_NULL;

                    if (EchoOn)
                        KdpDprintf("%c %c", KEY_BS, KEY_BS);
                    else
                        KdpDprintf(" %c", KEY_BS);
                }

                i = min(strlen(KdbCommandHistory[CmdHistIndex]), Size - 1);
                memcpy(Orig, KdbCommandHistory[CmdHistIndex], i);
                Orig[i] = ANSI_NULL;
                Buffer = Orig + i;
                KdpDprintf("%s", Orig);
            }
        }
        else if (ScanCode == KEY_SCAN_DOWN)
        {
            if (CmdHistIndex > 0 && CmdHistIndex != KdbCommandHistoryIndex)
            {
                i = CmdHistIndex + 1;
                if (i >= (INT)RTL_NUMBER_OF(KdbCommandHistory))
                    i = 0;

                if (KdbCommandHistory[i])
                {
                    CmdHistIndex = i;
                    while (Buffer > Orig)
                    {
                        Buffer--;
                        *Buffer = ANSI_NULL;

                        if (EchoOn)
                            KdpDprintf("%c %c", KEY_BS, KEY_BS);
                        else
                            KdpDprintf(" %c", KEY_BS);
                    }

                    i = min(strlen(KdbCommandHistory[CmdHistIndex]), Size - 1);
                    memcpy(Orig, KdbCommandHistory[CmdHistIndex], i);
                    Orig[i] = ANSI_NULL;
                    Buffer = Orig + i;
                    KdpDprintf("%s", Orig);
                }
            }
        }
        else
        {
            /* Do not accept characters anymore if the buffer is full */
            if ((SIZE_T)(Buffer - Orig) >= (Size - 1))
                continue;

            if (EchoOn)
                KdpDprintf("%c", Key);

            *Buffer = Key;
            Buffer++;
        }
    }
}

SIZE_T
KdpDprompt(
    _In_ PCSTRING Prompt,
    _Out_ PCHAR Buffer,
    _In_ SIZE_T Size)
{
    SIZE_T CmdLen;
    static CHAR LastCommand[1024] = "";

    /* Print the prompt */
    KdpDprintf(Prompt->Buffer);

    /* Bail out if the buffer is zero-sized */
    if (Size == 0)
        return 0;

    /* Read a command */
    CmdLen = KdbpReadCommand(Buffer, Size);

    /*
     * Repeat the last command if the user pressed Enter.
     * Reduces the risk of RSI when single-stepping.
     */
    if (CmdLen > 0) // i.e. (*Buffer != ANSI_NULL)
    {
        KdbRepeatLastCommand = TRUE;
        RtlStringCbCopyA(LastCommand, sizeof(LastCommand), Buffer);
    }
    else if (KdbRepeatLastCommand)
    {
        /* The user directly pressed Enter */
        RtlStringCbCopyA(Buffer, Size, LastCommand);
    }

    /* Remember it */
    KdbpCommandHistoryAppend(Buffer);

    return CmdLen;
}

/* EOF */
