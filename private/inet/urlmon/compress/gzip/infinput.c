//
// infinput.c
//
// Bitwise inputting for inflate (decompressor)
//
#include <stdio.h>
#include <crtdbg.h>
#include "inflate.h"
#include "infmacro.h"


void dumpBits(t_decoder_context *context, int n)
{
	context->bitbuf >>= n; 
	context->bitcount -= n; 
}


// retrieve n bits from the bit buffer, and dump them when done
// n can be up to 16
int getBits(t_decoder_context *context, int n)
{
	int result;

	context->bitcount -= n; 
	result = (context->bitbuf & g_BitMask[n]);
	context->bitbuf >>= n; 

	return result;
}


//
// Ensure that <num_bits> bits are in the bit buffer
//
// Returns FALSE if there are not and there was insufficient input to make this true
//
BOOL ensureBitsContext(t_decoder_context *context, int num_bits)
{
	if (context->bitcount + 16 < num_bits) 
	{ 
		if (INPUT_EOF())
			return FALSE;

		context->bitbuf |= ((*context->input_curpos++) << (context->bitcount+16)); 
		context->bitcount += 8; 
		
		if (context->bitcount + 16 < num_bits)
		{
			if (INPUT_EOF())
				return FALSE;

			context->bitbuf |= ((*context->input_curpos++) << (context->bitcount+16)); 
			context->bitcount += 8; 
		} 
	} 

	return TRUE;
}


// initialise the bit buffer
BOOL initBitBuffer(t_decoder_context *context) 
{
	if (context->input_curpos < context->end_input_buffer)
	{
		context->bitbuf = *context->input_curpos++;
		context->bitcount = -8;
		context->state = STATE_READING_BFINAL;
		return TRUE;
	}
	else
	{
		context->bitcount = -16;
		context->bitbuf = 0;
		return FALSE;
	}
}
