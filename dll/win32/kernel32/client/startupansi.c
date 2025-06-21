/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/proc.c
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

/* FUNCTIONS ****************************************************************/

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

                        /* Someone beat us to it, use their data instead */
                        StartupInfo = BaseAnsiStartupInfo;
                        Status = STATUS_SUCCESS;

                        /* We're going to free our own stuff, but not raise */
                        RtlFreeAnsiString(&TitleString);
                    }
                    RtlFreeAnsiString(&DesktopString);
                }
                RtlFreeAnsiString(&ShellString);
            }
            RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo);
        }
        else
        {
            /* No memory, fail */
            Status = STATUS_NO_MEMORY;
        }

        /* Raise an error unless we got here due to the race condition */
        if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
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