/*
 * PROJECT:     ReactOS KDBG Kernel Debugger
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Command History helpers
 * COPYRIGHT:   Copyright 2005 Gregor Anich <blight@blight.eu.org>
 *              Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

/* GLOBALS *******************************************************************/

/**
 * History range in ring buffer:
 * (KdbCommandHistoryIndex; RTL_NUMBER_OF(KdbCommandHistory) - 1]
 *   and [0; KdbCommandHistoryIndex].
 **/

/* Command history string ring buffer */
static CHAR KdbCommandHistoryBuffer[2048];
/* Command history ring buffer */
static PCHAR KdbCommandHistory[sizeof(KdbCommandHistoryBuffer) / 8] = { NULL };
static LONG KdbCommandHistoryBufferIndex = 0;
static LONG KdbCommandHistoryIndex = 0;

/* FUNCTIONS *****************************************************************/

/**
 * @brief   Appends a command to the command history.
 *
 * @param[in]   Command
 * Pointer to the command to append to the history.
 **/
VOID
KdbpCommandHistoryAppend(
    _In_ PCSTR Command)
{
    SIZE_T Length1 = strlen(Command) + 1;
    SIZE_T Length2 = 0;
    LONG i;
    PCHAR Buffer;

    ASSERT(Length1 <= RTL_NUMBER_OF(KdbCommandHistoryBuffer));

    /*
     * Do not append the string if:
     * - it is empty (just the NULL terminator);
     * - or the last command is the same.
     */
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

static PCSTR
KdbpGetPrevHistoryEntry(
    _Inout_ PLONG NextIndex)
{
    if (*NextIndex < 0)
    {
        /* Start at the end of the history */
        *NextIndex = KdbCommandHistoryIndex;
    }
    else
    {
        LONG i = *NextIndex - 1;
        if (i < 0)
            *NextIndex = RTL_NUMBER_OF(KdbCommandHistory) - 1;

        if (KdbCommandHistory[i] && i != KdbCommandHistoryIndex)
            *NextIndex = i;
        else
            return NULL;
    }

    return KdbCommandHistory[*NextIndex];
}

static PCSTR
KdbpGetNextHistoryEntry(
    _Inout_ PLONG NextIndex)
{
    LONG i;

    if (!(*NextIndex > 0 && *NextIndex != KdbCommandHistoryIndex))
        return NULL;

    i = *NextIndex + 1;
    if (i >= (LONG)RTL_NUMBER_OF(KdbCommandHistory))
        i = 0;

    if (KdbCommandHistory[i])
        *NextIndex = i;

    return KdbCommandHistory[i];
}

PCSTR
KdbGetHistoryEntry(
    _Inout_ PLONG NextIndex,
    _In_ BOOLEAN Next)
{
    if (Next)
        return KdbpGetNextHistoryEntry(NextIndex);
    else
        return KdbpGetPrevHistoryEntry(NextIndex);
}

/* EOF */
