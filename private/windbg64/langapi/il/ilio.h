/* SCCSWHAT( "@(#)io.h	3.2 89/12/06 13:46:10	" ) */
/* 
 * In order to put out the data in a machine independent format, the
 * host machine must be considered.  We will use a scheme ralative
 * to the VAX and 8086/286.  THIS IS NOT THE SAME AS THE XENIX
 * CONVENTION.  The terms BYTESWAP and WORDSWAP mean that the
 * machine order has the byte/word order swapped from the 8086.
 * If the machine can access words/longs on odd byte boundries
 * it is BYTEADDRESSABLE.
 *
 * Since the order of bytes in the IL is "little-endian" ( the
 * number 0x04030201 is put put as 0x01 0x02 0x03 0x04), the reader
 * can be machine independent :
 * a long is (left to right order) getc + getc<<8 + getc<<16 +getc<<24,
 * etc.  However for speed, we will also parameterize the reads.
 *
 * We define some ordering macros :
 * 		BYTE0 is the LEAST significant byte (0x01 in the example)
 *		BYTE1 is the next most significant (0x02 in the example)
 *		BYTE2 is the next most significant (0x03 in the example)
 * 		BYTE3 is the MOST significant byte (0x04 in the example)
 */

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#if VERS_H286
#define BYTEADDRESSABLE	1
#endif

/************* ORDER DEFINITIONS **************/
#if WORDSWAP
#if BYTESWAP

/* 68000 for example */
#define LBYTE0 3
#define LBYTE1 2
#define LBYTE2 1
#define LBYTE3 0
#define SBYTE0 1
#define SBYTE1 0

#else

/* only WORDSWAPPED -- PDP-11 or old Xenix */
#define LBYTE0 2
#define LBYTE1 3
#define LBYTE2 0
#define LBYTE3 1
#define SBYTE0 0
#define SBYTE1 1

#endif /* BYTESWAP */
#else
#if BYTESWAP

/* only BYTESWAP */
#define LBYTE0 1
#define LBYTE1 0
#define LBYTE2 3
#define LBYTE3 2
#define SBYTE0 1
#define SBYTE1 0

#else

/* not WORDSWAPPED and not BYTESWAPPED  -- VAX and 8086/286 */
#define LBYTE0 0
#define LBYTE1 1
#define LBYTE2 2
#define LBYTE3 3
#define SBYTE0 0
#define SBYTE1 1

#endif /* BYTESWAP */
#endif /* WORDSWAP */
