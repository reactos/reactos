/******************************************************************************
 *
 * Module Name: cmclib - Local implementation of C library functions
 * $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "acpi.h"
#include "acevents.h"
#include "achware.h"
#include "acnamesp.h"
#include "acinterp.h"
#include "amlcode.h"

/*
 * These implementations of standard C Library routines can optionally be
 * used if a C library is not available.  In general, they are less efficient
 * than an inline or assembly implementation
 */

#define _COMPONENT          MISCELLANEOUS
	 MODULE_NAME         ("cmclib")


#ifndef ACPI_USE_SYSTEM_CLIBRARY

/*******************************************************************************
 *
 * FUNCTION:    strlen
 *
 * PARAMETERS:  String              - Null terminated string
 *
 * RETURN:      Length
 *
 * DESCRIPTION: Returns the length of the input string
 *
 ******************************************************************************/


u32
acpi_cm_strlen (
	const NATIVE_CHAR       *string)
{
	u32                     length = 0;


	/* Count the string until a null is encountered */

	while (*string) {
		length++;
		string++;
	}

	return (length);
}


/*******************************************************************************
 *
 * FUNCTION:    strcpy
 *
 * PARAMETERS:  Dst_string      - Target of the copy
 *              Src_string      - The source string to copy
 *
 * RETURN:      Dst_string
 *
 * DESCRIPTION: Copy a null terminated string
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_cm_strcpy (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string)
{
	NATIVE_CHAR             *string = dst_string;


	/* Move bytes brute force */

	while (*src_string) {
		*string = *src_string;

		string++;
		src_string++;
	}

	/* Null terminate */

	*string = 0;

	return (dst_string);
}


/*******************************************************************************
 *
 * FUNCTION:    strncpy
 *
 * PARAMETERS:  Dst_string      - Target of the copy
 *              Src_string      - The source string to copy
 *              Count           - Maximum # of bytes to copy
 *
 * RETURN:      Dst_string
 *
 * DESCRIPTION: Copy a null terminated string, with a maximum length
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_cm_strncpy (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string,
	NATIVE_UINT             count)
{
	NATIVE_CHAR             *string = dst_string;


	/* Copy the string */

	for (string = dst_string;
		count && (count--, (*string++ = *src_string++)); ) {;}

	/* Pad with nulls if necessary */

	while (count--) {
		*string = 0;
		string++;
	}

	/* Return original pointer */

	return (dst_string);
}


/*******************************************************************************
 *
 * FUNCTION:    strcmp
 *
 * PARAMETERS:  String1         - First string
 *              String2         - Second string
 *
 * RETURN:      Index where strings mismatched, or 0 if strings matched
 *
 * DESCRIPTION: Compare two null terminated strings
 *
 ******************************************************************************/

u32
acpi_cm_strcmp (
	const NATIVE_CHAR       *string1,
	const NATIVE_CHAR       *string2)
{


	for ( ; (*string1 == *string2); string2++) {
		if (!*string1++) {
			return (0);
		}
	}


	return ((unsigned char) *string1 - (unsigned char) *string2);
}


/*******************************************************************************
 *
 * FUNCTION:    strncmp
 *
 * PARAMETERS:  String1         - First string
 *              String2         - Second string
 *              Count           - Maximum # of bytes to compare
 *
 * RETURN:      Index where strings mismatched, or 0 if strings matched
 *
 * DESCRIPTION: Compare two null terminated strings, with a maximum length
 *
 ******************************************************************************/

u32
acpi_cm_strncmp (
	const NATIVE_CHAR       *string1,
	const NATIVE_CHAR       *string2,
	NATIVE_UINT             count)
{


	for ( ; count-- && (*string1 == *string2); string2++) {
		if (!*string1++) {
			return (0);
		}
	}

	return ((count == -1) ? 0 : ((unsigned char) *string1 -
		(unsigned char) *string2));
}


/*******************************************************************************
 *
 * FUNCTION:    Strcat
 *
 * PARAMETERS:  Dst_string      - Target of the copy
 *              Src_string      - The source string to copy
 *
 * RETURN:      Dst_string
 *
 * DESCRIPTION: Append a null terminated string to a null terminated string
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_cm_strcat (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string)
{
	NATIVE_CHAR             *string;


	/* Find end of the destination string */

	for (string = dst_string; *string++; ) { ; }

	/* Concatinate the string */

	for (--string; (*string++ = *src_string++); ) { ; }

	return (dst_string);
}


/*******************************************************************************
 *
 * FUNCTION:    strncat
 *
 * PARAMETERS:  Dst_string      - Target of the copy
 *              Src_string      - The source string to copy
 *              Count           - Maximum # of bytes to copy
 *
 * RETURN:      Dst_string
 *
 * DESCRIPTION: Append a null terminated string to a null terminated string,
 *              with a maximum count.
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_cm_strncat (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string,
	NATIVE_UINT             count)
{
	NATIVE_CHAR             *string;


	if (count) {
		/* Find end of the destination string */

		for (string = dst_string; *string++; ) { ; }

		/* Concatinate the string */

		for (--string; (*string++ = *src_string++) && --count; ) { ; }

		/* Null terminate if necessary */

		if (!count) {
			*string = 0;
		}
	}

	return (dst_string);
}


/*******************************************************************************
 *
 * FUNCTION:    memcpy
 *
 * PARAMETERS:  Dest        - Target of the copy
 *              Src         - Source buffer to copy
 *              Count       - Number of bytes to copy
 *
 * RETURN:      Dest
 *
 * DESCRIPTION: Copy arbitrary bytes of memory
 *
 ******************************************************************************/

void *
acpi_cm_memcpy (
	void                    *dest,
	const void              *src,
	NATIVE_UINT             count)
{
	NATIVE_CHAR             *new = (NATIVE_CHAR *) dest;
	NATIVE_CHAR             *old = (NATIVE_CHAR *) src;


	while (count) {
		*new = *old;
		new++;
		old++;
		count--;
	}

	return (dest);
}


/*******************************************************************************
 *
 * FUNCTION:    memset
 *
 * PARAMETERS:  Dest        - Buffer to set
 *              Value       - Value to set each byte of memory
 *              Count       - Number of bytes to set
 *
 * RETURN:      Dest
 *
 * DESCRIPTION: Initialize a buffer to a known value.
 *
 ******************************************************************************/

void *
acpi_cm_memset (
	void                    *dest,
	NATIVE_UINT             value,
	NATIVE_UINT             count)
{
	NATIVE_CHAR             *new = (NATIVE_CHAR *) dest;


	while (count) {
		*new = (char) value;
		new++;
		count--;
	}

	return (dest);
}


#define NEGATIVE    1
#define POSITIVE    0


#define _ACPI_XA     0x00    /* extra alphabetic - not supported */
#define _ACPI_XS     0x40    /* extra space */
#define _ACPI_BB     0x00    /* BEL, BS, etc. - not supported */
#define _ACPI_CN     0x20    /* CR, FF, HT, NL, VT */
#define _ACPI_DI     0x04    /* '0'-'9' */
#define _ACPI_LO     0x02    /* 'a'-'z' */
#define _ACPI_PU     0x10    /* punctuation */
#define _ACPI_SP     0x08    /* space */
#define _ACPI_UP     0x01    /* 'A'-'Z' */
#define _ACPI_XD     0x80    /* '0'-'9', 'A'-'F', 'a'-'f' */

static const u8 _acpi_ctype[257] = {
	_ACPI_CN,            /* 0x0      0.     */
	_ACPI_CN,            /* 0x1      1.     */
	_ACPI_CN,            /* 0x2      2.     */
	_ACPI_CN,            /* 0x3      3.     */
	_ACPI_CN,            /* 0x4      4.     */
	_ACPI_CN,            /* 0x5      5.     */
	_ACPI_CN,            /* 0x6      6.     */
	_ACPI_CN,            /* 0x7      7.     */
	_ACPI_CN,            /* 0x8      8.     */
	_ACPI_CN|_ACPI_SP,   /* 0x9      9.     */
	_ACPI_CN|_ACPI_SP,   /* 0xA     10.     */
	_ACPI_CN|_ACPI_SP,   /* 0xB     11.     */
	_ACPI_CN|_ACPI_SP,   /* 0xC     12.     */
	_ACPI_CN|_ACPI_SP,   /* 0xD     13.     */
	_ACPI_CN,            /* 0xE     14.     */
	_ACPI_CN,            /* 0xF     15.     */
	_ACPI_CN,            /* 0x10    16.     */
	_ACPI_CN,            /* 0x11    17.     */
	_ACPI_CN,            /* 0x12    18.     */
	_ACPI_CN,            /* 0x13    19.     */
	_ACPI_CN,            /* 0x14    20.     */
	_ACPI_CN,            /* 0x15    21.     */
	_ACPI_CN,            /* 0x16    22.     */
	_ACPI_CN,            /* 0x17    23.     */
	_ACPI_CN,            /* 0x18    24.     */
	_ACPI_CN,            /* 0x19    25.     */
	_ACPI_CN,            /* 0x1A    26.     */
	_ACPI_CN,            /* 0x1B    27.     */
	_ACPI_CN,            /* 0x1C    28.     */
	_ACPI_CN,            /* 0x1D    29.     */
	_ACPI_CN,            /* 0x1E    30.     */
	_ACPI_CN,            /* 0x1F    31.     */
	_ACPI_XS|_ACPI_SP,   /* 0x20    32. ' ' */
	_ACPI_PU,            /* 0x21    33. '!' */
	_ACPI_PU,            /* 0x22    34. '"' */
	_ACPI_PU,            /* 0x23    35. '#' */
	_ACPI_PU,            /* 0x24    36. '$' */
	_ACPI_PU,            /* 0x25    37. '%' */
	_ACPI_PU,            /* 0x26    38. '&' */
	_ACPI_PU,            /* 0x27    39. ''' */
	_ACPI_PU,            /* 0x28    40. '(' */
	_ACPI_PU,            /* 0x29    41. ')' */
	_ACPI_PU,            /* 0x2A    42. '*' */
	_ACPI_PU,            /* 0x2B    43. '+' */
	_ACPI_PU,            /* 0x2C    44. ',' */
	_ACPI_PU,            /* 0x2D    45. '-' */
	_ACPI_PU,            /* 0x2E    46. '.' */
	_ACPI_PU,            /* 0x2F    47. '/' */
	_ACPI_XD|_ACPI_DI,   /* 0x30    48. '0' */
	_ACPI_XD|_ACPI_DI,   /* 0x31    49. '1' */
	_ACPI_XD|_ACPI_DI,   /* 0x32    50. '2' */
	_ACPI_XD|_ACPI_DI,   /* 0x33    51. '3' */
	_ACPI_XD|_ACPI_DI,   /* 0x34    52. '4' */
	_ACPI_XD|_ACPI_DI,   /* 0x35    53. '5' */
	_ACPI_XD|_ACPI_DI,   /* 0x36    54. '6' */
	_ACPI_XD|_ACPI_DI,   /* 0x37    55. '7' */
	_ACPI_XD|_ACPI_DI,   /* 0x38    56. '8' */
	_ACPI_XD|_ACPI_DI,   /* 0x39    57. '9' */
	_ACPI_PU,            /* 0x3A    58. ':' */
	_ACPI_PU,            /* 0x3B    59. ';' */
	_ACPI_PU,            /* 0x3C    60. '<' */
	_ACPI_PU,            /* 0x3D    61. '=' */
	_ACPI_PU,            /* 0x3E    62. '>' */
	_ACPI_PU,            /* 0x3F    63. '?' */
	_ACPI_PU,            /* 0x40    64. '@' */
	_ACPI_XD|_ACPI_UP,   /* 0x41    65. 'A' */
	_ACPI_XD|_ACPI_UP,   /* 0x42    66. 'B' */
	_ACPI_XD|_ACPI_UP,   /* 0x43    67. 'C' */
	_ACPI_XD|_ACPI_UP,   /* 0x44    68. 'D' */
	_ACPI_XD|_ACPI_UP,   /* 0x45    69. 'E' */
	_ACPI_XD|_ACPI_UP,   /* 0x46    70. 'F' */
	_ACPI_UP,            /* 0x47    71. 'G' */
	_ACPI_UP,            /* 0x48    72. 'H' */
	_ACPI_UP,            /* 0x49    73. 'I' */
	_ACPI_UP,            /* 0x4A    74. 'J' */
	_ACPI_UP,            /* 0x4B    75. 'K' */
	_ACPI_UP,            /* 0x4C    76. 'L' */
	_ACPI_UP,            /* 0x4D    77. 'M' */
	_ACPI_UP,            /* 0x4E    78. 'N' */
	_ACPI_UP,            /* 0x4F    79. 'O' */
	_ACPI_UP,            /* 0x50    80. 'P' */
	_ACPI_UP,            /* 0x51    81. 'Q' */
	_ACPI_UP,            /* 0x52    82. 'R' */
	_ACPI_UP,            /* 0x53    83. 'S' */
	_ACPI_UP,            /* 0x54    84. 'T' */
	_ACPI_UP,            /* 0x55    85. 'U' */
	_ACPI_UP,            /* 0x56    86. 'V' */
	_ACPI_UP,            /* 0x57    87. 'W' */
	_ACPI_UP,            /* 0x58    88. 'X' */
	_ACPI_UP,            /* 0x59    89. 'Y' */
	_ACPI_UP,            /* 0x5A    90. 'Z' */
	_ACPI_PU,            /* 0x5B    91. '[' */
	_ACPI_PU,            /* 0x5C    92. '\' */
	_ACPI_PU,            /* 0x5D    93. ']' */
	_ACPI_PU,            /* 0x5E    94. '^' */
	_ACPI_PU,            /* 0x5F    95. '_' */
	_ACPI_PU,            /* 0x60    96. '`' */
	_ACPI_XD|_ACPI_LO,   /* 0x61    97. 'a' */
	_ACPI_XD|_ACPI_LO,   /* 0x62    98. 'b' */
	_ACPI_XD|_ACPI_LO,   /* 0x63    99. 'c' */
	_ACPI_XD|_ACPI_LO,   /* 0x64   100. 'd' */
	_ACPI_XD|_ACPI_LO,   /* 0x65   101. 'e' */
	_ACPI_XD|_ACPI_LO,   /* 0x66   102. 'f' */
	_ACPI_LO,            /* 0x67   103. 'g' */
	_ACPI_LO,            /* 0x68   104. 'h' */
	_ACPI_LO,            /* 0x69   105. 'i' */
	_ACPI_LO,            /* 0x6A   106. 'j' */
	_ACPI_LO,            /* 0x6B   107. 'k' */
	_ACPI_LO,            /* 0x6C   108. 'l' */
	_ACPI_LO,            /* 0x6D   109. 'm' */
	_ACPI_LO,            /* 0x6E   110. 'n' */
	_ACPI_LO,            /* 0x6F   111. 'o' */
	_ACPI_LO,            /* 0x70   112. 'p' */
	_ACPI_LO,            /* 0x71   113. 'q' */
	_ACPI_LO,            /* 0x72   114. 'r' */
	_ACPI_LO,            /* 0x73   115. 's' */
	_ACPI_LO,            /* 0x74   116. 't' */
	_ACPI_LO,            /* 0x75   117. 'u' */
	_ACPI_LO,            /* 0x76   118. 'v' */
	_ACPI_LO,            /* 0x77   119. 'w' */
	_ACPI_LO,            /* 0x78   120. 'x' */
	_ACPI_LO,            /* 0x79   121. 'y' */
	_ACPI_LO,            /* 0x7A   122. 'z' */
	_ACPI_PU,            /* 0x7B   123. '{' */
	_ACPI_PU,            /* 0x7C   124. '|' */
	_ACPI_PU,            /* 0x7D   125. '}' */
	_ACPI_PU,            /* 0x7E   126. '~' */
	_ACPI_CN,            /* 0x7F   127.     */

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0x80 to 0x8F    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0x90 to 0x9F    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0xA0 to 0xAF    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0xB0 to 0xBF    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0xC0 to 0xCF    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0xD0 to 0xDF    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  /* 0xE0 to 0xEF    */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 /* 0xF0 to 0x100   */
};

#define IS_UPPER(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_UP))
#define IS_LOWER(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_LO))
#define IS_DIGIT(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_DI))
#define IS_SPACE(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_SP))
#define IS_XDIGIT(c) (_acpi_ctype[(unsigned char)(c)] & (_ACPI_XD))


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_to_upper
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Convert character to uppercase
 *
 ******************************************************************************/

u32
acpi_cm_to_upper (
	u32                     c)
{

	return (IS_LOWER(c) ? ((c)-0x20) : (c));
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_to_lower
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Convert character to lowercase
 *
 ******************************************************************************/

u32
acpi_cm_to_lower (
	u32                     c)
{

	return (IS_UPPER(c) ? ((c)+0x20) : (c));
}


/*******************************************************************************
 *
 * FUNCTION:    strupr
 *
 * PARAMETERS:  Src_string      - The source string to convert to
 *
 * RETURN:      Src_string
 *
 * DESCRIPTION: Convert string to uppercase
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_cm_strupr (
	NATIVE_CHAR             *src_string)
{
	NATIVE_CHAR             *string;


	/* Walk entire string, uppercasing the letters */

	for (string = src_string; *string; ) {
		*string = (char) acpi_cm_to_upper (*string);
		string++;
	}


	return (src_string);
}


/*******************************************************************************
 *
 * FUNCTION:    strstr
 *
 * PARAMETERS:  String1       -
 *              String2
 *
 * RETURN:
 *
 * DESCRIPTION: Checks if String2 occurs in String1. This is not really a
 *              full implementation of strstr, only sufficient for command
 *              matching
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_cm_strstr (
	NATIVE_CHAR             *string1,
	NATIVE_CHAR             *string2)
{
	NATIVE_CHAR             *string;


	if (acpi_cm_strlen (string2) > acpi_cm_strlen (string1)) {
		return (NULL);
	}

	/* Walk entire string, comparing the letters */

	for (string = string1; *string2; ) {
		if (*string2 != *string) {
			return (NULL);
		}

		string2++;
		string++;
	}


	return (string1);
}


/*******************************************************************************
 *
 * FUNCTION:    strtoul
 *
 * PARAMETERS:  String          - Null terminated string
 *              Terminater      - Where a pointer to the terminating byte is returned
 *              Base            - Radix of the string
 *
 * RETURN:      Converted value
 *
 * DESCRIPTION: Convert a string into an unsigned value.
 *
 ******************************************************************************/

NATIVE_UINT
acpi_cm_strtoul (
	const NATIVE_CHAR       *string,
	NATIVE_CHAR             **terminator,
	NATIVE_UINT             base)
{
	u32                     converted = 0;
	u32                     index;
	u32                     sign;
	const NATIVE_CHAR       *string_start;
	NATIVE_UINT             return_value = 0;
	ACPI_STATUS             status = AE_OK;


	/*
	 * Save the value of the pointer to the buffer's first
	 * character, save the current errno value, and then
	 * skip over any white space in the buffer:
	 */
	string_start = string;
	while (IS_SPACE (*string) || *string == '\t') {
		++string;
	}

	/*
	 * The buffer may contain an optional plus or minus sign.
	 * If it does, then skip over it but remember what is was:
	 */
	if (*string == '-') {
		sign = NEGATIVE;
		++string;
	}

	else if (*string == '+') {
		++string;
		sign = POSITIVE;
	}

	else {
		sign = POSITIVE;
	}

	/*
	 * If the input parameter Base is zero, then we need to
	 * determine if it is octal, decimal, or hexadecimal:
	 */
	if (base == 0) {
		if (*string == '0') {
			if (acpi_cm_to_lower (*(++string)) == 'x') {
				base = 16;
				++string;
			}

			else {
				base = 8;
			}
		}

		else {
			base = 10;
		}
	}

	else if (base < 2 || base > 36) {
		/*
		 * The specified Base parameter is not in the domain of
		 * this function:
		 */
		goto done;
	}

	/*
	 * For octal and hexadecimal bases, skip over the leading
	 * 0 or 0x, if they are present.
	 */
	if (base == 8 && *string == '0') {
		string++;
	}

	if (base == 16 &&
		*string == '0' &&
		acpi_cm_to_lower (*(++string)) == 'x') {
		string++;
	}


	/*
	 * Main loop: convert the string to an unsigned long:
	 */
	while (*string) {
		if (IS_DIGIT (*string)) {
			index = *string - '0';
		}

		else {
			index = acpi_cm_to_upper (*string);
			if (IS_UPPER (index)) {
				index = index - 'A' + 10;
			}

			else {
				goto done;
			}
		}

		if (index >= base) {
			goto done;
		}

		/*
		 * Check to see if value is out of range:
		 */

		if (return_value > ((ACPI_UINT32_MAX - (u32) index) /
				   (u32) base)) {
			status = AE_ERROR;
			return_value = 0L;          /* reset */
		}

		else {
			return_value *= base;
			return_value += index;
			converted = 1;
		}

		++string;
	}

done:
	/*
	 * If appropriate, update the caller's pointer to the next
	 * unconverted character in the buffer.
	 */
	if (terminator) {
		if (converted == 0 && return_value == 0L && string != NULL) {
			*terminator = (NATIVE_CHAR *) string_start;
		}

		else {
			*terminator = (NATIVE_CHAR *) string;
		}
	}

	if (status == AE_ERROR) {
		return_value = ACPI_UINT32_MAX;
	}

	/*
	 * If a minus sign was present, then "the conversion is negated":
	 */
	if (sign == NEGATIVE) {
		return_value = (ACPI_UINT32_MAX - return_value) + 1;
	}

	return (return_value);
}

#endif /* ACPI_USE_SYSTEM_CLIBRARY */

