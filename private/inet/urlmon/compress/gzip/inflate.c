//
// inflate.c
//
// Decompressor
//
#include <crtdbg.h>
#include <stdio.h>
#include "inflate.h"
#include "infmacro.h"
#include "infgzip.h"
#include "maketbl.h"


//
// Local function prototypes
//
static BOOL	decodeBlock(t_decoder_context *context);
static BOOL makeTables(t_decoder_context *context);


HRESULT WINAPI Decompress(
	PVOID				void_context,
	CONST BYTE *		input, 
	LONG				input_size,
	BYTE *				output, 
	LONG				output_size,
	PLONG				input_used,
	PLONG				output_used
)
{
	t_decoder_context *context = (t_decoder_context *) void_context;

	context->input_curpos		= input;
	context->end_input_buffer	= input + input_size;
	context->output_curpos		= output;
	context->end_output_buffer	= output + output_size;
	context->output_buffer		= output;

	//
	// Keep decoding blocks until the output fills up, we read all the input, or we enter
    // the "done" state
	//
    // Note that INPUT_EOF() is not a sufficient check for determining that all the input
    // has been used; there could be an additional block stored entirely in the bit buffer.
    // For this reason, if we're in the READING_BFINAL state (start of new block) after
    // calling decodeBlock(), don't quit the loop unless there is truly no input left in
    // the bit buffer.
    //
	while ( (context->output_curpos < context->end_output_buffer) && 
            (!INPUT_EOF()) && 
            (context->state != STATE_DONE && context->state != STATE_VERIFYING_GZIP_FOOTER)
          )
	{
retry:
		if (decodeBlock(context) == FALSE)
		{
			*input_used = 0;
			*output_used = 0;
			return E_FAIL;
		}

        // No more input bytes, but am starting a new block and there's at least one bit
        // in the bit buffer
        if (context->state == STATE_READING_BFINAL && INPUT_EOF() && context->bitcount > -16)
            goto retry;
	}

	*input_used  = (long) (context->input_curpos - input);
	*output_used = (long) (context->output_curpos - output);

    if (context->using_gzip)
    {
        // Calculate the crc32 of everything we just decompressed, and then, if our state
        // is STATE_DONE, verify the crc
        if (*output_used > 0)
        {
            context->gzip_crc32 = GzipCRC32(context->gzip_crc32, output, *output_used);
            context->gzip_output_stream_size += (*output_used);
        }

        if (context->state == STATE_VERIFYING_GZIP_FOOTER)
        {
            context->state = STATE_DONE;

            // Now do our crc/input size check
            if (context->gzip_crc32 != context->gzip_footer_crc32 ||
                context->gzip_output_stream_size != context->gzip_footer_output_stream_size)
            {
               	*input_used = 0;
	            *output_used = 0;
        		return E_FAIL;
            }
        }
    }

	if (*input_used == 0 && *output_used == 0)
    {
        if (context->state == STATE_DONE)
		    return S_FALSE; // End of compressed data
        else
            return E_FAIL; // Avoid infinite loops
    }
	else
    {
		return S_OK;
    }
}


//
// Returns TRUE for success, FALSE for an error of some kind (invalid data)
//
static BOOL decodeBlock(t_decoder_context *context)
{
	BOOL eob, result;

    if (context->state == STATE_DONE || context->state == STATE_VERIFYING_GZIP_FOOTER)
        return TRUE;

    if (context->using_gzip)
    {
        if (context->state == STATE_READING_GZIP_HEADER)
        {
            if (ReadGzipHeader(context) == FALSE)
                return FALSE;

            // If we're still reading the GZIP header it means we ran out of input
            if (context->state == STATE_READING_GZIP_HEADER)
                return TRUE;
        }

        if (context->state == STATE_START_READING_GZIP_FOOTER || context->state == STATE_READING_GZIP_FOOTER)
        {
            if (ReadGzipFooter(context) == FALSE)
                return FALSE;

            // Whether we ran out of input or not, return
            return TRUE;
        }
    }

	//
	// Do we need to fill our bit buffer?
	//
	// This will happen the very first time we call Decompress(), as well as after decoding
	// an uncompressed block
	//
	if (context->state == STATE_READING_BFINAL_NEED_TO_INIT_BITBUF)
	{
		//
		// If we didn't have enough bits to init, return
		//
		if (initBitBuffer(context) == FALSE)
			return TRUE;
	}

	//
	// Need to read bfinal bit
	//
	if (context->state == STATE_READING_BFINAL)
	{
		// Need 1 bit
		if (ensureBitsContext(context, 1) == FALSE)
			return TRUE;

		context->bfinal	= getBits(context, 1);
		context->state = STATE_READING_BTYPE;
	}

	if (context->state == STATE_READING_BTYPE)
	{
		// Need 2 bits
		if (ensureBitsContext(context, 2) == FALSE)
			return TRUE;

		context->btype = getBits(context, 2);

		if (context->btype == BLOCKTYPE_DYNAMIC)
		{
			context->state = STATE_READING_NUM_LIT_CODES;
		}
		else if (context->btype == BLOCKTYPE_FIXED)
		{
			context->state = STATE_DECODE_TOP;
		}
		else if (context->btype == BLOCKTYPE_UNCOMPRESSED)
		{
			context->state = STATE_UNCOMPRESSED_ALIGNING;
		}
		else
		{
            // unsupported compression mode
			return FALSE;
		}
	}

	if (context->btype == BLOCKTYPE_DYNAMIC)
	{
		if (context->state < STATE_DECODE_TOP)
		{
			if (readDynamicBlockHeader(context) == FALSE)
				return FALSE;

			if (context->state == STATE_DECODE_TOP)
			{
				if (makeTables(context) == FALSE)
					return FALSE; // bad tables
			}
            else
            {
                return TRUE; // not enough input
            }
		}

		result = DecodeDynamicBlock(context, &eob);

		if (eob)
			context->state = STATE_READING_BFINAL;
	}
	else if (context->btype == BLOCKTYPE_FIXED)
	{
		result = DecodeStaticBlock(context, &eob);

		if (eob)
			context->state = STATE_READING_BFINAL;
	}
	else if (context->btype == BLOCKTYPE_UNCOMPRESSED)
	{
		result = decodeUncompressedBlock(context, &eob);
	}
	else
	{
		//
		// Invalid block type
		//
		return FALSE;
	}

    //
    // If we reached the end of the block and the block we were decoding had
    // bfinal=1 (final block)
    //
	if (eob && context->bfinal)
    {
        if (context->using_gzip)
    		context->state = STATE_START_READING_GZIP_FOOTER;
        else
            context->state = STATE_DONE;
    }

	return result;
}


//
// Will throw an exception if a corrupt table is detected
//
static BOOL makeTables(t_decoder_context *context) 
{
	if (makeTable(
		MAX_LITERAL_TREE_ELEMENTS,
		LITERAL_TABLE_BITS,
		context->literal_tree_code_length,
		context->literal_table,
		context->literal_left,
		context->literal_right) == FALSE)
		return FALSE;

	return makeTable(
		MAX_DIST_TREE_ELEMENTS,
		DISTANCE_TABLE_BITS,
		context->distance_tree_code_length,
		context->distance_table,
		context->distance_left,
		context->distance_right
	);
}
