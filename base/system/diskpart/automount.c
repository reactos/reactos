/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/automount.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


EXIT_CODE
automount_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    BOOL bDisable = FALSE, bEnable = FALSE, bScrub = FALSE;
#if 0
    BOOL bNoErr = FALSE;
#endif
    INT i;
    BOOL Result, State;

    DPRINT("Automount()\n");

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
#if 0
            bNoErr = TRUE;
#endif
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"disable") == 0)
        {
            /* set automount state */
            bDisable = TRUE;
        }
        else if (_wcsicmp(argv[i], L"enable") == 0)
        {
            /* set automount state */
            bEnable = TRUE;
        }
        else if (_wcsicmp(argv[i], L"scrub") == 0)
        {
            /* scrub automount */
            bScrub = TRUE;
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    DPRINT("bDisable %u\n", bDisable);
    DPRINT("bEnable  %u\n", bEnable);
    DPRINT("bScrub   %u\n", bScrub);

    if (bDisable && bEnable)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if ((bDisable == FALSE) && (bEnable == FALSE) && (bScrub == FALSE))
    {
        DPRINT("Show automount\n");
        Result = GetAutomountState(&State);
        if (Result == FALSE)
        {
//            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }

        if (State)
            ConResPuts(StdOut, IDS_AUTOMOUNT_ENABLED);
        else
            ConResPuts(StdOut, IDS_AUTOMOUNT_DISABLED);
        ConPuts(StdOut, L"\n");
    }

    if (bDisable)
    {
        DPRINT("Disable automount\n");
        Result = SetAutomountState(FALSE);
        if (Result == FALSE)
        {
//            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }

        ConResPuts(StdOut, IDS_AUTOMOUNT_DISABLED);
        ConPuts(StdOut, L"\n");
    }

    if (bEnable)
    {
        DPRINT("Enable automount\n");
        Result = SetAutomountState(TRUE);
        if (Result == FALSE)
        {
//            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }

        ConResPuts(StdOut, IDS_AUTOMOUNT_ENABLED);
        ConPuts(StdOut, L"\n");
    }

    if (bScrub)
    {
        DPRINT("Scrub automount\n");
        Result = ScrubAutomount();
        if (Result == FALSE)
        {
//            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }

        ConResPuts(StdOut, IDS_AUTOMOUNT_SCRUBBED);
        ConPuts(StdOut, L"\n");
    }

    return EXIT_SUCCESS;
}
