//
// gentable.c
//
// Generates static Huffman tables to be included in the DLL
//
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


//#define GENERATE_C_CODE_TABLES

#ifdef GENERATE_C_CODE_TABLES
//
// Generates outputting tables for the fast encoder.
// 
// The other encoders do things differently; they have separate arrays for
// code[], len[], and they then have to check extra_bits[] afterwards to see
// how many (if any) low order bits to output.
//
// The fast encoder, on the other hand, is lean and mean.
//
// For a set of literal codes and lengths, generate a set of DWORDs with 
// these properties:
//
// [ code ] [ code_length ] 
//  27 bits     5 bits       
//
// Where "len" is the # bits in the code, and "code" is the FULL code to output,
// including ALL necessary g_LengthExtraBits[].
//
// The bitwise outputter cannot handle codes more than 16 bits in length, so
// if this happens (quite rare) whoever is using this table must output
// the code in two instalments.  
//
void MakeFastEncoderLiteralTable(BYTE *len, USHORT *code)
{
    ULONG outcode[(NUM_CHARS+1+(MAX_MATCH-MIN_MATCH+1))];
    int elements_to_output;
    int i;
    int match_length;

    elements_to_output = (NUM_CHARS+1+(MAX_MATCH-MIN_MATCH+1));

    // literals and end of block code are output without much fanfare
    for (i = 0; i <= NUM_CHARS; i++)
    {
        outcode[i] = len[i] | (code[i] << 5);
    }

    // match lengths are more interesting
    for (match_length = 0; match_length <= (MAX_MATCH-MIN_MATCH); match_length++)
    {
        int length_slot = g_LengthLookup[match_length];
        int extra_bits = g_ExtraLengthBits[length_slot];
        ULONG orig_code;
        int orig_len;
        ULONG tbl_code;
        int tbl_len;

        orig_code = (ULONG) code[(NUM_CHARS+1)+length_slot];
        orig_len = len[(NUM_CHARS+1)+length_slot];

        if (extra_bits == 0)
        {
            // if no extra bits, it's quite simple
            tbl_code = orig_code;
            tbl_len = orig_len;
        }
        else
        {
            // get extra bits data
            int extra_bits_data = match_length & g_BitMask[extra_bits];

            // stick it in the code and increase the code length appropriately
            tbl_code = orig_code | (extra_bits_data << orig_len);
            tbl_len = orig_len + extra_bits;
        }

        _ASSERT(tbl_len <= 27);
        outcode[(NUM_CHARS+1)+match_length] = tbl_len | (tbl_code << 5);
    }

    printf("#ifdef DECLARE_DATA\n");

    printf("const ULONG g_FastEncoderLiteralCodeInfo[] = {\n");

    for (i = 0; i < elements_to_output; i++)
    {
        if ((i % 7) == 0)
            printf("\n");

        printf("0x%08x,", outcode[i]);
    }

    printf("\n};\n");

    printf("#else /* !DECLARE_DATA */\n");
    printf("extern const ULONG g_FastEncoderLiteralCodeInfo[];\n");
    printf("#endif /* DECLARE_DATA */\n");

}


//
// The distance table is slightly different; obviously we cannot have an element
// for all 8192 possible distances.  Instead we merge the code[] and len[] arrays,
// and store extra_bits[] in there.
//
// [ code ]  [ # extra_bits ] [ len ]
//  24 bits      4 bits       4 bits
//
// The code part is always < 16 bits, since we aren't merging the actual extra 
// bits with it, unlike for the literals.
//
void MakeFastEncoderDistanceTable(BYTE *len, USHORT *code)
{
    ULONG outcode[MAX_DIST_TREE_ELEMENTS];
    int i;
    int pos_slot;

    for (pos_slot = 0; pos_slot < MAX_DIST_TREE_ELEMENTS; pos_slot++)
    {
        int extra_bits = g_ExtraDistanceBits[pos_slot];
        ULONG orig_code;
        int orig_len;

        orig_code = (ULONG) code[pos_slot];
        orig_len = len[pos_slot];

        outcode[pos_slot] = orig_len | (extra_bits << 4) | (orig_code << 8);
    }

    printf("#ifdef DECLARE_DATA\n");

    printf("const ULONG g_FastEncoderDistanceCodeInfo[] = {\n");

    for (i = 0; i < MAX_DIST_TREE_ELEMENTS; i++)
    {
        if ((i % 7) == 0)
            printf("\n");

        printf("0x%08x,", outcode[i]);
    }

    printf("\n};\n");

    printf("#else /* !DECLARE_DATA */\n");
    printf("extern const ULONG g_FastEncoderDistanceCodeInfo[];\n");
    printf("#endif /* DECLARE_DATA */\n");

}


void GenerateTable(char *table_name, int elements, BYTE *len, USHORT *code)
{
    int i;

    printf("#ifdef DECLARE_DATA\n");
    printf("const BYTE %sLength[] = {", table_name);

    for (i = 0; i < elements; i++)
    {
        if ((i % 16) == 0)
            printf("\n");

        printf("0x%02x,", len[i]);
    }

    printf("\n};\n");

    printf("const USHORT %sCode[] = {", table_name);

    for (i = 0; i < elements; i++)
    {
        if ((i % 8) == 0)
            printf("\n");

        printf("0x%04x,", code[i]);
    }

    printf("\n};\n");
    printf("#else /* !DECLARE_DATA */\n");
    printf("extern const BYTE %sLength[];\n", table_name);
    printf("extern const USHORT %sCode[];\n", table_name);
    printf("#endif /* DECLARE_DATA */\n");

}
#endif
