/*
 * output.c
 *
 * General outputting routines
 */
#include "deflate.h"
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>


//
// Output an element from the pre-tree
//
#define OUTPUT_PRETREE_ELEMENT(element) \
	_ASSERT(pretree_len[element] != 0); \
	outputBits(context, pretree_len[element], pretree_code[element]);


//
// Output the tree structure for a dynamic block
//
void outputTreeStructure(t_encoder_context *context, const BYTE *literal_tree_len, const BYTE *dist_tree_len)
{
	int		hdist, hlit, combined_tree_elements, i, pass;
	USHORT	pretree_freq[NUM_PRETREE_ELEMENTS*2];
	USHORT	pretree_code[NUM_PRETREE_ELEMENTS];
	byte	pretree_len[NUM_PRETREE_ELEMENTS];

	//
	// combined literal + distance length code array for outputting the trees
	// in compressed form
	//
	// +3 is so we can overflow the array when performing run length encoding
	// (dummy values are inserted at the end so that run length encoding fails
	// before falling off the end of the array)
	//
	BYTE	lens[MAX_LITERAL_TREE_ELEMENTS + MAX_DIST_TREE_ELEMENTS + 3];

	//
	// Calculate HDIST
	//
	for (hdist = MAX_DIST_TREE_ELEMENTS - 1; hdist >= 1; hdist--)
	{
		if (dist_tree_len[hdist] != 0)
			break;
	}

	hdist++;

	//
	// Calculate HLIT
	//
	for (hlit = MAX_LITERAL_TREE_ELEMENTS - 1; hlit >= 257; hlit--)
	{
		if (literal_tree_len[hlit] != 0)
			break;
	}

	hlit++;

	//
	// Now initialise the array to have all of the hlit and hdist codes
	// in it
	//
	combined_tree_elements = hdist + hlit;

	memcpy(lens, literal_tree_len, hlit);
	memcpy(&lens[hlit], dist_tree_len, hdist);

	//
	// Stick in some dummy values at the end so that we don't overflow the 
	// array when comparing
	//
	for (i = combined_tree_elements; i < sizeof(lens); i++)
		lens[i] = -1;

	for (i = 0; i < NUM_PRETREE_ELEMENTS; i++)
		pretree_freq[i] = 0;

	//
	// Output the bitlengths in compressed (run length encoded) form.
	//
	// Make two passes; on the first pass count the various codes, create
	// the tree and output it, on the second pass output the codes using
	// the tree.
	//
	for (pass = 0; pass < 2; pass++)
	{
		int		cur_element;

		// are we outputting during this pass?
		BOOL	outputting = (pass == 1); 

		cur_element = 0;

		while (cur_element < combined_tree_elements)
		{
			int curlen = lens[cur_element];
			int run_length;

			//
			// See how many consecutive elements have the same value
			//
			// This won't run off the end of the array; it will hit the -1's
			// we stored there
			//
			for (run_length = cur_element+1; lens[run_length] == curlen; run_length++)
				;

			run_length -= cur_element;

			//
			// For non-zero codes need 4 identical in a row (original code
			// plus 3 repeats).  We decrement the run_length by one if the
			// code is not zero, since we don't count the first (original)
			// code in this case.
			//
			// For zero codes, need 3 zeroes in a row.
			//
			if (curlen != 0)
				run_length--;

			if (run_length < 3)
			{
				if (outputting)
				{
					OUTPUT_PRETREE_ELEMENT(curlen);
				}
				else
					pretree_freq[curlen]++;

				cur_element++;
			}
			else 
			{
				//
				// Elements with zero values are encoded specially
				//
				if (curlen == 0)
				{
					//
					// Do we use code 17 (3-10 repeated zeroes) or 
					// code 18 (11-138 repeated zeroes)?
					//
					if (run_length <= 10)
					{
						// code 17
						if (outputting)
						{
							OUTPUT_PRETREE_ELEMENT(17);
							outputBits(context, 3, run_length - 3);
						}
						else
						{
							pretree_freq[17]++;
						}
					}
					else
					{
						// code 18
						if (run_length > 138)
							run_length = 138;

						if (outputting)
						{
							OUTPUT_PRETREE_ELEMENT(18);
							outputBits(context, 7, run_length - 11);
						}
						else
						{
							pretree_freq[18]++;
						}
					}  

					cur_element += run_length;
				}
				else
				{
					//
					// Number of lengths actually encoded.  This may end up 
					// being less than run_length if we have a run length of
					// 7 (6 + 1 [which cannot be encoded with a code 16])
					//
					int run_length_encoded = 0;

					// curlen != 0

					// can output 3...6 repeats of a non-zero code, so split
					// longer runs into short ones (if possible)

					// remember to output the code itself first!
					if (outputting)
					{
						OUTPUT_PRETREE_ELEMENT(curlen);

						while (run_length >= 3)
						{
							int this_run = (run_length <= 6) ? run_length : 6;

							OUTPUT_PRETREE_ELEMENT(16);
							outputBits(context, 2, this_run - 3);

							run_length_encoded += this_run;
							run_length -= this_run;
						}
					}
					else
					{
						pretree_freq[curlen]++;

						while (run_length >= 3)
						{
							int this_run = (run_length <= 6) ? run_length : 6;

							pretree_freq[16]++;

							run_length_encoded += this_run;
							run_length -= this_run;
						}
					}

					// +1 for the original code itself
					cur_element += (run_length_encoded+1);
				}
			}
		}

		//
		// If this is the first pass, create the pretree from the
		// frequency data and output it, as well as the values of
		// HLIT, HDIST, HDCLEN (# pretree codes used)
		//
		if (pass == 0)
		{
			int hclen, i;

			makeTree(
				NUM_PRETREE_ELEMENTS,
				7, 
				pretree_freq, 
				pretree_code,
				pretree_len
			);

			//
			// Calculate HCLEN
			//
			for (hclen = NUM_PRETREE_ELEMENTS-1; hclen >= 4; hclen--)
			{
				if (pretree_len[ g_CodeOrder[hclen] ] != 0)
					break;
			}
			
			hclen++;

			//
			// Dynamic block header
			//
			outputBits(context, 5, hlit - 257);
			outputBits(context, 5, hdist - 1);
			outputBits(context, 4, hclen - 4);

			for (i = 0; i < hclen; i++)
			{
				outputBits(context, 3, pretree_len[g_CodeOrder[i]]);
			}
		}
	}
}


//
// bitwise i/o
//
void flushOutputBitBuffer(t_encoder_context *context)
{
	if (context->bitcount > 0)
	{
		int prev_bitcount = context->bitcount;
			
		outputBits(context, 16 - context->bitcount, 0);

		// backtrack if we have to; ZIP is byte aligned, not 16-bit word aligned
		if (prev_bitcount <= 8)
			context->output_curpos--;
	}
}


//
// Does not check for output overflow, so make sure to call checkOutputOverflow()
// often enough!
//
void outputBits(t_encoder_context *context, int n, int x)
{
	_ASSERT(context->output_curpos < context->output_endpos-1);
    _ASSERT(n > 0 && n <= 16);

	context->bitbuf |= (x << context->bitcount);
	context->bitcount += n;

	if (context->bitcount >= 16)                     
	{   
		*context->output_curpos++ = (BYTE) context->bitbuf;
		*context->output_curpos++ = (BYTE) (context->bitbuf >> 8);

		context->bitbuf >>= 16;
		context->bitcount -= 16;                         
	} 
}


// initialise the bit buffer
void InitBitBuffer(t_encoder_context *context)
{
	context->bitbuf		= 0;
	context->bitcount	= 0;
}


void OutputBlock(t_encoder_context *context)
{
    _ASSERT(context->std_encoder != NULL || context->optimal_encoder != NULL);
    
    // we never call OutputBlock() with the fast encoder
    _ASSERT(context->fast_encoder == NULL);

    if (context->std_encoder != NULL)
	    StdEncoderOutputBlock(context);
    else if (context->optimal_encoder != NULL)
        OptimalEncoderOutputBlock(context);
}


void FlushRecordingBuffer(t_encoder_context *context)
{
    _ASSERT(context->std_encoder != NULL || context->optimal_encoder != NULL);
    _ASSERT(context->fast_encoder == NULL); // fast encoder does not record

    if (context->std_encoder != NULL)
    {
        *context->std_encoder->recording_bufptr++ = (BYTE) context->std_encoder->recording_bitbuf; 
		*context->std_encoder->recording_bufptr++ = (BYTE) (context->std_encoder->recording_bitbuf >> 8); 
    }
    else if (context->optimal_encoder != NULL)
    {
        *context->optimal_encoder->recording_bufptr++ = (BYTE) context->optimal_encoder->recording_bitbuf; 
	    *context->optimal_encoder->recording_bufptr++ = (BYTE) (context->optimal_encoder->recording_bitbuf >> 8); 
    }
}
