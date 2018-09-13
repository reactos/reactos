/*
 * deflate.c
 *
 * Main compression entrypoint for all three encoders
 */
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"
#include "fasttbl.h"
#include "defgzip.h"


typedef struct config_s
{
   int good_length; /* reduce lazy search above this match length */
   int max_lazy;    /* do not perform lazy search above this match length */
   int nice_length; /* quit search above this match length */
   int max_chain;
} compression_config;


static const compression_config configuration_table[11] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0 },  /* store only */
/* 1 */ {4,    4,  8,    4 }, /* maximum speed, no lazy matches */
/* 2 */ {4,    5, 16,    8 },
/* 3 */ {4,    6, 32,   32 },

/* 4 */ {4,    4, 16,   16 },  /* lazy matches */
/* 5 */ {8,   16, 32,   32 },
/* 6 */ {8,   16, 128, 128 },
/* 7 */ {8,   32, 128, 256 },
/* 8 */ {32, 128, 258, 1024 },
/* 9 */ {32, 258, 258, 4096 }, 
/* 10 */ {32, 258, 258, 4096 } /* maximum compression */
};


//
// Destroy the std encoder, optimal encoder, and fast encoder, but leave the 
// compressor context around
//
VOID DestroyIndividualCompressors(PVOID void_context)
{
    t_encoder_context *context = (t_encoder_context *) void_context;

    if (context->std_encoder != NULL)
    {
        LocalFree((PVOID) context->std_encoder);
        context->std_encoder = NULL;
    }

    if (context->optimal_encoder != NULL)
    {
        LocalFree((PVOID) context->optimal_encoder);
        context->optimal_encoder = NULL;
    }

    if (context->fast_encoder != NULL)
    {
        LocalFree((PVOID) context->fast_encoder);
        context->fast_encoder = NULL;
    }
}


//
// Mark the final block in the compressed data
// 
// There must be one final block with bfinal=1 indicating that it is the last one.  In the case of
// the fast encoder we just need to output the end of block code, since the fast encoder just outputs
// one very long block.
//
// In the case of the standard and optimal encoders we have already finished outputting blocks,
// so we output a new block (a static/fixed block) with bfinal=1, consisting merely of the
// end of block code.
//
static void markFinalBlock(t_encoder_context *context)
{
    if (context->fast_encoder != NULL)
    {
        // The fast encoder outputs one long block, so it just needs to terminate this block
        outputBits(
            context, 
            g_FastEncoderLiteralTreeLength[END_OF_BLOCK_CODE], 
            g_FastEncoderLiteralTreeCode[END_OF_BLOCK_CODE]
        );
    }
    else
    {
        // To finish, output a static block consisting of a single end of block code

        // Combined these three outputBits() calls (commented out) into one call
        // The total number of bits output in one shot must be <= 16, but we're ok
        // since the the length of END_OF_BLOCK_CODE is 7 for a static (fixed) block
#if 0
    	outputBits(context, 1, 1); // bfinal = 1
        outputBits(context, 2, BLOCKTYPE_FIXED);
        outputBits(context, g_StaticLiteralTreeLength[END_OF_BLOCK_CODE], g_StaticLiteralTreeCode[END_OF_BLOCK_CODE]);
#endif

        // note: g_StaticLiteralTreeCode[END_OF_BLOCK_CODE] == 0x0000
        outputBits(
            context,
            (7 + 3), // StaticLiteralTreeLength[END_OF_BLOCK_CODE]=7, + 1 bfinal bit + 2 blocktype bits
            ((0x0000) << 3) | (BLOCKTYPE_FIXED << 1) | 1
        );
    }

    // flush bits from bit buffer to output buffer
    flushOutputBitBuffer(context);

    if (context->using_gzip)
        WriteGzipFooter(context);
}


//
// Returns a pointer to the start of the window of the currently active compressor
//
// Used for memcpy'ing window data when we reach the end of the window
//
static BYTE *GetEncoderWindow(t_encoder_context *context)
{
    _ASSERT(context->std_encoder != NULL || context->optimal_encoder != NULL || context->fast_encoder != NULL);

    if (context->std_encoder != NULL)
        return context->std_encoder->window;
    else if (context->optimal_encoder != NULL)
        return context->optimal_encoder->window;
    else
        return context->fast_encoder->window;
}


//
// This function does the actual work of resetting the compression state.
// However, it does not free the std/fast/optimal encoder memory (something
// that the external ResetCompression() API currently does).
//
void InternalResetCompression(t_encoder_context *context)
{
	context->no_more_input      = FALSE;
	context->marked_final_block = FALSE;
	context->state              = STATE_NORMAL;
	context->outputting_block_num_literals = 0;

    if (context->using_gzip)
        EncoderInitGzipVariables(context);

	InitBitBuffer(context);
}


//
// The compress API
//
HRESULT WINAPI Compress(
	PVOID				void_context,
	CONST BYTE *		input_buffer,
	LONG				input_buffer_size,
	PBYTE				output_buffer,
	LONG				output_buffer_size,
	PLONG				input_used,
	PLONG				output_used,
	INT					compression_level
)
{
	int				    lazy_match_threshold;
    int                 search_depth;
    int                 good_length;
    int                 nice_length;
	t_encoder_context * context = (t_encoder_context *) void_context;
    t_std_encoder *     std_encoder;
    t_optimal_encoder * optimal_encoder;
    t_fast_encoder *    fast_encoder;
    HRESULT             result = S_OK; // default to success

    *input_used = 0;
    *output_used = 0;

    // validate compression level
	if (compression_level < 0 || compression_level > 10)
    {
        result = E_INVALIDARG;
        goto exit;
    }

	context->output_curpos				= output_buffer;
	context->output_endpos				= output_buffer + output_buffer_size;
	context->output_near_end_threshold	= output_buffer + output_buffer_size - 16;

    //
    // Have we allocated the particular compressor we want yet?
    //
    if (context->std_encoder == NULL && context->optimal_encoder == NULL && context->fast_encoder == NULL)
    {
        // No
        if (compression_level <= 3) // fast encoder
        {
    		if (FastEncoderInit(context) == FALSE)
            {
	    		result = E_OUTOFMEMORY;
                goto exit;
            }
        }
        else if (compression_level == 10) // optimal encoder
        {
    		if (OptimalEncoderInit(context) == FALSE)
            {
	    		result = E_OUTOFMEMORY;
                goto exit;
            }
        }
        else
        {
	    	if (StdEncoderInit(context) == FALSE)
            {
	    		result = E_OUTOFMEMORY;
                goto exit;
            }
        }
    }

    std_encoder     = context->std_encoder;
    optimal_encoder = context->optimal_encoder;
    fast_encoder    = context->fast_encoder;

	_ASSERT(std_encoder != NULL || optimal_encoder != NULL || fast_encoder != NULL);

	// set search depth
    if (fast_encoder != NULL)
    {
    	search_depth = configuration_table[compression_level].max_chain;
    	good_length = configuration_table[compression_level].good_length; 
    	nice_length = configuration_table[compression_level].nice_length; 
    	lazy_match_threshold = configuration_table[compression_level].max_lazy;
    }
    else if (std_encoder != NULL)
    {
    	search_depth = configuration_table[compression_level].max_chain;
    	good_length = configuration_table[compression_level].good_length; 
    	nice_length = configuration_table[compression_level].nice_length; 
    	lazy_match_threshold = configuration_table[compression_level].max_lazy;
    }

	// the output buffer must be large enough to contain an entire tree
	if (output_buffer_size < MAX_TREE_DATA_SIZE)
	{
        result = E_INVALIDARG;
        goto exit;
	}

    if (context->using_gzip && context->gzip_fOutputGzipHeader == FALSE)
    {
        // Write the GZIP header
        WriteGzipHeader(context, compression_level);
        context->gzip_fOutputGzipHeader = TRUE;
    }

	//
	// Check if previously we were in the middle of outputting a block
	//
	if (context->state != STATE_NORMAL)
	{
        // The fast encoder is a special case; it doesn't use OutputBlock()
        if (fast_encoder != NULL)
            goto start_encoding;

        // yes we were, so continue outputting it
        OutputBlock(context);

		//
		// Check if we're still outputting a block (it may be a long block that
		// has filled up the output buffer again)
		//
        // If we're coming close to the end of the buffer, and may not have enough space to
        // output a full tree structure, stop now.
        //
		if (context->state != STATE_NORMAL || 
            context->output_endpos - context->output_curpos < MAX_TREE_DATA_SIZE)
		{
			*output_used = (long) (context->output_curpos - output_buffer);
            goto set_output_used_then_exit; // success
		}

		//
		// We finished outputting the previous block, so time to compress some more input 
		//
	}

#ifdef _DEBUG
    // Fast encoder doesn't use outputBlock, so it doesn't have the tree limitation
    if (fast_encoder == NULL)
        _ASSERTE(context->output_endpos - context->output_curpos >= MAX_TREE_DATA_SIZE);
#endif

	//
	// input_buffer_size == 0 means "this is the final block"
	//
	// Of course, the client may still need to call Compress() many more times if the output 
	// buffer is small and there is a big block waiting to be sent.
	//
	// We may even have some pending input data in our buffer waiting to be compressed.
	//
	if ((input_buffer_size == 0 || context->no_more_input) && context->bufpos >= context->bufpos_end)
	{
		// if we're ever passed zero bytes of input, it means that there will never be any
		// more input
		context->no_more_input = TRUE;

		// output existing block
        // this never happens for the fast encoder, since we don't record blocks
   		if (context->outputting_block_num_literals != 0)
        {
            FlushRecordingBuffer(context);
            OutputBlock(context);

	    	//
    		// Still outputting a block?
   			//
	    	if (context->state != STATE_NORMAL)
                goto set_output_used_then_exit; // success
        }

        // for the fast encoder only, we won't have output our fast encoder preamble if the
        // file size == 0, so output it now if we haven't already.
        if (fast_encoder != NULL)
        {
            if (fast_encoder->fOutputBlockHeader == FALSE)
            {
                fast_encoder->fOutputBlockHeader = TRUE;
                FastEncoderOutputPreamble(context);
            }
        }

		// if we've already marked the final block, don't do it again
		if (context->marked_final_block)
		{
            result = S_FALSE;
            goto set_output_used_then_exit; // should be zero output used
		}

		// ensure there is enough space to output the final block (max 8 bytes)
		if (context->output_curpos + 8 >= context->output_endpos)
            goto set_output_used_then_exit; // not enough space - do it next time

		// output the final block (of length zero - we just want the bfinal=1 marker)
		markFinalBlock(context);
		context->marked_final_block = TRUE;

        result = S_FALSE;
        goto set_output_used_then_exit;
	}

	// while there is more input data (passed in as parameters) or existing data in
	// the window to compress
start_encoding:
	while ((input_buffer_size > 0) || (context->bufpos < context->bufpos_end))
	{
		long amount_to_compress;
		long window_space_available;

		_ASSERT(context->bufpos >= context->window_size && context->bufpos < (2*context->window_size));

#ifdef _DEBUG
        // Fast encoder doesn't use outputBlock, so it doesn't have the tree limitation
        if (fast_encoder == NULL)
            _ASSERTE(context->output_endpos - context->output_curpos >= MAX_TREE_DATA_SIZE);
#endif

		// read more input data into the window if there is space available
		window_space_available = (2*context->window_size) - context->bufpos_end;

		amount_to_compress = (input_buffer_size < window_space_available) ? input_buffer_size : window_space_available;

		if (amount_to_compress > 0)
		{
			*input_used += amount_to_compress;

			// copy data into history window
            if (context->using_gzip)
            {
                // In addition to copying data into the history window, GZIP wants a crc32 of the input data.
                // We will do both of these things at the same time for the purposes of data locality,
                // performance etc.
                GzipCRCmemcpy(context, GetEncoderWindow(context) + context->bufpos_end, input_buffer, amount_to_compress);
            }
            else
            {
                // Copy data into history window
    		    memcpy(GetEncoderWindow(context) + context->bufpos_end, input_buffer, amount_to_compress);
            }

			input_buffer		+= amount_to_compress;
			input_buffer_size	-= amount_to_compress;

			// last input location
			context->bufpos_end += amount_to_compress;
		}

		if (optimal_encoder != NULL)
			OptimalEncoderDeflate(context);
		else if (std_encoder != NULL)
			StdEncoderDeflate(context, search_depth, lazy_match_threshold, good_length, nice_length);
        else if (fast_encoder != NULL)
			FastEncoderDeflate(context, search_depth, lazy_match_threshold, good_length, nice_length);

		// either we reached the end of the buffer, or we had to output a block and ran out
		// of output space midway
		_ASSERT(context->bufpos == context->bufpos_end || context->state != STATE_NORMAL);

		// if we ran out of output space, break now
		if (context->state != STATE_NORMAL)
			break;

        // another check for running out of output space
        if (fast_encoder == NULL && context->output_endpos - context->output_curpos >= MAX_TREE_DATA_SIZE)
            break;

	} /* end ... while (input_buffer_size > 0) */

set_output_used_then_exit:
	*output_used = (long) (context->output_curpos - output_buffer);

exit:
    _ASSERT(*output_used < output_buffer_size); // make sure we didn't overflow the output buffer
	_ASSERT(context->bufpos >= context->window_size && context->bufpos <= 2*context->window_size); // make sure bufpos is sane

    return result;
}
