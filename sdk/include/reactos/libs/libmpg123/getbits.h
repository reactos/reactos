/*
	getbits

	copyright ?-2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp

	All code is in the header to suggest/force inlining of these small often-used functions.
	This indeed has some impact on performance.
*/

#ifndef _MPG123_GETBITS_H_
#define _MPG123_GETBITS_H_

#include "mpg123lib_intern.h"
#include "debug.h"

#define backbits(fr,nob) ((void)( \
  fr->bits_avail  += nob, \
  fr->bitindex    -= nob, \
  fr->wordpointer += (fr->bitindex>>3), \
  fr->bitindex    &= 0x7 ))

#define getbitoffset(fr) ((-fr->bitindex)&0x7)
/* Precomputing the bytes to be read is error-prone, and some over-read
   is even expected for Huffman. Just play safe and return zeros in case
   of overflow. This assumes you made bitindex zero already! */
#define getbyte(fr) ( (fr)->bits_avail-=8, (fr)->bits_avail >= 0 \
  ? *((fr)->wordpointer++) \
  : 0 )

static unsigned int getbits(mpg123_handle *fr, int number_of_bits)
{
  unsigned long rval;

#ifdef DEBUG_GETBITS
fprintf(stderr,"g%d",number_of_bits);
#endif
  fr->bits_avail -= number_of_bits;
  /* Safety catch until we got the nasty code fully figured out. */
  /* No, that catch stays here, even if we think we got it figured out! */
  if(fr->bits_avail < 0)
  {
    if(NOQUIET)
      error2( "Tried to read %i bits with %li available."
      ,  number_of_bits, fr->bits_avail );
    return 0;
  }
/*  This is actually slow: if(!number_of_bits)
    return 0; */

#if 0
   check_buffer_range(number_of_bits+fr->bitindex);
#endif

  {
    rval = fr->wordpointer[0];
    rval <<= 8;
    rval |= fr->wordpointer[1];
    rval <<= 8;
    rval |= fr->wordpointer[2];

    rval <<= fr->bitindex;
    rval &= 0xffffff;

    fr->bitindex += number_of_bits;

    rval >>= (24-number_of_bits);

    fr->wordpointer += (fr->bitindex>>3);
    fr->bitindex &= 7;
  }

#ifdef DEBUG_GETBITS
fprintf(stderr,":%lx\n",rval);
#endif

  return rval;
}


#define skipbits(fr, nob) fr->ultmp = ( \
  fr->ultmp = fr->wordpointer[0], fr->ultmp <<= 8, fr->ultmp |= fr->wordpointer[1], \
  fr->ultmp <<= 8, fr->ultmp |= fr->wordpointer[2], fr->ultmp <<= fr->bitindex, \
  fr->ultmp &= 0xffffff, fr->bitindex += nob, fr->bits_avail -= nob, \
  fr->ultmp >>= (24-nob), fr->wordpointer += (fr->bitindex>>3), \
  fr->bitindex &= 7 )

#define getbits_fast(fr, nob) ( \
  fr->ultmp = (unsigned char) (fr->wordpointer[0] << fr->bitindex), \
  fr->ultmp |= ((unsigned long) fr->wordpointer[1]<<fr->bitindex)>>8, \
  fr->ultmp <<= nob, fr->ultmp >>= 8, \
  fr->bitindex += nob, fr->bits_avail -= nob, \
  fr->wordpointer += (fr->bitindex>>3), \
  fr->bitindex &= 7, fr->ultmp )

#define get1bit(fr) ( \
  fr->uctmp = *fr->wordpointer << fr->bitindex, \
  ++fr->bitindex, --fr->bits_avail, \
  fr->wordpointer += (fr->bitindex>>3), fr->bitindex &= 7, fr->uctmp>>7 )


#endif
