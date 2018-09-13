//
// definit.c
//
// Initialisation code for deflate (compression stage)
//
// Includes both some one-time init routines, as well as a per context/reset init routine
//
#include "types.h"
#include "deflate.h"
#include "inflate.h"
#include "defproto.h"
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>


//
// This function is called by the standard and optimal encoders, and creates the initial tree
// used to record literals for the first block.  After the first block we use the last block's
// trees to record data.
//
// This function does not change global data, and is called one a per context creation/reset.
//
VOID DeflateInitRecordingTables(
    BYTE *  recording_literal_len,
    USHORT *recording_literal_code,
    BYTE *  recording_dist_len,
    USHORT *recording_dist_code
)
{
    // BUGBUG These frequencies were taken from running on some text file, better stats could
    // be obtained from using an html page.  This barely affects compression though; bad estimates
    // will just make the recording buffer fill up a little bit sooner, making us output a block
    // a little sooner, which isn't always a bad thing anyway.
	USHORT	recording_dist_tree_freq[MAX_DIST_TREE_ELEMENTS*2] = 
	{
		2,2,3,4,3,7,16,22,42,60,100,80,149,158,223,200,380,324,537,
		477,831,752,1231,999,1369,1100,2034,1667,2599,2216,0,0
	};

	USHORT	recording_literal_tree_freq[MAX_LITERAL_TREE_ELEMENTS*2];

    int i;

	makeTree(
		MAX_DIST_TREE_ELEMENTS, 
		RECORDING_DIST_MAX_CODE_LEN, 
		recording_dist_tree_freq, 
		recording_dist_code, 
		recording_dist_len
	);

    // BUGBUG Put a better estimation in here!  This assumes all literals (chars and matches)
    // are equally likely, which they aren't (although all chars might be fairly equal for a
    // binary file).
	for (i = 0; i < MAX_LITERAL_TREE_ELEMENTS; i++)
		recording_literal_tree_freq[i] = 1;

	makeTree(
		MAX_LITERAL_TREE_ELEMENTS, 
		RECORDING_LIT_MAX_CODE_LEN, 
		recording_literal_tree_freq, 
		recording_literal_code, 
		recording_literal_len
	);
}


//
// One-time init
//
// Generate the global slot tables which allow us to convert a distance
// (0..32K) to a distance slot (0..29), and a length (3..258) to
// a length slot (0...28)
//
static void GenerateSlotTables(void)
{
	int code, length, dist, n;

        /* Initialize the mapping length (0..255) -> length code (0..28) */
	length = 0;
	
	for (code = 0; code < NUM_LENGTH_BASE_CODES-1; code++)
	{
		for (n = 0; n < (1 << g_ExtraLengthBits[code]); n++) 
			g_LengthLookup[length++] = (byte) code;
	}

	g_LengthLookup[length-1] = (byte) code;

        /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
	dist = 0;
    
	for (code = 0 ; code < 16; code++)
	{
		for (n = 0; n < (1 << g_ExtraDistanceBits[code]); n++)
			g_DistLookup[dist++] = (byte) code;
	}

	dist >>= 7; /* from now on, all distances are divided by 128 */
    
	for ( ; code < NUM_DIST_BASE_CODES; code++) 
	{
		for (n = 0; n < (1 << (g_ExtraDistanceBits[code]-7)); n++) 
			g_DistLookup[256 + dist++] = (byte) code;
	}

    // ensure we didn't overflow the array
    _ASSERT(256 + dist <= sizeof(g_DistLookup)/sizeof(g_DistLookup[0]));
}


//
// One-time init
//
// Generate tables for encoding static blocks
//
static void GenerateStaticEncodingTables(void)
{
    int     i;
    int     len_cnt[17];
    BYTE    StaticDistanceTreeLength[MAX_DIST_TREE_ELEMENTS];

    // ensure we have already created the StaticLiteralTreeLength array
    // if we haven't, then this value would be zero
    _ASSERT(g_StaticLiteralTreeLength[0] != 0);

    //
    // Make literal tree
    //
    for (i = 0; i < 17; i++)
        len_cnt[i] = 0;

    // length count (how many length 8's, 9's, etc. there are) - needed to call makeCode()
    len_cnt[8] = 144;
    len_cnt[9] = 255-144+1;
    len_cnt[7] = 279-256+1;
    len_cnt[8] += (287-280)+1;

    makeCode(
        MAX_LITERAL_TREE_ELEMENTS, 
        len_cnt, 
        g_StaticLiteralTreeLength,
        g_StaticLiteralTreeCode
    );

    //
    // Make distance tree; there are 32 5-bit codes
    //
    for (i = 0; i < 17; i++)
        len_cnt[i] = 0;

    len_cnt[5] = 32;

    // We don't store StaticDistanceTreeLength[] globally, since it's 5 for everything,
    // but we need it to call makeCode()
    for (i = 0; i < MAX_DIST_TREE_ELEMENTS; i++)
        StaticDistanceTreeLength[i] = 5;

    makeCode(
        MAX_DIST_TREE_ELEMENTS, 
        len_cnt, 
        StaticDistanceTreeLength,
        g_StaticDistanceTreeCode
    );
}


//
// Initialise global deflate data in the DLL
//
VOID deflateInit(VOID)
{
    GenerateSlotTables();
    InitStaticBlock();
    GenerateStaticEncodingTables();

    // For the fast encoder, take the hard-coded global tree we're using (which is NOT the same as
    // a static block's tree), generate the bitwise output for outputting the structure of that
    // tree, and record that globally, so that we can do a simple memcpy() to output the tree for
    // the fast encoder, instead of calling the tree output routine all the time.  This is a nifty
    // performance optimisation.
    FastEncoderGenerateDynamicTreeEncoding();
}
