/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.c
 * PURPOSE:         Virtual DOS Machine
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "ntvdm.h"

BOOLEAN VdmRunning = TRUE;
LPVOID BaseAddress = NULL;
LPCWSTR ExceptionName[] =
{
    L"Division By Zero",
    L"Debug",
    L"Unexpected Error",
    L"Breakpoint",
    L"Integer Overflow",
    L"Bound Range Exceeded",
    L"Invalid Opcode",
    L"FPU Not Available"
};

VOID DisplayMessage(LPCWSTR Format, ...)
{
    WCHAR Buffer[256];
    va_list Parameters;

    va_start(Parameters, Format);
    _vsnwprintf(Buffer, 256, Format, Parameters);
    MessageBox(NULL, Buffer, L"NTVDM Subsystem", MB_OK);
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
    BOOLEAN PrintUsage = TRUE;
    CHAR CommandLine[128];

    /* Set the handler routine */
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    /* Parse the command line arguments */
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != L'-' && argv[i][0] != L'/') continue;

        switch (argv[i][1])
        {
            case L'f':
            case L'F':
            {
                if (argv[i+1] != NULL)
                {
                    /* The DOS command line must be ASCII */
                    WideCharToMultiByte(CP_ACP, 0, argv[i+1], -1, CommandLine, 128, NULL, NULL);

                    /* This is the only mandatory parameter */
                    PrintUsage = FALSE;
                }
                break;
            }
            default:
            {
                wprintf(L"Unknown option: %s", argv[i]);
            }
        }
    }

    if (PrintUsage)
    {
        wprintf(L"ReactOS Virtual DOS Machine\n\n");
        wprintf(L"Usage: NTVDM /F <PROGRAM>\n");
        return 0;
    }

    if (!EmulatorInitialize()) return 1;

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
        return -1;
    }

    /* Main loop */
    while (VdmRunning) EmulatorStep();

Cleanup:
    EmulatorCleanup();

    return 0;
}

/* EOF */
