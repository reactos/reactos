/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS shutdown/logoff utility
 * FILE:            base/applications/shutdown/shutdown.c
 * PURPOSE:         Initiate logoff, shutdown or reboot of the system
 */

#include "precomp.h"

#include <stdlib.h>
#include <tchar.h>
#include <powrprof.h>

/*
 * This takes strings from a resource stringtable
 * and outputs it to the console.
 */
VOID PrintResourceString(INT resID, ...)
{
    WCHAR tmpBuffer[MAX_BUFFER_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadStringW(GetModuleHandle(NULL), resID, tmpBuffer, MAX_BUFFER_SIZE);
    vfwprintf(stdout, tmpBuffer, arg_ptr);
    va_end(arg_ptr);
}

/*
 * Takes the commandline arguments, and creates a
 * struct which matches the arguments supplied.
 */
static DWORD
ParseArguments(struct CommandLineOptions* pOpts, int argc, WCHAR *argv[])
{
    int index;

    if (!pOpts)
        return ERROR_INVALID_PARAMETER;

    /* Reset all flags in struct */
    pOpts->abort = FALSE;
    pOpts->force = FALSE;
    pOpts->logoff = FALSE;
    pOpts->restart = FALSE;
    pOpts->shutdown = FALSE;
    pOpts->document_reason = FALSE;
    pOpts->hibernate = FALSE;
    pOpts->shutdown_delay = 30;
    pOpts->remote_system = NULL;
    pOpts->reason = ParseReasonCode(NULL); /* NOTE: NEVER use 0 here since it can delay the shutdown. */
    pOpts->message = NULL;
    pOpts->show_gui = FALSE;

    /*
     * Determine which flags the user has specified
     * to the program so we can use them later.
     */
    for (index = 1; index < argc; index++)
    {
        if (argv[index][0] == L'-' || argv[index][0] == L'/')
        {
            switch (towlower(argv[index][1]))
            {
                case L'?': /* Help */
                    PrintResourceString(IDS_USAGE);
                    return ERROR_SUCCESS;

                case L'a': /* Cancel delayed shutdown */
                    pOpts->abort = TRUE;
                    break;

                case L'c': /* Comment on reason for shutdown */
                    if(CheckCommentLength(argv[index+1]))
                    {
                        if (index+1 <= argc)
                            pOpts->message = argv[index+1];
                        else
                            return ERROR_INVALID_DATA;
                        index++;
                    }
                    else
                    {
                        PrintResourceString(IDS_ERROR_MAX_COMMENT_LENGTH);
                        return ERROR_BAD_LENGTH;
                    }
                    break;

                case L'd': /* Reason code [p|u:]xx:yy */
                    if (index+1 <= argc)
                        pOpts->reason = ParseReasonCode(argv[index+1]);
                    else
                        return ERROR_INVALID_DATA;
                    index++;
                    break;

                case L'e': /* Documents reason for shutdown */
                    /* TODO: Determine what this flag does exactly. */
                    pOpts->document_reason = TRUE;
                    break;

                case L'f': /* Force shutdown without warning */
                    pOpts->force = TRUE;
                    break;

                case L'h': /* Hibernate the local computer */
                    pOpts->hibernate = TRUE;
                    break;

                case L'i': /* Shows GUI version of the tool */
                    pOpts->show_gui = TRUE;
                    break;

                case L'l': /* Logoff the current user */
                    pOpts->logoff = TRUE;
                    break;

                case L'm': /* Target remote systems (UNC name/IP address) */
                    pOpts->remote_system = argv[index+1];
                    break;

                case L'p': /* Turn off local computer with no warning/time-out */
                    pOpts->force = TRUE;
                    pOpts->shutdown_delay = 0;
                    break;

                case L'r': /* Restart computer */
                    pOpts->restart = TRUE;
                    break;

                case L's': /* Shutdown */
                    pOpts->shutdown = TRUE;
                    break;

                case L't': /* Shutdown delay */
                    pOpts->shutdown_delay = _wtoi(argv[index+1]);
                    if (pOpts->shutdown_delay > 0) 
                        pOpts->force = TRUE;
                    break;

                default:
                    /* Unknown arguments will exit the program. */
                    PrintResourceString(IDS_USAGE);
                    return ERROR_SUCCESS;
            }
        }
    }

    return ERROR_SUCCESS;
}

static DWORD
EnablePrivilege(LPCWSTR lpszPrivilegeName, BOOL bEnablePrivilege)
{
    DWORD  dwRet  = ERROR_SUCCESS;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES,
                         &hToken))
    {
        TOKEN_PRIVILEGES tp;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

        if (LookupPrivilegeValueW(NULL,
                                  lpszPrivilegeName,
                                  &tp.Privileges[0].Luid))
        {
            if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
            {
                if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
                    dwRet = ERROR_NOT_ALL_ASSIGNED;
            }
            else
            {
                dwRet = GetLastError();
            }
        }
        else
        {
            dwRet = GetLastError();
        }

        CloseHandle(hToken);
    }
    else
    {
        dwRet = GetLastError();
    }

    /* Display the error description if any */
    if (dwRet != ERROR_SUCCESS) DisplayError(dwRet);

    return dwRet;
}

/* Main entry for program */
int wmain(int argc, WCHAR *argv[])
{
    DWORD error = ERROR_SUCCESS;
    struct CommandLineOptions opts;

    if (argc == 1) /* i.e. no commandline arguments given */
    {
        PrintResourceString(IDS_USAGE);
        return EXIT_SUCCESS;
    }

    error = ParseArguments(&opts, argc, argv);
    if (error != ERROR_SUCCESS)
    {
        DisplayError(error);
        return EXIT_FAILURE;
    }

    /* If the user wants to abort a shutdown */
    if (opts.abort)
    {
        /* First, the program has to determine if the shutdown/restart is local
        or remote. This is done since each one requires separate privileges. */
        if (opts.remote_system == NULL)
            EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);
        else
            EnablePrivilege(SE_REMOTE_SHUTDOWN_NAME, TRUE);

        /* Abort the delayed system shutdown specified. */
        if (!AbortSystemShutdownW(opts.remote_system))
        {
            PrintResourceString(IDS_ERROR_ABORT);
            DisplayError(GetLastError());
            return EXIT_FAILURE;
        }
        else
        {
            return EXIT_SUCCESS;
        }
    }

    /*
     * If the user wants to hibernate the computer. Assume
     * that the user wants to wake the computer up from 
     * hibernation and it should not force it on the system.
     */
    if (opts.hibernate)
    {
        if (IsPwrHibernateAllowed())
        {
            EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);
            
            /* The shutdown utility cannot hibernate remote systems */
            if (opts.remote_system != NULL)
            {
                return EXIT_FAILURE;
            }
        
            if (!SetSuspendState(TRUE, FALSE, FALSE))
            {
                PrintResourceString(IDS_ERROR_HIBERNATE);
                DisplayError(GetLastError());
                return EXIT_FAILURE;
            }
            else
            {
                PrintResourceString(IDS_ERROR_HIBERNATE_ENABLED);
                return EXIT_SUCCESS;
            }
        }
        else
        {
            return EXIT_FAILURE;
        }
    }

    /* Both shutdown and restart flags cannot both be true */
    if (opts.shutdown && opts.restart)
    {
        PrintResourceString(IDS_ERROR_SHUTDOWN_REBOOT);
        return EXIT_FAILURE;
    }

    /* Ensure that the timout amount is not too high or a negative number */
    if (opts.shutdown_delay > MAX_SHUTDOWN_TIMEOUT)
    {
        PrintResourceString(IDS_ERROR_TIMEOUT, opts.shutdown_delay);
        return EXIT_FAILURE;
    }

    /* If the user wants a GUI environment */
    if (opts.show_gui)
    {
        if (ShutdownGuiMain(opts))
            return EXIT_SUCCESS;
        else
            return EXIT_FAILURE;
    }

    if (opts.logoff && (opts.remote_system == NULL))
    {
        /*
         * NOTE: Sometimes, shutdown and logoff are used together. If the logoff
         * flag is used by itself, then simply logoff. But if used with shutdown,
         * then skip logging off of the computer and eventually go to the action
         * for shutdown.
         */
        if (!opts.shutdown && !opts.restart)
        {
            EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);

            if (ExitWindowsEx(EWX_LOGOFF, opts.reason))
            {
                return EXIT_SUCCESS;
            }
            else
            {
                PrintResourceString(IDS_ERROR_LOGOFF);
                DisplayError(GetLastError());
                return EXIT_FAILURE;
            }
        }
    }

    /*
     * Since both shutting down the system and restarting calls the exact same
     * function, all we need to know is if we wanted to restart or shutdown.
     */
    if (opts.shutdown || opts.restart)
    {
        /*
         * First, the program has to determine if the shutdown/restart is local
         * or remote. This is done since each one requires separate privileges.
         */
        if (opts.remote_system == NULL)
        {
            EnablePrivilege(SE_SHUTDOWN_NAME, TRUE);
        }
        else
        {
            /* TODO: Remote shutdown is not supported yet */
            // EnablePrivilege(SE_REMOTE_SHUTDOWN_NAME, TRUE);
            return EXIT_SUCCESS;
        }

        /**
         ** HACK: When InitiateSystemShutdownExW will become really implemented,
         ** activate this line and delete the other...
         **
        if(!InitiateSystemShutdownExW(opts.remote_system,
                                      opts.message,
                                      opts.shutdown_delay,
                                      opts.force,
                                      opts.restart,
                                      opts.reason))
        ***/
        if (!ExitWindowsEx((opts.shutdown ? EWX_SHUTDOWN : EWX_REBOOT) |
                           (opts.force ? EWX_FORCE : 0),
                           opts.reason))
        {
            /*
             * If there is an error, give the proper output depending
             * on whether the user wanted to shutdown or restart.
             */
            if (opts.restart)
                PrintResourceString(IDS_ERROR_RESTART);
            else
                PrintResourceString(IDS_ERROR_SHUTDOWN);
            
            DisplayError(GetLastError());
            return EXIT_FAILURE;
        }
        else
        {
            return EXIT_SUCCESS;
        }
    }

    return EXIT_SUCCESS;
}

/* EOF */
