/* $Id: ntvdm.c,v 1.1 2002/10/28 13:59:59 robd Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            subsys/ntvdm/ntvdm.c
 * PURPOSE:         Virtual DOS Machine
 * PROGRAMMER:      Robert Dickenson (robd@mok.lvcm.com)
 * UPDATE HISTORY:
 *                  Created 23/10/2002
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/


/* FUNCTIONS *****************************************************************/

void PrintString(char* fmt,...)
{
   char buffer[512];
   va_list ap;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);

   OutputDebugString(buffer);
}

/*
GetVersion
GetVolumeInformationW
GetWindowsDirectoryA
GlobalMemoryStatus
HeapAlloc
HeapCreate
HeapDestroy
HeapFree
HeapReAlloc

GetNextVDMCommand
ExitVDM
RegisterConsoleVDM
SetVDMCurrentDirectories
VDMConsoleOperation
WriteConsoleInputVDMW

NtSetLdtEntries
NtTerminateProcess

NtMapViewOfSection
NtUnmapViewOfSection

NtVdmControl
 */

BOOLEAN
StartVirtualMachine(VOID)
{
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   CHAR CommandLine[MAX_PATH];
   CHAR CurrentDirectory[MAX_PATH];

   GetSystemDirectory(CommandLine, MAX_PATH);
   strcat(CommandLine, "\\hello.exe");
   GetWindowsDirectory(CurrentDirectory, MAX_PATH);

   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;

   Result = CreateProcess(CommandLine,
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          DETACHED_PROCESS,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
    if (!Result) {
        PrintString("WL: Failed to execute target process\n");
        return FALSE;
    }
    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
    CloseHandle(ProcessInformation.hProcess);
    CloseHandle(ProcessInformation.hThread);
    return TRUE;
}

int STDCALL
WinMain(HINSTANCE hInstance,  HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    DWORD Result;
    BOOL Success;
    ULONG i;
    NTSTATUS Status;

    CHAR WelcomeMsg[] = "ReactOS Virtual DOS Machine support.\nType q<cr> to quit.";
    CHAR InputBuffer[255];
    
    AllocConsole();
    SetConsoleTitle("ntvdm");
    StartVirtualMachine();
   
    for (;;) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                     WelcomeMsg, strlen(WelcomeMsg),  // wcslen(WelcomeMsg),
                     &Result, NULL);
        i = 0;
        do {
            ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
                        &InputBuffer[i], 1,
                        &Result, NULL);
            if (++i >= (sizeof(InputBuffer) - 1)) {
                break;
            }
        } while (InputBuffer[i - 1] != '\n');
        InputBuffer[i - 1] = '\0';

        if (InputBuffer[0] == 'q' || InputBuffer[0] == 'Q') {
            break;
        }
    }

    ExitProcess(0);
    return 0;
}
