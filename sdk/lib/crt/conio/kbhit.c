/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/conio/kbhit.c
 * PURPOSE:     Checks for keyboard hits
 * PROGRAMERS:  Ariadne, Russell
 * UPDATE HISTORY:
 *              28/12/98: Created
 *              27/9/08: An almost 100% working version of _kbhit()
 */

#include <precomp.h>

static CRITICAL_SECTION CriticalSection;
volatile BOOL CriticalSectionInitialized=FALSE;

/*
 * @implemented
 */

int _kbhit(void)
{
    PINPUT_RECORD InputRecord = NULL;
    DWORD NumberRead = 0;
    DWORD EventsRead = 0;
    DWORD RecordIndex = 0;
    DWORD BufferIndex = 0;
    HANDLE StdInputHandle = 0;
    DWORD ConsoleInputMode = 0;

    /* Attempt some thread safety */
    if (!CriticalSectionInitialized)
    {
        InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x80000400);
        CriticalSectionInitialized = TRUE;
    }

    EnterCriticalSection(&CriticalSection);

    if (char_avail)
    {
        LeaveCriticalSection(&CriticalSection);
        return 1;
    }

    StdInputHandle = GetStdHandle(STD_INPUT_HANDLE);

    /* Turn off processed input so we get key modifiers as well */
    GetConsoleMode(StdInputHandle, &ConsoleInputMode);

    SetConsoleMode(StdInputHandle, ConsoleInputMode & ~ENABLE_PROCESSED_INPUT);

    /* Start the process */
    if (!GetNumberOfConsoleInputEvents(StdInputHandle, &EventsRead))
    {
        LeaveCriticalSection(&CriticalSection);
        return 0;
    }

    if (!EventsRead)
    {
        LeaveCriticalSection(&CriticalSection);
        return 0;
    }

    if (!(InputRecord = (PINPUT_RECORD)malloc(EventsRead * sizeof(INPUT_RECORD))))
    {
        LeaveCriticalSection(&CriticalSection);
        return 0;
    }

    if (!PeekConsoleInput(StdInputHandle, InputRecord, EventsRead, &NumberRead))
    {
        free(InputRecord);
        LeaveCriticalSection(&CriticalSection);
        return 0;
    }

    for (RecordIndex = 0; RecordIndex < NumberRead; RecordIndex++)
    {
        if (InputRecord[RecordIndex].EventType == KEY_EVENT &&
            InputRecord[RecordIndex].Event.KeyEvent.bKeyDown)
        {
            BufferIndex = 1;
            break;
        }
    }

    free(InputRecord);

    /* Restore console input mode */
    SetConsoleMode(StdInputHandle, ConsoleInputMode);

    LeaveCriticalSection(&CriticalSection);

    return BufferIndex;
}


