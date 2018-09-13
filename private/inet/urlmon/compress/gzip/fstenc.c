/*
 * fstenc.c
 *
 * Fast encoder
 *
 * This is a one pass encoder which uses predefined trees.  However, since these are not the same
 * trees defined for a fixed block (we use better trees than that), we output a dynamic block header.
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"
#include "fasttbl.h"


//
// For debugging purposes:
//
// Verifies that all of the hash pointers in the hash table are correct, and that everything
// in the same hash chain has the same hash value
//
#ifdef FULL_DEBUG
#define VERIFY_HASHES(bufpos) FastEncoderVerifyHashes(context, bufpos)
#else
#define VERIFY_HASHES(bufpos) ;
#endif


//
// Update hash variable "h" with character c
//
#define UPDATE_HASH(h,c) \
	h = ((h) << FAST_ENCODER_HASH_SHIFT) ^ (c);


//
// Insert a string into the hash chain at location bufpos
//
#define INSERT_STRING(search,bufpos) \
{ \
	UPDATE_HASH(hash, window[bufpos+2]); \
\
	_ASSERT((unsigned int) FAST_ENCODER_RECALCULATE_HASH(bufpos) == (unsigned int) (hash & FAST_ENCODER_HASH_MASK)); \
\
    search = lookup[hash & FAST_ENCODER_HASH_MASK]; \
	lookup[hash & FAST_ENCODER_HASH_MASK] = (t_search_node) (bufpos); \
	prev[bufpos & FAST_ENCODER_WINDOW_MASK] = (t_search_node) (search); \
}


//
// Output bits function which uses local variables for the bit buffer
//
#define LOCAL_OUTPUT_BITS(n, x) \
{ \
	bitbuf |= ((x) << bitcount); \
	bitcount += (n); \
	if (bitcount >= 16) \
    { \
		*output_curpos++ = (BYTE) bitbuf; \
		*output_curpos++ = (BYTE) (bitbuf >> 8); \
		bitcount -= 16; \
		bitbuf >>= 16; \
	} \
}


//
// Output unmatched symbol c
//
#define OUTPUT_CHAR(c) \
    LOCAL_OUTPUT_BITS(g_FastEncoderLiteralCodeInfo[c] & 31, g_FastEncoderLiteralCodeInfo[c] >> 5);


//
// Output a match with length match_len (>= MIN_MATCH) and displacement match_pos
//
// Optimisation: unlike the other encoders, here we have an array of codes for each match
// length (not just each match length slot), complete with all the extra bits filled in, in
// a single array element.  
//
// There are many advantages to doing this:
//
// 1. A single array lookup on g_FastEncoderLiteralCodeInfo, instead of separate array lookups
//    on g_LengthLookup (to get the length slot), g_FastEncoderLiteralTreeLength, 
//    g_FastEncoderLiteralTreeCode, g_ExtraLengthBits, and g_BitMask
//
// 2. The array is an array of ULONGs, so no access penalty, unlike for accessing those USHORT
//    code arrays in the other encoders (although they could be made into ULONGs with some
//    modifications to the source).
//
// Note, if we could guarantee that code_len <= 16 always, then we could skip an if statement here.
//
// A completely different optimisation is used for the distance codes since, obviously, a table for 
// all 8192 distances combining their extra bits is not feasible.  The distance codeinfo table is 
// made up of code[], len[] and # extra_bits for this code.
//
// The advantages are similar to the above; a ULONG array instead of a USHORT and BYTE array, better
// cache locality, fewer memory operations.
//
#define OUTPUT_MATCH(match_len, match_pos) \
{ \
    int extra_bits; \
    int code_len; \
    ULONG code_info; \
\
	_ASSERT(match_len >= MIN_MATCH && match_len <= MAX_MATCH); \
\
    code_info = g_FastEncoderLiteralCodeInfo[(NUM_CHARS+1-MIN_MATCH)+match_len]; \
    code_len = code_info & 31; \
    _ASSERT(code_len != 0); \
    if (code_len <= 16) \
    { \
        LOCAL_OUTPUT_BITS(code_len, code_info >> 5); \
    } \
    else \
    { \
        LOCAL_OUTPUT_BITS(16, (code_info >> 5) & 65535); \
        LOCAL_OUTPUT_BITS(code_len-16, code_info >> (5+16)); \
    } \
    code_info = g_FastEncoderDistanceCodeInfo[POS_SLOT(match_pos)]; \
    LOCAL_OUTPUT_BITS(code_info & 15, code_info >> 8); \
    extra_bits = (code_info >> 4) & 15; \
    if (extra_bits != 0) LOCAL_OUTPUT_BITS(extra_bits, (match_pos) & g_BitMask[extra_bits]); \
}


//
// This commented out code is the old way of doing things, which is what the other encoders use
//
#if 0
#define OUTPUT_MATCH(match_len, match_pos) \
{ \
	int pos_slot = POS_SLOT(match_pos); \
	int len_slot = g_LengthLookup[match_len - MIN_MATCH]; \
    int extra_bits; \
\
	_ASSERT(match_len >= MIN_MATCH && match_len <= MAX_MATCH); \
    _ASSERT(g_FastEncoderLiteralTreeLength[(NUM_CHARS+1)+len_slot] != 0); \
    _ASSERT(g_FastEncoderDistanceTreeLength[pos_slot] != 0); \
\
    LOCAL_OUTPUT_BITS(g_FastEncoderLiteralTreeLength[(NUM_CHARS+1)+len_slot], g_FastEncoderLiteralTreeCode[(NUM_CHARS+1)+len_slot]); \
    extra_bits = g_ExtraLengthBits[len_slot]; \
    if (extra_bits != 0) LOCAL_OUTPUT_BITS(extra_bits, (match_len-MIN_MATCH) & g_BitMask[extra_bits]); \
\
    LOCAL_OUTPUT_BITS(g_FastEncoderDistanceTreeLength[pos_slot], g_FastEncoderDistanceTreeCode[pos_slot]); \
    extra_bits = g_ExtraDistanceBits[pos_slot]; \
    if (extra_bits != 0) LOCAL_OUTPUT_BITS(extra_bits, (match_pos) & g_BitMask[extra_bits]); \
}
#endif


//
// Local function prototypes
//
static void FastEncoderMoveWindows(t_encoder_context *context);

static int FastEncoderFindMatch(
    const BYTE *    window,
    const USHORT *  prev,
    long            bufpos, 
    long            search, 
    t_match_pos *   match_pos, 
    int             cutoff,
    int             nice_length
);


//
// Output the block type and tree structure for our hard-coded trees.
//
// Functionally equivalent to:
//
// outputBits(context, 1, 1); // "final" block flag
// outputBits(context, 2, BLOCKTYPE_DYNAMIC);
// outputTreeStructure(context, g_FastEncoderLiteralTreeLength, g_FastEncoderDistanceTreeLength);
//
// However, all of the above has smartly been cached in global data, so we just memcpy().
//
void FastEncoderOutputPreamble(t_encoder_context *context)
{
#if 0
    // slow way:
    outputBits(context, 1+2, 1 | (BLOCKTYPE_DYNAMIC << 1));
    outputTreeStructure(context, g_FastEncoderLiteralTreeLength, g_FastEncoderDistanceTreeLength);
#endif

    // make sure tree has been init
    _ASSERT(g_FastEncoderTreeLength > 0);

    // make sure we have enough space to output tree
    _ASSERT(context->output_curpos + g_FastEncoderTreeLength < context->output_endpos);

    // fast way:
    memcpy(context->output_curpos, g_FastEncoderTreeStructureData, g_FastEncoderTreeLength);
    context->output_curpos += g_FastEncoderTreeLength;

    // need to get final states of bitbuf and bitcount after outputting all that stuff
    context->bitbuf = g_FastEncoderPostTreeBitbuf;
    context->bitcount = g_FastEncoderPostTreeBitcount;
}


//
// Fast encoder deflate function
//
void FastEncoderDeflate(
	t_encoder_context *	context, 
    int                 search_depth, // # hash links to traverse
	int					lazy_match_threshold, // don't search @ X+1 if match length @ X is > lazy
    int                 good_length, // divide traversal depth by 4 if match length > good
    int                 nice_length // in match finder, if we find >= nice_length match, quit immediately
)
{
	long			bufpos;
	unsigned int	hash;
    unsigned long   bitbuf;
    int             bitcount;
    BYTE *          output_curpos;
    t_fast_encoder *encoder = context->fast_encoder;
	byte *			window = encoder->window; // make local copies of context variables
	t_search_node *	prev = encoder->prev;
	t_search_node *	lookup = encoder->lookup;

    //
    // If this is the first time in here (since last reset) then we need to output our dynamic
    // block header
    //
    if (encoder->fOutputBlockHeader == FALSE)
    {
        encoder->fOutputBlockHeader = TRUE;

        //
        // Watch out!  Calls to outputBits() and outputTreeStructure() use the bit buffer 
        // variables stored in the context, not our local cached variables.
        //
        FastEncoderOutputPreamble(context);
    }

    //
    // Copy bitbuf vars into local variables since we're now using OUTPUT_BITS macro.
    // Do not call anything that uses the context structure's bit buffer variables!
    //
    output_curpos   = context->output_curpos;
    bitbuf          = context->bitbuf;
    bitcount        = context->bitcount;

    // copy bufpos into local variable
    bufpos = context->bufpos;

	VERIFY_HASHES(bufpos); // debug mode: verify that the hash table is correct

    // initialise the value of the hash
    // no problem if locations bufpos, bufpos+1 are invalid (not enough data), since we will 
    // never insert using that hash value
	hash = 0;
	UPDATE_HASH(hash, window[bufpos]);
	UPDATE_HASH(hash, window[bufpos+1]);

    // while we haven't come to the end of the input, and we still aren't close to the end
    // of the output
	while (bufpos < context->bufpos_end && output_curpos < context->output_near_end_threshold)
	{
		int				match_len;
		t_match_pos		match_pos;
		t_match_pos		search;

    	VERIFY_HASHES(bufpos); // debugger: verify that hash table is correct

		if (context->bufpos_end - bufpos <= 3)
		{
			// The hash value becomes corrupt when we get within 3 characters of the end of the
            // input buffer, since the hash value is based on 3 characters.  We just stop
            // inserting into the hash table at this point, and allow no matches.
			match_len = 0;
		}
		else
		{
            // insert string into hash table and return most recent location of same hash value
			INSERT_STRING(search,bufpos);

            // did we find a recent location of this hash value?
			if (search != 0)
			{
    			// yes, now find a match at what we'll call position X
				match_len = FastEncoderFindMatch(window, prev, bufpos, search, &match_pos, search_depth, nice_length);

				// truncate match if we're too close to the end of the input buffer
				if (bufpos + match_len > context->bufpos_end)
					match_len = context->bufpos_end - bufpos;
			}
			else
			{
                // no most recent location found
				match_len = 0;
			}
		}

		if (match_len < MIN_MATCH)
		{
            // didn't find a match, so output unmatched char
			OUTPUT_CHAR(window[bufpos]);
    		bufpos++;
		}
		else
		{
	    	// bufpos now points to X+1
    		bufpos++;

			// is this match so good (long) that we should take it automatically without
			// checking X+1 ?
			if (match_len <= lazy_match_threshold)
			{
				int				next_match_len;
				t_match_pos		next_match_pos;

				// sets search
				INSERT_STRING(search,bufpos);

				// no, so check for a better match at X+1
				if (search != 0)
				{
					next_match_len = FastEncoderFindMatch(
						window,
                        prev,
						bufpos, 
						search,
						&next_match_pos,
						match_len < good_length ? search_depth : (search_depth >> 2),
                        nice_length
					);
				
					// truncate match if we're too close to the end of the buffer
					// note: next_match_len could now be < MIN_MATCH
					if (bufpos + next_match_len > context->bufpos_end)
						next_match_len = context->bufpos_end - bufpos;
				}
				else
				{
					next_match_len = 0;
				}

				// right now X and X+1 are both inserted into the search tree
				if (next_match_len > match_len)
				{
					// since next_match_len > match_len, it can't be < MIN_MATCH here

					// match at X+1 is better, so output unmatched char at X
					OUTPUT_CHAR(window[bufpos-1]);

					// now output match at location X+1
					OUTPUT_MATCH(next_match_len, next_match_pos);

					// insert remainder of second match into search tree
					// 
					// example: (*=inserted already)
					//
					// X      X+1               X+2      X+3     X+4
					// *      *
					//        nextmatchlen=3
					//        bufpos
					//
					// If next_match_len == 3, we want to perform 2
					// insertions (at X+2 and X+3).  However, first we must 
					// inc bufpos.
					//
					bufpos++; // now points to X+2
					match_len = next_match_len;
					goto insert;
				}
				else
				{
					// match at X is better, so take it
					OUTPUT_MATCH(match_len, match_pos);

					//
					// Insert remainder of first match into search tree, minus the first
					// two locations, which were inserted by the FindMatch() calls.
					// 
					// For example, if match_len == 3, then we've inserted at X and X+1
					// already (and bufpos is now pointing at X+1), and now we need to insert 
					// only at X+2.
					//
					match_len--;
					bufpos++; // now bufpos points to X+2
					goto insert;
				}
			}
			else /* match_length >= good_match */
			{
				// in assertion: bufpos points to X+1, location X inserted already
					
				// first match is so good that we're not even going to check at X+1
				OUTPUT_MATCH(match_len, match_pos);

				// insert remainder of match at X into search tree
insert:
				if (context->bufpos_end - bufpos <= match_len)
				{
					bufpos += (match_len-1);
				}
				else
				{
   					while (--match_len > 0)
    				{
	    				t_match_pos ignore;

		    			INSERT_STRING(ignore,bufpos);
			    		bufpos++;
				    }
				}
			}
		}
	} /* end ... while (bufpos < bufpos_end) */

    // store local variables back in context
	context->bufpos = bufpos;
    context->bitbuf = bitbuf;
    context->bitcount = bitcount;
    context->output_curpos = output_curpos;

	VERIFY_HASHES(bufpos); // debugger: verify that hash table is correct

    if (bufpos == context->bufpos_end)
        context->state = STATE_NORMAL;
    else
        context->state = STATE_OUTPUTTING_BLOCK;

    // slide the window if bufpos has reached 2*window size
    if (context->bufpos == 2*FAST_ENCODER_WINDOW_SIZE)
        FastEncoderMoveWindows(context);
}


static void FastEncoderMoveWindows(t_encoder_context *context)
{
	t_search_node *lookup = context->fast_encoder->lookup;
	t_search_node *prev = context->fast_encoder->prev;
	BYTE *window = context->fast_encoder->window;
	int i;

    _ASSERT(context->bufpos == 2*FAST_ENCODER_WINDOW_SIZE);

    // verify that the hash table is correct
	VERIFY_HASHES(2*FAST_ENCODER_WINDOW_SIZE);

	memcpy(&window[0], &window[context->bufpos - FAST_ENCODER_WINDOW_SIZE], FAST_ENCODER_WINDOW_SIZE);

    // move all the hash pointers back
    // BUGBUG We are incurring a performance penalty since lookup[] is a USHORT array.  Would be
    // nice to subtract from two locations at a time.
	for (i = 0; i < FAST_ENCODER_HASH_TABLE_SIZE; i++)
	{
		long val = ((long) lookup[i]) - FAST_ENCODER_WINDOW_SIZE;

		if (val <= 0) // too far away now? then set to zero
			lookup[i] = (t_search_node) 0;
		else
			lookup[i] = (t_search_node) val;
	}

    // prev[]'s are absolute pointers, not relative pointers, so we have to move them back too
    // making prev[]'s into relative pointers poses problems of its own
	for (i = 0; i < FAST_ENCODER_WINDOW_SIZE; i++)
	{
		long val = ((long) prev[i]) - FAST_ENCODER_WINDOW_SIZE;

		if (val <= 0)
			prev[i] = (t_search_node) 0;
		else
			prev[i] = (t_search_node) val;
	}

#ifdef FULL_DEBUG
    // For debugging, wipe the window clean, so that if there is a bug in our hashing,
    // the hash pointers will now point to locations which are not valid for the hash value
    // (and will be caught by our ASSERTs).
	memset(&window[FAST_ENCODER_WINDOW_SIZE], 0, FAST_ENCODER_WINDOW_SIZE);
#endif

	VERIFY_HASHES(2*FAST_ENCODER_WINDOW_SIZE); // debug: verify hash table is correct

	context->bufpos = FAST_ENCODER_WINDOW_SIZE;
	context->bufpos_end = context->bufpos;
}


//
// Find match
//
// Returns match length found.  A match length < MIN_MATCH means no match was found.
//
static int FastEncoderFindMatch(
    const BYTE *    window, // window array
    const USHORT *  prev,   // prev ptr array
    long            bufpos, // current buffer position
    long            search, // where to start searching
    t_match_pos *   match_pos, // return match position here
    int             cutoff, // # links to traverse
    int             nice_length // stop immediately if we find a match >= nice_length
)
{
    // make local copies of context variables
	long			earliest;
	int				best_match = 0; // best match length found so far
	t_match_pos		l_match_pos; // absolute match position of best match found
    BYTE            want_char;

	_ASSERT(bufpos >= 0 && bufpos < 2*FAST_ENCODER_WINDOW_SIZE);
	_ASSERT(search < bufpos);
	_ASSERT(FAST_ENCODER_RECALCULATE_HASH(search) == FAST_ENCODER_RECALCULATE_HASH(bufpos));

    // the earliest we can look
	earliest = bufpos - FAST_ENCODER_WINDOW_SIZE;
    _ASSERT(earliest >= 0);

    // store window[bufpos + best_match]
    want_char = window[bufpos];

	while (search > earliest)
	{
        // make sure all our hash links are valid
		_ASSERT(FAST_ENCODER_RECALCULATE_HASH(search) == FAST_ENCODER_RECALCULATE_HASH(bufpos));

        // Start by checking the character that would allow us to increase the match
        // length by one.  This improves performance quite a bit.
		if (window[search + best_match] == want_char)
		{
			int j;

            // Now make sure that all the other characters are correct
			for (j = 0; j < MAX_MATCH; j++)
			{
				if (window[bufpos+j] != window[search+j])
					break;
			}
	
			if (j > best_match)
			{
				best_match	= j;
				l_match_pos	= search; // absolute position

				if (j > nice_length)
					break;

                want_char = window[bufpos+j];
			}
		}

		if (--cutoff == 0)
			break;

        // make sure we're always going backwards
        _ASSERT(prev[search & FAST_ENCODER_WINDOW_MASK] < search);

		search = (long) prev[search & FAST_ENCODER_WINDOW_MASK];
	}

    // doesn't necessarily mean we found a match; best_match could be > 0 and < MIN_MATCH
	*match_pos = bufpos - l_match_pos - 1; // convert absolute to relative position

    // don't allow match length 3's which are too far away to be worthwhile
	if (best_match == 3 && *match_pos >= FAST_ENCODER_MATCH3_DIST_THRESHOLD)
		return 0;

	_ASSERT(best_match < MIN_MATCH || *match_pos < FAST_ENCODER_WINDOW_SIZE);

	return best_match;
}


void FastEncoderReset(t_encoder_context *context)
{
	_ASSERT(context->fast_encoder != NULL);

    // zero hash table
	memset(context->fast_encoder->lookup, 0, sizeof(context->fast_encoder->lookup));

    context->window_size = FAST_ENCODER_WINDOW_SIZE;
	context->bufpos = FAST_ENCODER_WINDOW_SIZE;
    context->bufpos_end = context->bufpos;
    context->fast_encoder->fOutputBlockHeader = FALSE;
}


BOOL FastEncoderInit(t_encoder_context *context)
{
	context->fast_encoder = (t_fast_encoder *) LocalAlloc(LMEM_FIXED, sizeof(t_fast_encoder));

    if (context->fast_encoder == NULL)
        return FALSE;

	FastEncoderReset(context);
	return TRUE;
}


//
// Pregenerate the structure of the dynamic tree header which is output for
// the fast encoder.  Also record the final states of bitcount and bitbuf
// after outputting.
//
void FastEncoderGenerateDynamicTreeEncoding(void)
{
    t_encoder_context context;

    // Create a fake context with output pointers into our global data
    memset(&context, 0, sizeof(context));
    context.output_curpos = g_FastEncoderTreeStructureData;
    context.output_endpos = g_FastEncoderTreeStructureData + sizeof(g_FastEncoderTreeStructureData);
    context.output_near_end_threshold = context.output_endpos - 16;
    InitBitBuffer(&context);

    outputBits(&context, 1, 1); // "final" block flag
    outputBits(&context, 2, BLOCKTYPE_DYNAMIC);
   
    outputTreeStructure(
        &context,
	    g_FastEncoderLiteralTreeLength, 
	    g_FastEncoderDistanceTreeLength
    );

    g_FastEncoderTreeLength = (int) (context.output_curpos - (BYTE *) g_FastEncoderTreeStructureData);
    g_FastEncoderPostTreeBitbuf = context.bitbuf;
    g_FastEncoderPostTreeBitcount = context.bitcount;
}
