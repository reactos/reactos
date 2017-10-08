/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS shutdown/logoff utility
 * FILE:            base/applications/shutdown/misc.c
 * PURPOSE:         Misc. functions used for the shutdown utility
 * PROGRAMMERS:     Lee Schroeder
 */

#include "precomp.h"

#include <stdio.h>

const DWORD defaultReason = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER;

REASON shutdownReason[] =
{
    {L"U" , 0,  0, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER},                                             /* Other (Unplanned) */
    {L"E" , 0,  0, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER},                                             /* Other (Unplanned) */
    {L"EP", 0,  0, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED},                 /* Other (Planned) */
    {L"U" , 0,  5, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_HUNG},                                              /* Other Failure: System Unresponsive */
    {L"E" , 1,  1, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE},                                    /* Hardware: Maintenance (Unplanned) */
    {L"EP", 1,  1, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED},        /* Hardware: Maintenance (Planned) */
    {L"E" , 1,  2, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION},                                   /* Hardware: Installation (Unplanned) */
    {L"EP", 1,  2, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED},       /* Hardware: Installation (Planned) */
    {L"P" , 2,  3, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED},     /* Operating System: Upgrade (Planned) */
    {L"E" , 2,  4, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG},                                /* Operating System: Reconfiguration (Unplanned) */
    {L"EP", 2,  4, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED},    /* Operating System: Reconfiguration (Planned) */
    {L"P" , 2, 16, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED},    /* Operating System: Service pack (Planned) */
    {L"U" , 2, 17, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_HOTFIX},                                  /* Operating System: Hotfix (Unplanned) */
    {L"P" , 2, 17, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_HOTFIX | SHTDN_REASON_FLAG_PLANNED},      /* Operating System: Hotfix (Planned) */
    {L"U" , 2, 18, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SECURITYFIX},                             /* Operating System: Security fix (Unplanned) */
    {L"P" , 2, 18, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_SECURITYFIX | SHTDN_REASON_FLAG_PLANNED}, /* Operating System: Security fix (Planned) */
    {L"E" , 4,  1, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE},                                 /* Application: Maintenance (Unplanned) */
    {L"EP", 4,  1, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE | SHTDN_REASON_FLAG_PLANNED},     /* Application: Maintenance (Planned) */
    {L"EP", 4,  2, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED},    /* Application: Installation (Planned) */
    {L"E" , 4,  5, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_HUNG},                                        /* Application: Unresponsive */
    {L"E" , 4,  6, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_UNSTABLE},                                    /* Application: Unstable */
    {L"U" , 5, 15, SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_BLUESCREEN},                                       /* System Failure: Stop Error */
    {L"E" , 5, 19, SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY},                                         /* Security Issue */
    {L"U" , 5, 19, SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY},                                         /* Security Issue */
    {L"EP", 5, 19, SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_SECURITY | SHTDN_REASON_FLAG_PLANNED},             /* Security Issue (Planned) */
    {L"E" , 5, 20, SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_NETWORK_CONNECTIVITY},                             /* Loss of Network Connectivity (Unplanned) */
    {L"U" , 6, 11, SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_CORDUNPLUGGED},                                     /* Power Failure: Cord Unplugged */
    {L"U" , 6, 12, SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_ENVIRONMENT},                                       /* Power Failure: Environment */
    {L"P" , 7,  0, SHTDN_REASON_MAJOR_POWER | SHTDN_REASON_MINOR_ENVIRONMENT}                                        /* Legacy API shutdown (Planned) */
};

/*
 * This command helps to work around the fact that the shutdown utility has
 * different upper limits for the comment flag since each version of Windows
 * seems to have different upper limits.
 */
BOOL CheckCommentLength(LPCWSTR comment)
{
    DWORD finalLength = 0;
    size_t strLength = 0;
    DWORD osVersion = 0;
    DWORD osMajorVersion = 0;
    DWORD osMinorVersion = 0;

    /* An empty string is always valid. */
    if (!comment || *comment == 0)
        return TRUE;

    /* Grab the version of the current Operating System. */
    osVersion = GetVersion();

    osMajorVersion = (DWORD)(LOBYTE(LOWORD(osVersion)));
    osMinorVersion = (DWORD)(HIBYTE(LOWORD(osVersion)));

    /*
     * Check to make sure that the proper length is being used
     * based upon the version of Windows currently being used.
     */
    if (osMajorVersion == 5) /* Windows XP/2003 */
    {
        if ((osMinorVersion == 1) || (osMinorVersion == 2))
        {
            finalLength = 127;
        }
    }
    else if (osMajorVersion == 6) /* Windows Vista/7/2008 */
    {
        if ((osMinorVersion == 0) || (osMinorVersion == 1))
        {
            finalLength = 512;
        }
    }

    /* Grab the length of the comment string. */
    strLength = wcslen(comment);

    /*
     * Compare the size of the string to make sure
     * it fits with the current version of Windows,
     * and return TRUE or FALSE accordingly.
     */
    return (strLength <= finalLength);
}

/*
 * This function parses the reason code to a usable format that will specify
 * why the user wants to shut the computer down. Although this is used for
 * both client and server environments, use of a reason code is more important
 * in a server environment since servers are supposed to be on all the time
 * for easier access.
 */
DWORD ParseReasonCode(LPCWSTR code)
{
    PREASON reasonptr;
    int majorCode = 0;
    int minorCode = 0;
    LPWSTR tmpPrefix = NULL;
    size_t codeSize;

    /* If no reason code is specified, use "Other (Unplanned)" as the default option */
    if(code == NULL)
    {
        return defaultReason;
    }
    else
    {
        /* Store the size of the code so we can use it later */
        codeSize = (size_t)wcslen(code);

        /* A colon cannot be the first or last character in the reason code */
        if ((code[0] == L':') || (code[codeSize] == L':'))
        {
            return defaultReason;
        }

        /* The minimum length that a reason code can be is 5-7 characters in length */
        if ((codeSize < 5) || (codeSize > 7))
        {
            return defaultReason;
        }

        /* TODO: Add code for reason parsing here. */

        /* Make sure that the major and minor codes are within size limits */
        if ((majorCode > 7 ) || (majorCode < 0) ||
            (minorCode > 20) || (minorCode < 0))
        {
            return defaultReason;
        }

        /* Figure out what flags to return */
        for (reasonptr = shutdownReason ; reasonptr->prefix ; reasonptr++)
        {
            if ((majorCode == reasonptr->major) &&
                (minorCode == reasonptr->minor) &&
                (_wcsicmp(tmpPrefix, reasonptr->prefix) != 0))
            {
                return reasonptr->flag;
            }
        }
    }

    return defaultReason;
}

/* Writes the last error as both text and error code to the console */
VOID DisplayError(DWORD dwError)
{
    ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
               NULL, dwError, LANG_USER_DEFAULT);
    ConPrintf(StdErr, L"Error code: %lu\n", dwError);
}

/* EOF */
