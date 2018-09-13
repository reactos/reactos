//
// gzip.c
//
// All of the gzip-related additions to deflate (both encoder and decoder) are in this file
//

#include <string.h>
#include <stdio.h>
#include <crtdbg.h>
#include "deflate.h"
#include "inflate.h"
#include "infmacro.h"
#include "defgzip.h"
#include "infgzip.h"
#include "crc32.h"


#define GZIP_FLG_FTEXT      1
#define GZIP_FLG_CRC        2
#define GZIP_FLG_FEXTRA     4
#define GZIP_FLG_FNAME      8
#define GZIP_FLG_FCOMMENT   16


typedef enum
{
    // GZIP header
    GZIP_HDR_STATE_READING_ID1,
    GZIP_HDR_STATE_READING_ID2,
    GZIP_HDR_STATE_READING_CM,
    GZIP_HDR_STATE_READING_FLG,
    GZIP_HDR_STATE_READING_MMTIME, // iterates 4 times
    GZIP_HDR_STATE_READING_XFL,
    GZIP_HDR_STATE_READING_OS,
    GZIP_HDR_STATE_READING_XLEN1,
    GZIP_HDR_STATE_READING_XLEN2,
    GZIP_HDR_STATE_READING_XLEN_DATA,
    GZIP_HDR_STATE_READING_FILENAME,
    GZIP_HDR_STATE_READING_COMMENT,
    GZIP_HDR_STATE_READING_CRC16_PART1,
    GZIP_HDR_STATE_READING_CRC16_PART2,
    GZIP_HDR_STATE_DONE, // done reading GZIP header

    // GZIP footer
    GZIP_FTR_STATE_READING_CRC, // iterates 4 times
    GZIP_FTR_STATE_READING_FILE_SIZE // iterates 4 times
} t_gzip_state;


void EncoderInitGzipVariables(t_encoder_context *context)
{
    context->gzip_crc32 = 0;
    context->gzip_input_stream_size = 0;
    context->gzip_fOutputGzipHeader = FALSE;
}


void DecoderInitGzipVariables(t_decoder_context *context)
{
    context->gzip_crc32 = 0;
    context->gzip_output_stream_size = 0;
}


void WriteGzipHeader(t_encoder_context *context, int compression_level)
{
    BYTE *output_curpos = context->output_curpos;

    // only need 11 bytes
    _ASSERT(context->output_curpos + 16 <  context->output_endpos);

#ifndef TESTING
    // the proper code path
    *output_curpos++ = 0x1F; // ID1
    *output_curpos++ = 0x8B; // ID2
    *output_curpos++ = 8; // CM = deflate
    *output_curpos++ = 0; // FLG, no text, no crc, no extra, no name, no comment

    *output_curpos++ = 0; // MTIME (Modification Time) - no time available
    *output_curpos++ = 0;
    *output_curpos++ = 0;
    *output_curpos++ = 0;

    // XFL
    // 2 = compressor used max compression, slowest algorithm
    // 4 = compressor used fastest algorithm
    if (compression_level == 10)
        *output_curpos++ = 2; 
    else
        *output_curpos++ = 4; 

    *output_curpos++ = 0; // OS: 0 = FAT filesystem (MS-DOS, OS/2, NT/Win32)
#else /* TESTING */
    // this code is for code path testing only
    // it uses all of the headers to ensure that the decoder can handle them correctly
    *output_curpos++ = 0x1F; // ID1
    *output_curpos++ = 0x8B; // ID2
    *output_curpos++ = 8; // CM = deflate
    *output_curpos++ = (GZIP_FLG_CRC|GZIP_FLG_FEXTRA|GZIP_FLG_FNAME|GZIP_FLG_FCOMMENT); // FLG

    *output_curpos++ = 0; // MTIME (Modification Time) - no time available
    *output_curpos++ = 0;
    *output_curpos++ = 0;
    *output_curpos++ = 0;

    *output_curpos++ = 2; // XFL
    *output_curpos++ = 0; // OS: 0 = FAT filesystem (MS-DOS, OS/2, NT/Win32)
    
    // FEXTRA
    *output_curpos++ = 3; // LSB
    *output_curpos++ = 0; // MSB
    output_curpos += 3; // 3 bytes of data

    // FNAME, null terminated filename
    output_curpos += strlen(strcpy(output_curpos, "my filename"))+1;

    // FCOMMENT, null terminated comment
    output_curpos += strlen(strcpy(output_curpos, "my comment"))+1;

    // CRC16
    *output_curpos++ = 0x12;
    *output_curpos++ = 0x34;
#endif

	context->output_curpos = output_curpos;
}


void WriteGzipFooter(t_encoder_context *context)
{
    BYTE *output_curpos = context->output_curpos;

    *output_curpos++ = (BYTE) (context->gzip_crc32 & 255);
    *output_curpos++ = (BYTE) ((context->gzip_crc32 >> 8) & 255);
    *output_curpos++ = (BYTE) ((context->gzip_crc32 >> 16) & 255);
    *output_curpos++ = (BYTE) ((context->gzip_crc32 >> 24) & 255);

    *output_curpos++ = (BYTE) (context->gzip_input_stream_size & 255);
    *output_curpos++ = (BYTE) ((context->gzip_input_stream_size >> 8) & 255);
    *output_curpos++ = (BYTE) ((context->gzip_input_stream_size >> 16) & 255);
    *output_curpos++ = (BYTE) ((context->gzip_input_stream_size >> 24) & 255);

	context->output_curpos = output_curpos;
}


BOOL ReadGzipFooter(t_decoder_context *context)
{
	if (context->state == STATE_START_READING_GZIP_FOOTER)
    {
        context->state = STATE_READING_GZIP_FOOTER;
        context->gzip_footer_substate = GZIP_FTR_STATE_READING_CRC;
        context->gzip_footer_loop_counter = 0;
    }

    _ASSERT(context->state == STATE_READING_GZIP_FOOTER);

	if (INPUT_EOF())
		return TRUE;

    if (context->gzip_footer_substate == GZIP_FTR_STATE_READING_CRC)
    {
        if (context->gzip_footer_loop_counter == 0)
            context->gzip_footer_crc32 = 0;

        while (context->gzip_footer_loop_counter < 4)
        {
            context->gzip_footer_crc32 |= ((*context->input_curpos++) << (8*context->gzip_footer_loop_counter));
            context->gzip_footer_loop_counter++;

            if (INPUT_EOF())
			    break;
        }

        if (context->gzip_footer_loop_counter >= 4)
        {
            context->gzip_footer_substate = GZIP_FTR_STATE_READING_FILE_SIZE;
            context->gzip_footer_loop_counter = 0;
        }

        if (INPUT_EOF())
	        return TRUE;
    }

    if (context->gzip_footer_substate == GZIP_FTR_STATE_READING_FILE_SIZE)
    {
        if (context->gzip_footer_loop_counter == 0)
            context->gzip_footer_output_stream_size = 0;

        while (context->gzip_footer_loop_counter < 4)
        {
            context->gzip_footer_output_stream_size |= ((*context->input_curpos++) << (8*context->gzip_footer_loop_counter));
            context->gzip_footer_loop_counter++;

            if (INPUT_EOF())
                break;
        }

        if (context->gzip_footer_loop_counter >= 4)
            context->state = STATE_VERIFYING_GZIP_FOOTER;
    }

    return TRUE;
}


BOOL ReadGzipHeader(t_decoder_context *context)
{
	if (context->state != STATE_READING_GZIP_HEADER)
    {
        context->state = STATE_READING_GZIP_HEADER;
        context->gzip_header_substate = GZIP_HDR_STATE_READING_ID1;
    }

	if (INPUT_EOF())
		return TRUE;

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_ID1)
    {
        if (*context->input_curpos++ != 0x1F)
            return FALSE;

        context->gzip_header_substate = GZIP_HDR_STATE_READING_ID2;

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_ID2)
    {
        if (*context->input_curpos++ != 0x8B)
            return FALSE;

        context->gzip_header_substate = GZIP_HDR_STATE_READING_CM;

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_CM)
    {
        // compression mode must be 8 (deflate)
        if (*context->input_curpos++ != 8)
            return FALSE;

        context->gzip_header_substate = GZIP_HDR_STATE_READING_FLG;

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_FLG)
    {
        context->gzip_header_flag = *context->input_curpos++;
        context->gzip_header_substate = GZIP_HDR_STATE_READING_MMTIME;
        context->gzip_header_loop_counter = 0; // 4 MMTIME bytes

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_MMTIME)
    {
        // MTIME
        while (context->gzip_header_loop_counter < 4)
        {
            context->input_curpos++;
            context->gzip_header_loop_counter++;

            if (INPUT_EOF())
			    return TRUE;
        }

        context->gzip_header_substate = GZIP_HDR_STATE_READING_XFL;
        context->gzip_header_loop_counter = 0;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_XFL)
    {
        context->input_curpos++; // ignore XFL
        context->gzip_header_substate = GZIP_HDR_STATE_READING_OS;

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_OS)
    {
        context->input_curpos++; // ignore OS
        context->gzip_header_substate = GZIP_HDR_STATE_READING_XLEN1;

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_XLEN1)
    {
        // skip over some states if there's no "extra" data
        if ((context->gzip_header_flag & GZIP_FLG_FEXTRA) == 0)
        {
            context->gzip_header_substate = GZIP_HDR_STATE_READING_FILENAME;
            goto gzip_state_reading_fname;
        }

        context->gzip_header_xlen1_byte = *context->input_curpos++; 
        context->gzip_header_substate = GZIP_HDR_STATE_READING_XLEN2;

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_XLEN2)
    {
        BYTE xlen2 = *context->input_curpos++; 
        context->gzip_header_xlen = context->gzip_header_xlen1_byte | (xlen2 << 8);
        context->gzip_header_substate = GZIP_HDR_STATE_READING_XLEN_DATA;
        context->gzip_header_loop_counter = 0; // 0 bytes of XLEN data read so far

		if (INPUT_EOF())
			return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_XLEN_DATA)
    {
        while (context->gzip_header_loop_counter < context->gzip_header_xlen)
        {
            context->input_curpos++;
            context->gzip_header_loop_counter++;

            if (INPUT_EOF())
                break;
        }

        if (context->gzip_header_loop_counter >= context->gzip_header_xlen)
            context->gzip_header_substate = GZIP_HDR_STATE_READING_FILENAME;

        if (INPUT_EOF())
            return TRUE;
    }

gzip_state_reading_fname:

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_FILENAME)
    {
        // skip over this state if there's no filename
        if ((context->gzip_header_flag & GZIP_FLG_FNAME) == 0)
        {
            context->gzip_header_substate = GZIP_HDR_STATE_READING_COMMENT;
            goto gzip_state_reading_comment;
        }

        do
        {
            if (*context->input_curpos++ == 0)
            {
                // filename null terminator found
                context->gzip_header_substate = GZIP_HDR_STATE_READING_COMMENT;
                break;
            }
        } while (!INPUT_EOF());

        if (INPUT_EOF())
            return TRUE;
    }

gzip_state_reading_comment:

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_COMMENT)
    {
        // skip over this state if there's no filename
        if ((context->gzip_header_flag & GZIP_FLG_FCOMMENT) == 0)
        {
            context->gzip_header_substate = GZIP_HDR_STATE_READING_CRC16_PART1;
            goto gzip_state_reading_crc16;
        }

        do
        {
            if (*context->input_curpos++ == 0)
            {
                // filename null terminator found
                context->gzip_header_substate = GZIP_HDR_STATE_READING_CRC16_PART1;
                break;
            }
        } while (!INPUT_EOF());

        if (INPUT_EOF())
            return TRUE;
    }

gzip_state_reading_crc16:

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_CRC16_PART1)
    {
        // skip over these states if there's no crc16
        if ((context->gzip_header_flag & GZIP_FLG_CRC) == 0)
        {
            context->gzip_header_substate = GZIP_HDR_STATE_DONE;
            goto gzip_state_done;
        }

        context->input_curpos++; // ignore crc
        context->gzip_header_substate = GZIP_HDR_STATE_READING_CRC16_PART2;

        if (INPUT_EOF())
            return TRUE;
    }

    if (context->gzip_header_substate == GZIP_HDR_STATE_READING_CRC16_PART2)
    {
        context->input_curpos++; // ignore crc
        context->gzip_header_substate = GZIP_HDR_STATE_DONE;

        if (INPUT_EOF())
            return TRUE;
    }

gzip_state_done:

    if (context->gzip_header_substate == GZIP_HDR_STATE_DONE)
        context->state = STATE_READING_BFINAL_NEED_TO_INIT_BITBUF;

    return TRUE;
}


#define DO1(buf) crc = g_CrcTable[((ULONG)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

ULONG GzipCRC32(ULONG crc, const BYTE *buf, ULONG len)
{
    crc = crc ^ 0xffffffffUL;

    while (len >= 8)
    {
        DO8(buf);
        len -= 8;
    }

    if (len)
    {
        do
        {
          DO1(buf);
        } while (--len);
    }

    return crc ^ 0xffffffffUL;
}


//
// Works just like memcpy() except that we update context->crc32 and context->input_stream_size
// at the same time.
//
// BUGBUG Could possibly improve the perf by copying 4 or 8 bytes at a time as above
//
void GzipCRCmemcpy(t_encoder_context *context, BYTE *dest, const BYTE *src, ULONG count)
{
    ULONG crc = context->gzip_crc32 ^ 0xffffffffUL;

    context->gzip_input_stream_size += count;

    while (count-- > 0)
    {
        *dest++ = *src;
        DO1(src); // increments src
    }

    context->gzip_crc32 = crc ^ 0xffffffffUL;
}

