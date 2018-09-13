/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivestat.c

Abstract:

    Dumps various statistics on hv (low) level structures in a hive.  (See
    regstat for higher level stuff.)

    Statistics:

        Short:  # of bins
                average bin size
                max bin size
                # of cells
                # of free cells
                # of allocated cells
                average free cell size
                total free size
                max free cell size
                average allocated size
                total allocated size
                max allocated cell size
                overhead summary (header, bin headers, cell headers)

        Long: bin#, offset, size
              cell offset, size, allocated
              cell offset, size, free


    Usage: {[+|-][<option>]} <filename>
           (+ = on by default, - = off by default)
           +s = summary - all of the short statistics
           -t[bafc] = trace, line per entry, bin, allocated, free, all cells
                (+tbc == +tbaf)
           -c = cell type summary
           -a[kvs] = Access Export (key nodes, values, SDs)

Author:

    Bryan M. Willman (bryanwi) 2-Sep-1992

Revision History:

--*/

/*

    NOTE:   Unlike other hive/registry tools, this one will not read the
            entire hive into memory, but will instead read it in via
            file I/O.  This makes it faster/easier to apply to very large
            hives.

*/
#include "regutil.h"
#include "edithive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

UCHAR *helptext[] = {
 "hivestat:                                                               ",
 "Statistics:                                                             ",
 "    Short:  # of bins                                                   ",
 "            average bin size                                            ",
 "            max bin size                                                ",
 "            # of cells                                                  ",
 "            # of free cells                                             ",
 "            # of allocated cells                                        ",
 "            average free cell size                                      ",
 "            total free size                                             ",
 "            max free cell size                                          ",
 "            average allocated size                                      ",
 "            total allocated size                                        ",
 "            max allocated cell size                                     ",
 "            overhead summary (header, bin headers, cell headers)        ",
 "    Long: bin#, offset, size                                            ",
 "          cell offset, size, allocated                                  ",
 "          cell offset, size, free                                       ",
 "Usage: {[+|-][<option>]} <filename>                                     ",
 "       (+ = on by default, - = off by default)                          ",
 "       +s = summary - all of the short statistics                       ",
 "       -t[bafc] = trace, line per entry, bin, allocated, free, all cells",
 "            (+tbc == +tbaf)                                             ",
 "       -c = cell type summary                                           ",
 "       -a[kvs] = Access Export (key nodes, values, SDs)                 ",
 NULL
};


VOID
ParseArgs(
    int     argc,
    char    *argv[]
    );

VOID
ScanHive(
    VOID
    );

VOID
ScanCell(
    PHCELL Cell,
    ULONG CellSize
    );

VOID
ScanKeyNode(
    IN PCM_KEY_NODE Node,
    IN ULONG CellSize
    );

VOID
ScanKeyValue(
    IN PCM_KEY_VALUE Value,
    IN ULONG CellSize
    );

VOID
ScanKeySD(
    IN PCM_KEY_SECURITY Security,
    IN ULONG CellSize
    );

VOID
ScanKeyIndex(
    IN PCM_KEY_INDEX Index,
    IN ULONG CellSize
    );

VOID
ScanUnknown(
    IN PCELL_DATA Data,
    IN ULONG CellSize
    );


//
//  CONTROL ARGUMENTS
//
BOOLEAN DoCellType = FALSE;
BOOLEAN DoSummary = TRUE;
BOOLEAN DoTraceBin = FALSE;
BOOLEAN DoTraceFree = FALSE;
BOOLEAN DoTraceAlloc = FALSE;

BOOLEAN AccessKeys = FALSE;
BOOLEAN AccessValues = FALSE;
BOOLEAN AccessSD = FALSE;
LPCTSTR FileName = NULL;

ULONG HiveVersion;

//
//  SUMMARY TOTALS
//
ULONG SizeKeyData=0;
ULONG SizeValueData=0;
ULONG SizeSDData=0;
ULONG SizeIndexData=0;
ULONG SizeUnknownData=0;

ULONG NumKeyData=0;
ULONG NumValueData=0;
ULONG NumSDData=0;
ULONG NumIndexData=0;
ULONG NumUnknownData=0;

void
main(
    int argc,
    char *argv[]
    )
{
    ParseArgs(argc, argv);
    ScanHive();
    exit(0);
}

VOID
ParseArgs(
    int     argc,
    char    *argv[]
    )
/*++

Routine Description:

    Read arguments and set control arguments and file name from them.

Arguments:

    argc, argv, standard meaning

Return Value:

    None.

--*/
{
    char *p;
    int i;
    BOOLEAN command;

    if (argc == 1) {
        for (i = 0; helptext[i] != NULL; i++) {
            fprintf(stderr, "%s\n", helptext[i]);
        }
        exit(1);
    }

    for (i = 1; i < argc; i++) {
        p = argv[i];

        if (*p == '+') {
            // switch something on
            command = TRUE;

        } else if (*p == '-') {
            // switch something off
            command = FALSE;

        } else {
            FileName = p;
            continue;
        }

        p++;
        if (*p == '\0')
            continue;

        switch (*p) {
        case 's':
        case 'S':
            DoSummary = command;
            break;

        case 'c':
        case 'C':
            DoCellType = command;
            break;

        case 'a':
        case 'A':
            p++;
            while (*p != '\0') {
                switch (*p) {
                    case 'k':
                    case 'K':
                        AccessKeys = command;
                        break;

                    case 's':
                    case 'S':
                        AccessSD = command;
                        break;

                    case 'v':
                    case 'V':
                        AccessValues = command;
                        break;

                    default:
                        break;
                }
                p++;
            }
            break;

        case 't':
        case 'T':
            p++;
            while (*p != '\0') {

                switch (*p) {
                case 'b':
                case 'B':
                    DoTraceBin = command;
                    break;

                case 'a':
                case 'A':
                    DoTraceAlloc = command;
                    break;

                case 'f':
                case 'F':
                    DoTraceFree = command;
                    break;

                case 'c':
                case 'C':
                    DoTraceAlloc = command;
                    DoTraceFree = command;
                    break;

                default:
                    break;
                }

                p++;
            }
            break;

        default:
            break;
        }
    }
    return;
}

VOID
ScanHive(
    )
/*++

Routine Description:

    Scan the hive, report what we see, based on control arguments.

--*/
{
    static char buffer[HBLOCK_SIZE];
    PHBASE_BLOCK bbp;
    HANDLE filehandle;
    BOOL rf;
    ULONG readcount;
    ULONG hivelength;
    ULONG hiveposition;
    PHCELL cp;
    PHCELL guard;
    PHBIN hbp;
    ULONG hoff;
    ULONG StatBinCount = 0;
    ULONG StatBinTotal = 0;
    ULONG StatBinMax = 0;
    ULONG StatFreeCount = 0;
    ULONG StatFreeTotal = 0;
    ULONG StatFreeMax = 0;
    ULONG StatAllocCount = 0;
    ULONG StatAllocTotal = 0;
    ULONG StatAllocMax = 0;
    ULONG binread;
    ULONG binsize;
    ULONG cellsize;
    ULONG boff;
    ULONG lboff;
    ULONG SizeTotal;

    //
    // open the file
    //
    filehandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (filehandle == INVALID_HANDLE_VALUE) {
        fprintf(stderr,
                "hivestat: Could not open file '%s' error = %08lx\n",
                FileName, GetLastError()
                );
        exit(1);
    }


    //
    // read the header
    //
    rf = ReadFile(filehandle, buffer, HBLOCK_SIZE, &readcount, NULL);
    if ( ( ! rf ) || (readcount != HBLOCK_SIZE) ) {
        fprintf(stderr, "hivestat: '%s' - cannot read base block!\n", FileName);
        exit(1);
    }

    bbp = (PHBASE_BLOCK)(&(buffer[0]));

    if ((bbp->Major != HSYS_MAJOR) ||
        (bbp->Minor > HSYS_MINOR))
    {
        fprintf(stderr,
                "hivestat: major/minor != %d/%d get newer hivestat\n",
                HSYS_MAJOR, HSYS_MINOR
                );
        exit(1);
    }

    HiveVersion = bbp->Minor;

    hivelength = bbp->Length + HBLOCK_SIZE;
    hiveposition = HBLOCK_SIZE;
    hoff = 0;

    printf("hivestat: '%s'\n", FileName);
    if (DoTraceBin || DoTraceFree || DoTraceAlloc) {
        printf("\nTrace\n");
        printf("bi=bin, fr=free, al=allocated\n");
        printf("offset is file offset, sub HBLOCK to get HCELL\n");
        printf("type,offset,size\n");
        printf("\n");
    }

    //
    // scan the hive
    //
    guard = (PHCELL)(&(buffer[0]) + HBLOCK_SIZE);

    //
    // hiveposition is file relative offset of next block we will read
    //
    // hoff is the file relative offset of the last block we read
    //
    // hivelength is actual length of file (header's recorded length plus
    // the size of the header.
    //
    // cp is pointer into memory, within range of buffer, it's a cell pointer
    //
    while (hiveposition < hivelength) {

        //
        // read in first block of bin, check signature, get bin stats
        //
        rf = ReadFile(filehandle, buffer, HBLOCK_SIZE, &readcount, NULL);
        if ( ( ! rf ) || (readcount != HBLOCK_SIZE) ) {
            fprintf(stderr, "hivestat: '%s' read error @%08lx\n", FileName, hiveposition);
            exit(1);
        }
        hbp = (PHBIN)(&(buffer[0]));

        if (hbp->Signature != HBIN_SIGNATURE) {
            fprintf(stderr,
                    "hivestat: '%s' bad bin sign. @%08lx\n", FileName, hiveposition);
            exit(1);
        }
        hiveposition += HBLOCK_SIZE;
        hoff += HBLOCK_SIZE;
        ASSERT(hoff+HBLOCK_SIZE == hiveposition);

        StatBinCount++;
        binsize = hbp->Size;
        StatBinTotal += binsize;
        if (binsize > StatBinMax) {
            StatBinMax = binsize;
        }

        if (DoTraceBin) {
            printf("bi,x%08lx,%ld\n", hoff, binsize);
        }

        //
        // scan the bin
        //
        // cp = pointer to cell we are looking at
        // boff = offset within bin
        // lboff = last offset within bin, used only for consistency checks
        // binread = number of bytes of bin we've read so far
        //
        cp = (PHCELL)((PUCHAR)hbp + sizeof(HBIN));
        boff = sizeof(HBIN);
        lboff = -1;
        binread = HBLOCK_SIZE;

        while (binread <= binsize) {

            //
            // if free, do free stuff
            // else do alloc stuff
            // do full stuff
            //
            if (cp->Size > 0) {
                //
                // free
                //
                cellsize = cp->Size;
                StatFreeCount++;
                StatFreeTotal += cellsize;
                if (cellsize > StatFreeMax) {
                    StatFreeMax = cellsize;
                }

                if (DoTraceFree) {
                    printf("fr,x%08lx,%ld\n",
                           hoff+((PUCHAR)cp - &(buffer[0])), cellsize);
                }


            } else {
                //
                // alloc
                //
                cellsize = -1 * cp->Size;
                StatAllocCount++;
                StatAllocTotal += cellsize;
                if (cellsize > StatAllocMax) {
                    StatAllocMax = cellsize;
                }

                if (DoTraceAlloc) {
                    printf("al,x%08lx,%ld\n",
                           hoff+((PUCHAR)cp - &(buffer[0])), cellsize);
                }

                ScanCell(cp,cellsize);

            }

            //
            // do basic consistency check
            //
#if 0
            if (cp->Last != lboff) {
                printf("e!,x%08lx  bad LAST pointer %08lx\n",
                        hoff+((PUCHAR)cp - &(buffer[0])), cp->Last);
            }
#endif

            //
            // advance to next cell
            //
            lboff = boff;
            cp = (PHCELL)((PUCHAR)cp + cellsize);
            boff += cellsize;

            //
            // scan ahead in bin, if cp has reached off end of block,
            // AND there's bin left to read.
            // do this BEFORE breaking out for boff at end.
            //
            while ( (cp >= guard) && (binread < binsize) ) {

                rf = ReadFile(filehandle, buffer, HBLOCK_SIZE, &readcount, NULL);
                if ( ( ! rf ) || (readcount != HBLOCK_SIZE) ) {
                    fprintf(stderr, "hivestat: '%s' read error @%08lx\n", FileName, hiveposition);
                    exit(1);
                }
                cp = (PHCELL)((PUCHAR)cp - HBLOCK_SIZE);
                hiveposition += HBLOCK_SIZE;
                hoff += HBLOCK_SIZE;
                binread += HBLOCK_SIZE;
                ASSERT(hoff+HBLOCK_SIZE == hiveposition);
            }

            if (boff >= binsize) {
                break;              // we are done with this bin
            }
        }
    }

    //
    // Traces are done, stats gathered, print summary
    //
    if (DoSummary) {

        printf("\nSummary:\n");
        printf("type\tcount/max single/total space\n");
        printf("%s\t%7ld\t%7ld\t%7ld\n",
                "bin", StatBinCount, StatBinMax, StatBinTotal);
        printf("%s\t%7ld\t%7ld\t%7ld\n",
                "free", StatFreeCount, StatFreeMax, StatFreeTotal);
        printf("%s\t%7ld\t%7ld\t%7ld\n",
                "alloc", StatAllocCount, StatAllocMax, StatAllocTotal);

    }

    if (DoSummary && DoCellType) {

        printf("\n");

        SizeTotal = SizeKeyData +
                    SizeValueData +
                    SizeSDData +
                    SizeIndexData +
                    SizeUnknownData;

        printf("Total Key Data     %7d (%5.2f %%)\n", SizeKeyData,
            (float)SizeKeyData*100/SizeTotal);
        printf("Total Value Data   %7d (%5.2f %%)\n", SizeValueData,
            (float)SizeValueData*100/SizeTotal);
        printf("Total SD Data      %7d (%5.2f %%)\n", SizeSDData,
            (float)SizeSDData*100/SizeTotal);
        printf("Total Index Data   %7d (%5.2f %%)\n", SizeIndexData,
            (float)SizeIndexData*100/SizeTotal);
        printf("Total Unknown Data %7d (%5.2f %%)\n", SizeUnknownData,
            (float)SizeUnknownData*100/SizeTotal);

        printf("\n");
        printf("Average Key Data     %8.2f (%d cells)\n",
            (float)SizeKeyData/NumKeyData,
            NumKeyData);
        printf("Average Value Data   %8.2f (%d cells)\n",
            (float)SizeValueData/NumValueData,
            NumValueData);
        printf("Average SD Data      %8.2f (%d cells)\n",
            (float)SizeSDData/NumSDData,
            NumSDData);
        printf("Average Index Data   %8.2f (%d cells)\n",
            (float)SizeIndexData/NumIndexData,
            NumIndexData);
        printf("Average Unknown Data %8.2f (%d cells)\n",
            (float)SizeUnknownData/NumUnknownData,
            NumUnknownData);
    }
    return;
}

VOID
ScanCell(
    IN PHCELL Cell,
    IN ULONG CellSize
    )

/*++

Routine Description:

    Given a pointer to an HCELL, this tries to figure out what type
    of data is in it (key, value, SD, etc.) and gather interesting
    statistics about it.

Arguments:

    Cell - Supplies a pointer to the HCELL

    CellSize - Supplies the size of the HCELL

Return Value:

    None, sets some global statistics depending on content of the cell.

--*/

{
    PCELL_DATA Data;

    if (!DoCellType) {
        return;
    }

    if (HiveVersion==1) {
        Data = (PCELL_DATA)&Cell->u.OldCell.u.UserData;
    } else {
        Data = (PCELL_DATA)&Cell->u.NewCell.u.UserData;
    }

    //
    // grovel through the data, see if we can figure out what it looks like
    //
    if ((Data->u.KeyNode.Signature == CM_KEY_NODE_SIGNATURE) &&
        (CellSize > sizeof(CM_KEY_NODE))) {

        //
        // probably a key node
        //
        ScanKeyNode(&Data->u.KeyNode, CellSize);

    } else if ((Data->u.KeyValue.Signature == CM_KEY_VALUE_SIGNATURE) &&
               (CellSize > sizeof(CM_KEY_VALUE))) {

        //
        // probably a key value
        //
        ScanKeyValue(&Data->u.KeyValue, CellSize);

    } else if ((Data->u.KeySecurity.Signature == CM_KEY_SECURITY_SIGNATURE) &&
               (CellSize > sizeof(CM_KEY_SECURITY))) {

        //
        // probably a security descriptor
        //
        ScanKeySD(&Data->u.KeySecurity, CellSize);

    } else if ((Data->u.KeyIndex.Signature == CM_KEY_INDEX_ROOT) ||
               (Data->u.KeyIndex.Signature == CM_KEY_INDEX_LEAF)) {
        //
        // probably a key index
        //
        ScanKeyIndex(&Data->u.KeyIndex, CellSize);

    } else {
        //
        // Nothing with a signature, could be either
        //  name
        //  key list
        //  value data
        //
        ScanUnknown(Data, CellSize);

    }
}

VOID
ScanKeyNode(
    IN PCM_KEY_NODE Node,
    IN ULONG CellSize
    )
{
    int i;

    SizeKeyData += CellSize;
    NumKeyData++;

    if (AccessKeys) {
        printf("%d, %d, %d, %d, \"",
               Node->SubKeyCounts[Stable],
               Node->ValueList.Count,
               Node->NameLength,
               Node->ClassLength);

        for (i=0; i < Node->NameLength/sizeof(WCHAR); i++) {
            printf("%c",(CHAR)Node->Name[i]);
        }
        printf("\"\n");
    }

}
VOID
ScanKeyValue(
    IN PCM_KEY_VALUE Value,
    IN ULONG CellSize
    )
{
    int i;
    int DataLength;

    SizeValueData += CellSize;
    NumValueData++;
    if (AccessValues) {
        DataLength = Value->DataLength;
        if (DataLength >= CM_KEY_VALUE_SPECIAL_SIZE) {
            DataLength -= CM_KEY_VALUE_SPECIAL_SIZE;
        }
        printf("%d, %d, \"",
               DataLength,
               Value->NameLength);

        for (i=0; i < Value->NameLength/sizeof(WCHAR); i++) {
            printf("%c",(CHAR)Value->Name[i]);
        }
        printf("\"\n");
    }

}
VOID
ScanKeySD(
    IN PCM_KEY_SECURITY Security,
    IN ULONG CellSize
    )
{
    SizeSDData += CellSize;
    NumSDData++;

    if (AccessSD) {
        printf("%d,%d\n",
               Security->ReferenceCount,
               Security->DescriptorLength);
    }

}
VOID
ScanKeyIndex(
    IN PCM_KEY_INDEX Index,
    IN ULONG CellSize
    )
{
    SizeIndexData += CellSize;
    NumIndexData++;

}
VOID
ScanUnknown(
    IN PCELL_DATA Data,
    IN ULONG CellSize
    )
{
    SizeUnknownData += CellSize;
    NumUnknownData++;

}
