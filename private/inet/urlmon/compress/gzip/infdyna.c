//
// infdyna.c
//
// Decompress a dynamically compressed block
//
#include <stdio.h>
#include <crtdbg.h>
#include "inflate.h"
#include "infmacro.h"
#include "maketbl.h"


#define OUTPUT_EOF() (output_curpos >= context->end_output_buffer)

//
// This is the slow version, which worries about the input running out or the output
// running out.  The trick here is to not read any more bytes than we need to; theoretically
// the "end of block" code could be 1 bit, so we cannot always assume that it is ok to fill
// the bit buffer with 16 bits right before a table decode.
//
BOOL DecodeDynamicBlock(t_decoder_context *context, BOOL *end_of_block_code_seen) 
{
	const byte *	input_ptr;
	const byte *	end_input_buffer;
	byte *			output_curpos;
	byte *			window;
	unsigned long	bufpos;
	unsigned long	bitbuf;
	int				bitcount;
	int				length;
	long			dist_code;
	unsigned long	offset;
    t_decoder_state old_state;
    BYTE            fCanTryFastEncoder = TRUE;

	*end_of_block_code_seen = FALSE;

	//
	// Store these variables locally for speed
	//
top:
	output_curpos	= context->output_curpos;

	window = context->window;
	bufpos = context->bufpos;

	end_input_buffer = context->end_input_buffer;

	LOAD_BITBUF_VARS();

	_ASSERT(bitcount >= -16);

    old_state = context->state;
    context->state = STATE_DECODE_TOP; // reset state

	switch (old_state)
	{
		case STATE_DECODE_TOP:
			break;

		case STATE_HAVE_INITIAL_LENGTH:
			length = context->length;
			goto reenter_state_have_initial_length;

		case STATE_HAVE_FULL_LENGTH:
			length = context->length;
			goto reenter_state_have_full_length;

		case STATE_HAVE_DIST_CODE:
			length = context->length;
			dist_code = context->dist_code;
			goto reenter_state_have_dist_code;

		case STATE_INTERRUPTED_MATCH:
			length = context->length;
			offset = context->offset;
			goto reenter_state_interrupted_match;

        default:
            _ASSERT(0); // error, invalid state!
	}

	do
	{
        //
        // The first time we're at the top of this loop, check whether we can use the
        // fast encoder; we will do this if the input and output buffers are nowhere
        // near the end, which allows the fast encoder to be a little more relaxed
        // about checking for these conditions
        //
        // If we cannot enter the fast encoder when we first check, then we will not
        // be able to enter it again while we're in this function (the amount of
        // input/output available is not going to get any larger), so don't check
        // again.
        //
        if (fCanTryFastEncoder)
        {
    		if (context->output_curpos + MAX_MATCH < context->end_output_buffer &&
	    		context->input_curpos + 12 < context->end_input_buffer)
    		{
	    		SAVE_BITBUF_VARS();
		    	context->output_curpos = output_curpos;
    			context->bufpos = bufpos;

    			if (FastDecodeDynamicBlock(context, end_of_block_code_seen) == FALSE)
	    			return FALSE;

    			if (*end_of_block_code_seen)
	    			return TRUE;

    			goto top;
	    	}
            else
            {
                // don't check again
                fCanTryFastEncoder = FALSE;
            }
        }

		//
		// decode an element from the main tree
		//

		// we must have at least 1 bit available
		_ASSERT(bitcount >= -16);

		if (bitcount == -16)
		{
			if (input_ptr >= end_input_buffer)
				break;

			bitbuf |= ((*input_ptr++) << (bitcount+16)); 
			bitcount += 8; 
		}

retry_decode_literal:

		// assert that at least 1 bit is present
		_ASSERT(bitcount > -16);

		// decode an element from the literal tree
		length = context->literal_table[bitbuf & LITERAL_TABLE_MASK]; 
		
		while (length < 0) 
		{ 
			unsigned long mask = 1 << LITERAL_TABLE_BITS; 
			do 
			{ 
				length = -length; 
				if ((bitbuf & mask) == 0) 
					length = context->literal_left[length]; 
				else 
					length = context->literal_right[length]; 
				mask <<= 1; 
			} while (length < 0); 
		}

		//
		// If this code is longer than the # bits we had in the bit buffer (i.e.
		// we read only part of the code - but enough to know that it's too long),
		// read more bits and retry
		//
		if (context->literal_tree_code_length[length] > (bitcount+16))
		{
			// if we run out of bits, break
			if (input_ptr >= end_input_buffer)
				break;

			bitbuf |= ((*input_ptr++) << (bitcount+16)); 
			bitcount += 8; 
			goto retry_decode_literal;		
		}

		DUMPBITS(context->literal_tree_code_length[length]);
		_ASSERT(bitcount >= -16);

		//
		// Is it a character or a match?
		//
		if (length < 256)
		{
			// it's an unmatched symbol
			window[bufpos] = *output_curpos++ = (byte) length;
			bufpos = (bufpos + 1) & WINDOW_MASK;
		}
		else
		{
			// it's a match
			int extra_bits;

			length -= 257;

			// if value was 256, that was the end-of-block code
			if (length < 0)
			{
				*end_of_block_code_seen = TRUE;
				break;
			}


			//
			// Get match length
			//

			//
			// These matches are by far the most common case.
			//
			if (length < 8)
			{
				// no extra bits

				// match length = 3,4,5,6,7,8,9,10
				length += 3;
			}
			else
			{
				int extra_bits;

reenter_state_have_initial_length:

				extra_bits = g_ExtraLengthBits[length];

				if (extra_bits > 0)
				{
					// make sure we have this many bits in the bit buffer
					if (extra_bits > bitcount + 16)
					{
						// if we run out of bits, break
						if (input_ptr >= end_input_buffer)
						{
							context->state = STATE_HAVE_INITIAL_LENGTH;
							context->length = length;
							break;
						}

						bitbuf |= ((*input_ptr++) << (bitcount+16)); 
						bitcount += 8;
						
						// extra_length_bits will be no more than 5, so we need to read at
						// most one byte of input to satisfy this request
					}

					length = g_LengthBase[length] + (bitbuf & g_BitMask[extra_bits]);

					DUMPBITS(extra_bits);
					_ASSERT(bitcount >= -16);
				}
				else
				{
					/*
					 * we know length > 8 and extra_bits == 0, there the length must be 258
					 */
					length = 258; /* g_LengthBase[length]; */
				}
			}

			//
			// Get match distance
			//

			// decode distance code
reenter_state_have_full_length:

			// we must have at least 1 bit available
			if (bitcount == -16)
			{
				if (input_ptr >= end_input_buffer)
				{
					context->state = STATE_HAVE_FULL_LENGTH;
					context->length = length;
					break;
				}

				bitbuf |= ((*input_ptr++) << (bitcount+16)); 
				bitcount += 8; 
			}


retry_decode_distance:

			// assert that at least 1 bit is present
			_ASSERT(bitcount > -16);

			dist_code = context->distance_table[bitbuf & DISTANCE_TABLE_MASK]; 
			
			while (dist_code < 0) 
			{ 
				unsigned long mask = 1 << DISTANCE_TABLE_BITS; 
			
				do 
				{ 
					dist_code = -dist_code; 
				
					if ((bitbuf & mask) == 0) 
						dist_code = context->distance_left[dist_code]; 
					else 
						dist_code = context->distance_right[dist_code]; 
					
					mask <<= 1; 
				} while (dist_code < 0); 
			}

			//
			// If this code is longer than the # bits we had in the bit buffer (i.e.
			// we read only part of the code - but enough to know that it's too long),
			// read more bits and retry
			//
			if (context->distance_tree_code_length[dist_code] > (bitcount+16))
			{
				// if we run out of bits, break
				if (input_ptr >= end_input_buffer)
				{
					context->state = STATE_HAVE_FULL_LENGTH;
					context->length = length;
					break;
				}

				bitbuf |= ((*input_ptr++) << (bitcount+16)); 
				bitcount += 8; 

				_ASSERT(bitcount >= -16);
				goto retry_decode_distance;		
			}


			DUMPBITS(context->distance_tree_code_length[dist_code]);

			// To avoid a table lookup we note that for dist_code >= 2,
			// extra_bits = (dist_code-2) >> 1
			//
			// Old (intuitive) way of doing this:
			//    offset = distance_base_position[dist_code] + 
			//	   		   getBits(extra_distance_bits[dist_code]);

reenter_state_have_dist_code:

			_ASSERT(bitcount >= -16);

			extra_bits = (dist_code-2) >> 1;

			if (extra_bits > 0)
			{
				// make sure we have this many bits in the bit buffer
				if (extra_bits > bitcount + 16)
				{
					// if we run out of bits, break
					if (input_ptr >= end_input_buffer)
					{
						context->state = STATE_HAVE_DIST_CODE;
						context->length = length;
						context->dist_code = dist_code;
						break;
					}

					bitbuf |= ((*input_ptr++) << (bitcount+16)); 
					bitcount += 8;
						
					// extra_length_bits can be > 8, so check again
					if (extra_bits > bitcount + 16)
					{
						// if we run out of bits, break
						if (input_ptr >= end_input_buffer)
						{
							context->state = STATE_HAVE_DIST_CODE;
							context->length = length;
							context->dist_code = dist_code;
							break;
						}

						bitbuf |= ((*input_ptr++) << (bitcount+16)); 
						bitcount += 8;
					}
				}

				offset = g_DistanceBasePosition[dist_code] + (bitbuf & g_BitMask[extra_bits]); 

				DUMPBITS(extra_bits);
				_ASSERT(bitcount >= -16);
			}
			else
			{
				offset = dist_code + 1;
			}

			// copy remaining byte(s) of match
reenter_state_interrupted_match:

			do
			{
				window[bufpos] = *output_curpos++ = window[(bufpos - offset) & WINDOW_MASK];
				bufpos = (bufpos + 1) & WINDOW_MASK;

				if (--length == 0)
					break;

			} while (output_curpos < context->end_output_buffer);

			if (length > 0)
			{
				context->state = STATE_INTERRUPTED_MATCH;
				context->length = length;
				context->offset = offset;
				break;
			}
		}

		// it's "<=" because we end when we received the end-of-block code,
		// not when we fill up the output, however, this will catch cases
		// of corrupted data where there is no end-of-output code
	} while (output_curpos < context->end_output_buffer);

	_ASSERT(bitcount >= -16);

	SAVE_BITBUF_VARS();

	context->output_curpos = output_curpos;
	context->bufpos = bufpos;

	return TRUE;
}


//
// This is the fast version, which assumes that, at the top of the loop:
//
// 1. There are at least 12 bytes of input available at the top of the loop (so that we don't
// have to check input EOF several times in the middle of the loop)
//
// and
//
// 2. There are at least MAX_MATCH bytes of output available (so that we don't have to check
// for output EOF while we're copying matches)
//
// The state must also be STATE_DECODE_TOP on entering and exiting this function
//
BOOL FastDecodeDynamicBlock(t_decoder_context *context, BOOL *end_of_block_code_seen) 
{
	const byte *	input_ptr;
	const byte *	end_input_buffer;
	byte *			output_curpos;
	byte *			window;
	unsigned long	bufpos;
	unsigned long	bitbuf;
	int				bitcount;
	int				length;
	long			dist_code;
	unsigned long	offset;

	*end_of_block_code_seen = FALSE;

	//
	// Store these variables locally for speed
	//
	output_curpos	= context->output_curpos;

	window = context->window;
	bufpos = context->bufpos;

	end_input_buffer = context->end_input_buffer;

	LOAD_BITBUF_VARS();

	_ASSERT(context->state == STATE_DECODE_TOP);
	_ASSERT(input_ptr + 12 < end_input_buffer);
	_ASSERT(output_curpos + MAX_MATCH < context->end_output_buffer);

	// make sure there are at least 16 bits in the bit buffer
	while (bitcount <= 0)
	{
		bitbuf |= ((*input_ptr++) << (bitcount+16)); 
		bitcount += 8;
	}

	do
	{
		//
		// decode an element from the main tree
		//

		// decode an element from the literal tree
		length = context->literal_table[bitbuf & LITERAL_TABLE_MASK]; 
		
		while (length < 0) 
		{ 
			unsigned long mask = 1 << LITERAL_TABLE_BITS; 
			do 
			{ 
				length = -length; 
				if ((bitbuf & mask) == 0) 
					length = context->literal_left[length]; 
				else 
					length = context->literal_right[length]; 
				mask <<= 1; 
			} while (length < 0); 
		}

		DUMPBITS(context->literal_tree_code_length[length]);

		if (bitcount <= 0)
		{
			bitbuf |= ((*input_ptr++) << (bitcount+16)); 
			bitcount += 8;

			if (bitcount <= 0)
			{
				bitbuf |= ((*input_ptr++) << (bitcount+16)); 
				bitcount += 8;
			}
		}

		//
		// Is it a character or a match?
		//
		if (length < 256)
		{
			// it's an unmatched symbol
			window[bufpos] = *output_curpos++ = (byte) length;
			bufpos = (bufpos + 1) & WINDOW_MASK;
		}
		else
		{
			// it's a match
			int extra_bits;

			length -= 257;

			// if value was 256, that was the end-of-block code
			if (length < 0)
			{
				*end_of_block_code_seen = TRUE;
				break;
			}


			//
			// Get match length
			//

			//
			// These matches are by far the most common case.
			//
			if (length < 8)
			{
				// no extra bits

				// match length = 3,4,5,6,7,8,9,10
				length += 3;
			}
			else
			{
				int extra_bits;

				extra_bits = g_ExtraLengthBits[length];

				if (extra_bits > 0)
				{
					length = g_LengthBase[length] + (bitbuf & g_BitMask[extra_bits]);

					DUMPBITS(extra_bits);

					if (bitcount <= 0)
					{
						bitbuf |= ((*input_ptr++) << (bitcount+16)); 
						bitcount += 8;

						if (bitcount <= 0)
						{
							bitbuf |= ((*input_ptr++) << (bitcount+16)); 
							bitcount += 8;
						}
					}
				}
				else
				{
					/*
					 * we know length > 8 and extra_bits == 0, there the length must be 258
					 */
					length = 258; /* g_LengthBase[length]; */
				}
			}

			//
			// Get match distance
			//

			// decode distance code
			dist_code = context->distance_table[bitbuf & DISTANCE_TABLE_MASK]; 
			
			while (dist_code < 0) 
			{ 
				unsigned long mask = 1 << DISTANCE_TABLE_BITS; 
			
				do 
				{ 
					dist_code = -dist_code; 
				
					if ((bitbuf & mask) == 0) 
						dist_code = context->distance_left[dist_code]; 
					else 
						dist_code = context->distance_right[dist_code]; 
					
					mask <<= 1; 
				} while (dist_code < 0); 
			}

			DUMPBITS(context->distance_tree_code_length[dist_code]);

			if (bitcount <= 0)
			{
				bitbuf |= ((*input_ptr++) << (bitcount+16)); 
				bitcount += 8;

				if (bitcount <= 0)
				{
					bitbuf |= ((*input_ptr++) << (bitcount+16)); 
					bitcount += 8;
				}
			}


			// To avoid a table lookup we note that for dist_code >= 2,
			// extra_bits = (dist_code-2) >> 1
			//
			// Old (intuitive) way of doing this:
			//    offset = distance_base_position[dist_code] + 
			//	   		   getBits(extra_distance_bits[dist_code]);
			extra_bits = (dist_code-2) >> 1;

			if (extra_bits > 0)
			{
				offset	= g_DistanceBasePosition[dist_code] + (bitbuf & g_BitMask[extra_bits]);
                
				DUMPBITS(extra_bits);

				if (bitcount <= 0)
				{
					bitbuf |= ((*input_ptr++) << (bitcount+16)); 
					bitcount += 8;

					if (bitcount <= 0)
					{
						bitbuf |= ((*input_ptr++) << (bitcount+16)); 
						bitcount += 8;
					}
				}
			}
			else
			{
				offset = dist_code + 1;
			}

			// copy remaining byte(s) of match
			do
			{
				window[bufpos] = *output_curpos++ = window[(bufpos - offset) & WINDOW_MASK];
				bufpos = (bufpos + 1) & WINDOW_MASK;
			} while (--length != 0);
		}
	} while ((input_ptr + 12 < end_input_buffer) && (output_curpos + MAX_MATCH < context->end_output_buffer));

	// make sure there are at least 16 bits in the bit buffer
	while (bitcount <= 0)
	{
		bitbuf |= ((*input_ptr++) << (bitcount+16)); 
		bitcount += 8;
	}

	SAVE_BITBUF_VARS();

	context->output_curpos = output_curpos;
	context->bufpos = bufpos;

	return TRUE;
}
