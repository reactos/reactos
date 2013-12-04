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
#include "cmos.h"
#include "bios.h"
#include "speaker.h"
#include "vga.h"
#include "dos.h"
#include "timer.h"
#include "pic.h"
#include "ps2.h"

/*
 * Activate this line if you want to be able to test NTVDM with:
 * ntvdm.exe <program>
 */
#define TESTING

/* PUBLIC VARIABLES ***********************************************************/

BOOLEAN VdmRunning = TRUE;
LPVOID BaseAddress = NULL;

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

    /* Set the handler routine */
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

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

    /* Initialize the emulator */
    if (!EmulatorInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the CPU emulator\n");
        goto Cleanup;
    }

    /* Initialize the performance counter (needed for hardware timers) */
    if (!QueryPerformanceFrequency(&Frequency))
    {
        wprintf(L"FATAL: Performance counter not available\n");
        goto Cleanup;
    }

    /* Initialize the PIC */
    if (!PicInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the PIC.\n");
        goto Cleanup;
    }

    /* Initialize the PIT */
    if (!PitInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the PIT.\n");
        goto Cleanup;
    }

    /* Initialize the CMOS */
    if (!CmosInitialize())
    {
        wprintf(L"FATAL: Failed to initialize the VDM CMOS.\n");
        goto Cleanup;
    }

    /* Initialize the PC Speaker */
    SpeakerInitialize();

    /* Initialize the system BIOS */
    if (!BiosInitialize())
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
    SpeakerCleanup();
    BiosCleanup();
    CmosCleanup();
    EmulatorCleanup();

    DPRINT1("\n\n\nNTVDM - Exiting...\n\n\n");

    return 0;
}

/* EOF */
