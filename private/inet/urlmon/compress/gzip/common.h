/*
 * common.h
 *
 * Definitions common to inflate and deflate
 */
#include "types.h"

#define NUM_CHARS				256
#define MIN_MATCH				3
#define MAX_MATCH				258

// window size
#define WINDOW_SIZE				32768
#define WINDOW_MASK				32767

// ZIP block types
#define BLOCKTYPE_UNCOMPRESSED	0
#define BLOCKTYPE_FIXED			1
#define BLOCKTYPE_DYNAMIC		2

// it's 288 and not 286 because we of the two extra codes which can appear
// in a static block; same for 32 vs 30 for distances
#define MAX_LITERAL_TREE_ELEMENTS	288
#define MAX_DIST_TREE_ELEMENTS		32
	
#define END_OF_BLOCK_CODE		256
#define	NUM_PRETREE_ELEMENTS	19
