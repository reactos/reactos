/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.c
 * PURPOSE:         Virtual DOS Machine
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "bios/bios.h"
#include "hardware/cmos.h"
#include "hardware/ps2.h"
#include "hardware/timer.h"
#include "hardware/vga.h"
#include "dos/dos.h"

/*
 * Activate this line if you want to be able to test NTVDM with:
 * ntvdm.exe <program>
 */
#define TESTING

/* PUBLIC VARIABLES ***********************************************************/

static HANDLE ConsoleInput  = INVALID_HANDLE_VALUE;
static HANDLE ConsoleOutput = INVALID_HANDLE_VALUE;
static DWORD  OrgConsoleInputMode, OrgConsoleOutputMode;
static CONSOLE_CURSOR_INFO         OrgConsoleCursorInfo;
static CONSOLE_SCREEN_BUFFER_INFO  OrgConsoleBufferInfo;

/* PUBLIC FUNCTIONS ***********************************************************/

VOID DisplayMessage(LPCWSTR Format, ...)
{
    WCHAR Buffer[256];
    va_list Parameters;

    va_start(Parameters, Format);
    _vsnwprintf(Buffer, 256, Format, Parameters);
    DPRINT1("\n\nNTVDM Subsystem\n%S\n\n", Buffer);
    MessageBoxW(NULL, Buffer, L"NTVDM Subsystem", MB_OK);
    va_end(Parameters);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD ControlType)
{
    switch (ControlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        {
            /* Perform interrupt 0x23 */
            EmulatorInterrupt(0x23);
            break;
        }
        default:
        {
            /* Stop the VDM if the user logs out or closes the console */
            VdmRunning = FALSE;
        }
    }
    return TRUE;
}

BOOL ConsoleInit(VOID)
{
    /* Set the handler routine */
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    /* Get the input handle to the real console, and check for success */
    ConsoleInput = CreateFileW(L"CONIN$",
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
    if (ConsoleInput == INVALID_HANDLE_VALUE)
    {
        wprintf(L"FATAL: Cannot retrieve a handle to the console input\n");
        return FALSE;
    }

    /* Get the output handle to the real console, and check for success */
    ConsoleOutput = CreateFileW(L"CONOUT$",
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
    if (ConsoleOutput == INVALID_HANDLE_VALUE)
    {
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot retrieve a handle to the console output\n");
        return FALSE;
    }

    /* Save the original input and output console modes */
    if (!GetConsoleMode(ConsoleInput , &OrgConsoleInputMode ) ||
        !GetConsoleMode(ConsoleOutput, &OrgConsoleOutputMode))
    {
        CloseHandle(ConsoleOutput);
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot save console in/out modes\n");
        return FALSE;
    }

    /* Save the original cursor and console screen buffer information */
    if (!GetConsoleCursorInfo(ConsoleOutput, &OrgConsoleCursorInfo) ||
        !GetConsoleScreenBufferInfo(ConsoleOutput, &OrgConsoleBufferInfo))
    {
        CloseHandle(ConsoleOutput);
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot save console cursor/screen-buffer info\n");
        return FALSE;
    }

    return TRUE;
}

VOID ConsoleCleanup(VOID)
{
    SMALL_RECT ConRect;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    /* Restore the old screen buffer */
    SetConsoleActiveScreenBuffer(ConsoleOutput);

    /* Restore the original console size */
    GetConsoleScreenBufferInfo(ConsoleOutput, &ConsoleInfo);
    ConRect.Left = 0; // OrgConsoleBufferInfo.srWindow.Left;
    // ConRect.Top  = ConsoleInfo.dwCursorPosition.Y / (OrgConsoleBufferInfo.srWindow.Bottom - OrgConsoleBufferInfo.srWindow.Top + 1);
    // ConRect.Top *= (OrgConsoleBufferInfo.srWindow.Bottom - OrgConsoleBufferInfo.srWindow.Top + 1);
    ConRect.Top    = ConsoleInfo.dwCursorPosition.Y;
    ConRect.Right  = ConRect.Left + OrgConsoleBufferInfo.srWindow.Right  - OrgConsoleBufferInfo.srWindow.Left;
    ConRect.Bottom = ConRect.Top  + OrgConsoleBufferInfo.srWindow.Bottom - OrgConsoleBufferInfo.srWindow.Top ;
    /* See the following trick explanation in vga.c:VgaEnterTextMode() */
    SetConsoleScreenBufferSize(ConsoleOutput, OrgConsoleBufferInfo.dwSize);
    SetConsoleWindowInfo(ConsoleOutput, TRUE, &ConRect);
    // SetConsoleWindowInfo(ConsoleOutput, TRUE, &OrgConsoleBufferInfo.srWindow);
    SetConsoleScreenBufferSize(ConsoleOutput, OrgConsoleBufferInfo.dwSize);

    /* Restore the original cursor shape */
    SetConsoleCursorInfo(ConsoleOutput, &OrgConsoleCursorInfo);

    /* Restore the original input and output console modes */
    SetConsoleMode(ConsoleOutput, OrgConsoleOutputMode);
    SetConsoleMode(ConsoleInput , OrgConsoleInputMode );

    /* Close the console handles */
    if (ConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(ConsoleOutput);
    if (ConsoleInput  != INVALID_HANDLE_VALUE) CloseHandle(ConsoleInput);
}

INT wmain(INT argc, WCHAR *argv[])
{
    INT i;
    CHAR CommandLine[DOS_CMDLINE_LENGTH];
    LARGE_INTEGER StartPerfCount;
    LARGE_INTEGER Frequency, LastTimerTick, LastRtcTick, Counter;
    LONGLONG TimerTicks;
    DWORD StartTickCount, CurrentTickCount;
    DWORD LastClockUpdate;
    DWORD LastVerticalRefresh;
    DWORD LastCyclePrintout;
    DWORD Cycles = 0;
    INT KeyboardIntCounter = 0;

#ifndef TESTING
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    /* The DOS command line must be ASCII */
    WideCharToMultiByte(CP_ACP, 0, GetCommandLine(), -1, CommandLine, sizeof(CommandLine), NULL, NULL);
#else
    if (argc == 2 && argv[1] != NULL)
    {
        WideCharToMultiByte(CP_ACP, 0, argv[1], -1, CommandLine, sizeof(CommandLine), NULL, NULL);
    }
    else
    {
        wprintf(L"\nReactOS Virtual DOS Machine\n\n"
                L"Usage: NTVDM <executable>\n");
        return 0;
    }
#endif

    DPRINT1("\n\n\nNTVDM - Starting '%s'...\n\n\n", CommandLine);

    /* Initialize the console */
    if (!ConsoleInit())
    {
        wprintf(L"FATAL: A problem occurred when trying to initialize the console\n");
        goto Cleanup;
    }

    /* Initialize the emulator */
    if (!EmulatorInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the emulator\n");
        goto Cleanup;
    }

    /* Initialize the performance counter (needed for hardware timers) */
    if (!QueryPerformanceFrequency(&Frequency))
    {
        wprintf(L"FATAL: Performance counter not available\n");
        goto Cleanup;
    }

    /* Initialize the system BIOS */
    if (!BiosInitialize(ConsoleInput, ConsoleOutput))
    {
        wprintf(L"FATAL: Failed to initialize the VDM BIOS.\n");
        goto Cleanup;
    }

    /* Initialize the VDM DOS kernel */
    if (!DosInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the VDM DOS kernel.\n");
        goto Cleanup;
    }

    /* Start the process from the command line */
    if (!DosCreateProcess(CommandLine, 0))
    {
        DisplayMessage(L"Could not start program: %S", CommandLine);
        goto Cleanup;
    }

    /* Find the starting performance and tick count */
    StartTickCount = GetTickCount();
    QueryPerformanceCounter(&StartPerfCount);

    /* Set the different last counts to the starting count */
    LastClockUpdate = LastVerticalRefresh = LastCyclePrintout = StartTickCount;

    /* Set the last timer ticks to the current time */
    LastTimerTick = LastRtcTick = StartPerfCount;

    /* Main loop */
    while (VdmRunning)
    {
        DWORD PitResolution = PitGetResolution();
        DWORD RtcFrequency = RtcGetTicksPerSecond();

        /* Get the current number of ticks */
        CurrentTickCount = GetTickCount();

        if ((PitResolution <= 1000) && (RtcFrequency <= 1000))
        {
            /* Calculate the approximate performance counter value instead */
            Counter.QuadPart = StartPerfCount.QuadPart
                               + (CurrentTickCount - StartTickCount)
                               * (Frequency.QuadPart / 1000);
        }
        else
        {
            /* Get the current performance counter value */
            QueryPerformanceCounter(&Counter);
        }

        /* Get the number of PIT ticks that have passed */
        TimerTicks = ((Counter.QuadPart - LastTimerTick.QuadPart)
                     * PIT_BASE_FREQUENCY) / Frequency.QuadPart;

        /* Update the PIT */
        if (TimerTicks > 0)
        {
            PitDecrementCount(TimerTicks);
            LastTimerTick = Counter;
        }

        /* Check for RTC update */
        if ((CurrentTickCount - LastClockUpdate) >= 1000)
        {
            RtcTimeUpdate();
            LastClockUpdate = CurrentTickCount;
        }

        /* Check for RTC periodic tick */
        if ((Counter.QuadPart - LastRtcTick.QuadPart)
            >= (Frequency.QuadPart / (LONGLONG)RtcFrequency))
        {
            RtcPeriodicTick();
            LastRtcTick = Counter;
        }

        /* Check for vertical retrace */
        if ((CurrentTickCount - LastVerticalRefresh) >= 15)
        {
            VgaRefreshDisplay();
            LastVerticalRefresh = CurrentTickCount;
        }

        KeyboardIntCounter++;
        if (KeyboardIntCounter == KBD_INT_CYCLES)
        {
            GenerateKeyboardInterrupts();
            KeyboardIntCounter = 0;
        }

        /* Horizontal retrace occurs as fast as possible */
        VgaHorizontalRetrace();

        /* Continue CPU emulation */
        for (i = 0; (i < STEPS_PER_CYCLE) && VdmRunning; i++)
        {
            EmulatorStep();
            Cycles++;
        }

        if ((CurrentTickCount - LastCyclePrintout) >= 1000)
        {
            DPRINT1("NTVDM: %lu Instructions Per Second\n", Cycles);
            LastCyclePrintout = CurrentTickCount;
            Cycles = 0;
        }
    }

    /* Perform another screen refresh */
    VgaRefreshDisplay();

Cleanup:
    BiosCleanup();
    EmulatorCleanup();
    ConsoleCleanup();

    DPRINT1("\n\n\nNTVDM - Exiting...\n\n\n");

    return 0;
}

/* EOF */
