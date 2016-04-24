/*
 * PROJECT:         ReactOS Build Tools [Keyboard Layout Compiler]
 * LICENSE:         BSD - See COPYING.BSD in the top level directory
 * FILE:            tools/kbdtool/main.c
 * PURPOSE:         Main Logic Loop
 * PROGRAMMERS:     ReactOS Foundation
 */

/* INCLUDES *******************************************************************/

#include "kbdtool.h"

/* GLOBALS ********************************************************************/

/* Internal tool data */
ULONG gVersion = 3;
ULONG gSubVersion = 40;

/* Input file */
PCHAR gpszFileName;
FILE* gfpInput;

/* Command-line parameters */
BOOLEAN UnicodeFile, Verbose, NoLogo, FallbackDriver, SanityCheck, SourceOnly;
ULONG BuildType;

/* FUNCTIONS ******************************************************************/

VOID
PrintUsage(VOID)
{
    /* This is who we are */
    printf("\nKbdTool v%d.%02d - convert keyboard text file to C file or a keyboard layout DLL\n\n",
           gVersion, gSubVersion);

    /* This is what we do */
    printf("Usage: KbdTool [-v] [-n] [-w] [-k] [-n] [-u|a] [-i|x|m|o|s] FILE\n\n");
    printf("\t[-?] display this message\n");
    printf("\t[-n] no logo or normal build information displayed\n\n");
    printf("\t[-a] Uses non-Unicode source files (default)\n");
    printf("\t[-u] Uses Unicode source files\n\n");
    printf("\t[-v] Verbose diagnostics (and warnings, with -w)\n");
    printf("\t[-w] display extended Warnings\n\n");
    printf("\t[-x] Builds for x86 (default)\n");
    printf("\t[-i] Builds for IA64\n");
    printf("\t[-m] Builds for AMD64\n");
    printf("\t[-o] Builds for WOW64\n");
    printf("\t[-s] Generate Source files (no build)\n\n");
    printf("\tFILE The source keyboard file (required)\n\n");

    /* Extra hints */
    printf("\t-u/-a are mutually exclusive; kbdutool will use the last one if you specify more than one.\n");
    printf("\t-i/-x/-m/-o-s will exhibit the same behavior when than one of them is specified.\n\n");

    /* Quit */
    exit(1);
    printf("should not be here");
}

INT
main(INT argc,
     PCHAR* argv)
{
    ULONG i, ErrorCode, FailureCode;
    CHAR Option;
    PCHAR OpenFlags;
    CHAR BuildOptions[16] = {0};

    /* Loop for parameter */
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '/' && argv[i][0] != '-')
            break;

        if (argv[i][1] && !argv[i][2])
            Option = argv[i][1];
        else
            Option = 0;

        /* Check supported options */
        switch (Option)
        {
            /* ASCII File */
            case 'A':
            case 'a':
                UnicodeFile = 0;
                break;

            /* UNICODE File */
            case 'U':
            case 'u':
                UnicodeFile = 1;
                break;

            /* Verbose */
            case 'V':
            case 'v':
                Verbose = 1;
                break;

            /* No logo */
            case 'N':
            case 'n':
                NoLogo = 1;
                break;

            /* Fallback driver */
            case 'K':
            case 'k':
                FallbackDriver = 1;
                break;

            /* Sanity Check */
            case 'W':
            case 'w':
                SanityCheck = 1;
                break;

            /* Itanium */
            case 'I':
            case 'i':
                BuildType = 1;
                break;

            /* X86 */
            case 'X':
            case 'x':
                BuildType = 0;
                break;

            /* AMD64 */
            case 'M':
            case 'm':
                BuildType = 2;
                break;

            /* WOW64 */
            case 'O':
            case 'o':
                BuildType = 3;
                break;

            /* Source only */
            case 'S':
            case 's':
                SourceOnly = 1;
                break;

            default:
                /* If you got here, the options are invalid or missing */
                PrintUsage();
                break;
        }
    }

    /* Do we have no options? */
    if (i == argc) PrintUsage();

    /* Should we announce ourselves? */
    if (!NoLogo)
    {
        /* This is who we are */
        printf("\nKbdTool v%d.%02d - convert keyboard text file to C file or a keyboard layout DLL\n\n",
               gVersion, gSubVersion);
    }

    /* Save the file name */
    gpszFileName = argv[i];

    /* Open either as binary or text */
    OpenFlags = "rb";
    if (!UnicodeFile) OpenFlags = "rt";

    /* Open a handle to the file */
    gfpInput = fopen(gpszFileName, OpenFlags);
    if (!gfpInput)
    {
        /* Couldn't open it */
        printf("Unable to open '%s' for read.\n", gpszFileName);
        exit(1);
    }

    /* Should we print out what we're doing? */
    if (!NoLogo)
    {
        /* Are we only building the source files? */
        if (SourceOnly)
        {
            /* Then there's no target architecture */
            strcpy(BuildOptions, "source files");
        }
        else
        {
            /* Take a look at the target architecture*/
            switch (BuildType)
            {
                /* Print the appropriate message depending on what was chosen */
                case 0:
                    strcpy(BuildOptions, "i386/x86");
                    break;
                case 1:
                    strcpy(BuildOptions, "ia64");
                    break;
                case 2:
                    strcpy(BuildOptions, "amd64/x64");
                    break;
                case 3:
                    strcpy(BuildOptions, "wow64");
                    break;
                default:
                    strcpy(BuildOptions, "unknown purpose");
                    break;
            }
        }

        /* Now inform the user */
        printf("Compiling layout information from '%s' for %s.\n", gpszFileName, BuildOptions);
    }

    /* Now parse the keywords */
    FailureCode = DoParsing();

    /* Should we build? */
    if (!(SourceOnly) && !(FallbackDriver)) ErrorCode = 0;//DoBuild();

    /* Did everything work? */
    if (FailureCode == 0)
    {
        /* Tell the user, if he cares */
        if (!NoLogo) printf("All tasks completed successfully.\n");
    }
    else
    {
        /* Print the failure code */
        printf("\n     %13d\n", FailureCode);
    }

    /* Return the error code */
    return ErrorCode;
}

/* EOF */
