/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivehdr.c

Abstract:

    Dump the header of a hive primary, alternate, or log file.

    hivehdr filename filename filename ...

Author:

    Bryan Willman (bryanwi)  6-april-92

Revision History:

--*/


#define _ARCCODES_

#include "regutil.h"
#include "edithive.h"

void
DoDump(
    PUCHAR  Filename
    );

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    int i;

    if (argc == 1) {
        fprintf(stderr, "Usage: hivehdr filename filename...\n", argv[0]);
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        DoDump(argv[i]);
    }

    exit(0);
}

void
DoDump(
    PUCHAR  Filename
    )
{
    HANDLE  infile;
    static char buffer[HSECTOR_SIZE];
    PHBASE_BLOCK bbp;
    char   *validstring[] = { "BAD", "OK" };
    int     valid;
    char   *typename[] = { "primary", "alternate", "log", "external", "unknown" };
    int     typeselect;
    int     readcount;
    unsigned long checksum;
    unsigned long i;

    infile = (HANDLE)CreateFile(
            Filename,                           // file name
            GENERIC_READ,                       // desired access
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
            NULL,                               // security attributes
            OPEN_EXISTING,                      // creation disposition
            FILE_FLAG_SEQUENTIAL_SCAN,          // flags and attributes
            NULL                                // template file
    );
    if (infile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "hivehdr: Could not open '%s'\n", Filename);
        return;
    }

    if (!ReadFile(infile, buffer, HSECTOR_SIZE, &readcount, NULL)) {
        fprintf(
            stderr, "hivehdr: '%s' - cannot read full base block\n", Filename);
        return;
    }
    if (readcount != HSECTOR_SIZE) {
        fprintf(
            stderr, "hivehdr: '%s' - cannot read full base block\n", Filename);
        return;
    }

    bbp = (PHBASE_BLOCK)&(buffer[0]);

    if ((bbp->Major != 1) || (bbp->Minor != 1)) {
        printf("WARNING: Hive file is newer than hivehdr, or is invalid\n");
    }

    printf("         File: '%s'\n", Filename);
    printf("    BaseBlock:\n");

    valid = (bbp->Signature == HBASE_BLOCK_SIGNATURE);
    printf("    Signature: %08lx  '%4.4s'\t\t%s\n",
            bbp->Signature, (PUCHAR)&(bbp->Signature), validstring[valid]);

    valid = (bbp->Sequence1 == bbp->Sequence2);
    printf(" Sequence1//2: %08lx//%08lx\t%s\n",
            bbp->Sequence1, bbp->Sequence2, validstring[valid]);

    printf("    TimeStamp: %08lx:%08lx\n",
            bbp->TimeStamp.HighPart, bbp->TimeStamp.LowPart,
            (PUCHAR)&(bbp->Signature), validstring[valid]);

    valid = (bbp->Major == HSYS_MAJOR);
    printf("Major Version: %08lx\t\t\t%s\n",
            bbp->Major, validstring[valid]);

    valid = (bbp->Minor == HSYS_MINOR);
    printf("Minor Version: %08lx\t\t\t%s\n",
            bbp->Minor, validstring[valid]);

    valid = ( (bbp->Type == HFILE_TYPE_PRIMARY) ||
              (bbp->Type == HFILE_TYPE_ALTERNATE) ||
              (bbp->Type == HFILE_TYPE_LOG) );
    if (valid) {
        typeselect = bbp->Type;
    } else {
        typeselect = HFILE_TYPE_MAX;
    }

    printf("         Type: %08lx %s\t\t%s\n",
            bbp->Type, typename[typeselect], validstring[valid]);

    valid = (bbp->Format == HBASE_FORMAT_MEMORY);
    printf("       Format: %08lx\t\t\t%s\n",
            bbp->Format, validstring[valid]);

    printf("     RootCell: %08lx\n", bbp->RootCell);

    printf("       Length: %08lx\n", bbp->Length);

    printf("      Cluster: %08lx\n", bbp->Cluster);

    checksum = HvpHeaderCheckSum(bbp);
    valid = (checksum == bbp->CheckSum);
    if (checksum == bbp->CheckSum) {
        printf("     CheckSum: %08lx\t\t\t%s\n",
                bbp->CheckSum, validstring[TRUE]);
    } else {
        printf("     CheckSum: %08lx\t\t\t%s\tCorrect: %08lx\n",
                bbp->CheckSum, validstring[FALSE], checksum);
    }

    //
    // print last part of file name, aid to identification
    //
    printf("Hive/FileName: ");

    for (i = 0; i < HBASE_NAME_ALLOC;i+=sizeof(WCHAR)) {
        printf("%wc", bbp->FileName[i]);
    }


    return;
}
