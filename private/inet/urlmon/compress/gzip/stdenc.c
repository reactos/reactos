/*
 * stdenc.c
 *
 * Standard encoder
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


//
// Update hash variable "h" with character c
//
#define UPDATE_HASH(h,c) \
	h = ((h) << STD_ENCODER_HASH_SHIFT) ^ (c);


//
// Insert a string into the hash chain at location bufpos
//
// Assertions check that we never attempt to insert near the end of the buffer
// (since our hash value is based on values at bufpos, bufpos+1, bufpos+2) and
// that our hash value is always valid for the bytes we are inserting.
//
#define INSERT_STRING(search,bufpos) \
{ \
    _ASSERT((bufpos + 2) < context->bufpos_end); \
	UPDATE_HASH(hash, window[bufpos+2]); \
	_ASSERT((unsigned int) STD_ENCODER_RECALCULATE_HASH(bufpos) == (unsigned int) (hash & STD_ENCODER_HASH_MASK)); \
	search = lookup[hash & STD_ENCODER_HASH_MASK]; \
	lookup[hash & STD_ENCODER_HASH_MASK] = (t_search_node) (bufpos); \
	prev[bufpos & WINDOW_MASK] = (t_search_node) search; \
}


#define CHECK_FLUSH_RECORDING_BUFFER() \
	if (recording_bitcount >= 16) \
	{ \
		*recording_bufptr++ = (BYTE) recording_bitbuf; \
		*recording_bufptr++ = (BYTE) (recording_bitbuf >> 8); \
		recording_bitbuf >>= 16; \
		recording_bitcount -= 16; \
	}


#define OUTPUT_RECORDING_DATA(count,data) \
	recording_bitbuf |= ((data) << recording_bitcount); \
	recording_bitcount += (count);


//
// Record unmatched symbol c
//
#define RECORD_CHAR(c) \
    context->outputting_block_num_literals++; \
    context->std_encoder->literal_tree_freq[c]++; \
	_ASSERT(context->std_encoder->recording_literal_tree_len[c] != 0); \
	OUTPUT_RECORDING_DATA(context->std_encoder->recording_literal_tree_len[c], context->std_encoder->recording_literal_tree_code[c]); \
	CHECK_FLUSH_RECORDING_BUFFER();


//
// Record a match with length match_len (>= MIN_MATCH) and displacement match_pos
//
#define RECORD_MATCH(match_len, match_pos) \
{ \
	int pos_slot = POS_SLOT(match_pos); \
	int len_slot = g_LengthLookup[match_len - MIN_MATCH]; \
	int item = (NUM_CHARS+1) + len_slot; \
	int extra_dist_bits = g_ExtraDistanceBits[pos_slot]; \
	int extra_len_bits = g_ExtraLengthBits[len_slot]; \
	_ASSERT(match_len >= MIN_MATCH && match_len <= MAX_MATCH); \
	_ASSERT(context->outputting_block_num_literals >= 0 && context->outputting_block_num_literals < STD_ENCODER_MAX_ITEMS); \
	_ASSERT(context->std_encoder->recording_literal_tree_len[item] != 0); \
	_ASSERT(context->std_encoder->recording_dist_tree_len[pos_slot] != 0); \
    context->outputting_block_num_literals++; \
    context->std_encoder->literal_tree_freq[(NUM_CHARS + 1) + len_slot]++; \
    context->std_encoder->dist_tree_freq[pos_slot]++; \
	OUTPUT_RECORDING_DATA(context->std_encoder->recording_literal_tree_len[item], context->std_encoder->recording_literal_tree_code[item]); \
	CHECK_FLUSH_RECORDING_BUFFER(); \
	if (extra_len_bits > 0) \
	{ \
		OUTPUT_RECORDING_DATA(extra_len_bits, (match_len-MIN_MATCH) & ((1 << extra_len_bits)-1)); \
		CHECK_FLUSH_RECORDING_BUFFER(); \
	} \
	OUTPUT_RECORDING_DATA(context->std_encoder->recording_dist_tree_len[pos_slot], context->std_encoder->recording_dist_tree_code[pos_slot]); \
	CHECK_FLUSH_RECORDING_BUFFER(); \
	if (extra_dist_bits > 0) \
	{ \
		OUTPUT_RECORDING_DATA(extra_dist_bits, match_pos & ((1 << extra_dist_bits)-1)); \
		CHECK_FLUSH_RECORDING_BUFFER(); \
	} \
}


#define FLUSH_RECORDING_BITBUF() \
    *recording_bufptr++ = (BYTE) recording_bitbuf; \
	*recording_bufptr++ = (BYTE) (recording_bitbuf >> 8); 


//
// Verifies that all of the hash pointers in the hash table are correct, and that everything
// in the same hash chain has the same hash value
//
#ifdef FULL_DEBUG
#define VERIFY_HASHES(bufpos) StdEncoderVerifyHashes(context, bufpos)
#else
#define VERIFY_HASHES(bufpos) ;
#endif


static void StdEncoderMoveWindows(t_encoder_context *context);

static int StdEncoderFindMatch(
    const BYTE *        window,
    const USHORT *      prev,
    long                bufpos, 
    long                search, 
    unsigned int *      match_pos, 
    int                 cutoff,
    int                 nice_length
);


void StdEncoderDeflate(
	t_encoder_context *	context, 
    int                 search_depth,
	int					lazy_match_threshold,
    int                 good_length,
    int                 nice_length
)
{
	long			bufpos;
	unsigned int	hash;
    t_std_encoder * encoder = context->std_encoder;
	byte *			window = encoder->window;
	t_search_node *	prev = encoder->prev;
	t_search_node *	lookup = encoder->lookup;
	unsigned long	recording_bitbuf;
	int				recording_bitcount;
	byte *			recording_bufptr;
    byte *          end_recording_bufptr;

	// restore literal/match bitmap variables
    end_recording_bufptr    = &encoder->lit_dist_buffer[STD_ENCODER_LIT_DIST_BUFFER_SIZE-8];
	recording_bufptr        = encoder->recording_bufptr;
    recording_bitbuf        = encoder->recording_bitbuf;
    recording_bitcount      = encoder->recording_bitcount;
	bufpos			        = context->bufpos;

	VERIFY_HASHES(bufpos);

    //
    // Recalculate our hash
    //
    // One disadvantage of the way we do our hashing is that matches are not permitted in the last
    // few characters near bufpos_end.
	//
    hash = 0;
	UPDATE_HASH(hash, window[bufpos]);
	UPDATE_HASH(hash, window[bufpos+1]);

	while (bufpos < context->bufpos_end)
	{
		int				match_len;
		t_match_pos		match_pos;
		t_match_pos		search;

        if (context->bufpos_end - bufpos <= 3)
		{
			// don't insert any strings when we get close to the end of the buffer,
            // since we will end up using corrupted hash values (the data after bufpos_end
            // is undefined, and those bytes would be swept into the hash value if we
            // calculated a hash at bufpos_end-2, for example, since our hash value is
            // build from 3 consecutive characters in the buffer).
			match_len = 0;
		}
		else
		{
			INSERT_STRING(search,bufpos);

			// find a match at what we'll call position X
			if (search != 0)
			{
				match_len = StdEncoderFindMatch(window, prev, bufpos, search, &match_pos, search_depth, nice_length);

				// truncate match if we're too close to the end of the buffer
				if (bufpos + match_len > context->bufpos_end)
					match_len = context->bufpos_end - bufpos;
			}
			else
			{
				match_len = 0;
			}
		}

		if (match_len < MIN_MATCH)
		{
			// didn't find a match, so output unmatched char
			RECORD_CHAR(window[bufpos]);
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
					next_match_len = StdEncoderFindMatch(
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
					RECORD_CHAR(window[bufpos-1]);

					// now output match at location X+1
					RECORD_MATCH(next_match_len, next_match_pos);

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
					RECORD_MATCH(match_len, match_pos);

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
				RECORD_MATCH(match_len, match_pos);

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
						t_match_pos ignore; // we're not interested in the search position

						INSERT_STRING(ignore,bufpos);
						bufpos++;
					}
				}
			}
		}

		// literal buffer or distance buffer filled up (or close to filling up)?
		if (context->outputting_block_num_literals >= STD_ENCODER_MAX_ITEMS-4 ||
            recording_bufptr >= end_recording_bufptr)
		{
			// yes, then we must output a block
			_ASSERT(context->outputting_block_num_literals <= STD_ENCODER_MAX_ITEMS);

			// flush our recording matches bit buffer
            FLUSH_RECORDING_BITBUF();

			StdEncoderOutputBlock(context);

			// did we output the whole block?
			if (context->state != STATE_NORMAL)
				break;

			// we did output the whole block, so reset literal encoding
        	recording_bufptr = encoder->recording_bufptr;
            recording_bitbuf = encoder->recording_bitbuf;
            recording_bitcount = encoder->recording_bitcount;
		}
	} /* end ... while (bufpos < bufpos_end) */

    _ASSERT(bufpos <= context->bufpos_end);

	// save recording state
	encoder->recording_bufptr = recording_bufptr;
    encoder->recording_bitbuf = recording_bitbuf;
    encoder->recording_bitcount = recording_bitcount;

	context->bufpos = bufpos;

	VERIFY_HASHES(bufpos);

    if (context->bufpos == 2*WINDOW_SIZE)
        StdEncoderMoveWindows(context);
}


static int StdEncoderFindMatch(
    const BYTE *        window,
    const USHORT *      prev,
    long                bufpos, 
    long                search, 
    unsigned int *      match_pos, 
    int                 cutoff,
    int                 nice_length
)
{
	const BYTE *	window_bufpos = &window[bufpos];
	long			earliest; // how far back we can look
	int				best_match = 0; // best match length found so far
	t_match_pos		l_match_pos;

	_ASSERT(bufpos >= 0 && bufpos < 2*WINDOW_SIZE);
	_ASSERT(search < bufpos);
	_ASSERT(STD_ENCODER_RECALCULATE_HASH(search) == STD_ENCODER_RECALCULATE_HASH(bufpos));

	earliest = bufpos - WINDOW_SIZE;
    _ASSERT(earliest >= 0);

	while (search > earliest)
	{
		_ASSERT(STD_ENCODER_RECALCULATE_HASH(search) == STD_ENCODER_RECALCULATE_HASH(bufpos));
        _ASSERT(search < bufpos);

		if (window_bufpos[best_match] == window[search + best_match])
		{
			int j;

			for (j = 0; j < MAX_MATCH; j++)
			{
				if (window_bufpos[j] != window[search+j])
					break;
			}
	
			if (j > best_match)
			{
				best_match	= j;
				l_match_pos	= search; // absolute position

				if (j > nice_length)
					break;
			}
		}

		if (--cutoff == 0)
			break;

		search = (long) prev[search & WINDOW_MASK];
	}

    // turn l_match_pos into relative position
	l_match_pos = bufpos - l_match_pos - 1; 

	if (best_match == 3 && l_match_pos >= STD_ENCODER_MATCH3_DIST_THRESHOLD)
		return 0;

	_ASSERT(best_match < MIN_MATCH || l_match_pos < WINDOW_SIZE);
    *match_pos = l_match_pos;

	return best_match;
}


static void StdEncoderMoveWindows(t_encoder_context *context)
{
	if (context->bufpos >= 2*WINDOW_SIZE)
	{
		int		i;
		t_search_node *lookup = context->std_encoder->lookup;
		t_search_node *prev = context->std_encoder->prev;
		BYTE *window = context->std_encoder->window;

		VERIFY_HASHES(2*WINDOW_SIZE);

		memcpy(&window[0], &window[context->bufpos - WINDOW_SIZE], WINDOW_SIZE);

		for (i = 0; i < STD_ENCODER_HASH_TABLE_SIZE; i++)
		{
			long val = ((long) lookup[i]) - WINDOW_SIZE;
	
			if (val <= 0)
				lookup[i] = (t_search_node) 0;
			else
				lookup[i] = (t_search_node) val;
		}

		for (i = 0; i < WINDOW_SIZE; i++)
		{
			long val = ((long) prev[i]) - WINDOW_SIZE;
	
			if (val <= 0)
				prev[i] = (t_search_node) 0;
			else
				prev[i] = (t_search_node) val;
		}

#ifdef FULL_DEBUG
		memset(&window[WINDOW_SIZE], 0, WINDOW_SIZE);
#endif

		VERIFY_HASHES(2*WINDOW_SIZE);

		
		context->bufpos = WINDOW_SIZE;
		context->bufpos_end = context->bufpos;
	}
}


//
// Zero the running frequency counts
//
// Also set freq[END_OF_BLOCK_CODE] = 1
//
void StdEncoderZeroFrequencyCounts(t_std_encoder *encoder)
{
    _ASSERT(encoder != NULL);

  	memset(encoder->literal_tree_freq, 0, sizeof(encoder->literal_tree_freq));
    memset(encoder->dist_tree_freq, 0, sizeof(encoder->dist_tree_freq));
    encoder->literal_tree_freq[END_OF_BLOCK_CODE] = 1;
}


void StdEncoderReset(t_encoder_context *context)
{
    t_std_encoder *encoder = context->std_encoder;

	_ASSERT(encoder != NULL);
	memset(encoder->lookup, 0, sizeof(encoder->lookup));

    context->window_size        = WINDOW_SIZE;
	context->bufpos		        = context->window_size;
	context->bufpos_end         = context->bufpos;

	encoder->recording_bitbuf	= 0;
	encoder->recording_bitcount = 0;
    encoder->recording_bufptr   = encoder->lit_dist_buffer;

	DeflateInitRecordingTables(
	    encoder->recording_literal_tree_len,
    	encoder->recording_literal_tree_code, 
	    encoder->recording_dist_tree_len,
    	encoder->recording_dist_tree_code
    );

    StdEncoderZeroFrequencyCounts(encoder);
}


BOOL StdEncoderInit(t_encoder_context *context)
{
	context->std_encoder = (t_std_encoder *) LocalAlloc(LMEM_FIXED, sizeof(t_std_encoder));

    if (context->std_encoder == NULL)
        return FALSE;

	StdEncoderReset(context);
	return TRUE;
}
