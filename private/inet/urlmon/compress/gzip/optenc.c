/*
 * optenc.c
 *
 * Optimal encoder
 *
 * BUGBUG  Can improve compression by using the "redo" method of LZX; after the first 32K bytes,
 * reset the compressor but keep the tables, and start over.
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"


//
// If we get a match this good, take it automatically
//
// Note: FAST_DECISION_THRESHOLD can be set to anything; it's been set to BREAK_LENGTH
//       arbitrarily
//
#define FAST_DECISION_THRESHOLD BREAK_LENGTH


//
// After we have this many literals, create a tree to get updated statistical estimates
//
#define FIRST_TREE_UPDATE 1024


//
// Verifies that all of the hash pointers in the hash table are correct, and that
// the tree structure is valid.
//
#define DISABLE_VERIFY_HASHES

#ifdef _DEBUG
#ifndef DISABLE_VERIFY_HASHES
#define VERIFY_HASHES(bufpos) verifyHashes(context, bufpos)
#else
#define VERIFY_HASHES(bufpos) ;
#endif
#else
#define VERIFY_HASHES(bufpos) ;
#endif


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
    encoder->literal_tree_freq[c]++; \
	_ASSERT(encoder->recording_literal_tree_len[c] != 0); \
	OUTPUT_RECORDING_DATA(encoder->recording_literal_tree_len[c], encoder->recording_literal_tree_code[c]); \
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
	_ASSERT(context->outputting_block_num_literals >= 0 && context->outputting_block_num_literals < OPT_ENCODER_MAX_ITEMS); \
	_ASSERT(encoder->recording_literal_tree_len[item] != 0); \
	_ASSERT(encoder->recording_dist_tree_len[pos_slot] != 0); \
    context->outputting_block_num_literals++; \
    encoder->literal_tree_freq[(NUM_CHARS + 1) + len_slot]++; \
    encoder->dist_tree_freq[pos_slot]++; \
	OUTPUT_RECORDING_DATA(encoder->recording_literal_tree_len[item], encoder->recording_literal_tree_code[item]); \
	CHECK_FLUSH_RECORDING_BUFFER(); \
	if (extra_len_bits > 0) \
	{ \
		OUTPUT_RECORDING_DATA(extra_len_bits, (match_len-MIN_MATCH) & ((1 << extra_len_bits)-1)); \
		CHECK_FLUSH_RECORDING_BUFFER(); \
	} \
	OUTPUT_RECORDING_DATA(encoder->recording_dist_tree_len[pos_slot], encoder->recording_dist_tree_code[pos_slot]); \
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


static void calculateUpdatedEstimates(t_encoder_context *context);
static void OptimalEncoderMoveWindows(t_encoder_context *context);


static int match_est(t_optimal_encoder *encoder, int match_length, unsigned int match_pos)
{
	int dist_slot;
	int len_slot;

	// output match position
	len_slot = g_LengthLookup[match_length-MIN_MATCH];
	dist_slot = POS_SLOT(match_pos);

	return	encoder->literal_tree_len[NUM_CHARS + 1 + len_slot] +
			g_ExtraLengthBits[len_slot] +
			encoder->dist_tree_len[dist_slot] + 
			g_ExtraDistanceBits[dist_slot];
}


//
// Create initial estimations to output each element
//
static void initOptimalEstimates(t_encoder_context *context)
{
	int i, p;
    t_optimal_encoder *encoder = context->optimal_encoder;

	for (i = 0; i < NUM_CHARS; i++)
		encoder->literal_tree_len[i] = 8;

	p = NUM_CHARS+1;
	encoder->literal_tree_len[p] = 3;
	encoder->literal_tree_len[p+1] = 4;
	encoder->literal_tree_len[p+2] = 5;

	for (; p < MAX_LITERAL_TREE_ELEMENTS; p++)
		encoder->literal_tree_len[p] = 6;

	for (i = 0; i < MAX_DIST_TREE_ELEMENTS; i++)
		encoder->dist_tree_len[i] = (i/2)+1;
}


//
// Fix optimal estimates; if bitlen == 0 it doesn't mean that the element takes 0
// bits to output, it means that the element didn't occur, so come up with some estimate.
//
static void fixOptimalEstimates(t_encoder_context *context)
{
	int i;
    t_optimal_encoder *encoder = context->optimal_encoder;

	for (i = 0; i < NUM_CHARS; i++)
	{
		if (encoder->literal_tree_len[i] == 0)
			encoder->literal_tree_len[i] = 13;
	}

	for (i = NUM_CHARS+1; i < MAX_LITERAL_TREE_ELEMENTS; i++)
	{
		if (encoder->literal_tree_len[i] == 0)
			encoder->literal_tree_len[i] = 12;
	}

	for (i = 0; i < MAX_DIST_TREE_ELEMENTS; i++)
	{
		if (encoder->dist_tree_len[i] == 0)
			encoder->dist_tree_len[i] = 10;
	}
}


/*
 * Returns an estimation of how many bits it would take to output
 * a given character
 */
#define CHAR_EST(c) (numbits_t) (encoder->literal_tree_len[(c)])


/*
 * Returns an estimation of how many bits it would take to output
 * a given match.
 */
#define MATCH_EST(ml,mp,result) result = match_est(encoder, ml,mp);


//
// Returns whether the literal buffers are just about full
//
// Since we could output a large number of matches/chars in between these checks, we
// have to be careful.
//
// BUGBUG should check after each item output, so we don't have to be so careful; this
//        means we will utilise more of the recording buffer
//
#define LITERAL_BUFFERS_FULL() \
    (context->outputting_block_num_literals >= OPT_ENCODER_MAX_ITEMS-4-LOOK-MAX_MATCH || \
            recording_bufptr + 3*(MAX_MATCH + LOOK) >= end_recording_bufptr)


void OptimalEncoderDeflate(t_encoder_context *context)
{
	unsigned long	bufpos_end;
	unsigned long	MatchPos;
	unsigned long	i;
	int				EncMatchLength; /* must be a signed number */
	unsigned long	bufpos;
	unsigned long	recording_bitbuf;
	int				recording_bitcount;
	byte *			recording_bufptr;
    byte *          end_recording_bufptr;
    t_optimal_encoder *encoder = context->optimal_encoder;

    _ASSERT(encoder != NULL);
	_ASSERT(context->state == STATE_NORMAL);

	// reinsert the up to BREAK_LENGTH nodes we removed the last time we exit this function
	VERIFY_HASHES(context->bufpos);
	reinsertRemovedNodes(context);
	VERIFY_HASHES(context->bufpos);

	// restore literal/match bitmap variables
    end_recording_bufptr = &encoder->lit_dist_buffer[OPT_ENCODER_LIT_DIST_BUFFER_SIZE-8];
	recording_bufptr = encoder->recording_bufptr;
    recording_bitbuf = encoder->recording_bitbuf;
    recording_bitcount = encoder->recording_bitcount;

    bufpos			= context->bufpos;
	bufpos_end		= context->bufpos_end;

	/*
	 * While we haven't reached the end of the data
	 */
after_output_block:

	while (bufpos < bufpos_end)
	{
		// time to update our stats?
		if (context->outputting_block_num_literals >= encoder->next_tree_update)
		{
			encoder->next_tree_update += 1024;

            calculateUpdatedEstimates(context);
			fixOptimalEstimates(context);
		}

		// literal buffer or distance buffer filled up (or close to filling up)?
		if (LITERAL_BUFFERS_FULL())
			break;

		/*
		 * Search for matches of all different possible lengths, at bufpos
		 */
		EncMatchLength = optimal_find_match(context, bufpos); 

		if (EncMatchLength < MIN_MATCH)
		{

output_literal:
			/*
			 * No match longer than 1 character exists in the history 
			 * window, so output the character at bufpos as a symbol.
			 */
			RECORD_CHAR(encoder->window[bufpos]);
			bufpos++;
			continue;
		}

		/*
		 * Found a match.
		 *
		 * Make sure it cannot exceed the end of the buffer.
		 */
		if ((unsigned long) EncMatchLength + bufpos > bufpos_end)
		{
			EncMatchLength = bufpos_end - bufpos;    

			/*
			 * Oops, not enough for even a small match, so we 
			 * have to output a literal
			 */
			if (EncMatchLength < MIN_MATCH)
				goto output_literal;
		}

		if (EncMatchLength < FAST_DECISION_THRESHOLD)
		{
			/*
			 *  A match has been found that is between MIN_MATCH and 
			 *  FAST_DECISION_THRESHOLD bytes in length.  The following 
			 *  algorithm is the optimal encoder that will determine the 
			 *  most efficient order of matches and unmatched characters 
			 *  over a span area defined by LOOK.  
			 *
			 *  The code is essentially a shortest path determination 
			 *  algorithm.  A stream of data can be encoded in a vast number 
			 *  of different ways depending on the match lengths and offsets
			 *  chosen.  The key to good compression ratios is to chose the 
			 *  least expensive path.
			 */
			unsigned long	span;
			unsigned long	epos, bpos, NextPrevPos, MatchPos;
			t_decision_node *decision_node_ptr;
			t_decision_node *context_decision_node = encoder->decision_node;
			t_match_pos *matchpos_table = encoder->matchpos_table;
			long		iterations;

			/*
			 * Points to the end of the area covered by this match; the span
			 * will continually be extended whenever we find more matches
			 * later on.  It will stop being extended when we reach a spot
			 * where there are no matches, which is when we decide which
			 * path to take to output the matches.
			 */
			span = bufpos + EncMatchLength;

			/*
			 * The furthest position into which we will do our lookahead parsing 
			 */
			epos = bufpos + LOOK;

			/*
			 * Temporary bufpos variable
			 */
			bpos = bufpos;

			/* 
			 * Calculate the path to the next character if we output
			 * an unmatched symbol.
			 */

			/* bits required to get here */
			context_decision_node[1].numbits = CHAR_EST(encoder->window[bufpos]);
				
			/* where we came from */
			context_decision_node[1].path    = bufpos;

			/* bits required to get here */
			context_decision_node[2].numbits = CHAR_EST(encoder->window[bufpos+1]) + context_decision_node[1].numbits;
				
			/* where we came from */
			context_decision_node[2].path    = bufpos+1;

			/*
			 * For the match found, estimate the cost of encoding the match
			 * for each possible match length, shortest offset combination.
			 *
			 * The cost, path and offset is stored at bufpos + Length.  
			 */
			for (i = MIN_MATCH; i <= (unsigned long) EncMatchLength; i++)
			{
				/*
				 * Get estimation of match cost given match length = i,
				 * match position = matchpos_table[i], and store
				 * the result in numbits[i]
				 */
				MATCH_EST(i, matchpos_table[i], context_decision_node[i].numbits);

				/*
				 * Where we came from 
				 */
				context_decision_node[i].path = bufpos;

				/*
				 * Associated match position with this path
				 */
				context_decision_node[i].link = matchpos_table[i];
			}

			/*
			 * Set bit counter to zero at the start 
			 */
			context_decision_node[0].numbits = 0;

			decision_node_ptr = &context_decision_node[-(long) bpos];

			while (1)
			{
				numbits_t est, cum_numbits;

				bufpos++;
	
				/* 
				 *  Set the proper repeated offset locations depending on the
				 *  shortest path to the location prior to searching for a 
				 *  match.
				 */

				/*
				 * The following is one of the two possible break points from
				 * the inner encoding loop.  This break will exit the loop if 
				 * a point is reached that no match can incorporate; i.e. a
				 * character that does not match back to anything is a point 
				 * where all possible paths will converge and the longest one
				 * can be chosen.
				 */
				if (span == bufpos)
					break;
					
				/*
				 * Search for matches at bufpos 
				 */
				EncMatchLength = optimal_find_match(context, bufpos); 

				/* 
				 * Make sure that the match does not exceed the stop point
				 */
				if ((unsigned long) EncMatchLength + bufpos > bufpos_end)
				{
					EncMatchLength = bufpos_end - bufpos; 
					
					if (EncMatchLength < MIN_MATCH)
						EncMatchLength = 0;
				}

				/*
				 * If the match is very long or it exceeds epos (either 
				 * surpassing the LOOK area, or exceeding past the end of the
				 * input buffer), then break the loop and output the path.
				 */
				if (EncMatchLength > FAST_DECISION_THRESHOLD || 
					bufpos + (unsigned long) EncMatchLength >= epos)
				{
					MatchPos = matchpos_table[EncMatchLength];

					decision_node_ptr[bufpos+EncMatchLength].link = MatchPos;
					decision_node_ptr[bufpos+EncMatchLength].path = bufpos;

					/*
					 * Quickly insert data into the search tree without
					 * returning match positions/lengths
					 */
#ifndef INSERT_NEAR_LONG_MATCHES
					if (MatchPos == 3 && EncMatchLength > 16)
					{
						/*
						 * If we found a match 1 character away and it's
						 * length 16 or more, it's probably a string of
						 * zeroes, so don't insert that into the search
						 * engine, since doing so can slow things down
						 * significantly!
						 */
						optimal_insert(
							context,
                               bufpos + 1,
                               bufpos - WINDOW_SIZE + 2
                           );
					}
					else
#endif
					{
						for (i = 1; i < (unsigned long) EncMatchLength; i++)
							optimal_insert(
								context,
                                   bufpos + i,
                                   bufpos + i - WINDOW_SIZE + 4
                                );
					}

					bufpos += EncMatchLength;
					break;
				}


				/*
				 * The following code will extend the area spanned by the 
				 * set of matches if the current match surpasses the end of
				 * the span.  A match of length two that is far is not 
				 * accepted, since it would normally be encoded as characters,
				 * thus allowing the paths to converge.
				 */
				if (EncMatchLength >= 3)
				{
					if (span < (unsigned long) (bufpos + EncMatchLength))
					{
						long end;
						long i;

						end = min(bufpos+EncMatchLength-bpos, LOOK-1);

						/*
						 * These new positions are undefined for now, since we haven't
						 * gone there yet, so put in the costliest value
						 */
						for (i = span-bpos+1; i <= end; i++)
							context_decision_node[i].numbits = (numbits_t) -1;

						span = bufpos + EncMatchLength;
					}
				}

				/*
				 *  The following code will iterate through all combinations
				 *  of match lengths for the current match.  It will estimate
				 *  the cost of the path from the beginning of LOOK to 
				 *  bufpos and to every locations spanned by the current 
				 *  match.  If the path through bufpos with the found matches
				 *  is estimated to take fewer number of bits to encode than
				 *  the previously found match, then the path to the location
				 *  is altered.
				 *
				 *  The code relies on accurate estimation of the cost of 
				 *  encoding a character or a match.  Furthermore, it requires
				 *  a search engine that will store the smallest match offset
				 *  of each possible match length.
				 *
				 *  A match of length one is simply treated as an unmatched 
				 *  character.
				 */

				/* 
				 *  Get the estimated number of bits required to encode the 
				 *  path leading up to bufpos.
				 */
				cum_numbits = decision_node_ptr[bufpos].numbits;

				/*
				 *  Calculate the estimated cost of outputting the path through
				 *  bufpos and outputting the next character as an unmatched byte
				 */
				est = cum_numbits + CHAR_EST(encoder->window[bufpos]);

				/*
				 *  Check if it is more efficient to encode the next character
				 *  as an unmatched character rather than the previously found 
				 *  match.  If so, then update the cheapest path to bufpos + 1.
				 *
				 *  What happens if est == numbits[bufpos-bpos+1]; i.e. it
				 *  works out as well to output a character as to output a
				 *  match?  It's a tough call; however, we will push the
				 *  encoder to use matches where possible.
				 */
				if (est < decision_node_ptr[bufpos+1].numbits)
				{
					decision_node_ptr[bufpos+1].numbits = est;
					decision_node_ptr[bufpos+1].path    = bufpos;
				}

				/*
				 *	Now, iterate through the remaining match lengths and 
				 *  compare the new path to the existing.  Change the path
				 *  if it is found to be more cost effective to go through
				 *  bufpos.
				 */
				for (i = MIN_MATCH; i <= (unsigned long) EncMatchLength; i++)
				{
					MATCH_EST(i, matchpos_table[i], est);
					est += cum_numbits;

					/*
					 * If est == numbits[bufpos+i] we want to leave things
					 * alone, since this will tend to force the matches
					 * to be smaller in size, which is beneficial for most
					 * data.
					 */
					if (est < decision_node_ptr[bufpos+i].numbits)
					{
						decision_node_ptr[bufpos+i].numbits	= est;
						decision_node_ptr[bufpos+i].path	= bufpos;
						decision_node_ptr[bufpos+i].link	= matchpos_table[i];
					}
				}
			} /* continue to loop through span of matches */

			/*
			 *  Here bufpos == span, ie. a non-matchable character found.  The
			 *  following code will output the path properly.
			 */

			/*
			 *  Unfortunately the path is stored in reverse; how to get from
			 *  where we are now, to get back to where it all started.
			 *
			 *  Traverse the path back to the original starting position
			 *  of the LOOK span.  Invert the path pointers in order to be
			 *  able to traverse back to the current position from the start.
			 */

			/*
			 * Count the number of iterations we did, so when we go forwards
			 * we'll do the same amount
			 */
			iterations = 0;

			NextPrevPos = decision_node_ptr[bufpos].path;

   			do
			{
				unsigned long	PrevPos;

      			PrevPos = NextPrevPos;

   				NextPrevPos = decision_node_ptr[PrevPos].path;
   				decision_node_ptr[PrevPos].path = bufpos;

   				bufpos = PrevPos;
   				iterations++;
			} while (bufpos != bpos);

			/*
			 * Traverse from the beginning of the LOOK span to the end of 
			 * the span along the stored path, outputting matches and 
			 * characters appropriately.
			 */
			do
			{
   				if (decision_node_ptr[bufpos].path > bufpos+1)
   				{
					/*
					 * Path skips over more than 1 character; therefore it's a match
					 */
					RECORD_MATCH(
						decision_node_ptr[bufpos].path - bufpos,
						decision_node_ptr[ decision_node_ptr[bufpos].path ].link
					);

					bufpos = decision_node_ptr[bufpos].path;
				}
   				else
   				{
					/*
					 * Path goes to the next character; therefore it's a symbol
					 */
					RECORD_CHAR(encoder->window[bufpos]);
					bufpos++;
				}
			} while (--iterations != 0);
		}
		else  /* EncMatchLength >= FAST_DECISION_THRESHOLD */
		{
			/*
			 *  This code reflects a speed optimization that will always take
			 *  a match of length >= FAST_DECISION_THRESHOLD characters.
			 */

			/*
			 * The position associated with the match we found
			 */
			MatchPos = encoder->matchpos_table[EncMatchLength];

			/*
			 * Quickly insert match substrings into search tree
			 * (don't look for new matches; just insert the strings)
			 */
#ifndef INSERT_NEAR_LONG_MATCHES
			if (MatchPos == 3 && EncMatchLength > 16)
			{
				optimal_insert(
					context,
                       bufpos + 1,
                       bufpos - WINDOW_SIZE + 2 
                   );
			}
			else
#endif
			{
				for (i = 1; i < (unsigned long) EncMatchLength; i++)
					optimal_insert(
						context,
                           bufpos + i,
                           bufpos + i - WINDOW_SIZE + 1
                        );
			}

			/*
			 * Advance our position in the window
			 */
			bufpos += EncMatchLength;

			/*
			 * Output the match
			 */
			RECORD_MATCH(EncMatchLength, MatchPos);

		}  /* EncMatchLength >= FAST_DECISION_THRESHOLD */
	} /* end while ... bufpos <= bufpos_end */

	if (LITERAL_BUFFERS_FULL())
	{
		_ASSERT(context->outputting_block_num_literals <= OPT_ENCODER_MAX_ITEMS);

		// flush our recording matches bit buffer
        FLUSH_RECORDING_BITBUF();

        // BUGBUG Should check for failure result.  Luckily the only failure condition is
        // that the tree didn't fit into 500 bytes, which is basically impossible anyway.
		(void) OptimalEncoderOutputBlock(context);

		// fix estimates for optimal parser
		fixOptimalEstimates(context);

		encoder->next_tree_update = FIRST_TREE_UPDATE;

		// did we output the whole block?
		if (context->state == STATE_NORMAL)
		{
			// reset literal recording
        	recording_bufptr = encoder->recording_bufptr;
            recording_bitbuf = encoder->recording_bitbuf;
            recording_bitcount = encoder->recording_bitcount;
			goto after_output_block;
		}
	}

	// save recording state
	encoder->recording_bufptr = recording_bufptr;
    encoder->recording_bitbuf = recording_bitbuf;
    encoder->recording_bitcount = recording_bitcount;

    context->bufpos	= bufpos;

	VERIFY_HASHES(bufpos);
	removeNodes(context);
	VERIFY_HASHES(bufpos);

    if (context->bufpos == 2*WINDOW_SIZE)
        OptimalEncoderMoveWindows(context);
}


//
// Move the search windows when bufpos reaches 2*WINDOW_SIZE
//
static void OptimalEncoderMoveWindows(t_encoder_context *context)
{
	long	delta;
	int		i;
    t_optimal_encoder *encoder = context->optimal_encoder;
	t_search_node *search_tree_root = encoder->search_tree_root;
	t_search_node *left = encoder->search_left;
	t_search_node *right = encoder->search_right;

   	_ASSERT(context->bufpos == 2*WINDOW_SIZE);
 
	VERIFY_HASHES(context->bufpos);

	delta = context->bufpos - WINDOW_SIZE;

	memcpy(&encoder->window[0], &encoder->window[context->bufpos - WINDOW_SIZE], WINDOW_SIZE);

	for (i = 0; i < NUM_DIRECT_LOOKUP_TABLE_ELEMENTS; i++)
	{
		long val = ((long) search_tree_root[i]) - delta;
	
		if (val <= 0)
			search_tree_root[i] = (t_search_node) 0;
		else
			search_tree_root[i] = (t_search_node) val;

		_ASSERT(search_tree_root[i] < WINDOW_SIZE);
	}

	memcpy(&left[0], &left[context->bufpos - WINDOW_SIZE], sizeof(t_search_node)*WINDOW_SIZE);
	memcpy(&right[0], &right[context->bufpos - WINDOW_SIZE], sizeof(t_search_node)*WINDOW_SIZE);

	for (i = 0; i < WINDOW_SIZE; i++)
	{
		long val;
			
		// left
		val = ((long) left[i]) - delta;

		if (val <= 0)
			left[i] = (t_search_node) 0;
		else
			left[i] = (t_search_node) val;

		// right
		val = ((long) right[i]) - delta;

		if (val <= 0)
			right[i] = (t_search_node) 0;
		else
			right[i] = (t_search_node) val;
	}

#ifdef _DEBUG
	// force any search table references to be invalid
	memset(&encoder->window[WINDOW_SIZE], 0, WINDOW_SIZE);
#endif

	context->bufpos = WINDOW_SIZE;
	context->bufpos_end = context->bufpos;

	VERIFY_HASHES(context->bufpos);
}


//
// Calculate the frequencies of all literal and distance codes, for tree-making, then
// make the trees
//
static void calculateUpdatedEstimates(t_encoder_context *context)
{
    USHORT code[MAX_LITERAL_TREE_ELEMENTS];
    t_optimal_encoder *encoder = context->optimal_encoder;

	// create the trees, we're interested only in len[], not code[]
    // BUGBUG perf optimisation: make makeTree() not call MakeCode() in this situation
	makeTree(
		MAX_LITERAL_TREE_ELEMENTS, 
		15, 
		encoder->literal_tree_freq, 
		code,
		encoder->literal_tree_len
	);

	makeTree(
		MAX_DIST_TREE_ELEMENTS, 
		15, 
		encoder->dist_tree_freq, 
		code,
		encoder->dist_tree_len
	);
}


//
// Zero the running frequency counts
//
// Also set freq[END_OF_BLOCK_CODE] = 1
//
void OptimalEncoderZeroFrequencyCounts(t_optimal_encoder *encoder)
{
    _ASSERT(encoder != NULL);

    memset(encoder->literal_tree_freq, 0, sizeof(encoder->literal_tree_freq));
    memset(encoder->dist_tree_freq, 0, sizeof(encoder->dist_tree_freq));
    encoder->literal_tree_freq[END_OF_BLOCK_CODE] = 1;
}


void OptimalEncoderReset(t_encoder_context *context)
{
    t_optimal_encoder *encoder = context->optimal_encoder;

    _ASSERT(encoder != NULL);

	encoder->recording_bitbuf		= 0;
	encoder->recording_bitcount     = 0;
    encoder->recording_bufptr       = encoder->lit_dist_buffer;

    context->window_size            = WINDOW_SIZE;
	context->bufpos		            = context->window_size;
	context->bufpos_end             = context->bufpos;

	DeflateInitRecordingTables(
	    encoder->recording_literal_tree_len,
    	encoder->recording_literal_tree_code, 
	    encoder->recording_dist_tree_len,
    	encoder->recording_dist_tree_code
    );

	// clear the search table
	memset(
		encoder->search_tree_root,
		0, 
		sizeof(encoder->search_tree_root)
	);

	encoder->next_tree_update = FIRST_TREE_UPDATE;

	initOptimalEstimates(context);
    OptimalEncoderZeroFrequencyCounts(encoder);
}


BOOL OptimalEncoderInit(t_encoder_context *context)
{
	context->optimal_encoder = (t_optimal_encoder *) LocalAlloc(LMEM_FIXED, sizeof(t_optimal_encoder));

    if (context->optimal_encoder == NULL)
        return FALSE;

    OptimalEncoderReset(context);
	return TRUE;
}
