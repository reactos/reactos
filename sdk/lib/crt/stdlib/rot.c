/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/stdlib/rot.c
 * PURPOSE:     Performs a bitwise rotation
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              03/04/99: Created
 */

#include <stdlib.h>

#ifdef __clang__
#  define _rotl __function_rotl
#  define _rotr __function_rotr
#  define _lrotl __function_lrotl
#  define _lrotr __function_lrotr
#elif defined(_MSC_VER)
#  pragma function(_rotr, _rotl, _rotr, _lrotl, _lrotr)
#endif

#if defined (__clang__) && !defined(_MSC_VER)
#  ifdef _M_IX86
unsigned int _rotr( unsigned int value, int shift ) __asm__("__rotr");
unsigned long _lrotr(unsigned long value, int shift) __asm__("__lrotr");
unsigned int _rotl( unsigned int value, int shift ) __asm__("__rotl");
unsigned long _lrotl( unsigned long value, int shift ) __asm__("__lrotl");
#  else
unsigned int _rotr( unsigned int value, int shift ) __asm__("_rotr");
unsigned long _lrotr(unsigned long value, int shift) __asm__("_lrotr");
unsigned int _rotl( unsigned int value, int shift ) __asm__("_rotl");
unsigned long _lrotl( unsigned long value, int shift ) __asm__("_lrotl");
#  endif
#else
unsigned int _rotr( unsigned int value, int shift );
unsigned long _lrotr(unsigned long value, int shift);
unsigned int _rotl( unsigned int value, int shift );
unsigned long _lrotl( unsigned long value, int shift );
#endif
/*
 * @implemented
 */
unsigned int _rotl( unsigned int value, int shift )
{
	int max_bits = sizeof(unsigned int)<<3;
	if ( shift < 0 )
		return _rotr(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value << shift) | (value >> (max_bits-shift));
}

/*
 * @implemented
 */
unsigned int _rotr( unsigned int value, int shift )
{
	int max_bits = sizeof(unsigned int)<<3;
	if ( shift < 0 )
		return _rotl(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value >> shift) | (value <<  (max_bits-shift));
}

/*
 * @implemented
 */
unsigned long _lrotl( unsigned long value, int shift )
{
	int max_bits = sizeof(unsigned long)<<3;
	if ( shift < 0 )
		return _lrotr(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value << shift) | (value >> (max_bits-shift));
}

/*
 * @implemented
 */
unsigned long _lrotr( unsigned long value, int shift )
{
	int max_bits = sizeof(unsigned long)<<3;
	if ( shift < 0 )
		return _lrotl(value,-shift);

	if ( shift > max_bits )
		shift = shift % max_bits;
	return (value >> shift) | (value << (max_bits-shift));
}
