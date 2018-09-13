//
// stdblock.c
//
// Outputting blocks
//
#include "deflate.h"
#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "maketbl.h"


//
// Decode a recorded literal
//
#define DECODE_LITERAL(slot) \
    slot = encoder->recording_literal_tree_table[read_bitbuf & REC_LITERALS_DECODING_TABLE_MASK]; \
    while (slot < 0) \
    {  \
        unsigned long mask = 1 << REC_LITERALS_DECODING_TABLE_BITS; \
        do \
        { \
            slot = -slot; \
            if ((read_bitbuf & mask) == 0) \
                slot = encoder->recording_literal_tree_left[slot]; \
            else \
                slot = encoder->recording_literal_tree_right[slot]; \
            mask <<= 1; \
        } while (slot < 0); \
    }


//
// Decode a recorded distance slot
//
#define DECODE_POS_SLOT(slot) \
    slot = encoder->recording_dist_tree_table[read_bitbuf & REC_DISTANCES_DECODING_TABLE_MASK]; \
    while (slot < 0) \
    {  \
        unsigned long mask = 1 << REC_DISTANCES_DECODING_TABLE_BITS; \
        do \
        { \
            slot = -slot; \
            if ((read_bitbuf & mask) == 0) \
                slot = encoder->recording_dist_tree_left[slot]; \
            else \
                slot = encoder->recording_dist_tree_right[slot]; \
            mask <<= 1; \
        } while (slot < 0); \
    }


//
// Remove count bits from the bit buffer
//
#define DUMP_READBUF_BITS(count) \
    read_bitbuf >>= count; \
    read_bitcount -= count;


//
// Read more bits into the read buffer if our bit buffer if we need to
//
#define CHECK_MORE_READBUF() \
    if (read_bitcount <= 0) \
    { \
        read_bitbuf |= ((*read_bufptr++) << (read_bitcount+16)); \
        read_bitcount += 8; \
        if (read_bitcount <= 0) \
        { \
            read_bitbuf |= ((*read_bufptr++) << (read_bitcount+16)); \
            read_bitcount += 8; \
        } \
    }


// output an element from the literal tree
#define OUTPUT_LITERAL(element) \
{ \
    _ASSERT(encoder->literal_tree_len[element] != 0); \
    outputBits(context, encoder->literal_tree_len[element], encoder->literal_tree_code[element]); \
}


// output an element from the distance tree
#define OUTPUT_DIST_SLOT(element) \
{ \
    _ASSERT(encoder->dist_tree_len[element] != 0); \
    outputBits(context, encoder->dist_tree_len[element], encoder->dist_tree_code[element]); \
}



//
// Output a dynamic block
//
static BOOL StdEncoderOutputDynamicBlock(t_encoder_context *context)
{
    unsigned long    read_bitbuf;
    int                read_bitcount;
    byte *            read_bufptr;
    t_std_encoder *encoder = context->std_encoder;

    if (context->state == STATE_NORMAL)
    {
        //
        // If we haven't started to output a block yet
        //
        read_bufptr     = encoder->lit_dist_buffer;
        read_bitbuf        = 0;
        read_bitcount    = -16;

        read_bitbuf |= ((*read_bufptr++) << (read_bitcount+16)); 
        read_bitcount += 8;

        read_bitbuf |= ((*read_bufptr++) << (read_bitcount+16)); 
        read_bitcount += 8;

        context->outputting_block_bitbuf        = read_bitbuf;
        context->outputting_block_bitcount        = read_bitcount;
        context->outputting_block_bufptr        = read_bufptr;

        outputBits(context, 1, 0); // "final" block flag
        outputBits(context, 2, BLOCKTYPE_DYNAMIC); 

        context->state = STATE_OUTPUTTING_TREE_STRUCTURE;
    }

    if (context->state == STATE_OUTPUTTING_TREE_STRUCTURE)
    {
        //
        // Make sure there is enough room to output the entire tree structure at once
        //
        if (context->output_curpos > context->output_endpos - MAX_TREE_DATA_SIZE)
        {
            _ASSERT(0); // not enough room to output tree structure, fatal error!
            return FALSE;
        }

        outputTreeStructure(context, encoder->literal_tree_len, encoder->dist_tree_len);

        context->state = STATE_OUTPUTTING_BLOCK;
    }

    _ASSERT(context->state == STATE_OUTPUTTING_BLOCK);

    // load state into local variables
    read_bufptr        = context->outputting_block_bufptr;
    read_bitbuf        = context->outputting_block_bitbuf;
    read_bitcount    = context->outputting_block_bitcount;

    // output literals
    while (context->outputting_block_current_literal < context->outputting_block_num_literals)
    {
        int literal;

        // break when we get near the end of our output buffer
        if (context->output_curpos >= context->output_near_end_threshold)
            break;

        DECODE_LITERAL(literal);
        DUMP_READBUF_BITS(encoder->recording_literal_tree_len[literal]);
        CHECK_MORE_READBUF();

        if (literal < NUM_CHARS)
        {
            // it's a char
            OUTPUT_LITERAL(literal);
        }
        else
        {
            // it's a match
            int len_slot, pos_slot, extra_pos_bits;

            // literal == len_slot + (NUM_CHARS+1)
            _ASSERT(literal != END_OF_BLOCK_CODE);

            OUTPUT_LITERAL(literal);

            len_slot = literal - (NUM_CHARS+1);

            //
            // extra_length_bits[len_slot] > 0 when len_slot >= 8
            // (except when length is MAX_MATCH).
            //
            if (len_slot >= 8)
            {
                int extra_bits = g_ExtraLengthBits[len_slot];

                if (extra_bits > 0)
                {
                    unsigned int extra_data = read_bitbuf & ((1 << extra_bits)-1);

                    outputBits(context, extra_bits, extra_data);
                    
                    DUMP_READBUF_BITS(extra_bits);
                    CHECK_MORE_READBUF();
                }
            }

            DECODE_POS_SLOT(pos_slot);
            DUMP_READBUF_BITS(encoder->recording_dist_tree_len[pos_slot]);
            CHECK_MORE_READBUF();

            _ASSERT(pos_slot < 30);

            OUTPUT_DIST_SLOT(pos_slot);

            extra_pos_bits = g_ExtraDistanceBits[pos_slot];

            if (extra_pos_bits > 0)
            {
                unsigned int extra_data = read_bitbuf & ((1 << extra_pos_bits)-1);

                outputBits(context, extra_pos_bits, extra_data);

                DUMP_READBUF_BITS(extra_pos_bits);
                CHECK_MORE_READBUF();
            }
        }

        context->outputting_block_current_literal++;
    }

    // did we output all of our literals without running out of output space?
    if (context->outputting_block_current_literal >= context->outputting_block_num_literals)
    {
        // output the code signifying end-of-block
        OUTPUT_LITERAL(END_OF_BLOCK_CODE);

        // reset state
        context->state = STATE_NORMAL;
    }
    else
    {
        context->outputting_block_bitbuf    = read_bitbuf;
        context->outputting_block_bitcount    = read_bitcount;
        context->outputting_block_bufptr    = read_bufptr;
        context->state                        = STATE_OUTPUTTING_BLOCK;
    }

    return TRUE;
}


//
// Output a block.  This routine will resume outputting a block that was already being
// output if state != STATE_NORMAL.
//
BOOL StdEncoderOutputBlock(t_encoder_context *context)
{
    t_std_encoder *encoder = context->std_encoder;

    //
    // The tree creation routines cannot handle this overflow
    //
    _ASSERT(context->outputting_block_num_literals < 65536);

    if (context->state == STATE_NORMAL)
    {
        //
        // Start outputting literals and distances from the beginning
        //
        context->outputting_block_current_literal = 0;
    
        //
        // Nothing to output?  Then return
        //
        if (context->outputting_block_num_literals == 0)
            return TRUE;

        // make decoding table so that we can decode recorded items
        makeTable(
            MAX_LITERAL_TREE_ELEMENTS,
            REC_LITERALS_DECODING_TABLE_BITS,
            encoder->recording_literal_tree_len,
            encoder->recording_literal_tree_table,
            encoder->recording_literal_tree_left,
            encoder->recording_literal_tree_right
        );

        makeTable(
            MAX_DIST_TREE_ELEMENTS,
            REC_DISTANCES_DECODING_TABLE_BITS,
            encoder->recording_dist_tree_len,
            encoder->recording_dist_tree_table,
            encoder->recording_dist_tree_left,
            encoder->recording_dist_tree_right
        );

//        NormaliseFrequencies(context->literal_tree_freq, context->dist_tree_freq);
//context->dist_tree_freq[30] = 0;
//context->dist_tree_freq[31] = 0;

        // now make the trees used for encoding
        makeTree(
            MAX_LITERAL_TREE_ELEMENTS, 
            15, 
            encoder->literal_tree_freq, 
            encoder->literal_tree_code,
            encoder->literal_tree_len
        );

        makeTree(
            MAX_DIST_TREE_ELEMENTS, 
            15, 
            encoder->dist_tree_freq, 
            encoder->dist_tree_code,
            encoder->dist_tree_len
        );

//GenerateTable("g_FastEncoderLiteralTree", MAX_LITERAL_TREE_ELEMENTS, context->literal_tree_len, context->literal_tree_code);
//GenerateTable("g_FastEncoderDistanceTree", MAX_DIST_TREE_ELEMENTS, context->dist_tree_len, context->dist_tree_code);
    }

    //
    // Try outputting as a dynamic block
    //
    if (StdEncoderOutputDynamicBlock(context) == FALSE)
    {
        return FALSE;
    }

    if (context->state == STATE_NORMAL)
    {
           encoder->recording_bufptr           = context->std_encoder->lit_dist_buffer;
        encoder->recording_bitbuf           = 0;
        encoder->recording_bitcount         = 0;

        context->outputting_block_num_literals = 0;

        // make sure there are no zero frequency items
        NormaliseFrequencies(encoder->literal_tree_freq, encoder->dist_tree_freq);

        // make tree for recording new items
        makeTree(
            MAX_DIST_TREE_ELEMENTS, 
            RECORDING_DIST_MAX_CODE_LEN,
            encoder->dist_tree_freq, 
            encoder->recording_dist_tree_code, 
            encoder->recording_dist_tree_len
        );

        makeTree(
            MAX_LITERAL_TREE_ELEMENTS, 
            RECORDING_LIT_MAX_CODE_LEN,
            encoder->literal_tree_freq, 
            encoder->recording_literal_tree_code, 
            encoder->recording_literal_tree_len
        );

        StdEncoderZeroFrequencyCounts(encoder);
    }

    return TRUE;
}
