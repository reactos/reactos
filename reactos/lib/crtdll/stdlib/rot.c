/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/stdlib/rot.c
 * PURPOSE:     Performs a bit wise rotation
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              03/04/99: Created
 */

#include <crtdll/stdlib.h>

unsigned int _rotl( unsigned int value, int shift )
{
	int max_bits = sizeof(unsigned int)<<3;
	if ( shift < 0 )
		return _rotr(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value << shift) | (value >> (max_bits-shift));
}

unsigned int _rotr( unsigned int value, int shift )
{
	int max_bits = sizeof(unsigned int)<<3;
	if ( shift < 0 )
		return _rotl(value,-shift);

	if ( shift > max_bits<<3 )
		shift = shift % max_bits;
	return (value >> shift) | (value <<  (max_bits-shift));
}


unsigned long _lrotl( unsigned long value, int shift )
{
	int max_bits = sizeof(unsigned long)<<3;
	if ( shift < 0 )
		return _lrotr(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value << shift) | (value >> (max_bits-shift));
}

unsigned long _lrotr( unsigned long value, int shift )
{
	int max_bits = sizeof(unsigned long)<<3;
	if ( shift < 0 )
		return _lrotl(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value >> shift) | (value << (max_bits-shift));
}
