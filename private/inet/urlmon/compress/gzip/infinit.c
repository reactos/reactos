//
// infinit.c
//
// Inflate initialisation
//
#include <stdio.h>
#include <crtdbg.h>
#include "inflate.h"
#include "maketbl.h"


//
// Generate global tables for decoding static blocks
//
static VOID CreateStaticDecodingTables(VOID)
{
    SHORT StaticDistanceTreeLeft[MAX_DIST_TREE_ELEMENTS*2]; // temporary: not exported
    SHORT StaticDistanceTreeRight[MAX_DIST_TREE_ELEMENTS*2]; // temporary: not exported

    SHORT StaticLiteralTreeLeft[MAX_LITERAL_TREE_ELEMENTS*2]; // temporary: not exported
    SHORT StaticLiteralTreeRight[MAX_LITERAL_TREE_ELEMENTS*2]; // temporary: not exported
    
    SHORT TempStaticDistanceTreeTable[STATIC_BLOCK_DISTANCE_TABLE_SIZE];
    BYTE  TempStaticDistanceTreeLength[MAX_DIST_TREE_ELEMENTS];

    int i;

    _ASSERT(STATIC_BLOCK_LITERAL_TABLE_BITS == 9);
    _ASSERT(STATIC_BLOCK_DISTANCE_TABLE_BITS == 5);

    // The Table[] and Left/Right arrays are for the decoder only
    // We don't output Left/Right because they are not used; everything
    // fits in the lookup table, since max code length is 9, and tablebits
    // > 9.
    makeTable(
		MAX_LITERAL_TREE_ELEMENTS,
		STATIC_BLOCK_LITERAL_TABLE_BITS,
		g_StaticLiteralTreeLength,
		g_StaticLiteralTreeTable,
		StaticLiteralTreeLeft,
		StaticLiteralTreeRight);

    for (i = 0; i < MAX_DIST_TREE_ELEMENTS; i++)
        TempStaticDistanceTreeLength[i] = 5;

    makeTable(
		MAX_DIST_TREE_ELEMENTS,
		STATIC_BLOCK_DISTANCE_TABLE_BITS,
		TempStaticDistanceTreeLength,
		TempStaticDistanceTreeTable,
		StaticDistanceTreeLeft,
		StaticDistanceTreeRight);
    
    // Since all values are < 256, use a BYTE array
    for (i = 0; i < STATIC_BLOCK_DISTANCE_TABLE_SIZE; i++)
        g_StaticDistanceTreeTable[i] = TempStaticDistanceTreeTable[i];
}


VOID inflateInit(VOID)
{
    InitStaticBlock();
    CreateStaticDecodingTables();
}

