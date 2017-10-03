/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS logoff utility
 * FILE:        base/applications/logoff/logoff.c
 * PURPOSE:     Logoff current session, or another session, potentially on another machine
 * AUTHOR:      30.07.2007 - Frode Lillerud
 */

/* Note
 * This application is a lightweight version of shutdown.exe. It is intended
 * to be function-compatible with Windows' system32\logoff.exe application.
 */

#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include <conutils.h>

#include "resource.h"

/* Command-line argument switches */
LPWSTR szRemoteServerName = NULL;
BOOL bVerbose;

//----------------------------------------------------------------------
//
// Writes the last error as both text and error code to the console.
//
//----------------------------------------------------------------------
VOID DisplayError(DWORD dwError)
{
    ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
               NULL, dwError, LANG_USER_DEFAULT);
    ConPrintf(StdErr, L"Error code: %lu\n", dwError);
}

//----------------------------------------------------------------------
//
// Sets flags based on command-line arguments
//
//----------------------------------------------------------------------
BOOL ParseCommandLine(int argc, WCHAR *argv[])
{
    int i;

    // FIXME: Add handling of command-line arguments to select
    // the session number and name, and also name of remote machine.
    // Example: logoff.exe 4 /SERVER:Master
    // should logoff session number 4 on remote machine called Master.

    for (i = 1; i < argc; i++)
    {
        switch (argv[i][0])
        {
        case L'-':
        case L'/':
            // -v (verbose)
            if (argv[i][1] == L'v')
            {
                bVerbose = TRUE;
                break;
            }
            // -? (usage)
            else if (argv[i][1] == L'?')
            {
                /* Will display the Usage */
                return FALSE;
            }
        /* Fall through */
        default:
            /* Invalid parameter detected */
            ConResPuts(StdErr, IDS_ILLEGAL_PARAM);
            return FALSE;
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------
//
// Main entry for program
//
//----------------------------------------------------------------------
int wmain(int argc, WCHAR *argv[])
{
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Parse command line */
    if (!ParseCommandLine(argc, argv))
    {
        ConResPuts(StdOut, IDS_USAGE);
        return 1;
    }

    /* Should we log off session on remote server? */
    if (szRemoteServerName)
    {
        if (bVerbose)
            ConResPuts(StdOut, IDS_LOGOFF_REMOTE);

        // FIXME: Add Remote Procedure Call to logoff user on a remote machine
        ConPuts(StdErr, L"Remote Procedure Call in logoff.exe has not been implemented");
    }
    /* Perform logoff of current session on local machine instead */
    else
    {
        if (bVerbose)
        {
            /* Get resource string and print it */
            ConResPuts(StdOut, IDS_LOGOFF_LOCAL);
        }

        /* Actual logoff */
        if (!ExitWindowsEx(EWX_LOGOFF, 0))
        {
            DisplayError(GetLastError());
            return 1;
        }
    }

    return 0;
}

/* EOF */
