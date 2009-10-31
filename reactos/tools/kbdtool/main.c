/*
 * PROJECT:         ReactOS Build Tools [Keyboard Layout Compiler]
 * LICENSE:         BSD - See COPYING.BSD in the top level directory
 * FILE:            tools/kbdtool/main.c
 * PURPOSE:         Main Logic Loop
 * PROGRAMMERS:     ReactOS Foundation
 */

/* INCLUDES *******************************************************************/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#include <host/typedefs.h>

/* GLOBALS ********************************************************************/

ULONG gVersion = 3;
ULONG gSubVersion = 40;
BOOLEAN UnicodeFile, Verbose, NoLogo, FallbackDriver, SanityCheck, SourceOnly;
ULONG BuildType;

/* FUNCTIONS ******************************************************************/

void 
usage()
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
    _exit(1);
    printf("should not be here");
}

int
main(int argc,
     char** argv)
{
    CHAR Option;
    
    /* Loop for parameter */
    while (TRUE)
    {
        /* Get the options */
        Option = getopt(argc, argv, "aAeEiIkKmMnNOosSuUvVwWxX?");
        if (Option != -1)
        {
            /* Check supported options */
            switch (Option)
            {
                /* ASCII File */
                case 'A':
                case 'a':
                    UnicodeFile = 0;
                    continue;
                    
                /* UNICODE File */
                case 'U':
                case 'u':
                    UnicodeFile = 1;
                    continue;
                    
                /* Verbose */
                case 'V':
                case 'v':
                    Verbose = 1;
                    continue;
                    
                /* No logo */
                case 'N':
                case 'n':
                    NoLogo = 1;
                    continue;
                    
                /* Fallback driver */
                case 'K':
                case 'k':
                    FallbackDriver = 1;
                    continue;
                    
                /* Sanity Check */
                case 'W':
                case 'w':
                    SanityCheck = 1;
                    continue;
                    
                /* Itanium */
                case 'I':
                case 'i':
                    BuildType = 1;
                    continue;
                    
                /* X86 */
                case 'X':
                case 'x':
                    BuildType = 0;
                    continue;
                    
                /* AMD64 */
                case 'M':
                case 'm':
                    BuildType = 2;
                    continue;
                    
                /* WOW64 */
                case 'O':
                case 'o':
                    BuildType = 3;
                    continue;
                 
                /* Source only */
                case 'S':
                case 's':
                    SourceOnly = 1;
                    continue;
                default:
                    break;
            }

            /* If you got here, the options are invalid or missing */
            usage();
        }
        break;
    }
    
    /* Do we have no options? */
    if (optind == argc) usage();

    /* Should we announce ourselves? */
    if (!NoLogo)
    {
        /* This is who we are */
        printf("\nKbdTool v%d.%02d - convert keyboard text file to C file or a keyboard layout DLL\n\n",
               gVersion, gSubVersion);
    }
     
    /* Otherwise... do something */
    printf("Zoom zoom...\n");
}

/* EOF */
