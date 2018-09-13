//
// deflate.h
//

// common to inflate and deflate
#include "common.h"


// ZIP constants
#define NUM_LENGTH_BASE_CODES		29
#define NUM_DIST_BASE_CODES			30

#define NUM_PRETREE_ELEMENTS		19


//
// For std and optimal encoders, recording buffer encoding max bit lengths and
// decoding table sizes
//
#define REC_LITERALS_DECODING_TABLE_BITS 12
#define REC_DISTANCES_DECODING_TABLE_BITS 8

#define REC_LITERALS_DECODING_TABLE_SIZE (1 << REC_LITERALS_DECODING_TABLE_BITS)
#define REC_LITERALS_DECODING_TABLE_MASK (REC_LITERALS_DECODING_TABLE_SIZE-1)

#define REC_DISTANCES_DECODING_TABLE_SIZE (1 << REC_DISTANCES_DECODING_TABLE_BITS)
#define REC_DISTANCES_DECODING_TABLE_MASK (REC_DISTANCES_DECODING_TABLE_SIZE-1)

//
// The maximum code lengths to allow for recording (we don't want really large
// 15 bit codes, just in case uncommon chars suddenly become common due to a change
// in the data).
//
#define RECORDING_DIST_MAX_CODE_LEN	9
#define RECORDING_LIT_MAX_CODE_LEN	13


//
// Max size of tree output (in bytes)
//
// We require that the output buffer have at least this much data available, so that we can
// output the tree in one chunk
//
#define MAX_TREE_DATA_SIZE			512


//
// Return the position slot (0...29) of a match offset (0...32767)
//
#define POS_SLOT(pos) g_DistLookup[((pos) < 256) ? (pos) : (256 + ((pos) >> 7))]


// context structure
#include "defctxt.h"

// encoders
#include "stdenc.h"
#include "optenc.h"
#include "fastenc.h"

// prototypes
#include "defproto.h"

// variables
#include "defdata.h"
#include "comndata.h"
