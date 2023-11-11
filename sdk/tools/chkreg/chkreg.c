/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "chkreg.h"

/* FUNCTIONS ****************************************************************/

static
void
ChkRegShowUsage(
    void)
{
    printf("Usage: chkreg /h /a /r /v HIVE_FILE LOG_FILE\n\n"
           "  /a HIVE_FILE          - Analyze a registry hive.\n"
           "  /r HIVE_FILE LOG_FILE - Recover a damaged hive with a log (currently it's not implemented yet).\n"
           "  /h                    - Show help.\n"
           "  /v                    - Verbose mode (currently it's not implemented yet).\n");
}

static
void
ChkRegBuildFilePath(
    IN OUT char *Destination,
    IN char *Source)
{
    INT i;

    i = 0;
    while (Source[i] != 0)
    {
#ifdef _WIN32
        if (Source[i] == '/')
        {
            Destination[i] = '\\';
        }
#else
        if (Source[i] == '\\')
        {
            Destination[i] = '/';
        }
#endif
        else
        {
            Destination[i] = Source[i];
        }

        i++;
    }

    Destination[i] = 0;
}

int
main(int argc, char *argv[])
{
    BOOLEAN Success;
    INT i;
    BOOLEAN AnalyzeHive = FALSE;
    CHAR HiveName[260] = "";

    /* Display the usage description if the user only typed the program name */
    if (argc < 2)
    {
        ChkRegShowUsage();
        return -1;
    }

    /* Parse the arguments from the command line */
    for (i = 1; i < argc && *argv[i] == '/'; i++)
    {
        /* Display the usage description */
        if (argv[i][1] == 'h' && argv[i][2] == 0)
        {
            ChkRegShowUsage();
            return -1;
        }

        /* The user wants to analyze its registry hive */
        if (argv[i][1] == 'a')
        {
            /* Grab the hive name */
            AnalyzeHive = TRUE;
            ChkRegBuildFilePath(HiveName, argv[i] + 3);
            break;
        }
        /* The user wants to repair its hive with a log */
        else if (argv[i][1] == 'r')
        {
            printf("Repair mode with a hive log is not implemented yet!\n");
            return -1;
        }
        /* The user wants verbose mode whilst analyzing its hive */
        else if (argv[i][1] == 'v')
        {
            printf("Verbose mode is not implemented yet!\n");
            return -1;
        }
        else
        {
            fprintf(stderr, "Unrecognized command option: %s\n", argv[i]);
            return -1;
        }
    }

    /* The user asked to analyze its hive, ensure he submitted a hive file name */
    if (AnalyzeHive)
    {
        if (!*HiveName)
        {
            fprintf(stderr, "The hive name is missing!\n");
            return -1;
        }

        /* Analyze it now */
        Success = ChkRegAnalyzeHive(HiveName);
        if (!Success)
        {
            fprintf(stderr, "Hive analization finished, the hive has damaged parts!\n");
            return -1;
        }
    }

    return 0;
}

/* EOF */
