/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    uuidgen.c {v1.00}

Abstract:

usage: uuidgen [-isonvh?]
    i - Output UUID in an IDL interface template
    s - Output UUID as an initialized C struct
    o<filename> - redirect output to a file, specified immediately after o
    n<number> - Number of UUIDs to generate, specified immediately after n
    v - display version information about uuidgen
    h,? - Display command option summary

This command-line program simply uses the procedure I_UuidStringGenerate
to output one or more UUIDs in one of the formats normal, IDL, or C struct.
This code should build under nt to nt and nmake under os2 to dos.
Requires uuidfmt.c.

Author:

    Joev Dubach (t-joevd) 6/11/92

Revision History:

--*/

//
// Inclusions
//
// The order in which these includes are performed seems, after
// extensive testing and analysis, to be highly crucial to an
// NT build.

#include <sysinc.h>
#include <rpc.h>
#include <uuidfmt.h>
#include <stdio.h>
#include <stdlib.h>

#include <common.ver>

//
// Defines
//

#define IDL_BIT_FLAG 0x01
#define CSTRUCT_BIT_FLAG 0x02
#define BAD_SWITCH_STR "Invalid Switch Usage: %s\n\n"
#define CANNOT_OPEN_STR "Cannot open output file: %s\n\n"
#define TOO_SMALL_STR "Argument to %s must be an integer greater than 0.\n\n"
#define VERSION_INFO_STR \
 "Microsoft UUID Generator v1.00 " \
 VER_LEGALCOPYRIGHT_STR \
 "\n\n"
#define BAD_ARGS_STR "The arguments i and s are mutually exclusive.\n\n"
#define NO_ADDRESS_STR \
 "Cannot find the RPC End Point Mapper (RPCSS.EXE); Unable to allocate UUIDs.\n\n"
#define OUT_OF_MEMORY_STR \
 "Unable to allocate enough memory to create string.\n\n"
#define LOCAL_ONLY_STR \
 "Warning: Unable to determine your network address.  The UUID generated is\n" \
 "unique on this computer only.  It should not be used on another computer.\n"

//
// Function prototypes.
//

#ifdef NTENV
int __cdecl
#else // NTENV
int
#endif // NTENV
main(
    int argc,
    char **argv
    );

void Usage(void);

void ErrorUsageAndExit(void);

void NoErrorUsageAndExit(void);

//
// Global variables
//

FILE * OutputFile = stdout;

#ifdef NTENV
int __cdecl
#else // NTENV
int
#endif // NTENV
main(
    int argc,
    char **argv
    )
{
    int  ConditionFlags = 0;       // Holds conditions IDL_BIT_FLAG and
                                       // CSTRUCT_BIT_FLAG.
    char MyUuidString[255];        // The UUID string returned by
                                       // I_UuidStringGenerate.
    int  NumberOfUuids = 1;        // How many to make.
    int  i;                        // Current arg#.
    int Flag;                      // UUID Format requested.
    RPC_STATUS Result;
    int FirstTime = 1;

    //
    // Parse the command line.
    //

    for (i=1;argc-i;i++)
        {

        //
        // Make sure arg is in proper format.
        //

        if ( (argv[i][0] != '/') && (argv[i][0] != '-') )
            {
            fprintf(stderr, BAD_SWITCH_STR, argv[i]);
            ErrorUsageAndExit();
            }

        //
        // Which arg is it?
        //

        switch (argv[i][1])
            {
            case 'I':
            case 'i':
                ConditionFlags |= IDL_BIT_FLAG;
                break;

            case 'S':
            case 's':
                ConditionFlags |= CSTRUCT_BIT_FLAG;
                break;

            case 'O':
            case 'o':
                OutputFile = fopen(argv[i]+2, "wt");
                if (OutputFile == NULL)
                    {
                    fprintf(stderr,CANNOT_OPEN_STR, argv[i]+2);
                    exit(1);
                    }
                break;

            case 'N':
            case 'n':
                NumberOfUuids = atoi(argv[i]+2);
                if (NumberOfUuids <= 0)
                    {
                    fprintf(stderr, TOO_SMALL_STR, argv[i-1]);
                    ErrorUsageAndExit();
                    }
                break;

            case 'V':
            case 'v':
                fprintf(OutputFile, VERSION_INFO_STR);
                exit(0);

            case 'H':
            case 'h':
            case '?':
                NoErrorUsageAndExit();

            default:
                fprintf(stderr, BAD_SWITCH_STR, argv[i]);
                ErrorUsageAndExit();

            } // switch
        } // for

    //
    // Fulfill the user's request
    //

    if ((ConditionFlags & IDL_BIT_FLAG) && (ConditionFlags & CSTRUCT_BIT_FLAG))
        {
        fprintf(stderr,BAD_ARGS_STR);
        ErrorUsageAndExit();
        }

    if (ConditionFlags & IDL_BIT_FLAG)
        {
        Flag = UUIDGEN_FORMAT_IDL;
        }
    else if (ConditionFlags & CSTRUCT_BIT_FLAG)
        {
        Flag = UUIDGEN_FORMAT_CSTRUCT;
        }
    else
        {
        Flag = UUIDGEN_FORMAT_PLAIN;
        }

    for (;NumberOfUuids;NumberOfUuids--)
        {
        Result = I_UuidStringGenerate(
            Flag,
            MyUuidString,
            "INTERFACENAME"
            );
        ASSERT((Result == RPC_S_OK)
               || (Result == RPC_S_UUID_LOCAL_ONLY)
               || (Result == RPC_S_OUT_OF_MEMORY)
               || (Result == RPC_S_UUID_NO_ADDRESS));
        switch(Result)
            {
            case RPC_S_OUT_OF_MEMORY:
                fprintf(stderr,OUT_OF_MEMORY_STR);
                exit(1);

            case RPC_S_UUID_NO_ADDRESS:
                fprintf(stderr,NO_ADDRESS_STR);
                exit(1);

            case RPC_S_UUID_LOCAL_ONLY:
                if (FirstTime)
                    {
                    fprintf(stderr,LOCAL_ONLY_STR);
                    FirstTime = 0;
                    }

                // Fall through to the valid case.

            case RPC_S_OK:
                fprintf(
                    OutputFile,
                    MyUuidString
                    );
            } // end switch
        } // end for

    return(0);
}

void Usage(void)
{
    fprintf(OutputFile, VERSION_INFO_STR
                        "usage: uuidgen [-isonvh?]\n"
                        "\ti - Output UUID in an IDL interface template\n"
                        "\ts - Output UUID as an initialized C struct\n"

                        "\to<filename> - redirect output to a file, specified immediately after o\n"
                        "\tn<number> - Number of UUIDs to generate, specified immediately after n\n"
                        "\tv - display version information about uuidgen\n"
                        "\th,? - Display command option summary\n");
}

void ErrorUsageAndExit(void)
{
    OutputFile = stderr;
    Usage();
    exit(1);
}

void NoErrorUsageAndExit(void)
{
    Usage();
    exit(0);
}

