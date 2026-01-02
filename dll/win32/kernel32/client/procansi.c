/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/procansi.c
 * PURPOSE:         Process functions
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LPSTARTUPINFOA BaseAnsiStartupInfo = NULL;
WaitForInputIdleType UserWaitForInputIdleRoutine;

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
VOID
WINAPI
RegisterWaitForInputIdle(IN WaitForInputIdleType lpfnRegisterWaitForInputIdle)
{
    /* Write the global function pointer */
    UserWaitForInputIdleRoutine = lpfnRegisterWaitForInputIdle;
}

#if (NTDDI_VERSION < NTDDI_WIN8)
/*
 * @implemented
 */
BOOL
WINAPI
CreateProcessInternalW(IN HANDLE hUserToken,
                       IN LPCWSTR lpApplicationName,
                       IN LPWSTR lpCommandLine,
                       IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
                       IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       IN BOOL bInheritHandles,
                       IN DWORD dwCreationFlags,
                       IN LPVOID lpEnvironment,
                       IN LPCWSTR lpCurrentDirectory,
                       IN LPSTARTUPINFOW lpStartupInfo,
                       IN LPPROCESS_INFORMATION lpProcessInformation,
                       OUT PHANDLE hNewToken);

/*
 * @implemented
 */
BOOL
WINAPI
CreateProcessInternalA(HANDLE hToken,
                       LPCSTR lpApplicationName,
                       LPSTR lpCommandLine,
                       LPSECURITY_ATTRIBUTES lpProcessAttributes,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       BOOL bInheritHandles,
                       DWORD dwCreationFlags,
                       LPVOID lpEnvironment,
                       LPCSTR lpCurrentDirectory,
                       LPSTARTUPINFOA lpStartupInfo,
                       LPPROCESS_INFORMATION lpProcessInformation,
                       PHANDLE hNewToken)
{
    UNICODE_STRING CommandLine;
    UNICODE_STRING ApplicationName;
    UNICODE_STRING CurrentDirectory;
    BOOL bRetVal;
    STARTUPINFOW StartupInfo;

    DPRINT("dwCreationFlags %x, lpEnvironment %p, lpCurrentDirectory %p, "
            "lpStartupInfo %p, lpProcessInformation %p\n",
            dwCreationFlags, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation);

    /* Copy Startup Info */
    RtlMoveMemory(&StartupInfo, lpStartupInfo, sizeof(*lpStartupInfo));

    /* Initialize all strings to nothing */
    CommandLine.Buffer = NULL;
    ApplicationName.Buffer = NULL;
    CurrentDirectory.Buffer = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpTitle = NULL;

    /* Convert the Command line */
    if (lpCommandLine)
    {
        Basep8BitStringToDynamicUnicodeString(&CommandLine,
                                              lpCommandLine);
    }

    /* Convert the Name and Directory */
    if (lpApplicationName)
    {
        Basep8BitStringToDynamicUnicodeString(&ApplicationName,
                                              lpApplicationName);
    }
    if (lpCurrentDirectory)
    {
        Basep8BitStringToDynamicUnicodeString(&CurrentDirectory,
                                              lpCurrentDirectory);
    }

    /* Now convert Startup Strings */
    if (lpStartupInfo->lpReserved)
    {
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpReserved,
                                           &StartupInfo.lpReserved);
    }
    if (lpStartupInfo->lpDesktop)
    {
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpDesktop,
                                           &StartupInfo.lpDesktop);
    }
    if (lpStartupInfo->lpTitle)
    {
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpTitle,
                                           &StartupInfo.lpTitle);
    }

    /* Call the Unicode function */
    bRetVal = CreateProcessInternalW(hToken,
                                     ApplicationName.Buffer,
                                     CommandLine.Buffer,
                                     lpProcessAttributes,
                                     lpThreadAttributes,
                                     bInheritHandles,
                                     dwCreationFlags,
                                     lpEnvironment,
                                     CurrentDirectory.Buffer,
                                     &StartupInfo,
                                     lpProcessInformation,
                                     hNewToken);

    /* Clean up */
    RtlFreeUnicodeString(&ApplicationName);
    RtlFreeUnicodeString(&CommandLine);
    RtlFreeUnicodeString(&CurrentDirectory);
    RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo.lpDesktop);
    RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo.lpReserved);
    RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo.lpTitle);

    /* Return what Unicode did */
    return bRetVal;
}

/*
 * FUNCTION: The CreateProcess function creates a new process and its
 * primary thread. The new process executes the specified executable file
 * ARGUMENTS:
 *
 *     lpApplicationName = Pointer to name of executable module
 *     lpCommandLine = Pointer to command line string
 *     lpProcessAttributes = Process security attributes
 *     lpThreadAttributes = Thread security attributes
 *     bInheritHandles = Handle inheritance flag
 *     dwCreationFlags = Creation flags
 *     lpEnvironment = Pointer to new environment block
 *     lpCurrentDirectory = Pointer to current directory name
 *     lpStartupInfo = Pointer to startup info
 *     lpProcessInformation = Pointer to process information
 *
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
CreateProcessA(LPCSTR lpApplicationName,
               LPSTR lpCommandLine,
               LPSECURITY_ATTRIBUTES lpProcessAttributes,
               LPSECURITY_ATTRIBUTES lpThreadAttributes,
               BOOL bInheritHandles,
               DWORD dwCreationFlags,
               LPVOID lpEnvironment,
               LPCSTR lpCurrentDirectory,
               LPSTARTUPINFOA lpStartupInfo,
               LPPROCESS_INFORMATION lpProcessInformation)
{
    /* Call the internal (but exported) version */
    return CreateProcessInternalA(NULL,
                                  lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                  lpProcessInformation,
                                  NULL);
}
#endif // NTDDI_VERSION < NTDDI_WIN8

/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoA(IN LPSTARTUPINFOA lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;
    ANSI_STRING TitleString, ShellString, DesktopString;
    LPSTARTUPINFOA StartupInfo;
    NTSTATUS Status;

    /* Get the cached information as well as the PEB parameters */
    StartupInfo = BaseAnsiStartupInfo;
    Params = NtCurrentPeb()->ProcessParameters;

    /* Check if this is the first time we have to get the cached version */
    while (!StartupInfo)
    {
        /* Create new ANSI startup info */
        StartupInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      sizeof(*StartupInfo));
        if (StartupInfo)
        {
            /* Zero out string pointers in case we fail to create them */
            StartupInfo->lpReserved = NULL;
            StartupInfo->lpDesktop = NULL;
            StartupInfo->lpTitle = NULL;

            /* Set the size */
            StartupInfo->cb = sizeof(*StartupInfo);

            /* Copy what's already stored in the PEB */
            StartupInfo->dwX = Params->StartingX;
            StartupInfo->dwY = Params->StartingY;
            StartupInfo->dwXSize = Params->CountX;
            StartupInfo->dwYSize = Params->CountY;
            StartupInfo->dwXCountChars = Params->CountCharsX;
            StartupInfo->dwYCountChars = Params->CountCharsY;
            StartupInfo->dwFillAttribute = Params->FillAttribute;
            StartupInfo->dwFlags = Params->WindowFlags;
            StartupInfo->wShowWindow = (WORD)Params->ShowWindowFlags;
            StartupInfo->cbReserved2 = Params->RuntimeData.Length;
            StartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeData.Buffer;
            StartupInfo->hStdInput = Params->StandardInput;
            StartupInfo->hStdOutput = Params->StandardOutput;
            StartupInfo->hStdError = Params->StandardError;

            /* Copy shell info string */
            Status = RtlUnicodeStringToAnsiString(&ShellString,
                                                  &Params->ShellInfo,
                                                  TRUE);
            if (NT_SUCCESS(Status))
            {
                /* Save it */
                StartupInfo->lpReserved = ShellString.Buffer;

                /* Copy desktop info string */
                Status = RtlUnicodeStringToAnsiString(&DesktopString,
                                                      &Params->DesktopInfo,
                                                      TRUE);
                if (NT_SUCCESS(Status))
                {
                    /* Save it */
                    StartupInfo->lpDesktop = DesktopString.Buffer;

                    /* Copy window title string */
                    Status = RtlUnicodeStringToAnsiString(&TitleString,
                                                          &Params->WindowTitle,
                                                          TRUE);
                    if (NT_SUCCESS(Status))
                    {
                        /* Save it */
                        StartupInfo->lpTitle = TitleString.Buffer;

                        /* We finished with the ANSI version, try to cache it */
                        if (!InterlockedCompareExchangePointer((PVOID*)&BaseAnsiStartupInfo,
                                                               StartupInfo,
                                                               NULL))
                        {
                            /* We were the first thread through, use the data */
                            break;
                        }

                        /* Someone beat us to it, we will use their data instead */
                        Status = STATUS_SUCCESS;

                        /* We're going to free our own stuff, but not raise */
                        RtlFreeAnsiString(&TitleString);
                    }
                    RtlFreeAnsiString(&DesktopString);
                }
                RtlFreeAnsiString(&ShellString);
            }
            RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo);

            /* Get the cached information again: either still NULL or set by another thread */
            StartupInfo = BaseAnsiStartupInfo;
        }
        else
        {
            /* No memory, fail */
            Status = STATUS_NO_MEMORY;
        }

        /* Raise an error unless we got here due to the race condition */
        if (!StartupInfo) RtlRaiseStatus(Status);
    }

    /* Now copy from the cached ANSI version */
    lpStartupInfo->cb = StartupInfo->cb;
    lpStartupInfo->lpReserved = StartupInfo->lpReserved;
    lpStartupInfo->lpDesktop = StartupInfo->lpDesktop;
    lpStartupInfo->lpTitle = StartupInfo->lpTitle;
    lpStartupInfo->dwX = StartupInfo->dwX;
    lpStartupInfo->dwY = StartupInfo->dwY;
    lpStartupInfo->dwXSize = StartupInfo->dwXSize;
    lpStartupInfo->dwYSize = StartupInfo->dwYSize;
    lpStartupInfo->dwXCountChars = StartupInfo->dwXCountChars;
    lpStartupInfo->dwYCountChars = StartupInfo->dwYCountChars;
    lpStartupInfo->dwFillAttribute = StartupInfo->dwFillAttribute;
    lpStartupInfo->dwFlags = StartupInfo->dwFlags;
    lpStartupInfo->wShowWindow = StartupInfo->wShowWindow;
    lpStartupInfo->cbReserved2 = StartupInfo->cbReserved2;
    lpStartupInfo->lpReserved2 = StartupInfo->lpReserved2;

    /* Check if the shell is hijacking the handles for other features */
    if (lpStartupInfo->dwFlags &
        (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_SHELLPRIVATE))
    {
        /* It isn't, so we can return the raw values */
        lpStartupInfo->hStdInput = StartupInfo->hStdInput;
        lpStartupInfo->hStdOutput = StartupInfo->hStdOutput;
        lpStartupInfo->hStdError = StartupInfo->hStdError;
    }
    else
    {
        /* It is, so make sure nobody uses these as console handles */
        lpStartupInfo->hStdInput = INVALID_HANDLE_VALUE;
        lpStartupInfo->hStdOutput = INVALID_HANDLE_VALUE;
        lpStartupInfo->hStdError = INVALID_HANDLE_VALUE;
    }
}

/*
 * @implemented
 */
UINT
WINAPI
DECLSPEC_HOTPATCH
WinExec(LPCSTR lpCmdLine,
        UINT uCmdShow)
{
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION  ProcessInformation;
    DWORD dosErr;

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    StartupInfo.dwFlags = 0;

    if (!CreateProcessA(NULL,
                        (PVOID)lpCmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation))
    {
        dosErr = GetLastError();
        return dosErr < 32 ? dosErr : ERROR_BAD_FORMAT;
    }

    if (NULL != UserWaitForInputIdleRoutine)
    {
        UserWaitForInputIdleRoutine(ProcessInformation.hProcess,
                                           10000);
    }

    NtClose(ProcessInformation.hProcess);
    NtClose(ProcessInformation.hThread);

    return 33; /* Something bigger than 31 means success. */
}

/* EOF */
