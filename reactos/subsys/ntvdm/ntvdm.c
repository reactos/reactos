/* $Id: ntvdm.c,v 1.2 2003/01/12 01:54:40 robd Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            subsys/ntvdm/ntvdm->c
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
typedef struct tag_VDM_CONFIG {
    int dos_options;
    int files;
    int buffers;
    WCHAR** device_list;
//dos=high, umb
//device=%SystemRoot%\system32\himem.sys
//files=40
} VDM_CONFIG, *PVDM_CONFIG;

typedef struct tag_VDM_AUTOEXEC {
    WCHAR** load_list;
//lh %SystemRoot%\system32\mscdexnt.exe
//lh %SystemRoot%\system32\redir
//lh %SystemRoot%\system32\dosx
} VDM_AUTOEXEC, *PVDM_AUTOEXEC;

typedef struct tag_VDM_CONTROL_BLOCK {
    HANDLE hHeap;
    PVOID ImageMem;
    VDM_CONFIG vdmConfig;
    VDM_AUTOEXEC vdmAutoexec;
    PROCESS_INFORMATION ProcessInformation;
    CHAR CommandLine[MAX_PATH];
    CHAR CurrentDirectory[MAX_PATH];

} VDM_CONTROL_BLOCK, *PVDM_CONTROL_BLOCK;


BOOL
StartVDM(PVDM_CONTROL_BLOCK vdm)
{
   BOOL Result;
   STARTUPINFO StartupInfo;

   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;

   Result = CreateProcess(vdm->CommandLine,
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          DETACHED_PROCESS,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &vdm->ProcessInformation);
    if (!Result) {
        PrintString("VDM: Failed to execute target process\n");
        return FALSE;
    }
    WaitForSingleObject(vdm->ProcessInformation.hProcess, INFINITE);
    CloseHandle(vdm->ProcessInformation.hProcess);
    CloseHandle(vdm->ProcessInformation.hThread);
    return TRUE;
}

BOOL
ShutdownVDM(PVDM_CONTROL_BLOCK vdm)
{
    BOOL result = TRUE;

    return result;
}

BOOL ReadConfigForVDM(PVDM_CONTROL_BLOCK vdm)
{
    BOOL result = TRUE;
    DWORD dwError;
    HANDLE hFile;
    
    hFile = CreateFileW(L"\\system32\\config.nt",
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS /*OPEN_EXISTING*/,
                        FILE_ATTRIBUTE_NORMAL,
                        0);
    dwError = GetLastError();
    if (hFile == INVALID_HANDLE_VALUE) {
        // error with file path or system problem?
    } else {
        if (dwError == 0L) {
            // we just created a new file, perhaps we should set/write some defaults?
        }
        if (dwError == ERROR_ALREADY_EXISTS) {
            // read the line entries and cache in some struct...
        }
        CloseHandle(hFile);
    }

    hFile = CreateFileW(L"\\system32\\autoexec.nt",
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        0);
    dwError = GetLastError();
    if (hFile == INVALID_HANDLE_VALUE) {
        // error with file path or system problem?
    } else {
        if (dwError == 0L) {
            // we just created a new file, perhaps we should set/write some defaults?
        }
        if (dwError == ERROR_ALREADY_EXISTS) {
            // read the line entries and cache in some struct...
        }
        CloseHandle(hFile);
    }

	return result;
}

BOOL
LoadConfigDriversForVDM(PVDM_CONFIG vdmConfig)
{
    BOOL result = TRUE;
	
	return result;
}

BOOL
SetConfigOptionsForVDM(PVDM_AUTOEXEC vdmAutoexec)
{
    BOOL result = TRUE;
	
	return result;
}

BOOL
CreateVDM(PVDM_CONTROL_BLOCK vdm)
{
//    BOOL result = TRUE;
    SYSTEM_INFO inf;
    MEMORYSTATUS stat;
	PVOID lpMem = NULL;

    GlobalMemoryStatus(&stat);
    if (stat.dwLength != sizeof(MEMORYSTATUS)) {
        printf("WARNING: GlobalMemoryStatus() returned unknown structure version, size %ld, expected %d.\n", stat.dwLength, sizeof(stat));
    } else {
        printf("Memory Load: %ld percent in use.\n", stat.dwMemoryLoad);
        printf("\t%ld total bytes physical memory.\n", stat.dwTotalPhys);
        printf("\t%ld available physical memory.\n", stat.dwAvailPhys);
        printf("\t%ld total bytes paging file.\n", stat.dwTotalPageFile);
        printf("\t%ld available paging file.\n", stat.dwAvailPageFile);
        printf("\t%lx total bytes virtual memory.\n", stat.dwTotalVirtual);
        printf("\t%lx available bytes virtual memory.\n", stat.dwAvailVirtual);

#define OUT_OF_HEADROOM 90
        if (stat.dwMemoryLoad > OUT_OF_HEADROOM) {
            DPRINT("VDM: system resources deemed to low to start VDM.\n");
            //SetLastError();
            return FALSE;
        }
    }
 
    GetSystemInfo(&inf);
    vdm->hHeap = HeapCreate(0, inf.dwAllocationGranularity, 0);
    if (vdm->hHeap == NULL) {
        DPRINT("VDM: failed to create heap.\n");
        return FALSE;
    }

#define DEFAULT_VDM_IMAGE_SIZE 2000000
    vdm->ImageMem = HeapAlloc(vdm->hHeap, 0, DEFAULT_VDM_IMAGE_SIZE);
    if (vdm->ImageMem == NULL) {
        DPRINT("VDM: failed to allocate image memory from heap %x.\n", vdm->hHeap);
        HeapDestroy(vdm->hHeap);
        vdm->hHeap = NULL;
        return FALSE;
    }
    return TRUE;
}

BOOL
DestroyVDM(PVDM_CONTROL_BLOCK vdm)
{
    BOOL result = TRUE;

    if (vdm->ImageMem != NULL) {
        if (HeapFree(vdm->hHeap, 0, vdm->ImageMem) != FALSE) {
            DPRINT("VDM: failed to free memory from heap %x.\n", vdm->hHeap);
            result = FALSE;
        }
        vdm->ImageMem = NULL;
    }
    if (vdm->hHeap != NULL) {
        if (!HeapDestroy(vdm->hHeap)) {
            DPRINT("VDM: failed to destroy heap %x.\n", vdm->hHeap);
            result = FALSE;
        }
        vdm->hHeap = NULL;
    }
    return result;
}

int STDCALL
WinMain(HINSTANCE hInstance,  HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    VDM_CONTROL_BLOCK VdmCB;
    DWORD Result;
    BOOL Success;
    ULONG i;
    NTSTATUS Status;
    BOOL vdmStarted = FALSE;

    CHAR WelcomeMsg[] = "ReactOS Virtual DOS Machine support.\n";
    CHAR PromptMsg[] = "Type r<cr> to run, s<cr> to shutdown or q<cr> to quit now.";
    CHAR InputBuffer[255];
    
    AllocConsole();
    SetConsoleTitle("ntvdm");
	
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                 WelcomeMsg, strlen(WelcomeMsg),  // wcslen(WelcomeMsg),
                 &Result, NULL);

    if (!CreateVDM(&VdmCB)) {
        DPRINT("VDM: failed to create VDM.\n");
        //SetLastError();
        return 2;
    }
	
	ReadConfigForVDM(&VdmCB);

    if (!LoadConfigDriversForVDM(&(VdmCB.vdmConfig))) {
        DPRINT("VDM: failed to load configuration drivers.\n");
        //SetLastError();
        return 2;
    }
    if (!SetConfigOptionsForVDM(&(VdmCB.vdmAutoexec))) {
        DPRINT("VDM: failed to set configuration options.\n");
        //SetLastError();
        return 3;
    }
		
    GetSystemDirectory(VdmCB.CommandLine, MAX_PATH);
    strcat(VdmCB.CommandLine, "\\hello.exe");
    GetWindowsDirectory(VdmCB.CurrentDirectory, MAX_PATH);

    for (;;) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                     PromptMsg, strlen(PromptMsg),  // wcslen(PromptMsg),
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

        if (InputBuffer[0] == 'r' || InputBuffer[0] == 'R') {
            if (!vdmStarted) {
                if (StartVDM(&VdmCB)) {
                    vdmStarted = TRUE;
                } else {
                    DPRINT("VDM: failed to start.\n");
                }
            } else {
                DPRINT("VDM: already started.\n");
            }
        }
        if (InputBuffer[0] == 's' || InputBuffer[0] == 'S') {
            if (vdmStarted) {
                if (ShutdownVDM(&VdmCB)) {
                    vdmStarted = FALSE;
                } else {
                    DPRINT("VDM: failed to shutdown.\n");
                }
            } else {
                DPRINT("VDM: not started.\n");
            }
        }
        if (InputBuffer[0] == 'q' || InputBuffer[0] == 'Q') {
            break;
        }
    }

    if (!ShutdownVDM(&VdmCB)) {
        DPRINT("VDM: failed to cleanly shutdown VDM.\n");
        //SetLastError();
        return 5;
    }

    if (!DestroyVDM(&VdmCB)) {
        DPRINT("VDM: failed to cleanly destroy VDM.\n");
        //SetLastError();
        return 6;
    }

    ExitProcess(0);
    return 0;
}
