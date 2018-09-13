/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tprefix.c

Abstract:

    Test program for the Prefix table package

Author:

    Gary Kimura     [GaryKi]    03-Aug-1989

Revision History:

--*/

#include <stdio.h>
#include <string.h>

#include "nt.h"
#include "ntrtl.h"

//
//  Routines and types for generating random prefixes
//

ULONG RtlRandom ( IN OUT PULONG Seed );
ULONG Seed;

PSZ AnotherPrefix(IN ULONG MaxNameLength);
ULONG AlphabetLength;

//PSZ Alphabet = "AlphaBravoCharlieDeltaEchoFoxtrotGolfHotelIndiaJuliettKiloLimaMikeNovemberOscarPapaQuebecRomeoSierraTangoUniformVictorWhiskeyXrayYankeeZulu";

PSZ Alphabet = "\
Aa\
BBbb\
CCCccc\
DDDDdddd\
EEEEEeeeee\
FFFFFFffffff\
GGGGGGGggggggg\
HHHHHHHHhhhhhhhh\
IIIIIIIIIiiiiiiiii\
JJJJJJJJJJjjjjjjjjjj\
KKKKKKKKKKKkkkkkkkkkkk\
LLLLLLLLLLLLllllllllllll\
MMMMMMMMMMMMMmmmmmmmmmmmmm\
NNNNNNNNNNNNNNnnnnnnnnnnnnnn\
OOOOOOOOOOOOOOOooooooooooooooo\
PPPPPPPPPPPPPPPPpppppppppppppppp\
QQQQQQQQQQQQQQQQQqqqqqqqqqqqqqqqqq\
RRRRRRRRRRRRRRRRRRrrrrrrrrrrrrrrrrrr\
SSSSSSSSSSSSSSSSSSSsssssssssssssssssss\
TTTTTTTTTTTTTTTTTTTTtttttttttttttttttttt\
UUUUUUUUUUUUUUUUUUUUUuuuuuuuuuuuuuuuuuuuuu\
VVVVVVVVVVVVVVVVVVVVVVvvvvvvvvvvvvvvvvvvvvvv\
WWWWWWWWWWWWWWWWWWWWWWWwwwwwwwwwwwwwwwwwwwwwww\
XXXXXXXXXXXXXXXXXXXXXXXXxxxxxxxxxxxxxxxxxxxxxxxx\
YYYYYYYYYYYYYYYYYYYYYYYYYyyyyyyyyyyyyyyyyyyyyyyyyy\
ZZZZZZZZZZZZZZZZZZZZZZZZZZzzzzzzzzzzzzzzzzzzzzzzzzzz";

#define BUFFER_LENGTH 8192

CHAR Buffer[BUFFER_LENGTH];
ULONG NextBufferChar = 0;

//
//  record structure and variables for the prefix table and it
//  elements
//

typedef struct _PREFIX_NODE {
    PREFIX_TABLE_ENTRY PfxEntry;
    STRING String;
} PREFIX_NODE;
typedef PREFIX_NODE *PPREFIX_NODE;

#define PREFIXES 512

PREFIX_NODE Prefixes[PREFIXES];

PREFIX_TABLE PrefixTable;

int
main(
    int argc,
    char *argv[]
    )
{
    ULONG i;
    PSZ Psz;

    PPREFIX_TABLE_ENTRY PfxEntry;
    PPREFIX_NODE PfxNode;

    STRING String;

    //
    //  We're starting the test
    //

    DbgPrint("Start Prefix Test\n");

    //
    //  Calculate the alphabet size for use by AnotherPrefix
    //

    AlphabetLength = strlen(Alphabet);

    //
    //  Initialize the prefix table
    //

    PfxInitialize(&PrefixTable);

    //
    //  Insert the root prefix
    //

    RtlInitString( &Prefixes[i].String, "\\" );
    if (PfxInsertPrefix( &PrefixTable,
                         &Prefixes[0].String,
                         &Prefixes[0].PfxEntry )) {
        DbgPrint("Insert root prefix\n");
    } else {
        DbgPrint("error inserting root prefix\n");
    }

    //
    //  Insert prefixes
    //

    Seed = 0;

    for (i = 1, Psz = AnotherPrefix(3);
         (i < PREFIXES) && (Psz != NULL);
         i += 1, Psz = AnotherPrefix(3)) {

        DbgPrint("[0x%x] = ", i);
        DbgPrint("\"%s\"", Psz);

        RtlInitString(&Prefixes[i].String, Psz);

        if (PfxInsertPrefix( &PrefixTable,
                             &Prefixes[i].String,
                             &Prefixes[i].PfxEntry )) {

            DbgPrint(" inserted in table\n");

        } else {

            DbgPrint(" already in table\n");

        }

    }

    //
    //  Enumerate the prefix table
    //

    DbgPrint("Enumerate Prefix Table the first time\n");

    for (PfxEntry = PfxNextPrefix(&PrefixTable, TRUE);
         PfxEntry != NULL;
         PfxEntry = PfxNextPrefix(&PrefixTable, FALSE)) {

        PfxNode = CONTAINING_RECORD(PfxEntry, PREFIX_NODE, PfxEntry);

        DbgPrint("%s\n", PfxNode->String.Buffer);

    }

    DbgPrint("Start Prefix search 0x%x\n", NextBufferChar);

    //
    //  Search for prefixes
    //

    for (Psz = AnotherPrefix(4); Psz != NULL; Psz = AnotherPrefix(4)) {

        DbgPrint("0x%x ", NextBufferChar);

        RtlInitString(&String, Psz);

        PfxEntry = PfxFindPrefix( &PrefixTable, &String, FALSE );

        if (PfxEntry == NULL) {

            PfxEntry = PfxFindPrefix( &PrefixTable, &String, TRUE );

            if (PfxEntry == NULL) {

                DbgPrint("Not found      \"%s\"\n", Psz);

                NOTHING;

            } else {

                PfxNode = CONTAINING_RECORD(PfxEntry, PREFIX_NODE, PfxEntry);

                DbgPrint("Case blind     \"%s\" is \"%s\"\n", Psz, PfxNode->String.Buffer);

                PfxRemovePrefix( &PrefixTable, PfxEntry );

            }

        } else {

            PfxNode = CONTAINING_RECORD(PfxEntry, PREFIX_NODE, PfxEntry);

            DbgPrint(    "Case sensitive \"%s\" is \"%s\"\n", Psz, PfxNode->String.Buffer);

            if (PfxNode != &Prefixes[0]) {

                PfxRemovePrefix( &PrefixTable, PfxEntry );

            }

        }

    }

    //
    //  Enumerate the prefix table
    //

    DbgPrint("Enumerate Prefix Table a second time\n");

    for (PfxEntry = PfxNextPrefix(&PrefixTable, TRUE);
         PfxEntry != NULL;
         PfxEntry = PfxNextPrefix(&PrefixTable, FALSE)) {

        PfxNode = CONTAINING_RECORD(PfxEntry, PREFIX_NODE, PfxEntry);

        DbgPrint("%s\n", PfxNode->String.Buffer);

    }

    //
    //  Now enumerate and zero out the table
    //

    for (PfxEntry = PfxNextPrefix(&PrefixTable, TRUE);
         PfxEntry != NULL;
         PfxEntry = PfxNextPrefix(&PrefixTable, FALSE)) {

        PfxNode = CONTAINING_RECORD(PfxEntry, PREFIX_NODE, PfxEntry);

        DbgPrint("Delete %s\n", PfxNode->String.Buffer);

        PfxRemovePrefix( &PrefixTable, PfxEntry );

    }

    //
    //  Enumerate again but this time the table should be empty
    //

    for (PfxEntry = PfxNextPrefix(&PrefixTable, TRUE);
         PfxEntry != NULL;
         PfxEntry = PfxNextPrefix(&PrefixTable, FALSE)) {

        PfxNode = CONTAINING_RECORD(PfxEntry, PREFIX_NODE, PfxEntry);

        DbgPrint("This Node should be gone \"%s\"\n", PfxNode->String.Buffer);

    }

    DbgPrint("End PrefixTest()\n");

    return TRUE;
}


PSZ
AnotherPrefix(IN ULONG MaxNameLength)
{
    ULONG AlphabetPosition;

    ULONG NameLength;
    ULONG IndividualNameLength;

    ULONG StartBufferPosition;
    ULONG i;
    ULONG j;

    //
    //  Check if there is enough room for another name
    //

    if (NextBufferChar > (BUFFER_LENGTH - (MaxNameLength * 4))) {
        return NULL;
    }

    //
    //  Where in the alphabet soup we start
    //

    AlphabetPosition = RtlRandom(&Seed) % AlphabetLength;

    //
    //  How many names we want in our prefix
    //

    NameLength = (RtlRandom(&Seed) % MaxNameLength) + 1;

    //
    //  Compute each name
    //

    StartBufferPosition = NextBufferChar;

    for (i = 0; i < NameLength; i += 1) {

        Buffer[NextBufferChar++] = '\\';

        IndividualNameLength = (RtlRandom(&Seed) % 3) + 1;

        for (j = 0; j < IndividualNameLength; j += 1) {

            Buffer[NextBufferChar++] = Alphabet[AlphabetPosition];
            AlphabetPosition = (AlphabetPosition + 1) % AlphabetLength;

        }

    }

    Buffer[NextBufferChar++] = '\0';

    return &Buffer[StartBufferPosition];

}

