/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Timeout utility
 * FILE:            base/applications/cmdutils/timeout/timeout.c
 * PURPOSE:         An enhanced alternative to the Pause command.
 * PROGRAMMERS:     Lee Schroeder (spaceseel at gmail dot com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>

#include <conutils.h>

#include "resource.h"

VOID PrintError(DWORD dwError)
{
    if (dwError == ERROR_SUCCESS)
        return;

    ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
               NULL, dwError, LANG_USER_DEFAULT);
    ConPuts(StdErr, L"\n");
}

BOOL
WINAPI
CtrlCIntercept(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
            ConPuts(StdOut, L"\n");
            SetConsoleCtrlHandler(NULL, FALSE);
            ExitProcess(EXIT_FAILURE);
            return TRUE;
    }
    return FALSE;
}

INT InputWait(BOOL bNoBreak, INT timerValue)
{
    INT Status = EXIT_SUCCESS;
    HANDLE hInput;
    BOOL bUseTimer = (timerValue != -1);
    HANDLE hTimer = NULL;
    DWORD dwStartTime;
    LONG timeElapsed;
    DWORD dwWaitState;
    INPUT_RECORD InputRecords[5];
    ULONG NumRecords, i;
    BOOL DisplayMsg = TRUE;
    UINT WaitMsgId = (bNoBreak ? IDS_NOBREAK_INPUT : IDS_USER_INPUT);
    UINT WaitCountMsgId = (bNoBreak ? IDS_NOBREAK_INPUT_COUNT : IDS_USER_INPUT_COUNT);

    /* Retrieve the current input handle */
    hInput = ConStreamGetOSHandle(StdIn);
    if (hInput == INVALID_HANDLE_VALUE)
    {
        ConResPrintf(StdErr, IDS_ERROR_INVALID_HANDLE_VALUE, GetLastError());
        return EXIT_FAILURE;
    }

    /* Start a new wait if we use the timer */
    if (bNoBreak && bUseTimer)
    {
        hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
        if (hTimer == NULL)
        {
            /* A problem happened, bail out */
            PrintError(GetLastError());
            return EXIT_FAILURE;
        }
    }
    if (bUseTimer)
        dwStartTime = GetTickCount();

    /* If /NOBREAK is used, monitor for Ctrl-C input */
    if (bNoBreak)
        SetConsoleCtrlHandler(CtrlCIntercept, TRUE);

    /* Initially flush the console input queue to remove any pending events */
    if (!GetNumberOfConsoleInputEvents(hInput, &NumRecords) ||
        !FlushConsoleInputBuffer(hInput))
    {
        /* A problem happened, bail out */
        PrintError(GetLastError());
        Status = EXIT_FAILURE;
        goto Quit;
    }

    ConPuts(StdOut, L"\n");

    /* If the timer is not used, just show the message */
    if (!bUseTimer)
    {
        ConPuts(StdOut, L"\r");
        ConResPuts(StdOut, WaitMsgId);
    }

    while (TRUE)
    {
        /* Decrease the timer if we use it */
        if (bUseTimer)
        {
            /*
             * Compute how much time the previous operations took.
             * This allows us in particular to take account for any time
             * elapsed if something slowed down, or if the console has been
             * paused in the meantime.
             */
            timeElapsed = GetTickCount() - dwStartTime;
            if (timeElapsed >= 1000)
            {
                /* Increase dwStartTime by steps of 1 second */
                timeElapsed /= 1000;
                dwStartTime += (1000 * timeElapsed);

                if (timeElapsed <= timerValue)
                    timerValue -= timeElapsed;
                else
                    timerValue = 0;

                DisplayMsg = TRUE;
            }

            if (DisplayMsg)
            {
                ConPuts(StdOut, L"\r");
                ConResPrintf(StdOut, WaitCountMsgId, timerValue);
                ConPuts(StdOut, L" \b");

                DisplayMsg = FALSE;
            }

            /* Stop when the timer reaches zero */
            if (timerValue <= 0)
                break;
        }

        /* If /NOBREAK is used, only allow Ctrl-C input which is handled by the console handler */
        if (bNoBreak)
        {
            if (bUseTimer)
            {
                LARGE_INTEGER DueTime;

                /* We use the timer: use a passive wait of maximum 1 second */
                timeElapsed = GetTickCount() - dwStartTime;
                if (timeElapsed < 1000)
                {
                    DueTime.QuadPart = Int32x32To64(1000 - timeElapsed, -10000);
                    SetWaitableTimer(hTimer, &DueTime, 0, NULL, NULL, FALSE);
                    dwWaitState = WaitForSingleObject(hTimer, INFINITE);

                    /* Check whether the timer has been signaled */
                    if (dwWaitState != WAIT_OBJECT_0)
                    {
                        /* An error happened, bail out */
                        PrintError(GetLastError());
                        Status = EXIT_FAILURE;
                        break;
                    }
                }
            }
            else
            {
                /* No timer is used: wait indefinitely */
                Sleep(INFINITE);
            }

            continue;
        }

        /* /NOBREAK is not used, check for user key presses */

        /*
         * If the timer is used, use a passive wait of maximum 1 second
         * while monitoring for incoming console input events, so that
         * we are still able to display the timing count.
         * Indeed, ReadConsoleInputW() indefinitely waits until an input
         * event appears. ReadConsoleInputW() is however used to retrieve
         * the input events where there are some, as well as for waiting
         * indefinitely in case we do not use the timer.
         */
        if (bUseTimer)
        {
            /* Wait a maximum of 1 second for input events */
            timeElapsed = GetTickCount() - dwStartTime;
            if (timeElapsed < 1000)
                dwWaitState = WaitForSingleObject(hInput, 1000 - timeElapsed);
            else
                dwWaitState = WAIT_TIMEOUT;

            /* Check whether the input event has been signaled, or a timeout happened */
            if (dwWaitState == WAIT_TIMEOUT)
                continue;
            if (dwWaitState != WAIT_OBJECT_0)
            {
                /* An error happened, bail out */
                PrintError(GetLastError());
                Status = EXIT_FAILURE;
                break;
            }

            /* Be sure there is something in the console input queue */
            if (!PeekConsoleInputW(hInput, InputRecords, ARRAYSIZE(InputRecords), &NumRecords))
            {
                /* An error happened, bail out */
                ConResPrintf(StdErr, IDS_ERROR_READ_INPUT, GetLastError());
                Status = EXIT_FAILURE;
                break;
            }

            if (NumRecords == 0)
                continue;
        }

        /*
         * Some events have been detected, pop them out from the input queue.
         * In case we do not use the timer, wait indefinitely until an input
         * event appears.
         */
        if (!ReadConsoleInputW(hInput, InputRecords, ARRAYSIZE(InputRecords), &NumRecords))
        {
            /* An error happened, bail out */
            ConResPrintf(StdErr, IDS_ERROR_READ_INPUT, GetLastError());
            Status = EXIT_FAILURE;
            break;
        }

        /* Check the input events for a key press */
        for (i = 0; i < NumRecords; ++i)
        {
            /* Ignore any non-key event */
            if (InputRecords[i].EventType != KEY_EVENT)
                continue;

            /* Ignore any system key event */
            if ((InputRecords[i].Event.KeyEvent.wVirtualKeyCode == VK_CONTROL) ||
             // (InputRecords[i].Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED )) ||
             // (InputRecords[i].Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) ||
                (InputRecords[i].Event.KeyEvent.wVirtualKeyCode == VK_MENU))
            {
                continue;
            }

            /* This is a non-system key event, stop waiting */
            goto Stop;
        }
    }

Stop:
    ConPuts(StdOut, L"\n");

Quit:
    if (bNoBreak)
        SetConsoleCtrlHandler(NULL, FALSE);

    if (bNoBreak && bUseTimer)
        CloseHandle(hTimer);

    return Status;
}

int wmain(int argc, WCHAR* argv[])
{
    INT timerValue = -1;
    PWCHAR pszNext;
    BOOL bDisableInput = FALSE, fTimerFlags = 0;
    int index = 0;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc == 1)
    {
        ConResPrintf(StdOut, IDS_USAGE);
        return EXIT_SUCCESS;
    }

    /* Parse the command line for options */
    for (index = 1; index < argc; index++)
    {
        if (argv[index][0] == L'-' || argv[index][0] == L'/')
        {
            switch (towupper(argv[index][1]))
            {
                case L'?': /* Help */
                {
                    ConResPrintf(StdOut, IDS_USAGE);
                    return EXIT_SUCCESS;
                }

                case L'T': /* Timer */
                {
                    /* Consecutive /T switches are invalid */
                    if (fTimerFlags & 2)
                    {
                        ConResPrintf(StdErr, IDS_ERROR_ONE_TIME);
                        return EXIT_FAILURE;
                    }

                    /* Remember that a /T switch has been encountered */
                    fTimerFlags |= 2;

                    /* Go to the next (timer) value */
                    continue;
                }
            }

            /* This flag is used to ignore any keyboard keys but Ctrl-C */
            if (_wcsicmp(&argv[index][1], L"NOBREAK") == 0)
            {
                bDisableInput = TRUE;

                /* Go to next value */
                continue;
            }
        }

        /* The timer value can also be specified without the /T switch */

        /* Only one timer value is supported */
        if (fTimerFlags & 1)
        {
            ConResPrintf(StdErr, IDS_ERROR_ONE_TIME);
            return EXIT_FAILURE;
        }

        timerValue = wcstol(argv[index], &pszNext, 10);
        if (*pszNext)
        {
            ConResPrintf(StdErr, IDS_ERROR_OUT_OF_RANGE);
            return EXIT_FAILURE;
        }

        /* Remember that the timer value has been set */
        fTimerFlags |= 1;
    }

    /* A timer value is mandatory in order to continue */
    if (!(fTimerFlags & 1))
    {
        ConResPrintf(StdErr, IDS_ERROR_NO_TIMER_VALUE);
        return EXIT_FAILURE;
    }

    /* Make sure the timer value is within range */
    if ((timerValue < -1) || (timerValue > 99999))
    {
        ConResPrintf(StdErr, IDS_ERROR_OUT_OF_RANGE);
        return EXIT_FAILURE;
    }

    return InputWait(bDisableInput, timerValue);
}
