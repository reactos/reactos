/*
 * fastenc.h
 *
 * Defines for the fast encoder
 */

//
// Size of hash table for std encoder
//
#define FAST_ENCODER_HASH_TABLE_SIZE			2048
#define FAST_ENCODER_HASH_MASK					(FAST_ENCODER_HASH_TABLE_SIZE-1)
#define FAST_ENCODER_HASH_SHIFT					4

#define FAST_ENCODER_RECALCULATE_HASH(loc) \
	(((window[loc] << (2*FAST_ENCODER_HASH_SHIFT)) ^ \
	(window[loc+1] << FAST_ENCODER_HASH_SHIFT) ^ \
	(window[loc+2])) & FAST_ENCODER_HASH_MASK)


// 
// Be very careful about increasing the window size; the code tables will have to
// be updated, since they assume that extra_distance_bits is never larger than a
// certain size.
//
#define FAST_ENCODER_WINDOW_SIZE            8192
#define FAST_ENCODER_WINDOW_MASK            (FAST_ENCODER_WINDOW_SIZE - 1)


//
// Don't take a match 3 further away than this
//
#define FAST_ENCODER_MATCH3_DIST_THRESHOLD 16384


typedef struct fast_encoder
{
	// history window
	BYTE 					window[2*FAST_ENCODER_WINDOW_SIZE + MAX_MATCH + 4];

	// next most recent occurance of chars with same hash value
    t_search_node			prev[FAST_ENCODER_WINDOW_SIZE + MAX_MATCH];

	// hash table to find most recent occurance of chars with same hash value
	t_search_node			lookup[FAST_ENCODER_HASH_TABLE_SIZE];

    // have we output our block header (the whole data file will be one big dynamic block)?
    BOOL                    fOutputBlockHeader;

} t_fast_encoder;
