/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Option init and parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "util.h"
#include "compat.h"
#include "config.h"
#include "help.h"
#include "log2lines.h"
#include "options.h"

char *optchars       = "bcd:fFhl:L:mMP:rR:sS:tTuUvz:";
int   opt_buffered   = 0;        // -b
int   opt_help       = 0;        // -h
int   opt_force      = 0;        // -f
int   opt_exit       = 0;        // -e
int   opt_verbose    = 0;        // -v
int   opt_console    = 0;        // -c
int   opt_mark       = 0;        // -m
int   opt_Mark       = 0;        // -M
char *opt_Pipe       = NULL;     // -P
int   opt_quit       = 0;        // -q (cli only)
int   opt_cli        = 0;        // (cli internal)
int   opt_raw        = 0;        // -r
int   opt_stats      = 0;        // -s
int   opt_Source     = 0;        // -S <opt_Source>[+<opt_SrcPlus>][,<sources_path>]
int   opt_SrcPlus    = 0;        // -S <opt_Source>[+<opt_SrcPlus>][,<sources_path>]
int   opt_twice      = 0;        // -t
int   opt_Twice      = 0;        // -T
int   opt_undo       = 0;        // -u
int   opt_redo       = 0;        // -U
char *opt_Revision   = NULL;     // -R
int   opt_Revision_check = 0;    // -R check
char  opt_dir[PATH_MAX];         // -d <opt_dir>
char  opt_logFile[PATH_MAX];     // -l|L <opt_logFile>
char *opt_mod        = NULL;     // -mod for opt_logFile
char  opt_7z[PATH_MAX];          // -z <opt_7z>
char  opt_scanned[LINESIZE];     // all scanned options
char  opt_SourcesPath[LINESIZE]; //sources path

/* optionInit returns 0 for normal operation, and -1 in case just "loglines.exe" was written.
In such case, the help is shown */

int optionInit(int argc, const char **argv)
{
    int i;
    char *s;

    opt_mod = "a";
    strcpy(opt_dir, "");
    strcpy(opt_logFile, "");
    strcpy(opt_7z, CMD_7Z);
    strcpy(opt_SourcesPath, "");
    if ((s = getenv(SOURCES_ENV)))
        strcpy(opt_SourcesPath, s);
    revinfo.rev = getRevision(NULL, 1);
    revinfo.range = DEF_RANGE;
    revinfo.buildrev = getTBRevision(opt_dir);
    l2l_dbg(1, "Trunk build revision: %d\n", revinfo.buildrev);

    strcpy(opt_scanned, "");

    //The user introduced "log2lines.exe" or "log2lines.exe /?"
    //Let's help the user
    if ((argc == 1) ||
        ((argc == 2) && (argv[1][0] == '/') && (argv[1][1] == '?')))
    {
        opt_help++;
        usage(1);
        return -1;
    }

    for (i = 1; i < argc; i++)
    {

        if ((argv[i][0] == '-') && (i+1 < argc))
        {
            //Because these arguments can contain spaces we cant use getopt(), a known bug:
            switch (argv[i][1])
            {
            case 'd':
                strcpy(opt_dir, argv[i+1]);
                break;
            case 'L':
                opt_mod = "w";
                //fall through
            case 'l':
                strcpy(opt_logFile, argv[i+1]);
                break;
            case 'P':
                free(opt_Pipe);
                opt_Pipe = malloc(LINESIZE);
                strcpy(opt_Pipe, argv[i+1]);
                break;
            case 'z':
                strcpy(opt_7z, argv[i+1]);
                break;
            }
        }

        strcat(opt_scanned, argv[i]);
        strcat(opt_scanned, " ");
    }

    l2l_dbg(4,"opt_scanned=[%s]\n",opt_scanned);

    return 0;
}

int optionParse(int argc, const char **argv)
{
    int i;
    int optCount = 0;
    int opt;

    while (-1 != (opt = getopt(argc, (char **const)argv, optchars)))
    {
        switch (opt)
        {
        case 'b':
            opt_buffered++;
            break;
        case 'c':
            opt_console++;
            break;
        case 'd':
            optCount++;
            //just count, see optionInit()
            break;
        case 'f':
            opt_force++;
            break;
        case 'h':
            opt_help++;
            usage(1);
            return -1;
            break;
        case 'F':
            opt_exit++;
            opt_force++;
            break;
        case 'l':
            optCount++;
            //just count, see optionInit()
            break;
        case 'm':
            opt_mark++;
            break;
        case 'M':
            opt_Mark++;
            break;
        case 'r':
            opt_raw++;
            break;
        case 'P':
            optCount++;
            //just count, see optionInit()
            break;
        case 'R':
            optCount++;
            free(opt_Revision);
            opt_Revision = malloc(LINESIZE);
            sscanf(optarg, "%s", opt_Revision);
            if (strcmp(opt_Revision, "check") == 0)
                opt_Revision_check ++;
            break;
        case 's':
            opt_stats++;
            break;
        case 'S':
            optCount++;
            i = sscanf(optarg, "%d+%d,%s", &opt_Source, &opt_SrcPlus, opt_SourcesPath);
            if (i == 1)
                sscanf(optarg, "%*d,%s", opt_SourcesPath);
            l2l_dbg(3, "Sources option parse result: %d+%d,\"%s\"\n", opt_Source, opt_SrcPlus, opt_SourcesPath);
            if (opt_Source)
            {
                /* need to retranslate for source info: */
                opt_undo++;
                opt_redo++;
                opt_Revision_check ++;
            }
            break;
        case 't':
            opt_twice++;
            break;
        case 'T':
            opt_twice++;
            opt_Twice++;
            break;
        case 'u':
            opt_undo++;
            break;
        case 'U':
            opt_undo++;
            opt_redo++;
            break;
        case 'v':
            opt_verbose++;
            break;
        case 'z':
            optCount++;
            strcpy(opt_7z, optarg);
            break;
        default:
            usage(0);
            return -2;
            break;
        }
        optCount++;
    }
    if(opt_console)
    {
        l2l_dbg(2, "Note: use 's' command in console mode. Statistics option disabled\n");
        opt_stats = 0;
    }
    if (opt_SourcesPath[0])
    {
        strcat(opt_SourcesPath, PATH_STR);
    }
    if (!opt_dir[0])
    {
        strcpy(opt_dir, opt_SourcesPath);
        strcat(opt_dir, DEF_OPT_DIR);
    }

    return optCount;
}

/* EOF */
