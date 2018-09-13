//
// infuncmp.c
//
// Decodes uncompressed blocks
//
#include "inflate.h"
#include "infmacro.h"


//
// Returns whether there are >= n valid bits in the bit buffer
//
#define ASSERT_BITS_IN_BIT_BUFFER(n) (context->bitcount + 16 >= (n))


static int twoBytesToInt(byte a, byte b)
{
	return (((int) a) & 255) | ((((int) b) & 255) << 8);
}


static void dumpBits(t_decoder_context *context, int n)
{
	context->bitbuf >>= n; 
	context->bitcount -= n; 
}


// retrieve n bits from the bit buffer, and dump them when done
// n can be up to 16
static int getBits(t_decoder_context *context, int n)
{
	int result;

	context->bitcount -= n; 
	result = (context->bitbuf & g_BitMask[n]);
	context->bitbuf >>= n; 

	return result;
}


BOOL decodeUncompressedBlock(t_decoder_context *context, BOOL *end_of_block)
{
	unsigned int unc_len, complement;

	*end_of_block = FALSE;

	if (context->state == STATE_DECODING_UNCOMPRESSED)
	{
		unc_len = context->state_loop_counter;
	}
	else
	{
		int i;

		if (context->state == STATE_UNCOMPRESSED_ALIGNING)
		{
			// 
			// Right now we have between 0 and 32 bits in bitbuf
			//
			// However, we must flush to a byte boundary
			//
			if ((context->bitcount & 7) != 0)
			{
				int result;

				result = getBits(context, (context->bitcount & 7));

				//
				// Since this is supposed to be padding, we should read all zeroes,
				// however, it's not really specified in the spec that they have to
				// be zeroes, so don't count this as an error
				//
			}

			//
			// Now we have exactly 0, 8, 16, 24, or 32 bits in the bit buffer
			//
			context->state = STATE_UNCOMPRESSED_1;
		}

		//
		// Now we need to read 4 bytes from the input - however, some of these bytes may
		// be inside our bit buffer, so take them from there first
		//
		for (i = 0; i < 4; i++)
		{
			if (context->state == STATE_UNCOMPRESSED_1 + i)
			{
				if (ASSERT_BITS_IN_BIT_BUFFER(8))
				{
					context->unc_buffer[i] = (byte) ((context->bitbuf) & 255);
					context->bitbuf >>= 8;
					context->bitcount -= 8;
				}
				else
				{
					if (INPUT_EOF())
						return TRUE;

					context->unc_buffer[i] = *context->input_curpos++;
				}

				context->state++;
			}
		}

		unc_len = twoBytesToInt(
			context->unc_buffer[0], context->unc_buffer[1]
		);

		complement = twoBytesToInt(
			context->unc_buffer[2], context->unc_buffer[3]
		);

		// make sure complement matches
		if ((unsigned short) unc_len != (unsigned short) (~complement))
			return FALSE; // error!
	}

	// BUGBUG Make this into a memory copy loop for speed!
	while (unc_len > 0 && context->input_curpos < context->end_input_buffer && context->output_curpos < context->end_output_buffer)
	{
		unc_len--;
		*context->output_curpos++ = context->window[context->bufpos++] = *context->input_curpos++;
		context->bufpos &= WINDOW_MASK;
	}

	//
	// More bytes left to compress in this block?
	//
	if (unc_len != 0)
	{
		context->state = STATE_DECODING_UNCOMPRESSED;
		context->state_loop_counter = unc_len;
	}
	else
	{
		//
		// Done with this block, need to re-init bit buffer for next block
		//
		context->state = STATE_READING_BFINAL_NEED_TO_INIT_BITBUF;
		*end_of_block = TRUE;
	}

	return TRUE;
}
