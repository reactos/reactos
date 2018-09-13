//
// inftree.c
//
// Reads the tree for a dynamic block
//
#include <crtdbg.h>
#include "inflate.h"
#include "infmacro.h"
#include "maketbl.h"


//
// Decode an element from the pre-tree
//
static int decodePretreeElement(t_decoder_context *context)
{
	int element;

retry:
	element = context->pretree_table[context->bitbuf & PRETREE_TABLE_MASK];

	while (element < 0)
	{
		unsigned long mask = 1 << PRETREE_TABLE_BITS;

		do
		{
			element = -element;

			if ((context->bitbuf & mask) == 0)
				element = context->pretree_left[element];
			else
				element = context->pretree_right[element];

			mask <<= 1;
		} while (element < 0);
	}

	//
	// If this code is longer than the # bits we had in the bit buffer (i.e.
	// we read only part of the code - but enough to know that it's too long),
	// return -1.
	//
	if (context->pretree_code_length[element] > (context->bitcount+16))
	{
		// if we run out of bits, return -1
		if (context->input_curpos >= context->end_input_buffer)
			return -1;

		context->bitbuf |= ((*context->input_curpos++) << (context->bitcount+16)); 
		context->bitcount += 8; 
		goto retry;
	}

	dumpBits(context, context->pretree_code_length[element]);

	return element;
}



//
// Dilemma: 
// 
// This code runs slowly because bitcount and bitbuf are accessed through the context,
// not as local variables.  However, if they were made into local variables, the code
// size would be massively increased.  Luckily the speed of this code isn't so important
// compared to that of decodeCompressedBlock().
//          
BOOL readDynamicBlockHeader(t_decoder_context *context)
{
	int		i;
	int     code;

#define NUM_CODE_LENGTH_ORDER_CODES (sizeof(g_CodeOrder)/sizeof(g_CodeOrder[0]))
    // make sure extern g_CodeOrder[] declared with array size!

	switch (context->state)
	{
		case STATE_READING_NUM_LIT_CODES:
			goto reenter_state_reading_num_lit_codes;

		case STATE_READING_NUM_DIST_CODES:
			goto reenter_state_reading_num_dist_codes;

		case STATE_READING_NUM_CODE_LENGTH_CODES:
			goto reenter_state_reading_num_code_length_codes;

		case STATE_READING_CODE_LENGTH_CODES:
		{
			i = context->state_loop_counter;
			goto reenter_state_reading_code_length_codes;
		}

		case STATE_READING_TREE_CODES_BEFORE:
		{
			i = context->state_loop_counter;
			goto reenter_state_reading_tree_codes_before;
		}

		case STATE_READING_TREE_CODES_AFTER:
		{
			i = context->state_loop_counter;
			code = context->state_code;
			goto reenter_state_reading_tree_codes_after;
		}

		default:
			return TRUE;
	}


reenter_state_reading_num_lit_codes:

	if (ensureBitsContext(context, 5) == FALSE)
	{
		context->state = STATE_READING_NUM_LIT_CODES;
		return TRUE;
	}

	context->num_literal_codes		= getBits(context, 5) + 257;



reenter_state_reading_num_dist_codes:

	if (ensureBitsContext(context, 5) == FALSE)
	{
		context->state = STATE_READING_NUM_DIST_CODES;
		return TRUE;
	}

	context->num_dist_codes			= getBits(context, 5) + 1;



reenter_state_reading_num_code_length_codes:

	if (ensureBitsContext(context, 4) == FALSE)
	{
		context->state = STATE_READING_NUM_CODE_LENGTH_CODES;
		return TRUE;
	}

	context->num_code_length_codes	= getBits(context, 4) + 4;



	for (i = 0; i < context->num_code_length_codes; i++)
	{

reenter_state_reading_code_length_codes:

		if (ensureBitsContext(context, 3) == FALSE)
		{
			context->state = STATE_READING_CODE_LENGTH_CODES;
			context->state_loop_counter = i;
			return TRUE;
		}

		context->pretree_code_length[ g_CodeOrder[i] ] = (byte) getBits(context, 3);
	}

	for (i = context->num_code_length_codes; i < NUM_CODE_LENGTH_ORDER_CODES; i++)
		context->pretree_code_length[ g_CodeOrder[i] ] = 0;

	if (makeTable(
		NUM_PRETREE_ELEMENTS,
		PRETREE_TABLE_BITS,
		context->pretree_code_length,
		context->pretree_table,
		context->pretree_left,
		context->pretree_right
	) == FALSE)
	{
		return FALSE;
	}

	context->temp_code_array_size = context->num_literal_codes + context->num_dist_codes;


	for (i = 0; i < context->temp_code_array_size; )
	{

reenter_state_reading_tree_codes_before:

		_ASSERT(context->bitcount >= -16);

		if (context->bitcount == -16)
		{
			if (context->input_curpos >= context->end_input_buffer)
            {
    			context->state = STATE_READING_TREE_CODES_BEFORE;
	    		context->state_loop_counter = i;
                return TRUE;
            }

			context->bitbuf |= ((*context->input_curpos++) << (context->bitcount+16)); 
			context->bitcount += 8; 
		}

		code = decodePretreeElement(context);

        if (code < 0)
        {
			context->state = STATE_READING_TREE_CODES_BEFORE;
			context->state_loop_counter = i;
			return TRUE;
        }

reenter_state_reading_tree_codes_after:

		if (code <= 15)
		{
			context->temp_code_list[i++] = (unsigned char) code;
		}
		else
		{
			int		repeat_count, j;

			//
			// If the code is > 15 it means there is a repeat count of 2, 3, or 7 bits
			//
			if (ensureBitsContext(context, 7) == FALSE)
			{
				context->state = STATE_READING_TREE_CODES_AFTER;
				context->state_code = (unsigned char) code;
				context->state_loop_counter = i;
				return TRUE;
			}

			if (code == 16)
			{
				byte prev_code;

				// can't have "prev code" on first code
				if (i == 0)
					return FALSE;

				prev_code = context->temp_code_list[i-1];

				repeat_count = getBits(context, 2) + 3;

				if (i + repeat_count > context->temp_code_array_size)
					return FALSE;

				for (j = 0; j < repeat_count; j++)
					context->temp_code_list[i++] = prev_code;
			}
			else if (code == 17)
			{
				repeat_count = getBits(context, 3) + 3;

				if (i + repeat_count > context->temp_code_array_size)
					return FALSE;

				for (j = 0; j < repeat_count; j++)
					context->temp_code_list[i++] = 0;
			}
			else // code == 18
			{
				repeat_count = getBits(context, 7) + 11;

				if (i + repeat_count > context->temp_code_array_size)
					return FALSE;

				for (j = 0; j < repeat_count; j++)
					context->temp_code_list[i++] = 0;
			}
		}
	}

	//
	// Create literal and distance tables
	//
	memcpy(context->literal_tree_code_length, context->temp_code_list, context->num_literal_codes);

	for (i = context->num_literal_codes; i < MAX_LITERAL_TREE_ELEMENTS; i++)
		context->literal_tree_code_length[i] = 0;

	for (i = 0; i < context->num_dist_codes; i++)
		context->distance_tree_code_length[i] = context->temp_code_list[i + context->num_literal_codes];

	for (i = context->num_dist_codes; i < MAX_DIST_TREE_ELEMENTS; i++)
		context->distance_tree_code_length[i] = 0;

	//
	// Make sure there is an end-of-block code, otherwise how could we ever end?
	//
	if (context->literal_tree_code_length[END_OF_BLOCK_CODE] == 0)
		return FALSE;

	context->state = STATE_DECODE_TOP;

	return TRUE;
}


