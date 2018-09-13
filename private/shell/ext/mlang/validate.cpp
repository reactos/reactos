/*----------------------------------------------------------------------------
	%%File: validate.c
	%%Unit: fechmap
	%%Contact: jpick

	"Rolling" state machines that allow interactive verification of
    DBCS and EUC files.  Currently, separate tables are stored for
    each encoding so that the state machines can be run in parallel
    (i.e., multiple parse streams).

	These routines are used by auto-detection and if caller wants
    conversion routines to return errors on invalid characters.

    Following is a description of the structure of the DBCS and EUC 
    encodings handled by this module.  This information is taken from
    CJK.INF (maintained by Ken Lunde, author of _Understanding Japanese
    Information Processing_).  This information governs the structure
    of the class and validation state tables used in this module.

    Big5
      Two-byte Standard Characters         Encoding Ranges
          first byte range                     0xA1-0xFE
          second byte ranges                   0x40-0x7E, 0xA1-0xFE
      One-byte Characters                  Encoding Range
          ASCII                                0x21-0x7E
    
    GBK
      Two-byte Standard Characters         Encoding Ranges
          first byte range                     0x81-0xFE
          second byte ranges                   0x40-0x7E and 0x80-0xFE
      One-byte Characters                  Encoding Range
          ASCII                                0x21-0x7E

    HZ (information from HZ spec Fung F. Lee (lee@umunhum.stanford.edu))
	  One-byte characters					Encoding Ranges						   
      	first GB byte range						0x21-0x77
      	second GB byte range					0x21-0x7E
      	ASCII									0x21-0x7E
      Mode switching						Encoding sequence
		escape sequence from GB to ASCII		0x7E followed by 0x7B ("~{")
		escape sequence from ASCII to GB		0x7E followed by 0x7D ("~}")
		line continuation marker 				0x7E followed by 0x0A 
        (Note: ASCII mode is the default mode) 
    
    Shift-Jis
      Two-byte Standard Characters         Encoding Ranges
          first byte ranges                    0x81-0x9F, 0xE0-0xEF
          second byte ranges                   0x40-0x7E, 0x80-0xFC
      Two-byte User-defined Dharacters     Encoding Ranges
          first byte range                     0xF0-0xFC
          second byte ranges                   0x40-0x7E, 0x80-0xFC
      One-byte Characters                  Encoding Range
          Half-width katakana                  0xA1-0xDF
          ASCII/JIS-Roman                      0x21-0x7E
    
    Wansung
      Two-byte Standard Characters         Encoding Ranges
          first byte range                     0x81-0xFE
          second byte ranges                   0x40-0x7E and 0x80-0xFE
      One-byte Characters                  Encoding Range
          ASCII                                0x21-0x7E

    EUC-Cn
      Code set 0 (ASCII or GB 1988-89):        0x21-0x7E
      Code set 1 (GB 2312-80):                 0xA1A1-0xFEFE
      Code set 2:                              unused
      Code set 3:                              unused

    EUC-Jp
      Code set 0 (ASCII or JIS X 0201-1976 Roman):  0x21-0x7E
      Code set 1 (JIS X 0208):                 0xA1A1-0xFEFE
      Code set 2 (half-width katakana):        0x8EA1-0x8EDF
      Code set 3 (JIS X 0212-1990):            0x8FA1A1-0x8FFEFE

    EUC-Kr
      Code set 0 (ASCII or KS C 5636-1993):    0x21-0x7E
      Code set 1 (KS C 5601-1992):             0xA1A1-0xFEFE
      Code set 2:                              unused
      Code set 3:                              unused

    EUC-Tw
      Code set 0 (ASCII):                      0x21-0x7E
      Code set 1 (CNS 11643-1992 Plane 1):     0xA1A1-0xFEFE
      Code set 2 (CNS 11643-1992 Planes 1-16): 0x8EA1A1A1-0x8EB0FEFE
      Code set 3:                              unused

	UTF-7 (information from the RFC2152 by D.Goldsmith)
	  One-byte characters					Encoding Ranges						   
      	Direct and Optionally direct			0x21-0x2A, 0x2C-0x5B, 
      											0x5D-0x60, 0x7B-0x7D
      											0x09, 0x0A, 0x0D, 0x20
      	Modified Base64							0x2B, 0x2F-39, 0x41-0x5A, 0x61-0x7A
      Mode switching
      	escape sequence from D/O to M. Base64 	0x2B
      	escape sequence from M. Base64 to D/O 	0x2D (or any control character)
		
 ----------------------------------------------------------------------------*/
 
#include <stdio.h>
#include <stddef.h>

#include "private.h"
#include "fechmap_.h"
#include "lexint_.h"


/*----------------------------------------------------------------------------
	Common Defs for all Sequence Validation
----------------------------------------------------------------------------*/

// Characters are broken down into ranges -- the smallest ranges that
// are treated as important by either EUC or DBCS (all flavors).  In
// some cases, the smallest range is a single character.  It saves
// some space to avoid having two class tables (even though more states
// are added to the state machines), so both encodings share these
// tokens.

// Common Tokens
//
#define ollow       0		// "other" legal low ascii character
#define x000a       1		// 0x0a ("\n")
#define x212a       2		// characters in range 0x21-0x2a
#define x002b       3		// 0x2b	("+")
#define x002c       4		// 0x2c	(",")
#define x002d       5		// 0x2d	("-")
#define x002e       6       // 0x2e ("\")
#define x2f39       7		// characters in range 0x2f-0x39
#define x3a3f       8		// characters in range 0x3a-0x3f
#define x0040       9       // 0x40
#define x415a       10		// characters in range 0x41-0x5a
#define x005b       11		// 0x5b ("[")	
#define x005c       12		// 0x5c ("\")
#define x5d60       13		// characters in range 0x5d-0x60
#define x6177       14      // characters in range 0x61-0x77
#define	x787a       15		// characters in range 0x78-0x7a
#define x007b       16		// 0x7b ("{")
#define x007c       17		// 0x7c ("|")
#define x007d       18		// 0x7d ("}")
#define x007e       19		// 0x7e ("~")
#define x007f       20		// 0x7f (DEL)
#define x0080       21		// 0x80		
#define x818d       22		// characters in range 0x81-0x8d
#define x008e       23		// 0x8e
#define x008f       24		// 0x8f
#define x909f       25		// characters in range 0x90-0x9f
#define x00a0       26		// 0xa0
#define xa1b0       27		// characters in range 0xa1-0xb0
#define xb1df       28		// characters in range 0xb1-0xdf
#define xe0ef       29		// characters in range 0xe0-0xef
#define xf0fc       30		// characters in range 0xf0-0xfc
#define xfdfe       31		// characters in range 0xfd-0xfe

#define ateof       32		// end-of-file
#define other       33		// character not covered by above tokens

#define nTokens     34      //

// Class table
//
static char _rgchCharClass[256] =
//         0      1      2      3      4      5      6      7      8      9      a      b      c      d      e      f
    {
//  0      nul    soh    stx    etx    eot    enq    ack    bel    bs     tab    lf     vt     np     cr     so     si		0   
           other, other, other, other, other, other, other, other, other, ollow, x000a, other, other, ollow, other, other,

//  1      dle    dc1    dc2    dc3    dc4    nak    syn    etb    can    em     eof    esc    fs     gs     rs     us		1   
           other, other, other, other, other, other, other, other, other, other, ollow, other, other, other, other, other, 

//  2      sp     !      "      #      $      %      &      '      (      )      *      +      ,      -      .      /		2  
           ollow, x212a, x212a, x212a, x212a, x212a, x212a, x212a, x212a, x212a, x212a, x002b, x002c, x002d, x002e, x2f39, 

//  3      0      1      2      3      4      5      6      7      8      9      :      ;      <      =      >      ?		3  
           x2f39, x2f39, x2f39, x2f39, x2f39, x2f39, x2f39, x2f39, x2f39, x2f39, x3a3f, x3a3f, x3a3f, x3a3f, x3a3f, x3a3f, 

//  4      @      A      B      C      D      E      F      G      H      I      J      K      L      M      N      O		4  
           x0040, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, 

//  5      P      Q      R      S      T      U      V      W      X      Y      Z      [      \      ]      ^      _		5  
           x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x415a, x005b, x005c, x5d60, x5d60, x5d60, 

//  6      `      a      b      c      d      e      f      g      h      i      j      k      l      m      n      o		6  
           x5d60, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, 

//  7      p      q      r      s      t      u      v      w      x      y      z      {      |      }      ~      del		7  
           x6177, x6177, x6177, x6177, x6177, x6177, x6177, x6177, x787a, x787a, x787a, x007b, x007c, x007d, x007e, x007f, 

//	8                                                                                                                       8  
           x0080, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x818d, x008e, x008f, 

//	9                                                                                                                       9  
           x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, x909f, 

//	a                                                                                                                       a  
           x00a0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, xa1b0, 

//	b                                                                                                                       b  
           xa1b0, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, 

//	c                                                                                                                       c  
           xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, 

//	d                                                                                                                       d  
           xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, xb1df, 

//	e                                                                                                                       e  
           xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, xe0ef, 

//	f                                                                                                                       f  
           xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xf0fc, xfdfe, xfdfe, other, 

//         0      1      2      3      4      5      6      7      8      9      a      b      c      d      e      f
};


// Common States -- All SM's use these
//
#define ACC         0x4e
#define ERR         0x7f

// Other States -- All SM's use some of these, not all use all
//
#define ST0         0x00
#define ST0c        0x40
#define ST1         0x01
#define ST1c        0x41
#define ST2         0x02
#define ST2c        0x42
#define ST3         0x03
#define ST3c        0x43
#define ST4         0x04
#define ST4c        0x44

// Each state can have a corresponding counting stata i.e. stata with
// with the same transitions but during which we look for special sequences.
//
#define FTstCounting(tst)                   (((tst) & 0x40) != 0)   // If the state is counting (including ACC)
#define TstNotCountingFromTst(tst)          ((tst) & 0x3f)          // Obtain the real state from the counting

/*----------------------------------------------------------------------------
	DBCS character sequence validation
----------------------------------------------------------------------------*/

#define nSJisStates		2
static signed char _rgchSJisNextState[nSJisStates][nTokens] =
{
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     e     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//
                                                                                                                                        

// DBCS State 0 -- start (look for legal single byte or lead byte)
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ERR,  ACC,  ACC,  ST1,  ST1,  ERR,  ACC,  ERR,
	 
// DBCS State 1 -- saw lead byte, need legal trail byte
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ERR,
	 
};

#define nBig5States		2
static signed char _rgchBig5NextState[nBig5States][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// DBCS State 0 -- start (look for legal single byte or lead byte)
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ACC,  ERR,
	 
// DBCS State 1 -- saw lead byte, need legal trail byte
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,
	 
};

#define nGbkWanStates		2
static signed char _rgchGbkWanNextState[nGbkWanStates][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// DBCS State 0 -- start (look for legal single byte or lead byte)
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ST1,  ACC,  ERR,
	 
// DBCS State 1 -- saw lead byte, need legal trail byte
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,
     
	 
};
		

/*----------------------------------------------------------------------------
	EUC character sequence validation
----------------------------------------------------------------------------*/

#define nEucJpStates		4
static signed char _rgchEucJpNextState[nEucJpStates][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// EUC State 0 -- start
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ERR,  ST2,  ST3,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ACC,  ERR,
	 
// EUC State 1 -- saw a1fe, need one more a1fe
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,
	 
// EUC State 2 -- saw 8e, need a1df
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,
	 
// EUC State 3 -- saw 8f, need 2 a1fe
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ERR,  ERR,
	 
};

#define nEucKrCnStates		2
static signed char _rgchEucKrCnNextState[nEucKrCnStates][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// EUC State 0 -- start
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ACC,  ERR,
	 
// EUC State 1 -- saw a1fe, need one more a1fe
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,
	 
};

#define nEucTwStates		4
static signed char _rgchEucTwNextState[nEucTwStates][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// EUC State 0 -- start
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ERR,  ST2,  ERR,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ACC,  ERR,
	 
// EUC State 1 -- saw a1fe, need one more a1fe
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,
	 
// EUC State 2 -- saw 8e, need a1b0
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ST3,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,
	
// EUC State 3 -- saw 8e, a1b0; need 2 a1fe
    ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ST1,  ST1,  ST1,  ST1,  ST1,  ERR,  ERR,
	 
};

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	HZ character sequence validation
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// Currently some of the rules for HZ encoding outlined above are a bit loosened up.
// (e.g. the range for the first GB byte is expanded) The rules were adjusted based on real data. 

#define nHzStates		5
static signed char _rgchHzNextState[nHzStates][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// HZ State 0 -- ASCII
    ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ST1c, ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ERR,
	 
// HZ State 1 -- saw "~," looking for "{" to make transition to GB mode
    ERR,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ST2c, ERR,  ERR,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,
	 
// HZ State 2 -- just saw "{," expecting GB byte
    ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ERR,  ERR,  ERR,  ST4c, ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,
	
// HZ State 3 -- expecting GB byte
    ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST4c, ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,

// HZ State 4 -- saw "~," looking for "}" to make transition to ASCII mode
    ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ST3,  ACC,  ST3,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,

};

/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	UTF-7 character sequence validation
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define nUtf7States		3
static signed char _rgchUtf7NextState[nUtf7States][nTokens] =
{
//
//   o     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     x     a     o
//   l     0     2     0     0     0     0     2     3     0     4     0     0     5     6     7     0     0     0     0     0     0     8     0     0     9     0     a     b     e     f     f     t     t
//   l     0     1     0     0     0     0     f     a     0     1     0     0     d     1     8     0     0     0     0     0     0     1     0     0     0     0     1     1     0     0     d     e     h
//   o     0     2     2     2     2     2     3     3     4     5     5     5     6     7     7     7     7     7     7     7     8     8     8     8     9     a     b     d     e     f     f     o     e
//   w     a     a     b     c     d     e     9     f     0     a     b     c     0     7     a     b     c     d     e     f     0     d     e     f     f     0     0     f     f     c     e     f     r
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//

// UTF7 State 0 -- Direct/optionally direct ACSII mode, state transition can happen on "+"
    ACC,  ACC,  ACC,  ST1c, ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ACC,  ACC,  ACC,  ACC,  ACC,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ERR,
	 
// UTF7 State 1 -- Expecting first character from Modified Base64 alphabet
    ERR,  ERR,  ERR,  ST2,  ERR,  ACC,  ERR,  ST2,  ERR,  ERR,  ST2,  ERR,  ERR,  ERR,  ST2,  ST2,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,
	 
// UTF7 State 2 -- Modified Base64 alphabet mode, can be exited with "-" or any control character.
    ACC,  ACC,  ERR,  ST2,  ERR,  ACC,  ERR,  ST2,  ERR,  ERR,  ST2,  ERR,  ERR,  ERR,  ST2,  ST2,  ERR,  ERR,  ERR,  ERR,  ACC,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ACC,  ERR,
};

/*----------------------------------------------------------------------------
	UTF-8 character sequence validation
----------------------------------------------------------------------------*/

static int _nUtf8Tb = 0;

#define BIT7(a)           ((a) & 0x80)
#define BIT6(a)           ((a) & 0x40)

/* N  U T F  8 */
/*----------------------------------------------------------------------------
	%%Function: _NUtf8
	%%Contact: jpick

	UTF-8 doesn't require a state table for validation, just a count
	of the number of expected trail bytes.  See utf8lex.c for an 
	explanation of this code.
----------------------------------------------------------------------------*/
static int __inline NUtf8(UCHAR uch, BOOL fEoi)
{
	// BIT7(uch) == 0 implies single ASCII byte.
	// BIT6(uch) == 0 implies one of n trail bytes.
	// Otherwise, lead byte, with number of bits set
	//   up to first 0 equal to the total number bytes
	//   in the sequence.
	//
	// REVIEW: _nUtf8Tb *is* really the state of this
	//   validator -- use nState in structure?
	//
	if (fEoi && (_nUtf8Tb != 0))
		{
		return 0;				// unexpected end-of-input
		}
    else if (BIT7(uch) == 0)
        {
		if (_nUtf8Tb != 0)		// unexpected single byte
			return 0;
		return 1;
        }
    else if (BIT6(uch) == 0)
        {
		if (_nUtf8Tb == 0)		// unexpected trail byte
			return 0;
		if ((--_nUtf8Tb) == 0)
			return 1;
        }
    else
        {
		if (_nUtf8Tb != 0)		// unexpected lead byte
			return 0;
        while (BIT7(uch) != 0)
            {
            uch <<= 1;
            _nUtf8Tb++;
            }
        _nUtf8Tb--;				// don't count lead byte
        }
	return -1;
}


/*----------------------------------------------------------------------------
	Character Mapping Defs
----------------------------------------------------------------------------*/

// If caller wants us to check characters as part of validation
//
typedef BOOL (*PFNCHECKCHAR)(ICET icetIn);

#define cchMaxBuff		5
typedef struct _cc
{
	int nCp;						// code page
	int cchBuff;					// fill count of character buffer
	PFNCHECKCHAR pfnCheckChar;		// character check routine
	char rgchBuff[cchMaxBuff];		// character buffer
} CC;
	
// Character validation prototypes
//
static BOOL _FDbcsCheckChar(ICET icetIn);

	
// DBCS character checker structures
//

// Big5
static CC _ccBig5 =
{
	nCpTaiwan,
	0,
	_FDbcsCheckChar,
};

// Gbk
static CC _ccGbk =
{
	nCpChina,
	0,
	_FDbcsCheckChar,
};

// ShiftJis
static CC _ccSJis =
{
	nCpJapan,
	0,
	_FDbcsCheckChar,
};

// Wansung
static CC _ccWan =
{
	nCpKorea,
	0,
	_FDbcsCheckChar,
};


// Character checker structures just used as buffers.
//

// Euc-Jp
static CC _ccEucJp =
{
	0,
	0,
	0,
};

// Hz
static CC _ccHz =
{
	0,
	0,
	0,
};

// Utf7
static CC _ccUtf7 =
{
	0,
	0,
	0,
};

/*----------------------------------------------------------------------------
	Character Occurrence Counters
----------------------------------------------------------------------------*/

// If calling app wants us to track occurrences of common character
// sequences during validation (used only by auto-detection, so far).
//

typedef struct _coce
{
	int   cHits;
	short cwch;
	WCHAR rgwch[2];
} COCE;

typedef struct _coc
{
	BOOL  fMatching;
	short nCoceCurr;
	short nCoceIndex;
	int   ccoce;
	COCE *rgcoce;
} COC;
	
// Big5
//
static COCE _rgcoceBig5[] =
{
	{0, 2, {(WCHAR)0xa7da, (WCHAR)0xadcc},},			// "wo men"
	{0, 2, {(WCHAR)0xa8e4, (WCHAR)0xb9ea},},			// "qi shi"
	{0, 2, {(WCHAR)0xa65d, (WCHAR)0xacb0},},			// "yin wei"
	{0, 2, {(WCHAR)0xb8ea, (WCHAR)0xb054},},			// "zi xun"
	{0, 2, {(WCHAR)0xb971, (WCHAR)0xb8a3},},			// "diam nao"
	{0, 2, {(WCHAR)0xbaf4, (WCHAR)0xb8f4},},			// "wang lu"
	{0, 2, {(WCHAR)0xbd75, (WCHAR)0xa457},},			// "xian shang"
	{0, 2, {(WCHAR)0xc577, (WCHAR)0xaaef},},			// "huan ying"
	{0, 2, {(WCHAR)0xa477, (WCHAR)0xb867},},			// "yi jing"
};
		
static COC _cocBig5 =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceBig5) / sizeof(_rgcoceBig5[0]), 		// ccoce
	_rgcoceBig5,										// rgcoce
};

// Euc-Cn
//
static COCE _rgcoceEucCn[] =
{
	{0, 2, {(WCHAR)0xcbfb, (WCHAR)0xc3c7},},			// "ta men"
	{0, 2, {(WCHAR)0xced2, (WCHAR)0xc3c7},},			// "wo men"
	{0, 2, {(WCHAR)0xd2f2, (WCHAR)0xb4cb},},			// "yin ci"
	{0, 2, {(WCHAR)0xcab2, (WCHAR)0xc3b4},},			// "shen mo"
	{0, 2, {(WCHAR)0xc8e7, (WCHAR)0xb9fb},},			// "ru guo"
	{0, 2, {(WCHAR)0xd2f2, (WCHAR)0xceaa},},			// "yin wei"
	{0, 2, {(WCHAR)0xcbf9, (WCHAR)0xd2d4},},			// "suo yi"
	{0, 2, {(WCHAR)0xbbb6, (WCHAR)0xd3ad},},			// "huan ying"
	{0, 2, {(WCHAR)0xcdf8, (WCHAR)0xc2e7},},			// "wang luo"
	{0, 2, {(WCHAR)0xd0c5, (WCHAR)0xcfa2},},			// "xin xi"
	{0, 2, {(WCHAR)0xbcc6, (WCHAR)0xcbe3},},			// "ji guan"
};
		
static COC _cocEucCn =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceEucCn) / sizeof(_rgcoceEucCn[0]), 	// ccoce
	_rgcoceEucCn,										// rgcoce
};
	
// Euc-Kr
//
static COCE _rgcoceEucKr[] =
{
	{0, 2, {(WCHAR)0xb0a1, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xb0a1, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xb4c2, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xb4c2, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xb4d9, (WCHAR)0x002e},},
	{0, 2, {(WCHAR)0xb4d9, (WCHAR)0xa3ae},},
	{0, 2, {(WCHAR)0xb8a6, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xb8a6, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xc0ba, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xc0ba, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xc0bb, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xc0bb, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xc0cc, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xc0cc, (WCHAR)0xa1a1},},
};
		
static COC _cocEucKr =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceEucKr) / sizeof(_rgcoceEucKr[0]), 	// ccoce
	_rgcoceEucKr,										// rgcoce
};
	
// EUC-Jp
//
static COCE _rgcoceEucJp[] =
{
	{0, 2, {(WCHAR)0xa4c7, (WCHAR)0xa4b9},},			// "de su"
	{0, 2, {(WCHAR)0xa4c0, (WCHAR)0xa1a3},},			// "da ."
	{0, 2, {(WCHAR)0xa4a4, (WCHAR)0xa4eb},},			// "i ru"
	{0, 2, {(WCHAR)0xa4de, (WCHAR)0xa4b9},},			// "ma su"
	{0, 2, {(WCHAR)0xa4b7, (WCHAR)0xa4bf},},			// "shi ta"
	{0, 2, {(WCHAR)0xa4b9, (WCHAR)0xa4eb},},			// "su ru"
	{0, 2, {(WCHAR)0xa4bf, (WCHAR)0xa1a3},},			// "ta ."
	{0, 2, {(WCHAR)0xa4eb, (WCHAR)0xa1a3},},			// "ru ."
};
		
static COC _cocEucJp =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceEucJp) / sizeof(_rgcoceEucJp[0]), 	// ccoce
	_rgcoceEucJp,										// rgcoce
};

// GBK
//
static COCE _rgcoceGbk[] =
{
	{0, 2, {(WCHAR)0xcbfb, (WCHAR)0xc3c7},},			// "ta men"
	{0, 2, {(WCHAR)0xced2, (WCHAR)0xc3c7},},			// "wo men"
	{0, 2, {(WCHAR)0xd2f2, (WCHAR)0xb4cb},},			// "yin ci"
	{0, 2, {(WCHAR)0xcab2, (WCHAR)0xc3b4},},			// "shen mo"
	{0, 2, {(WCHAR)0xc8e7, (WCHAR)0xb9fb},},			// "ru guo"
	{0, 2, {(WCHAR)0xd2f2, (WCHAR)0xceaa},},			// "yin wei"
	{0, 2, {(WCHAR)0xcbf9, (WCHAR)0xd2d4},},			// "suo yi"
	{0, 2, {(WCHAR)0xbbb6, (WCHAR)0xd3ad},},			// "huan ying"
	{0, 2, {(WCHAR)0xcdf8, (WCHAR)0xc2e7},},			// "wang luo"
	{0, 2, {(WCHAR)0xd0c5, (WCHAR)0xcfa2},},			// "xin xi"
	{0, 2, {(WCHAR)0xbcc6, (WCHAR)0xcbe3},},			// "ji guan"
};
		
static COC _cocGbk =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceGbk) / sizeof(_rgcoceGbk[0]), 		// ccoce
	_rgcoceGbk,											// rgcoce
};
	
// Shift-JIS
//
static COCE _rgcoceSJis[] =
{
	{0, 2, {(WCHAR)0x82c5, (WCHAR)0x82b7},},			// "de su"
	{0, 2, {(WCHAR)0x82be, (WCHAR)0x8142},},			// "da ."
	{0, 2, {(WCHAR)0x82a2, (WCHAR)0x82e9},},			// "i ru"
	{0, 2, {(WCHAR)0x82dc, (WCHAR)0x82b7},},			// "ma su"
	{0, 2, {(WCHAR)0x82b5, (WCHAR)0x82bd},},			// "shi ta"
	{0, 2, {(WCHAR)0x82b7, (WCHAR)0x82e9},},			// "su ru"
	{0, 2, {(WCHAR)0x82bd, (WCHAR)0x8142},},			// "ta ."
	{0, 2, {(WCHAR)0x82e9, (WCHAR)0x8142},},			// "ru ."
};

static COC _cocSJis =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceSJis) / sizeof(_rgcoceSJis[0]), 		// ccoce
	_rgcoceSJis,										// rgcoce
};
	
// Wansung
//
// REVIEW: bug (1/2 this table is being ignored)
//
static COCE _rgcoceWan[] =
{
	{0, 2, {(WCHAR)0xb0a1, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xb0a1, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xb4c2, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xb4c2, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xb4d9, (WCHAR)0x002e},},
	{0, 2, {(WCHAR)0xb4d9, (WCHAR)0xa3ae},},
	{0, 2, {(WCHAR)0xb8a6, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xb8a6, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xc0ba, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xc0ba, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xc0bb, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xc0bb, (WCHAR)0xa1a1},},
	{0, 2, {(WCHAR)0xc0cc, (WCHAR)0x0020},},
	{0, 2, {(WCHAR)0xc0cc, (WCHAR)0xa1a1},},
};

static COC _cocWan =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceWan) / sizeof(_rgcoceWan[0]), 		// ccoce
	_rgcoceWan,											// rgcoce
};

// Hz
//
static COCE _rgcoceHz[] =
{
	{0, 2, {(WCHAR)0x007e, (WCHAR)0x007b},},			// ~{
	{0, 2, {(WCHAR)0x007e, (WCHAR)0x007d},},            //  ~}
};

static COC _cocHz =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceHz) / sizeof(_rgcoceHz[0]), 		    // ccoce
	_rgcoceHz,											// rgcoce
};

// Utf7
//
static COCE _rgcoceUtf7[] =
{
	{0, 2, {(WCHAR)0x002b, (WCHAR)0x002d},},			// +-
};

static COC _cocUtf7 =
{
	fFalse,												// fMatching
	0,													// nCoceCurr
	0,													// nCoceIndex
	sizeof(_rgcoceUtf7) / sizeof(_rgcoceUtf7[0]), 		// ccoce
	_rgcoceUtf7,										// rgcoce
};
	
// Character counter prototype.
//
static void _CountChars(ICET icetIn);


/*----------------------------------------------------------------------------
	Main Definitions
----------------------------------------------------------------------------*/

// Structure to keep state, state machine and other associated
// information for a given character set "parse stream."
//
typedef struct _vr
{
	BOOL  fInUse;
	DWORD dwFlags;
	int   nState;
	CC   *ccCheck;
	signed char (*rgchNextState)[nTokens];
} VR;

// Array of validation records.  We allow multiple, active parse
// streams for auto-detect -- this way, it can concurrently keep
// a parse stream for each encoding type, without needing to read
// its input multiple times.
//
static VR _mpicetvr[icetCount] =
{
	{fTrue,  0, ST0, 0,         _rgchEucKrCnNextState,},		// icetEucCn
	{fTrue,  0, ST0, &_ccEucJp, _rgchEucJpNextState,},			// icetEucJp
	{fTrue,  0, ST0, 0,         _rgchEucKrCnNextState,},		// icetEucKr
	{fTrue,  0, ST0, 0,         _rgchEucTwNextState,},			// icetEucTw
	{fFalse, 0, ST0, 0,         0,},							// icetIso2022Cn
	{fFalse, 0, ST0, 0,         0,},							// icetIso2022Jp
	{fFalse, 0, ST0, 0,         0,},							// icetIso2022Kr
	{fFalse, 0, ST0, 0,         0,},							// icetIso2022Tw
	{fTrue,  0, ST0, &_ccBig5,  _rgchBig5NextState,},			// icetBig5
	{fTrue,  0, ST0, &_ccGbk,   _rgchGbkWanNextState,},			// icetGbk
	{fTrue,  0, ST0, &_ccHz,    _rgchHzNextState,},             // icetHz
	{fTrue,  0, ST0, &_ccSJis,  _rgchSJisNextState,},			// icetShiftJis
	{fTrue,  0, ST0, &_ccWan,   _rgchGbkWanNextState,},			// icetWansung
	{fTrue,  0, ST0, &_ccUtf7,  _rgchUtf7NextState,},           // icetUtf7
	{fTrue,  0, ST0, 0,        0,},								// icetUtf8
};

// Array of character sequence counters, one per encoding type.
//
static COC *_mpicetlpcoc[icetCount] =
{
	&_cocEucCn,			// icetEucCn
	&_cocEucJp,			// icetEucJp
	&_cocEucKr,			// icetEucKr
	0,					// icetEucTw
	0,					// icetIso2022Cn
	0,					// icetIso2022Jp
	0,					// icetIso2022Kr
	0,					// icetIso2022Tw
	&_cocBig5,			// icetBig5
	&_cocGbk,			// icetGbk
	&_cocHz,            // icetHz
	&_cocSJis,			// icetShiftJis
	&_cocWan,			// icetWansung
	&_cocUtf7,          // icetUtf7
	0,					// icetUtf8
};


/* V A L I D A T E  I N I T */
/*----------------------------------------------------------------------------
	%%Function: ValidateInit
	%%Contact: jpick

	Initialize the state machine for the given character set (set its
	state to ST0 (the start state) and store its parsing options).
----------------------------------------------------------------------------*/
void ValidateInit(ICET icetIn, DWORD dwFlags)
{
	// Initialize the character occurrence counter, if caller wants
	// us to count common character sequences (auto-detect, only,
	// for now).  Turn off the count-common-chars flag if we're not
	// set up to count sequences (meaning we don't have a set of
	// common characters for this encoding type or have no place
	// to buffer them).
	//	
	if (dwFlags & grfCountCommonChars)
		{
		if ((_mpicetlpcoc[icetIn]) && (_mpicetvr[icetIn].ccCheck))
			{
			int i;
			for (i = 0; i < _mpicetlpcoc[icetIn]->ccoce; i++)
				_mpicetlpcoc[icetIn]->rgcoce[i].cHits = 0;
			_mpicetlpcoc[icetIn]->fMatching = fFalse;
			}
		else
			{
			dwFlags &= ~grfCountCommonChars;
			}
		}
		
	// If validation not supported for the encoding type, there's
	// nothing else for us to do here.
	//
	if (!_mpicetvr[icetIn].fInUse)
		return;
		
	_mpicetvr[icetIn].nState = ST0;
	
	// Can't do character mapping validation without character 
	// checker information.  (If we do have the character checker,
	// initialize its buffer length to 0).
	//
	if (_mpicetvr[icetIn].ccCheck)
		_mpicetvr[icetIn].ccCheck->cchBuff = 0;
	else
		dwFlags &= ~grfValidateCharMapping;
		
	// It's also impossible without a valid code page.
	//
	if ((dwFlags & grfValidateCharMapping) && !IsValidCodePage(_mpicetvr[icetIn].ccCheck->nCp))
		dwFlags &= ~grfValidateCharMapping;
	
	_mpicetvr[icetIn].dwFlags = dwFlags;
	
	if (icetIn == icetUtf8)
		_nUtf8Tb = 0;
}


/* V A L I D A T E  R E S E T  A L L*/
/*----------------------------------------------------------------------------
	%%Function: ValidateInitAll
	%%Contact: jpick

	Initialize the state machines for all character sets (set their
	states to ST0 (the start state) and store their parsing options).
----------------------------------------------------------------------------*/
void ValidateInitAll(DWORD dwFlags)
{
	int i;
	for (i = 0 ; i < icetCount; i++)
		{
		if (!_mpicetvr[i].fInUse)
			continue;
		ValidateInit((ICET)i, dwFlags);	
		}
}


/* V A L I D A T E  R E S E T */
/*----------------------------------------------------------------------------
	%%Function: ValidateReset
	%%Contact: jpick

	Reset the state machine for the given character set (set its state
	to ST0 (the start state)).
----------------------------------------------------------------------------*/
void ValidateReset(ICET icetIn)
{
	// Initialize the character occurrence counter, if caller wants
	// us to count common character sequences (auto-detect, only,
	// for now).  We're guaranteed to have the structures if the
	// flag is set by ValidateInit(), above.
	//	
	if (_mpicetvr[icetIn].dwFlags & grfCountCommonChars)
		{
		int i;
		for (i = 0; i < _mpicetlpcoc[icetIn]->ccoce; i++)
			_mpicetlpcoc[icetIn]->rgcoce[i].cHits = 0;
		_mpicetlpcoc[icetIn]->fMatching = fFalse;
		}
		
	// If validation not supported for the encoding type, there's
	// nothing else for us to do here.
	//
	if (!_mpicetvr[icetIn].fInUse)
		return;
		
	_mpicetvr[icetIn].nState = ST0;
	
	if (_mpicetvr[icetIn].ccCheck)
		_mpicetvr[icetIn].ccCheck->cchBuff = 0;
		
	if (icetIn == icetUtf8)
		_nUtf8Tb = 0;
}


/* V A L I D A T E  R E S E T  A L L */
/*----------------------------------------------------------------------------
	%%Function: ValidateResetAll
	%%Contact: jpick

	Reset the state machines for all character sets (set their states to
	ST0 (the start state)).
----------------------------------------------------------------------------*/
void ValidateResetAll(void)
{
	int i;
	
	for (i=0 ; i < icetCount; i++)
		{
		if (!_mpicetvr[i].fInUse)
			continue;
		ValidateReset((ICET)i);
		}
}


/* N  V A L I D A T E  U C H */
/*----------------------------------------------------------------------------
	%%Function: NValidateUch
	%%Contact: jpick

	Single step parser, takes one transition through the state table
	for the given character set.  Current state is kept for each
	character set's parse stream.
	
	Routine returns -1 if it does not reach a final state on this
	transition; 0 if transitioned to ERR(or) and 1 if transtioned
	to ACC(ept).
	
	If final state is ACC(ept), machine reset to ST0 (start state).
	(i.e., there's no need to manually reset on ACC(ept)).
	
	Routine is also a convenient collection point for certain
	statistics (currently only the counting of occurrences of common
	character sequences (defined for character sets, above)).
----------------------------------------------------------------------------*/
int NValidateUch(ICET icetIn, UCHAR uch, BOOL fEoi)
{
	int nToken;
	int nPrevState;
	int rc = -1;
	
	// If not validating this icet, nothing to do (so say 
	// we accept the character).
	//
	if (!_mpicetvr[icetIn].fInUse)
		return 1;
	if (_mpicetvr[icetIn].nState == ERR)
		return 0;

	// Ignore all zeros in the detection file.
	if (!uch && !fEoi)
    	{
            goto _LRet;
        }

	// Hack -- want to validate UTF-8, but don't need a state
	// table to do so.  Treat as special case here and return.
	//
	if (icetIn == icetUtf8)
		{
		if ((rc = NUtf8(uch, fEoi)) == 0)
			_mpicetvr[icetIn].nState = ERR;
		return rc;
		}
		
	// Classify the character...
	//
	nPrevState = _mpicetvr[icetIn].nState;
	nToken = fEoi ? ateof : _rgchCharClass[uch];
	
	// First obtain a real number for a state based on the counting state...
	// Then do the transition...
	//
	_mpicetvr[icetIn].nState = (_mpicetvr[icetIn].rgchNextState)[TstNotCountingFromTst(_mpicetvr[icetIn].nState)][nToken];

#if 0
	if (_mpicetvr[icetIn].nState == ERR) 
		printf("Character 0x%.2x; Going from state %.2x to state %.2x\n", uch, nPrevState, _mpicetvr[icetIn].nState);
#endif

	// If we're in an error state or have seen end-of-input, return.
	//
	if ((_mpicetvr[icetIn].nState == ERR) || (nToken == ateof))
		goto _LRet;
	
	// Are we to do character mapping validation?  (If this flag
	// is set, we're guaranteed to have a character checker 
	// structure).  How about character occurrence counting?
	// (This also guarantees us a character checker structure).
	//
	if (!(_mpicetvr[icetIn].dwFlags & grfValidateCharMapping) &&
			!(_mpicetvr[icetIn].dwFlags & grfCountCommonChars))
		{
		goto _LRet;
		}
			
	// Buffer the current character (trusting that we'll never get
	// more than the max amount -- present tables enforce this)
	// (if it's Utf7 or Hz, buffer only if we are in the counting state
	//
	if (FTstCounting(_mpicetvr[icetIn].nState) || (icetIn != icetHz && icetIn != icetUtf7)) 
		_mpicetvr[icetIn].ccCheck->rgchBuff[_mpicetvr[icetIn].ccCheck->cchBuff++] = uch;

	// Return if we are not in the counting state
	//
	if (!(FTstCounting(_mpicetvr[icetIn].nState)))
		goto _LRet;
		
	// Call the character checker, if we have one.
	//
	if (_mpicetvr[icetIn].dwFlags & grfValidateCharMapping)
		{
		if (_mpicetvr[icetIn].ccCheck->pfnCheckChar && !(_mpicetvr[icetIn].ccCheck->pfnCheckChar)(icetIn))
			{
			_mpicetvr[icetIn].nState = ERR;
			goto _LRet;
			}
		}
		
	// If we're counting common characters, do so now.
	//
	if (_mpicetvr[icetIn].dwFlags & grfCountCommonChars)
		_CountChars(icetIn);
	
	// Reset the character checker/counter buffer.
	//
	_mpicetvr[icetIn].ccCheck->cchBuff = 0;
	
_LRet:

	// Return the appropriate code.
	//
	switch (_mpicetvr[icetIn].nState)
		{
		case ERR:
			return 0;
		case ACC:
			_mpicetvr[icetIn].nState = ST0;			// Reset
			return 1;
		default:
			return -1;								// need more data
		}
}


/* F  V A L I D A T E  C H A R  C O U N T */
/*----------------------------------------------------------------------------
	%%Function: FValidateCharCount
	%%Contact: jpick

	Return the number of matched special character sequences for the
	given character set.  If we're not keeping track of these sequences
	for the character set, either because we don't have the necessary
	static data or because the flag wasn't set by the calling routine,
	return fFalse.  Otherwise, return the count in *lpcMatch and return
	fTrue;
	
	(We track the counts separately for each sequence, just in case
	we want to weight them differently in the future.  Return the
	total, here).
----------------------------------------------------------------------------*/
BOOL FValidateCharCount(ICET icetIn, int *lpcMatch)
{
	int i;
	COC *lpcoc = _mpicetlpcoc[icetIn];
	VR *lpvr = &_mpicetvr[icetIn];
		
	if (!lpcoc || !lpvr->fInUse || !(lpvr->dwFlags & grfCountCommonChars))
		return fFalse;
		
	for (i = 0, *lpcMatch = 0; i < lpcoc->ccoce; i++)
		*lpcMatch += lpcoc->rgcoce[i].cHits;
		
	return fTrue;
}


/* _  C O U N T  C H A R S */
/*----------------------------------------------------------------------------
	%%Function: _CountChars
	%%Contact: jpick

	We've just completed a legal character for the given character
	set.  Match it against the set of special character sequences for
	the character set, if we have them.  Update match counts and
	current match indices (since sequences can span multiple legal
	characters) as needed.
----------------------------------------------------------------------------*/
static void _CountChars(ICET icetIn)
{
	WCHAR wch;
	int i;
	BOOL fFound;
	
	// Anything to do?
	//
	if (!_mpicetlpcoc[icetIn] || !_mpicetvr[icetIn].ccCheck)
		return;
		
	// Build the WCHAR.
	//
	switch (_mpicetvr[icetIn].ccCheck->cchBuff)
		{
		case 1:
			wch = WchFromUchUch(0, _mpicetvr[icetIn].ccCheck->rgchBuff[0]);
			break;
		case 2:
			wch = WchFromUchUch(_mpicetvr[icetIn].ccCheck->rgchBuff[0],
								_mpicetvr[icetIn].ccCheck->rgchBuff[1]);
			break;
		case 3:
			wch = WchFromUchUch(_mpicetvr[icetIn].ccCheck->rgchBuff[1],
								_mpicetvr[icetIn].ccCheck->rgchBuff[2]);
			break;
		case 4:
			wch = WchFromUchUch(_mpicetvr[icetIn].ccCheck->rgchBuff[2],
								_mpicetvr[icetIn].ccCheck->rgchBuff[3]);
			break;
		default:
			return;
		}
		
	// Are we currently working on matching a sequence?
	//
	if ((_mpicetlpcoc[icetIn]->fMatching) && 
		(wch == _mpicetlpcoc[icetIn]->rgcoce[_mpicetlpcoc[icetIn]->nCoceCurr].rgwch[_mpicetlpcoc[icetIn]->nCoceIndex]))
		{
		// Did we just match the entire sequence?  If so, increment the
		// hit count and reset.
		//
		if (++_mpicetlpcoc[icetIn]->nCoceIndex >= _mpicetlpcoc[icetIn]->rgcoce[_mpicetlpcoc[icetIn]->nCoceCurr].cwch)
			{
			++_mpicetlpcoc[icetIn]->rgcoce[_mpicetlpcoc[icetIn]->nCoceCurr].cHits;
			_mpicetlpcoc[icetIn]->fMatching = fFalse;
			}
			
		// All done.
		//
		return;
		}
		
	// If we need to start matching again (either because we're not
	// currently in a sequence or because a 2nd or later character
	// didn't match), try the current character as a lead character.
	//
	// REVIEW: wrong for sequences longer than 2 wchars.
	//
	for (i = 0, fFound = fFalse; (!fFound && (i < _mpicetlpcoc[icetIn]->ccoce)); i++)
		{
		if (wch == _mpicetlpcoc[icetIn]->rgcoce[i].rgwch[0])
			fFound = fTrue;
		}
		
	// Any luck?
	//
	if (!fFound)
		{
		_mpicetlpcoc[icetIn]->fMatching = fFalse;
		return;
		}
		
	// Store the matching state.
	//
	_mpicetlpcoc[icetIn]->fMatching = fTrue;
	_mpicetlpcoc[icetIn]->nCoceCurr = i - 1;
	_mpicetlpcoc[icetIn]->nCoceIndex = 1;			// where to look next
}


/* _  D B C S  C H E C K  C H A R */
/*----------------------------------------------------------------------------
	%%Function: _DbcsCheckChar
	%%Contact: jpick

	Character validator for DBCS formats.  Attempts to round-trip a
	legal multi-byte sequence to ensure that its valid for the given
	character set.
	
	REVIEW:  Slow, slow, slow -- do we really gain anything from the
    round-trip check, or is conversion *to* Unicode a sufficient test?
----------------------------------------------------------------------------*/
static WCHAR _rgwBuff[10];
static UCHAR _rgchBuff[30];

static BOOL _FDbcsCheckChar(ICET icetIn)
{
	int cCvt;
	
	// skip 1 byte characters, mostly uninteresting (Shift-Jis ??).
	//
	if (_mpicetvr[icetIn].ccCheck->cchBuff == 1)
		return fTrue;
	
	if (!(cCvt = MultiByteToWideChar(_mpicetvr[icetIn].ccCheck->nCp,
									 MB_ERR_INVALID_CHARS,
									 _mpicetvr[icetIn].ccCheck->rgchBuff,
									 _mpicetvr[icetIn].ccCheck->cchBuff,
									 _rgwBuff, 10)))
		{
		if (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
			return fFalse;
		}
		
	return fTrue;  // probably not always right
}
