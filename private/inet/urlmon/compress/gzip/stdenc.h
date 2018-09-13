/*
 * stdenc.h
 *
 * Defines for the standard encoder
 */

//
// Size of hash table for std encoder
//
#define STD_ENCODER_HASH_TABLE_SIZE				8192
#define STD_ENCODER_HASH_MASK					(STD_ENCODER_HASH_TABLE_SIZE-1)
#define STD_ENCODER_HASH_SHIFT					5

#define STD_ENCODER_RECALCULATE_HASH(loc) \
	(((window[loc] << (2*STD_ENCODER_HASH_SHIFT)) ^ \
	(window[loc+1] << STD_ENCODER_HASH_SHIFT) ^ \
	(window[loc+2])) & STD_ENCODER_HASH_MASK)


//
// Maximum number of item we allow; this must be <= 65534, since this doesn't include
// freq[END_OF_BLOCK_CODE] = 1, which brings us to 65535; any more than this would make
// the frequency counts overflow, since they are stored in ushort's
//
// Note that this number does not affect the memory requirements in any way; that is
// determined by LIT_DIST_BUFFER_SIZE
//
// -8 for some slack (not really necessary)
//
#define STD_ENCODER_MAX_ITEMS				(65534-8)

//
// Size of the literal/distance buffer
//
#define STD_ENCODER_LIT_DIST_BUFFER_SIZE	32768

//
// Don't take a match 3 further away than this
// BUGBUG 4K seems a little close, but does do a marginally better job than 8K on 
// an 80K html file, so might as well leave it be
//
#define STD_ENCODER_MATCH3_DIST_THRESHOLD   4096


//
// Standard encoder context
//
typedef struct std_encoder
{
	// history window
	BYTE 					window[2*WINDOW_SIZE + MAX_MATCH + 4];

	// next most recent occurance of chars with same hash value
    t_search_node			prev[WINDOW_SIZE + MAX_MATCH];

	// hash table to find most recent occurance of chars with same hash value
	t_search_node			lookup[STD_ENCODER_HASH_TABLE_SIZE];

	// recording buffer for recording literals and distances
	BYTE					lit_dist_buffer[STD_ENCODER_LIT_DIST_BUFFER_SIZE];
	unsigned long			recording_bitbuf;
	unsigned long			recording_bitcount;
    BYTE *                  recording_bufptr;

	short					recording_dist_tree_table[REC_DISTANCES_DECODING_TABLE_SIZE];
	short					recording_dist_tree_left[2*MAX_DIST_TREE_ELEMENTS];
	short					recording_dist_tree_right[2*MAX_DIST_TREE_ELEMENTS];
    BYTE					recording_dist_tree_len[MAX_DIST_TREE_ELEMENTS];
    USHORT                  recording_dist_tree_code[MAX_DIST_TREE_ELEMENTS];

	short					recording_literal_tree_table[REC_LITERALS_DECODING_TABLE_SIZE];
	short					recording_literal_tree_left[2*MAX_LITERAL_TREE_ELEMENTS];
	short					recording_literal_tree_right[2*MAX_LITERAL_TREE_ELEMENTS];
	BYTE					recording_literal_tree_len[MAX_LITERAL_TREE_ELEMENTS];
	USHORT                  recording_literal_tree_code[MAX_LITERAL_TREE_ELEMENTS];

	// literal trees
    USHORT                  literal_tree_freq[2*MAX_LITERAL_TREE_ELEMENTS];
	USHORT                  literal_tree_code[MAX_LITERAL_TREE_ELEMENTS];
	BYTE					literal_tree_len[MAX_LITERAL_TREE_ELEMENTS];
	
	// dist trees
    USHORT                  dist_tree_freq[2*MAX_DIST_TREE_ELEMENTS];
	USHORT                  dist_tree_code[MAX_DIST_TREE_ELEMENTS];
	BYTE					dist_tree_len[MAX_DIST_TREE_ELEMENTS];
} t_std_encoder;
